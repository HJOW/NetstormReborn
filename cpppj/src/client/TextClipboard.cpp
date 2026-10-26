// 실제 OS의 실패 반환을 성공으로 바꾸지 않고 원문의 검사/무시/Close 위치를 유지한다.
#include "client/TextClipboard.h"
#include "client/RawTextEntryContext.h"
#include <cstring>
#include <limits>
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
    if (input.size()>=std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("TextClipboard 쓰기의 호스트 할당 물리 범위");
    if (!Need(devices_.open)(0)) return;static_cast<void>(Need(devices_.empty)());
    const auto handle=Need(devices_.allocate)(kAllocateFlags,static_cast<std::uint32_t>(input.size()+1));
    if (!handle) { static_cast<void>(Need(devices_.close)());return; }
    auto* target=Need(devices_.lock)(handle);if (!target) throw std::out_of_range("원문 TextClipboard 쓰기의 NULL GlobalLock 물리 접근"); // 원문은 이곳에서 접근 실패한다. 성공/Close/Free로 위장하지 않는다.
    if (!input.empty()) std::memcpy(target,input.data(),input.size());target[input.size()]=0;
    static_cast<void>(Need(devices_.unlock)(handle));static_cast<void>(Need(devices_.set)(kTextFormat,handle));static_cast<void>(Need(devices_.close)());
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
