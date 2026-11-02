// 원문raw child의두독립if와각선택지점의부분쓰기/실패를보존한다.
#include "client/AiDebugCommandInput.h"
#include "o/OriginalText.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 현재Player/brain/진단저장소만빌리고장치를열지않는다.
namespace { // 실제선택된하위물리만요구한다.
template<class F> const F& NeedAiInput(const F& hook) { if (!hook) throw std::logic_error("AI 진단 입력 실제 경계 미연결");return hook; } //빈writer를정상토글로세지않는다.
std::uint32_t AiInputWord(std::span<const std::uint8_t> bytes,std::size_t offset) { //원문DWORD읽기폭을보존한다.
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("AI 진단 현재 DWORD 물리범위");std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[offset+index])<<(8*index); //현재BYTE만읽는다.
    return value;
}
}
// 원문이름표에는두이름만있으며임의별칭을만들지않는다.
std::uint32_t AiDebugCommandScriptCode(std::string_view name) { const auto key=o::AsciiLower(name.substr(0,name.find('\0')));if (key=="togglereporting") return kAiReportingCommand;if (key=="toggledebug") return kDebugSwitchCommand;return 0xffffffffU; }
// 분류시에도원문과같이최상위수식/뗌비트만제거한다.
bool IsAiDebugCommand(std::uint32_t code) { code&=0x03ffffffU;return code==kAiReportingCommand || code==kDebugSwitchCommand; }
// 같은현재본문+0xb00과실제진단DWORD만쓰며원문추가관문을합성하지않는다.
void ApplyAiDebugCommandState(InputEvent event,const AiDebugInputState& state) {
    const auto code=event.code&0x03ffffffU;
    if (code==kAiReportingCommand && !NeedAiInput(state.metamap)() && event.y>0) {
        const auto owner=static_cast<std::uint32_t>(event.y);if (owner>=9) throw std::out_of_range("AI Reporting Player owner 물리범위");
        const auto token=AiInputWord(NeedAiInput(state.player)(owner),0xb0);
        if (token) {
            auto body=NeedAiInput(state.body)(token);const auto value=AiInputWord(body,0xb00)==0 ? 1U : 0U;
            for (std::size_t index=0;index<4;++index) body[0xb00+index]=static_cast<std::uint8_t>(value>>(8*index)); //앞물리읽기성공뒤동일DWORD전체비트를덮어쓴다.
        }
    }
    if (code==kDebugSwitchCommand) {
        const auto address=std::bit_cast<std::uint32_t>(event.y);const auto value=NeedAiInput(state.readDebug)(address);NeedAiInput(state.writeDebug)(address,value==29 ? 99U : 29U);
    }
}
// 뗌은모드/Player/포인터resolver를읽지않고소비만한다.
bool DispatchAiDebugCommandInput(InputEvent event,const AiDebugInputState& state) { if (!IsAiDebugCommand(event.code)) return false;if (!(event.code&InputCode::kRelease)) ApplyAiDebugCommandState(event,state);return true; }
}
