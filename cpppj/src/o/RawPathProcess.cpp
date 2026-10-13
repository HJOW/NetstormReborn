// 이동/도착은 실제 raw 공간 효과를 호출하고, 자기 삭제 뒤에는 shared watch만 읽는다.
#include "o/RawPathProcess.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 좌표 거의 올림 배율과 사제 프레임의 각도 주기다.
constexpr float kCellBias=0.9999F;
constexpr double kStepAngle=45.0;
// 원본의 정렬되지 않은 raw 필드를 바이트 순서대로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 정렬/호스트 엔디언에 의존하지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 원본 _ftol의 int 좌표를 유효 spot 인덱스로 바꾼다.
int Cell(float value,bool biased) {
    const double adjusted=static_cast<double>(value)+(biased ? kCellBias : 0.0);
    if (!std::isfinite(adjusted) || adjusted<0 || adjusted>=256) throw std::out_of_range("경로 지도 좌표");
    return static_cast<int>(adjusted);
}
}
// 필수 하위 가상 효과를 확인한다. 아직 없는 실제 BFS는 Init에서만 명시적으로 진단한다.
RawPathProcess::RawPathProcess(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const RiftTypeFrames> frames,std::span<const SquidDisplayShape> shapes,std::span<const std::uint8_t> spots,
    RawPathProcessState& state,RawPathProcessHooks hooks,bool priest)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),shapes_(shapes),spots_(spots),state_(state),hooks_(std::move(hooks)),priest_(priest),runtime_(pool.Edition(),priest) {
    if (types.size()!=frames.size() || types.size()!=shapes.size() || spots.size()!=65536 || !hooks_.unpop || !hooks_.repop || !hooks_.pop || !hooks_.halt || !hooks_.arrival || !hooks_.soundId || !hooks_.stepSound || !hooks_.clearBlocked || !hooks_.cleanup)
        throw std::invalid_argument("경로 월드 연결 누락");
}
// 원본 stack deadFlag가 하는 자기 삭제 관찰을 수명 안전하게 보존한다.
RawPathProcess::~RawPathProcess() { *alive_=false; }
// 직접 부착도 이 모듈의 동일 풀 계약을 확인한다.
RawPathProcess* RawPathProcess::Attach(SquidProcessHost& host,std::unique_ptr<RawPathProcess> process,Sid parent,std::uint32_t flags,Sid form) {
    if (!process || &host.Pool()!=&process->pool_) throw std::invalid_argument("경로 부착 풀 불일치");
    if (process->priest_ && process->pool_.Slot(parent)[10]!=process->state_.priestType) throw std::invalid_argument("사제 경로 부모 타입");
    const auto type=process->priest_ ? process->state_.priestProcess : process->state_.teleportProcess;
    return static_cast<RawPathProcess*>(host.Attach(std::move(process),type,parent,flags,form));
}
// +0x14 원본 base 통지는 빈 몸체이며 현재 이미지의 번호만 같은 base 필드에 기록한다.
void RawPathProcess::OnAttached() { runtime_.SetAttachment(Form(),Parent()); }
// 같은 풀의 raw 좌표다.
RawPathPoint RawPathProcess::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// CD는 frame BYTE만 읽는다.
std::int32_t RawPathProcess::Frame(Sid sid) const { const auto raw=pool_.Slot(sid);return pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)) : raw[34]; }
// 프레임 바꾸기는 raw 폭대로만 쓰고 가상 Pop은 호출자가 순서를 보존한다.
void RawPathProcess::SetFrame(Sid sid,std::int32_t frame) {
    auto raw=pool_.AllocatedBytes(sid);const auto value=static_cast<std::uint32_t>(frame);
    if (pool_.Edition()==OriginalEdition::Cd1072) { raw[34]=static_cast<std::uint8_t>(value);return; }
    // 패치 frame은 네 바이트 전체다.
    for (std::size_t i=0;i<4;++i) raw[36+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 원본 start 좌표가 보드 밖이면 경로/단계를 바꾸지 않는다.
void RawPathProcess::InitializePath(Sid parent,RawPathPoint destination,RawPathPoint start) {
    if (!hooks_.search) throw std::logic_error("실제 Path BFS 미복원 경계");
    if (!(std::isfinite(start.x) && std::isfinite(start.y) && start.x>0 && start.y>0 && start.x<256 && start.y<256)) return;
    if ((pool_.Slot(parent)[11]&4)!=0) hooks_.pop(parent,start,0);
    hooks_.search(runtime_,parent,start,destination,0);runtime_.ResetCurrent();
}
// 외부 타입 속도는 mutable 원본 속성을 매 호출에서 읽어 공급해야 한다.
void RawPathProcess::Prepare(float speedOverride,float speed,float turningSpeed) {
    if (speed==0 || turningSpeed==0) throw std::invalid_argument("경로 타입 속도0");
    runtime_.Prepare(state_.clock.now,state_.clock.now,speedOverride==0 ? speed : speedOverride,turningSpeed,runtime_.Direction(0));
}
// 클라이언트 범위는 현재 풀 판본의 실제 serverFirst다.
bool RawPathProcess::MayMutate() const { return state_.clock.boss || (Form().value>4 && Form().value<pool_.Layout().serverFirst); }
// 원본 isRobust(1)의 핵심은 현재 form/부모가 살아 있고 같은 프로세스가 등록된 경우다.
bool RawPathProcess::Robust() const {
    return Host() && Host()->IsRegistered(Pid(),this) && Form().value!=0 && (pool_.Slot(Form())[11]&3)==0 && Parent().value!=0 && (pool_.Slot(Parent())[11]&3)==0;
}
// callback 자체가 this를 지우면 돌아온 코드가 객체 필드를 읽지 않는다.
bool RawPathProcess::Halt() {
    const auto watch=alive_;const auto parent=Parent();auto result=hooks_.halt(parent);
    if (!*watch) { if (result!=12) throw std::logic_error("경로 Halt 자기 삭제 반환값");return true; }
    if (result==10 && (pool_.Slot(parent)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&0x20)!=0) result=11;
    if (result==11) { if (MayMutate()) Host()->Kill(*this,0);return true; }
    const auto type=pool_.Slot(parent)[10];
    if (state_.clock.boss && (types_[type].flags2&0x30000)!=0 && (runtime_.TargetA()!=0 || runtime_.TargetB()!=0)) {
        if (!hooks_.retarget) throw std::logic_error("실제 운반 재탐색 몸체 미복원 경계");
        hooks_.retarget(runtime_,parent);if (!*watch) return true;
    }
    return result!=10;
}
// 길 밖 검사는 타입 허용→ceil/floor spot6→권한→실제 일반 finder 순서다.
bool RawPathProcess::GroundStop(RawPathPoint point) {
    const auto type=pool_.Slot(Parent())[10];const auto count=pool_.Edition()==OriginalEdition::Patch1078 ? 5U : 4U;
    if (std::find(state_.groundTypes.begin(),state_.groundTypes.begin()+count,type)==state_.groundTypes.begin()+count) return false;
    const int cx=Cell(point.x,true),cy=Cell(point.y,true),fx=Cell(point.x,false),fy=Cell(point.y,false);
    if ((spots_[cy*256+cx]&6)!=0 && (spots_[fy*256+fx]&6)!=0) return false;
    if (MayMutate()) {
        RawSquidFinder finder(pool_,hash_,types_);
        auto candidate=finder.Begin({cx-1,cy-1,cx+1,cy+1},1);
        // 원본 finder 순서 그대로 주변의 gDAIS(0x400000)만 확인한다.
        while (candidate.value!=0) { if ((types_[pool_.Slot(candidate)[10]].flags2&0x400000)!=0) return false;candidate=finder.Next(); }
        Host()->Kill(*this,0);return true;
    }
    return false;
}
// 추가 shape finder는 해당 정수 칸의 실제 일반 hash를 읽는다.
bool RawPathProcess::Blocked(RawPathPoint point) const {
    const int x=Cell(point.x,true),y=Cell(point.y,true);if ((spots_[y*256+x]&8)==0) return false;
    RawSquidFinder finder(pool_,hash_,types_);auto candidate=finder.Begin({Cell(point.x,false),Cell(point.y,false),Cell(point.x,false),Cell(point.y,false)});
    // 원본은 첫 일치가 나오면 끝낸다.
    while (candidate.value!=0) { if ((types_[pool_.Slot(candidate)[10]].flags2&state_.blockingGenus)!=0) return true;candidate=finder.Next(); }
    return false;
}
// type54은 사망/HP 이동 불가가 아니라 원본 Regular0x5105 존재를 먼저 검사한다.
void RawPathProcess::RunFrame() {
    if (!Host()) throw std::logic_error("미부착 경로 RunFrame");
    const auto parent=Parent();const auto type=pool_.Slot(parent)[10];
    if (priest_ && (types_[type].flags2&0x10000)!=0 && Host()->FindEvent(parent,0x5105)) return;
    if (runtime_.Finished() || Halt()) return;
    const auto watch=alive_;
    const auto sample=runtime_.Step(state_.clock.now,Position(parent),state_.observation,[this,parent] {
        const auto position=Position(parent);hooks_.stepSound(parent,position.x,position.y,hooks_.soundId(parent));
    },hooks_.observationStamp ? hooks_.observationStamp() : 0);
    if (!*watch) return;
    if (sample.finished) { Arrive();return; }
    if (!priest_) {
        const auto current=Position(parent);
        if (sample.position.x!=current.x || sample.position.y!=current.y) { hooks_.unpop(parent,0);hooks_.pop(parent,sample.position,0x40); }
        return;
    }
    if (GroundStop(sample.position) || !*watch || (pool_.Slot(parent)[11]&1)!=0) return;
    const auto current=Position(parent);const auto originalFrame=Frame(parent);auto nextFrame=originalFrame;
    if (originalFrame<0 || static_cast<std::size_t>(originalFrame)>=shapes_[type].frames.size()) throw std::out_of_range("경로 SHP 프레임");
    const auto& shape=shapes_[type].frames[originalFrame];
    const float footX=static_cast<float>(static_cast<double>(current.x)+shape.cellHotspotX),footY=static_cast<float>(static_cast<double>(current.y)+shape.cellHotspotY);
    const auto direction=runtime_.Direction(runtime_.Current());
    if (std::fmod(static_cast<double>(sample.angle),kStepAngle)==0) {
        const double dx=static_cast<double>(sample.position.x)-current.x,dy=static_cast<double>(sample.position.y)-current.y;
        const double hx=static_cast<double>(footX)-current.x,hy=static_cast<double>(footY)-current.y;
        if (hx*hx+hy*hy<dx*dx+dy*dy) {
            if (direction<0 || direction>15) throw std::out_of_range("경로 프레임 글자");
            const auto run=frames_[type].Run(static_cast<std::uint8_t>('A'+direction));
            if (run.count==0) throw std::out_of_range("경로 프레임 구간0");
            nextFrame=((originalFrame-run.first)+1)%run.count+run.first;
            if (nextFrame<0 || static_cast<std::size_t>(nextFrame)>=frames_[type].Codes().size()) throw std::out_of_range("경로 다음 프레임");
        }
    }
    if (nextFrame!=originalFrame) { hooks_.unpop(parent,0);SetFrame(parent,nextFrame);hooks_.pop(parent,sample.position,0x40); }
}
// 도착이 부모·Form·프로세스 자신을 지워도 after-call은 watch와 보존한 외부 효과만 사용한다.
void RawPathProcess::Arrive() {
    const auto watch=alive_;const auto parent=Parent();
    if (priest_) { RawPathAnimation animation(pool_,frames_,{hooks_.unpop,hooks_.repop});animation.Stop(parent); }
    const auto sample=runtime_.Step(state_.clock.now,Position(parent),state_.observation,[this,parent] {
        const auto position=Position(parent);hooks_.stepSound(parent,position.x,position.y,hooks_.soundId(parent));
    },hooks_.observationStamp ? hooks_.observationStamp() : 0);
    hooks_.unpop(parent,0);hooks_.pop(parent,sample.position,0);if (!*watch || !Robust()) return;
    const bool blocked=Blocked(sample.position);const auto clear=hooks_.clearBlocked;const auto pool=&pool_;
    const auto clearCurrent=[clear,pool,parent] {
        const auto raw=pool->Slot(parent);clear(parent,{std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))},raw[10],raw[pool->Edition()==OriginalEdition::Patch1078 ? 34 : 25]);
    };
    if (Halt()) { if (blocked) clearCurrent();return; }
    runtime_.SetFinished(true);const auto result=hooks_.arrival(parent,runtime_.TargetA(),runtime_.TargetB());
    if (result!=1000 && result!=1001 && result!=1002) throw std::logic_error("경로 도착 반환값");
    if (!*watch) { if (result!=1001) throw std::logic_error("경로 도착 자기 삭제 반환값");if (blocked) clearCurrent();return; }
    if (result==1002) runtime_.SetFinished(false);else Host()->Kill(*this,0);
    if (blocked) clearCurrent();
}
// 원본 공간 접두 이후 예약·결정·의식·대상 삭제는 동일 상태를 받는 필수 실제 하위 경계다.
void RawPathProcess::OnFormDestroy(std::uint32_t flags) {
    const auto parent=Parent();
    if (MayMutate() && (pool_.Slot(parent)[11]&2)==0) { hooks_.unpop(parent,0);hooks_.repop(parent,0); }
    hooks_.cleanup(runtime_,parent,flags);
}
// 외부 어댑터가 같은 저장소를 직접 사용한다.
RawPathRuntime& RawPathProcess::Runtime() { return runtime_; }
// 외부 조회는 저장소를 바꾸지 않는다.
const RawPathRuntime& RawPathProcess::Runtime() const { return runtime_; }
}
