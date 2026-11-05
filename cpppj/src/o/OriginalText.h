// 새로 쓰는 코드: 원본 Windows-1252 바이트와 신규 UTF-8 텍스트의 경계 처리.
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::o {
// 6차 한국어: 8비트 글꼴 파이프라인은 Windows-1252 바이트 하나가 글리프 하나다. 1252로 표현할 수 없는 한글 문자는 4바이트 제어문자 시퀀스
// [시작 바이트 0x1f] + [25진수 숫자 3개](각 숫자는 0x01..0x08, 0x0b, 0x0c, 0x0e..0x19, 0x1c..0x1e)로 바꿔 기존 바이트 문자열 경로를 그대로 통과시킨다.
// 숫자 바이트는 어떤 정상 글리프(0x20 이상)와도 겹치지 않고 탭/개행/CR/EOF/ESC도 쓰지 않는다. 원본에는 없는 새 확장이다.
inline constexpr std::uint8_t kExtendedTextLead = 0x1f;  // 확장 문자 시퀀스의 시작 바이트다.
inline constexpr std::size_t kExtendedTextLength = 4;    // 시작 바이트 1개 + 숫자 3개다.
// 바이트가 확장 시퀀스의 숫자 바이트인지 판정한다(글자 폭 표에서 폭 0으로 다루는 데 쓴다).
bool IsExtendedTextDigit(std::uint8_t byte);
// 확장 시퀀스의 선두에서 코드 포인트를 해독한다. bytes[position]부터 정확한 시퀀스가 아니면 빈 값이다(그 바이트는 일반 문자다).
std::optional<std::uint32_t> DecodeExtendedSequence(std::span<const std::uint8_t> bytes, std::size_t position);
// 코드 포인트가 확장 부호화 대상(한글 음절·호환 자모·CJK 기호·전각 ASCII)이면 4바이트 시퀀스를 만든다. 대상이 아니면 빈 값이다.
std::optional<std::array<std::uint8_t, kExtendedTextLength>> EncodeExtendedCodePoint(std::uint32_t codePoint);
// ASCII만 소문자로 바꾼다. 원본의 키·경로 비교에 사용하며 UTF-8 바이트는 보존한다.
std::string AsciiLower(std::string_view text);
// 원본 텍스트를 UTF-8로 변환한다. UTF-8 BOM이 있으면 이미 변환된 텍스트로 취급한다. 파일/설정 경계용이라 확장 시퀀스는 해독하지 않는다.
std::string DecodeOriginalText(std::span<const std::uint8_t> bytes);
// 내부 UTF-8을 원본 Windows-1252 바이트로 되돌린다. 표현할 수 없는 문자·잘못된 UTF-8은 거부한다. 설정 저장/유니코드 경로 정책이 이 거부에 의존하므로 한글을 허용하지 않는다.
std::vector<std::uint8_t> EncodeOriginalText(std::string_view text);
// 6차 한국어: 표시(글꼴 바이트) 경계용 DecodeOriginalText다. 정확한 확장 시퀀스를 한 문자로 되돌린다.
std::string DecodeDisplayText(std::span<const std::uint8_t> bytes);
// 6차 한국어: 표시(글꼴 바이트) 경계용 EncodeOriginalText다. 한글 등 확장 대상 문자는 4바이트 시퀀스로 만들고 그 밖의 표현할 수 없는 문자·잘못된 UTF-8은 거부한다.
std::vector<std::uint8_t> EncodeDisplayText(std::string_view text);
}
