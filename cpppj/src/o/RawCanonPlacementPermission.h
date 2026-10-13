// 원본 후보 권한 함수의 그래프/소유 관계 접두와 DWORD 작업장 조회 반환값을 복원한다.
#pragma once
#include "o/RawCanonPlacementTerrain.h"

namespace netstorm::o {
// 일반 타입·패턴 배치 허가 처리 중 조회·갱신하는 상태 묶음이다.
struct CanonPlacementPermissionState {
    std::uint32_t graphReady{}; // 00540414 / CD 005207f8: 그래프 조회가 가능한 상태다.
    std::uint32_t editor{},useAlliances{},allowOtherOwners{}; // 편집기·방향 동맹·다른 소유자 허용 전역이다.
    std::array<std::uint32_t,81> alliances{}; // 현재 소유자×9+요청자 순서인 원본 DWORD 관계 표다.
};
// 원본 후보 권한 함수의 그래프/소유 관계 접두와 DWORD 작업장 조회 반환값을 복원한다.
class RawCanonPlacementPermission {
public:
    // Player 작업장/그래프 locator는 필수 경계이며 실제 player/type/좌표를 받고 DWORD를 돌려준다.
    RawCanonPlacementPermission(const SidPool& pool,CanonPlacementPermissionState& state,
        std::function<std::uint32_t(std::uint32_t,std::uint32_t,float,float)> playerAnchor);
    // 00462cb0 / CD 0045bae0: 그래프 비활성→요청자 0→현재 raw 소유 관계→Player 조회 순서다.
    std::uint32_t Candidate(Sid candidate,std::uint32_t owner) const;
    // MayPlace 주변 순회의 바깥 관계 검사를 먼저 적용한다. 중립도 이 바깥 검사를 통과해야 한다.
    std::uint32_t Surrounding(Sid candidate,std::uint32_t owner) const;
    // 004629e0: 편집기/동일 소유자/방향 동맹을 검사한다. 중립/다른 소유자 허용은 여기 없다.
    bool Related(std::uint32_t currentOwner,std::uint32_t owner) const;
    // 최종 소유자 거부는 원본처럼 관계 표를 직접 읽으며 편집기/동일 owner/활성 조건을 붙이지 않는다.
    bool Alliance(std::uint32_t currentOwner,std::uint32_t owner) const;
    // 같은 raw 풀을 사용하는 지형 경계만 연결한다.
    const SidPool& Pool() const;
private:
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // state_: 처리 중 조회·갱신하는 상태 (CanonPlacementPermissionState).
    CanonPlacementPermissionState& state_;
    // playerAnchor_: 플레이어·타입·좌표에 맞는 실제 배치 기준 객체를 찾는 콜백.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,float,float)> playerAnchor_;
};
// 원본 MayPlace의 요청 owner 하위 BYTE를 부호 확장하고 후보 권한만 실제 helper에 연결한다.
CanonPlacementTerrainHooks MakeCanonPermissionTerrainHooks(const SidPool& pool,const RawCanonPlacementPermission& permission,
    CanonPlacementTerrainHooks hooks);
}
