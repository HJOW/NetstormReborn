// 캡처 실패의 재전달 금지와 콜백 이후 현재 캡처/표 읽는 순서를 보존한다.
#include "client/GumpInput.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 overlay 대상은 visible/dead만 먼저 검사한다. void는 실제 전체 선택에서 판정한다.
constexpr std::uint32_t kVisible=1,kDead=0x10000,kDialogOverflow=0x13;
}
// 이벤트/위치 처리기는 별도 사각형이나 재귀 전달의 사본을 만들지 않고 같은 노드를 사용한다.
GumpInput::GumpInput(GumpCreateState& create,GumpLifetimeState& lifetime,GumpInputState& state,
    const GumpCollection& collection,const GumpEvents& events,GumpInputHooks hooks)
    :create_(create),lifetime_(lifetime),state_(state),collection_(collection),events_(events),hooks_(std::move(hooks)) {
    if (!hooks_.node) throw std::invalid_argument("Gump 입력 저장소 필요");
}
// 유효 포인터의 원본 계약을 안정된 호스트 토큰으로 표현한다.
GumpCreateNode& GumpInput::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node) throw std::runtime_error("Gump 입력 토큰 없음");return *node;
}
// 캡처/overlay의 직접 호출에는 기본 필터를 넣지 않는다.
std::uint32_t GumpInput::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    (void)Node(token);
    if (!hooks_.handle) throw std::invalid_argument("Gump 입력 처리기 필요");return hooks_.handle(token,event,first,second);
}
// 첫 캡처만 유지하고 기존 캡처를 교체하지 않는다.
void GumpInput::Capture(std::uint32_t token) const { if (!lifetime_.focus) lifetime_.focus=token; }
// 이 API는 단순 대입이 아니라 빈 슬롯 설정/그 외 해제 동작이다.
void GumpInput::SetOverride(std::uint32_t token,bool enabled) const {
    create_.collection.overrideHead=enabled && !create_.collection.overrideHead ? token : 0U;
}
// 표의 활성 구간에서만 중복을 찾고 마지막에 현재 dialog를 갱신한다.
bool GumpInput::AddDialog(std::uint32_t token) const {
    if (std::bit_cast<std::int32_t>(lifetime_.dialogCount)<0) throw std::out_of_range("Gump dialog 개수 음수");
    if (lifetime_.dialogCount>=lifetime_.dialogs.size()) {
        if (!hooks_.overflow) throw std::invalid_argument("Gump dialog 한도 로그 필요");hooks_.overflow(kDialogOverflow);return false;
    }
    // 사용하지 않는 뒤 칸의 같은 토큰은 새 등록을 막지 않는다.
    for (std::uint32_t i=0;i<lifetime_.dialogCount;++i) if (lifetime_.dialogs[i]==token) return false;
    lifetime_.dialogs[lifetime_.dialogCount++]=token;lifetime_.currentDialog=token;return true;
}
// 성공 처리기가 캡처를 바꿔도 호출 전 객체를 반환하지 않고 현재 값을 읽은 뒤 지운다.
std::uint32_t GumpInput::Route(std::uint32_t event,std::uint32_t first,std::uint32_t second,bool includeHidden) const {
    if (lifetime_.focus) {
        if (Handle(lifetime_.focus,event,first,second)) {
            const auto result=lifetime_.focus;lifetime_.focus=0;return result;
        }
        return 0;
    }
    const auto target=lifetime_.currentDialog ? lifetime_.currentDialog : create_.entries[0];
    return target ? events_.Dispatch(target,event,first,second,includeHidden) : 0U;
}
// CD의 별도 helper와 패치의 인라인 부모 조회는 같은 visible 계약이다.
bool GumpInput::ParentsVisible(std::uint32_t token) const {
    std::vector<std::uint32_t> seen;
    // 현재 부모 링크를 읽으며 손상 순환만 호스트 예외로 거부한다.
    for (auto current=token;current;current=Node(current).parent) {
        if (std::find(seen.begin(),seen.end(),current)!=seen.end()) throw std::runtime_error("Gump overlay 부모 순환");
        seen.push_back(current);if (!(Node(current).flags&kVisible)) return false;
    }
    return true;
}
// 실제 후보가 일치하면 처리기 반환이 0이어도 그 자리에서 종료한다.
std::uint32_t GumpInput::Overlay(std::uint32_t event,ScreenPoint point) const {
    (void)event;
    // 표의 뒤 칸은 앞 대상의 위치/선택 콜백 이후 현재 값을 읽는다.
    for (auto token:lifetime_.overlay) {
        if (!token) continue;const auto flags=Node(token).flags;
        if (!(flags&kVisible) || (flags&kDead) || !ParentsVisible(token)) continue;
        const auto rect=collection_.Bounds(token);
        if (rect.left<=point.x && point.x<rect.right && rect.top<=point.y && point.y<rect.bottom && events_.Pick(point,0)==token)
            return Handle(token,state_.overlayEvent,std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y));
    }
    return 0;
}
}
