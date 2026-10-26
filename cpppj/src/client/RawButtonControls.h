// UberGump의 돌 버튼 입력을 복원된 Gump 트리/수명/캡처/프레임 처리기로 연결한다.
#pragma once
#include "client/Gump.h"
#include "client/GumpVisual.h"
#include "client/GumpBackground.h"
#include "o/RiftType.h"
#include "o/GumpFormHost.h"
#include <functional>
#include <memory>
#include <string>

namespace netstorm::client {
class RawCampaignSession; // 생산 패널과 같은 현재 실제 월드다.
struct RawProductionHooks; // 현재 생산 UI 장치 효과다.
struct ProductionControlContext; // 초기 HUD의 실제 물리/현재 전역/Player/옵션이다.
struct RawNsInfoContext; // NSInfo의 실제 장기 전역/문자열을 빌린다.
class RawMinimapControls; // 같은 월드의 실제 Sky Overview 처리기다.
class RawSurfaceStorage; // Session보다 오래 보관하는 실제 Pool/Hash/TYPE 물리다.
struct RawMinimapHooks; // 현재 카메라/입력 장치다.
class RawProductionControls; // 같은 등록표의 실제 생산 파생 처리기다.
class RawMenuControls; // 같은 트리에 연결하는 실제 메뉴 처리기다.
struct RawMenuHooks; // 현재 Client의 메뉴 외부 효과다.
class GameAssets; // Client가 소유하는 실제 TYPE/SHP 저장소다.
class FontStore; // Client가 소유하는 실제 글꼴 저장소다.
class WarningMessages; // 같은 등록표/삭제 큐의 실제 Warning/Pause 파생 처리기다.
class RawUberRootControls;struct RawUberRootContext; // 같은 공통 등록표의 실제 전체 루트와 도킹 문맥이다.
struct MessageGumpHooks; // 현재 모드/이름/번역의 Pause 생성 경계다.
// 실제 fortGump A00 헤더와 현재 팔레트다. 픽셀 출력과 raw 배경 생성은 같은 자산을 사용한다.
struct RawButtonAssets {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    // frame: 타입에서 사용할 그림 프레임 번호.
    std::uint32_t type{},frame{};
    // width: 가로 크기.
    // height: 세로 크기.
    std::int16_t width{},height{};
    // palette: 그림의 색 인덱스를 실제 출력 색으로 바꿀 팔레트.
    ButtonPalette palette;
};
// 벽 시계/Win32 폴링/효과음은 실제 Client가 공급한다. 비어 있으면 독립 콘솔 관찰 상태를 사용한다.
struct RawButtonHooks {
    // now: 현재 시각을 초 단위로 공급하는 콜백.
    std::function<double()> now;
    // poll: 요청한 키·마우스 버튼의 현재 입력을 폴링하는 콜백.
    std::function<InputEvent(std::uint32_t)> poll;
    // sound: 해당 동작의 소리를 재생하는 콜백.
    std::function<void()> sound;
    std::function<void(std::uint32_t,std::uint32_t)> deleteForm; // 현재Session의UI종속Form을같은공통삭제로반납한다.
};
// 같은 Gump Route의 소비 토큰과 부모 click 명령을 구별한다.
struct RawButtonInputResult {
    std::uint32_t consumed{}; // 실제00466200의 성공 Gump 토큰이며 단순 화면 hit와 다르다.
    std::optional<int> activated; // 현재 루트의 돌 버튼 부모 click이 확정한 화면 명령이다.
};
// 입력 폴링 통지의 raw Overlay 반환과 부모 click 활성화 및 표시 변경을 각각 보존한다.
struct RawButtonPointerResult {
    std::optional<std::uint32_t> overlay; // 미호출은 빈 값이며 실제 반환0과 구별한다. 성공 값은 Gump 토큰으로 바꾸지 않는다.
    std::optional<int> activated; // 현재 통지 콜백이 실제 부모 click으로 확정한 화면 명령이다.
    bool changed{}; // 이 단계가 소비한 현재 표시 변경 요청이다.
};
// 안정된 파생 저장소와 실제 원본 UI 처리기를 소유한다. 메뉴 행은 기존 메뉴 어댑터가 처리한다.
class RawButtonControls {
public:
    // 원본 판본별 가상 표/등록 계약을 선택하며 선택적 Client 갱신 DWORD는 모든 자식보다 오래 살아야 한다. 생성 중 장치나 창을 열지 않는다.
    explicit RawButtonControls(o::OriginalEdition edition,RawButtonHooks hooks={},std::uint32_t* sharedFullRefresh=nullptr);
    // 참조하는 처리기를 먼저 해제하고 마지막에 노드 저장소를 해제한다.
    ~RawButtonControls();
    RawButtonControls(const RawButtonControls&)=delete;
    RawButtonControls& operator=(const RawButtonControls&)=delete;
    // 기본은 기존 자식을 Kill/Cleanup한다. preserveWorldUi는 돌 버튼만 교체한다. deferCleanup은 보존 모드에서 실제 HTML의 후반 소멸을 유지한다.
    void SetControls(std::span<const GumpControl> controls,std::span<const std::string> labels,
        int width,int height,RawButtonAssets assets,bool preserveWorldUi=false,bool deferCleanup=false,std::optional<std::int32_t> familyDepth={}); //선택 깊이는 실제 메인 container 자식의 깊이이며 AddDepth로 배경의 상대+1을 보존한다.
    // 실제 GumpInput::Route와 부모 click 통지로 활성화된 화면 명령 번호를 반환한다.
    std::optional<int> Event(InputEvent event);
    RawButtonInputResult RouteInput(InputEvent event); // 같은 실제 Route를 한 번 실행해 월드 전 입력 소비를 돌려준다.
    std::uint32_t RootToken() const; // 현재 등록표0칸의 실제 공통 루트다.
    std::uint32_t SerialGump(std::uint32_t serial) const; //00464020: 등록1..999에서 같은serial·비dead의 첫 대상이다.
    void KillStateGumps(); //004666d0의 등록1..999·현재 제외타입11/dead 관문으로 Kill만 예약한다.
    void RefreshStormPower(); //0043dad0의 현재 예산 serial 조회/가상 Push/Attach/깊이 복원을 실행한다.
    void KillGump(std::uint32_t token); // 같은 실제 수명으로 Kill만 하고 callback 밖 프레임 Cleanup을 유지한다.
    std::uint32_t DialogCount() const; // Guide가 공통 입력/수명과 같은 현재 모달 개수를 읽는다. Kill의 즉시 반납과 소멸 전 창 인스턴스 수를 구별한다.
    std::uint32_t CurrentDialog() const; // 원문565dd4의 실제 현재 모달 토큰이다.4c8e90은 개수가 아닌 이 값의0 여부를 읽는다.

