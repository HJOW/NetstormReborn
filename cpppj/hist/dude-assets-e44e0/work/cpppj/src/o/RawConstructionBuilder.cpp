// 원본 생성/이동의 현재 자료 읽기와 실패 전 효과·공급원 소비 순서를 보존한다.
#include "o/RawConstructionBuilder.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// raw 타입·좌표·단어와 판본별 소유자/프레임 위치다.
constexpr std::size_t kType=10,kWord=12,kX=14,kY=18,kPatchOwner=34,kCdOwner=32,kPatchFrame=36,kCdFrame=34;
// 기준점/첫 경로 실패의 원본 double 반환값과 처음 객체를 놓는 플래그다.
constexpr double kFailureTime=5.0;
constexpr std::uint32_t kFirstPop=0x10,kClientCreate=2;
// 정렬되지 않은 raw 값을 little endian DWORD/WORD로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 지정 폭의 바이트만 조합한다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return result;
}
// 정렬되지 않은 좌표의 비트를 단정도로 해석한다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) { return std::bit_cast<float>(Read(raw,offset)); }
// raw WORD/프레임 쓰기는 지정 폭의 낮은 바이트만 바꾸며 주변 필드를 보존한다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) {
    // native 쓰기의 폭을 넘기지 않는다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 실제 발자국/중심 계산 전에 호스트의 비유한 좌표와 잘못된 크기를 거부한다.
