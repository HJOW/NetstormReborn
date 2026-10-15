// 원본 HtmlGump의 본문 변환을 실제 StyleText 입력 문자열로 제공한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 기본 파서의 현재 필드다. 출력 설정은 afterHeading만 지우고 조건 가림 상태는 유지한다.
struct HtmlTextParserState {
    std::uint32_t shown{1}; // +1c: 현재 평문/태그 효과를 표시하는 DWORD다.
    std::uint32_t hiddenDepth{}; // +20: 현재 닫힘 조건 태그를 허용하는 숨김 깊이다.
    std::uint32_t inTag{}; // +24: '<' 이후 '>' 전의 태그 본문인지 나타낸다.
    std::uint32_t textAllowed{}; // +28: shown에서 시작하고 소문자 a 열림/닫힘이 증감하는 signed 판정 카운터다.
    std::uint32_t afterHeading{}; // +2c: H닫힘 뒤 공백만 출력한 상태이며 다음 P의 빈 줄을 줄인다.
    std::size_t inputOffset{}; // +4: 현재 NUL 문자열에서 처리 중인 바이트 위치다. virtual callback도 변경할 수 있다.
    std::string output; // +18..+8: 실제 StyleText 제어문자를 포함한 출력이다. size가 출력 포인터의 이동량이다.
};
// 같은 미션/Tell의 HTML 변환이 읽는 원본 전역이다.
struct HtmlTextParserGlobals {
    std::uint32_t suppressParagraph{}; // PATCH00565ec0/CD00536368: 다음 P에서 출력 대신0으로 돌아간다.
};
// 조건/명령/그림/링크/배경 및 TextHtml 파생 동작은 실제 공급자가 필요하다.
struct HtmlTextParserHooks {
    std::function<void(HtmlTextParserState&,std::string_view)> command; // '$' 다음부터 NUL까지의 원시 명령을 현재 큐 처리기로 보낸다.
    std::function<void(HtmlTextParserState&,std::string_view,bool)> condition; // '?'부터 NUL까지의 원문/닫힘과 현재 shown/depth를 실제 조건 처리기로 보낸다.
    std::function<std::string(HtmlTextParserState&,std::string_view)> picture; // '!'부터 NUL까지를 받아 실제 그림 helper가 쓸 제어문자열을 돌려준다.
    std::function<std::string(HtmlTextParserState&,std::string_view,bool)> link; // 열림 원문 또는 닫힘의 빈 원문에서 실제 T태그/링크 저장소 효과를 공급한다.
    std::function<void(HtmlTextParserState&,std::string_view)> background; // 원본이 '=' 뒤에서 추출한 배경 이름을 실제 전역에 저장한다.
    std::function<void(HtmlTextParserState&,std::string_view)> insideTag; // virtual0: 태그 안 일반 바이트 위치다. 기본 VT는 실제 noop다.
    std::function<void(HtmlTextParserState&,std::string_view)> beforeClose; // virtual1: 닫힘에서 '/'로 이동한 뒤 호출한다. 기본 VT는 실제 noop다.
    std::function<void(HtmlTextParserState&,std::string_view)> afterTag; // virtual2: '>'를 읽고 inTag=0을 쓴 뒤 호출한다. 기본 VT는 현재 인자 반환만 한다.
};
// PATCH0046bea0/bed0/bf00/c090/c810의 기본 변환이다. Config나 게임 명령을 직접 재구현하지 않는다.
class HtmlTextParser {
public:
    // 같은 현재 상태/전역을 참조한다. 파생/외부 효과는 해당 분기가 선택될 때만 요구한다.
    HtmlTextParser(HtmlTextParserState& state,HtmlTextParserGlobals& globals,HtmlTextParserHooks hooks={},
        o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // NUL 앞 원본 바이트를 변환한다. logical capacity-256의 strict 경계와 마지막 LF/NUL을 유지한다.
    // 태그가 실제 물리 capacity 밖을 쓰려는 원본 오버런은 예외로 거부한다. 반환 문자열의 NUL은 size에 포함하지 않는다.
    std::string Parse(std::string_view source,std::int32_t capacity=8192) const;
private:
    // 다음 제어문자/평문을 물리 출력 버퍼 범위에서만 추가한다.
    void Append(std::string_view text,std::int32_t capacity) const;
    // 원본의 첫 바이트만 signed char로 확장해 소문자 ASCII 분기를 선택한다. 태그 전체를 소문자로 바꾸지 않는다.
    void Tag(std::string_view source,std::size_t offset,bool closing,std::int32_t capacity) const;
    // callback이 바꾼 현재 입력/출력 위치를 다음 native 조회 전에 확인한다.
    void Validate(std::string_view source,std::int32_t capacity) const;
    HtmlTextParserState& state_; // 현재 파서 필드와 출력 문자열을 같은 저장소에 유지한다.
    HtmlTextParserGlobals& globals_; // 같은 현재 P 억제 DWORD를 조회하고 쓴다.
    HtmlTextParserHooks hooks_; // 원본 외부/파생 효과를 명시적으로 공급한다.
    o::OriginalEdition edition_; // H 글꼴 등 두 원본의 실제 변환 차이를 선택한다.
};
}
