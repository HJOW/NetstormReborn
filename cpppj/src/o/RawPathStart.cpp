// 경로 생성/필드 쓰기와 통지 순서는 실제 원본과 같다. 다른 이동 파생 클래스는 명시 경계로 남긴다.
#include "o/RawPathStart.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kMaximumDestination=254.0F,kHalfCell=0.5F,kCellBias=0.9999F; // 실제 목적지 clamp·스냅·기존 경로 시작 보정값이다.
constexpr std::uint32_t kFailureGenus=0x8c044200,kTeleportProcess=23,kPriestProcess=54; // 무효 경로 정리 genus와 실제 복원된 두 경로 클래스다.
// 정렬되지 않은 원본 raw DWORD를 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result=0;
    // 주변 raw 필드를 합치지 않고 지정4바이트만 조합한다.
    for (std::size_t i=0;i<4;++i) result|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return result;
}
}
// 새 객체 생성/기존 삭제 이전에 명시적 같은 월드/필수 효과 계약을 확인한다.
RawPathStart::RawPathStart(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,SquidProcessHost& host,
    const RawContainedLookup& lookup,const SquidFrame& frame,RawPathStartState state,RawPathStartHooks hooks)
    :pool_(pool),hash_(hash),types_(types),host_(host),lookup_(lookup),frame_(frame),state_(state),hooks_(std::move(hooks)) {
    if (&host.Pool()!=&pool || &lookup.Pool()!=&pool || &frame.Pool()!=&pool || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        !hooks_.processType || !hooks_.create || !hooks_.metadata || !hooks_.clearBlocked) throw std::invalid_argument("Path 시작 월드/필수 효과 연결 누락");
}
// 같은 raw 위치를 읽어 callback 뒤의 현재 좌표를 사용한다.
RawPathPoint RawPathStart::Position(Sid sid) const {
    const auto bytes=pool_.Slot(sid);return {std::bit_cast<float>(Read(bytes,14)),std::bit_cast<float>(Read(bytes,18))};
}
// 원본0041d7a0/CD00440a80의 float32에 double bias를 더하고0쪽 절삭한 float 좌표다.
RawPathPoint RawPathStart::CeilPoint(RawPathPoint point) {
    if (!std::isfinite(point.x) || !std::isfinite(point.y)) throw std::out_of_range("Path 시작 비정상 좌표");
    return {static_cast<float>(std::trunc(static_cast<double>(point.x)+kCellBias)),static_cast<float>(std::trunc(static_cast<double>(point.y)+kCellBias))};
}
// 원본004ade70→finder 생성자는 발자국 왼쪽/위는 절삭하고 오른쪽/아래는 거의 올림한다.
SquidSearchArea RawPathStart::Footprint(Sid sid) const {
    const auto point=Position(sid);const auto type=pool_.Slot(sid)[10];const auto& record=types_[type];
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || record.footX<1 || record.footY<1) throw std::out_of_range("Path 시작 부모 발자국");
    const float left=std::clamp(point.x-static_cast<float>(record.footX-1),1.0F,255.0F),top=std::clamp(point.y-static_cast<float>(record.footY-1),1.0F,255.0F);
    const auto end=CeilPoint(point);
    return {static_cast<int>(left),static_cast<int>(top),static_cast<int>(end.x),static_cast<int>(end.y)};
}
// 목적지 clamp→기존 경로/시작 보정→생성/Init→명령 필드→유효 검사→Prepare/Attach→부모통지다.
RawPathProcess* RawPathStart::Start(RawPathStartRequest request) {
    if (!request.parent.value || request.parent.value>=pool_.Capacity() || !std::isfinite(request.destination.x) || !std::isfinite(request.destination.y))
        throw std::out_of_range("Path 시작 입력 범위");
    request.destination.x=std::clamp(request.destination.x,0.0F,kMaximumDestination);request.destination.y=std::clamp(request.destination.y,0.0F,kMaximumDestination);
    const auto parentType=pool_.Slot(request.parent)[10];const auto processType=hooks_.processType(parentType);
    const std::uint32_t flags=(parentType==state_.dudeType || parentType==state_.manaBoltType || parentType==state_.teleportType) ? 0x10U : 0U;
    if (!flags && state_.snapPriestDestination && parentType==state_.priestType) {
        request.destination=CeilPoint(request.destination);request.destination.x-=kHalfCell;request.destination.y-=kHalfCell;
    }
    auto start=Position(request.parent);std::uint32_t first=0,second=0;
    if (state_.clock.boss) {
        const auto oldForm=lookup_.Find(request.parent,processType);
        if (oldForm.value) {
            auto* old=dynamic_cast<RawPathProcess*>(host_.FromForm(oldForm));
            if (!old || &old->Pool()!=&pool_) throw std::logic_error("기존 다른 파생 Path 수명 미복원 경계");
            if (pool_.Edition()==OriginalEdition::Patch1078) {
                if (start.x<=request.destination.x) start.x-=kHalfCell;
                if (start.y<=request.destination.y) start.y-=kHalfCell;
            }
            first=old->Runtime().TargetA();second=old->Runtime().TargetB();host_.Kill(*old,0);
        }
    }
    constexpr std::uint32_t kFlyProcess=39; // 원본0054110c의 실제 비행 경로 생성자다.
    constexpr std::uint32_t kWormProcess=40; // 원본00541110의 실제 썬 워커/empty 경로다.
    constexpr std::uint32_t kBalloonProcess=42; //00541118의 실제 이륙/착륙 경로다.
    constexpr std::uint32_t kDudeProcess=24; //005410d0의 실제 로컬 Dude 보행 경로다.
    if (processType!=kTeleportProcess && processType!=kPriestProcess && processType!=kFlyProcess && processType!=kWormProcess && processType!=kBalloonProcess && processType!=kDudeProcess) throw std::logic_error("다른 파생 이동 생성/Init 미복원 경계");
    if ((processType==kFlyProcess || processType==kWormProcess || processType==kBalloonProcess || processType==kDudeProcess) && !hooks_.createType) throw std::logic_error("파생 이동의 실제 생성자 미연결");
    auto owned=hooks_.createType ? hooks_.createType(processType) : hooks_.create(processType==kPriestProcess);
    if (!owned) throw std::bad_alloc();
    if (&owned->Pool()!=&pool_) throw std::invalid_argument("Path 시작 생성 풀 불일치");
    if (owned->MovementType()!=processType) throw std::invalid_argument("Path 시작의 실제 생성 종류 불일치");
    owned->InitializePath(request.parent,request.destination,start,0);
    auto& runtime=owned->Runtime();runtime.SetTargets(first,second);runtime.SetFinished(false);
    runtime.SetCommandFields(request.helper,request.tailWord,request.tailByte,request.destination);
    if (runtime.Count()<1) {
        bool cleared=false;
        if (pool_.Edition()==OriginalEdition::Patch1078) {
            RawSquidFinder finder(pool_,hash_,types_);auto candidate=finder.Begin(Footprint(request.parent),1);
            // 실제 finder의 현재 후보 순서로 해당 genus마다 현재 부모 위치/타입/소유자를 다시 읽는다.
            while (candidate.value) {
                if (types_[pool_.Slot(candidate)[10]].flags2&kFailureGenus) {
                    const auto current=pool_.Slot(request.parent);hooks_.clearBlocked(Position(request.parent),current[10],current[34]);cleared=true;
                }
                candidate=finder.Next();
            }
        }
        owned.reset();
        if (pool_.Edition()==OriginalEdition::Cd1072) return nullptr;
        if (state_.retrying) { state_.retrying=false;return nullptr; }
        if (cleared) { state_.retrying=true;(void)Start(request); }
        return nullptr;
    }
    if (request.replaceStart) {
        const auto current=Position(request.parent),rounded=CeilPoint(current);
        // 시작의 연속 동일 꼭짓점만 현재 부모 좌표로 교체하고 시간/방향은 보존한다.
        for (std::int32_t i=0;i<runtime.Count();++i) {
            const auto point=runtime.Point(i);if (point.x!=rounded.x || point.y!=rounded.y) break;runtime.ReplacePoint(i,start);
        }
    }
    const auto meta=hooks_.metadata(request.parent);owned->Prepare(0,meta.speed,meta.turningSpeed);
    auto* result=RawPathProcess::Attach(host_,std::move(owned),request.parent,flags);
    if (!result) throw std::logic_error("Path 시작 중 부모 소멸 계약 미복원 경계");
    NotifyStarted(request.parent,request.destination,*result);return result;
}
// 지원된 원본 부모의 두 가상 몸체를 실제 같은 공간/Host/Frame로 실행한다.
void RawPathStart::NotifyStarted(Sid parent,RawPathPoint destination,RawPathProcess& process) {
    const auto type=pool_.Slot(parent)[10];
    if (type==state_.teleportType || type==state_.manaBoltType || type==state_.dudeType) return; // 세 실제 부모의+44/+40은480260/4437f0 빈 몸체다.
    if (type!=state_.priestType && process.MovementType()!=40 && process.MovementType()!=42) throw std::logic_error("다른 부모 이동 통지 미복원 경계");
    const int x=static_cast<int>(destination.x),y=static_cast<int>(destination.y);RawSquidFinder finder(pool_,hash_,types_);auto candidate=finder.Begin({x,y,x,y},1);
    // 원본은 첫 일치에서 멈추지 않고 마지막 간헐천을targetA에 기록한다.
    while (candidate.value) { if (pool_.Slot(candidate)[10]==state_.geyserType) process.Runtime().SetTargets(candidate.value,process.Runtime().TargetB());candidate=finder.Next(); }
    if (process.MovementType()==40 || process.MovementType()==42) return; //두 Carrier의427750 공통+44 뒤 +40은4437f0이다.
    // 두 사건 각각을 처음부터 다시 검색하므로 삭제 callback이 추가한 같은 사건도 원본처럼 처리한다.
    for (const auto event:{0x25cU,0x25dU}) {
        while (auto* regular=host_.FindEvent(parent,event)) host_.Kill(*regular,0);
    }
    frame_.Set(parent,hooks_.metadata(parent).defaultFrame,0);
}
// 후속 월드 어댑터는 실제 저장소가 같은지 검사할 수 있다.
const SidPool& RawPathStart::Pool() const { return pool_; }
}
