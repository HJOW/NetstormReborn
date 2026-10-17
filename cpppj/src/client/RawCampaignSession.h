// 실제 캠페인 초기화/전투/이동의 상태와 모듈 수명을 클라이언트 세션 하나에 보관한다.
#pragma once
#include "client/GameAssets.h"
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
#include "o/RawBattleBroadcast.h"
#include "o/RawMissionOutcome.h"
#include "o/RawAiBrain.h"
#include "o/SpStore.h"
#include "o/RawPriestParticipation.h"
#include "o/RawDamageablePostDestroy.h"
#include "client/CameraHome.h"
#include "client/RawPriestMenu.h"
#include "client/RawCarrierMenu.h"
#include "client/RawObjectMenuOpen.h"
#include "client/RawObjectMenuInfo.h"
#include "client/RawUserSelection.h"
#include "client/RawPlayerMenu.h"
#include "o/RawCarrierContentNotice.h"
#include "o/RawCarrierTransfer.h"
#include "o/RawCarrierTarget.h"
#include "o/RawCarrierChoice.h"
#include "client/RawCursorMana.h"
#include "client/RawCursorRange.h"
#include "o/RawEnergySupply.h"
#include "o/RawConstructionBuilder.h"
#include "client/RawCampaignParticles.h"
namespace netstorm::client {
class RawCampaignCursor; // 세션과 같은 실제 커서/로컬 예측의 고정 주소 소유자다.
struct CursorPlacementHooks; // 실제 커서 배치의 설명/에너지 경계만 연결할 때 사용한다.
// 실제 Client의 메뉴 작성·번역·Chat·정적 문자열 수명을 공급한다. 게임 계산 훅은 factory가 같은 세션으로 교체한다.
struct CampaignObjectMenuUi {
    std::span<const std::uint8_t> chat,meta; // 현재 Client Chat100×48과 선택적 metamap40×460 원문이며 복사하지 않는다.
    o::PlayerIdentityReads identity; // 주소/실패 이름의 실제 현재 getter다. 현재 이름/subscriber/모드는 같은 세션이 연결한다.
    ObjectMenuState objectState; // 현재 도움말 문자열/회수 금지·명령 전역이다. owner/모드는 세션 getter로 교체한다.
    PlayerMenuState playerState; // 실제 asksend 정적 버퍼와 명령 토큰이며 반환 체인이 수명을 소유한다.
    PriestMenuHooks priest; // 실제 GUI/번역/Tell이다. 게임 조회·생성·커서 취득은 세션이 공급한다.
    CarrierMenuHooks carrier; // 실제 GUI/번역/타입 설명이다. 지면·이동·위치·SP는 세션이 공급한다.
    ObjectMenuHooks object; // 실제 GUI/서식/설정/문자열 토큰이다. 두 미복원 파생 가상80만 count 경계를 사용한다.
    PlayerMenuHooks player; // 실제 Chat 상태/통신/허용 전역/GUI다. 현재 미션·요새·SP는 세션이 공급한다.
    ObjectMenuInfoHooks info; // 실제제목/머리·번역·순수서식·Config치환이다. 게임/Identity조회는같은Session이교체한다.
};
// 사제→Carrier→Damageable→공통 객체→Player의 실제 몸체를 한 수명으로 소유한다.
// 같은 세션/Client/Gump 트리는 이 객체보다 오래 살아야 하며 다음 메뉴 생성 전 이전 메뉴를 닫아야 한다.
class RawCampaignObjectMenus {
public:
    ~RawCampaignObjectMenus(); // 참조 대상인 부모 모듈보다 먼저 자식 메뉴 몸체를 해제한다.
    RawCampaignObjectMenus(const RawCampaignObjectMenus&)=delete; // 훅의 고정 주소와 정적 문자열 수명을 보존한다.
    RawCampaignObjectMenus& operator=(const RawCampaignObjectMenus&)=delete; // 서로 다른 세션 체인의 대입을 금지한다.
    void OpenPriest(o::Sid sid) const; // 루트 메뉴 한 칸을 caller가 만든 뒤 실제 사제 Open을 작성한다.
    void OpenCarrier(o::Sid sid) const; // 원본 가상 메뉴가 Carrier일 때만 같은 전체 부모 체인을 작성한다.
    void OpenDamageable(o::Sid sid) const; // 원본 가상 메뉴가 Damageable일 때 실제 회수/공통 체인을 작성한다.
    void OpenObject(o::Sid sid) const; // 원본 기본 객체 메뉴를 작성한다. 파생 메뉴를 임의로 이 함수로 대체하지 않는다.
    void Summary(o::Sid sid) const; // 현재객체의실제4인자정보/extra/포획표시를같은메뉴에작성한다.
    void Describe(std::uint32_t type,std::uint32_t count,std::uint32_t owner,std::uint32_t sid) const; // 원본4ad780의4인자를직접보존한다.
    bool SelectBuild(std::uint32_t type,o::Sid priest) const; // 같은 Open의 현재 기술/SP 관문과 실제 세션 커서 취득을 실행한다.
    std::string_view TextParameter(std::uint32_t token) const; // Player의 현재 정적 asksend 토큰만 해석한다.
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
    o::RawAiBrainMemory::Fill allocationBytes; // 실제 host 할당 직후의 입력 바이트다. 누락을 임의 0채움 성공으로 바꾸지 않는다.
    std::function<std::string()> defaultName; // 현재 defaultAiName 설정의 원본 좁은문자열을 반환한다.
    std::function<void(std::uint32_t,std::uint32_t)> changeColor; // 실제0043ca70의 표/섬 프레임 변경을 마친 뒤 보내는 선택형 화면 통지다.
    o::PlayerParticipationHooks participation; // 실제 Chat/동맹 송신/연결·진단이다. brain 소멸/반납은 Session의 실제 소유자로 교체한다.
    std::function<bool()> localTransport; // CheckLogin이 현재 로컬 전송 모드를 읽는 실제 경계다.
};
// 원본 Deck의 할당 입력과 현재 설정/진단을 실제 클라이언트에 연결한다.
struct CampaignProductionServices {
    o::RawProductionDeck::Fill allocationBytes; // 실제 할당 직후 바이트이며 생성자가 보존하는 꼬리를 임의 초기화하지 않는다.
    CampaignProductionEffects effects; // 현재 getTeleportTechFree와 원본 모드 진단이다.
};
// 게임 계산을 대신하지 않고 실제 클라이언트 UI/음향/전송의 선택 경계를 공급한다.
struct RawCampaignSessionServices {
    std::function<std::int32_t()> unixSeconds; // 실제 time32의 현재 Unix 초다.
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
    SoundProcessHooks sound; // 실제 같은 SoundPlayer/벽시계에 연결한 재생·정지 경계다.
    std::function<void(std::span<const std::uint8_t>)> pathPacket; // 실제 Path 직렬화 버퍼의 송신 소비자다.
    std::function<void(std::span<const std::uint8_t>)> formPacket; // 실제 감시/내용물/삭제 직렬화 후의 송신 경계다. 없으면 동일 BattleBroadcast를 사용한다.
    o::BattlePacketPeers battlePeers; // 실제0..8 연결 객체 표다. 단독 캠페인은 NULL 연결이며 별도 송신 관찰 함수가 필요하지 않다.
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
    // 실제443fe0을 같은 시각/Host/표시/음향으로 실행한다. duration0은 원본 기본10초다.
    void StartConstruction(o::Sid parent,float duration);
    // 현재 같은 풀/TYPE/실제 로컬 소유자·모드로 원본 초기 시야를 계산하여 화면에 적용한다.
    void ResetCamera(CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> apply) const;
    // 현재 같은 시각을 모든 모듈에 공급한 뒤 동일 Kernel을 실행한다.
    void RunFrame(o::FrameTime time,bool paused);
    // 같은 실제 카메라와 현재 장치의 dirty 경계를 process11에 공급한다.
    void SetParticleView(SoundView view,SurfaceDisplayHooks display);
    // 실제 원본 표시 목록의 현재 직접 그리기를 다른 월드 SHP와 함께 정렬한다.
    std::vector<RenderSprite> ParticleSprites(ScreenRect clip) const;
    // 후속 검증이 같은 Host/Kernel의 등록을 읽는 현재 표시 관리자다.
    const RawCampaignParticles& Particles() const;
    // 원본 미션 게이트의 첫 checkOutcome 위치에서만 호출한다. 설치/활성화 전에 자동 실행하지 않는다.
    void CheckMissionOutcome();
    // 원본 로그인/월드 로더가 공유하는 현재 globals.mySubscriber를 읽기만 한다. 별도 캐시를 만들지 않는다.
    std::uint32_t CurrentSubscriber() const;
    // 같은 실제 PlayerWorld의00540c70을 읽는다. 자산/UI 어댑터의 로컬 번호 사본을 사용하지 않는다.
    std::uint32_t LocalOwner() const;
    // 실제 PlayerWorld가 공유하는00595344 DWORD를 읽는다. subscriber/Ready와 합치지 않는다.
    std::uint32_t PlayerKnown() const;
    // 실제 캠페인 로더와 같은005c85a4 DWORD를 읽는다.
    std::uint32_t Editor() const;
    // 사제/Player 내용물/건설/보상이 공유하는 실제 분산 SP를0040ec50의 float비트로 읽는다.
    float Sp(std::uint32_t owner) const;
    // 원본0040edb0처럼 직접 float비트만 쓴다. AddSp의 지갑/화면 갱신을 대신 호출하지 않는다.
    void SetSp(std::uint32_t owner,float value);
    // 같은 실제32×9 원문 배열/offset/추적주소를 읽기만 한다. 별도 SP 저장소를 생성하지 않는다.
    const o::ScrambledSpStore& SpStore() const;
    // 실제 로그인/AI 초기화가 사용하는 동일9 Player 원문을 빌린다. 프레임별 사본을 만들지 않는다.
    std::span<std::uint8_t> PlayerRecords();
    // 실제 방향별동맹 초기화가 쓰고 승패가 읽는 같은81DWORD를 빌린다.
    std::span<std::uint32_t> Alliances();
    // 로그인/Stage3의 후속 부모가 같은 현재 Player9/동맹/brain 수명을 직접 사용한다.
    o::RawPlayerParticipation& Participation();
    // 같은 현재 채팅 숨김 BYTE9 저장소다. 방향별 동맹표나 참여 flag와 합치지 않는다.
    std::span<std::uint8_t> HiddenPlayers();
    // 설치 뒤 실제 생성자→현재 Player 부착→초기 갱신을 실행한다. 전략 프레임/자동 승패는 켜지 않는다.
    std::uint32_t CreateAi(std::uint32_t owner,std::span<const std::uint8_t> settings);
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
    // 같은 실제0x5b 채취 수신을 현재 캠페인의 수량/고갈 처리기에 등록한다.
    void RegisterCarrierCommands(o::RawPacketDispatch& dispatch);
    // 원본 MayPlace를 통과한 일반 서버 요청의 기준점/송신/직접 통지/실제 배치를 실행한다.
    bool ConfirmConstruction(const o::ConstructionConfirmRequest& request);
    // 설치 뒤 로컬 간헐천의 noIsland/Player0 받침만 실제 서버 확정 사슬로 만든다.
    bool ConfirmSupport(float x,float y);
    // 원본 미션 부모가 정한 위치에서 명시 호출한다. Load/프레임의 자동 생성은 하지 않는다.
    void GenerateLocalGeysers();
    // 같은 실제 Pool/현재 Bounds/최근목록/자원/마지막 패킷을 읽는 연결부다.
    const RawCampaignConstructionBindings& Construction() const;
    // 같은 Player+68과 현재 Factory 목록을 사용하는 실제 생산 계산기를 빌린다. 조회만으로 재계산하지 않는다.
    const RawCampaignProductionBindings& Production() const;
    // 생산/튜토리얼/건설 표시가 함께 읽는 금지DWORD 표와 실제 세 카운터를 직접 조작한다.
    o::RawTotalMade& TotalMade();
    // 실제490250으로 명시 Player 부모를 생성한다. 로그인/참여 부모가 요청할 때만 호출하며 일반 확정에서 자동 생성하지 않는다.
    o::Sid CreatePlayer(std::int32_t owner);
    // 실제 시작 설정/AI와 같은 Player 기술 부모에 원본 목록 파서를 적용한다.
    void AddTechnology(std::int32_t owner,std::string_view list);
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
    // 같은 Pool/Host/기술/허용/SP/실제 ContentForm 생성에 연결한 메뉴다. UI·번역·Tell·커서 취득만 caller가 공급한다.
    // 반환 객체는 Session보다 먼저 해제한다. 설치 전에는 메뉴/내용물 생성을 허용하지 않는다.
    std::unique_ptr<RawPriestMenu> MakePriestMenu(PriestMenuHooks hooks);
    // caller는 기존 Bind*MenuControls로 같은 Gump 작성기를 공급한다. 미공급 Chat/번역/통신 분기는 선택 시 오류다.
    // 반환 체인은 메뉴가 열린 동안 유지하고 월드 교체/Session 해제 전에 반납한다.
    std::unique_ptr<RawCampaignObjectMenus> MakeObjectMenus(CampaignObjectMenuUi ui);
    // 공통Open의제단/투영/내용물/사정거리를같은현재월드에연결한다. GUI·실제가상분배는caller가공급한다.
    // 반환몸체와모든GUI참조는Session보다먼저해제한다. 제목/파생메뉴의미연결은원래오류를유지한다.
    std::unique_ptr<RawObjectMenuOpen> MakeObjectMenuOpen(ObjectMenuOpenHooks hooks);
    // 실제 선택 비트/표시/Host/마나·사정거리/내용물/제단/491생성을 같은 Session에 연결한다.
    // 현재 UI 관문·강조·진단 dump는 caller가 공급하며 반환 몸체는 Session보다 먼저 해제한다.
    std::unique_ptr<RawUserSelection> MakeUserSelection(UserSelectionState state,UserSelectionHooks hooks);
    // 같은풀/공통삭제/혼합종속분배에등록한실제GumpFormHost다. UI는월드반납전에연결메뉴를닫아야한다.
    o::GumpFormHost& GumpForms();
    // 현재 타입/명령·특수 모드의 장기 저장소다. 알려진 공통 전역은 실제 getter로 연결한다.
    PriestMenuState& PriestMenuSettings();
    // 0052f7e0의 TYPE 번호 제외 목록이다. 공간 앵커의 SID 추가 목록과 별개로 소유한다.
    o::SquidPostPopList& ConstructionExcludedTypes();
    // 실제 Client 커서가 같은 취득/내려놓기/프로세스 토큰을 공유하는 단일 상태다.
    CursorDropState& CursorDropSettings();
    // 설치 뒤 실제 수명/위치/미리보기/내려놓기/로컬 예측을 소유한 단일 커서를 빌린다.
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
    o::EnergySupplyState& EnergySettings(); // 실제 공급 제곱 반경·userCheat/ignore/동맹 전역이다.
    o::SquidPostPopList& EnergySources(); // 이후 Builder도 같은 lastSource30 저장소를 빌린다.
    // 실제4734d0의 전체 DWORD와 문구를 커서 Query에 연결한다. GUI/공간 판정은 유지한다.
    void BindCursorEnergy(CursorPlacementHooks& hooks);
    // 실제 기준점/Factory/Owner/Pop/TYPE/현재 전역/Regular 및 같은 에너지 목록을 연결한다.
    // 접근점·경로 시작·속도 적용은 caller가 실제 몸체로 공급하며 반환 객체/State는 Session보다 먼저 해제한다.
    std::unique_ptr<o::RawConstructionBuilder> MakeConstructionBuilder(o::ConstructionBuilderState& state,o::ConstructionBuilderHooks hooks);
    o::RawPathProcess* StartConstructionMovement(o::RawPathStartRequest request); // 같은 월드의 실제134/105를 각각23/39 경로로 움직인다.
    o::RawPathProcess* StartCarrierMovement(o::RawPathStartRequest request); // 같은 실제 썬 워커133의40 경로를 시작한다.다른Halt는 별도 경계다.
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
    struct Impl; // 단일 월드의 실제 모듈/현재 배열과 외부 서비스의 고정 수명 소유자다.
    std::unique_ptr<Impl> impl_; // Kernel 해제 뒤 실제 상태/서비스를 해제한다.
};
}
