// PATCH 신전의 실제 가상 PreDestroy와 현재 Player/두 목록/지역 효과 순서를 복원한다.
#pragma once
#include "o/SquidPostPop.h"

namespace netstorm::o {
// 같은 원본 신전/섬/지역/공통 Damageable의 실제 소비자다.
struct VortexPreDestroyHooks {
    std::function<std::int32_t(float,float)> islandNumber; // 0046f530의 현재 좌표 지역이다.
    std::function<bool(std::int32_t)> islandExists; // 0046ee30의 현재 존재 판정이다.
    std::function<std::uint32_t(std::int32_t)> islandTemple; // 0046f260의 현재 신전 SID다.
    std::function<void(std::int32_t)> clearIslandTemple; // 004db890(region,0)의 존재 검사→Temple0→생산 갱신 전체다.
    std::function<void(std::int32_t,std::uint32_t)> islandOwner; // 0046ef10의 실제 소유자/거주 구역 전파다.
    std::function<void(float,float)> regionOwnership; // 00455380의 같은 현재 지도/Factory 투표다.
    std::function<void(Sid,std::uint32_t)> damageablePre; // 0044b4b0의 실제 해방/공통 PreDestroy다.
    std::function<void()> refreshProduction; // 0047d890의 현재 생산 표시 갱신이다.
    std::function<void(std::uint32_t)> assertion; // Vortex.cpp의 정확한 진단 줄 번호 뒤 계속한다.
};
// 실제 Player9와 자산 후처리가 사용한 동일 목록을 수정한다. 새 삭제 결과 사본을 만들지 않는다.
class RawVortexPreDestroy {
public:
    // 입력은 같은 PATCH Pool/Player/temple/residence/outpost와 현재 loading DWORD여야 한다.
    RawVortexPreDestroy(SidPool& pool,std::span<std::uint8_t> players,SquidPostPopList& temples,
        SquidPostPopList& residences,const SquidPostPopList& outposts,const std::uint32_t& loading,VortexPreDestroyHooks hooks);
    // 004dca50 전체의 목록 제거→지역→공통 Damageable→현재 UI→현재 Player4필드 순서다.
    void PreDestroy(Sid sid,std::uint32_t flags) const;
private:
    // 원본 removeAll과 같은 앞방향 압축이며 사용하지 않는 물리 꼬리는 보존한다.
    void Remove(SquidPostPopList& list,std::uint32_t sid) const;
    // 효과 뒤 부모/후보의 실제 현재 float 좌표를 다시 읽는다.
    std::array<float,2> Position(Sid sid) const;
    // 현재 raw 소유자의 해당 PlayerDWORD 주소이며 범위 밖은 호스트 오류다.
    std::size_t Field(Sid sid,std::size_t field) const;
    SidPool& pool_; // 삭제되는 신전과 outpost가 공유하는 실제 풀이다.
    std::span<std::uint8_t> players_; // 원본9×0xb4의 같은 Player 기록이다.
    SquidPostPopList& temples_; SquidPostPopList& residences_; // 현재 두 원본 등록 목록이다.
    const SquidPostPopList& outposts_; // 효과 뒤의 현재 count/owner/좌표를 읽는다.
    const std::uint32_t& loading_; // 005c89b8의 실제 로딩 깊이다.
    VortexPreDestroyHooks hooks_; // 선택된 실제 하위와 진단 소비자다.
};
}
