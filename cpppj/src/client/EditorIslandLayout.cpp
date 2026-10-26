// 가상 효과를 넘어서 기하/자식 포인터를 캐시하지 않고 원문 x87 저장 시점을 유지한다.
#include "client/EditorIslandLayout.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client { // 배치와 미리보기는 동일한 실제 창/자식 노드를 수정한다.
namespace { // 실제 도달한 장치만 필수로 요구하며 미연결을 성공으로 숨기지 않는다.
template<class T> const T& IslandNeed(const T& effect) { if (!effect) throw std::logic_error("섬 추가 창의 실제 배치 장치 미연결");return effect; } // 현재 가상 효과의 명시 연결 관문이다.
// 원문 CRT ftol은64비트 절삭의 하위 DWORD를 사용하며 비유한/범위 밖은0이다.
std::int32_t IslandFtol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 원문 SHL/ADD의32비트 감김 뒤 signed 정수를 float으로 바꾼다.
float IslandGridPosition(std::int32_t chunk,std::int32_t origin) { return static_cast<float>(std::bit_cast<std::int32_t>((static_cast<std::uint32_t>(chunk)<<4U)+static_cast<std::uint32_t>(origin))); }
}
// 필수 장치의 확인은 실제 도달 지점에서 수행한다.
EditorIslandLayout::EditorIslandLayout(EditorIslandLayoutHooks hooks):hooks_(std::move(hooks)) {}
// 창 필드는 가상 Push/Attach 뒤에 다시 읽는다.
EditorIslandLayoutFields& EditorIslandLayout::Fields(std::uint32_t token) const { return IslandNeed(hooks_.fields)(token); }
// 자식의 기본/배율 크기와 깊이는 같은 공통 노드에 쓴다.
GumpCreateNode& EditorIslandLayout::Node(std::uint32_t token) const { auto* node=token ? IslandNeed(hooks_.node)(token) : nullptr;if (!node) throw std::out_of_range("섬 추가 창의 실제 Gump 없음");return *node; }
// 색상과 Image 주소를 공통 기본 노드의 복사본에 쓰지 않는다.
MetaImageGumpNode& EditorIslandLayout::Image(std::uint32_t token) const { auto* node=token ? IslandNeed(hooks_.image)(token) : nullptr;if (!node) throw std::out_of_range("섬 추가 창의 실제 MetaImage 없음");return *node; }
// 원문 fst(float)는 x87 레지스터 값을 버리지 않으므로 배율 곱셈은 좁히기 전 값이다.
void EditorIslandLayout::Width(std::uint32_t token,double value) const { auto& node=Node(token);node.baseWidth=static_cast<float>(value);node.width=static_cast<float>(value*node.scale); }
// 높이도 원문과 같은 기본값/배율값 순서로 저장한다.
void EditorIslandLayout::Height(std::uint32_t token,double value) const { auto& node=Node(token);node.baseHeight=static_cast<float>(value);node.height=static_cast<float>(value*node.scale); }
// 실제 원문은 Image를 먼저 측정한 후 기본 부착과 일곱 자식의 가상 효과를 실행한다.
void EditorIslandLayout::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    auto& fields=Fields(token);if (!fields.image) throw std::logic_error("섬 추가 창의 현재 지도 Image 없음");Image(fields.map).image=fields.image;
    IslandNeed(hooks_.resizeImage)(Fields(token).map);
    Width(Fields(token).background,static_cast<double>(Node(Fields(token).map).baseWidth)+32.0);
    Height(Fields(token).background,static_cast<double>(Node(Fields(token).map).baseHeight)+32.0);
    IslandNeed(hooks_.baseAttach)(token,x,y,parent,mode);
    IslandNeed(hooks_.push)(Fields(token).title);IslandNeed(hooks_.push)(Fields(token).background);IslandNeed(hooks_.push)(Fields(token).map);
    IslandNeed(hooks_.push)(Fields(token).hidden);IslandNeed(hooks_.push)(Fields(token).list);IslandNeed(hooks_.push)(Fields(token).done);IslandNeed(hooks_.push)(Fields(token).tell);
    Width(Fields(token).hidden,static_cast<double>(Node(token).baseWidth)-(static_cast<double>(Node(Fields(token).background).baseWidth)+40.0));
    Height(Fields(token).hidden,static_cast<double>(Node(token).baseHeight)-(static_cast<double>(Node(Fields(token).done).baseHeight)+Node(Fields(token).title).baseHeight+48.0));
    Width(Fields(token).tell,static_cast<double>(Node(token).baseWidth)-(static_cast<double>(Node(Fields(token).background).baseWidth)+40.0));
    Height(Fields(token).tell,static_cast<double>(Node(token).baseHeight)-(static_cast<double>(Node(Fields(token).done).baseHeight)+Node(Fields(token).title).baseHeight+48.0));
    Width(Fields(token).list,Node(Fields(token).background).baseWidth);
    Height(Fields(token).list,static_cast<double>(Node(token).baseHeight)-(static_cast<double>(Node(Fields(token).done).baseHeight)+Node(Fields(token).background).baseHeight+Node(Fields(token).title).baseHeight+48.0));
    IslandNeed(hooks_.attach)(Fields(token).background,16,static_cast<float>(static_cast<double>(Node(Fields(token).title).baseHeight)+24.0),token,0);
    IslandNeed(hooks_.attach)(Fields(token).map,0,0,Fields(token).background,2);
    IslandNeed(hooks_.attach)(Fields(token).title,0,16,token,2);
    IslandNeed(hooks_.attach)(Fields(token).hidden,static_cast<float>(static_cast<double>(Node(Fields(token).background).baseWidth)+24.0),static_cast<float>(static_cast<double>(Node(Fields(token).title).baseHeight)+24.0),token,0);
    IslandNeed(hooks_.attach)(Fields(token).tell,static_cast<float>(static_cast<double>(Node(Fields(token).background).baseWidth)+24.0),static_cast<float>(static_cast<double>(Node(Fields(token).title).baseHeight)+24.0),token,0);
    IslandNeed(hooks_.attach)(Fields(token).list,16,static_cast<float>(static_cast<double>(Node(Fields(token).background).baseHeight)+Node(Fields(token).title).baseHeight+32.0),token,0);
    IslandNeed(hooks_.attach)(Fields(token).done,0,static_cast<float>((static_cast<double>(Node(token).baseHeight)-16.0)-Node(Fields(token).done).baseHeight),token,2);
}
// 원문 미리보기는 Bounds 원점을 ftol 전에 빼며 click-drop의 두 단계 절삭과 구별한다.
void EditorIslandLayout::Motion(std::uint32_t token,std::int32_t x,std::int32_t y) const {
    auto* state=Fields(token).interaction;if (!state) throw std::logic_error("섬 추가 창의 실제 드래그 상태 없음");if (!state->drag) IslandNeed(hooks_.assertion)("dragIsland != NULL");
    IslandNeed(hooks_.push)(state->drag);const auto bounds=IslandNeed(hooks_.bounds)(Fields(token).map);auto& drag=Image(state->drag);
    const auto gridX=IslandFtol(static_cast<double>(x)-static_cast<double>(drag.baseWidth)*0.5-bounds.left)/16;
    const auto gridY=IslandFtol(static_cast<double>(y)-static_cast<double>(drag.baseHeight)*0.5-bounds.top)/16;
    drag.colorMode=1;
    if (gridX>=0 && gridX<14 && gridY>=0 && gridY<14) {
        state->record[2]=static_cast<std::uint8_t>((gridY<<4)|(gridX&15));
        if (IslandNeed(hooks_.canPlace)(state->record)) Image(state->drag).colorMode=0;
    }
    const auto position=IslandNeed(hooks_.position)(Fields(token).map);
    IslandNeed(hooks_.attach)(state->drag,IslandGridPosition(gridX,position.x),IslandGridPosition(gridY,position.y),IslandNeed(hooks_.root)(),0);
    Node(state->drag).depth=0xffffff9cU;
}
}
