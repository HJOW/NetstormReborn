// 원본 돌 버튼 배경의 화면 기준 질감 반복·2픽셀 명암 테두리 출력 계획이다.
#pragma once
#include "client/Screen.h"
#include "client/VFXDraw.h"
#include <vector>

namespace netstorm::client {
namespace GumpBackgroundFlags {
inline constexpr std::uint32_t kTexture=0x02000000; // 질감 경로 선택. 단색/체크 fill 경로는 이 API의 범위 밖이다.
inline constexpr std::uint32_t kLeft=0x01000000; // 왼쪽 2픽셀 테두리를 그린다.
inline constexpr std::uint32_t kTop=0x00800000; // 위쪽 2픽셀 테두리를 그린다.
inline constexpr std::uint32_t kRight=0x00400000; // 오른쪽 2픽셀 테두리를 그린다.
inline constexpr std::uint32_t kBottom=0x00200000; // 아래쪽 2픽셀 테두리를 그린다.
inline constexpr std::uint32_t kDark=0x00100000; // 어두운 RGB 변환표를 선택한다.
inline constexpr std::uint32_t kBright=0x00080000; // 밝은 RGB 변환표를 선택한다. 두 명암 비트가 함께 있으면 우선한다.
inline constexpr std::uint32_t kPressed=0x00040000; // 눌렸을 때 테두리 방향을 바꾼다. 바탕 전체를 어둡게 하지 않는다.
inline constexpr std::uint32_t kButton=0x03e00000; // 부모 버튼 생성자가 배경 자식에 전달하는 기본 플래그다.
}
// 하나의 실제 질감 함수 호출: 요청 영역/플래그, 일시 클립과 화면 기준 SHP 기준점 목록이다.
struct GumpTexturePass {
    ScreenRect rect,clip;
    std::uint32_t flags{};
    unsigned shade{}; // 0 원래 색, 1 밝은 표, 2 어두운 표.
    bool validTile{}; // SHP signed short 폭/높이가 각각 2 이상일 때만 clip/map/draw 호출을 한다.
    std::vector<ScreenPoint> origins;
};
// 00465880→00465690/CD 00494a20→00494720 질감 경로다. SHP 메타 폭/높이에서 1을 뺀 주기를 사용한다.
// rect/clip은 순서가 정상인 사각형으로 전달한다. 음수 원점·빈 영역은 허용하고 원본 나눗셈은 0 방향으로 자른다.
std::vector<GumpTexturePass> PlanGumpBackground(ScreenRect rect,ScreenRect clip,std::int16_t width,std::int16_t height,std::uint32_t flags);
// pass 순서로 원본 색/명암 색을 합성한다. frameOffset은 SHP rect의 기준점 상대 left/top이며 opacity는 유지한다.
void DrawGumpBackground(IndexedImage& canvas,const IndexedImage& texture,ScreenPoint frameOffset,
    std::span<const GumpTexturePass> passes,const GumpShadeMaps& maps);
}
