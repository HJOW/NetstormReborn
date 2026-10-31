#include "client/ClientCameraGlobals.h"
// 실제 캠페인 초기화/전투/이동의 상태와 모듈 수명을 클라이언트 세션 하나에 보관한다.
#pragma once
#include "client/RawImage.h"
#include "o/RawTerrainBitmap.h"
#include "client/GameAssets.h"
#include "client/CampaignLoginServices.h"
#include "client/MissionFortRequest.h"
#include "client/MissionFortSource.h"
#include "client/CampaignTemplateStorage.h"
#include "o/RawExistingEditorInstallOuter.h"
#include "o/RawStartupStateCheck.h"
#include <optional>
#include <memory>
#include "client/RawCampaignWorldRuntime.h"
#include "client/RawCampaignMotionBindings.h"
#include "client/RawCampaignAssetBindings.h"
#include "client/RawCampaignArcherBindings.h"
#include "client/RawCampaignHitBindings.h"
#include "client/RawCampaignLifetimeBindings.h"
#include "client/RawCampaignConstructionBindings.h"
#include "client/RawCampaignProductionBindings.h"
#include "client/SoundProcess.h"
#include "o/RawPlayerRegular.h"
#include "o/RawFortMissionSave.h"
#include "o/RawNewTechnology.h" //같은 현재 기술부모·snapshot과 실제 안내를 연결한다.
#include "o/RawFortSubscriberLoad.h"
#include "o/RawEditorPlayers.h"
#include "o/RawBattleBroadcast.h"
#include "o/RawMissionOutcome.h"
#include "o/RawAiBrain.h"
#include "o/RawSquidName.h"
#include "o/SpStore.h"
#include "o/RawPriestParticipation.h"
#include "o/RawDamageablePostDestroy.h"
#include "client/CameraHome.h"
#include "client/RawPriestMenu.h"
#include "client/RawFactoryMenu.h"
#include "client/RawCarrierMenu.h"
#include "client/RawObjectMenuOpen.h"
#include "client/RawObjectMenuInfo.h"
#include "client/RawObjectMenuVariants.h"
#include "client/RawUserSelection.h"
#include "client/RawCarrierInput.h"
#include "client/RawPlayerMenu.h"
#include "o/RawCarrierContentNotice.h"
#include "o/RawCarrierTransfer.h"
#include "o/RawCarrierTarget.h"
#include "o/RawCarrierChoice.h"
#include "client/RawCursorMana.h"
#include "client/RawCursorRange.h"
#include "o/RawEnergySupply.h"
#include "o/RawConstructionBuilder.h"
#include "o/RawTutorialOne.h"
#include "o/RawTutorialTwo.h"
#include "o/RawTutorialLater.h"
#include "o/RawMissionFrameGate.h"
#include "client/RawCampaignParticles.h"
namespace netstorm::o { class RawCarrierSteal; class RawCarrierPickup; class RawCarrierMove; class RawGemReward; class RawDudeBehavior; struct MissionStartGlobals; struct MissionAiSettingsState; struct FortObjectLoadState; class RawFortBadgesGlobals; } // 포획/Dude 몸체와 미션 시작 전역의 고정 참조만 공개한다.
namespace netstorm::o { class RawConnectionRegistry; } //실제 Client와 로컬 양끝이 빌리는 같은 연결표다.
namespace netstorm::client {
struct SendMoneyHooks; // 같은 실제 세션의 경제 상태를 일반 입력과 숫자 창에 빌린다.
class CampaignTemplateStorage; // Client보다 짧은 Session이 빌리는 같은 실제 Template9 수명이다.
class EditorFiles; // 실제 작업본 파일 장치이며 연결한 Session보다 오래 살아야 한다.
class FortTypeConversionStorage; // 같은 Client의 현재 conversion DWORD와 실제 할당 소유자다.
class ClientRandomGlobals; // 실제 Client의 CRT 상태와 Subscriber ID 최초시드 DWORD를 함께 빌린다.
class ClientSpGlobals; // 공통 시작에서 초기화한 같은 SP/추적 offset을 모든 실제 Session이 직접 빌린다.
class ClientSubscriberGlobals; // 실제 Client의 my/login 두 Subscriber DWORD만 빌린다.
struct ClientChatWindowGlobals; // 같은 현재 중계 마스크/무시·뉴스 상태의 Client 수명이다.
class ClientConnectionActivityGlobals; // Session보다 오래 사는 두 활동 표와 실제 Zone 간극 owner다.
class ClientPlayerRecords; // 실제 Player/factory와 로그인 카운터를 Client 수명에 보관한다.
class ClientCoreDataGlobals; // 현재 Core/active/cache를 같은 Client 수명에서 빌린다.
class ClientGameplayGlobals;struct ProductionControlContext; // 실제 초기 HUD와 같은 현재 전역/물리를 공유한다.
class ClientKencloudGlobals; // 같은0059a8b0 월드 다시 그리기 DWORD를 지형/섬도 직접 빌린다.
class ClientColorSortInput;class OwnerColorTableState; // 같은 Client 최초 RGB와 현재 전역 색표 소유자를 빌린다.
class ClientLocalStateGlobals; // 실제 Client의 State 다섯 블록과 활동 기한의 장기 소유자다.
struct ClientHtmlWindowGlobals; //같은 실제 HTML 배경80BYTE와 창/Tell/P 억제의 장기 owner다.
class EditorLocalTransport; // 준비된 Editor가 생성한 같은 listener/client9/accepted(-1)와 단일 등록표를 빌린다.
class InputQueue;struct ExistingEditorAfterCheckpointEffects; // 원문 tail의 같은 입력 고리와 실제 HUD/음악/팔레트 장치를 동기 호출로 빌린다.
struct TemplateWriteStream; // 실제 Template 저장소가 발급하는 명시 쓰기 수명이다.
struct QuickHelpTypeInfo; // 전용 도움말이 같은 현재 TYPE/요구 에너지를 읽는 정보다.
class RawCampaignCursor; // 세션과 같은 실제 커서/로컬 예측의 고정 주소 소유자다.
class RawProductionMaintenance;struct ProductionMaintenanceGlobals;struct ProductionMaintenancePanel; // 생산 전역/패널의 단일 상태를 빌리는 실제 유지 처리기다.
struct KnowledgeMenuState;struct KnowledgeMenuHooks; // 지식 창의 게임 조회는 같은 실제 세션에서 공급한다.
struct GameMenusHooks; // 상단 메뉴 부모도 같은 실제 모드/Player/SP/사제 수명을 빌린다.
struct CursorPlacementHooks; // 실제 커서 배치의 설명/에너지 경계만 연결할 때 사용한다.
// 실제 Client의 메뉴 작성·번역·Chat·정적 문자열 수명을 공급한다. 게임 계산 훅은 factory가 같은 세션으로 교체한다.
struct CampaignObjectMenuUi {
    std::span<const std::uint8_t> chat,meta; // 현재 Client Chat100×48과 선택적 metamap40×460 원문이며 복사하지 않는다.
    o::PlayerIdentityReads identity; // 주소/실패 이름의 실제 현재 getter다. 현재 이름/subscriber/모드는 같은 세션이 연결한다.
    ObjectMenuState objectState; // 현재 도움말 문자열/회수 금지·명령 전역이다. owner/모드는 세션 getter로 교체한다.
    PlayerMenuState playerState; // 실제 asksend 정적 버퍼와 명령 토큰이며 반환 체인이 수명을 소유한다.
    FactoryMenuHooks factory; // 실제 Factory 생산/지식 메뉴의 번역/controlled 작성 경계다.
    PriestMenuHooks priest; // 실제 GUI/번역/Tell이다. 게임 조회·생성·커서 취득은 세션이 공급한다.
    CarrierMenuHooks carrier; // 실제 GUI/번역/타입 설명이다. 지면·이동·위치·SP는 세션이 공급한다.
    ObjectMenuHooks object; // 실제 GUI/서식/설정/문자열 토큰이다. 두 미복원 파생 가상80만 count 경계를 사용한다.
    PlayerMenuHooks player; // 실제 Chat 상태/통신/허용 전역/GUI다. 현재 미션·요새·SP는 세션이 공급한다.
    ObjectMenuInfoHooks info; // 실제제목/머리·번역·순수서식·Config치환이다. 게임/Identity조회는같은Session이교체한다.
    ObjectMenuVariantHooks variants; // 여덟 파생+a8의 실제 번역/항목/설명/구분선이며 게임 조회는 같은 Session에 연결한다.
};
// 사제→Carrier→Damageable→공통 객체→Player의 실제 몸체를 한 수명으로 소유한다.
// 같은 세션/Client/Gump 트리는 이 객체보다 오래 살아야 하며 다음 메뉴 생성 전 이전 메뉴를 닫아야 한다.
class RawCampaignObjectMenus {
public:
    ~RawCampaignObjectMenus(); // 참조 대상인 부모 모듈보다 먼저 자식 메뉴 몸체를 해제한다.
    RawCampaignObjectMenus(const RawCampaignObjectMenus&)=delete; // 훅의 고정 주소와 정적 문자열 수명을 보존한다.
    RawCampaignObjectMenus& operator=(const RawCampaignObjectMenus&)=delete; // 서로 다른 세션 체인의 대입을 금지한다.
    void OpenFactory(o::Sid sid) const; // 실제 Factory +a8의 생산/지식/기본 Damageable 체인을 작성한다.
    void OpenPriest(o::Sid sid) const; // 루트 메뉴 한 칸을 caller가 만든 뒤 실제 사제 Open을 작성한다.
    void OpenCarrier(o::Sid sid) const; // 원본 가상 메뉴가 Carrier일 때만 같은 전체 부모 체인을 작성한다.
    void OpenDamageable(o::Sid sid) const; // 원본 가상 메뉴가 Damageable일 때 실제 회수/공통 체인을 작성한다.
    void OpenObject(o::Sid sid) const; // 원본 기본 객체 메뉴를 작성한다. 파생 메뉴를 임의로 이 함수로 대체하지 않는다.
    void OpenVariant(o::Sid sid,std::uint32_t originalBody) const; // 제네레이터/다리/풀숲/간헐천/신전 등 실제 파생+a8만 분배한다.
    std::uint32_t OpenPlayer(std::uint32_t subscriber) const; // 실제상단Players목록도같은현재RawPlayerMenu부모를사용한다.
    void Summary(o::Sid sid) const; // 현재객체의실제4인자정보/extra/포획표시를같은메뉴에작성한다.
    void Describe(std::uint32_t type,std::uint32_t count,std::uint32_t owner,std::uint32_t sid) const; // 원본4ad780의4인자를직접보존한다.
    bool SelectBuild(std::uint32_t type,o::Sid priest) const; // 같은 Open의 현재 기술/SP 관문과 실제 세션 커서 취득을 실행한다.
    std::string_view TextParameter(std::uint32_t token) const; // Player의 현재 정적 asksend 토큰만 해석한다.
    bool ResolveIdentity(o::PlayerIdentityRecord& record) const; // Player 메뉴와 같은 현재 식별자 검색을 Chat 조회가 사용한다.
private:
    friend class RawCampaignSession; // 같은 실제 세션 factory만 내부 참조를 구성할 수 있다.
    struct Impl; // 현재 입력/상태와 원본 메뉴 계층의 고정 주소 소유자다.
    explicit RawCampaignObjectMenus(std::unique_ptr<Impl> impl); // factory가 완전히 구성한 몸체만 받는다.
    std::unique_ptr<Impl> impl_; // Gump/Session보다 먼저 반납한다.
};
// 미션 입력/정지/UI 수명은 실제 UberGump의 현재 저장소와 동작을 빌린다.
struct CampaignMissionOutcomeUi {
    std::function<std::uint32_t()> demo; // 현재 미션 Demo DWORD를 읽는다.
    std::function<std::uint32_t()> tellBlocked; // 현재 실제 Tell 차단 DWORD를 읽는다.
    std::function<bool(std::string_view)> section; // 실제 미션/Tell 섹션을 실행하고 발견 여부를 반환한다.
    std::function<void()> closeTransient; // 성공한 섹션 뒤 원본 임시 Gump 정리를 실행한다.
    std::function<void()> onSection; // 성공한 섹션 뒤 현재 미션 가상 통지/정지 소유권을 처리한다.
};
// AI의 게임 계산과 색 표는 같은 Session이 소유하고 현재 언어/채팅/할당 및 표시 통지만 외부에서 받는다.
struct CampaignAiServices {
    o::SquidNameHooks names; //AI이동로그의같은현재언어/번역/설정치환이다.레벨은실제Session이연결한다.
    o::RawAiBrainMemory::Fill allocationBytes; // 실제 host 할당 직후의 입력 바이트다. 누락을 임의 0채움 성공으로 바꾸지 않는다.
    o::RawAiBrainMemory::Fill bridgeScratchBytes; //검색caller의초기8BYTE를별도로공급한다.본문힙할당입력이나임의성공좌표와합치지않는다.
    std::function<std::string()> defaultName; // 현재 defaultAiName 설정의 원본 좁은문자열을 반환한다.
    std::function<void(std::uint32_t,std::uint32_t)> changeColor; // 실제0043ca70의 표/섬 프레임 변경을 마친 뒤 보내는 선택형 화면 통지다.
    o::PlayerParticipationHooks participation; // 실제 Chat/동맹 송신/연결·진단이다. brain 소멸/반납은 Session의 실제 소유자로 교체한다.
    std::function<bool()> localTransport; // CheckLogin이 현재 로컬 전송 모드를 읽는 실제 경계다.
    std::function<bool()> blockingGumpOpen; //004641d0의 현재 실제 루트 직계 차단 창 조회다.
    std::function<void(std::string_view)> assertion; // AI 원본 진단 후 계속하는 장치 경계다. 누락을 빈 성공으로 바꾸지 않는다.
};
// 원본 Deck의 할당 입력과 현재 설정/진단을 실제 클라이언트에 연결한다.
struct CampaignProductionServices {
    o::RawProductionDeck::Fill allocationBytes; // 실제 할당 직후 바이트이며 생성자가 보존하는 꼬리를 임의 초기화하지 않는다.
    CampaignProductionEffects effects; // 현재 getTeleportTechFree와 원본 모드 진단이다.
};
// 실제Subscriber의등록설정/시각/진단만공급한다.정상ID의미선택장치를미리요구하지않는다.
struct CampaignSubscriberServices {
    std::function<bool(std::string_view,std::uint32_t&)> readRegistered; //00441270의실제같은ID참조/변경여부다.
    std::function<void(std::string_view)> configure; //00441580의원문오탈자키와unsigned값/실제변경callback이다.
    std::function<std::uint32_t()> clock; //새ID최초시드가선택할실제CRT clock이며벽시각으로대체하지않는다.
    std::function<void(o::FortSubscriberLoadAssertion)> assertion; //고정Template/직접읽기의원문진단이다.
};
// 생성전선택이며캠페인생성후Editor bool을켜는전환으로대체하지않는다.
enum class CampaignInstallMode { Campaign,ExistingEditor }; // Fort 편집 설치는 기존/신규 파일을 공통 부모에서 구분하고 원문 alternate0/1을 유지한다.
// 기존raw부모가소유한외곽순서에현재Client장치만공급한다.게임저장소는Session이덮어연결한다.
struct CampaignExistingEditorCheckpointServices {
    o::ExistingEditorFortHooks fort; //현재/pending이름·비교·Guide·setName·Config·outside만외부에서공급한다.CoreWrite를포함한게임child는Session이연결한다.
    o::ExistingEditorFileHooks file; //현재path/exists·실제제목·진단410·outside를공급한다.own/load/valid/State/Notify/Subscriber/handle/Config는같은Session으로연결한다.
    TemplateFileLoadHooks load; //같은작업본read/readonly/CRT비교/원문진단이다.현재TYPE/owner/중간Title은Session이연결한다.
    o::StartupStateCheckHooks startup; //실제004b5ba0이다.옵션0의자연ret외의선택reader/벽시각/서식/Tell누락을성공으로채우지않는다.
    o::EditorPlayersHooks players; //번역/서식만외부장치다.같은Player/TYPE/ContainedCreate/실제맵tail은Session이연결한다.
    o::InvalidEditorFileHooks invalid; // 로그/현재 기본 이름 비교/작업본 삭제만 외부 장치다. 헤더/반납/전체 이름 선택은 같은 Session 부모에 연결한다.
};
// 같은 Renderer/공유 다시 그리기 전역과 현재 논리 크기를 실제497960 장치에 전달한다.
struct CampaignCameraCommitContext {
    ClientCameraGlobals& camera; // Session과 Client가 공유하는 현재view/snapshot/목표의 같은 주소다.
    std::uint32_t& worldRedraw; //0059a8b0의 현재 DWORD이며 독립 복사본을 만들지 않는다.
    std::int32_t width{},height{}; // 즉시 중심 요청이 사용한 현재 논리 화면 크기다.
};
// 게임 계산을 대신하지 않고 실제 클라이언트 UI/음향/전송의 선택 경계를 공급한다.
struct RawCampaignSessionServices {
    std::function<void(const CampaignCameraCommitContext&)> cameraCommit; // 실제497960의 커서/화면/dirty/Gump 장치다. 생략된 실제 호출은 두setter/noop로 대체하지 않는다.
    std::function<ScreenRect()> mapView; // 초기 Reset 전부터 같은 현재 Gump 지도 영역을 읽는다. 독립 비GUI 장치만 기존 전체 viewport 계약을 사용한다.
    std::shared_ptr<ClientChatWindowGlobals> chatGlobals; // 실제 Client는 같은 Chat 상태를 제공하며 독립 Session만 별도 BSS를 소유한다.
    std::function<void(std::span<std::uint8_t>)> chatReceive; // 실제 현재 UI 수신이다. 중계 후 선택된 호출에서만 요구한다.
    CampaignInstallMode installMode{CampaignInstallMode::Campaign}; //기존모든호출은캠페인입력을보존한다.
    bool deferExistingEditorFile{}; //명시ExistingEditor만ctor파일설치를checkpoint의실제File.load까지지연한다.기존Campaign/LocalLogin/단계검사는false를유지한다.
    bool adoptPreparedWorld{}; // 실제 State cleanup/prefix가 준비한 같은 Client 전역·표면·Player를 초기화 없이 빌린다. 지연 ExistingEditor 설치에서만 사용한다.
    std::function<void(std::span<std::uint8_t>)> templateStateBytes; // 빈 State 초기화의 미정 바이트를 현재 실제 할당 입력으로 공급한다.
    std::shared_ptr<CampaignTemplateStorage> templateStorage; // Init의 명시 정리까지 실제 manager/State/이미지를 같은 저장소에 유지한다.
    std::shared_ptr<const std::vector<std::uint8_t>> missionFortImage; // 실제 선택 파일의 같은 원문이며 초기 manager 읽기 부모가 소비한 뒤 반납한다.
    std::function<std::vector<std::uint8_t>(std::string_view)> missionFortRead; // 독립 합성/명시 이미지 어댑터의 reader다. 실제10.78 Client는 아래 지연 선택기로 원문 파일 시점을 사용한다.
    MissionFortFileSelector missionFortFileSelect; // 실제10.78은 경로를 먼저 선택하고 Template 부모가 요구할 때만 읽는다. 같은 이름 manager는 I/O 없이 보존한다.
    TemplateFileLoadHooks missionFortLoad; // 실제 파일의 권한/거부/진단 경계다. read·현재타입·현재owner·제목은 같은 Session이 연결한다.
    std::string missionFortFileName; //최초 missionFortImage와 같은 읽기에서 선택한 실제 파일 경로다. 생략한 독립 호출만 기존 논리 이름을 유지한다.
    std::shared_ptr<MissionFortLoadRequest> missionFortRequest; // Init에서 만든 같은 요청/Zone을 소비하며 설치 때 새 Zone을 만들지 않는다.
    std::shared_ptr<std::array<std::uint8_t,87>> missionOptions; // Client Init와 실제 로그인/세계/메뉴가 같은 전역87BYTE를 소유한다. 독립 검사는 자체 배열을 쓴다.
    std::shared_ptr<std::array<std::uint32_t,256>> missionRestrictions; // 설치 전 Header와 설치 후 TotalMade가 같은 현재 표를 빌린다.
    std::optional<std::uint32_t> missionSubscriber; // Init이 현재54265c에서 복사한540c74다. 독립 로그인은 기존1을 사용한다.
    std::function<void()> refreshSp; // 같은 실제 Reward.AddSp의0043dad0 표시 요청이다. 장치 없는 독립 검사는 비워 둘 수 있다.
    o::NewTechnologyHooks newTechnology; // 이름/설정/지역 그림 서식/Tell의 실제 Client 장치다. 게임 조회/추가/저장은 Session이 덮어 연결한다.
    std::optional<CampaignLoginServices> localLogin; //실제Client는 원본 로컬로그인을 사용하며 독립 모듈 검사는 명시Load를 유지한다.
    std::shared_ptr<EditorLocalTransport> editorTransport; // 준비된 기존 Editor만 동일 Player 물리의 로그인 없는 실제 송수신 큐를 빌린다. Session보다 긴 Client 수명을 허용한다.
    std::function<std::int32_t()> unixSeconds; // 실제 time32의 현재 Unix 초다.
    CampaignSubscriberServices subscriber; //현재같은Client등록Config/CRT clock/진단이며소진로그9는player.log를사용한다.
    RawPriestWorldEffects priest; // 원본 소유자 통지/보호막 안내/음향 등의 실제 외부 소비자다.
    CampaignOwnershipEffects ownership; // 실제 생산 UI/현재 themeMode 및 진단 경계다.
    RawCampaignInstallEffects install; // 실제 번역/설정/창 제목/카메라의 외부 소비자다.
    RawCampaignAssetEffects assets; // 실제 입력 큐/총 전송/자산 음향의 현재 소비자다.
    RawCampaignHitEffects hit; // 실제 owner 피해 통지/nonzero Burning/Dude 대피 생성 경계다.
    RawCampaignArcherEffects archer; // 실제 음향/fencemark 코너 및 미복원 다른 총의 경계다.
    RawCampaignMotionEffects motion; // 실제 경로의 음향/운반/의식 등 외부 하위 소비자다.
    o::PlayerRegularHooks player; // 실제 Player 로그와 예약 외부 경계다.
    CampaignMissionOutcomeUi missionOutcome; // 현재 미션의 실제 UI/차단 조회 경계다. Check를 호출할 때만 필요하다.
    CampaignAiServices ai; // 선택된 실제 AI 초기화/참여의 외부 경계이며 Load/프레임에서 자동 호출하지 않는다.
    CampaignProductionServices production; // 같은 Player/Factory/허용 표가 사용할 Deck의 실제 외부 입력이다.
    std::shared_ptr<o::RawFortBadgesGlobals> fortBadges; // 같은 Client의 단일6160BYTE 장부이며 로드/변경/저장이 이 저장소를 공유한다.
    std::shared_ptr<FortTypeConversionStorage> fortTypeConversion; // Objects/Technology/Deck이 같은 현재 토큰과 할당을 빌린다.
    std::optional<o::FortMissionSaveHooks> fortMission; // Guide-only가 아닌 현재 실제 Runtime getter/진단이다. write는 각 Snapshot이 같은 새 stream에 연결한다.
    std::function<void()> fortTechnologyUi; //0042f540의현재Chatmanager/CoreData관문과랭크prefix갱신이다.생산refresh나입력접두로대체하지않는다.
    o::EditorPlayersHooks editorPlayerUi; // 같은 실제 Client 번역/임시 고리의 기본 장치다. 명시 설치 효과가 제공한 장치는 그대로 우선한다.
    std::function<std::uint32_t()> editorIslandGreen; // bitmap 최초 분류가 실제 현재 Screen005acd20 녹색을 읽는 장치다.
    std::function<void(std::string_view)> factoryTell; //Factory 입력의 같은004cf960 안내 섹션을 실제 현재Tell에연결한다.
    std::function<void(std::string_view,std::uint32_t)> factoryHelp; //researchHelp의실제전용도움말창/모드1이다.
    o::DamageablePreDestroyHooks damageablePre; // 붕괴/폭발/음향의 실제 장치/효과 경계다. 해방/공통 삭제는 Session이 공급한다.
    o::DamageablePostDestroyHooks damageablePost; // 드롭 Dude/받침의 실제 생성 경계다. 예약/현재 다리/공통 삭제는 Session이 공급한다.
    RawCampaignConstructionEffects construction; // 로컬 간헐천의 실제 설정/벽시각/표시/진단이며 일반 건설을 대신하지 않는다.
    o::SquidDestroyHooks deletion; // 다른 파생 삭제/선택/전송의 실제 외부 경계다.
    std::function<void(o::Sid)> unitLost; // 공통 삭제가 요청한 실제 unitLost 전역 소리를 재생한다.
    o::CarrierContentNoticeHooks carrierContent; // 실제 Carrier 폭탄/포획 표시·음향·공간 효과이며 선택한 분기만 필수다.
    std::function<void(std::uint32_t,std::string_view)> cursorRangeDiagnostic; // 실제 사정거리 color29 진단 출력이며 해당 분기에서만 필요하다.
    o::ContentLifetimeHooks contentLifetime; // 같은 ContentForm의 실제 커서/수입/AI 지갑/소리 경계다.
    o::GumpFormNotice gumpNotice; // 현재등록ID에서실제UI의가상+4로부모삭제를통지한다. UI는Session보다오래살아야한다.
    o::PlayerContentNoticeHooks playerContent; // 같은 현재 Gump 등록표와 대상 자신의 가상 처리를 공급한다.
    o::SquidAssetDestroy destroy; // 다른 파생 자산의 실제 가상 삭제 분배기다.
    o::FormPopTransmit dependencyTransmit; // 감시 Form의 현재 서버 송신 소비자다.
    std::function<void(o::Sid,o::Sid,std::uint32_t)> contentNotice; // 실제 내용물 부모 통지다.
    SoundList* soundNames{}; // 클라이언트의 단일 실제 이름표를 공유한다. 별도 토큰표를 만들지 않는다.
    std::function<void(std::string_view)> requestMusic; // 기존 ClientAudio의 실제 의식 곡 요청이다.
    SoundProcessHooks sound; // 실제 같은 SoundPlayer/벽시계에 연결한 재생·정지 경계다.
    std::function<void(std::span<const std::uint8_t>)> pathPacket; // 실제 Path 직렬화 버퍼의 송신 소비자다.
    std::function<void(std::span<const std::uint8_t>)> formPacket; // 실제 감시/내용물/삭제 직렬화 후의 송신 경계다. 없으면 동일 BattleBroadcast를 사용한다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> carrierRequest; //4e2dc0의실제로컬전송/연결장치다.직접서버처리로대체하지않는다.
    std::function<void(std::int32_t,std::int32_t,std::int32_t,std::uint32_t)> carrierDropPopup; // 실제43dd20 비용 Gump의금액/화면x/y/참조SID다.
    o::BattlePacketPeers battlePeers; // 실제0..8 연결 객체 표다. 단독 캠페인은 NULL 연결이며 별도 송신 관찰 함수가 필요하지 않다.
    std::shared_ptr<ClientColorSortInput> colorSortInput; // 실제 Client가 최초 고정 lookup 입력을 수명째 빌린다. 독립 장치 없는 기존 검사만 두 색 서비스 모두 생략한다.
    std::shared_ptr<OwnerColorTableState> ownerColorTables; // 실제 Client는 같은557940을 연결한다. 기존 독립 표시 입력의 지역 색표와 구별한다.
    std::function<std::span<const ScreenColor>()> screenPalette; // 연결 시점의 현재 화면 팔레트를 빌리며 Screen 교체 전 주소를 캐시하지 않는다.
    std::shared_ptr<ClientRandomGlobals> randomGlobals; // 생략된 독립 검사만 기존Session CRT0을 사용하고 실제Client는 같은장기소유자를 전달한다.
    std::shared_ptr<ClientSpGlobals> spGlobals; // 실제 Client는 같은 초기화 완료 SP를 제공한다. 독립 Session만 기존 지역 SP 초기화를 유지한다.
    std::shared_ptr<ClientSubscriberGlobals> subscriberGlobals; // 생략된 독립 검사만 기존 0/0 및 Begin 값 복사를 유지한다.
    std::shared_ptr<ClientPlayerRecords> playerRecords; // 미연결 독립 Session만 자체 실제 할당 소유자를 만든다.
    std::shared_ptr<ClientCoreDataGlobals> coreDataGlobals; // 실제 Client는 Login/Chat과 같은 현재 Core 슬롯·캐시·active를 사용한다.
    bool adoptInitializedPlayers{}; // 앞선 Native 부모의 실제 초기화 완료 기록을 대여하며 Session 생성자에서 다시 재할당하지 않는다.
    std::shared_ptr<ClientGameplayGlobals> gameplayGlobals; // 실제 Client의 Player/커서/검색/미션 전역이며 독립 Session만 자체 소유한다.
    std::shared_ptr<ClientKencloudGlobals> kencloudGlobals; // 실제 Client의0059a8b0이며 미니맵005949d4와 다른 주소다.
    o::Kernel* processKernel{}; // 실제 Client의 한 전역 Kernel을 빌린다. 소유자는 모든 Session/월드 소비자보다 오래 살아야 한다.
    std::shared_ptr<RawSurfaceStorage> surfaceStorage; // 실제 Client의 한 Pool/Hash/TYPE/spot을 빌리며 독립 Session만 자체 물리를 만든다.
    std::shared_ptr<ClientLocalStateGlobals> localStateGlobals; // 생략된 독립 로그인 검사만 기존 local 블록/기한을 사용한다.
    std::shared_ptr<ClientConnectionActivityGlobals> connectionActivityGlobals; // 실제 Client와 Login/Editor 수신이 같은 두22DWORD 및 Zone DWORD를 직접 공유한다.
    std::function<std::uint32_t()> connectionFrameNumber; // 실제 Client::Time().number의 현재 하위32비트를 Receive의 원문 시점에 읽는다.
    std::function<void(std::uint32_t,std::uint32_t)> removeConnectionStatus; // missionPost 관문 뒤004d3090(kind,player,0,0,빈문자열)의 실제 WarningMessages 삭제를 호출한다.
    std::shared_ptr<o::RawConnectionRegistry> connectionRegistry; //실제 Client의 생성/수락/재배치/정리 표이며 독립 전송만 자체 표를 만든다.
    std::shared_ptr<ClientHtmlWindowGlobals> htmlWindowGlobals; //실제 Client는 Tell/Help/HUD와 로그인 시작이 같은565e70 첫BYTE를 읽고 쓴다.
};
// 초기 객체 생성부터 Kernel 해제까지 동일 주소에서 실제 모듈과 상태를 소유한다.
class RawCampaignSession {
public:
    // assets/files는 세션보다 오래 살아야 한다. fort와 이름은 세션이 보관하며 PATCH10.78을 사용한다.
    RawCampaignSession(const GameAssets& assets,const o::BaseFileSystem& files,o::FortTemplate fort,
        std::string mission,RawCampaignSessionServices services);
    // 내부 callback 대상의 해제 전에 모든 실제 프로세스를 비운다.
    ~RawCampaignSession();
    RawCampaignSession(const RawCampaignSession&)=delete; // callback의 고정 주소를 유지한다.
    RawCampaignSession& operator=(const RawCampaignSession&)=delete; // 다른 월드 상태와 복사하지 않는다.
    // 실제 부모 설치를 한 번 실행한다. 실제 큰 하위가 누락되면 오류를 그대로 반환한다.
    void Load();
    // 기존Fort의Player→CoreData자연생략→Territory회복→일반World단계까지만실행한다.전체loaded성공을설정하지않는다.
    void LoadExistingEditorWorld();
    void LoadExistingEditorSubscriber(); //실제파일꼬리에서World보다먼저ID/handle을읽는다.전체외곽/loaded는완료하지않는다.
    void LoadExistingEditorDeck(); //실제World/Graph뒤의Deck부모만실행한다.외곽완료/loaded확정은별도다.
    void InstallExistingEditorCheckpoint(const CampaignExistingEditorCheckpointServices& effects,const std::uint32_t& readOnlyForts,const EditorFiles& files); //명시지연Editor의실제파일부터004b743b까지한번조립한다.참조장치는Session보다오래살며최종loaded는쓰지않는다.
    bool ExistingEditorCheckpointReady() const; //전체설치성공이아닌위한정부모의정상복귀여부만조회한다.
    void WithExistingEditorPrefixUi(ClientLocalStateGlobals& owner,std::function<void()> prepareUi); //State1 prefix의 실제 HUD 동기화만 파일 설치 전에 허용한다. Session은 게임 상태를 쓰지 않는 prepared 대여여야 한다.
    void WithExistingEditorInstallUi(ClientLocalStateGlobals& owner,InputQueue& input,const ExistingEditorAfterCheckpointEffects& effects,std::function<void()> prepareUi); // 같은 checkpoint 뒤 게임/UI 하위를 한 번 준비하고 실제 원문 tail을 직접 호출한다. 공개 입력·저장은 계속 거부한다.
    void CommitExistingEditorInstall(ClientLocalStateGlobals& owner); // 같은 전체144BYTE State의 정상 공통 확정과 UI/tail 완료를 검사한 마지막 한 곳에서 Editor loaded를 공개한다.
    void WriteExistingEditorCoreData(std::uint32_t slot); //명시Editor의실제004beb30을현재Storage/모드에연결한다.직렬화하위미복원은선택위치에서거부하며loaded를변경하지않는다.
    void RefreshCurrentPlayerCore(); //실제선택Login Core/현재기술/다이스count로0044fc00을실행한다.선택Core없음은필수누락이며loaded/owner를바꾸지않는다.

