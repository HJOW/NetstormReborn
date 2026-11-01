//4d77f3/4d7f45와 뒤 명시 제외 분기의 실제 입력을 보존한다.
#include "client/ScenarioEffectInput.h"
#include "o/OriginalText.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 메뉴/대본이 공유하는 원문 명령이다.
namespace { // 실제 도달한 표시 경계만 요구한다.
// 원문 이름표5427cc/5429cc/542acc의 서로 다른 명령 번호다.
constexpr std::uint32_t kSmoke=0x02000035U,kLightning=0x020000b6U,kRandomGuns=0x020000f6U,kDumpSquids=0x0200011bU; // DumpSquids는00543418→00542b3c의 실제 등록 이름/명시 제외 코드다.
// 호출되지 않는 장치는 허용하지만 선택된 부모의 누락은 조용한 성공으로 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("시나리오 표시의 실제 장치 미연결");return function; }
}
// 원문 등록 철자를 사용하며 다른 도움말/진단 명령의 별칭을 추가하지 않는다.
std::uint32_t ScenarioEffectScriptCode(std::string_view name) {
    const auto key=o::AsciiLower(name);
    if (key=="startsmoke") return kSmoke;
    if (key=="lightning") return kLightning;
    if (key=="createrandomguns") return kRandomGuns;
    if (key=="dumpsquids") return kDumpSquids; // 일반 대본 입력은 원문처럼 무동작이다. 개발자 F7의 실제 객체/프로세스 덤프와 구별한다.
    return 0xffffffffU;
}
// UI 버튼 허용과 입력 소비가 같은 네 코드를 사용한다.
bool IsScenarioEffectCommand(std::uint32_t command) { command&=0x03ffffffU;return command==kSmoke || command==kLightning || command==kRandomGuns || command==kDumpSquids; }
// 원문 두 생성 호출에는 메뉴/소유자/Edit/미션 관문이 없다.
bool DispatchScenarioEffectInput(const InputEvent& event,const ScenarioEffectInputHooks& hooks) {
    const auto code=event.code&0x03ffffffU;if (!IsScenarioEffectCommand(code)) return false;
    if ((event.code&InputCode::kRelease) || (code==kRandomGuns || code==kDumpSquids)) return true; // 두 이름은 실제 일반 입력에서 효과 없이 제외하며 조회/덤프 장치를 호출하지 않는다.
    if (code==kSmoke) Need(hooks.smoke)(std::bit_cast<std::uint32_t>(event.y),0,0);
    else {
        const auto width=Need(hooks.width)();const auto parent=Need(hooks.technology)(); // 기술 조회 효과가 폭을 바꿔도 원문 선행 push 값을 사용한다.
        Need(hooks.lightning)(parent,{0,60},{width,60},20.0F);
    }
    return true;
}
}
