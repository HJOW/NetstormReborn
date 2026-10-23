// 원본0x28BYTE 문자 편집 버퍼를 StyleTextBuffer와 구별해 복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client {
// 원본 열DWORD다. 메모리 토큰은 같은 가상주소 공간의 DWORD 주소이며 내부오프셋 합산을 허용한다.
struct TextEntryBufferState {
    std::uint32_t vtable{},offsets{}; // +0/+4의 할당가상표와 줄offset 배열 주소다.
    std::int32_t offsetCapacity{},lineCount{}; // +8/+c의 실제DWORD 칸 수와 현재 표시 줄 수다.
    std::uint32_t dirty{}; // +10이 비0이면 LF 줄offset을 다시 만든다.
    std::int32_t selectionBegin{},selectionEnd{},length{},capacity{}; // +14/+18/+1c/+20의 선택/본문길이/용량이다.
    std::uint32_t data{}; // +24의 현재 NUL 문자열 저장 주소다.
};
// alloc/free/resize는 실제소유 저장소이며 bytes는 주소의 남은범위를 반환하는 순수조회다.
struct TextEntryBufferHooks {
    std::function<std::uint32_t(std::uint32_t)> allocate; // malloc 및 가상+0이다.0은 원문할당실패다.
    std::function<void(std::uint32_t)> release; // free 및 가상+4다.0도원문대로전달한다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> resize; // 가상+8이다.실패시원래저장소를보존한다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> bytes; // base 또는base+offset의 현재안정범위다.
    std::function<void(std::span<char>)> clipboardRead; //004dd810: 최대256BYTE의NUL문자열을공급한다.
    std::function<void(std::span<const char>)> clipboardWrite; //004dd870: 선택또는전체본문의명시길이복사다.
};
// 실제TextEntry가+108토큰으로소유하는상태/메모리는이처리기보다오래살아야한다.
class TextEntryBuffer {
public:
    // bytes조회는콜백효과를만들지않는다.alloc/free/resize/clipboard는현재state를바꿀수있다.
    TextEntryBuffer(TextEntryBufferState& state,TextEntryBufferHooks hooks);
    //00451a80: 요청0이면2048BYTE,줄offset512BYTE를할당한다.본문의첫NUL은아직쓰지않는다.
    void Construct(std::uint32_t requested=0) const;
    //00451680: data가비0일때본문반납→data/capacity0→현재offset반납→offset0이다.
    void Destroy() const;
    //004516c0: 두signed위치를정렬하고현재length로clamp한다.
    void Select(std::int32_t begin,std::int32_t end) const;
    //00451710: NUL앞바이트로선택을교체한다.빈span은원문의null→빈문자열이다.
    // 입력범위는할당콜백동안유효해야하며본문재할당으로무효가되는자기참조는호출자가피한다.
    void Insert(std::span<const char> text={}) const;
    //00451930: 첫NUL·길이·선택·dirty를초기화한뒤같은Insert를실행한다.
    void Reset(std::span<const char> text={}) const;
    //00451550: 실제LF줄offset과마지막length를쓴다.할당실패는부분상태를유지하고false다.
    bool EnsureLines() const;
    //00451950/451960: EnsureLines반환과별개로현재count/유효행길이를읽는다.
    std::int32_t LineCount() const;
    std::int32_t LineLength(std::int32_t line) const;
    //00451990: 줄본문과NUL·선택구간쌍을쓴다.범위/용량거부는ffffffff이며출력을보존한다.
    std::uint32_t CopyLine(std::int32_t line,std::span<char> output,std::array<std::int32_t,2>& selection) const;
    //004518f0: 현재전체NUL문자열복사이며용량거부는ffffffff다.
    std::uint32_t CopyAll(std::span<char> output) const;
    //004518c0: 선택이비어있으면전체본문을복사한다.0길이는효과가없다.
    void Copy() const;
    //00451b10: 빈선택은전체선택으로바꾼뒤복사→null삽입→dirty1이다.
    void Cut() const;
    //00451800: 실제clipboard256BYTE에서제외문자들을지운뒤비어있지않으면삽입한다.
    void Paste(std::span<const char> excluded={}) const;
    // 같은실제상태의현재필드를TextEntry/Console이직접재조회한다.
    TextEntryBufferState& State() const;
    // 현재주소의실제남은저장범위다.원문문자열/선택/Draw가같은바이트를사용한다.
    std::span<std::uint8_t> Data() const;
    // 원본length앞바이트의읽기전용뷰다.다음재할당/해제이후에는다시조회해야한다.
    std::string_view Text() const;
private:
    // 원본주소의범위안에서만현재바이트를빌린다.본문길이와물리할당폭은구별한다.
    std::span<std::uint8_t> Memory(std::uint32_t address,std::size_t count) const;
    // 같은현재주소에서정렬되지않은DWORD를읽고쓴다.
    std::uint32_t Read(std::uint32_t address) const;
    void Write(std::uint32_t address,std::uint32_t value) const;
    // 현재주소부터NUL앞의LF첫위치이며없으면0이다.
    std::uint32_t FindNewline(std::uint32_t address) const;
    TextEntryBufferState& state_; // 실제0x28필드를보유하는같은상태다.
    TextEntryBufferHooks hooks_; // 실제소유메모리와현재clipboard효과다.
};
}
