// 같은 실제 월드/공통 Gump에 원본 Sky Overview를 연결한다.
#pragma once
#include "client/RawMenuControls.h"
#include "client/MinimapGeometry.h"
#include "o/RawMetadisplayDirty.h"
#include "o/RawMinimapRaster.h"

namespace netstorm::client {
class RawSurfaceStorage;struct RawWorldCore; // 장기 물리와 짧은 실제 Pop/Unpop 바인딩을 구별한다.
// 입력과 카메라는 현재 GameWorld가 소유하며 검사 전용 상태를 공급하지 않는다.
struct RawMinimapHooks {
    std::function<MinimapGeometryView()> view; // 현재 같은 카메라와 실제 지도 viewport다.
    std::function<void(float,float)> center,move; //497f20 카메라 중심과42a360/4d5bb0 선택 이동이다.
    std::function<std::uint32_t()> inputMode; // 실제 현재 모드2/5를 매 입력에서 읽는다.
    std::function<InputEvent()> poll; // 현재 원본ffff 마우스 폴링 사건이다.
};
// 원문의현재표시포인터와생성serial은개별노드보다오래살며이전노드소멸이새값을지우지않는다.
struct RawMinimapGlobals {
    std::uint32_t current{},serial{}; //005949d0/005949dc의최초BSS0이며호스트의같은공통노드토큰을보관한다.
    o::MinimapRasterState colors; // 팔레트/표면색 전역도 개별표시와 구별해 같은 모든 셀 그림에서 공유한다.
};
// 실제 minmapGump 한 개와 공유 bitmap/dirty의 연결 수명을 소유한다.
class RawMinimapControls {
public:
    // 공통 트리/메뉴/Session은 이 처리기보다 오래 살아야 한다. 창이나 게임은 생성하지 않는다.
    RawMinimapControls(RawMenuTree tree,RawMenuControls& menus,RawCampaignSession& session,o::MetadisplayDirtyState& state,RawMinimapGlobals& globals,RawMinimapHooks hooks);
    RawMinimapControls(RawMenuTree tree,RawMenuControls& menus,std::shared_ptr<RawSurfaceStorage> storage,o::MetadisplayDirtyState& state,RawMinimapGlobals& globals,RawMinimapHooks hooks); // Session 이전에도 같은 실제 물리/현재전역으로 전체 미니맵을 구성한다.
    void BindWorld(RawWorldCore core); // 같은 물리의 현재 Pop/Unpop 통지만 연결한다. 중복 연결은 거부한다.
    void UnbindWorld(); // 현재 월드 모듈 반납 전에 통지를 끊고 실제 물리/미니맵은 유지한다.
    // 현재 Pop/Unpop 통지와 외부 Draw를 끊고 같은 수명 처리기로 반납한다.
    ~RawMinimapControls();
    std::uint32_t Construct(); //004815b0→4801b0의실제현재bitmap폭/부모0생성→serial/현재표시갱신이다.도킹과bitmap생성을하지않는다.
    std::uint32_t RootToken() const; // 같은 실제 미니맵 토큰이며 새/이전 처리기를 구별한다.
    void Open(); //004815b0/4801b0의 실제 생성이며 기본 도킹 배치는 Resize가 맡는다.
    void Close(); // Session보다 먼저 같은 노드/캡처/Idle을 반납한다.
    void Resize(int screenHeight); // 정상HUD mode3의 divider 결과(0,H-75),80×75 crop이다.
    void SetPalette(const PaletteColorTable& palette); // 현재 파일 팔레트의 실제 미니맵 전용 색 전역이다.
    bool Refresh(); //00481320을 실제 Client의 커널 앞 프레임 위치에서 실행한다.
    GumpCreateNode* Node(std::uint32_t token) const; // 공통 트리와 같은 안정된 기본 주소다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode); //004805e0 뒤 기본 부착이다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second); //00481620의 클릭/캡처/이동/기본 Help다.
    void Idle(std::uint32_t token); //00480440의 실제 폴링/로컬 변환/카메라 이동이다.
    void DeleteScalar(std::uint32_t token,std::uint32_t flags); // 원본은 기본 소멸자이며 bitmap은 독립 수명이다.
    void Release(std::uint32_t token); // 실제 scalar 해제가 끝난 뒤만 주소를 무효화한다.
    std::optional<ScreenPoint> Point(std::string_view label) const; // 실제 경계에 상대적인 자동 마우스 입력점이다.
    std::string Report() const; // 현재 등록/경계/crop/큐/bitmap을 읽기 전용으로 보고한다.
private:
    struct Impl; // 같은 Pool과 Gump 저장소를 참조하는 안정된 구현이다.
    std::unique_ptr<Impl> impl_; // 월드/공통 메뉴보다 먼저 반납한다.
};
}
