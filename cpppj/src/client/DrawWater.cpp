// 원문 좌표 저장·signed 반크기·현재 color 재조회와 실제 finder 순서를 보존한다.
#include "client/DrawWater.h"
#include "client/VFXLine.h"
#include "o/OriginalText.h"
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client { //장치 없는 집중 검사도 같은 실제 표시 계산을 사용한다.
namespace { //원문 DWORD 산술과 선택 경계만 공통 처리한다.
std::int32_t Add(std::int32_t value,std::uint32_t delta) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+delta); } //signed overflow 없이 원문 하위DWORD를 유지한다.
std::int32_t Sub(std::int32_t value,std::int32_t delta) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)-static_cast<std::uint32_t>(delta)); } //투영 좌표/반크기 빼기다.
std::int32_t Divide(std::int32_t value,std::int32_t denominator) { if (!denominator || (value==std::numeric_limits<std::int32_t>::min() && denominator==-1)) throw std::domain_error("DrawWater의 원문 viewport IDIV 예외");return value/denominator; } //원문 divide trap을 정상 가시영역으로 숨기지 않는다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("DrawWater 현재 화면 효과 미연결");return value; } //flag0/미선택 효과는 선행 요구하지 않는다.
float Coordinate(std::span<const std::uint8_t> slot,std::size_t offset) { std::uint32_t value{};for (std::size_t i=0;i<4;++i) value|=std::uint32_t(slot[offset+i])<<(8*i);return std::bit_cast<float>(value); } //정렬되지 않은 실제 pool+0e/+12의 float다.
std::int32_t FinderCoordinate(std::int32_t value,bool upper) { const float first=static_cast<float>(value);const float snapped=static_cast<float>(CameraFtolLow(static_cast<double>(first)+(upper ? static_cast<double>(std::bit_cast<float>(0x3f7fff58U)) : 0.0)));return CameraFtolLow(snapped); } //41d770/41d7a0의 두 float 저장 및 최종ftol이다.
}
//이름 비교는 첫NUL까지이며 파생 alias나 기본 debug 메뉴를 추가하지 않는다.
std::uint32_t DrawWaterScriptCode(std::string_view name) { return o::AsciiLower(name.substr(0,name.find('\0'))) == "drawwater" ? kDrawWaterCommand : 0xffffffffU; }
//원문은 현재 비영 전체DWORD를0으로 만들며 redraw store가 flag store보다 먼저다.
bool DispatchDrawWater(InputEvent event,std::uint32_t& enabled,std::uint32_t& worldRedraw) {
    if ((event.code&0x03ffffffU)!=kDrawWaterCommand) return false;if (event.code&InputCode::kRelease) return true;
    const auto next=static_cast<std::uint32_t>(enabled==0);worldRedraw=1;enabled=next;return true;
}
//ftol은 분수 원점을 절삭한 값이며 오른쪽/아래는 원문 signed 나눗셈 후 DWORD로 더한다.
ScreenRect DrawWaterVisibleArea(const RawCameraViewState& renderer,std::int32_t width,std::int32_t height) {
    const auto top=CameraFtolLow(std::bit_cast<float>(renderer.worldBits[1]));const auto bottom=Add(top,static_cast<std::uint32_t>(Divide(height,renderer.cellPixels[1])));
    const auto left=CameraFtolLow(std::bit_cast<float>(renderer.worldBits[0]));return {left,top,Add(left,static_cast<std::uint32_t>(Divide(width,renderer.cellPixels[0]))),bottom};
}
//큰 정수의 선행float32 반올림도 보존하며 finder의 현재 보드 제한은 기존 Begin이 수행한다.
o::SquidSearchArea DrawWaterFinderArea(ScreenRect visible) { return {FinderCoordinate(visible.left,false),FinderCoordinate(visible.top,false),FinderCoordinate(visible.right,true),FinderCoordinate(visible.bottom,true)}; }
//현재 실제 저장소를 빌리며 게임/해시 상태를 생성하지 않는다.
RawDrawWater::RawDrawWater(const o::SidPool& pool,const o::SquidHash& hash,std::span<const o::RiftTypeRecord> types):pool_(pool),hash_(hash),types_(types) {}
//전용 객체 bounds/지식/상태 필터를 추가하지 않고 실제 영역 탐색의 모든 현재 후보를 표시한다.
void RawDrawWater::Draw(const RawCameraViewState& renderer,const RawCameraViewState& snapshot,std::int32_t width,std::int32_t height,const std::uint32_t& enabled,const std::uint32_t& color,const DrawWaterHooks& hooks) const {
    if (!enabled) return;o::RawSquidFinder finder(pool_,hash_,types_);const auto area=DrawWaterFinderArea(DrawWaterVisibleArea(renderer,width,height));
    for (auto sid=finder.Begin(area,0);sid.value;sid=finder.Next()) { //원문 current의 다음 체인을 미리 저장하는 같은 네 단계 finder다. free/dead/void를 새로 거르지 않는다.
        const auto slot=pool_.Slot(sid);const auto point=ProjectCameraView(snapshot,Coordinate(slot,14),Coordinate(slot,18));const auto halfX=snapshot.cellPixels[0]/2,halfY=snapshot.cellPixels[1]/2;
        ScreenRect rectangle{Sub(point[0],halfX),Sub(point[1],halfY),Sub(point[0],halfX),Sub(point[1],halfX)}; //원문 left==right 및 bottom의 halfX 사용을 일반 bounds로 고치지 않는다.
        Need(hooks.clip)(rectangle);
        for (std::uint32_t index=0;index<3;++index) { //각 호출 직전 현재005ad128을 다시 읽고 네 변을 한 픽셀씩 확장한다.
            Need(hooks.rectangle)(rectangle,color);rectangle={Add(rectangle.left,0xffffffffU),Add(rectangle.top,0xffffffffU),Add(rectangle.right,1),Add(rectangle.bottom,1)};
        }
    }
    Need(hooks.present)(); //후보가0개여도 flag가 비영이면 전체 viewport 출력/clip 복구를 한 번 실행한다.
}
//원문 wrapper와 같은 상단/하단/왼쪽/오른쪽 순서이며 뒤 선이 같은 모서리를 다시 쓴다.
// 434970의 do-while 세 번 반복을 좌표 계산만으로 옮긴다. 감김은 부호 없는 DWORD 연산으로 정의된 동작으로 보존한다(M7.4, 1271).
std::array<ScreenRect,3> PathMarkerRectangles(std::array<std::int32_t,2> point,std::array<std::int32_t,2> cellPixels) {
    const auto bits=[](std::int32_t value) { return std::bit_cast<std::uint32_t>(value); };const auto wrap=[](std::uint32_t value) { return std::bit_cast<std::int32_t>(value); };
    std::int32_t right=point[0],bottom=point[1],left=wrap(bits(point[0])-bits(cellPixels[0])),top=wrap(bits(point[1])-bits(cellPixels[1])); // local_8/local_4와 iVar1/iVar2다.
    std::array<ScreenRect,3> result{};
    // 먼저 줄이고 기록하는 세 겹 반복이다.
    for (auto& rectangle:result) { bottom=wrap(bits(bottom)-1U);right=wrap(bits(right)-1U);top=wrap(bits(top)+1U);left=wrap(bits(left)+1U);rectangle=ScreenRect{left,top,right,bottom}; }
    return result;
}
void DrawWaterRectangle(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect fullClip,ScreenRect rectangle,std::uint32_t color) {
    const auto right=Add(rectangle.right,0xffffffffU),bottom=Add(rectangle.bottom,0xffffffffU);
    DrawVfxLine(pixels,pitch,height,fullClip,{rectangle.left,rectangle.top},{right,rectangle.top},color);
    DrawVfxLine(pixels,pitch,height,fullClip,{rectangle.left,bottom},{right,bottom},color);
    DrawVfxLine(pixels,pitch,height,fullClip,{rectangle.left,rectangle.top},{rectangle.left,bottom},color);
    DrawVfxLine(pixels,pitch,height,fullClip,{right,rectangle.top},{right,bottom},color);
}
}
