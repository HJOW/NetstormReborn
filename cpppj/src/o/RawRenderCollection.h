// Particle.cpp의 표시 영역 환산·가시 영역 교차·16바이트 그리기 항목 수집을 복원한다.
#pragma once
#include "o/RawConstructionLifetime.h"

namespace netstorm::o {
// 화면 픽셀이 아닌 월드 좌표의 사각형이다. 역전 영역은 교차하지 않으며 내부의 점/선 영역은 허용한다.
struct RenderWorldRect {
    float left{},top{},right{},bottom{}; // 원본 collector의 네 float 인자다.
};
// 원본 공통 16바이트 draw entry다. token은 객체 포인터 대신 쓰며 마지막 BYTE의 쓰기는 종류별로 다르다.
struct RenderCollectionEntry {
    float x{},y{}; // kind 3은 부모 월드 좌표/y+0.001, kind 2는 Gump의 로컬 좌표다.
    std::uint32_t token{}; // 원본 +8 객체 포인터의 호스트 식별자다.
    std::int16_t depth{}; // 원본 +0xc: 등록 zOrder DWORD의 하위 WORD다.
    std::uint8_t kind{},tail{}; // +0xe 종류와 +0xf 부가 바이트다. 건설은 tail 보존, UI는 0으로 쓴다.
};
// 목록 토큰에 대응하는 현재 등록 필드를 읽는다. null은 생성 중인 호스트 객체를 건너뛰는 확장 계약이다.
using RenderFieldsLookup=std::function<const RenderProcessFields*(std::uint32_t)>;
// 실제 목록/부모 raw/배율을 읽어 원본 슬롯 순서의 가시 표시 항목을 만든다. 정렬/출력은 별도 단계다.
class RawRenderCollection {
public:
    // 풀/전역/목록/lookup의 자료는 collector보다 오래 살아야 한다. 같은 풀과 비어 있지 않은 lookup만 받는다.
    RawRenderCollection(const SidPool& pool,const ConstructionVisualState& state,const RawRenderRegistry& registry,RenderFieldsLookup lookup);
    // 00487a40 / CD 0045c9e0: 부모 head 확인 뒤 패치는 확장 정밀도 역수×좌표, CD는 좌표÷배율이다.
    // 배율은 유한 비영 float여야 한다. 실제 필드와 계산 결과의 NaN/무한대는 교차 판정에 그대로 전달한다.
    RenderWorldRect Bounds(const RenderProcessFields& fields) const;
    // 00487740 / CD collector 인라인: 네 변의 엄격한 교차로 경계만 닿은 영역을 제외한다.
    // 기본은 패치판이다. CD는 x87 분기 차이로 view.top/bounds.bottom의 NaN을 일부 허용한다.
    static bool Intersects(RenderWorldRect view,RenderWorldRect bounds,OriginalEdition edition=OriginalEdition::Patch1078);
    // 00487f10 / CD 0045c4b0: 첫 슬롯부터 limit개까지 쓴다. limit<=0이면 0이며 나머지 출력 바이트는 보존한다.
    // limit>output.size는 변경 전에 거부한다. 별도의 부모 free/dead/void/abstract 필터는 추가하지 않는다.
    std::int32_t Collect(RenderWorldRect view,std::span<RenderCollectionEntry> output,std::int32_t limit) const;
private:
    // +8 DWORD 부모를 같은 풀에서 읽고 원본 getter의 head!=0 조건을 확인한다.
    std::span<const std::uint8_t> Parent(const RenderProcessFields& fields) const;
    const SidPool& pool_; // 판본별 raw 부모 자료다.
    const ConstructionVisualState& state_; // 현재 좌표 배율이다.
    const RawRenderRegistry& registry_; // 구멍을 포함한 표시 목록이다.
    RenderFieldsLookup lookup_; // 호스트 토큰의 현재 필드 조회다. 자료를 변경하지 않아야 한다.
};
}
