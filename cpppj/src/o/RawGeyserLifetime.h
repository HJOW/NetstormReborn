// 간헐천의 실제485580 전처리와485540 후처리를 기존 Damageable 수명에 연결한다.
#pragma once
#include "o/SquidPostPop.h"
namespace netstorm::o {
// 가상 기본 프레임/표시와 큰 AI·섬·공통 수명 효과의 실제 경계다.
struct GeyserLifetimeHooks {
    std::function<std::uint32_t(Sid)> defaultFrame; // 가상+78의 현재 TYPE+128 기본 프레임이다.
    std::function<void(Sid)> update; // 기본 프레임을 raw+24에 쓴 뒤 가상+88 한 번이다.
    std::function<void(Sid,std::uint32_t)> registerAi; //416720(sid,0)의 현재 Player AI 제거 요청이다.
    std::function<void(Sid,std::uint32_t)> damageablePre,damageablePost; //44b4b0/44b9e0의 기존 실제 공통 수명이다.
    std::function<void(float,float,std::uint32_t,std::uint32_t)> dropIsland; // 편집 모드에서만46e6c0을 현재 좌표/flags/owner로 호출한다.
};
// 현재 지면 목록은 PostPop/선택기와 같은 저장소이며 복사본이 아니다.
class RawGeyserLifetime {
public:
    RawGeyserLifetime(SidPool& pool,SquidPostPopList& geysers,const std::uint32_t& editor,GeyserLifetimeHooks hooks); // PATCH의 같은 풀/목록/현재 편집 모드 참조를 빌린다.
    bool Handles(Sid sid) const; // 실제50da88 가상 표만 허용한다.
    void PreDestroy(Sid sid,std::uint32_t flags) const; // 프레임 직접 쓰기→표시→현재extra→AI0→목록 압축→Damageable다.
    void PostDestroy(Sid sid,std::uint32_t flags) const; // 현재editor/extra의 섬 정리 뒤 Damageable를 호출한다.
    const SidPool& Pool() const; // 다른 실제 월드의 하위 혼합을 확인한다.
private:
    void Remove(Sid sid) const; // 모든 중복을 제거하며 물리 꼬리는 지우지 않는다.
    SidPool& pool_; // 죽은 상태도 읽을 실제 raw 저장소다.
    SquidPostPopList& geysers_; //5950dc의 원래 현재 목록이다.
    const std::uint32_t& editor_; //5c85a4를 후처리 호출 시 읽는다.
    GeyserLifetimeHooks hooks_; // 선택된 효과 뒤에는 현재 raw/목록을 다시 읽는다.
};
}
