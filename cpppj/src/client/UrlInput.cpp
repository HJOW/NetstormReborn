// 원문4d7b09..4d7b1e의 모드 독립/두번째 인자/뗌 거부를 유지한다.
#include "client/UrlInput.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 링크 정책을 새로 만들지 않는 현재 입력 연결이다.
bool DispatchUrlInput(const InputEvent& event,const std::function<std::string_view(std::uint32_t)>& text,
    const std::function<void(std::string_view)>& activate) {
    if ((event.code&0x03ffffffU)!=kUrlCommand) return false;
    if (event.code&InputCode::kRelease) return true;
    if (!text || !activate) throw std::logic_error("URL 일반 입력의 현재 문자열/링크 분배기 미연결");
    activate(text(std::bit_cast<std::uint32_t>(event.y)));return true;
}
}
