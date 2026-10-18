// 같은 캠페인 풀의 실제 Factory/아웃포스트를 원본 수집/일반 Draw에 연결한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "client/Renderer.h"
namespace netstorm::client {
class GameAssets; // 현재 TYPE/SHP/파일 팔레트를 소유하는 실제 자산 저장소다.
// 실제 Factory/아웃포스트와 플레어의 세 가상 표만 같은 원본 Draw에 분배한다.
class RawCampaignFactoryDisplay {
public:
    RawCampaignFactoryDisplay(RawWorldCore core,const GameAssets& assets); // 같은 현재 TYPE/SHP/풀을 빌린다.
    ~RawCampaignFactoryDisplay(); // 현재 장면의 procedure보다 오래 살아야 한다.
    bool Handles(o::Sid sid) const; // 실제 가상 표가 원본 Factory/아웃포스트/플레어인가.
    void SetPalette(std::span<const ScreenColor> palette); // 파일 팔레트의 원본 색 표를 갱신한다.
    // 실제 화면 영역을 월드 영역으로 바꿔 본체·그림자를 별도 정렬 항목으로 수집한다.
    std::vector<RenderSprite> Sprites(ScreenRect clip,int cameraX,int cameraY,std::uint32_t localPlayer);
private:
    struct Impl; // 실제 공유 참조/원본 출력 상태의 고정 주소 소유자다.
    std::unique_ptr<Impl> impl_; // callback이 읽는 수명 전체를 보관한다.
};
}
