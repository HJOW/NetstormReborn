// 원본 HTML 링크의 연속 NUL 문자열과 StyleText T태그를 같은 호스트 수명에 보관한다.
#pragma once
#include "client/HtmlTextParser.h"
#include <vector>
namespace netstorm::client {
// 0046bfc0의 +10 현재 쓰기 포인터를 호스트 토큰+바이트 위치로 표현한다.
struct HtmlLinkTextState {
    std::uint32_t base{0x61010000}; // 원본 포인터 대신 사용하는 문자열 저장소의 시작 토큰이다.
    std::size_t capacity{65536}; // 원본 출력 설정이 공급하는 물리 링크 저장소 한도다.
    bool enabled{true}; // 원본 +10이0일 때는 링크 문자열과 T태그를 모두 만들지 않는다.
    std::vector<char> bytes; // 각 주제 뒤 NUL을 포함하며 다음 링크를 바로 뒤에 붙인다.
    std::size_t cursor{}; // 파서별 현재 쓰기 위치다. 중첩 파서는 바이트를 공유하되 부모 cursor를 전진시키지 않는다.
};
// 열림은 첫 인용 문자열의 #을 한 번 생략한다. 닫힘/인용 없음은 T0이며 저장소를 소비하지 않는다.
std::string HtmlLinkText(HtmlLinkTextState& links,std::string_view raw,bool closing);
// 현재 저장소의 유효한 호스트 문자열 토큰만 읽는다. 원본 PE 주소를 역참조하지 않는다.
std::string_view ResolveHtmlLink(const HtmlLinkTextState& links,std::uint32_t token);
// 같은 문자열 소유자와 실제 기본/파생 HTML 파서를 연결한다. 파서는 links보다 먼저 해제한다.
void BindHtmlLinkText(HtmlTextParserHooks& hooks,HtmlLinkTextState& links);
}
