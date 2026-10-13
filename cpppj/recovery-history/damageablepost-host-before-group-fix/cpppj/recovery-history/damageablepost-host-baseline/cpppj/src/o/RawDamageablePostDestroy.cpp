// 매 효과 뒤 현재 raw를 재조회하며 packed 피해는 payload가 아닌 Regular count로 전달한다.
#include "o/RawDamageablePostDestroy.h"
#include "o/Bridge.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 공통 raw 필드와 HP 피해/다리 후처리 마스크다.
constexpr std::size_t kType=10,kState=11,kWord=12,kX=14,kY=18;
constexpr std::uint32_t kDropMask=0x174200,kVortex=0x200,kBridge=4,kRubble=0x1000000;
// finder 반경 helper의 두 PE 상수는00502310/CD005017a0이며 중간float 저장없이 더한다.
constexpr float kAreaBias=0.99999F;
// 비정렬 원본 필드를 지정 폭의 little endian 값으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 필요한 필드의 각 BYTE만 결합한다.
    for (std::size_t byte=0;byte<width;++byte) value|=static_cast<std::uint32_t>(raw[offset+byte])<<(byte*8);
    return value;
}
// 원본 WORD/프레임의 폭만 써 인접 필드를 보존한다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 낮은 바이트부터 원본 지정폭만 대입한다.
    for (std::size_t byte=0;byte<width;++byte) raw[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// _ftol의signed64 절삭 후 하위 DWORD를 보존한다. 손상 비유한 입력은 계약 밖이다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0)
        throw std::out_of_range("Damageable post 좌표 CRT 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// signed add/sub의 원본 DWORD 감김을 유지한다.
std::int32_t Add(std::int32_t value,std::int32_t delta) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(delta));
}
}
// 같은 실제 Pool/Hash/HP/중심 자료와 모든 원본 하위 효과를 확인한다.
RawDamageablePostDestroy::RawDamageablePostDestroy(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const RiftTypeFrames> frames,const SquidReward& hp,const RawSquidNeighbors& footprint,
    const DamageablePostDestroyState& state,DamageablePostDestroyHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),hp_(hp),footprint_(footprint),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || frames.size()!=types.size() ||
        &hp.Pool()!=&pool || &footprint.Pool()!=&pool || &footprint.Hash()!=&hash ||
        !hooks_.dropDude || !hooks_.dropIsland || !hooks_.regular || !hooks_.hasBridgeFall ||
        !hooks_.notifySurface || !hooks_.displayUpdate || !hooks_.destroy || !hooks_.base)
        throw std::invalid_argument("Damageable post 실제 자료/효과 연결 오류");
}
// 삭제 뒤 raw의dead 상태도 원본처럼 읽으며 명시 SID/타입 범위만 제한한다.
std::span<const std::uint8_t> RawDamageablePostDestroy::Slot(Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("Damageable post SID 오류");
    const auto raw=pool_.Slot(sid);if (raw[kType]>=types_.size()) throw std::out_of_range("Damageable post 타입 오류");return raw;
}
// callback 뒤 새 raw 타입을 다시 조회한다.
const RiftTypeRecord& RawDamageablePostDestroy::Type(Sid sid) const { return types_[Slot(sid)[kType]]; }
// 현재 원본 genus와 abstract/buried 상태다.
std::uint32_t RawDamageablePostDestroy::Genus(Sid sid) const { return Type(sid).flags2; }
// 현재 extra의abstract/buried만검사하며free/dead는별도원본분기에서읽는다.
bool RawDamageablePostDestroy::Ordinary(Sid sid) const { return (Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U]&9)==0; }
// 부모/후보의 raw 좌표를 별도 중심 값으로 대신하지 않는다.
std::array<float,2> RawDamageablePostDestroy::Position(Sid sid) const {
    const auto raw=Slot(sid);return {std::bit_cast<float>(Read(raw,kX)),std::bit_cast<float>(Read(raw,kY))};
}
// 기존 일반 탐색기로 한 칸 genus 조회의 plain truncate와 skipSurface를 보존한다.
Sid RawDamageablePostDestroy::GenusAt(float x,float y,std::uint32_t genus) const {
    const auto ix=Truncate(x),iy=Truncate(y);RawSquidFinder finder(pool_,hash_,types_);
    // 원본의 첫 통과 후보만 반환하고 이 helper가 free/dead 필터를 별도 추가하지 않는다.
    for (auto current=finder.Begin({ix,iy,ix,iy},(genus&6)==0 ? 1U : 0U);current.value;current=finder.Next())
        if (Genus(current)&genus) return current;
    return {};
}
// 중심 float저장 뒤 원본 거의올림을 적용하고 반경 덧셈은 DWORD 감김으로 보존한다.
SquidSearchArea RawDamageablePostDestroy::Area(Sid sid,std::int32_t radius) const {
    const auto center=footprint_.Center(sid);const auto x=Truncate(static_cast<double>(center[0])+kAreaBias),y=Truncate(static_cast<double>(center[1])+kAreaBias);
    return {Add(x,-radius),Add(y,-radius),Add(x,radius),Add(y,radius)};
}
// 현재 Bridge frame의 signed/unsigned 비교가 쓰는 flags 바이트를 그대로 읽는다.
std::uint8_t RawDamageablePostDestroy::FrameFlags(Sid sid) const {
    const auto raw=Slot(sid);const auto frame=Read(raw,pool_.Edition()==OriginalEdition::Patch1078 ? 36U : 34U,
        pool_.Edition()==OriginalEdition::Patch1078 ? 4U : 1U);const auto codes=frames_[raw[kType]].Codes();
    if (frame>=codes.size()) throw std::out_of_range("Damageable post 다리 프레임 오류");return codes[frame].flags;
}
// 원본 WORD 쓰기/현재 global/공간 callback 이후의 타입·frame 재조회 순서를 유지한다.
void RawDamageablePostDestroy::Weaken(Sid sid) const {
    const auto word=Read(Slot(sid),kWord,2);const auto next=(word&~0x78U)|(((state_.bridgeCrackLife-1U)*8U)&0x78U);
    Write(pool_.AllocatedBytes(sid),kWord,next,2);hooks_.notifySurface(sid);
    const auto raw=Slot(sid);const auto old=Read(raw,pool_.Edition()==OriginalEdition::Patch1078 ? 36U : 34U,
        pool_.Edition()==OriginalEdition::Patch1078 ? 4U : 1U);
    const auto frame=Bridge::CrackedFrame(frames_[raw[kType]],std::bit_cast<std::int32_t>(old),true);
    if (frame!=-1) { Write(pool_.AllocatedBytes(sid),pool_.Edition()==OriginalEdition::Patch1078 ? 36U : 34U,
        static_cast<std::uint32_t>(frame),pool_.Edition()==OriginalEdition::Patch1078 ? 4U : 1U);hooks_.displayUpdate(sid); }
}
// 실제 생성 효과의 미복원 부분을 기본값으로 대체하지 않고 수량 결정 뒤 매 반복 요청한다.
void RawDamageablePostDestroy::Drops(Sid sid,std::uint32_t genus,bool random) const {
    if (!sid.value) return;const auto raw=Slot(sid);
    if ((raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U]&1) || !(genus&kDropMask)) return;
    std::uint32_t count=1;if (genus&0x4200U) count=5;if (genus&0x20000U) count=2;
    const auto type=raw[kType];const auto excluded=pool_.Edition()==OriginalEdition::Patch1078 ? 5U : 4U;
    // 판본별 제외 타입과 현재 특별 타입을 원본 우선순서로 검사한다.
    for (std::size_t index=0;index<excluded;++index) if (type==state_.excludedDropTypes[index]) return;
    if (type==state_.specialDropType) count=3;if (state_.dudeCount>20) count/=2;
    // 생성 뒤 부모/전역이 변해도 처음 계산한 반복 수는 다시 계산하지 않는다.
    for (std::uint32_t index=0;index<count;++index) hooks_.dropDude(sid,static_cast<std::int32_t>(index),random);
}
// 첫 드롭/섬 정리 뒤 부모 상태를 현재 읽고 HP 피해 예약과 다리 순회를 수행한다.
void RawDamageablePostDestroy::PostDestroy(Sid sid,std::uint32_t flags) const {
    Slot(sid);if (flags&0x300000U) Drops(sid,Genus(sid),true);
    if (Ordinary(sid) && (Type(sid).flags1&0x400U)) {
        const auto position=Position(sid);
        if (GenusAt(position[0],position[1],kRubble).value && !(Genus(sid)&0x10000000U) &&
            ((flags&0x200000U) || Slot(sid)[kType]!=state_.silentSurfaceType)) {
            const auto current=Position(sid);hooks_.dropIsland(current[0],current[1],flags,
                Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U]);
        }
    }
    if (state_.battle && hp_.HasHitPoints(sid) && Ordinary(sid)) {
        const auto attackerOwner=(flags>>16)&15U;
        if (!(flags&0x200000U) && (Type(sid).group==0 || Type(sid).group==1 || (Genus(sid)&kVortex))) {
            auto damage=Add(hp_.MaxHitPoints(sid)/2,1);if (Genus(sid)&kVortex) damage=400;
            if (!(flags&0x400000U)) {
                const auto radius=Add(Type(sid).footX/2,1);RawSquidFinder finder(pool_,hash_,types_);
                // 첫후보부터현재 일반 finder의skipSurface=1 순서로 HP 대상을 처리한다.
                for (auto current=finder.Begin(Area(sid,radius),1);current.value;current=finder.Next())
                    if (hp_.HasHitPoints(current) && !(Slot(current)[kState]&2) && !(Slot(current)[pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U]&1))
                        hooks_.regular(current,state_.damageEvent,0.0F,attackerOwner*65536U+static_cast<std::uint32_t>(damage));
            }
        }
        if (!(Genus(sid)&0x130000U)) {
            RawSquidFinder finder(pool_,hash_,types_);
            // 기존순회가 저장한next를 사용하며 각 후보 frame/state는 효과 뒤 새로 읽는다.
            for (auto current=finder.Begin(Area(sid,2));current.value;current=finder.Next()) if (Genus(current)&kBridge) {
                if (!(FrameFlags(current)&0x20U) || hooks_.hasBridgeFall(current)) { if (!(FrameFlags(current)&0x40U)) Weaken(current); }
                else if (state_.boss) hooks_.destroy(current,0);
            }
        }
    }
    hooks_.base(sid,flags);
}
// Dais/Altar 연결이 같은 실제 저장소를 사용하는지 확인한다.
const SidPool& RawDamageablePostDestroy::Pool() const { return pool_; }
}
