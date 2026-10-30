// 같은 캠페인 풀의 실제 Factory/아웃포스트를 원본 수집/일반 Draw에 연결한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "client/Renderer.h"
#include <functional>
namespace netstorm::o { struct RenderCollectionEntry; } //공통16바이트 표시 항목은 참조로 빌리며 이 헤더에서 새 정의를 만들지 않는다.
namespace netstorm::client {
class GameAssets;class OwnerColorTableState; // 실제 자산과 장기 현재 소유자 색표를 따로 빌린다.
// 실제 건물의 본체/추가 그림과 비행/벌룬 그림자를 같은 원본 수집/일반Draw에 분배한다.
class RawCampaignFactoryDisplay {
public:
    using TempleDraw=std::function<void(o::Sid,const std::function<void(std::int32_t)>&)>; // 같은 Session의 실제+a4와 조건부 음향 경계다.
    RawCampaignFactoryDisplay(RawWorldCore core,const GameAssets& assets,TempleDraw templeDraw={},const std::uint32_t* patchDiagnostic=nullptr); // 실제 GameWorld는 장기005318ec도 빌린다. 독립 표시 검사는 PE 초기값6을 유지한다.
    ~RawCampaignFactoryDisplay(); // 현재 장면의 procedure보다 오래 살아야 한다.
    bool Handles(o::Sid sid) const; // 실제 가상 표가 확인된 건물·간헐천·다이스인가.
    void BindOwnerColors(std::shared_ptr<OwnerColorTableState> owner); // 실제 Client의 같은 현재 표/배너 DWORD를 Draw와 Hit에 빌린다.
    bool Hit(o::Sid sid,int x,int y,const std::array<std::int32_t,2>& camera,std::uint32_t localPlayer,const std::function<void(o::Sid,int,int)>& bridgeDraw) const; // 같은현재+78/+a4와200×200 선택용 버퍼를 사용한다. 다리는Session의실제전역을공유한다.
    void SetPalette(std::span<const ScreenColor> palette,std::span<const std::uint32_t> colorSortLogical={}); //실제Client는같은고정초기RGB를별도전달하며기존독립검사는명시fallback이다. // 파일 팔레트의 원본 색 표를 갱신한다.
    // 실제 화면 영역을 월드 영역으로 바꿔 본체·그림자를 별도 정렬 항목으로 수집한다.
    std::vector<RenderSprite> Sprites(ScreenRect clip,int cameraX,int cameraY,std::uint32_t localPlayer,std::uint32_t hideBuildings=0); // 마지막인자는같은현재0059a8ac이며기본값은원본BSS초기0이다.
    void DrawEntry(const o::RenderCollectionEntry& entry,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,const std::array<std::int32_t,2>& camera,std::uint32_t localPlayer,std::uint32_t hideBuildings,const std::function<void(o::Sid,int,int)>& bridge); //종류0/지형 묶음의 실제 SID를 공통498220과 현재+78/+a4로 그린다. 타입별 별도 목록은 만들지 않는다.
private:
    struct Impl; // 실제 공유 참조/원본 출력 상태의 고정 주소 소유자다.
    std::unique_ptr<Impl> impl_; // callback이 읽는 수명 전체를 보관한다.
};
}
