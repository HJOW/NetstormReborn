// 원본10.78 Cursor.cpp의 위치 판정445440·연관 지역 검사445340·화면 갱신4458e0이다.
#pragma once
#include "client/RawCursorLifecycle.h"
#include "o/CanonTypeDecoder.h"
#include "o/RawCanonPlacementPipeline.h"

namespace netstorm::client {
class Renderer; // 갱신 사각형을 실제8비트 표면에 전달하는 기존 출력기다.
// 내려놓기/수명 상태와 겹치지 않는 원본 커서 전역만 보관한다.
struct CursorPlacementState {
    std::uint32_t placementFlags{}; // 55a2dc: 배치 경고 조건이다. Drop.costFlags의 Take 호출 플래그55a2e0과 별도다.
    std::uint32_t placementAllowed{},descriptionAllowed{},spaceAllowed{},sourceAllowed{}; // 55a290/294/298/29c의 원래 DWORD다.
    std::uint32_t obstruction{},acceptEvent{0x0200003f}; // 59ab2c의 공간 판정과5427f4의 PE 기본 Gump 수신 이벤트다.
    std::int32_t labelSpacing{16}; // 5325e0: 설명의 한 글자당 세로 여백과 가로 여백이다.
    std::array<std::int32_t,4> bounds{}; // 55a2b0..bc: 기준점 x/y와 전체 폭/높이다. 좌상/우하 좌표가 아니다.
    std::int32_t hotspotX{},hotspotY{},imageWidth{},imageHeight{}; // 55a2c0..cc의 첫 그림 signed WORD다.
    ScreenPoint screenOrigin{}; // 59a91c/920: 금액 출력에서 빼는 현재 화면 원점이다.
    std::string description; // 55a308의 현재 NUL 종료 설명 문구다.
};
// 원본 CanonDecoder의 한 유효 칸이다. 실제 반복자는 아래 연결기가 공급한다.
struct CursorCanonCell {
    std::int32_t frame{}; // 현재 물리 프레임 번호다.
    float x{},y{}; // decoder가 저장한 단정도 좌표다.
};
// 공간/GUI/표시의 실제 공급자가 연결되어야 한다. 판정 본문을 성공 기본값으로 대체하지 않는다.
struct CursorPlacementHooks {
    std::function<std::uint32_t(std::string&,float,float,std::uint32_t,std::uint32_t)> describe; // 4734d0: 문구와 원래 DWORD 허용값이다.
    std::function<void(o::CanonTypeQuery,const std::function<bool(CursorCanonCell)>&)> walk; // 실제425c20/425860; false이면 Advance 없이 끝낸다.
    std::function<CursorCostImage(std::uint32_t,std::int32_t)> image; // 419850(TYPE,0,frame)의 signed 추가 헤더다.
    std::function<std::uint32_t(std::int32_t,std::int32_t)> gumpAt; // 465ed0: 현재 화면 위치의 Gump다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> accept; // 4651b0(this,event,0,0,0)이다.
    std::function<std::uint32_t(std::uint32_t)> parent; // 수신 Gump+24의 실제 부모 SID다.
    std::function<std::uint32_t(const o::CanonPlacementQuery&)> mayPlace; // 49b510의 TYPE/argument/반올림 좌표/방향/owner BYTE/1이다.
    std::function<o::Sid(o::Sid)> source; // Form 가상+0: 원래 연관 객체다.
    std::function<std::int32_t(o::Sid)> region; // 462c80: 객체의 현재 연결 지역 번호다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> processEvent; // 마나/범위 프로세스 가상+1c다.
    std::function<ScreenPoint(float,float)> project; // 497220의 첫 출력 점이다.
    std::function<void(ScreenRect,std::uint32_t)> invalidate; // 497580: 원본 사각형과 flags2/0이다.
};
// 같은 내려놓기·수명·위치 상태를 읽으며 실제 판정과 dirty 사각형을 계산한다.
class RawCursorPlacement {
public:
    // 참조한 단일 풀/타입/표면 지도/상태는 이 객체보다 오래 살아야 한다.10.78만 허용한다.
    RawCursorPlacement(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
        std::span<const std::uint16_t> surfaceMap,CursorDropState& drop,CursorLifecycleState& lifecycle,
        CursorPlacementState& state,CursorPlacementHooks hooks);
    // 445440: 표시 범위를 계산하고 월드/Gump/연관 지역 허용 및 프로세스 전이를 반영한다.
    void Query() const;
    // 445340: flags20이면 한 Canon 칸이라도 연관 객체와 같은 연결 지역인지 확인한다.
    std::uint32_t SourceAllowed() const;
    // 4458e0: 미리보기/경고/금액의 갱신 사각형을 원래 순서대로 보낸다.
    void Prepare() const;
    // 연결기에서 단일 풀 계약을 확인한다.
    const o::SidPool& Pool() const;
private:
    // 현재 abstract TYPE의 범위를 확인한다. 커서의 별도 type 번호로 바꾸지 않는다.
    const o::RiftTypeRecord& Type() const;
    const o::SidPool& pool_; // Form/첫 미리보기 좌표를 읽는 단일 풀이다.
    std::span<const o::RiftTypeRecord> types_; // 메뉴/건설과 같은 현재 TYPE 표다.
    std::span<const std::uint16_t> surfaceMap_; // 542514의256×256 표면 SID 지도다.
    CursorDropState& drop_; // legal/type/argument/flags/좌표/Gump의 공유 전역이다.
    CursorLifecycleState& lifecycle_; // 현재 Gump/abstract TYPE/목록/글자 치수의 공유 전역이다.
    CursorPlacementState& state_; // 표시 범위와 나머지 허용 DWORD다.
    CursorPlacementHooks hooks_; // 명시 공간/GUI/그리기 경계다.
};
// 취득/취소/내려놓기가 같은 실제 위치 판정과 화면 준비 몸체를 사용하게 한다.
void BindCursorPlacement(CursorLifecycleHooks& lifecycle,CursorDropHooks& drop,RawCursorPlacement& placement);
// 같은 실제 자산의 Canon/SHP만 연결한다. 장치나 배치 계산을 대체하지 않는다.
void BindCursorPlacementAssets(CursorPlacementHooks& hooks,const GameAssets& assets,const o::PriestPlacementGeometryState& geometry);
// 현재 자산의 프레임/Canon 패턴과 같은 배치 처리 흐름/출력기를 연결한다. GUI와 설명/지역은 유지한다.
void BindCursorPlacementWorld(CursorPlacementHooks& hooks,const o::SidPool& pool,const GameAssets& assets,
    const o::PriestPlacementGeometryState& geometry,o::RawCanonPlacementPipeline& placement,Renderer& renderer);
}
