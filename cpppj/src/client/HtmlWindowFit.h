// 원문 HTML4c7d40의 전체 폭·높이/메뉴·버튼/중앙 부착을 같은 실제 노드에서 계산한다.
#pragma once
#include "client/HtmlWindowGump.h"
namespace netstorm::client { // 실제 HTML 전체 가상 부착의 계약이다.
struct HtmlWindowFitHooks { // 측정/트리/현재 전역의 실제 하위 몸체만 연결한다.
    std::function<HtmlWindowGumpNode*(std::uint32_t)> node;std::function<GumpCreateNode*(std::uint32_t)> base; // 같은 현재 부모/자식이다.
    std::function<void(std::uint32_t)> fitButton,push,fitHeight;std::function<void(std::uint32_t,std::int32_t,std::int32_t)> fitContent; // 실제 돌 버튼/가상Push/ScrollingText 측정이다.
    std::function<void(std::uint32_t,std::uint32_t)> slider,scrollable;std::function<void(std::uint32_t,std::int32_t)> requestScroll; // 화면 제한 뒤 실제 본문의 상태 변경이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach,attachBase; // 자식 가상 부착과 부모의 실제 기본 부착을 구별한다.
    std::function<std::uint32_t()> currentMenu;std::function<void(std::int32_t,std::int32_t,std::uint32_t)> attachMenu; //475c40/475ca0의 현재 열 조회/부착·pop이다.
    std::function<std::uint32_t(std::uint32_t)> reference,read;std::function<void(std::uint32_t,std::uint32_t)> addDepth; // 현재serial/전역 DWORD와 실제 깊이 덧셈이다.
};
class HtmlWindowFit { //4c7d40/ret16의 모든 정상 크기/메뉴/배치 분기를 수행한다.
public:
    explicit HtmlWindowFit(HtmlWindowFitHooks hooks); // 같은 부모/자식 주소만 미리 요구한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // x/y는 원문처럼 현재 중앙 좌표로 바꾸며 parent/mode는 보존한다.
private:
    HtmlWindowGumpNode& Node(std::uint32_t token) const;GumpCreateNode& Base(std::uint32_t token) const;std::int32_t Read(std::uint32_t address) const; // 현재 실제 주소/전역을 조회한다.
    HtmlWindowFitHooks hooks_; // 같은 실제 UI/장치 수명을 빌린다.
};
}
