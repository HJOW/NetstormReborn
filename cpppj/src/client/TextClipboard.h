// 원문004dd810/004dd870의 CF_TEXT와 GlobalAlloc/Lock 순서를 실제 OS 또는 명시 장치에 연결한다.
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

namespace netstorm::client { // 공통 TextEntry가 빌리는 클립보드의 원문 부모다.
struct RawTextEntryContext; // 기존 공통 문자 UI 문맥을 정의의 복제 없이 빌린다.
struct TextClipboardDevices { // OS 반환은 원문이 검사하는 위치에서만 해석한다.
    std::function<std::uint32_t(std::uint32_t)> formatAvailable; // IsClipboardFormatAvailable(1)의 비영 여부다.
    std::function<std::uint32_t(std::uintptr_t)> open; // 원문 HWND는 고정 NULL이며 현재 게임 창 핸들을 대신 전달하지 않는다.
    std::function<std::uintptr_t(std::uint32_t)> get; // GetClipboardData(CF_TEXT)의 실제 HANDLE이다.
    std::function<char*(std::uintptr_t)> lock; // GlobalLock이 돌려준 동일 메모리이며 NULL 실패도 유지한다.
    std::function<std::uint32_t(std::uintptr_t)> unlock; // GlobalUnlock 반환은 원문처럼 무시한다.
    std::function<std::uint32_t()> empty; // EmptyClipboard 반환은 원문처럼 무시한다.
    std::function<std::uintptr_t(std::uint32_t,std::uint32_t)> allocate; // GlobalAlloc(flags=0x2000,len+1)의 HANDLE이다.
    std::function<std::uintptr_t(std::uint32_t,std::uintptr_t)> set; // SetClipboardData(1,handle)의 반환은 원문처럼 무시한다.
    std::function<std::uint32_t()> close; // Open에 성공한 원문 정상 경로의 CloseClipboard다.
};
class TextClipboard { // CF_UNICODETEXT 변환이나 새 줄/문자열 정규화를 추가하지 않는 부모다.
public:
    explicit TextClipboard(TextClipboardDevices devices); // 실제 선택한 OS 하위만 호출 시 요구한다.
    void Read(std::span<char> output) const; // output[0]을 먼저0으로 쓰고 strncpy의 NUL padding/가득찬 비NUL을 유지한다.
    void Write(std::span<const char> input) const; // 명시 길이를 복사하고 별도 마지막 NUL을 한 바이트 쓴다.
private:
    TextClipboardDevices devices_; // 창이나 Config보다 독립적인 실제 OS/검사 장치의 수명이다.
};
TextClipboardDevices Win32TextClipboardDevices(); // 실제 Win32 API를 연결한다. 호출 자체는 클립보드를 열거나 수정하지 않는다.
void BindTextClipboard(RawTextEntryContext& context,std::shared_ptr<const TextClipboard> clipboard); // 실제 같은 장치 수명을 두 callback이 공유하며 다른 now/Console callback은 보존한다.
}
