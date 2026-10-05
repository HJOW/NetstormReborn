// 새로 쓰는 코드: 원본 Windows-1252 바이트와 신규 UTF-8 텍스트의 경계 처리.
#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::o {
// ASCII만 소문자로 바꾼다. 원본의 키·경로 비교에 사용하며 UTF-8 바이트는 보존한다.
std::string AsciiLower(std::string_view text);
// 원본 텍스트를 UTF-8로 변환한다. UTF-8 BOM이 있으면 이미 변환된 텍스트로 취급한다.
std::string DecodeOriginalText(std::span<const std::uint8_t> bytes);
// 내부 UTF-8을 원본 Windows-1252 바이트로 되돌린다. 표현할 수 없는 문자·잘못된 UTF-8은 거부한다.
std::vector<std::uint8_t> EncodeOriginalText(std::string_view text);
}
