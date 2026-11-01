// HTML $Input의 확인은 입력을 다음 Tell 섹션의 인자로 전달한다. 설정 변경과 구별한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string>

namespace netstorm::client { //문자 편집/공통 모달은 기존 실제 처리기가 담당한다.
struct HtmlInputConfirmHooks { //004cfae0의 선택된 실제 하위 경계를 호출 시점에 조회한다.
    std::function<void(std::span<char>)> copyText; //현재 편집 자식의 CopyText(output,256)다.
    std::function<int(std::int32_t)> space; //원문은 마지막 NUL부터 isspace를 검사하므로 보통0을 한 번 읽는다.
    std::function<std::int32_t(std::string_view)> parseLong; //모드1 외에는 atol을 호출하지만 반환값을 쓰지 않는다.
    std::function<std::string(std::string_view,std::string_view)> format; //원문의 고정256BYTE "%s(%s)" 서식이다.
    std::function<void(std::string_view)> tell; //같은 현재 Tell(section,0)이며 즉시 설정에 쓰지 않는다.
};
std::uint32_t ConfirmHtmlInput(const std::uint32_t& mode,std::span<const char> continuation,const HtmlInputConfirmHooks& hooks); //첫LF에서자르고현재mode/후속섹션을읽은뒤Tell하고1을반환한다.
}
