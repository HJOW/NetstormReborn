// 현재 전역을 원문 위치에서만 읽고 실패 때 앞선 설정/큐 효과를 되돌리지 않는다.
#include "client/LanguageRequest.h"
#include "client/LanguageOptions.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //같은 원문 요청을 실제 Client와 독립 장치에서 사용한다.
namespace { //선택되지 않은 장치는 요구하지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("언어 요청의 실제 장치 미연결");return hook; } //원문의 도달 경계에서만 거부한다.
}
// 같은 번호면 문자열 표/설정/큐/카운터 모두 읽거나 수정하지 않는다.
void RequestOriginalLanguage(std::uint32_t requested,const std::int32_t& current,std::uint32_t& refresh,const LanguageRequestHooks& hooks) {
    if (requested==std::bit_cast<std::uint32_t>(current)) return;
    Need(hooks.set)("currentLanguage",OriginalLanguageName(requested));const auto command=Need(hooks.changed)();Need(hooks.queue)(command,0,0);++refresh;
}
// 첫 번째 DWORD는 원문 부모가 사용하지 않으며 수정키는 별도 금지하지 않는다.
bool DispatchSetLanguage(const InputEvent& event,const std::function<void(std::uint32_t)>& request) {
    if ((event.code&0x03ffffffU)!=kSetLanguageCommand) return false;if (!(event.code&InputCode::kRelease)) Need(request)(std::bit_cast<std::uint32_t>(event.y));return true;
}
// Chat의 기존 처리기는 release 표시도 원문처럼 해석하므로 여기서 알림을 지우지 않는다.
bool DispatchLanguageChanged(const InputEvent& event,bool alreadyRouted,const std::function<void(const InputEvent&)>& route) {
    if ((event.code&0x03ffffffU)!=kLanguageChangedCommand) return false;if (!alreadyRouted) Need(route)(event);return true;
}
}
