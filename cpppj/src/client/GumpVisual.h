// 원본 Buttongump의 팔레트 색·글자 문맥·외곽선/글자 배치 계획이다. 자식 배경/입력 수명은 별도 계층이 맡는다.
#pragma once
#include "client/Screen.h"
#include "client/VFXDraw.h"
#include <vector>

namespace netstorm::client {
// Screen 팔레트 로더의 전역에서 복사하는 버튼 색 번호다. 일시 번개 팔레트에서 다시 검색하지 않는다.
struct ButtonPalette { std::uint32_t black{},white{},edge{}; };
// 원본 32바이트 TextContext 순서다. extra는 +0x10 필드이며 이 단계의 버튼 출력에서는 사용하지 않는다.
struct GumpTextContext {
    std::uint32_t flags{},font{},color{},style{},extra{},shadowColor{};
    std::int32_t shadowX{},shadowY{};
};
// 팔레트 기본 검정/흰색과 RGB(55,51,54)의 005bec38 색 번호를 읽는다. 파일 로딩을 마친 표만 전달한다.
ButtonPalette ButtonColors(const PaletteColorTable& palette);
// 004254b0/CD 00441310의 정상·눌림·비활성 문맥이다. 읽지 않는 미초기화 필드는 안전 API에서 0으로 둔다.
std::array<GumpTextContext,3> ButtonTextContexts(ButtonPalette colors);
// 원본 VFX 선의 양 끝은 포함한다. 색 번호는 원본 DWORD를 보존하고 실제 8비트 출력에서 좁힌다.
struct GumpLine { ScreenPoint first,last;std::uint32_t color{}; };
// 부모 버튼의 화면 사각형, 측정한 글자 폭/높이, 두 눌림 상태(+a0/+a4), 버튼 플래그(+ac)다.
struct ButtonVisualState {
    ScreenRect rect;
    std::int32_t textWidth{},textHeight{},value{},pressed{};
    std::uint32_t flags{};
};
// 원본 그리기 함수의 선 호출 목록과 상대 글자 위치/문맥이다. 순서를 유지하여 모서리 덮어쓰기를 재현한다.
struct ButtonDrawPlan {
    std::vector<GumpLine> lines;
    ScreenPoint textOffset;
    GumpTextContext text;
};
// 00424ff0/CD 00441660: 플래그 8은 외곽선 생략, 플래그 1은 비활성 문맥 우선이다. 두 상태 중 하나라도 0이 아니면 눌림 배치를 쓴다.
ButtonDrawPlan PlanButtonDraw(const ButtonVisualState& state,const std::array<GumpTextContext,3>& contexts,std::uint32_t edge);
// 원본의 수평/수직 선 계획을 클리핑한 8비트 그림에 그린다. 선 끝점도 포함하며 역방향 선을 처리한다.
void DrawGumpLines(IndexedImage& canvas,std::span<const GumpLine> lines);
}
