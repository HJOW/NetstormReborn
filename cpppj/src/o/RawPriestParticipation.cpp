// 원본의 live 우선·Carrier 즉시 반환·Altar 마지막 후보와 원소유자 필드 폭을 보존한다.
#include "o/RawPriestParticipation.h"
#include "o/RawAltarSacrifice.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 풀 순회에서 사용하는 상태/타입/원소유자와 좌표 필드 위치다.
constexpr std::size_t kType=10,kStatus=11,kOriginalOwner=12,kX=14,kY=18;
// Carrier genus 검사와 포획된 사제 종속의 maskWORD 조건이다.
constexpr std::uint32_t kCarrierGenus=0x30000,kCapturedMask=0x20;
// 비정렬 little endian의 필요한 필드 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t result=0;
    // 정확한 BYTE/WORD/DWORD 폭만 결합해 인접 필드를 섞지 않는다.
    for (std::size_t index=0;index<width;++index) result|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return result;
}
// 실제0041d770/CRT __ftol처럼0방향 절삭하며 호스트의 범위 밖 변환은 거부한다.
int Cell(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<-2147483648.0 || static_cast<double>(value)>=2147483648.0)
        throw std::out_of_range("사제 참여 typed 좌표 절삭 범위");
    return static_cast<int>(value);
}
}
// 현재 장기 저장소와 같은 Carrier 조회를 연결하며 조회와 무관한 삭제 효과를 호출하지 않는다.
RawPriestParticipation::RawPriestParticipation(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    PriestPostPopState& priests,const RawCarrierPreDestroy& carrier,PriestParticipationState state)
    :pool_(pool),hash_(hash),types_(types),priests_(priests),carrier_(carrier),state_(state) {
    if (&carrier.Pool()!=&pool || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U))
        throw std::invalid_argument("사제 참여의 같은 풀/현재 타입 표 연결 누락");
}
// 제단의 실제 typed 조회가 준비되어 있으면 동일 풀임을 확인하고 그 몸체를 사용한다.
RawPriestParticipation::RawPriestParticipation(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    PriestPostPopState& priests,const RawCarrierPreDestroy& carrier,PriestParticipationState state,const RawAltarSacrifice& altar)
    :RawPriestParticipation(pool,hash,types,priests,carrier,state) {
    if (&altar.Pool()!=&pool) throw std::invalid_argument("사제 참여/제단의 SID 풀이 다릅니다");
    altar_=&altar;
}
// 기존 Altar.DaisAt과 동일한 전체 typed helper이며 결과를 가짜 포획 여부로 공급받지 않는다.
Sid RawPriestParticipation::DaisAt(Sid altar) const {
    if (altar_) return altar_->DaisAt(altar);
    const auto raw=pool_.Slot(altar);const auto type=state_.daisType;
    if (type>=types_.size()) throw std::out_of_range("사제 참여의 현재 다이스 타입 범위");
    const auto x=Cell(std::bit_cast<float>(Read(raw,kX,4))),y=Cell(std::bit_cast<float>(Read(raw,kY,4)));
    RawSquidFinder finder(pool_,hash_,types_);
    // 현재 타입 flags2와 실제 버킷·체인을 사용하며 owner/dead/free 추가 필터는 적용하지 않는다.
    for (auto current=finder.Begin({x,y,x,y},(types_[type].flags2&6)==0 ? 1U : 0U);current.value;current=finder.Next())
        if (pool_.Slot(current)[kType]==type) return current;
    return {};
}
// 원본 iterator는 첫 증가로SID2부터 시작하고 live 조회가 성공하면 풀을 읽지 않는다.
std::uint32_t RawPriestParticipation::Find(std::uint32_t player) const {
    if (const auto live=RawPriestLookup(pool_,priests_).Find(player)) return live;
    const auto iteratorEnd=pool_.Capacity();std::uint32_t candidate=2,savedAltar=0;
    // 최초 iterator의 end는 고정하고 각 바깥 반복의 유효 범위는 현재 capacity로 다시 확인한다.
    while (candidate<iteratorEnd && (pool_.Slot(Sid{static_cast<std::uint16_t>(candidate)})[kStatus]&1)) ++candidate;
    // free만 제외한다. dead/void/contained 및 예측 슬롯도 실제 후보로 검사한다.
    for (;;) {
        if (candidate<1 || candidate>=pool_.Capacity()) return savedAltar;
        const Sid sid{static_cast<std::uint16_t>(candidate)};const auto type=pool_.Slot(sid)[kType];
        if (type>=types_.size()) throw std::out_of_range("사제 참여의 현재 후보 타입 범위");
        if (types_[type].flags2&kCarrierGenus) {
            const auto captured=carrier_.FindContained(sid,kCapturedMask,state_.priestType,0);
            if (captured.value && (static_cast<std::uint32_t>(pool_.Slot(captured)[kOriginalOwner])&0x7fU)==player) return candidate;
        }
        // Carrier 조회 뒤의 현재 타입/현재 원소유자를 읽으며 Altar 원소유자는WORD전체다.
        if (pool_.Slot(sid)[kType]==state_.altarType) {
            const auto dais=DaisAt(sid);
            if (dais.value && !(pool_.Slot(dais)[kStatus]&2) && Read(pool_.Slot(sid),kOriginalOwner,2)==player) savedAltar=candidate;
        }
        ++candidate;
        // 원본의 snapshot end까지 free를 건너뛰고 다음 반복에서 현재 capacity를 다시 검사한다.
        while (candidate<iteratorEnd && (pool_.Slot(Sid{static_cast<std::uint16_t>(candidate)})[kStatus]&1)) ++candidate;
    }
}
// 유효하지 않은 소유자도 원본 조회 순서를 유지한 뒤 쓰기만 생략한다.
void RawPriestParticipation::Refresh(std::uint32_t player) const {
    const auto sid=Find(player);const auto value=!sid ? 2U : pool_.Slot(Sid{static_cast<std::uint16_t>(sid)})[kType]==state_.priestType ? 0U : 1U;
    if (player>0 && player<priests_.ownerState.size()) priests_.ownerState[player]=value;
}
// 같은 월드의 소비자 연결 검사에 사용한다.
const SidPool& RawPriestParticipation::Pool() const { return pool_; }
// 사제 등록·제거와 같은 ownerState 저장소를 호출자가 확인할 수 있다.
const PriestPostPopState& RawPriestParticipation::Priests() const { return priests_; }
}
