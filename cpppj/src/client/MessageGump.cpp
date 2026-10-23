#include "client/MessageGump.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kMessageVtable=0x00517b60U; // 보호10.78의 메시지 파생 가상 표다.
constexpr std::int32_t kPause=11; // 원문 분배기와 생성자가 사용하는 실제 Pause 종류다.
constexpr std::string_view kPauseText="~E~wGame Paused!"; // PE00516010의 현재 원문 번역 키다.
}
// 선택되지 않은 Challenge 이름 조회는 도달할 때만 요구한다.
MessageGump::MessageGump(const GumpCreateState& create,MessageGumpHooks hooks):hooks_(std::move(hooks)) {
    if (create.collection.edition!=o::OriginalEdition::Patch1078 || !hooks_.node || !hooks_.constructText || !hooks_.challenge || !hooks_.playerName || !hooks_.translate || !hooks_.style || !hooks_.fit)
        throw std::invalid_argument("Pause 메시지 실제 TextGump/이름/번역/측정 연결 누락");
}
// 별도 기본 노드를 만들지 않고 같은 실제 파생을 반환한다.
MessageGumpNode& MessageGump::Node(std::uint32_t token) const {
    auto* current=token ? hooks_.node(token) : nullptr;
    if (!current) throw std::logic_error("Pause 메시지의 현재 공유 노드 없음");
    return *current;
}
// Pause는 시계/음향/idle을 호출하지 않고 기한0의 영구 메시지를 측정한다.
std::uint32_t MessageGump::Construct(std::uint32_t token,std::int32_t kind,std::int32_t player,std::int32_t parameter) const {
    if (kind!=kPause) throw std::logic_error("다른 원본 메시지 종류의 생성 분기 미복원");
    hooks_.constructText(token);
    Node(token).vtable=kMessageVtable;
    Node(token).deadline=0.0;
    Node(token).kind=kind;
    Node(token).player=player;
    Node(token).parameter=parameter;
    if (hooks_.challenge()) {
        if (!hooks_.challengePlayerName) throw std::logic_error("Challenge 메시지의 실제42bbb0 이름 조회 미연결");
        hooks_.challengePlayerName(player);
    } else hooks_.playerName(player);
    const auto text=hooks_.translate(kPauseText); // 번역은 원문 이름 조회 뒤에 실행한다.
    hooks_.style(token,text);
    hooks_.fit(token,0,0);
    return token;
}
}
