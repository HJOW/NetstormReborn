// UTF-8/UTF-16 확정 입력과 기존 표시 BYTE 문자열의 문자 경계를 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client { // 원문 큐의 크기/배치를 유지하는 한국어 입력 경계다.
// 비명령 사건의 미사용 상위 비트로 확정 Unicode 한 문자를 식별한다. 실제 VK는 0xff 이하라 겹치지 않는다.
inline constexpr std::uint32_t kCommittedTextInput = 0x01000000U;
// 문자 태그/명령 비트와 Unicode 범위를 구별한다.
bool IsCommittedTextInput(std::uint32_t event);
// 확정 입력 한 사건을 기존 표시 바이트로 바꾼다. 잘못된 태그/미지원 문자는 거부한다.
std::string CommittedTextBytes(std::uint32_t event);
// 전체 UTF-8을 먼저 검증한다. ASCII/1252는 기존 한 BYTE 사건, 한글은 태그 한 사건이다.
// 제어문자/NUL은 받지 않는다. Enter/Tab/Backspace는 기존 키 동작으로 전달한다.
std::vector<InputEvent> MakeCommittedTextEvents(std::string_view utf8,std::int32_t x=0,std::int32_t y=0);
// IMM의 UTF-16 확정 본문을 UTF-8로 바꾼다. 비정상 surrogate는 전체 거부한다.
std::string CommittedUtf16ToUtf8(std::u16string_view text);
// 표시 바이트를 UTF-16으로 되돌린다. 클립보드 경계에서만 사용한다.
std::u16string DisplayTextToUtf16(std::span<const char> text);
// 정확히 인식되는 네 BYTE 확장 토큰이 하나라도 있는지 읽는다.
bool HasExtendedDisplayText(std::string_view text);
// BYTE 오프셋을 이전/다음 문자 경계로 보정한다. 잘못된 선두는 원래 한 BYTE로 취급한다.
std::size_t DisplayTextFloor(std::string_view text,std::size_t position);
std::size_t DisplayTextCeil(std::string_view text,std::size_t position);
// 현재 위치의 이전/다음 문자 경계다. 한글 네 BYTE를 한 번에 건너뛴다.
std::size_t PreviousDisplayText(std::string_view text,std::size_t position);
std::size_t NextDisplayText(std::string_view text,std::size_t position);
// LF 기준 실제 줄의 읽기 전용 뷰다. 원문 BYTE 줄번호를 바꾸지 않는다.
std::string_view DisplayTextLine(std::string_view text,std::int32_t line);
// BYTE 상한까지 완전한 토큰만 복사한다. 제외문자는 확장 토큰 내부에 적용하지 않는다.
std::string FilterDisplayText(std::string_view text,std::string_view excluded,std::size_t maximum);
}
