// 현재 복원한 저장 미션·지면·Squid 필드를 클라이언트에 연결하는 새 월드 어댑터.
// 원본 Mission/CoreData/타입별 생성자 전체의 복원과는 구분한다.
#pragma once
#include "client/GameAssets.h"
#include "client/Renderer.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawCampaignSession.h"
#include "client/Sound.h"
#include "client/RawUserHighlight.h"
#include "client/ClientGameplayGlobals.h"
#include "client/MinimapGeometry.h"
#include "o/Player.h"
#include "o/Squid.h"
#include "o/Islandbuilder.h"
#include <map>

namespace netstorm::client {
class RawCampaignFactoryDisplay; // 같은 실제 풀의 원본 Factory 화면 출력기다.
// 미션 Session과 표시/입력을 소유하는 호스트 어댑터다. 원문 Kernel에는 실제 게임 프로세스만 등록한다.
class GameWorld {
public:
    // 저장 위치를 쓰는 Tutorial 미션의 초기 월드를 만든다. Battle 재배치는 아직 지원하지 않는다.
    GameWorld(const GameAssets& assets,const o::FortTemplate& fort,o::MissionPlayers players);
    // 실제 캠페인 세션을 소유하며 모든 그림/선택/이동이 같은 raw SID를 사용한다. viewport는 로더 실행 전 실제 화면 크기다.
    GameWorld(const GameAssets& assets,const o::BaseFileSystem& files,o::FortTemplate fort,o::MissionPlayers players,
        std::string mission,RawCampaignSessionServices services,ScreenPoint viewport,bool deferConnection=false);
    //실제Client가현재월드포인터를등록한뒤로그인한다. Chat의자기subscriber 조회도같은Session을보아야한다.
    void BeginLocalConnection();
    // 콜백이 읽는 선택/그림 저장소의 수명이 끝나기 전에 실제 캠페인 프로세스를 정리한다.
    ~GameWorld();
    // 프레임 준비에 사용할 실제 게임 시간과 정지 상태의 공급자를 연결한다.
    std::function<double()> elapsed;
    // paused: 현재 게임 시계가 정지했는지 조회하는 콜백.
    std::function<bool()> paused;
    // 클라이언트 GameClock이 고정한 절대 게임 시각/프레임 번호를 raw Kernel과 공유한다.
    std::function<o::FrameTime()> frameTime;
    // surfaceDisplay: 해상도 변경으로 Renderer가 교체되어도 현재 장치에 전달할 수 있는 표시 콜백이다 (SurfaceDisplayHooks).
    SurfaceDisplayHooks surfaceDisplay;
    // 현재 카메라와 실제 월드 표시 영역을 위치 효과음에 전달한다. Resize·Scroll 및 raw 프레임 처리 전에 호출한다.
    // 수신 객체는 월드보다 오래 살아야 한다. 비어 있으면 소리 없이 같은 월드 경로를 사용한다.
    std::function<void(SoundView)> soundViewChanged;
    // 독립 호출자는 준비/Kernel 한 번/표시 수집을 함께 진행한다.
    void RunFrame();
    void PrepareKernelFrame(); // 실제 Client의 Kernel 앞에서 현재 같은 시각/연결/AI/그림 입력만 준비한다.
    void FinishKernelFrame(); // 같은 전역 Kernel 실행 뒤 변경된 실제 SID를 표시 자료로 수집한다.
    // 실제 미션 게이트의 checkOutcome 위치에서 동일 캠페인의 현재 Player/사제/영역을 판정한다.
    void CheckMissionOutcome();
    // 검사에서는 OS 시계 없이 같은 이동 경로를 진행한다.
    void Step(double seconds);
    // 화면 크기를 갱신한다. 실제 캠페인의 최초 시야는 생성자의 원본 로더가 설정하며 이전 어댑터는 최초 Resize에서 홈을 고른다.
    void Resize(int width,int height);
    // 표시 팔레트를 바꾸고 팔레트에 의존하는 지면 명도 변환표를 다시 계산하게 한다. 화면 팔레트 적용 직후 부른다.
    void SetPalette(std::span<const ScreenColor> palette,std::span<const std::uint32_t> colorSortLogical={}); //실제Client는같은고정초기RGB를별도전달하며기존독립검사는명시fallback이다.
    void BindHideBuildings(std::function<std::uint32_t()> current); // 실제0059a8ac를매표시/선택시현재조회한다.호출대상은월드보다오래살아야한다.
    void BindMapView(std::function<ScreenRect()> current); // 같은 실제 HUD의 현재 지도LTRB를 표시/미니맵/선택/음향에서 빌린다.
    ScreenRect MapView() const; // 실제 설치된 dock가 없으면 기존 독립 호출자의 초기 영역을 유지한다.
    // 카메라를 월드 픽셀 범위 안에서 옮긴다.
    void Scroll(int dx,int dy);
    MinimapGeometryView MinimapView() const; // 같은 실제 카메라와 현재 월드 viewport를 미니맵에 빌린다.
    void CenterCamera(float x,float y); // 미니맵도 초기 시야와 같은497f20/변경 전역을 사용한다.
    void MoveFromMinimap(float x,float y); // 실제 선택Carrier에월드좌표그대로명령한뒤강조0을적용한다.
    // F4/H·F5의 원본 홈 신전/사제 보기를 연결한다.
    void Home(bool priest);
    // 실제 캠페인은433ba0의 명시 SID/Carrier preview를 같은 Session과 현재 화면 원점에 연결한다.
    void CenterOnObject(std::uint32_t sid);
    // Players 메뉴의 명시 owner DWORD로4950d0을 실행한다. Home(true)의 로컬 소유자로 바꾸지 않는다.
    void ViewPriest(std::uint32_t owner);
    // 화면 좌표와 실제 렌더링 그림으로 선택 가능한 오브젝트를 찾는다.
    o::SquidId Pick(ScreenPoint point) const;
    // 선택 번호를 현재 월드에서 검증하고 표시를 갱신한다.
    void Select(o::SquidId id);
    o::SquidId Selected() const { return selected_; } // 004d5430의 현재 단일SID 조회이며 선택을 바꾸거나 캐시하지 않는다.
    ClientUserSelectionGlobals& SelectionGlobals();const ClientUserSelectionGlobals& SelectionGlobals() const; // 같은 장기 raw DWORD/배열이며 실제 SID 변환은 소비자가 검증한다.
    void ResetSelection(); // 미션이 요구한 실제004d5c90(0,0,0)을 셋째1인 일반 Select와 구별한다.
    void ClearSelection(); // 후속004d5c60의 별도 enabled/다중 선택/강조 정리다.
    // 강조는 같은 현재 선택을 읽기만 한다. 마우스 모드/TYPE/좌표를 별도 원본 전역으로 빌린다.
    UserHighlightState HighlightSettings();
    // 실제 선택/강조/운반체 명령을 같은 현재 저장소와 Client 장치에 연결한다.
    void BindUserInput(UserSelectionHooks selection,UserHighlightHooks highlight,CarrierInputHooks carrier);
    void Highlight(std::uint32_t mode); // 건설 커서와 선택이 같은 원본 강조 몸체를 쓴다.
    // 원본 파생+ac가 같은 선택 상태를 먼저 실행한 뒤 공통 메뉴에 동일 사건 참조를 넘긴다.
    std::uint32_t OpenSelectionMenu(o::Sid sid,const InputEvent& event,bool archer,const std::function<std::uint32_t(o::Sid,const InputEvent&)>& common);
    void UpdateMovementCursor(); // 현재 폴링 점으로 이동 커서를 다시 평가한다.
    std::uint32_t InputMode() const { return highlightMode_; } // 선택과 다른 현재 원본 강조 모드를 읽는다.

