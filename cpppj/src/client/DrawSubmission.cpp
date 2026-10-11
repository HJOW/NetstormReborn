// 근거: 10.78 004994b0 / CD 004575e0의 유효 dirty 사각형 내부, CD 묶음 순회 00456900.
#include "client/DrawSubmission.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 연결된 종류의 그리기 경계가 빠졌다면 빈 함수를 호출하기 전에 계약 오류를 알려 준다.
template<class Hook> const Hook& Required(const Hook& hook) {
    if (!hook) throw std::invalid_argument("공통 표시 분배 콜백 누락");return hook;
}
}
// 실제 수집기는 남은 칸만 전달받으며 음수/용량 밖 반환은 다음 수집기로 넘기지 않는다.
DrawCollectionResult CollectDrawEntries(const DrawCollectionRequest& request,
    std::span<o::RenderCollectionEntry> work,const DrawCollectionHooks& hooks) {
    work=work.first(std::min(work.size(),static_cast<std::size_t>(kDrawSubmissionCapacity)));
    DrawCollectionResult result;
    // 빈 수집기도 건너뛰되 연결된 수집기는 남은 용량 0에서 호출되는 원본 순서를 유지한다.
    const auto append=[&](const auto& hook,auto rectangle) {
        if (!hook) return;
        const auto remaining=work.subspan(static_cast<std::size_t>(result.count));
        const auto count=hook(rectangle,remaining,static_cast<std::int32_t>(remaining.size()));
        if (count<0 || static_cast<std::size_t>(count)>remaining.size()) throw std::length_error("표시 수집 반환 개수 오류");
        result.count+=count;
    };
    append(hooks.ui,request.screen);
    if (request.dirtyFlags&4U) return result;
    append(hooks.construction,request.visible);
    if (hooks.groups) {
        const auto groups=hooks.groups(request.visible,request.groupTime,result.backgroundCovered);
        if (groups.size()>work.size()-static_cast<std::size_t>(result.count)) throw std::length_error("표시 묶음 용량 초과");
        // 묶음의 첫 키와 8바이트 헤더 토큰을 복사하며 사용하지 않는 마지막 바이트는 쓰지 않는다.
        for (const auto& group:groups) {
            if (group.entries.empty() || !group.count) throw std::invalid_argument("표시 묶음 저장소 누락");
            const auto& first=group.entries.front();auto& entry=work[static_cast<std::size_t>(result.count++)];
            entry.x=first.x;entry.y=first.y;entry.depth=first.depth;entry.token=group.token;entry.kind=1;
        }
    }
    append(hooks.objects,request.visible);
    append(hooks.preview,request.unclipped);
    return result;
}
// 상위 항목과 묶음 내부의 두 순회를 구분한다. 내부 kind/tail은 sprite 그리기 경계가 해석한다.
void DispatchDrawEntries(std::span<const o::RenderCollectionEntry> entries,
    const DrawDispatchState& state,const DrawDispatchHooks& hooks) {
    // 원본은 현재 목록의 항목 수만 처리하며 그리는 도중 새로 수집하지 않는다.
    for (const auto& entry:entries) {
        switch (entry.kind) {
        case 0: Required(hooks.sprite)(entry);break;
        case 1:
            if (!state.hideGroups) {
                const auto group=Required(hooks.group)(entry.token);
                if (!group.count) throw std::invalid_argument("표시 묶음 개수 저장소 누락");
                // 그리기 콜백이 현재 개수를 줄이거나 늘릴 수 있다. 원래 배열 안에서만 계속 진행한다.
                for (std::int32_t index=0;index<*group.count;++index) {
                    if (static_cast<std::size_t>(index)>=group.entries.size()) throw std::length_error("표시 묶음 개수 초과");
                    Required(hooks.sprite)(group.entries[static_cast<std::size_t>(index)]);
                }
            }
            break;
        case 2: Required(hooks.ui)(entry.token);break;
        case 3: Required(hooks.construction)(entry.token);break;
        case 4: Required(hooks.preview)(entry);break;
        default: break;
        }
        if (state.debugAfterEach) Required(hooks.afterEach)();
    }
}
}
