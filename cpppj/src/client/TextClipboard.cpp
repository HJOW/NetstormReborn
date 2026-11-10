// 실제 OS의 실패 반환을 성공으로 바꾸지 않고 원문의 검사/무시/Close 위치를 유지한다.
#include "client/TextClipboard.h"
#include "client/DisplayTextInput.h" // 확장 BYTE를 외부 클립보드에 제어문자로 내보내지 않는다.
#include "o/OriginalText.h"
#include "client/RawTextEntryContext.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //기존 빌드 정의를 덮지 않고 필요한 Win32 장치 선언만 사용한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX //Win32 min/max 매크로가 현재 표준 함수 호출을 바꾸지 않게 한다.
#endif
#include <windows.h>
#endif

namespace netstorm::client { // TextEntry의 선택/전체 BYTE 복사 부모다.
namespace { // 명시 선택된 경계와 원문 숫자 상수만 모은다.
constexpr std::uint32_t kTextFormat=1; // CF_TEXT이며 UTF-8/Unicode 변환을 요청하지 않는다.
constexpr std::uint32_t kAllocateFlags=0x2000; // 원문 GlobalAlloc의 GMEM_DDESHARE 비트다. MOVEABLE을 새로 추가하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("TextClipboard의 선택된 실제 OS 장치 미연결");return function; } // 누락 callback은 선택한 위치에서만 거부한다.
}
TextClipboard::TextClipboard(TextClipboardDevices devices):devices_(std::move(devices)) {} // 소유만 넘기며 아직 OS 효과를 만들지 않는다.
void TextClipboard::Read(std::span<char> output) const { // 원문 최대256BYTE 호출 외에도 같은 명시 capacity를 받는다.
    if (output.empty() || output.size()>std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("TextClipboard 읽기의 실제 출력 물리 범위");
    if (devices_.readUnicode) if (const auto text=devices_.readUnicode()) {
        try {
            for (const auto scalar:*text) if ((scalar<0x20 && scalar!=9 && scalar!=10 && scalar!=13) || scalar==0x7f) throw std::invalid_argument("Unicode 클립보드 제어문자 거부"); // 외부 U+001F를 내부 토큰 선두로 해석하지 않는다.
            const auto bytes=o::EncodeDisplayText(CommittedUtf16ToUtf8(*text));const std::string_view input(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            const auto filtered=FilterDisplayText(input,{},output.size()-1);std::fill(output.begin(),output.end(),0);std::copy(filtered.begin(),filtered.end(),output.begin()); }
        catch (const std::invalid_argument&) { std::fill(output.begin(),output.end(),0); }return; // 지원하지 않는 외부 Unicode는 일부 붙이지 않고 빈 입력으로 거부한다.
    }
    output[0]=0;if (!Need(devices_.formatAvailable)(kTextFormat)) return;if (!Need(devices_.open)(0)) return;
    const auto handle=Need(devices_.get)(kTextFormat);
    if (handle) {
        const auto* source=Need(devices_.lock)(handle);
        if (source) {
            std::size_t index=0; // strncpy와 같은 첫NUL 뒤0 padding이며 capacity 마지막 바이트를 별도로 덮지 않는다.
            for (;index<output.size() && source[index];++index) output[index]=source[index]; // 원문의 capacity 이하 BYTE만 복사한다.
            for (;index<output.size();++index) output[index]=0; // 짧은 C문자열의 남은 capacity는 원문 strncpy처럼0이다.
            static_cast<void>(Need(devices_.unlock)(handle));
        }
    }
    static_cast<void>(Need(devices_.close)());
}
void TextClipboard::Write(std::span<const char> input) const { // 선택한 본문 길이에 첫NUL 절단을 추가하지 않는다.
    if (devices_.writeUnicode && HasExtendedDisplayText({input.data(),input.size()})) { devices_.writeUnicode(DisplayTextToUtf16(input));return; } // ASCII/원문 BYTE 복사는 기존 장치 순서를 유지한다.
    if (input.size()>=std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("TextClipboard 쓰기의 호스트 할당 물리 범위");
    if (!Need(devices_.open)(0)) return;static_cast<void>(Need(devices_.empty)());
    const auto handle=Need(devices_.allocate)(kAllocateFlags,static_cast<std::uint32_t>(input.size()+1));
    if (!handle) { static_cast<void>(Need(devices_.close)());return; }
    auto* target=Need(devices_.lock)(handle);if (!target) throw std::out_of_range("원문 TextClipboard 쓰기의 NULL GlobalLock 물리 접근"); // 원문은 이곳에서 접근 실패한다. 성공/Close/Free로 위장하지 않는다.
    if (!input.empty()) std::memcpy(target,input.data(),input.size());target[input.size()]=0;
    static_cast<void>(Need(devices_.unlock)(handle));static_cast<void>(Need(devices_.set)(kTextFormat,handle));static_cast<void>(Need(devices_.close)());
}
// Unicode 확장만 GMEM_MOVEABLE/실제 창을 사용한다. 원문 장치 플래그/NULL HWND는 별도 경로에 보존한다.
TextClipboardDevices Win32DisplayTextClipboardDevices(std::uintptr_t window) {
    auto devices=Win32TextClipboardDevices();
#ifdef _WIN32
    devices.readUnicode=[window]()->std::optional<std::u16string> {
        if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return std::nullopt;
        if (!OpenClipboard(reinterpret_cast<HWND>(window))) throw std::runtime_error("Unicode 클립보드 열기 실패");
        struct Close { ~Close() { CloseClipboard(); } };const Close close; // 성공한 Open은 모든 반환에서 반납한다.
        const HANDLE data=GetClipboardData(CF_UNICODETEXT);if (!data) throw std::runtime_error("Unicode 클립보드 본문 없음");
        const auto capacity=GlobalSize(data)/sizeof(char16_t);const auto* text=static_cast<const char16_t*>(GlobalLock(data));if (!text) throw std::runtime_error("Unicode 클립보드 잠금 실패");
        struct Unlock { HANDLE data;~Unlock() { GlobalUnlock(data); } };const Unlock unlock{data}; // 시스템 소유 본문을 수정하거나 반납하지 않는다.
        const auto end=std::find(text,text+capacity,char16_t{});if (end==text+capacity) throw std::length_error("Unicode 클립보드 NUL 범위");return std::u16string(text,end);
    };
    devices.writeUnicode=[window](std::u16string_view text) {
        if (!window || !OpenClipboard(reinterpret_cast<HWND>(window))) throw std::runtime_error("Unicode 클립보드 실제 창/열기 실패");
        struct Close { ~Close() { CloseClipboard(); } };const Close close; // 결과와 무관하게 현재 창의 클립보드 잠금을 반납한다.
        if (text.size()>(std::numeric_limits<SIZE_T>::max()/sizeof(char16_t))-1) throw std::length_error("Unicode 클립보드 할당 범위");
        const auto bytes=(text.size()+1)*sizeof(char16_t);const HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);if (!memory) throw std::bad_alloc();
        auto* target=static_cast<char16_t*>(GlobalLock(memory));if (!target) { GlobalFree(memory);throw std::runtime_error("Unicode 클립보드 쓰기 잠금 실패"); }
        std::copy(text.begin(),text.end(),target);target[text.size()]=0;GlobalUnlock(memory);
        if (!EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT,memory)) { GlobalFree(memory);throw std::runtime_error("Unicode 클립보드 게시 실패"); } // 성공한 Set만 OS로 소유권을 넘긴다.
    };
#else
    static_cast<void>(window);
#endif
    return devices;
}
TextClipboardDevices Win32TextClipboardDevices() { // HWND NULL과 CF_TEXT를 원문 그대로 User32/Kernel32에 전달한다.
#ifdef _WIN32
    TextClipboardDevices devices; // 실제 API 반환을 값 변환만 하며 오류나 내용은 고치지 않는다.
    devices.formatAvailable=[](auto format) { return static_cast<std::uint32_t>(::IsClipboardFormatAvailable(format)); }; // 사용 가능 형식 검사다.
    devices.open=[](auto window) { return static_cast<std::uint32_t>(::OpenClipboard(reinterpret_cast<HWND>(window))); }; // 실제 부모가 고른 NULL HWND다.
    devices.get=[](auto format) { return reinterpret_cast<std::uintptr_t>(::GetClipboardData(format)); }; // 실제 클립보드 소유 HANDLE이다.
    devices.lock=[](auto handle) { return static_cast<char*>(::GlobalLock(reinterpret_cast<HGLOBAL>(handle))); }; // NUL과 용량의 의미를 바꾸지 않는다.
    devices.unlock=[](auto handle) { return static_cast<std::uint32_t>(::GlobalUnlock(reinterpret_cast<HGLOBAL>(handle))); }; // 성공0도 정규화하지 않는다.
    devices.empty=[] { return static_cast<std::uint32_t>(::EmptyClipboard()); }; // 원문은 결과를 검사하지 않는다.
    devices.allocate=[](auto flags,auto length) { return reinterpret_cast<std::uintptr_t>(::GlobalAlloc(flags,length)); }; // 원문0x2000을 그대로 사용한다.
    devices.set=[](auto format,auto handle) { return reinterpret_cast<std::uintptr_t>(::SetClipboardData(format,reinterpret_cast<HANDLE>(handle))); }; // 원문은 결과와 무관하게Close로 진행한다.
    devices.close=[] { return static_cast<std::uint32_t>(::CloseClipboard()); };return devices; // OS의 실제 정리 반환이다.
#else
    throw std::logic_error("cpppj Win32 TextClipboard의 다른 OS 장치 미구현"); // cpppj Windows 범위 밖을 빈 성공으로 만들지 않는다.
#endif
}
void BindTextClipboard(RawTextEntryContext& context,std::shared_ptr<const TextClipboard> clipboard) { // UI가 장치를 빌리는 동안 같은 실제 OS 부모를 유지한다.
    if (!clipboard) throw std::invalid_argument("TextEntry clipboard의 실제 수명 소유자 누락");
    context.clipboardRead=[clipboard](auto bytes) { clipboard->Read(bytes); };context.clipboardWrite=[clipboard=std::move(clipboard)](auto bytes) { clipboard->Write(bytes); }; // 두 callback이 같은 부모/핸들 계약을 사용한다.
}
}
