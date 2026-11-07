// 원문 사용자 입력의 현재 WORD 해석/단락 평가와 객체 반환 무시를 보존한다.
#include "client/VortexCommandInput.h"
#include "o/OriginalText.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 화면 갱신/World 교체는 이 분배기의 효과가 아니다.
namespace { // 도달한 실제 원문 효과만 요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("신전/Descend 입력의 실제 장치 미연결");return function; } // 미선택 경계는 요구하지 않는다.
constexpr std::array<std::pair<std::string_view,std::uint32_t>,4> kVortexCommands{{{"wipeout",0x0300006f},{"ascend",0x0300007c},{"descend",0x030000a7},{"viewmeta",0x02000013}}}; //542f00/542f68/5430c0/542c20의 실제 이름표다.
}
std::uint32_t VortexCommandScriptCode(std::string_view name) { // 이름표는 원문처럼 ASCII 대소문자만 무시한다.
    const auto value=o::AsciiLower(name);
    for (const auto& [text,code]:kVortexCommands) if (text==value) return code;return 0xffffffffU; // 원문 네 이름 이외에는 공통 조회를 계속한다.
}
bool DispatchVortexCommandInput(const InputEvent& event,const std::function<void(o::Sid,std::uint32_t,std::uint16_t)>& command) { // 보호 PE5428b4/5428e8/542990의 명령이다.
    const auto code=event.code&0x03ffffffU;if (code!=0x0300006fU && code!=0x0300007cU && code!=0x030000a7U) return false;
    if (event.code&InputCode::kRelease) return true;
    const auto parameter=std::bit_cast<std::uint32_t>(event.y);const o::Sid target{static_cast<std::uint16_t>(parameter)};
    if (target.value) Need(command)(target,event.code,static_cast<std::uint16_t>(parameter>>16));return true;
}
bool DispatchViewMetaInput(const InputEvent& event,std::uint32_t command,const ViewMetaInputHooks& hooks) { // 서버 검증이 끝나도 현재 선택 전역은 쓰지 않는다.
    if ((event.code&0x03ffffffU)!=command) return false;if (event.code&InputCode::kRelease) return true;
    const auto server=event.y>>16;
    if (server<-1 || server>=Need(hooks.serverCount)()) Need(hooks.invalidServer)();
    else Need(hooks.descend)(static_cast<std::uint16_t>(std::bit_cast<std::uint32_t>(event.y)));return true;
}
}