    // 외곽전체Snapshot/State/lastEditFort뒤호출한다.번역/format을받고 마지막0046e980은같은실제맵에연결한다.
    void InstallExistingEditorPlayers(o::EditorPlayersHooks ui);
    //현재실제설정/시계/Player/Template로 원본로컬로그인 후 같은Load를 정확한 위치에서 실행한다.
    void LoadLocalConnection();
    void StartLocalConnection(); // 독립 편의 접합은 같은 준비/상태8까지 실행한다. 실제 Client의 실행 중 부모는 PrepareLocalConnectionForState를 사용한다.
    void PrepareLocalConnectionForState(); // 실행 중인 외부 State7 부모에서 Zone/후반만 설치한다. 공통 확정이나 상태8 접속을 중첩 실행하지 않는다.
    void ConnectLocalConnectionForState(std::span<const std::uint8_t> address,std::uint32_t port); // 부모의 현재8이 같은 로컬 접속 몸체를 사용한다. 외부 TCP/IP 접속을 대신하지 않는다.
    bool PollLocalConnectionState(); // 다음8/9/10/13/14를 한 번 소비했으면 true다. 메인/Edit/이탈 부모는 소비하지 않아 같은 Tick에 두 상태를 실행하지 않는다.
    bool LocalConnectionReady() const; // 실제 loaded/phase14/PlayerKnown/현재 Player 참여 비트만 조회한다. 별도 성공 플래그를 만들지 않는다.
    //프레임마다현재실제송신/수신큐와로그인타이머를진행한다.
    void PumpLocalConnection();
    //현재연결상태/실제수신패킷은진단전용읽기다. 별도성공flag를만들지않는다.
    std::uint32_t LocalConnectionPhase() const;
    std::span<const std::uint8_t> LocalConnectionPackets() const;
    // 커서가 작성한 원본 건설 요청도 같은 실제 로컬 로그인 전송에 보낸다.
    void SendLocalPacket(std::int32_t target,std::span<const std::uint8_t> packet);
    void SendMutableLocalPacket(std::int32_t target,std::span<std::uint8_t> packet); // 실제 packet observer→같은 로컬 큐를 사용하는 원시 작성기 경계다.
    // 실제443fe0을 같은 시각/Host/표시/음향으로 실행한다. duration0은 원본 기본10초다.
    void StartConstruction(o::Sid parent,float duration);
    // 현재 같은 풀/TYPE/실제 로컬 소유자·모드로 원본 초기 시야를 계산하여 화면에 적용한다.
    void ResetCamera(CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> apply) const;
    void RequestCameraCenter(float x,float y,std::int32_t width,std::int32_t height); //미니맵/사건의497f20 직접 호출만 실제 Client의 목표와005c85dc에 연결하며433aa0의 즉시commit과 구별한다.
    void CommitRequestedCamera(std::int32_t width,std::int32_t height); //이미433aa0이 쓴 목표를 같은 실제497960 장치에 적용하며 새 목표/카운터·flip/제한을 더하지 않는다.
    //00433ba0의 현재 SID 중심 이동과 Carrier 내용물 표시를 같은 실제 로딩 깊이/Kernel에 연결한다.
    void CenterCameraOn(std::uint32_t sid,CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> apply) const;
    //현재 localPlayer의 같은 Player +7c를 먼저 읽고0일 때만 +84로 직접 CenterOn한다. 외부 메뉴 모드 관문은 호출자가 처리한다.
    void ViewHome(CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> apply) const;
    //004950d0→00494f10: live 사제/포획 Carrier/제단을 실제 참여 조회로 찾은 뒤 같은 CenterOn을 호출한다.
    void ViewYourPriest(std::uint32_t owner,CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> apply) const;
    //ReloadTheams의 현재 signed count다. first는0055a70c outposts, second는005e189c temples이며 사본을 만들지 않는다.
    std::int32_t GameMenuReloadCount(bool second) const;
    //현재 목록의 DWORD SID를 그 호출 시점에 다시 읽는다. count 확인/순회는 원본 GameMenuActions가 소유한다.
    std::uint32_t GameMenuReloadSid(bool second,std::uint32_t index) const;
    //455380의 실제 mode0 지역 투표/도장을 같은 ownership 몸체로 실행한다. 좌표는 원문 float비트다.
    void ReloadTheamAt(std::uint32_t x,std::uint32_t y,std::uint32_t mode=0) const;
    // 현재 같은 시각을 모든 모듈에 공급한 뒤 동일 Kernel을 실행한다.
    void RunFrame(o::FrameTime time,bool paused);
    void PrepareFrame(o::FrameTime time,bool paused); // 실제 GUI가 단일 전역 Kernel 앞에서 시각/AI/표시 입력만 준비한다.
    // 같은 실제 카메라와 현재 장치의 dirty 경계를 process11에 공급한다.
    ClientCameraGlobals& CameraGlobals(); // Client와 같은 현재64BYTE/목표 소유자다.
    const ClientCameraGlobals& CameraGlobals() const; // const Session도 현재 snapshot을 복사하지 않는다.
    void SetParticleView(SoundView view,SurfaceDisplayHooks display,bool updateCamera=true); // 실제 World는 viewport만 전달하고 기존 독립 호출자는 픽셀 입력을 명시 적용한다.
    // 실제 원본 표시 목록의 현재 직접 그리기를 다른 월드 SHP와 함께 정렬한다.
    std::vector<RenderSprite> ParticleSprites(ScreenRect clip) const;
    // 후속 검증이 같은 Host/Kernel의 등록을 읽는 현재 표시 관리자다.
    const RawCampaignParticles& Particles() const;
    RawCampaignParticles& Particles(); //실제 Renderer도 같은 Session의 현재 단일 표시 목록/장치를 직접 빌린다.
    // 원본 미션 게이트의 첫 checkOutcome 위치에서만 호출한다. 설치/활성화 전에 자동 실행하지 않는다.
    void CheckMissionOutcome();
    // 같은 현재 Player/ownerState를 읽는 원본4922e0이며 TellAt의 사제 생존 관문이다.
    bool MissionPriestDead(std::uint32_t owner) const;
    // 최근 간헐천5칸/현재 Player/장부/Graph/SP를 공유하는 Early1 판정기를 만든다. gate는 더 오래 살아야 한다.
    std::unique_ptr<o::RawTutorialOne> MakeTutorialOne(o::RawMissionFrameGate& gate);
    std::unique_ptr<o::RawTutorialTwo> MakeTutorialTwo(o::RawMissionFrameGate& gate,o::TutorialTwoHooks hooks); // 같은 로그인 옵션/범위/장부/금지를 빌리고 선택·Tell·시계만 실제 UI에서 받는다.
    // Early3~6의 실제 옵션/사제/Graph/생산/HP/지역을 연결한다. UI와 같은 Start 상태는 호출자가 빌려준다.
    std::unique_ptr<o::RawTutorialLater> MakeTutorialLater(o::RawMissionFrameGate& gate,o::MissionStartState& state,o::TutorialLaterHooks hooks);
    // 원본 로그인/월드 로더가 공유하는 현재 globals.mySubscriber를 읽기만 한다. 별도 캐시를 만들지 않는다.
    const std::uint32_t& CurrentSubscriber() const; // 같은 Client 또는 독립 Session의 실제 My DWORD 참조다.
    const std::shared_ptr<ClientCoreDataGlobals>& PlayerCoreGlobals() const; // 같은 현재 Core 소유자를 조회하며 선택/캐시 복사를 하지 않는다.
    std::string_view CurrentHandle() const; // 실제 월드 로더가 갱신한 같은64BYTE 사용자 이름을 NUL까지 빌린다.
    // 같은 실제 PlayerWorld의00540c70을 읽는다. 자산/UI 어댑터의 로컬 번호 사본을 사용하지 않는다.
    std::uint32_t LocalOwner() const;
    std::uint32_t TemplateOwner() const; // 실제00540cac의 템플릿 소유자이며 로컬 소유자와 구별한다.
    std::uint32_t BattleActive() const; // 원문00594fbc의 현재 실제 자산 Battle 관문을 읽는다. Editor/표시phase로 대체하지 않는다.
    std::uint32_t CurrentTechnologyParent() const; // 현재48fc60의 실제 기술 부모다.
    std::uint32_t IslandCount() const; // 설치중 Add Island 관문도 현재3204BYTE IslandList의 countDWORD를 직접 읽는다.
    std::uint32_t OwnedIslandCount(std::uint32_t owner) const; //0046f430의 같은 현재 섬 목록에서 지정 owner의 실제 개수를 읽는다. owner0도 원문대로 비교한다.
    std::span<std::uint8_t> MutableFortState(std::uint32_t slot); //004bd550의 실제 현재 State/최소28BYTE를 빌린다. 다른 callback 뒤에는 다시 조회해야 한다.
    void SynchronizeProductionContents(); // 실제43ec00(first1)의43df80 직접 호출이다.
    const o::ContainedFinderState& ProductionFinder() const; // 같은 현재 내용물 검색 전역이다.
    ProductionControlContext ProductionContext() const; // 실제 같은 물리/현재 전역/Player/옵션을 초기 HUD 생성자에 빌린다.
    bool UpdateProductionContents(); // 실제현재모드/부모/dirty→Deck 재계산→43df80 단락이다.
    void GetFullDeck(); //0044f9d0의실제전체Deck명령을같은현재Client DWORD·Player Deck·공유난수로실행한다.
    std::unique_ptr<RawProductionMaintenance> MakeProductionMaintenance(ProductionMaintenanceGlobals& globals,ProductionMaintenancePanel& panel,std::function<double()> wall,std::function<void(std::uint32_t)> refresh); // 같은 현재 Player/SP/난수/생성/삭제를 실제 UI 유지에 연결한다.
    // 실제 PlayerWorld가 공유하는00595344 DWORD를 읽는다. subscriber/Ready와 합치지 않는다.
    std::uint32_t PlayerKnown() const;
    // 실제 캠페인 로더와 같은005c85a4 DWORD를 읽는다.
    std::uint32_t Editor() const;
    // 사제/Player 내용물/건설/보상이 공유하는 실제 분산 SP를0040ec50의 float비트로 읽는다.
    float Sp(std::uint32_t owner) const;
    // 원본0040edb0처럼 직접 float비트만 쓴다. AddSp의 지갑/화면 갱신을 대신 호출하지 않는다.
    void SetSp(std::uint32_t owner,float value);
    SendMoneyHooks MoneyInputHooks(); // 메뉴·숫자 창이 같은 실제 SP/Player/AI 지갑/로컬 송신을 사용한다.
    std::int32_t AiMoney(std::uint32_t owner) const; // 현재 AI 본체 지갑의 읽기 전용 관측이며 분산 SP와 구별한다.
    // 같은 미션의 회수·승천·포획·간헐천 설정을 Start와 후속 게임 명령이 직접 공유한다.
    o::MissionStartGlobals& MissionStartSettings();
    // 원본 AI 설정 기본 전역(3/2500)과 진단 상태를 같은 세션의 Start에서 사용한다.
    o::MissionAiSettingsState& MissionAiSettings();
    // 같은 실제32×9 원문 배열/offset/추적주소를 읽기만 한다. 별도 SP 저장소를 생성하지 않는다.
    const o::ScrambledSpStore& SpStore() const;
    void PrepareGameMenus(); //00491680의같은실제SP회전을호출한다. 메뉴열기를위해게임정지를추가하지않는다.
    std::int32_t& GameMenuTrackingOffset(); //0054d4e0의기존사제/SP공유소유주소를직접빌린다. 값복사나사본동기화가아니다.
    GameMenusHooks BindGameMenus(GameMenusHooks hooks); //현재Session의모드/Player/사제/SP/로그인/신전비용으로게임조회만연결한다.
    // 실제 로그인/AI 초기화가 사용하는 동일9 Player 원문을 빌린다. 프레임별 사본을 만들지 않는다.
    std::span<std::uint8_t> PlayerRecords();
    RawProcessHostStorage::ClearPass ClearProcessesForState(); // 실제 같은 Kernel의 단일 Clear 정상 반환만 증빙한다.
    void ReturnProcessBindingsForState(RawProcessHostStorage::ClearPass&& pass); // Regular 잔류의 장기 Host를 유지하며 현재 처리기 대여를 실제 반납한다.
    void ReleasePlayerLinks(); //0048fcd0의 실제9기록 링크/개수/덱만 정리한다. 실패는 전파하며 Kernel/Core/Player 할당은 유지한다.
    o::RawProductionDeck::PlayerLinksPass ClearPlayerLinksForState(); // 같은 실제0048fcd0 정상 복귀 뒤에만 이동 전용 증빙을 발급한다.
    void ReturnPlayerLinksForState(o::RawProductionDeck::PlayerLinksPass&& pass); // 같은 실제 Deck의 증빙만 소비하며 전체 Session/PlayerUse는 유지한다.
    o::RawPlayerParticipation::BrainResetPass ResetAiForState(std::uint32_t owner); // 원문 owner1..8 resetAi 정상 복귀만 증명한다.
    void ReturnAiForState(o::RawPlayerParticipation::BrainResetPass&& pass); // 같은 실제 AI 모듈의 owner별 증빙만 소비한다.

