// 후보 권한은 조회 결과의 비트값을 그대로 보존하며 지형 어댑터에서만 bool로 바꾼다.
#include "o/RawCanonPlacementPermission.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 현재 raw의 타입 번호·좌표 DWORD·소유자 BYTE 오프셋이다.
constexpr std::size_t kType=10,kX=14,kY=18,kPatchOwner=34,kCdOwner=32;
// 비정렬 원본 float 비트값을 DWORD로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 낮은 바이트부터 조립하며 NaN/부호 있는 0을 산술 변환하지 않는다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(value);
}
}
// 실제 Player 조회를 임의 허용으로 대체하지 않도록 필수 경계를 검사한다.
RawCanonPlacementPermission::RawCanonPlacementPermission(const SidPool& pool,CanonPlacementPermissionState& state,
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,float,float)> playerAnchor)
    :pool_(pool),state_(state),playerAnchor_(std::move(playerAnchor)) {
    if (!playerAnchor_) throw std::invalid_argument("배치 권한 Player 조회 경계 누락");
}
// 원본 인덱스 산술은 DWORD로 감기며 주소 밖의 손상된 관계 표 읽기는 진단한다.
bool RawCanonPlacementPermission::Related(std::uint32_t currentOwner,std::uint32_t owner) const {
    if (state_.editor || currentOwner==owner) return true;
    if (!state_.useAlliances) return false;
    const std::uint32_t index=currentOwner*9U+owner;
    if (index>=state_.alliances.size()) throw std::out_of_range("배치 권한 관계 표 범위 오류");
    return state_.alliances[index]!=0;
}
// 중립/다른 소유자 허용도 원본처럼 관계 조회 이후에 적용한다.
std::uint32_t RawCanonPlacementPermission::Candidate(Sid candidate,std::uint32_t owner) const {
    if (!state_.graphReady) return 0;
    if (!owner) return 1;
    const auto raw=pool_.Slot(candidate);
    const auto currentOwner=raw[pool_.Edition()==OriginalEdition::Patch1078 ? kPatchOwner : kCdOwner];
    if (!Related(currentOwner,owner) && currentOwner && !state_.allowOtherOwners) return 0;
    return playerAnchor_(owner,raw[kType],Coordinate(raw,kX),Coordinate(raw,kY));
}
// 연결 시 동일 풀인지 확인할 참조를 반환한다.
const SidPool& RawCanonPlacementPermission::Pool() const { return pool_; }
// 캡처한 요청자의 원래 DWORD는 유지하고 helper 인자만 signed BYTE 계약으로 바꾼다.
CanonPlacementTerrainHooks MakeCanonPermissionTerrainHooks(const SidPool& pool,const RawCanonPlacementPermission& permission,
    CanonPlacementTerrainHooks hooks) {
    if (&pool!=&permission.Pool()) throw std::invalid_argument("배치 권한/지형 SID 풀 불일치");
    hooks.candidatePermission=[&permission](const CanonPlacementQuery& query,Sid sid) {
        const auto owner=static_cast<std::int32_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(query.owner)));
        return permission.Candidate(sid,std::bit_cast<std::uint32_t>(owner))!=0;
    };return hooks;
}
}