    // 입력 폴링의 현재 화면 좌표를 강조 역투영과 공유한다.
    void SetMousePoint(ScreenPoint point);
    // 선택한 내 이동 유닛에 땅 좌클릭 명령을 내린다. 실패해도 선택은 해제한다.
    bool MoveSelected(ScreenPoint point);
    // 현재 스프라이트·지형의 깊이 순 목록을 수집한다. 정적 InspectView를 거치지 않는다.
    std::vector<RenderSprite> Sprites() const;
    // 선택 괄호와 체력 막대를 원본 팔레트 색으로 합성한다.
    std::shared_ptr<IndexedImage> SelectionImage() const;
    // 입력/검사에서 쓰는 동일 화면 좌표 변환.
    ScreenPoint ToScreen(double x,double y) const;
    o::CellPoint ToCell(ScreenPoint point) const;
    // 자동 검사의 실제 그림/칸 좌표. 명령 실행에는 사용하지 않는다.
    std::optional<ScreenPoint> Point(std::string_view label) const;
    // 미션과 객체 수명 안에서만 유효한 런타임 번호를 조회한다.
    const o::Squid* Object(o::SquidId id) const;
    // 모드 변경·선택·이동·카메라 변경이 표시를 요구했는지 읽고 지운다.
    bool TakeChanged();
    // 원본 파일 없이도 스모크가 초기 상태/이동 결과를 관찰하도록 TSV를 만든다.
    std::string Report(bool mask=false) const;
    // 현재 미션의 시작 자원·기술·동맹 상태를 읽기 전용으로 조회한다.
    const o::MissionPlayers& Players() const;
    // 현재 실제 캠페인 Session의 globals.mySubscriber다. 이전 값형 어댑터에는 로그인 전역이 없어 0이다.
    std::uint32_t CurrentSubscriber() const;
    // 검사/후속 게임 명령이 사용하는 실제 raw 표면 월드다. 표시도 같은 슬롯을 읽는다.
    RawSurfaceWorld& Surfaces();
    // Client 명령/메뉴는 표시와 같은 실제 Session을 빌린다. 이전 값형 어댑터에서의 혼용은 거부한다.
    RawCampaignSession& Session();
    // 원본 미션 Start와 로더가 같은 현재 카메라 선택/표시 갱신을 사용한다.
    void ResetCampaignCamera();
private:
    // 현재 실제 슬롯에서 읽기 전용 UI 뷰만 갱신하며 별도 게임 상태를 진행하지 않는다.
    void RefreshCampaignObjects();
    // 월드 지형 타일의 타입·프레임·좌표·소유자·깊이이다.
    struct Tile { /* type: 객체 종류를 구별하는 타입 자료. */ const TypeAsset* type{}; /* frame: 타입에서 사용할 그림 프레임 번호. */ std::size_t frame{}; /* x: X 좌표. */ /* y: Y 좌표. */ /* owner: 객체나 지역을 소유한 플레이어 번호. */ int x{},y{},owner{}; /* depth: 출력이나 이벤트 후보의 깊이 값. */ std::int16_t depth{}; };
    // 청크 순서의 저장 객체를 월드 좌표에 생성한다.
    void Place(const o::FortChunkSection& section,std::span<const o::ChunkCoordinate> chunks,int territory);
    // 지면·받침·절벽 프레임과 정지 점유 격자를 만든다.
    void BuildTerrain();
    // 지형/저장 입력을 실제 표면 SID로 등록한다. 비표면 유닛 이동은 기존 어댑터에 남긴다.
    void BuildSurfaces();
    // 기존 비표면 유닛의 관찰 기반 이동을 진행한다.
    void AdvanceObjects(double seconds);
    // 현재 카메라/장치 변경을 raw 표시 경계에 전달한다.
    void SurfaceView();
    // 임시 보행 격자의 표면/다리 상태도 현재 raw 슬롯에서 갱신한다.
    void RefreshSurfaceGround();
    // 지면 방향·원소와 조명 조건으로 허용 프레임을 선택한다. 변형 선택은 고정 시드의 어댑터다.
    std::size_t TerrainFrame(const TypeAsset& type,char a,char b,int theme,int x,int y,bool core) const;
    // 클러스터 기준 프레임과 보행 상태에서 실제 표시 프레임을 결정한다.
    std::size_t Frame(const o::Squid& object) const;
    // 그림 사각형을 원본 SHP의 기준점으로 변환한다.
    ScreenRect Bounds(const o::Squid& object) const;
    // 선택·소유자 표시에 쓰는 가장 가까운 팔레트 색을 찾는다.
    std::uint8_t Color(int r,int g,int b) const;
    // 원본 기본 색 띠 및 지면 명도 표 중 연결한 타입의 변환표.
    std::optional<ColorMap> Remap(const TypeAsset& type,int owner) const;
    std::shared_ptr<ClientGameplayGlobals> gameplayGlobals_; // 선택 ref와 Session/Input 모두보다 오래 같은 owner를 유지한다. 독립 어댑터만 자체 owner를 한 번 소유한다.
    // assets_: 타입·셰이프·팔레트 자산 또는 해당 자산의 처리 경계.
    const GameAssets& assets_;
    // chunks_: 월드를 청크 격자로 나누어 섬·영역 소속을 조회하는 지도 (o::ChunkMap).
    o::ChunkMap chunks_;
    // islands_: 청크 배치에 사용하는 섬의 소유자·영역 목록 (o::IslandList).
    o::IslandList islands_;
    // terrain_: 영역과 청크 패턴을 월드 지면 타일로 펼치는 처리기 (std::unique_ptr<o::TerrainBuilder>).
    std::unique_ptr<o::TerrainBuilder> terrain_;
    // ground_: 통행·점유·소유권을 조회하는 지면 격자.
    std::unique_ptr<o::GroundGrid> ground_;
    // players_: 미션 플레이어 목록과 기술·회수·승급·AI 허용 상태 (o::MissionPlayers).
    o::MissionPlayers players_;
    // objects_: 현재 월드 또는 수집 단계의 객체 목록.
    std::vector<o::Squid> objects_;
    // tiles_: 지형 생성 결과를 표시할 타일 목록.
    std::vector<Tile> tiles_;
    // surfaces_: 표면 객체의 공간·연결·수명 처리 자료.
    std::unique_ptr<RawSurfaceWorld> surfaces_;
    struct Input; // 같은 선택/강조/명령 및 기억점/다중 선택의 고정 주소 소유자다.
    std::unique_ptr<Input> input_; // 현재 Client 장치를 설치한 뒤 원본 입력을 처리한다.
    std::unique_ptr<RawCampaignSession> campaign_; // 실제 설치/전투/경로/HP/현재 Kernel의 단일 수명 소유자다.
    std::unique_ptr<RawCampaignFactoryDisplay> factoryDisplay_; // Session보다 먼저 해제할 실제 Factory 그림/선택 출력기다.
    std::function<std::uint32_t()> hideBuildings_=[] { return 0U; }; // 원본BSS초기0의입력이다.실제Client는같은메뉴DWORD조회기를설치한다.
    std::function<ScreenRect()> mapView_; // 실제 Client 도킹 전역보다 짧게 빌리며 사각형 값의 별도 동기화를 하지 않는다.
    CameraHomeState cameraHome_; // 실제 화면 크기·초기 시야 억제 깊이·요청/변환 갱신 번호를 보관한다.
    // surfaceTime_: 표면 프레임 갱신에 누적한 게임 시각(초).
    o::FrameTime surfaceTime_{};
    // surfaceCells_: 월드 칸별 표면 객체 등록 정보.
    std::vector<o::CellPoint> surfaceCells_;
    // bridgeType_: 다리 자산을 타입 표에서 찾는 번호.
    // noIslandType_: noIsland 자산을 타입 표에서 찾는 번호.
    std::uint32_t bridgeType_{},noIslandType_{};
    // regionOwners_: 지역 번호별 현재 소유 플레이어 표.
    // regionThemes_: 지역 번호별 현재 지면 원소 테마 표.
    std::array<int,o::kTerritoryCount> regionOwners_{},regionThemes_{};
    // coreVariants_: 본섬 지면 프레임의 변형 선택 자료.
    std::array<int,99> coreVariants_{};
    // palette_: 그림의 색 인덱스를 실제 출력 색으로 바꿀 팔레트.
    std::array<PaletteColor,256> palette_{};
    std::span<const std::uint32_t> colorSortLogical_; //World보다오래사는Client의고정초기입력이며별도값동기화는없다.
    // remaps_: 소유자·타입별 출력 색 변환표 캐시.
    mutable std::map<std::pair<std::size_t,int>,ColorMap> remaps_;
    // width_: 가로 크기.
    // height_: 세로 크기.
    // scrollX_: 카메라 스크롤의 X 원점.
    // scrollY_: 카메라 스크롤의 Y 원점.
    int width_{},height_{},scrollX_{},scrollY_{};
    // selected_: 현재 선택한 객체·컨트롤의 번호 또는 토큰.
    std::uint32_t& selected_{gameplayGlobals_->selection.selected}; // SquidId도uint32이며 Sid16bit로 alias하거나 잘라 저장하지 않는다.
    std::uint32_t& highlightMode_{gameplayGlobals_->selection.mode};std::uint32_t& mouseType_{gameplayGlobals_->selection.mouseType}; //5cae90/5caea4의 같은 현재 DWORD다.
    ScreenPoint& mousePoint_{gameplayGlobals_->selection.mouse}; //5cae94/98의 같은 원시 정수 입력이며 커서 보정 좌표와 합치지 않는다.
    // changed_: 상태 변경으로 후속 갱신이 필요한지.
    bool changed_{true};
};
}
