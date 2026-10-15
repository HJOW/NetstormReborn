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
#include "client/SoundProcess.h"
#include "o/RawPlayerRegular.h"
#include "o/RawBattleBroadcast.h"
#include "o/RawMissionOutcome.h"
#include "o/RawAiBrain.h"
#include "client/CameraHome.h"
namespace netstorm::client {
// 미션 입력/정지/UI 수명은 실제 UberGump의 현재 저장소와 동작을 빌린다.
struct CampaignMissionOutcomeUi {
    std::function<std::uint32_t()> demo; // 현재 미션 Demo DWORD를 읽는다.
    std::function<std::uint32_t()> tellBlocked; // 현재 실제 Tell 차단 DWORD를 읽는다.
    std::function<bool(std::string_view)> section; // 실제 미션/Tell 섹션을 실행하고 발견 여부를 반환한다.
    std::function<void()> closeTransient; // 성공한 섹션 뒤 원본 임시 Gump 정리를 실행한다.
    std::function<void()> onSection; // 성공한 섹션 뒤 현재 미션 가상 통지/정지 소유권을 처리한다.
};
// AI의 게임 계산은 같은 Session이 소유하고 현재 언어/채팅/색/할당 입력만 실제 외부 경계로 받는다.
struct CampaignAiServices {
    o::RawAiBrainMemory::Fill allocationBytes; // 실제 host 할당 직후의 입력 바이트다. 누락을 임의 0채움 성공으로 바꾸지 않는다.
    std::function<std::string()> defaultName; // 현재 defaultAiName 설정의 원본 좁은문자열을 반환한다.
    std::function<void(std::uint32_t,std::uint32_t)> changeColor; // PATCH0043ca70의 실제 색 변경 전체이며 nonzero Color에서만 필요하다.
    o::PlayerParticipationHooks participation; // 실제 Chat/동맹 송신/연결·진단이다. brain 소멸/반납은 Session의 실제 소유자로 교체한다.
    std::function<bool()> localTransport; // CheckLogin이 현재 로컬 전송 모드를 읽는 실제 경계다.
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
    RawCampaignConstructionEffects construction; // 로컬 간헐천의 실제 설정/벽시각/표시/진단이며 일반 건설을 대신하지 않는다.
    o::SquidDestroyHooks deletion; // 다른 파생 삭제/선택/전송의 실제 외부 경계다.
    std::function<void(o::Sid)> unitLost; // 공통 삭제가 요청한 실제 unitLost 전역 소리를 재생한다.
    o::ContentLifetimeHooks contentLifetime; // 같은 ContentForm의 실제 커서/수입/AI 지갑/소리 경계다.
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
    // 현재 같은 풀/TYPE/실제 로컬 소유자·모드로 원본 초기 시야를 계산하여 화면에 적용한다.
    void ResetCamera(CameraHomeState& state,std::function<void(std::int32_t,std::int32_t)> apply) const;
    // 현재 같은 시각을 모든 모듈에 공급한 뒤 동일 Kernel을 실행한다.
    void RunFrame(o::FrameTime time,bool paused);
    // 원본 미션 게이트의 첫 checkOutcome 위치에서만 호출한다. 설치/활성화 전에 자동 실행하지 않는다.
    void CheckMissionOutcome();
    // 원본 로그인/월드 로더가 공유하는 현재 globals.mySubscriber를 읽기만 한다. 별도 캐시를 만들지 않는다.
    std::uint32_t CurrentSubscriber() const;
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
    // 설치 뒤 로컬 간헐천의 noIsland/Player0 받침만 실제 서버 확정 사슬로 만든다.
    bool ConfirmSupport(float x,float y);
    // 원본 미션 부모가 정한 위치에서 명시 호출한다. Load/프레임의 자동 생성은 하지 않는다.
    void GenerateLocalGeysers();
    // 같은 실제 Pool/현재 Bounds/최근목록/자원/마지막 패킷을 읽는 연결부다.
    const RawCampaignConstructionBindings& Construction() const;
    // 실제 시작 설정/AI와 같은 Player 기술 부모에 원본 목록 파서를 적용한다.
    void AddTechnology(std::int32_t owner,std::string_view list);
    // 실제 flags4 기술을 가상 삭제하여 부모 체인/등록/삭제 깊이를 함께 정리한다.
    void ClearTechnology(std::int32_t owner);
    // 현재 같은 Player 부모에서 지정한 실제 기술 ContentForm을 조회한다.
    o::Sid FindTechnology(std::int32_t owner,std::int32_t type) const;
    // 실제 사제의 같은 Start/BFS/Host54 경로를 시작한다.
    bool MovePriest(o::Sid sid,o::RawPathPoint destination);
    // 같은 raw 신전의 가상 Draw를 실행하고 실제 SoundList/재생 경계로 상승음을 보낸다.
    void DrawRainTemple(o::Sid sid,const std::function<void(std::int32_t)>& frame) const;
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