    // 004d68c2/004d69f4: 안쪽 포인터의 조건부 Overlay 뒤 raw code/x/y Broadcast만 실행한다. 일반 큐 드레인보다 앞에서 한 번 부른다.
    // cursorActive는445330의 참/거짓이고 mode는005cae90이다. allowOverlay는 아직 공통 부모가 아닌 논리 HTML의 명시 호스트 차단 경계다.
    RawButtonPointerResult NotifyPointer(InputEvent event,std::uint32_t mode,bool cursorActive,bool allowOverlay=true);
    // 00439b13: 같은 공통 kill 큐의 실제 소멸만 실행하며 GameClock 정지를 검사하지 않는다.
    bool CleanupGumps();
    // 00439b18/00439b69: 같은 현재 idle 표를 한 번 순회한다. Broadcast/정리/활성화 초기화를 섞지 않는다.
    bool IdleGumps();
    // 기존 독립 검사 호환용 Broadcast(0)/밖 release/Idle 결합이다. 실제 Client는 위 세 단계를 각각 사용한다.
    bool Poll(InputEvent event);
    // 화면 전환/포커스 상실 때 실제 캡처 처리기에 화면 밖 release를 전달한다.
    void Cancel();
    void RetirePageControls(); // 현재 HTML 돌 버튼만 즉시 detach/Kill하고 실제 소멸은 후반 Cleanup에 남긴다. 월드 HUD는 보존한다.
    // 0047d890/CD004773a0: 현재 캡처/dialog/루트에 생산 갱신 사건을 원본 Route 순서로 분배한다.
    // 생산 패널 노드가 아직 없으면 실제 트리의 미처리 반환을 유지한다.
    void RefreshProduction();
    std::uint32_t NotifyHudMode(std::uint32_t command,std::uint32_t mode); //00466200/280의 현재 캡처·modal·root에 명시 원문 모드를 숨김 포함1로 전달한다.
    std::uint32_t NotifyHudMode(std::uint32_t command); //현재 캡처/dialog/root에(command,1,0,includeHidden1)을 보내며 반환토큰과 캡처해제 효과를 보존한다.
    // 같은 트리/입력/수명에 실제 원본 메뉴 계층을 한 번 연결한다. 자산/글꼴은 Client가 소유한다.
    RawMenuControls& EnableMenus(const GameAssets& assets,FontStore& fonts,RawMenuHooks hooks);
    // 연결된 실제 메뉴를 빌린다. 연결 전에는 nullptr다.
    RawMenuControls* Menus() const;
    RawUberRootControls& EnableUberRoot(RawUberRootContext context); // 실제 전체 루트/구분선의 가상 분배를 한 번 설치한다.
    RawUberRootControls* UberRoot() const; // 같은 실제 루트 처리기를 빌린다.
    ScreenRect& MapViewRect(); // GumpTree 정렬과 실제 dock가 같은 원문 정수 지도LTRB를 직접 공유한다.
    std::uint32_t BubbleGump(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second); // 원문00464e90의 현재 부모 체인에만 전달한다.
    std::uint32_t ConstructNsInfo(RawNsInfoContext context); // 같은 공통 Button/배경 전역으로 전체 NSInfo 생성만 한다.
    // 실제 공통 Gump/StyleText를 빌리는 안내 처리기를 한 번 설치한다. 별도 루트/등록표를 만들지 않는다.
    WarningMessages& EnableWarnings(std::function<double()> now,std::function<void()> sound,MessageGumpHooks pauseContext);
    WarningMessages* Warnings() const; // 같은 공통 가상 분배가 소유하는 현재 안내 처리기다.
    RawProductionControls& EnableProduction(RawCampaignSession& session,const GameAssets& assets,RawProductionHooks hooks); // 현재 미션 패널을 조립한다.
    RawProductionControls& ConstructProduction(ProductionControlContext context,const GameAssets& assets,RawProductionHooks hooks); // Session 이전에도 같은 원문 생성만 수행하며 루트 부착은 실제 부모가 담당한다.
    RawProductionControls* Production() const; // 현재 살아 있는 패널 처리기다.
    void ConfigureTestBridgePatterns(std::string_view patterns); // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 초기/진행 중 지정 순서를 교체한다.
    RawMinimapControls& EnableMinimap(RawCampaignSession& session,RawMinimapHooks hooks); // 같은 실제 트리/월드에 미니맵을 생성한다.
    RawMinimapControls& EnableMinimap(std::shared_ptr<RawSurfaceStorage> storage,RawMinimapHooks hooks); // Session 이전에도 같은 실제 장기 물리에 미니맵을 설치한다.
    RawMinimapControls& ConstructMinimap(std::shared_ptr<RawSurfaceStorage> storage,RawMinimapHooks hooks); // 실제공장만호출하며이전표시가살아있어도새부모0노드를만든다.부착/도킹은원문Uber가담당한다.
    RawMinimapControls* Minimap() const; // 살아 있는 실제 미니맵 처리기다.
    void DisableMinimap(); // 현재Session보다먼저통지/입력/표시참조를반납한다.
    void DisableProduction(); // Session 해제 전에 연결 노드/참조를 반납한다.
    // 실제현재등록표→가상+4의기본/메뉴Kill알림이다. 반환콜백은이UI보다먼저해제한다.
    o::GumpFormNotice ObjectNotice() const;
    // Player 내용물 통지는 현재 등록ID를 직접 조회하며 생산 갱신 사건으로 바꾸지 않는다.
    std::uint32_t RegisteredGump(std::uint32_t id) const;
    // 등록된 대상 자신의 가상+28만 실행한다. 전체 트리 Route/재귀 분배를 수행하지 않는다.
    std::uint32_t HandleGump(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second);
    // 00465ed0: 루트 직계 자식만 현재 화면 경계/visible/dead/signed 깊이로 고른다. 월드 영역은0이다.
    std::uint32_t PickGump(ScreenPoint point) const;
    // 004651b0: 자식 우선의 필터/가상 처리를 실행하고 성공한 Gump 토큰을 반환한다.
    // 직접 가상 반환이 필요한 커서 허용 플래그/수신은 HandleGump를 사용한다.
    std::uint32_t DispatchGump(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second,bool includeHidden=false) const;
    // 원본 +0x24 parentSid를 현재 조회한다. UI 부모 토큰(+0x14) 및 Form 포인터(+0x28)와 구별한다.
    std::uint32_t GumpParentSid(std::uint32_t token) const;
    // 원본 +0x30의 현재 전체 DWORD 깊이를 읽는다. signed 해석은 원본 소비자가 결정한다.
    std::uint32_t GumpDepth(std::uint32_t token) const;
    // 00466710의 루트 직계 팝업 Kill만 요청한다. 현재 callback이 끝나기 전에 Cleanup하지 않는다.
    void RequestClosePopups();
    // 004641d0: 현재 루트 직계 자식의4000h 플래그만 검사한다. focus/overlay 여부와 합치지 않는다.
    bool BlockingGumpOpen() const;
    // 현재 raw value/pressed 중 하나라도 비영인 첫 돌 버튼 번호다.
    std::optional<int> Pressed() const;
    // 커서가 월드로 이동해도 release를 같은 raw 캡처에 먼저 전달할지 판정한다.
    bool HasCapture() const;
    // 파일 팔레트의 현재 전역 색을 갱신한다. 이미 생성된 노드의 문맥 복사는 원본처럼 유지한다.
    void SetPalette(ButtonPalette palette);
    // 실제 생성자의 63바이트 복사 결과를 표시/측정에 공급한다. UI의 긴 원문 사본을 다시 그리지 않는다.
    std::string Label(int id) const;
    // 표시와 입력이 같은 raw 경계/두 상태/비활성 플래그를 사용하는 출력 계획이다.
    ButtonDrawPlan DrawPlan(int id,std::int32_t textWidth,std::int32_t textHeight) const;
    // 생성된 실제 자식 배경의 경계/타일 크기/눌림 플래그로 출력 계획을 만든다.
    std::vector<GumpTexturePass> BackgroundPlan(int id,ScreenRect clip) const;
    // 등록 수·serial·focus·idle·노드 수를 자동 검사에서 읽는다.
    std::string Report() const;
private:
    struct Impl;
    // impl_: 공개 인터페이스 뒤의 구현 상태와 연결 객체.
    std::unique_ptr<Impl> impl_;
};
}
