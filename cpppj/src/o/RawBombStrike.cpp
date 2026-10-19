// 직접 피해 주문의 서로 다른 죽음 검사와 피해 전 HP·난수 소비를 보존한다.
#include "o/RawBombStrike.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::array<std::uint32_t,4> kTables{0x502908,0x5029d0,0x502aa8,0x502b70}; // 실제 네 가상 표의 순서이며 첫 계열은 특별 대상 삭제를 포함한다.
constexpr std::uint32_t kDais=0x400000,kHasHp=0x10,kAllowed=0x80074200,kFirstAllowed=0x120000; // 원본 자격이 읽는 HP 보유/genus 비트다.
// 선택한 실제 효과가 빠졌으면 무동작 성공으로 처리하지 않는다.
template<class F> const F& Need(const F& hook) {
    if (!hook) throw std::logic_error("직접 피해 주문 실제 하위 미연결");return hook;
}
}
// 복원 판본과 타입 표의 원래 범위를 확인한다.
RawBombStrike::RawBombStrike(const SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,BombStrikeState& state,BombStrikeHooks hooks)
    :pool_(pool),types_(types),random_(random),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("직접 피해 주문 판본/TYPE 오류");
}
// 현재 바이트를 little endian의 지정 폭으로 조합한다.
std::uint32_t RawBombStrike::Read(Sid sid,std::size_t offset,std::size_t width) const {
    const auto raw=pool_.Slot(sid);std::uint32_t value{};
    // 원본 물리 필드의 현재 바이트만 읽는다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);
    return value;
}
// 큰 효과 뒤 바뀐 종류를 이전 TYPE로 캐시하지 않는다.
const RiftTypeRecord& RawBombStrike::Type(Sid sid) const {
    const auto kind=Read(sid,10,1);if (kind>=types_.size()) throw std::out_of_range("직접 피해 주문 현재TYPE 범위");return types_[kind];
}
// 실제 중심 함수의double 계산 뒤float 저장을 보존한다.
std::array<float,2> RawBombStrike::Center(Sid sid) const {
    const auto& type=Type(sid);
    return {static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(sid,14)))-type.footX*0.5),static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(sid,18)))-type.footY*0.5)};
}
// 타입 번호나 공통 genus만으로 다른 주문까지 포함하지 않는다.
bool RawBombStrike::Handles(Sid parent) const {
    const auto table=Read(parent,0);return std::find(kTables.begin(),kTables.end(),table)!=kTables.end();
}
// 자격은 현재HP26 수치·free/dead 비트를 검사하지 않는다.
std::uint32_t RawBombStrike::MayTarget(Sid parent,Sid target) const {
    const auto table=Read(parent,0);if (!Handles(parent)) throw std::invalid_argument("직접 피해 주문 자격 가상 표 오류");
    const auto& type=Type(target);
    if (table==kTables[0]) return type.flags2&kFirstAllowed;
    const bool hasHp=(type.flags2&kDais) ? (Read(target,40,1)&1U)!=0 : (type.flags1&kHasHp)!=0;
    return hasHp && (type.flags2&kAllowed)!=0 ? 1U : 0U;
}
// 실제 가상98 요청 직전에 현재 표와 부모 소유자를 읽는다.
void RawBombStrike::Hit(Sid parent,Sid target,std::int32_t amount) const {
    Need(hooks_.damage)(target,Read(target,0),amount,static_cast<std::uint8_t>(Read(parent,34,1)),parent);
}
// 생성이 현재 대상 위치/타입을 바꿀 수 있으므로 중심을 생성 전에 캡처하지 않는다.
void RawBombStrike::Spawn(Sid target,std::uint32_t kind) const {
    const auto created=Need(hooks_.create)(kind,2);const auto center=Center(target);
    Need(hooks_.pop)(created,Read(created,0),center[0],center[1],0);
}
// 기둥/빔 효과 이후의 현재 부모 중심으로 소리를 요청한다.
void RawBombStrike::Sound(Sid parent) const {
    const auto center=Center(parent);Need(hooks_.sound)(center[0],center[1]);
}
// 각 계열의 실제 조기 반환과 외부 효과 사이 재조회를 독립적으로 보존한다.
void RawBombStrike::Apply(Sid parent,Sid target) const {
    const auto table=Read(parent,0);if (!Handles(parent)) throw std::invalid_argument("직접 피해 주문 효과 가상 표 오류");
    if (target.value==Read(parent,12,2)) return;
    if (table==kTables[0]) {
        Spawn(target,state_.firstEffectType);
        if (Read(target,10,1)!=state_.specialType) Hit(parent,target,2000);
        else if (state_.boss) Need(hooks_.destroy)(target,Read(target,0),0x100008);
        return;
    }
    if (table==kTables[1]) {
        const Sid savedSource{static_cast<std::uint16_t>(Read(parent,12,2))};
        if (Read(target,11,1)&2U) return;
        Sound(parent);
        if (savedSource.value && !(Read(savedSource,11,1)&3U)) Need(hooks_.beam)(Sid{static_cast<std::uint16_t>(Read(parent,12,2))},target);
        Hit(parent,target,500);return;
    }
    if (table==kTables[2]) {
        if (Read(target,11,1)&2U) return;
        const auto center=Center(target);const auto from=Need(hooks_.project)(center[0],center[1]);const auto to=Need(hooks_.project)(center[0],center[1]);
        Need(hooks_.column)(target,{from[0],0},to,5.0F);Sound(parent);
        const auto count=random_.Next(4)+1;
        // 반복 횟수는 최초 난수이며 현재HP는 피해량 난수를 소비한 뒤 읽는다.
        for (std::uint32_t i=0;i<count;++i) {
            const auto choice=random_.Next(3);
            if (choice<2) {
                const auto amount=static_cast<std::int32_t>(random_.Next(400)+100);const auto hp=std::bit_cast<std::int32_t>(Read(target,26));
                if (hp<1) return;
                Hit(parent,target,amount);if (hp<amount) return;
            }
        }
        return;
    }
    const auto count=random_.Next(49)+1;
    // 마지막 계열은 초기dead 관문이 없고 성공 난수마다 생성/Pop을 피해 판단보다 먼저 실행한다.
    for (std::uint32_t i=0;i<count;++i) {
        if (random_.Next(2)) {
            Spawn(target,state_.hailEffectType);const auto amount=static_cast<std::int32_t>(random_.Next(30)+20);const auto hp=std::bit_cast<std::int32_t>(Read(target,26));
            if (hp<1) return;
            Hit(parent,target,amount);if (hp<amount) return;
        }
    }
}
// 같은 실제 저장소인지 상위 어댑터에서 확인한다.
const SidPool& RawBombStrike::Pool() const { return pool_; }
}
