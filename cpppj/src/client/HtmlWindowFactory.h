// 원문4cf270의 전체 HTML 창 생성·두 번 변환·메타·실제 부착 순서를 유지한다.
#pragma once
#include "client/HtmlWindowGump.h"
#include "client/HtmlTextParser.h"
namespace netstorm::client { // 실제 전체 HTML 생성과 선택 장치의 계약이다.
struct HtmlWindowFactoryHooks { // 부모와 파서/Config/메뉴·공통 입력의 현재 장치만 공급한다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> create;std::function<HtmlWindowGumpNode*(std::uint32_t)> node; // 실제 전체 할당/생성과 같은 현재 부모다. 생성0은 원문 실패다.
    std::function<std::string(std::string_view,std::uint32_t)> substitute;std::function<std::string()> fallback; // 현재441660과OkButton 조회다. 좁은 바이트를 정규화하지 않는다.
    std::function<std::string(HtmlTextParserState&,std::uint32_t,std::string_view,std::uint32_t)> parse; // 같은 파서에 출력/링크cursor·입력을 설정하고 변환한다. 두 호출은 같은 상태/부모 링크 바이트를 쓴다.
    std::function<std::uint32_t(std::uint32_t,std::span<char>)> metadata;std::function<double()> wallSeconds; //4c83b0과 현재 벽 시각이다.
    std::function<void(std::uint32_t,std::string_view)> body;std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 실제 본문 생성과 전체 Fit 가상 부착이다.
    std::function<void(std::uint32_t,bool)> modal;std::function<void(std::uint32_t)> kill;std::function<std::uint32_t(std::uint32_t)> read; // 같은 공통 모달/수명·현재 화면/루트 전역이다.
};
class HtmlWindowFactory { // 부분 미션 안내를 대체할 원문 전체 공장 본체다.
public:
    HtmlWindowFactory(HtmlWindowGumpState& state,HtmlWindowFactoryHooks hooks); // 같은 Client 장기 전역을 빌리며 선택 장치는 해당 시점에 요구한다.
    std::uint32_t Show(std::string_view source,std::uint32_t suppress,std::uint32_t width,std::uint32_t height) const; // suppress비영은 창 없이 첫 파서 효과만 실행한다. 실제 최종serial 또는0을 반환한다.
private:
    HtmlWindowGumpNode& Node(std::uint32_t token) const; // 현재 같은 부모 주소만 읽는다.
    HtmlWindowGumpState& state_;HtmlWindowFactoryHooks hooks_; // 파서 실행·부착 동안 살아 있는 실제 전역/장치다.
};
}