    // 실제 방향별동맹 초기화가 쓰고 승패가 읽는 같은81DWORD를 빌린다.
    std::span<std::uint32_t> Alliances();
    // 로그인/Stage3의 후속 부모가 같은 현재 Player9/동맹/brain 수명을 직접 사용한다.
    o::RawPlayerParticipation& Participation();
    // 같은 현재 채팅 숨김 BYTE9 저장소다. 방향별 동맹표나 참여 flag와 합치지 않는다.
    std::span<std::uint8_t> HiddenPlayers();
    std::uint8_t BattleOption(std::size_t offset) const; // 같은 실제87BYTE의 현재 메뉴 허용 값을 읽는다.
    // 설치 뒤 실제 생성자→현재 Player 부착→초기 갱신을 실행한다. 전략 프레임/자동 승패는 켜지 않는다.
    std::uint32_t CreateAi(std::uint32_t owner,std::span<const std::uint8_t> settings);
    bool PrepareAi(std::uint32_t owner); // 같은 현재 brain에00416eb0 준비 전체를 명시 호출한다. 없음은false이며 전체 전략/프레임 자동 활성화는 후속이다.
    bool InitializeAi(std::uint32_t owner); //같은 현재 brain의00414af0 초기 전략을 명시 재시도한다.258 완료 여부를 반환하며 자동 프레임은 후속이다.
    bool RefreshAiBridges(std::uint32_t owner); //같은 현재 brain/기술부모로0040fcb0 재고를 갱신한다. 본체 없음은false이며 자동 프레임은 후속이다.
    bool AdvanceAiBridge(std::uint32_t owner,o::RawPathPoint target,o::RawPathPoint& endpointScratch); //004174c0의실제시작점/예산검색/배치/소비/재고갱신이다.원본caller임시XY는명시입력/출력이며false는본체없음또는실제timeout이다.전체자동전략순서는후속이다.
    bool ReconcileAiEconomy(std::uint32_t owner); //같은 본체 지갑과 분산SP로004138c0을 명시 실행한다. 본체 없음은false이며 자동 프레임은 후속이다.
    bool DeployAi(std::uint32_t owner,std::uint32_t type,float x,float y,std::uint32_t argument=0,std::uint32_t direction=0); //선택된 전략 입력으로004106e0 실제 생산/비용/배치를 명시 실행한다. 자동 선택은 후속이다.
    bool EnsureAiCollectors(std::uint32_t owner); //원본00415a10 수집자 선택→자기 영역 후보→실제 배치를 명시 실행한다. 전체 자동 프레임은 후속이다.
    bool ManageAiFactories(std::uint32_t owner); //004136b0의실제생산/시설큐·업그레이드요청을명시실행한다.전체자동전략순서는후속이다.
    bool RecordAiDrySpot(std::uint32_t owner,float x,float y,std::uint32_t type); //0040f1c0의같은실제본체/좌표목록등록이다.본체없음/중복/full은false다.
    std::uint32_t RecheckAiDrySpots(std::uint32_t owner); //0040f2b0/417c70의한좌표에너지재검사/제거·현재로그다.반환은제거TYPE 또는0이다.
    bool SeedAiDrySpot(std::uint32_t owner); //004133e0을 같은 실제 본체/공용 난수·Dry Spot 목록으로 호출한다. true는 본체 존재/호출이며 등록 성공을 뜻하지 않는다.
    o::RawPathPoint ComputeAiDryCandidate(o::RawPathPoint source,o::RawPathPoint target); //00415bb0의 실제 현재 경계/지도·디버그 표식 탐색이다.전체415fe0도 같은 하위를 공유하며 자동프레임은 후속이다.
    bool ExtendAiFakeDrySpots(std::uint32_t owner); //00415fe0 전체 가짜좌표 갱신이다.true는 본체 존재/호출이며 실제 추가 성공을 뜻하지 않는다.
    o::Sid AiMovementForm(o::Sid actor) const; //00411630의현재실제경로Form조회다.
    void StopAiMovement(o::Sid actor); //00411680의같은종속/Host경로삭제다.0은무동작이다.
    bool MoveAiToObject(std::uint32_t owner,o::Sid actor,o::Sid target,std::uint32_t sacrifice=0); //004116d0명령을현재본체로실행한다.true는경로도착이아닌본체존재/호출을뜻한다.
    bool MoveAiToPoint(std::uint32_t owner,o::Sid actor,o::RawPathPoint point,std::uint32_t sacrifice=0); //004117e0의현재권한별실제송신/경로시작이다.
    bool ReturnAiHome(std::uint32_t owner,o::Sid actor); //004118a0의거리2경계/현재신전복귀다.전체행동순서는후속이다.
    bool CollectAiOrReturnHome(std::uint32_t owner,o::Sid actor); //00411900의실제내용물/그래프별간헐천선택또는신전복귀다.true는본체존재/호출이며도착성공이아니다.
    bool RecheckAiGeysers(std::uint32_t owner); //00411350의현재330/348간헐천재분류다.전체자동AI순서는후속이다.
    bool ManageAiCarriers(std::uint32_t owner); //00412130전체수집/포획행동이다.true는현재본체존재/호출이며포획성공이아니다.
    bool StartAiBuildSearch(std::uint32_t owner,std::uint32_t type); //0040f660을현재자기경계/적기준좌표로실행한다.원본14af0준비뒤사용한다.
    bool AdvanceAiBuildSearch(std::uint32_t owner,std::uint32_t budgeted); //0040f700의같은574상태/벽시각/실제배치검사를진행한다.0은전체행,비영은행시작예산이다.
    std::uint32_t SelectAiBuildType(std::uint32_t owner) const; //00411b40의실제다음건설TYPE다.본체없음/선택없음은0이다.
    bool AiEnemyLowFunds(std::uint32_t owner) const; //00411ad0의현재적자금1000미만조건이다.본체없음은false다.
    bool ManageAiPriest(std::uint32_t owner,o::Sid priest); //00411cd0전체사제행동을같은실제하위로호출한다.true는본체존재/호출이며건설성공이아니다.
    std::int32_t ScoreAiTarget(std::uint32_t owner,o::Sid target) const; //00412770의 실제 현재 표적 점수다.본체가 없으면0이다.
    std::int32_t ScoreAiGun(std::uint32_t owner,std::uint32_t type,float x,float y,std::int32_t direction,o::Sid& result) const; //004128f0:같은 Gun 검색/출력과 점수다.본체 부재는 출력을 보존한다.
    std::int32_t ScoreAiBlockers(std::uint32_t owner,std::uint32_t type,float x,float y,std::int32_t direction,o::Sid& result) const; //00412950:같은 실제 목록의 감김 총점과 원본 첫SID 출력이다.
    std::int32_t ScoreAiDrop(std::uint32_t owner,std::uint32_t type,float x,float y,std::int32_t direction,o::Sid& target) const; //00412ab0:현재시설별실제에너지/블로커/지상/Gun평가다.본체부재는출력을보존한다.
    bool ConsiderAiDrop(std::uint32_t owner,std::uint32_t type,float x,float y,std::int32_t direction,o::RawPathPoint& bestPoint,std::int32_t& bestScore,o::Sid& target) const; //00412b90:현재배치/양수동률/실제에너지승인과부족좌표기록이다.배치실행성공과구분한다.
    bool AdvanceAiDropScan(std::uint32_t owner,std::uint32_t type,std::int32_t direction); //00412e10의같은3e8/af8·afc/실제지도·공용난수스캔다.true는미완료이며false는완료또는본체없음이다.
    bool RemoveAiDropChoice(std::uint32_t owner,std::int32_t index); //0040f390의같은30후보목록압축이다.true는본체존재/호출이며배치성공이아니다.
    bool SelectAiDropType(std::uint32_t owner); //00413100의실제기술순환/회전/직전후보재사용·기한선택이다.true는본체존재/호출이며TYPE비영을뜻하지않는다.
    bool ManageAiDrops(std::uint32_t owner); //004162e0 전체 배치 행동이다.true는 본체 존재/호출이며 실제 배치/성공표적이 생겼다는 뜻이 아니다.자동프레임은 후속이다.
    std::string ObjectName(o::Sid parent) const; //실제가상68의현재번역/치환·Factory레벨접미사다.
    // 같은 현재 Player의 실제 목록 소멸→본체 반납→포인터0을 수행한다. flags20은 원본대로 보존한다.
    void RemoveAi(std::uint32_t owner);
    // 후속 실제 전략/검사가 같은 본체/목록 소유 저장소를 확인한다. 여기서 메모리를 합성하지 않는다.
    const o::RawAiBrainMemory& AiMemory() const;
    // 원본 로그인 종료의 live/포획 사제 조회다. 현재 같은 풀/목록을 읽으며 상태를 갱신하지 않는다.
    std::uint32_t FindPriestParticipation(std::uint32_t owner) const;
    // 로그인 종료 타이머가 명시 요청할 때만 기존 ownerState를 갱신한다. Load/프레임 자동 갱신은 없다.
    void RefreshPriestParticipation(std::uint32_t owner);
    // 같은 현재 사제 목록/ownerState/풀에 연결된 참여 몸체를 읽기 전용으로 빌린다.
    const o::RawPriestParticipation& PriestParticipation() const;
    // 같은 실제 현재 모드/방향 동맹/Graph/섬을 읽어 전체 MayPlace를 실행한다.
    bool MayPlaceConstruction(const o::CanonPlacementQuery& query);
    // 원본 전체 서버 요청의 공간 검사/밀어내기/받침/확정을 실행한다.
    bool RequestConstruction(const o::ConstructionConfirmRequest& request);
    // 같은 실제0x3e 수신을 기존 RawPacketDispatch 또는 LocalMissionTransport.Dispatch에 등록한다.
    void RegisterConstructionCommands(o::RawPacketDispatch& dispatch);
    // 실제44c420의 현재 제단/금지 관문을 거쳐54h만 요청한다. 환불/삭제는 등록된 서버 수신에서 실행한다.
    void RequestSalvage(std::uint32_t sid);
    // 같은 실제58h 수신과 현재 입력의signedWORD 지식 선택을 연결한다.
    void RegisterFactoryResearchCommands(o::RawPacketDispatch& dispatch);
    std::uint32_t SelectFactoryKnowledge(o::Sid parent,std::uint32_t code,std::uint32_t parameter); // 실제 생성·예측·송신 몸체다.
    std::uint32_t CommandFactory(o::Sid parent,std::uint32_t code,std::uint32_t parameter); //004547d0의상태순환/업그레이드관문/연구/공통부모입력이다.
    void RequestFactoryUpgrade(o::Sid parent); //00453bf0의 실제 대기/58h 송신이다.사용자메뉴는CommandFactory의자금/안내관문을사용한다.
    bool FactoryUpgrading(o::Sid parent) const; //00453c90의실제예측그림→event4→event3조회다.
    // 같은 실제0x4f 일반포획/0x5b 채취/0x60 크랩포획 수신을현재캠페인에등록한다.
    void RegisterCarrierCommands(o::RawPacketDispatch& dispatch);
    // 실제427340은 선택 해제/연결·중복 검사 뒤40h만 송신한다. 경로 생성은 등록 수신에서 수행한다.
    void RequestCarrierMove(o::Sid parent,o::RawPathPoint destination,std::uint32_t sacrifice=0);
    void RequestCarrierDrop(o::Sid parent,o::Sid content={}); // 실제427070의비용/선택/9바이트송신만 실행하며 서버 수신을 직접 부르지 않는다.
    std::uint32_t CommandCarrierDrop(o::Sid parent,std::uint32_t command,o::Sid listParent); // 실제42aa50 Cast/Drop 메뉴와 최초 내용물 선택 순서를 실행한다.
    std::uint32_t CommandObject(o::Sid parent,std::uint32_t command); // 실제 비석/주문6c 전환과4ae010/4ac9b0 공통 부모 명령·미처리0 반환이다.
    void SetObjectOwner(o::Sid parent,std::uint32_t owner); // 실제 가상74의 신전·사제 재정의와 공통 소유자 변경을 같은 월드에서 분배한다.
    std::uint32_t CommandPriest(o::Sid parent,std::uint32_t command,std::uint32_t parameter); // 실제4940b0의packed y 기도 요청과 Carrier 부모 위임이다.
    // 같은 실제40h 요청/수신 처리기를 읽기 전용으로 빌린다.
    const o::RawCarrierMove& CarrierMove() const;
    // 원본 MayPlace를 통과한 일반 서버 요청의 기준점/송신/직접 통지/실제 배치를 실행한다.
    bool ConfirmConstruction(const o::ConstructionConfirmRequest& request);
    // 설치 뒤 로컬 간헐천의 noIsland/Player0 받침만 실제 서버 확정 사슬로 만든다.
    bool ConfirmSupport(float x,float y);
    // 원본 미션 부모가 정한 위치에서 명시 호출한다. Load/프레임의 자동 생성은 하지 않는다.
    void GenerateLocalGeysers();
    //00486440 전체 부모를 같은 실제 지도/Player/Pool/건설과 장기 전역으로 실행한다.
    void GenerateWorldGeysers();
    void SetAllBridgeQuality(std::uint32_t quality); //실제 Editor 또는 missionPost에서 전체 다리의 Cracked/Normal/Hard 원문 명령을 실행한다.
    void SetBridgeQuality(std::uint32_t sid,std::uint32_t quality); //일반 입력의 Editor/missionPost 뒤 실제421f20의 Editor/TYPE 관문을 유지해 개별 다리를 바꾼다.
    // 같은 실제 Pool/현재 Bounds/최근목록/자원/마지막 패킷을 읽는 연결부다.
    const RawCampaignConstructionBindings& Construction() const;
    // 같은 Player+68과 현재 Factory 목록을 사용하는 실제 생산 계산기를 빌린다. 조회만으로 재계산하지 않는다.
    const RawCampaignProductionBindings& Production() const;
    void SnapshotFortTypeNames(std::uint32_t slot); // 같은 실제 State/TYPE에서004bd6f0을 명시 실행한다. 전체 월드 snapshot/파일 저장은 별도다.
    void SnapshotFortDeck(std::uint32_t slot); // 같은 실제 생산 재계산/덱에서004befd0을 명시 실행한다. 기존 Fort의 읽기 사본을 쓰지 않는다.
    void SnapshotFortTechnology(std::uint32_t slot); // 현재 Player 기술 부모와 같은 Form6 체인을004bcea0 순서로 새 Technology에 저장한다.
    void SnapshotFortTerritory(std::uint32_t slot); // 기존120BYTE와 현재 섬/지도/공유 난수에서004bf6d0을 명시 실행한다.
    void SnapshotFortObjects(std::uint32_t slot); // 현재 월드의 Terr00~19와 조건부 Chaff를 실제 Temp/rename·청크 writer로 저장한다. 전체 snapshot/Notify는 별도다.
    void SnapshotFort(std::uint32_t slot); // 설치된 명시 Fort 모드에서 원본 전체 부모와 마지막 Notify를 실행하며 모드를 자동 변경하지 않는다.
    void SnapshotBattle(std::uint32_t slot); //실제004bf880의 기술/Deck/Objects/Territory/Money/Mission/CoreData/Notify를 같은 저장소로 실행한다.
    bool BuildEditorIslandMap(o::ChunkMap& target); // 0046e4a0의 독립14×14 지도 부분이다. Bitmap 생성과 전체 GUI 완료를 뜻하지 않는다.
    std::optional<o::TerrainBitmapImage> BuildEditorIslandMapImage(o::ChunkMap& target); // 같은 실제 지도와004c1cc0 Bitmap을 만들고 manager 부재는 nullopt로 구별한다.
    o::TerrainBitmapImage BuildEditorIslandChoiceImage(std::span<const std::uint8_t,6> record); // 원문 owner0 단일 지도/로컬 난수/보호 texture의 실제 목록 이미지를 만든다.
    std::optional<RawImage> BuildEditorIslandMapShape(o::ChunkMap& target); //0047f9b0의 현재 지도는 crop0으로 변환하고 bitmap 소유권을 소비한다.
    RawImage BuildEditorIslandChoiceShape(std::span<const std::uint8_t,6> record); // 같은 후보 bitmap을 crop1 Image로 바꾸어 전용 목록/드래그의 실제 크기를 제공한다.
    bool SeedEditorIslandChoices(std::span<std::uint8_t,126> records); // 0047f9b0의 창 전용21행에서 비활성 레코드만 같은 공유 난수로 보충한다.
    void BuildEditorIslandChoiceMap(std::span<const std::uint8_t,6> record,o::ChunkMap& target); // 0046e5a0의 owner0/ignorePosition 단일 지도 부분이다.
    bool DropEditorIsland(const o::ChunkMap& preview,std::span<std::uint8_t,6> record); // 실제 섬 배치→지형→population→Graph→현재 Territory 쓰기를 실행한다.
    bool CanPlaceEditorIsland(const o::ChunkMap& preview,std::span<const std::uint8_t,6> record); //0047d5f0의 같은 독립 preview/현재 TYPE·owner 검사이며 월드/record/난수를 변경하지 않는다.
    bool EditorIslandManagerAvailable() const; //0047f9b0의 현재 owner/manager 관문이며 dirty/이전 Image를 바꾸기 전에 조회한다.
    void SaveCurrentMap(); //004bfb50의 현재 owner/manager 존재/독립 Battle·Fort 선택으로 전체 저장을 실행한다.
    void BindFortFileSave(const std::uint32_t& readOnlyForts,const EditorFiles& files); // 두 참조는 Session보다 오래 살아야 하며 현재 manager 이름은 이미 준비한 작업본 경로여야 한다.
    o::FortCurrentNameHooks BindFortCurrentName(o::FortCurrentNameHooks hooks); // 같은 실제 Template/State/Notify와 독립 Battle/Fort snapshot을 연결한다.
    o::FortObjectLoadState& FortObjectSettings(); // 로드/저장 양쪽의 같은 현재 판본과 범위를 빌린다.
    void WriteFortObjectChunk(TemplateWriteStream stream,std::int32_t chunkX,std::int32_t chunkY); // 같은 저장소의 명시 stream에 한 청크만 저장한다. 전체 지역 부모는 별도다.
    // 생산/튜토리얼/건설 표시가 함께 읽는 금지DWORD 표와 실제 세 카운터를 직접 조작한다.
    o::RawTotalMade& TotalMade();
    // 실제490250으로 명시 Player 부모를 생성한다. 로그인/참여 부모가 요청할 때만 호출하며 일반 확정에서 자동 생성하지 않는다.
    o::Sid CreatePlayer(std::int32_t owner);
    // 실제 시작 설정/AI와 같은 Player 기술 부모에 원본 목록 파서를 적용한다.
    void AddTechnology(std::int32_t owner,std::string_view list);
    void LearnNewTechnology(std::uint32_t type); //00492a30의 직접 현재 부모·중복/추가·전체 snapshot·이름/그림/NewTech를 실행한다. 포획/희생을 합성하지 않는다.
    void NotifyPlayerResults(); //00490da0의 프레임말 대기 결과를 같은 Player/Core/내용물·기존 보상/기록 부모에 전달한다.
    // 실제 flags4 기술을 가상 삭제하여 부모 체인/등록/삭제 깊이를 함께 정리한다.
    void ClearTechnology(std::int32_t owner);
    // 현재 같은 Player 부모에서 지정한 실제 기술 ContentForm을 조회한다.
    o::Sid FindTechnology(std::int32_t owner,std::int32_t type) const;
    // 같은 실제 Form6 생성·정렬 부착·부모 통지를 실행한다. 원본 인자는 RawContainedCreate::Create와 같고 설치 뒤에만 허용한다.
    o::Sid CreateContained(o::Sid parent,std::uint32_t type,std::uint32_t argument,std::uint32_t flags,std::uint32_t attachFlags,std::uint32_t quality);
    // 실제 사제의 같은 Start/BFS/Host54 경로를 시작한다.
    bool MovePriest(o::Sid sid,o::RawPathPoint destination);
    // 같은 raw 신전의 가상 Draw를 실행하고 실제 SoundList/재생 경계로 상승음을 보낸다.
    void DrawRainTemple(o::Sid sid,const std::function<void(std::int32_t)>& frame) const;
    void DrawTemple(o::Sid sid,const std::function<void(std::int32_t)>& frame) const; // 실제 세 템플의+a4만 분배한다. 기본 그림은 SpriteDraw의+78이다.
    void DrawBridgeAdditional(o::Sid sid,std::int32_t x,std::int32_t y) const; // 실제Session의수명숫자전역을읽으며선택용캡처에별도기본0사본을만들지않는다.
    // 같은 Pool/Host/기술/허용/SP/실제 ContentForm 생성에 연결한 메뉴다. UI·번역·Tell·커서 취득만 caller가 공급한다.
    // 반환 객체는 Session보다 먼저 해제한다. 설치 전에는 메뉴/내용물 생성을 허용하지 않는다.
    std::unique_ptr<RawPriestMenu> MakePriestMenu(PriestMenuHooks hooks);
    // 현재 Player 기술/실제 내용물·제외·Deck으로 전체 지식 창을 작성한다. GUI는 같은 Client의 실제 처리기를 받는다.
    std::vector<std::uint32_t> ShowKnowledge(KnowledgeMenuState& state,KnowledgeMenuHooks hooks);
    QuickHelpTypeInfo HelpTypeInfo(std::uint32_t type) const; // 같은 TYPE/무기 보충 표/실제 AST 후처리의 현재 도움말 값이다.
    // caller는 기존 Bind*MenuControls로 같은 Gump 작성기를 공급한다. 미공급 Chat/번역/통신 분기는 선택 시 오류다.
    // 반환 체인은 메뉴가 열린 동안 유지하고 월드 교체/Session 해제 전에 반납한다.
    std::unique_ptr<RawCampaignObjectMenus> MakeObjectMenus(CampaignObjectMenuUi ui);
    // 공통Open의제단/투영/내용물/사정거리를같은현재월드에연결한다. GUI·실제가상분배는caller가공급한다.
    // 반환몸체와모든GUI참조는Session보다먼저해제한다. 제목/파생메뉴의미연결은원래오류를유지한다.
    std::unique_ptr<RawObjectMenuOpen> MakeObjectMenuOpen(ObjectMenuOpenHooks hooks);
    // 실제 선택 비트/표시/Host/마나·사정거리/내용물/제단/491생성을 같은 Session에 연결한다.
    // 현재 UI 관문·강조·진단 dump는 caller가 공급하며 반환 몸체는 Session보다 먼저 해제한다.
    std::unique_ptr<RawUserSelection> MakeUserSelection(UserSelectionState state,UserSelectionHooks hooks);
    // 같은 현재 커서/실제 요청/포획/후보/음향 장부를 사용자입력 장치에 연결한다.
    std::unique_ptr<RawCarrierInput> MakeCarrierInput(CarrierInputHooks hooks);
    // 자동 입력 보고에서 현재 선택/역투영 점의 실제 Graph·spot·간헐천 자격만 읽는다. 평가/명령은 실행하지 않는다.
    std::string ReportCarrierHover(std::uint32_t selected,float x,float y) const;
    // 같은풀/공통삭제/혼합종속분배에등록한실제GumpFormHost다. UI는월드반납전에연결메뉴를닫아야한다.
    o::GumpFormHost& GumpForms();
    // 현재 타입/명령·특수 모드의 장기 저장소다. 알려진 공통 전역은 실제 getter로 연결한다.
    PriestMenuState& PriestMenuSettings();
    // 0052f7e0의 TYPE 번호 제외 목록이다. 공간 앵커의 SID 추가 목록과 별개로 소유한다.
    o::SquidPostPopList& ConstructionExcludedTypes();
    // 실제 Client 커서가 같은 취득/내려놓기/프로세스 토큰을 공유하는 단일 상태다.
    CursorDropState& CursorDropSettings();
    // 설치 뒤 실제 수명/위치/미리보기/내려놓기/로컬 예측을 소유한 단일 커서를 빌린다.
    bool HasCursorRuntimeForState() const; // loaded성공과구별하여실제C++커서실행기존재만조회한다.
    void CancelCursorForState(); // 실제실행기가있을때외곽game.prepare없이00446e70만실행하며미설치는거부한다.
    RawCampaignCursor& Cursor();
    // 현재 맵 경계·마나 반경/대체점과 타입 전역을 같은 프로세스 몸체에 공급한다.
    CursorManaGlobals& CursorManaSettings();
    // 실제 Session의 마나 월드 객체와 Host/Kernel 생성·제거·이벤트를 연결한다. 설치 후에만 허용한다.
    void BindCursorMana(CursorLifecycleHooks& hooks);
    // 같은 좌표/총 전역을 빌리는 현재 사정거리 종류/프로세스 전역이다.
    CursorRangeGlobals& CursorRangeSettings();
    // 실제 마나·사정거리 생성과 공통 제거/이벤트를 같은 Host/Kernel에 연결한다.
    void BindCursorRange(CursorLifecycleHooks& hooks);
    // 현재 공급원/비대칭 동맹/TYPE mana/마지막 선택 목록으로 계산하는 실제 몸체다.
    const o::RawEnergySupply& EnergySupply() const;
    const o::RawDudeBehavior& DudeBehavior() const; // 현재 동일 지도/목록/task를 쓰는 실제 Dude 부모 몸체다.
    const o::RawDudeNotice& Dudes() const; //피격과 같은 현재24 조회·대피 경로·가변 속도를 쓰는 실제 통지 몸체다.
    // 사정거리 표시/자동 입력이 실제 GunProcess와 같은 현재 원본 표적 판정을 읽는다. 목표나 게임 상태를 바꾸지 않는다.
    const o::RawGunTarget& GunTargeting() const;
    o::EnergySupplyState& EnergySettings(); // 실제 공급 제곱 반경·userCheat/ignore/동맹 전역이다.
    o::SquidPostPopList& EnergySources(); // 이후 Builder도 같은 lastSource30 저장소를 빌린다.
    // 실제4734d0의 전체 DWORD와 문구를 커서 Query에 연결한다. GUI/공간 판정은 유지한다.
    void BindCursorEnergy(CursorPlacementHooks& hooks);
    // 실제 기준점/Factory/Owner/Pop/TYPE/현재 전역/Regular 및 같은 에너지 목록을 연결한다.
    // 접근점·경로 시작·속도 적용은 caller가 실제 몸체로 공급하며 반환 객체/State는 Session보다 먼저 해제한다.
    std::unique_ptr<o::RawConstructionBuilder> MakeConstructionBuilder(o::ConstructionBuilderState& state,o::ConstructionBuilderHooks hooks);
    o::RawPathProcess* StartConstructionMovement(o::RawPathStartRequest request); // 같은 월드의 실제134/105를 각각23/39 경로로 움직인다.
    o::RawPathProcess* StartCarrierMovement(o::RawPathStartRequest request); // 같은 실제 썬 워커133의40 경로를 시작한다.다른Halt는 별도 경계다.
    o::RawPathProcess* StartDudeMovement(o::RawPathStartRequest request); //같은 실제 Dude70의24 경로와 현재 속도·부모 효과를 연결한다.
    void RequestCarrierPickup(o::Sid parent); //같은실제후보/헤더/서버9 전송을요청하며설치후에만허용한다.
    const o::RawGemReward& GemReward() const; //같은실제오벨리스크보상추첨과변환몸체를빌린다.
    const o::RawCarrierPickup& CarrierPickup() const; //실제4f 요청/수신/기존내용물삭제몸체를빌린다.
    const o::RawCarrierSteal& CarrierSteal() const; // 같은 실제 포획/로컬 수신/방향 동맹을 빌린다.
    const o::RawCarrierTransfer& CarrierTransfer() const; // 현재 같은 풀의 로컬 전달 수신/주기/공유 상태를 빌린다.
    const o::RawCarrierTarget& CarrierTarget() const; // 현재 같은 월드의 일반 대상 접근/간헐천 이동/귀환을 실행한다.
    std::uint32_t EscapeCarrier(o::Sid parent,float distance,std::uint32_t event); // 같은 현재 경로/난수/공간에서 실제429010 회피를 실행한다.
    const o::RawCarrierChoice& CarrierChoice() const; // 현재 실제 Player/목록/Graph/BFS의 귀환·간헐천 선택기를 빌린다.
    // 화면/명령은 같은 현재 raw 월드와 HP를 읽는다.
    RawSurfaceWorld& World();
    // 같은 월드의 현재 Pool/장부/Host 참조를 빌린다.
    RawWorldCore Core();
    // 화면의 현재 HP/최대 HP 조회는 실제 SquidReward를 사용한다.
    const o::SquidReward& HitPoints() const;
    // 실제 사제 목록/움직임/보호막 상태의 읽기 경계를 빌린다.
    const RawPriestWorldBindings& Priests() const;
private:
    void SnapshotFortBasicSection(std::uint32_t slot,bool names); // 실제 manager/State/덱과 같은 생성 수명의 쓰기 참조를 묶는다.
    void SnapshotFortBody(std::uint32_t slot); // 실제 월드 설치 뒤의 Editor 외곽만 loaded 이전에 호출할 수 있는 전체 저장 본문이다.
    void SnapshotTemplateBody(std::uint32_t slot,bool battle); //같은 실제 writer를 빌리되 원본 Battle/Fort 부모의 서로 다른 순서/진단을 유지한다.
    void WriteFortCoreDataBody(std::uint32_t slot); //설치 외곽과 실제 Battle이 같은004beb30의 자연 missionPost/editor 관문을 사용한다.
    void WriteFortTerritory(std::uint32_t slot); // 현재 지형의 같은 저장 본문이며 공개 호출의 설치 검사는 별도다.
    void WriteFortObjects(std::uint32_t slot); // 전체 지역/Chaff 저장 본문을 설치 외곽에서도 같은 순서로 실행한다.
    void WriteFortObjectChunkBody(TemplateWriteStream stream,std::int32_t chunkX,std::int32_t chunkY); // 같은 청크 저장 본문이며 공개 설치 계약을 우회 노출하지 않는다.
    void WriteFortTechnology(std::uint32_t parent,TemplateWriteStream stream); // 부모가 이미 만든 같은 stream에 body-token1로 기술 본문만 쓴다.
    void BeginLocalConnection(bool parentRunning); // 새 로그인 소유자를 한 번 만들며 실제 외부 부모/독립 편의 실행의 경계만 구별한다.
    struct Impl; // 단일 월드의 실제 모듈/현재 배열과 외부 서비스의 고정 수명 소유자다.
    std::unique_ptr<Impl> impl_; // Kernel 해제 뒤 실제 상태/서비스를 해제한다.
};
}
