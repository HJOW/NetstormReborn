#include "client/ConfigurationCommandInput.h"
#include "o/OriginalText.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { //기존 장치의 실패/부분 효과를 되돌리거나 성공으로 대체하지 않는다.
namespace { //원문 등록 표와 도달한 부모의 연결 검사를 한곳에 모은다.
constexpr std::array<std::pair<std::string_view,std::uint32_t>,3> kCommands{{{"changepal",kChangePalCommand},{"changepriority",kChangePriorityCommand},{"setforttype",kSetFortTypeCommand}}}; //추측 별칭이나 기본 메뉴 항목은 추가하지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("현재설정일반명령의실제부모미연결");return function; } //실제 선택되지 않은 장치는 요구하지 않는다.
std::string OriginalString(std::string_view text) { return std::string(text.substr(0,text.find('\0'))); } //Config 알림이 공유 고리를 바꾸기 전에 첫 NUL 앞 요청을 소유한다.
}
std::uint32_t ConfigurationCommandScriptCode(std::string_view name) { //원문stricmp와 같은 ASCII 이름 및 미등록ffffffff다.
    const auto lower=o::AsciiLower(name);for (const auto& [text,code]:kCommands) if (lower==text) return code; //실제 세 등록 이름만 순서대로 비교한다.
    return 0xffffffffU;
}
bool IsConfigurationCommand(std::uint32_t code) { //상위 사건 플래그를 제외한26비트 명령 전체를 비교한다.
    code&=0x03ffffffU;for (const auto& [name,value]:kCommands) if (code==value) return true; //다른 장치의 같은 낮은 BYTE는 허용하지 않는다.
    return false;
}
NativeThreadPriorityRequest PlanNativeThreadPriority(std::int32_t wanted,std::int32_t currentOs) { //범위 밖 raw 요청도 NORMAL을 매프레임 다시 요청하는 원문 효과를 유지한다.
    const auto level=(currentOs==1 || currentOs==2) ? 1 : currentOs==15 ? 2 : 0; //현재 OS 값만0/1/2의 비교용 등급으로 바꾼다.
    return {wanted,level,wanted==1 ? 1 : wanted==2 ? 15 : 0,level!=wanted};
}
bool DispatchConfigurationCommandInput(InputEvent event,ConfigurationCommandReads reads,const ConfigurationCommandHooks& hooks) { //별도 pause/console/월드 존재 관문은 원문에 없다.
    const auto code=event.code&0x03ffffffU;if (!IsConfigurationCommand(code)) return false;if (event.code&InputCode::kRelease) return true;
    const auto parameter=std::bit_cast<std::uint32_t>(event.y); //첫 번째 인자를 추측 사용하거나 낮은 WORD로 자르지 않는다.
    if (code==kChangePalCommand) {
        const std::string_view key=Need(reads.battle)() ? "battlePal" : Need(reads.challenge)() ? "chalPal" : "fortPal"; //Set 알림 전 선택한 키는 뒤 모드 변경에도 유지한다.
        const auto requested=OriginalString(Need(hooks.text)(parameter));Need(hooks.set)(key,requested);
        const auto selected=OriginalString(Need(hooks.get)(key));Need(hooks.palette)(selected); //Set의 입력 대신 현재 설정을 다시 읽어 실제 파일 reader에 넘긴다.
    } else if (code==kChangePriorityCommand) {
        reads.highPriority=std::bit_cast<std::int32_t>(parameter); //OS 호출 실패 전에도 원문 공유 DWORD 쓰기는 이미 관측 가능하다.
        Need(hooks.thread)(parameter==1 ? 1 : parameter==2 ? 15 : 0); //원문435c80은 범위 밖 값을 clamp하지 않고 NORMAL로 요청한다.
        const auto assignment=std::string("highPriorityValue = \"")+std::to_string(reads.highPriority)+'"';Need(hooks.config)(assignment); //OS 콜백 뒤 현재 공유값을 signed%d로 다시 읽는다.
    } else {
        if (!Need(reads.editor)()) Need(hooks.assertion)(2012); //원문 진단이 정상 복귀하면 다음 State 쓰기를 계속한다.
        auto state=Need(hooks.state)(Need(reads.owner)());if (state.size()<28) throw std::out_of_range("SetFortType의실제State28BYTE범위");state[0x11]=static_cast<std::uint8_t>(parameter); //Meta가 아닌 실제 State의 fort type BYTE다.
        Need(hooks.notify)(Need(reads.owner)()); //State 생성/확장/진단에서 변경된 현재 Template owner를 다시 조회한다.
    }
    return true;
}
}
