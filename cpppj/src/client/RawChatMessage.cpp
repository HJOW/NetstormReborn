// 고정 서식·현재 물리 접근과 원문처럼 관측 가능한 앞선 부분 쓰기만 구현한다.
#include "client/RawChatMessage.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <string>

namespace netstorm::client { // 원문 포인터는 호스트에서 같은 소유자의 opaque 토큰으로 표현한다.
namespace { // 순수 물리 조회/복사와 첫 비자기 수신자 탐색의 내부 부품이다.
// 선택된 경계만 요구하고 미복원 송신을 성공으로 바꾸지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Chat 본문 조합의 실제 장치 미연결");return function; }
// 원문 C문자열은 명시 물리 안의 첫 NUL까지 읽는다.
std::string_view Text(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("Chat 본문의 실제 NUL 물리 없음");
    return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
// 같은 토큰의 순수 뷰를 필요한 읽기/쓰기 위치에서 다시 빌린다.
std::span<std::uint8_t> Memory(const ChatMessageHooks& hooks,std::uint32_t token) { return Need(hooks.memory)(token); }
// 실제 비정렬 DWORD의 네 바이트를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::uint32_t at) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Chat 본문 DWORD 물리 범위");std::uint32_t value{};
    for (std::uint32_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(8*index); // 원문의 little endian 네 BYTE다.
    return value;
}
// manager 안의 현재 출력 물리를 반환하고 원문 영역을 임의로 확장하지 않는다.
std::span<std::uint8_t> Output(const ChatMessageHooks& hooks,std::uint32_t manager) {
    const auto bytes=Memory(hooks,manager);if (bytes.size()<=kChatMessageOffset) throw std::out_of_range("Chat 본문 출력 manager 물리 부족");return bytes.subspan(kChatMessageOffset);
}
// 입력 NUL 앞부분과 끝 NUL만 순방향으로 복사하며 뒤쪽 흔적은 보존한다.
void Copy(std::span<std::uint8_t> destination,std::string_view text) {
    text=text.substr(0,text.find('\0'));if (destination.size()<=text.size()) throw std::out_of_range("Chat 본문 복사의 실제 물리 부족");
    for (std::size_t index=0;index<text.size();++index) destination[index]=static_cast<std::uint8_t>(text[index]); // 같은 원문 순방향 문자 복사를 수행한다.
    destination[text.size()]=0;
}
// 실제 현재 출력 끝을 매번 찾은 뒤 같은 물리에 이어 쓴다.
void Append(const ChatMessageHooks& hooks,std::uint32_t manager,std::string_view text) { const auto bytes=Output(hooks,manager);Copy(bytes.subspan(Text(bytes).size()),text); }
//0042d250의 signed count와 첫 비자기 subscriber를 실제 대상 manager에서 찾는다.
std::uint32_t FirstOther(std::uint32_t recipients,std::uint32_t subscriber,const ChatMessageHooks& hooks) {
    if (std::bit_cast<std::int32_t>(Read(Memory(hooks,recipients),0))<=0) return 0;
    const auto buffer=Read(Memory(hooks,recipients),8);std::uint32_t index{};
    do { // 원문은 비교 포인터를 먼저 고정하고 반환할 때 현재 배열 포인터를 다시 읽는다.
        if (Read(Memory(hooks,buffer),index*4U)!=subscriber) return Read(Memory(hooks,Read(Memory(hooks,recipients),8)),index*4U);
        ++index;
    } while (std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(Read(Memory(hooks,recipients),0)));
    return 0;
}
}
// %s 인자는 원문 바이트 그대로 붙이며 전달된 세 실제 호출 서식 외에는 명시 오류로 남긴다.
void WriteChatNameFormat(std::span<std::uint8_t> output,std::string_view pattern,std::string_view name,std::string_view other) {
    pattern=pattern.substr(0,pattern.find('\0'));name=name.substr(0,name.find('\0'));other=other.substr(0,other.find('\0'));std::string result;
    if (pattern=="%s> ") result=std::string(name)+"> ";else if (pattern=="%s ") result=std::string(name)+' ';
    else if (pattern=="%s (to %s)> ") result=std::string(name)+" (to "+std::string(other)+")> ";else throw std::logic_error("Chat 이름의 미지원 원문 서식");
    Copy(output,result);
}
// 이름 시작 위치는 lookup 전 고정하되 front/본문은 하위 callback 뒤 현재 값에서 다시 읽는다.
std::uint32_t FormatChatMessage(std::uint32_t manager,std::uint32_t recipients,std::span<const std::uint8_t> body,std::optional<std::string_view> pattern,std::span<std::uint8_t> nameWork,ChatMessageGlobals globals,const ChatMessageHooks& hooks) {
    if (Text(body).empty()) return 0;
    Output(hooks,manager)[0]=0;const auto count=Read(Memory(hooks,recipients),0);
    if (Read(Memory(hooks,manager),16)&8U) Append(hooks,manager,"~[Iicon.D36]");
    if (Read(Memory(hooks,manager),16)&0x40U) Append(hooks,manager,"~[Iicon.D37]");
    if (Read(Memory(hooks,manager),16)&0x80U) Append(hooks,manager,"~[Iicon.D39]");
    if (Read(Memory(hooks,manager),16)&0x20U) Copy(Output(hooks,manager),"~[Iicon.D41]");
    if (Read(Memory(hooks,manager),12)) Append(hooks,manager,count==1 ? "~[Iicon.D38]" : "~[Iicon.D40]");
    Append(hooks,manager,Text(Memory(hooks,manager).subspan(0x14)));
    const auto nameAt=Text(Output(hooks,manager)).size();if (!pattern) pattern="%s> ";
    if (nameWork.size()<kChatMessageNameBytes) throw std::out_of_range("Chat 대상 이름의 실제256BYTE 필요");nameWork[0]=0;
    if (Read(Memory(hooks,recipients),12)) {
        const auto other=FirstOther(recipients,globals.subscriber,hooks);
        if (other && !Need(hooks.nameForSubscriber)(nameWork,other)) nameWork[0]=0;
    }
    auto name=Need(hooks.currentName)();name=name.substr(0,name.find('\0'));if (name.empty()) name="++Nameless++";
    Need(hooks.formatName)(Output(hooks,manager).subspan(nameAt),*pattern,name,Text(nameWork));
    if (!Read(Memory(hooks,recipients),12)) Append(hooks,manager,Text(Memory(hooks,manager).subspan(0x50)));
    Append(hooks,manager,Text(body));Need(hooks.stripSlots)(Output(hooks,manager));
    const auto bytes=Output(hooks,manager);const auto length=Text(bytes).size()+1;return Need(hooks.deliver)(manager,recipients,bytes.first(length));
}
// 현재 입력의 첫 글자와 this의 현재 직접 대상 표시만 세 서식 선택에 사용한다.
std::uint32_t FormatChatInput(std::uint32_t manager,std::span<const std::uint8_t> input,std::span<std::uint8_t> nameWork,ChatMessageGlobals globals,const ChatMessageHooks& hooks) {
    if (input.empty()) throw std::out_of_range("Chat wrapper의 실제 첫 입력 BYTE 없음");
    if (input.front()=='!') return FormatChatMessage(manager,manager,input.subspan(1),"%s ",nameWork,globals,hooks);
    return FormatChatMessage(manager,manager,input,Read(Memory(hooks,manager),12) ? "%s (to %s)> " : "%s> ",nameWork,globals,hooks);
}
// 하위 반환값을 일반 명령 오류로 올리지 않는 원문 say의0 반환을 유지한다.
std::uint32_t SayChatMessage(std::span<const std::uint8_t> input,std::span<std::uint8_t> nameWork,ChatMessageGlobals globals,const ChatMessageHooks& hooks) {
    if (!globals.current) return 0;if (input.empty()) throw std::out_of_range("Chat say의 실제 첫 입력 BYTE 없음");if (input.front()==' ') return 0;
    if (!input.front()) { Need(hooks.inputVisible)(0);return 0; }
    static_cast<void>(FormatChatInput(globals.current,input,nameWork,globals,hooks));return 0;
}
}
