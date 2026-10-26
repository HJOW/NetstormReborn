// 원본 HTML 부모의 두8192바이트 영역과 실제 다중 줄 자식의 생성/갱신/부착을 복원한다.
#pragma once
#include "client/GumpCreate.h"
#include <string_view>
namespace netstorm::client {
inline constexpr std::size_t kHtmlBodyCapacity=8192; // 원본 +a4 본문/+20a4 링크 영역의 같은 물리 크기다.
// 표시 자식은 공통 GumpLifetime이 소유하고 두 배열은 이 부모와 같은 수명이다.
struct HtmlBodyGumpNode: GumpCreateNode {
    std::uint32_t text{}; // +a0의 현재 다중 줄 표시 자식이다.
    std::array<char,kHtmlBodyCapacity> body{},links{}; // 첫NUL 뒤의 기존 바이트도 생성/파싱 중 임의로 지우지 않는다.
};
// 생성 시 현재 화면/식별 BYTE/다중 줄 기본 색을 읽는다.
struct HtmlBodyGumpState {
    std::int32_t screenWidth{},screenHeight{}; // 원본531860/64의 signed 화면 크기다.
    std::int8_t kind{20}; // 원본54042f의 종류 BYTE다.
    std::uint32_t color{}; // 현재5b5dc8의 실제 다중 줄 기본 색이다.
};
// 기본/파생 노드는 같은 안정된 저장소이며 실제 다중 줄 처리기와 공통 트리를 공급한다.
struct HtmlBodyGumpHooks {
    std::function<HtmlBodyGumpNode*(std::uint32_t)> node; // 실제 부모 필드/배열이다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // 같은 현재 자식의 실제 기본 필드다.
    std::function<void(std::uint32_t,std::uint32_t,float,float,std::uint32_t)> constructBase; // 실제 기본 Gump 생성자다.
    std::function<std::uint32_t(std::size_t)> allocate; // 원본 new0xd8의 호스트 식별값이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::uint32_t,std::uint32_t)> constructText; // 실제 다중 줄(0,0,5,color) 생성이다.
    std::function<void(std::uint32_t)> kill,push; // 현재 자식 우선 Kill과 가상 Push다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach,baseAttach; // 파생 가상/기본 부착을 구별한다.
    std::function<void(std::uint32_t,std::string_view)> styled; // 실제 다중 줄 StyleText 입력이다.
    std::function<void(std::uint32_t,std::int32_t)> scroll; // 실제 절대 스크롤 요청이다.
    std::function<void(std::uint32_t,std::uint32_t)> slider,scrollable; // 실제 Slider/스크롤 허용 설정이다.
};
// 원본46ca70의 현재 파서 출력 포인터와 실제 순회를 공급한다. 같은 두 배열을 직접 빌린다.
struct HtmlBodyParserHooks {
    std::function<bool()> hasOutput; // 현재 파서 +8이 비0인지 읽는다.
    std::function<void(std::span<char>,std::span<char>)> bind; // 비0 출력이 없을 때만 두 배열/8192 한도를 설치한다.
    std::function<void()> run; // 실제 기본/전용 HTML 순회다.
};
// 실제46c9c0/ca50/ca70/cac0/cbc0/cc30이며 일반 Tell 부모와 구별한다.
class HtmlBodyGump {
public:
    HtmlBodyGump(HtmlBodyGumpState& state,HtmlBodyGumpHooks hooks); // 같은 현재 화면/색과 실제 트리의 수명을 빌린다.
    std::uint32_t Construct(std::uint32_t token) const; // 기본 생성 뒤 자식0/두 첫NUL/화면 절반 크기를 설치한다.
    void Clear(std::uint32_t token) const; // 현재 표시 자식을 Kill한 뒤0으로 쓴다.
    void Parse(std::uint32_t token,const HtmlBodyParserHooks& parser) const; // 두 첫바이트0→필요 시 출력 설치→실제 순회다.
    void SetText(std::uint32_t token,std::string_view text) const; // 이전 자식 Kill→새 다중 줄/본문/scroll0/Slider·scrollable→현재 부모 재부착이다.
    void SetHtml(std::uint32_t token,std::string_view source,const std::function<void(std::string_view,std::span<char>,std::span<char>)>& parse) const; //0046cc60: 두 첫NUL→같은8192BYTE 본문/링크를 쓰는 기본 파서→현재 본문의 SetText다.
    void Fit(std::uint32_t token,float inset) const; // 현재 표시 자식 Push→부모 크기-가로 여백/같은 높이→부착이다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // 기본 부착 뒤 inset0으로 표시 자식을 다시 맞춘다.
private:
    HtmlBodyGumpNode& Node(std::uint32_t token) const; // 현재 실제 부모가 없으면 명시 거부한다.
    GumpCreateNode& Base(std::uint32_t token) const; // 현재 실제 기본/자식을 읽는다.
    HtmlBodyGumpState& state_; // 현재 원본 화면/식별/색 전역이다.
    HtmlBodyGumpHooks hooks_; // 기존 실제 생성/StyleText/트리/수명 경계다.
};
}
