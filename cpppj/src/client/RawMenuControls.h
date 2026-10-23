// 실제 Client Gump 트리에서 원본 메뉴·장식·제목·Slider와 StyleText를 함께 소유한다.
#pragma once
#include "client/Gump.h"
#include "client/MenuGumpControls.h"
#include "client/MenuGumpWidgets.h"
#include "client/MenuGumpInput.h"
#include "client/MenuGumpDraw.h"
#include "client/StoneButtonGump.h"
#include "client/QuickHelpControl.h"
#include "client/QuickHelpHtml.h"
#include "o/GumpFormHost.h"
#include <memory>

namespace netstorm::client {
class RawCampaignSession; // 현재 지식 창도 같은 세션의 실제 기술/제외/생산 상태를 빌린다.
// 같은 수집/정렬/클립 안에서 생산 패널의 원본 파생 Draw를 호출한다.
struct RawMenuExternalGumps {
    std::function<TextGumpNode*(std::uint32_t)> text; // 같은 TextGump/StyleText가 읽는 외부 텍스트 노드다.
    std::function<bool(std::uint32_t)> owns; // 공통 등록표의 현재 생산 노드인지 판정한다.
    std::function<void(std::uint32_t,GumpPixelTarget&,GumpGlyphColorState&,GumpBackgroundDraw&)> draw; // 현재 실제 픽셀/색/배경 문맥이다.
};
// 돌 버튼이 이미 사용하는 단일 트리의 실제 처리기다. 원본 상태/등록표 사본을 생성하지 않는다.
struct RawMenuTree {
    GumpCreateState& create; // 공통 등록표와 루트다.
    GumpLifetimeState& life; // 같은 overlay/idle/삭제 목록이다.
    const GumpCreate& factory; // 같은 노드를 등록하는 실제 기본 생성자다.
    const GumpCollection& collection; // 공통 위치/경계/수집이다.
    const GumpTree& tree; // 공통 부착/분리 처리기다.
    const GumpLifetime& lifetime; // 공통 자식 우선 수명이다.
    const GumpEvents& events; // 공통 재귀 통지다.
    const GumpInput& input; // 공통 focus/overlay 입력이다.
    const StoneButtonGump& stones; // Slider 화살표의 원본 돌 버튼 생성자다.
    std::function<GumpBackgroundNode*(std::uint32_t)> background; // 공통 돌 버튼이 소유한 배경을 조회한다.
    std::function<std::uint32_t()> token; // 공통 단조 증가 호스트 식별값을 확보한다.
    std::function<GumpCreateNode*(std::uint32_t)> node; // 모든 파생 종류의 동일 기본 노드다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 현재 파생 가상 부착이다.
    std::function<void()> changed; // 같은 Client의 표시 변경을 요청한다.
};
// 게임 명령/설정/음향/커서는 현재 Client가 공급한다. 메뉴 입력은 원본 토큰과 전체 DWORD를 보존한다.
struct RawMenuHooks {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> command; // command,등록ID,인자다.
    std::function<void(std::string_view,std::int32_t&)> readInt; // 현재 Menu 색 설정 조회다.
    std::function<void(std::string_view)> sound; // 원본 openGump/openSubGump 음향이다.
    std::function<InputEvent(std::uint32_t)> poll; // 실제 현재 마우스 폴링이다.
    std::function<void(std::uint32_t)> cursor; // 실제 커서 리소스 선택이다.
    std::function<o::GumpFormHost&()> objectForms; // 매Form효과에서현재Session의같은호스트를조회한다. 메뉴보다먼저Session을반납하지않는다.
    std::function<double()> gameSeconds; // 비용 표시의0055b4d0 현재 게임 시각이다. 벽시계와 구별한다.
};
// 전용 Help가 읽는 현재 Client 경계다. 창/파서/다중 줄/그림은 같은 실제 Gump 처리기를 사용한다.
struct RawHelpHooks {
    HtmlTextParserGlobals* globals{}; // Tell과 같은 원본 단락 억제 전역이다.
    HtmlTextParserHooks html; // 같은 Config 조건/명령/그림 효과이며 링크만 창의 소유 영역에 연결한다.
    std::function<std::string(std::string_view)> translate,config,substitute; // 현재 언어/파일 설정과 원본 바이트 치환이다.
    std::function<std::optional<std::string>(std::string_view)> load; // 현재 VFS 파일의 바이트와 실패를 구별한다.
    std::function<std::string(std::uint32_t)> text,typeName,description; // 실제 호스트 문자열/현재 TYPE 이름·설명이다.
    std::function<std::uint32_t(std::string_view)> storeText; // 입력 큐와 같은80칸 문자열 고리다.
    std::function<QuickHelpTypeInfo(std::uint32_t)> type; // 실제 Session/TYPE/AST 후처리의 현재 값이다.
    std::function<std::int32_t(std::uint32_t)> hitPoints,helpFrame; // 실제 최대HP/TYPE+120이다.
    std::function<std::int32_t(std::string_view)> lookup; // 실제 타입 이름 조회의 signed 결과다.
    std::function<std::uint32_t(std::int32_t,std::int32_t,std::int32_t)> nearestColor; // 현재 화면 논리 팔레트의 실제 RGB 검색이다.
    std::function<void(std::uint32_t,std::string_view,std::string_view)> queue; // Back의 현재 문자열 두 인자를 예약한다.
    std::function<void(std::string_view)> script,external; // 원본 링크의 cmd/외부 프로토콜 효과다. 미복원 외부 경계는 명시 거부한다.
    std::function<void(std::uint32_t)> learn; // 실제 기술 학습 효과다. 미연결을 성공으로 숨기지 않는다.
    std::function<bool()> scripted,battle,challenge; // 현재 스크립트 깊이/기본 주제 선택 모드다.
    std::function<std::string()> scriptName; // 현재 스크립트 복귀 이름이다.
};
// 가상 분배만 조립한다. 개별 생성/배치/입력/출력의 계산은 기존 원본 몸체를 그대로 사용한다.
class RawMenuControls {
public:
    // 공유 트리/자산/글꼴은 메뉴보다 오래 살아야 한다. 생성 중 게임/창을 실행하지 않는다.
    RawMenuControls(RawMenuTree tree,const GameAssets& assets,FontStore& fonts,RawMenuHooks hooks);
    // 공통 트리의 자식 삭제가 끝난 뒤 해제한다.
    ~RawMenuControls();
    RawMenuControls(const RawMenuControls&)=delete; // 고정 주소를 참조하는 훅을 복사하지 않는다.
    RawMenuControls& operator=(const RawMenuControls&)=delete; // 같은 저장소/처리기를 유지한다.
    void SetExternalGumps(RawMenuExternalGumps hooks);
    void SetExternalGumps(std::uint32_t slot,RawMenuExternalGumps hooks); // 독립 월드 UI가 같은 수집/정렬/클립을 공유하며 슬롯별로 수명을 반납한다.
    std::uint32_t BaseHandle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; // 외부 파생도 같은 기본 우클릭 Help로 위임한다. // 생산 패널의 현재 저장소를 같은 출력 계층에 연결/해제한다.
    const TextGump& TextBackend() const; // 수량/예산도 기존 원본 한 줄 글자 처리기를 공유한다.
    const StyleText& StyleBackend() const; // 생산 글자도 같은 실제 조각·글꼴·그림 파서를 공유한다.
    void MeasureText(std::span<const char> bytes,const std::array<std::uint32_t,8>& context,std::int32_t& width,std::int32_t& height,std::uint32_t limit) const; // 공통 돌 버튼 Fit도 같은 원본 글꼴 측정기를 사용한다.
    const GumpBackgroundCreate& BackgroundBackend() const; // 실제 TYPE/SHP를 읽는 같은 배경 생성기다.
    QuickHelpPopupState& PopupState() const; // 지식 카드 표식과 전용 Help Back이 공유할 같은 실제 저장소다.
    void EnableHelp(RawHelpHooks hooks); // 현재 Client 경계를 같은 공통 트리에 한 번 연결한다.
    std::uint32_t OpenHelp(std::uint32_t topic=0,std::int32_t mode=1,std::uint32_t learnType=0); // 실제 전용 창 재사용/모달/요청이다.
    // 원본 cf570의 미션 시작 안내를 같은 등록표/장식/다중 줄/즉시 돌 버튼에 작성한다.
    // 현재 복원 범위는 기본 Starting Mission/Cancel 경로이며 일반 Tell 전체 부모와 구별한다.
    std::uint32_t ShowMissionLoading(std::string_view message,std::string_view cancel,ScreenRect mapView);
    void CloseMissionLoading(); // 실제 transient serial을 찾아 Kill하며 프레임 Cleanup까지 저장소를 유지한다.
    std::optional<ScreenRect> MissionLoadingBounds() const; // 살아 있는 원본 부모의 현재 화면 경계다.
    bool HelpOpen() const; // 현재 serial/등록/visible/dead를 조회한다.
    void CloseHelp(); // Session 해제 전에 같은 창과 지연 삭제 큐를 정리한다.
    // 공통 기본 생성/가상 분배에서 현재 파생 노드를 직접 조회한다. 없는 토큰은 nullptr다.
    GumpCreateNode* Node(std::uint32_t token) const;
    StoneButtonGumpNode* Button(std::uint32_t token) const;
    GumpBackgroundNode* Background(std::uint32_t token) const;
    // 공통 트리가 호출하는 실제 파생 가상 동작이다. 기본 노드에는 위임하지 않는다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    void Idle(std::uint32_t token) const;
    void Redraw(std::uint32_t token,bool erase) const;
    void DeleteScalar(std::uint32_t token,std::uint32_t flags) const;
    void Release(std::uint32_t token);
    // 이전 sticky 메뉴를 닫고 실제 controlled 생성/작성/부착을 실행한다. builder는 루트 메뉴를 한 칸 남겨야 한다.
    void Open(ScreenPoint point,const std::function<void(const MenuGumpWidgets&,const MenuGumpControls&,const MenuGump&)>& builder);
    // builder가원본객체부착까지마치고루트메뉴토큰(거부시0)을반환한다. controlled스택은0칸이어야한다.
    // 원본Open/위치의계산을반복하거나마우스점에다시부착하지않는다.
    void OpenAttached(const std::function<std::uint32_t(const MenuGumpWidgets&,const MenuGumpControls&,const MenuGump&)>& builder,bool closeExisting=true);
    // 실제 루트 직계 sticky 팝업을 Kill하고 같은 삭제 큐를 정리한다.
    void Close();
    // 원본00466710처럼 현재 루트 팝업을 Kill만 한다. callback 반환 전에는 노드/등록을 해제하지 않는다.
    void RequestClosePopups();
    // 실제 메뉴 생성 전에 현재 파일 팔레트의 색 전역을 공급한다.
    void SetPalette(const PaletteColorTable& palette);
    // 현재 Client 픽셀에 원본 수집/정렬/Draw를 실행한다. 배경/그림/글자 모두 실제8비트 클립을 공유한다.
    void Draw(std::span<std::uint8_t> pixels,int pitch,int height,const PaletteColorTable& palette,const GumpShadeMaps& maps,std::optional<ScreenRect> clip={});
    // 검사/자동 입력은 실제 저장 항목과 배치를 읽는다. label은 스타일 명령을 포함한 원문 바이트다.
    std::optional<ScreenPoint> Point(std::string_view label) const;
    std::vector<GumpControl> Controls() const;
    bool HasMenus() const;
    std::uint32_t MenuInstances() const; //00593024의 같은 현재 MenuGump 인스턴스 전체DWORD다.
    // 같은 controlled/Gump/StyleText에 실제 전체 지식 카드를 작성하고 원본 네 줄에 부착한다.
    bool OpenKnowledge(RawCampaignSession& session,std::function<std::string(std::string_view)> translate,std::uint32_t language);
    // 저장한 카드 토큰의 현재 등록/visible/dead를 직접 읽는다. 별도 열림 bool을 두지 않는다.
    bool KnowledgeOpen() const;
    // 실제 비용 표시를 같은 루트/StyleText/idle/수명에 생성한다. 원본 커서의 reference는0이다.
    std::uint32_t ShowSp(std::int32_t cost,std::int32_t x,std::int32_t y,std::uint32_t reference=0);
    // 메뉴 또는 비용 표시 중 현재 실제 표시 노드가 있는지 원본 visible/dead로 읽는다.
    bool HasOverlay() const;
    // 원본004662b0의 같은 지연 삭제 큐를 정리한다. Client는 비정지 프레임의 Idle 전에 호출한다.
    void CleanupFrame();
    // 모든 부모가 visible인 실제 메뉴/자식 안인지 읽는다. 하위 메뉴를 루트 사각형으로 자르지 않는다.
    bool Contains(ScreenPoint point) const;
    std::string Report() const;
private:
    struct Impl; // 안정된 파생 노드/StyleText/버퍼와 현재 Draw 문맥의 소유자다.
    std::unique_ptr<Impl> impl_; // 공통 트리와 Client보다 먼저 해제한다.
};
}
