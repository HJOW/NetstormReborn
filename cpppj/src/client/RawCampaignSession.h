// 실제 캠페인 초기화/전투/이동의 상태와 모듈 수명을 클라이언트 세션 하나에 보관한다.
#pragma once
#include "client/GameAssets.h"
#include "client/RawCampaignWorldRuntime.h"
#include "client/RawCampaignMotionBindings.h"
#include "client/RawCampaignAssetBindings.h"
#include "client/RawCampaignArcherBindings.h"
#include "client/RawCampaignHitBindings.h"
#include "client/SoundProcess.h"
#include "o/RawPlayerRegular.h"
#include "o/RawBattleBroadcast.h"
#include "client/CameraHome.h"
namespace netstorm::client {
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
    o::SquidDestroyHooks deletion; // 다른 파생 삭제/선택/전송의 실제 외부 경계다.
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
    // 실제 사제의 같은 Start/BFS/Host54 경로를 시작한다.
    bool MovePriest(o::Sid sid,o::RawPathPoint destination);
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
