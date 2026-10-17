// PATCH 썬 바리케이드의 연결 정리·이웃 갱신·가상 preDestroy를 같은 SID 풀에 적용한다.
#pragma once
#include "o/SidPool.h"
#include <array>
#include <functional>

namespace netstorm::o {
// 실제 현재 이웃/연결선/Pop/Regular/Damageable 몸체를 제공하는 효과 경계다.
struct FenceLifetimeHooks {
    std::function<Sid(Sid,std::uint32_t,bool)> neighbor; // allied=true는455fb0, false는456220이다.
    std::function<void(Sid,Sid)> clearMarkers; // 실제4566d0의 두 끝점 표식 삭제다.
    std::function<void(std::array<float,4>,std::uint32_t,std::uint32_t,Sid,Sid)> clearLine; // 455b20(mode0)의 현재 좌표/소유자/끝점이다.
    std::function<void(Sid,std::uint32_t)> unpop,repop; // 현재 가상+48(0)/+4c(40)다.
    std::function<void(Sid)> rebuild; // 현재457460의 동맹/사거리/연결 재생성이다.
    std::function<bool()> reserveRegular; // 실제 new(28)의 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 이웃 부모에 event2/payload0.1을 부착한다.
    std::function<void(Sid,std::uint32_t)> damageable; // 현재44b4b0의 같은 flags 전처리다.
};
// 4577f0/4578f0/457990의 원본 순서·스냅샷·현재 raw 재조회를 보존한다.
class RawFenceLifetime {
public:
    RawFenceLifetime(SidPool& pool,FenceLifetimeHooks hooks); // 실제 PATCH 풀을 빌리며 효과 대상은 더 오래 살아야 한다.
    void Clear(Sid parent) const; // extra9가 없을 때 네 방향의 표식/연결선을 현재 인수로 지운다.
    void Refresh(Sid parent) const; // 동맹4스냅샷→자기 정리/Unpop→현재 같은 타입 이웃 갱신→Repop이다.
    void PreDestroy(Sid parent,std::uint32_t flags) const; // Refresh→일반4스냅샷→첫2Regular→Damageable이다.
    const SidPool& Pool() const; // 다른 월드의 객체 효과를 혼용하지 않게 한다.
private:
    std::span<const std::uint8_t> Raw(Sid sid) const; // 원본의 예약 슬롯도 범위 안에서만 읽는다.
    std::array<float,4> Endpoints(Sid first,Sid second) const; // 효과 직전의 현재 두 좌표를 읽는다.
    SidPool& pool_; // 삭제/재생성이 변경하는 단일 raw 풀이다.
    FenceLifetimeHooks hooks_; // 계산을 대신하지 않는 실제 월드 효과다.
};
}