void Validate(float x,float y,const RiftTypeRecord& type) {
    if (!std::isfinite(x) || !std::isfinite(y) || type.footX<1 || type.footY<1 || type.footX>256 || type.footY>256)
        throw std::out_of_range("건설 사제 좌표/발자국 오류");
}
}
// 미연결 하위 효과는 첫 생성/소유자 쓰기 전에 확인한다.
RawConstructionBuilder::RawConstructionBuilder(SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const ConstructionBuilderType> metadata,ConstructionBuilderState& state,ConstructionBuilderHooks hooks)
    :pool_(pool),types_(types),metadata_(metadata),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || metadata.size()!=types.size() ||
        !hooks_.anchor || !hooks_.create || !hooks_.setOwner || !hooks_.approach || !hooks_.pop || !hooks_.move || !hooks_.speed ||
        !hooks_.chooseSources || !hooks_.sortSources || !hooks_.notifySurface || !hooks_.reserveTimer || !hooks_.addTimer)
        throw std::invalid_argument("건설 사제 자료/효과 연결 오류");
}
// DWORD가 풀 안인지 먼저 확인해 임의 WORD SID로 감기지 않도록 한다. 예약 슬롯 0도 조회는 허용한다.
Sid RawConstructionBuilder::CheckedSid(std::uint32_t sid) const {
    if (sid>=pool_.Capacity()) throw std::out_of_range("건설 사제 SID 범위 오류");
    return Sid{static_cast<std::uint16_t>(sid)};
}
// 현재 raw 타입을 읽어 발자국/genus 자료를 찾는다.
const RiftTypeRecord& RawConstructionBuilder::Type(Sid sid) const {
    const auto type=pool_.Slot(sid)[kType];
    if (type>=types_.size()) throw std::out_of_range("건설 사제 타입 오류");
    return types_[type];
}
// 경계 호출 뒤의 현재 raw 타입으로 속도/프레임 메타 자료를 다시 읽는다.
const ConstructionBuilderType& RawConstructionBuilder::Metadata(Sid sid) const {
    const auto type=pool_.Slot(sid)[kType];
    if (type>=metadata_.size()) throw std::out_of_range("건설 사제 메타 타입 오류");
    return metadata_[type];
}
// 현재 발자국의 두 끝을 지도 범위로 제한해 left/top/right/bottom 사각형을 만든다.
std::array<float,4> RawConstructionBuilder::Footprint(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto x=Coordinate(raw,kX),y=Coordinate(raw,kY);const auto& type=Type(sid);Validate(x,y,type);
    const auto px=std::clamp(static_cast<float>(static_cast<double>(x)+1-type.footX),1.0F,255.0F);
    const auto py=std::clamp(static_cast<float>(static_cast<double>(y)+1-type.footY),1.0F,255.0F);
    const bool valid=x>0 && y>0 && x<256 && y<256;
    return {valid ? std::min(x,px) : px,valid ? std::min(y,py) : py,
        valid ? std::max(x,px) : px,valid ? std::max(y,py) : py};
}
// 원본 발자국의 위쪽 가운데를 구하고 floor(좌표+0.5)로 반올림한다.
ConstructionNoticePoint RawConstructionBuilder::Destination(Sid sid) const {
    const auto rect=Footprint(sid);const auto left=rect[0],top=rect[1],right=rect[2];
    const auto middle=static_cast<float>((static_cast<double>(right)-left)*0.5+left);
    return {static_cast<float>(std::floor(static_cast<double>(middle)+0.5)),static_cast<float>(std::floor(static_cast<double>(top)+0.5))};
}
// 004adea0 / CD 004aef30은 좌표에서 발자국 폭/높이의 절반을 뺀다. (크기-1)/2가 아니다.
ConstructionNoticePoint RawConstructionBuilder::Center(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto x=Coordinate(raw,kX),y=Coordinate(raw,kY);const auto& type=Type(sid);Validate(x,y,type);
    return {static_cast<float>(static_cast<double>(x)-type.footX*0.5),static_cast<float>(static_cast<double>(y)-type.footY*0.5)};
}
// 속도 조정/운반 이동 뒤에도 현재 경로 단계로 시각을 다시 읽는다.
double RawConstructionBuilder::Arrival(const ConstructionBuilderPath& path) const {
    if (path.currentStep>=path.arrivalTimes.size()) throw std::out_of_range("건설 사제 경로 시각 인덱스 오류");
    return path.arrivalTimes[path.currentStep];
}
// factory가 새 객체를 실제 풀 안에 만들었는지 효과 전달 전에 확인한다.
Sid RawConstructionBuilder::Create(std::uint32_t type) const {
    if (type>=types_.size()) throw std::out_of_range("건설 사제 생성 타입 오류");
    const auto sid=hooks_.create(type,kClientCreate);
    if (sid.value<5 || sid.value>=pool_.Capacity() || (pool_.Slot(sid)[11]&1)) throw std::invalid_argument("건설 사제 생성 결과 오류");
    return sid;
}
// 원본 cursor는 개수를 줄이지 않고 활성 저장소의 첫 0이 아닌 항목만 지운다.
std::uint32_t RawConstructionBuilder::TakeSource() const {
    if (std::bit_cast<std::int32_t>(state_.sources.count)<=0) return 0;
    (void)state_.sources.Items();
    // 매번 처음부터 스캔하여 경계가 목록을 바꾼 경우에도 현재 순서로 소비한다.
    for (std::uint32_t i=0;i<state_.sources.count;++i) {
        const auto sid=state_.sources.entries[i];if (!sid) continue;
        state_.sources.entries[i]=0;return sid;
    }
    return 0;
}
// 생성/경로 실패의 앞선 효과를 유지하고, 공급원 경계 뒤에는 현재 목표/기준점/타입을 다시 읽는다.
double RawConstructionBuilder::Move(std::uint32_t player,Sid target,std::uint32_t timeLow,std::uint32_t timeHigh) const {
    const auto raw=pool_.Slot(target);const auto anchorValue=hooks_.anchor(player,raw[kType],Coordinate(raw,kX),Coordinate(raw,kY));
    if (!anchorValue) return kFailureTime;
    const auto anchor=CheckedSid(anchorValue);const auto first=Create(state_.builderType);hooks_.setOwner(first,player);
    const auto destination=Destination(target);
    const auto mask=state_.approachMask;const auto flags=(Type(anchor).flags2&mask) ? mask|1U : 1U;
    const auto start=hooks_.approach(Footprint(anchor),destination.x,destination.y,flags);hooks_.pop(first,start.x,start.y,kFirstPop);
    auto* path=hooks_.move(first,destination.x,destination.y);if (!path) return kFailureTime;
    const auto requested=std::bit_cast<double>((static_cast<std::uint64_t>(timeHigh)<<32)|timeLow);
    if (requested!=0.0) {
        const auto arrival=Arrival(*path);const auto speed=Metadata(first).speed;
        // 패치는 먼저 속도를 곱하고 CD는 먼저 나눈다. x87 중간값을 단정도로 조기 절단하지 않는다.
        const double rate=pool_.Edition()==OriginalEdition::Patch1078 ?
            ((requested-state_.now)*speed)/(arrival-state_.now) : ((requested-state_.now)/(arrival-state_.now))*speed;
        hooks_.speed(*path,static_cast<float>(rate),0);
    }
    const auto current=pool_.Slot(target);const auto ownerOffset=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchOwner : kCdOwner;
    hooks_.chooseSources(Coordinate(current,kX),Coordinate(current,kY),current[kType],pool_.Slot(anchor)[ownerOffset]);
    const auto sorted=pool_.Slot(target);hooks_.sortSources(Coordinate(sorted,kX),Coordinate(sorted,kY));std::uint32_t index=0;
    // first의 경로 객체를 보관하며 운반 객체의 경로 실패는 다음 공급원 처리를 막지 않는다.
    for (auto sourceValue=TakeSource();sourceValue;sourceValue=TakeSource()) {
        const auto source=CheckedSid(sourceValue);const auto carrier=Create(state_.carrierType);
        Write(pool_.AllocatedBytes(carrier),kWord,index,2);
        if (pool_.Slot(carrier)[kType]==state_.bridgeType) hooks_.notifySurface(carrier);
        hooks_.setOwner(carrier,pool_.Slot(anchor)[ownerOffset]);
        const auto frameIndex=Metadata(source).sourceFrameIndex;const auto& meta=Metadata(carrier);
        if (frameIndex>=meta.firstFrames.size()) throw std::out_of_range("건설 운반 프레임 표 인덱스 오류");
        const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
        Write(pool_.AllocatedBytes(carrier),patch ? kPatchFrame : kCdFrame,std::bit_cast<std::uint32_t>(meta.firstFrames[frameIndex]),patch ? 4 : 1);
        const auto position=Center(source);hooks_.pop(carrier,position.x,position.y,0);
        if (hooks_.reserveTimer()) hooks_.addTimer(carrier,0,state_.timerPayload);
        (void)hooks_.move(carrier,destination.x,destination.y);++index;
    }
    return Arrival(*path);
}
// 동일 풀 연결 검사용 참조다.
const SidPool& RawConstructionBuilder::Pool() const { return pool_; }
// factory/기준점/일반 owner와 선택적 실제 Regular host를 합성한다. 파생 owner 재정의는 별도 분배한다.
ConstructionBuilderHooks MakeConstructionBuilderHooks(const SidPool& pool,const RawPlayerPlacementAnchor& anchor,
    SquidFactory& factory,SquidOwner& owner,ConstructionBuilderHooks hooks,SquidProcessHost* process) {
    if (&pool!=&anchor.Pool() || &pool!=&factory.Pool() || &pool!=&owner.Pool() || (process && &pool!=&process->Pool()))
        throw std::invalid_argument("건설 사제 어댑터의 풀 연결 오류");
    hooks.anchor=MakePlayerAnchorQuery(pool,anchor);
    hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };
    hooks.setOwner=[&owner](Sid sid,std::uint32_t player) { owner.Set(sid,player); };
    if (process) hooks.addTimer=[process](Sid sid,std::uint32_t event,float payload) { process->AddRegular(sid,event,payload); };
    return hooks;
}
// 호출자는 원본처럼 double 결과를 버리되 생성/이동의 모든 효과를 실행한다.
ConstructionLocalHooks MakeConstructionBuilderLocalHooks(const SidPool& pool,const RawConstructionBuilder& builder,ConstructionLocalHooks hooks) {
    if (&pool!=&builder.Pool()) throw std::invalid_argument("로컬 건설 사제 어댑터의 풀 연결 오류");
    hooks.moveBuilder=[&builder](std::uint32_t player,Sid sid,std::uint32_t low,std::uint32_t high) { (void)builder.Move(player,sid,low,high); };
    return hooks;
}
// 통지의 double 시각 비트를 원래 순서로 실제 생성/이동에 전달한다.
ConstructionNoticeHooks MakeConstructionBuilderNoticeHooks(const SidPool& pool,const RawConstructionBuilder& builder,ConstructionNoticeHooks hooks) {
    if (&pool!=&builder.Pool()) throw std::invalid_argument("통지 건설 사제 어댑터의 풀 연결 오류");
    hooks.moveBuilder=[&builder](std::uint32_t player,Sid sid,std::uint32_t low,std::uint32_t high) { (void)builder.Move(player,sid,low,high); };
    return hooks;
}
}
