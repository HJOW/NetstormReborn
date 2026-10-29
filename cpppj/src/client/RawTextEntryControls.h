// 공통 등록표·글꼴·배경·Slider에 실제 편집 노드를 연결한다.
#pragma once
#include "client/RawTextEntryContext.h"
#include "client/PaletteColors.h"
#include <memory>
namespace netstorm::client { // 원문 편집 UI의 실제 조립 계층이다.
struct RawMenuTree;class GumpTextDraw;class GumpPixelTarget;class ScrollGumpWidgets;class FontStore;class TextEntryStorage;class GumpBackgroundDraw; // 같은 장기 공통 장치다.
class RawTextEntryControls { // 생성·입력·Draw·수명이 같은 노드/문자 물리를 사용한다.
public:
    RawTextEntryControls(RawMenuTree tree,FontStore& fonts,const GumpBackgroundCreate& background,const GumpBackgroundDraw& draw,
        const ScrollGump& slider,const ScrollGumpWidgets& widgets,std::function<ScrollGumpNode*(std::uint32_t)> sliderNode,
        std::function<std::uint32_t(std::size_t)> allocateSlider,RawTextEntryContext context); // 공통 장치는 이 조립 계층보다 오래 유지한다.
    ~RawTextEntryControls(); // 노드는 공통 수명에서 먼저 반납한다.
    void SetPalette(const PaletteColorTable& palette); // 현재 실제 파일 팔레트 색을 사용 지점에 공급한다.
    void SetRecipientValidator(std::function<bool(std::string_view)> validator); //먼저만든이름입력창의같은Console처리기에도실제Chat대상검사를설치한다.버퍼/시계/clipboard는재생성하지않는다.
    std::uint32_t Allocate(bool console); // 기본 생성 전에도 정확한 파생 노드와 소유 토큰을 확보한다.
    std::uint32_t ConstructConsole(std::int32_t width,std::int32_t height,std::optional<std::string_view> text,
        std::uint32_t font,std::uint32_t background,std::uint32_t color); // 실제 기본 버퍼·Slider 생성과 Console 파생 초기화다.
    TextEntryGumpNode* Node(std::uint32_t token) const; // 실제 공통 기본 주소와 같은 노드다.
    ConsoleGumpNode* Console(std::uint32_t token) const; // Console 파생인 경우에만 현재 주소다.
    const TextEntryGump& Backend() const; // ChatWindow도 같은 처리기에 연결한다.
    TextEntryStorage& Storage() const; // 실제 문자/줄표 물리와 처리기를 빌린다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // 실제 두 Slider 부착을 포함한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; // 실제 Console/기본 분배다.
    void Idle(std::uint32_t token) const; // 실제 벽시계 점멸이다.
    void Draw(std::uint32_t token,const GumpTextDraw& text,const GumpPixelTarget& pixels) const; // 현재 공통 클립/표면에만 그린다.
    void DeleteScalar(std::uint32_t token,std::uint32_t flags); // 원문 본문 소멸 후 객체 반납이다.
    void Release(std::uint32_t token); // 공통 scalar의 저장소 반납 경계다.
    void Close(); // 미부착/부분 생성도 등록표와 같은 자식 우선 수명으로 반납한다.
private:
    struct Impl;std::unique_ptr<Impl> impl_; // 노드/문자 물리/가상 콜백의 주소를 고정한다.
};
}
