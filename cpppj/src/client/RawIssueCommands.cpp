// 현재 원문 토큰/CRT 숫자 해석을 공유하며 명령에서 설정 파일이나 게임 모드를 임의로 바꾸지 않는다.
#include "client/RawIssueCommands.h"
#include "o/ConfigInterface.h"
#include "o/RawTextToken.h"
#include <stdexcept>

namespace netstorm::client { // 직접 호출과 실제 Chat 분배기가 같은 원문 본체를 쓴다.
namespace { // 선택된 장치와 원문의 유일한 허용 값만 공통 처리한다.
constexpr std::uint32_t kCheatValue=0x845fedU; //004d5360이 유일하게005cab30에 저장하는 값이다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("초기 점 명령의 선택 장치 미연결");return function; } //호출하지 않는 MP/음향 장치를 미리 요구하지 않는다.
}
// 네 실제 호출을 같은 순서로 수행하며 기존 count/표시 포인터의 부수 효과를 등록기가 소유한다.
void RegisterIssueCommands(const std::function<void(std::string_view,std::uint32_t,std::uint32_t)>& registration) {
    Need(registration)("flash",0x4d5300U,0);Need(registration)("flash",0x4d5300U,0);
    Need(registration)("desc",0x4d5340U,1);Need(registration)("cheatorama",0x4d5360U,1);
}
// 인자 부재·첫 토큰·atol 뒤 실제 효과 순서를 보존한다. 치트 비밀번호를 Config에 저장하지 않는다.
std::optional<std::uint32_t> InvokeIssueCommand(std::uint32_t callback,std::optional<std::string_view> argument,std::uint32_t& cheat,const IssueCommandHooks& hooks) {
    if (callback==0x4d5300U) {
        Need(hooks.flash)(argument ? o::ConfigParseLong(*argument) : 0);Need(hooks.sound)("thunderQuietDistant.wav");return 0;
    }
    if (callback==0x4d5340U) { Need(hooks.description)(-1,argument,0);return 0; }
    if (callback==0x4d5360U) {
        if (!argument) throw std::invalid_argument("cheatorama 원문의 실제 인자 포인터 없음");
        if (static_cast<std::uint32_t>(o::ConfigParseLong(o::ReadRawTextToken(*argument).text))==kCheatValue) cheat=kCheatValue;
        return 0;
    }
    return std::nullopt;
}
}
