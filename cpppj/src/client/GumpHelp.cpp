// 도움말의 메뉴 경계를 호출하며 앞선 콜백이 바꾼 현재 문자열/전역/루트를 읽는다.
#include "client/GumpHelp.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 사건의 코드 마스크다. release는 코드와 별개로 최상위 비트에서 판정한다.
constexpr std::uint32_t kHelpEventMask=0x03ffffff;
}
// 실행하지 않는 분기의 메뉴/소리 경계는 미리 요구하지 않는다.
GumpHelp::GumpHelp(GumpCreateState& create,GumpHelpState& state,GumpHelpHooks hooks)
    :create_(create),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.node) throw std::invalid_argument("Gump 도움말 노드 저장소 필요");
}
// 없는 토큰은 원본 유효 this 포인터 계약 밖이므로 호스트 예외로 거부한다.
GumpCreateNode& GumpHelp::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node) throw std::runtime_error("Gump 도움말 토큰 없음");return *node;
}
// 주제 조립→메뉴 시작→현재 제목→현재 본문/이벤트→현재 루트 부착→소리 순서다.
std::uint32_t GumpHelp::Handle(std::uint32_t token,std::uint32_t event,
    std::uint32_t first,std::uint32_t second) const {
    auto& node=Node(token);
    if ((event&kHelpEventMask)!=InputCode::kRightButton || (event&InputCode::kRelease) || !node.alternateTextPointer) return 0;
    if (!hooks_.text) throw std::invalid_argument("Gump 도움말 문자열 경계 필요");
    const auto text=hooks_.text(node.alternateTextPointer);
    state_.topic=text.substr(0,text.find('\0'))+"Help";
    if (!hooks_.beginMenu) throw std::invalid_argument("Gump 도움말 메뉴 시작 경계 필요");
    hooks_.beginMenu(0);
    if (!hooks_.title) throw std::invalid_argument("Gump 도움말 제목 경계 필요");
    hooks_.title(node.textPointer ? node.textPointer : node.alternateTextPointer);
    if (!hooks_.entry) throw std::invalid_argument("Gump 도움말 메뉴 항목 경계 필요");
    const auto extra=create_.collection.edition==o::OriginalEdition::Patch1078 ?
        std::optional<std::uint32_t>{0} : std::nullopt;
    hooks_.entry(state_.body,state_.event,state_.topic,1,1,extra);
    if (!hooks_.attachMenu) throw std::invalid_argument("Gump 도움말 메뉴 부착 경계 필요");
    hooks_.attachMenu(std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second),create_.entries[0]);
    if (!hooks_.sound) throw std::invalid_argument("opengump.wav 재생 경계 필요");
    hooks_.sound("opengump.wav");return 1;
}
// 기본 ButtonGump에서 거절하지 않고 도달한 사건만 원본 도움말 분기로 전달한다.
void BindGumpHelp(ButtonGumpHooks& buttons,const GumpHelp& help) {
    buttons.baseEvent=[&help](std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
        return help.Handle(token,event,first,second);
    };
}
}
