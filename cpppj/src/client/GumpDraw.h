// 원본 공통 kind 2 Gump Draw의 임시 클립과 복구를 같은 UI 트리 저장소에 연결한다.
#pragma once
#include "client/GumpCollection.h"
#include "client/GumpVisual.h"

namespace netstorm::client {
// Gump flags +4의 이 비트만 임시 클립을 만든다. 배경 자식의 별도 비트와 구분한다.
inline constexpr std::uint32_t kGumpDrawClip=0x00400000;
// 원본 half-open 화면 전역과 양 끝 포함 VFX 전역이다. Draw/투영 경계가 현재 값을 바꿀 수 있다.
struct GumpDrawState { ScreenRect clip,vfxClip; };
// 004a1580/CD 00422e10처럼 두 전역을 함께 설정한다. right/bottom -1의 DWORD 감김도 유지한다.
void SetGumpDrawClip(GumpDrawState& state,ScreenRect clip);
// 기본 Gump와 텍스트 파생 Draw가 공유하는 원본 임시 ClipGump다. 복사는 복구 순서를 깨므로 금지한다.
class GumpClipScope {
public:
    // 경계 조회 이후의 현재 클립을 저장하고 bounds와 교차한다. 빈 클립도 그대로 설정한다.
    GumpClipScope(GumpDrawState& state,ScreenRect bounds);
    // 정상/예외 반환에서 저장한 논리 영역과 VFX 끝점을 함께 복구한다.
    ~GumpClipScope();
    // 하나의 보호 객체만 해당 클립을 복구하도록 복사 생성/대입을 금지한다.
    GumpClipScope(const GumpClipScope&)=delete;
    GumpClipScope& operator=(const GumpClipScope&)=delete;
private:
    GumpDrawState& state_;
    ScreenRect saved_;
};
// 각 Gump 가상 표 +0x24의 실제 처리기를 공급한다. 하위 버튼/배경/텍스트의 생성/분류는 호출자 책임이다.
struct GumpDrawHooks { std::function<void(std::uint32_t)> draw; };
// 00466310/CD 00492aa0의 전체 정상 분기와 C++ 예외 시 클립 보호다. 자식은 표시 목록에서 따로 그린다.
class GumpDraw {
public:
    // 수집기/클립/가상 처리기의 수명은 Draw와 연결한 공통 분배 동안 유지해야 한다.
    GumpDraw(const GumpCollection& collection,GumpDrawState& state,GumpDrawHooks hooks);
    // 현재 플래그를 읽고 필요할 때만 경계/캐시를 조회한다. 빈 클립에서도 가상 Draw를 호출한다.
    void Draw(std::uint32_t token) const;
private:
    const GumpCollection& collection_;
    GumpDrawState& state_;
    GumpDrawHooks hooks_;
};
// 공통 kind 2 제출이 수집기와 같은 Gump의 실제 클립 보호 경로를 사용하게 한다.
void BindGumpDraw(DrawDispatchHooks& hooks,const GumpDraw& draw);
// 버튼 선과 압축 해제한 배경/SHP 그림을 현재 VFX 클립으로 8비트 표면에 합성한다.
class GumpPixelTarget {
public:
    // 저장소/상태는 표면보다 오래 살아야 한다. pitch는 픽셀 행의 실제 바이트 수다.
    GumpPixelTarget(std::span<std::uint8_t> pixels,int pitch,int height,const GumpDrawState& state);
    // 양 끝 포함 직교 선을 순서대로 그리며 매 선에서 현재 클립을 읽는다.
    void Lines(std::span<const GumpLine> lines) const;
    // position은 해제한 이미지의 왼쪽 위다. 투명도/선택적 소스 색 치환을 유지한다.
    void Image(const IndexedImage& image,ScreenPoint position,const ColorMap* colors=nullptr) const;
    // position은 실제 VFX 기준점이다. 회전 없는 signed 16.16 배율을 원본 끝점/표본으로 현재 클립에 그린다.
    void ScaledImage(const IndexedImage& image,const ShapeFrame& frame,ScreenPoint position,
        std::int32_t scaleX,std::int32_t scaleY,const ColorMap* colors=nullptr) const;
    // 현재 클립 안의 half-open 사각형을 채운다. checker는 화면 절대 좌표의 x/y 홀짝이 다른 픽셀만 쓴다.
    void Fill(ScreenRect rect,std::uint32_t color,bool checker=false) const;
private:
    // 현재 inclusive 전역을 표면과 교차한 half-open 영역으로 바꾼다. 극단 끝점은 64비트로 계산한다.
    ScreenRect Clip() const;
    std::span<std::uint8_t> pixels_;
    int pitch_,height_;
    const GumpDrawState& state_;
};
}
