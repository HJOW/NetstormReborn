// 원본의 분기 순서를 유지하며 필요한 경로에서만 HP/좌표/spot을 읽는다.
#include "o/RawPriestState.h"
#include "o/RawPriestOwner.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// raw 타입/상태/좌표/HP의 두 판본 공통 위치다.
constexpr std::size_t kType=10,kState=11,kX=14,kY=18,kHp=26;
// 사제 부류, 강제 이동 불가 및 이동 가능한 지면의 원본 비트다.
constexpr std::uint32_t kPriestGenus=0x200000;
constexpr std::uint8_t kForcedImmobile=0x20,kSupportedGround=6,kInvalidState=9;
// 두 PE의 00500edc/CD 0050038c: 거의 올림에 쓰는 실제 float 비트다.
constexpr float kCoordinateBias=0.9999f;
// HP 전환의 좌표 helper 00502310/CD 005017a0은 지면 조회보다 큰 별도 배율을 쓴다.
constexpr float kSnapBias=0.99999f;
// 정렬되지 않은 raw 필드를 원본 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 낮은 바이트부터 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// x87의 float+float 중간 합을 float으로 좁히지 않고 0방향 절삭한다.
int Cell(float coordinate) {
    const double shifted=static_cast<double>(coordinate)+static_cast<double>(kCoordinateBias);
    if (!std::isfinite(coordinate) || shifted<0 || shifted>=kWorldCells)
        throw std::out_of_range("사제 지면 조회 좌표 범위 오류");
    return static_cast<int>(shifted);
}
}
// 풀 불일치와 타입/지도 크기 오류는 조회 전에 확인한다.
RawPriestState::RawPriestState(SidPool& pool,const SquidReward& hp,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> spots):pool_(pool),hp_(hp),types_(types.begin(),types.end()),spots_(spots) {
    if (&hp.Pool()!=&pool || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        spots.size()!=kWorldCells*kWorldCells) throw std::invalid_argument("사제 상태 조회 풀/타입/지도 연결 오류");
}
// 예약/풀 밖 번호와 자산이 아닌 슬롯을 보호한다. dead/void/abstract는 추가로 제외하지 않는다.
std::span<const std::uint8_t> RawPriestState::Raw(Sid sid) const {
    if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::out_of_range("사제 상태 조회 SID 오류");
    const auto raw=pool_.Slot(sid);
    if ((raw[kState]&kInvalidState) || raw[kType]<kFirstAssetTypeNumber || raw[kType]>=types_.size())
        throw std::invalid_argument("사제 상태 조회 raw 자산 오류");
    return raw;
}
// CD의 음수 WORD를 32비트로 부호 확장하며 옆의 stale 두 바이트는 읽지 않는다.
std::int32_t RawPriestState::CurrentHitPoints(Sid sid) const {
    const auto raw=Raw(sid);
    if (pool_.Edition()==OriginalEdition::Patch1078) return std::bit_cast<std::int32_t>(Read(raw,kHp));
    return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(raw,kHp,2)));
}
// HP 경계는 signed 정수 최대 HP/2의 0방향 절삭이다. 저HP일 때에는 지면 좌표를 읽지 않는다.
bool RawPriestState::GroundImmobile(Sid sid) const {
    const auto raw=Raw(sid);
    if ((types_[raw[kType]].flags2&kPriestGenus)==0) return false;
    if (CurrentHitPoints(sid)<hp_.MaxHitPoints(sid)/2) return true;
    const int y=Cell(std::bit_cast<float>(Read(raw,kY))),x=Cell(std::bit_cast<float>(Read(raw,kX)));
    const auto extra=pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U;
    return (spots_[static_cast<std::size_t>(y*kWorldCells+x)]&kSupportedGround)==0 || (raw[extra]&kForcedImmobile)!=0;
}
// 이 외부 wrapper는 genus가 사제가 아니어도 강제 비트를 먼저 검사한다.
bool RawPriestState::Immobile(Sid sid) const {
    const auto raw=Raw(sid);const auto extra=pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U;
    return (raw[extra]&kForcedImmobile)!=0 || GroundImmobile(sid);
}
// CD는 실제 저장 후의 signed WORD로 경계를 검사한다. 절반 경계 전환이 없으면 가상 효과도 없다.
void RawPriestState::SetHitPoints(Sid sid,std::int32_t value,const PriestHitPointMode& mode,const PriestHitPointHooks& hooks) const {
    const auto raw=Raw(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (raw[kType]!=kPriestType || Read(raw,0)!=(patch ? kPatchPriestVtable : kCdPriestVtable))
        throw std::invalid_argument("사제 HP 변경 타입/가상 표 오류");
    const auto oldHp=CurrentHitPoints(sid),half=hp_.MaxHitPoints(sid)/2;
    const auto stored=patch ? value : static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value)));
    const bool wasLow=oldHp<half,transition=(stored<half)!=wasLow;
    const auto owner=raw[patch ? 34 : 32];
    // 실제로 참조하는 소유자 칸과 필요한 가상 효과만 쓰기 전에 검사한다.
    if (transition && mode.authority && wasLow && owner>kPlayerCount) throw std::out_of_range("사제 HP 회복 소유자 오류");
    const bool space=transition && mode.authority && (!wasLow || mode.recoveryAllowed[owner]);
    if (space && (!hooks.unpop || !hooks.repop)) throw std::invalid_argument("사제 HP 변경 공간 효과 누락");
    // 원본 setter 폭 밖의 stale 바이트는 그대로 보존한다.
    const auto write=[&](std::int32_t hp) {
        auto target=pool_.AllocatedBytes(sid);
        // 패치 DWORD/CD WORD의 낮은 바이트만 쓴다.
        for (std::size_t i=0;i<(patch ? 4U : 2U);++i) target[kHp+i]=static_cast<std::uint8_t>(static_cast<std::uint32_t>(hp)>>(8*i));
    };
    write(value);
    if (!transition) return;
    if (!mode.authority) { write(half-(wasLow ? 1 : 0));return; }
    if (wasLow && !mode.recoveryAllowed[owner]) { write(oldHp);return; }
    hooks.unpop(sid,0);
    // 가상 효과 이후 HP/좌표를 다시 읽는다. 원본도 새 HP가 저HP일 때만 현재 위치를 거의 올림한다.
    if (CurrentHitPoints(sid)<hp_.MaxHitPoints(sid)/2) {
        auto target=pool_.AllocatedBytes(sid);
        // x/y에 HP 전환 전용 배율을 더하고 원본 _ftol→float 저장으로 바꾼다.
        for (auto offset:{kX,kY}) {
            const float coordinate=std::bit_cast<float>(Read(target,offset));
            if (!std::isfinite(coordinate) || coordinate<0 || coordinate>=kWorldCells)
                throw std::out_of_range("사제 HP 전환 좌표 범위 오류");
            const auto bits=std::bit_cast<std::uint32_t>(static_cast<float>(static_cast<int>(
                static_cast<double>(coordinate)+static_cast<double>(kSnapBias))));
            // HP 및 다른 좌표 필드 밖은 보존한다.
            for (std::size_t i=0;i<4;++i) target[offset+i]=static_cast<std::uint8_t>(bits>>(8*i));
        }
    }
    hooks.repop(sid,0);
}
}
