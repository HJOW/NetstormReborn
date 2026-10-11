// 원본 Renderer의 한 유효 dirty 사각형 안에서 표시 목록 병합과 kind 0~4 분배를 복원한다.
// 좌표 환산/dirty 순회/배경/장치 잠금은 호출자의 책임이며 수집기의 내부 몸체는 별도 복원 범위다.
#pragma once
#include "client/Renderer.h"
#include "o/RawRenderCollection.h"

namespace netstorm::client {
// 원본 공통 작업 배열은 16바이트 표시 항목 5,000개다.
inline constexpr std::int32_t kDrawSubmissionCapacity=5000;
// 묶음은 첫 항목의 키로 정렬하며 내부 항목은 원래 순서로 그린다.
// count는 그리기 중 바뀔 수 있어 매 차례 읽는다. 항목 배열과 count의 저장소는 호출 동안 살아야 한다.
struct DrawGroupView {
    std::uint32_t token{};
    std::span<const o::RenderCollectionEntry> entries;
    const std::int32_t* count{};
};
// 수집 순서는 UI→건설 표시→묶음→일반 자산/효과→미리보기다. 빈 콜백은 아직 연결하지 않은 수집기다.
// 일반 수집기는 넘긴 span/limit 안에만 쓰고 실제 기록 수를 반환해야 한다.
struct DrawCollectionHooks {
    std::function<std::int32_t(ScreenRect,std::span<o::RenderCollectionEntry>,std::int32_t)> ui;
    std::function<std::int32_t(o::RenderWorldRect,std::span<o::RenderCollectionEntry>,std::int32_t)> construction;
    std::function<std::span<const DrawGroupView>(o::RenderWorldRect,std::int32_t,bool&)> groups;
    std::function<std::int32_t(o::RenderWorldRect,std::span<o::RenderCollectionEntry>,std::int32_t)> objects;
    std::function<std::int32_t(o::RenderWorldRect,std::span<o::RenderCollectionEntry>,std::int32_t)> preview;
};
// 이미 환산한 두 월드 영역을 구분해 전달한다. 미리보기는 클립 전 영역, 나머지는 가시 영역을 받는다.
// dirtyFlags bit 2는 UI만 수집한다. groupTime은 원본 판본별 계산/상수를 호출자가 공급한다.
struct DrawCollectionRequest {
    ScreenRect screen;
    o::RenderWorldRect visible,unclipped;
    std::uint32_t dirtyFlags{};
    std::int32_t groupTime{};
};
// 묶음 수집기의 배경 덮음 관찰을 함께 반환한다. false면 원본은 월드 배경을 다시 그린다.
struct DrawCollectionResult { std::int32_t count{};bool backgroundCovered{}; };
// 용량을 5,000 이하로 제한해 순차 병합한다. 묶음의 kind만 1로 바꾸고 작업 칸의 tail을 보존한다.
// 손상된 반환 개수/빈 묶음/넘치는 묶음은 예외로 거부하며 그 전 단계의 기록은 유지될 수 있다.
DrawCollectionResult CollectDrawEntries(const DrawCollectionRequest& request,
    std::span<o::RenderCollectionEntry> work,const DrawCollectionHooks& hooks);
// 원본 묶음 숨김 전역과 항목별 debug 표시 전역은 각 항목에서 현재 값을 읽는다.
struct DrawDispatchState { bool hideGroups{},debugAfterEach{}; };
// kind 0은 항목, 2/3은 토큰, 4는 tail을 포함한 항목 전체를 받는다. kind 1은 내부를 sprite로 보낸다.
// afterEach는 내부 묶음 항목마다가 아니라 상위 항목마다 한 번 호출하며 알 수 없는 kind에도 적용한다.
struct DrawDispatchHooks {
    std::function<void(const o::RenderCollectionEntry&)> sprite;
    std::function<DrawGroupView(std::uint32_t)> group;
    std::function<void(std::uint32_t)> ui,construction;
    std::function<void(const o::RenderCollectionEntry&)> preview;
    std::function<void()> afterEach;
};
// 정렬된 상위 목록을 종류별로 제출한다. 알 수 없는 kind는 원본 switch처럼 그리기만 건너뛴다.
// 사용한 종류의 필수 콜백이나 묶음 저장소가 없으면 예외로 거부한다. 정렬은 호출자가 별도로 수행한다.
void DispatchDrawEntries(std::span<const o::RenderCollectionEntry> entries,
    const DrawDispatchState& state,const DrawDispatchHooks& hooks);
}
