#include "client/GumpVisual.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 x86 정수 덧셈의 DWORD 감김을 유지한다. 화면/폭 경계에서 C++ signed overflow를 만들지 않는다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
// 원본 x86 정수 뺄셈의 DWORD 감김을 유지한다. 나눗셈 전 중간 값에도 적용한다.
std::int32_t Sub(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b)); }
}
// 파일 로더의 저장 표에서 원본 버튼 외곽선 전역을 찾는다. 목표 색이 없는 미초기화 표는 거부한다.
ButtonPalette ButtonColors(const PaletteColorTable& palette) {
    // 판본마다 전체 표의 위치가 다르므로 목표 RGB를 찾고 저장된 결과 번호를 사용한다.
    for (const auto& color:palette.named) if (color.red==55 && color.green==51 && color.blue==54)
        return {palette.basic[0],palette.basic[7],color.index};
    throw std::invalid_argument("Button palette table is not loaded");
}
// 활성 두 문맥은 그림자 플래그 2와 오프셋 (1,1), 비활성은 그림자 없이 검정 글자를 쓴다.
std::array<GumpTextContext,3> ButtonTextContexts(ButtonPalette colors) {
    // 정상과 눌림에 공통으로 쓰는 기본 글꼴/흰 잉크/검정 그림자 문맥이다.
    const GumpTextContext active{2,0,colors.white,0,0,colors.black,1,1};
    // 비활성은 검정 잉크만 쓰고 읽지 않는 필드를 안전하게 0으로 둔다.
    const GumpTextContext disabled{0,0,colors.black,0,0,0,0,0};
    return {active,active,disabled};
}
// 원본 선/상대 위치/문맥을 출력 계획으로 분리한다. 배경 자식과 글자 래스터 출력은 경계로 남긴다.
ButtonDrawPlan PlanButtonDraw(const ButtonVisualState& state,const std::array<GumpTextContext,3>& contexts,std::uint32_t edge) {
    auto rect=state.rect;ButtonDrawPlan result;
    // 두 원본 상태 중 어느 하나라도 설정되어 있으면 글자 배치를 한 픽셀 민다.
    const bool down=state.value!=0 || state.pressed!=0;
    if ((state.flags&8)==0) {
        result.lines={{{Add(rect.left,1),rect.top},{Sub(rect.right,2),rect.top},edge},
            {{Add(rect.left,1),Sub(rect.bottom,2)},{Sub(rect.right,2),Sub(rect.bottom,2)},edge},
            {{rect.left,Add(rect.top,1)},{rect.left,Sub(rect.bottom,3)},edge},
            {{Sub(rect.right,1),Add(rect.top,1)},{Sub(rect.right,1),Sub(rect.bottom,3)},edge}};
        rect={Add(rect.left,1),Add(rect.top,1),Sub(rect.right,1),Sub(rect.bottom,2)};
    }
    result.textOffset={Add(Sub(Sub(rect.right,state.textWidth),rect.left)/2,down ? 3 : 2),
        Add(Sub(Sub(rect.bottom,state.textHeight),rect.top)/2,down ? 2 : 1)};
    result.text=contexts[(state.flags&1)!=0 ? 2 : down ? 1 : 0];return result;
}
// 항상 수평/수직인 원본 네 선을 그림 경계와 교차시킨 뒤 양 끝을 포함하여 채운다.
void DrawGumpLines(IndexedImage& canvas,std::span<const GumpLine> lines) {
    if (canvas.indices.size()!=std::size_t(canvas.width)*canvas.height) throw std::invalid_argument("Invalid GUI canvas");
    // 원본 호출 순서대로 그려 겹치는 픽셀을 뒤 선이 덮게 한다.
    for (const auto& line:lines) {
        if (line.first.x!=line.last.x && line.first.y!=line.last.y) throw std::invalid_argument("GUI line is not axial");
        const auto left=std::max<std::int64_t>(0,std::min(line.first.x,line.last.x));
        const auto top=std::max<std::int64_t>(0,std::min(line.first.y,line.last.y));
        const auto right=std::min<std::int64_t>(canvas.width,std::int64_t(std::max(line.first.x,line.last.x))+1);
        const auto bottom=std::min<std::int64_t>(canvas.height,std::int64_t(std::max(line.first.y,line.last.y))+1);
        if (left>=right || top>=bottom) continue;
        // 각 유효 줄을 8비트 팔레트 번호로 채운다. 끝점 +1은 64비트로 계산해 넘침을 막는다.
        for (auto y=top;y<bottom;++y) std::fill(canvas.indices.begin()+y*canvas.width+left,canvas.indices.begin()+y*canvas.width+right,static_cast<std::uint8_t>(line.color));
    }
}
}
