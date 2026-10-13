// 실제 캠페인의 자산 PostPop/Regular/간헐천 AI/Gun을 동일 raw 월드와 공유한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "o/RawAssetPostPop.h"
#include "o/RawAssetRegular.h"
#include "o/RawGeyserAiRegistration.h"
#include "o/RawGunProcess.h"
#include "o/RawIslandRuntime.h"
#include "o/RawPlayerWorld.h"
#include <memory>

namespace netstorm::client {
// 실제 장기 전역/TYPE 표는 상위 캠페인이 소유하며 연결 객체는 참조만 유지한다.
struct RawCampaignAssetGlobals {
    o::AssetRegularState& regular; // 자산 예약 사건이 매호출 읽는 현재 모드/통지/조사 전역이다.
    o::GunProcessState& gun; // 같은 GameClock 시각과 서버/동맹/총 상태를 상위 월드가 소유한다.
    std::span<const std::int32_t> elements; // 실제 TYPE+98의 현재 원소 자료이며 타입 전체와 같은 번호순이다.
    std::uint32_t& archerNoticeCommand; // PATCH5429b4/CD512b3c의 현재 명령 토큰이다. 두 PE 초기값030000b0이다.
};
// UI/음향/미복원 하위는 호출자가 실제 소비자를 공급한다. 누락된 선택 효과는 예외로 거부한다.
struct RawCampaignAssetEffects {
    o::AssetPostPopHooks postPop; // 실제 TYPE sound토큰/가상 Vortex owner/region/UI/Slot 등 Core 밖의 몸체다.
    o::AssetRegularHooks regular; // 실제 Ascend/이탈/레지던스 사건2/피해/건설/음향/UI 하위다.
    o::GunProcessHooks gun; // 실제 검색/표적감시/사거리통지/Animation/shot의 추가 소비자다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> queueCommand; // 동일 실제 InputQueue::PushCommand에 command/SID/0을 넣는다.
    std::function<void(std::span<const std::uint8_t>)> sendGunPacket; // 실제 flags0의 현재 GunProcess packet 전체바이트를 같은 송신기로 전달한다.
};
// 실제 모듈을 조립하며 world/Core/Player/상태의 복사 월드를 만들지 않는다.
class RawCampaignAssetBindings {
public:
    // 동일 Pool의 실제 Carrier Damageable/섬 목록과 현재9 Player BYTE를 받는다.
    // shared BindPlayerRecords API가 있는 단위15 이후 소스에서 사용한다.
    RawCampaignAssetBindings(RawSurfaceWorld& world,const o::RawCarrierPostPop& damageable,
        o::AssetPostPopState& assets,o::GameRandom& random,o::RawIslandRuntime& islands,
        std::span<std::uint8_t> playerRecords,o::PlayerWorldState& playerState,
        RawCampaignAssetGlobals globals,RawCampaignAssetEffects effects);
    // 연결 해제 전에 상위 월드가 ClearProcesses를 호출하고 반환한 Handler/Transmission 사용을 끝내야 한다.
    ~RawCampaignAssetBindings();
    RawCampaignAssetBindings(const RawCampaignAssetBindings&)=delete; // 참조/콜백의 동일 주소를 유지한다.
    RawCampaignAssetBindings& operator=(const RawCampaignAssetBindings&)=delete; // 활성 분배기에 복사 객체를 넣지 않는다.
    // 첫 저장객체 Pop 전에 실제 자산 모듈을 같은 공통 PostPop 분배기에 등록한다.
    void ActivatePostPop();
    // 현재 같은 원본 clock/Player 모드를 매 틱 직전에 갱신한다. 신전DWORD 복사는 수행하지 않는다.
    void PrepareFrame(double now);
    // 실제 Fort 로딩 중첩값을 공유 AssetPostPop 상태에 전달한다.
    void SetLoadingDepth(std::uint32_t depth);
    // 사제/Player 등 기존 실제 사건 처리기를 fallback으로 합성한다.
    o::RegularHandler Handler(o::RegularHandler fallback={}) const;
    // Gun34만 실제 writer/송신으로 분배하고 기존 다른 프로세스 전송은 fallback에 전달한다.
    o::ProcessFormTransmission Transmission(o::ProcessFormTransmission fallback={});
    // 같은 원본 모듈을 상위 설치/검사에서 조회하며 수명은 이 연결 객체에 있다.
    const o::RawAssetPostPop& PostPop() const;
    const o::RawAssetRegular& Regular() const;
    const o::RawGunProcess& Gun() const;
private:
    struct Impl; // 현재 Core 참조와 실제 모듈의 고정 주소를 소유하는 내부 자료다.
    std::unique_ptr<Impl> impl_; // 상위 실제 월드가 이 연결 객체보다 오래 살아야 한다.
};
}
