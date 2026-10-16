// 화면 픽셀/카메라 환산은 개별 Draw 몸체가 수행하며 수집/정렬은 월드 좌표를 사용한다.
#include "client/ConstructionVisualRenderer.h"
#include "client/DrawSort.h"
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 클라이언트의 원본 비교 키를 그대로 사용한다. 수집 순서 보존 옵션은 정렬 전역이 꺼진 상태에 해당한다.
std::int32_t DrawVisibleConstruction(o::ConstructionVisualSystem& system,o::RenderWorldRect view,
    std::span<o::RenderCollectionEntry> work,bool sort) {
    if (work.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::length_error("건설 표시 작업 버퍼 크기 오류");
    return DrawVisibleScene(system,{{},view,view,0,0},work,{}, {},{},sort).count;
}
// 수집기는 호출 동안만 system을 참조하며 정렬은 같은 풀의 판본을 따른다.
DrawCollectionResult DrawVisibleScene(o::ConstructionVisualSystem& system,const DrawCollectionRequest& request,
    std::span<o::RenderCollectionEntry> work,DrawCollectionHooks collection,
    const DrawDispatchState& state,DrawDispatchHooks dispatch,bool sort) {
    return DrawVisibleScene(system.Rendering(),request,work,std::move(collection),state,std::move(dispatch),sort);
}
// 원본 collector의 단일 슬롯 목록을 처리해 효과 종류별 정렬/번호 체계를 만들지 않는다.
DrawCollectionResult DrawVisibleScene(o::RenderProcessSystem& system,const DrawCollectionRequest& request,
    std::span<o::RenderCollectionEntry> work,DrawCollectionHooks collection,
    const DrawDispatchState& state,DrawDispatchHooks dispatch,bool sort) {
    collection.construction=[&system](o::RenderWorldRect view,std::span<o::RenderCollectionEntry> output,std::int32_t limit) {
        return system.Collect(view,output,limit);
    };
    dispatch.construction=[&system](std::uint32_t token) { system.DrawToken(token); };
    const auto result=CollectDrawEntries(request,work,collection);
    auto entries=work.first(static_cast<std::size_t>(result.count));
    if (sort) SortDrawEntries(entries,system.Pool().Edition());
    DispatchDrawEntries(entries,state,dispatch);return result;
}
}
