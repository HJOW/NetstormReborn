// DrawWater 일반 명령과 원문 후단의 실제 영역 finder/객체 표시를 같은 Client 상태에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include "client/RawCameraView.h"
#include "client/Screen.h"
#include "o/RawSquidFinder.h"

namespace netstorm::client { //일반 물결 배경 선택과 구별되는 원문의 진단 사각형 표시다.
inline constexpr std::uint32_t kDrawWaterCommand=0x0200001aU; //00542760의 실제 명령이며 기본 메뉴 항목을 추가하지 않는다.
std::uint32_t DrawWaterScriptCode(std::string_view name); //실제 원문 이름표의 ASCII 비교다.
bool DispatchDrawWater(InputEvent event,std::uint32_t& enabled,std::uint32_t& worldRedraw); //Guide/Gump/release 뒤 현재 DWORD 토글과 worldRedraw=1을 원문 순서로 쓴다.
//00497530은 snapshot 대신 현재 Renderer view와 전체 화면 폭/높이를 읽는다.
ScreenRect DrawWaterVisibleArea(const RawCameraViewState& renderer,std::int32_t width,std::int32_t height);
//0041def0의 입력 float32 및 lower/upper snap→F32→ftol 저장을 실제 기존 finder에 전달한다.
o::SquidSearchArea DrawWaterFinderArea(ScreenRect visible);
//실제 화면의 clip/외곽선/전체 viewport 출력만 장치에 위임한다.
struct DrawWaterHooks {
    std::function<void(ScreenRect)> clip; //004a1580은 각 객체의 초기0폭/비대칭 사각형을 그대로 쓴다.
    std::function<void(ScreenRect,std::uint32_t)> rectangle; //004a2270의 네 선·임시 전체 clip·잠금과 복구다.
    std::function<void()> present; //004a46d0의 전체 viewport 즉시 출력 및 현재 clip 복구다. Unlock과 다르다.
};
//실제 Pool/Hash/TYPE를 복제하지 않고 같은 현재 네 단계 영역 finder를 빌린다.
class RawDrawWater {
public:
    RawDrawWater(const o::SidPool& pool,const o::SquidHash& hash,std::span<const o::RiftTypeRecord> types); //생성 자체는 순회/표시/상태 변경을 하지 않는다.
    void Draw(const RawCameraViewState& renderer,const RawCameraViewState& snapshot,std::int32_t width,std::int32_t height,const std::uint32_t& enabled,const std::uint32_t& color,const DrawWaterHooks& hooks) const; //실제00434dd0이며 flag는 입구에서만 검사한다.
private:
    const o::SidPool& pool_;const o::SquidHash& hash_; //Client의 같은 장기 물리이며 Session/Renderer 교체로 복사하지 않는다.
    std::span<const o::RiftTypeRecord> types_; //기존 finder가 현재 TYPE 발자국을 매 후보에서 다시 읽는다.
};
//원문004a2270의 네 선을 기존 실제 VFX로 그린다. right/bottom -1을 감김으로 보존하고 역전/0폭을 임의 생략하지 않는다.
void DrawWaterRectangle(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect fullClip,ScreenRect rectangle,std::uint32_t color);
// 원문 434970의 세 겹 사각형 좌표다. 투영한 화면점을 오른쪽 아래로 하고 현재 칸 크기(0059a908/90c)만큼 왼쪽 위를 정한 뒤, 매 겹마다 먼저 한 픽셀씩 줄인다(signed DWORD 감김 보존, M7.4, 1271).
std::array<ScreenRect,3> PathMarkerRectangles(std::array<std::int32_t,2> point,std::array<std::int32_t,2> cellPixels);
}
