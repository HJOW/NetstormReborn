// 같은 캠페인 월드에서 로컬 간헐천의 noIsland 확정과 실제 생성 사슬을 연결한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "o/RawBattleBroadcast.h"
#include "o/RawConstructionNotice.h"
#include "o/RawConstructionPacket.h"
#include "o/RawLocalGeyser.h"
#include "o/RawPlayerGeyserBounds.h"
#include <memory>

namespace netstorm::client {
// 세션이 끝날 때까지 유지할 실제 건설 전역이다. 호출자가 현재 모드/소유자 값을 직접 갱신한다.
struct RawCampaignConstructionState {
    o::ConstructionConfirmState confirm; // 현재 확정/기록/Canon 타입 전역이다.
    o::ConstructionClearState clear; // 현재 권한과 예측 조각 정리의 Canon 타입 전역이다.
    o::ConstructionNoticeState notice; // 같은 로컬 소유자/권한과 실제 최근 건설 좌표 이력이다.
    o::ConstructionPlaceState place; // 같은 서버/권한/로컬 소유자와 비용 전역이다.
    o::ConstructionPacketHeader packet; // 실제0x3f 패킷의 현재 등록 헤더다.
    o::LocalGeyserState geyser; // 실제 요청 수량과 현재 간헐천/받침 타입 전역이다.
};
// 장치/설정/진단만 외부에서 빌린다. 게임 생성/삭제/공간/확정은 같은 월드의 실제 몸체를 사용한다.
struct RawCampaignConstructionEffects {
    std::function<std::int32_t(std::string_view,std::int32_t)> configInt; // 현재 predictableGeysers 설정을 읽는다.
    std::function<double()> wallSeconds; // 비결정적 생성에서 TimeSeed에 전달할 현재 벽시각이다.
    std::function<void()> refresh; // 확정 통지 마지막의 실제 전역 표시 갱신 요청이다.
    o::ConstructionPlaceHooks presentation; // 비용 문구/안내/SP·취소 송신만 사용하며 생성/소유자/SP/공간은 내부 실제 몸체가 공급한다.
    std::function<void()> negativeQuantity; // reps>=0 원본 진단을 전달한다.
    std::function<void(o::PlayerGeyserBoundsAssertion)> boundsAssertion; // 현재 Player/모드/Canon 진단을 전달한다.
};
// 같은 Session의 기준점/실제 Builder/사제 경로/건설 시작을 연결하는 현재 게임 몸체다.
struct RawCampaignConstructionGameplay {
    o::ConstructionConfirmHooks confirm; // 기준점과 선택적 실제 기록기다. 생성/송신/직접 통지는 내부가 연결한다.
    o::ConstructionNoticeHooks notice; // 운반/사제 조회·접근·이동/시작이다. 정리/배치/환불/삭제/갱신은 내부가 연결한다.
};
// 원본 일반 서버 확정/배치 통지와 로컬 간헐천이 같은 실제 월드를 사용한다.
class RawCampaignConstructionBindings {
public:
    // 모든 참조는 이 연결보다 오래 살아야 한다. recent는 전체 geysers 목록과 서로 다른 전용5칸 이상 저장소다.
    RawCampaignConstructionBindings(RawSurfaceWorld& world,o::SquidReward& hp,o::ScrambledSpStore& sp,
        o::SquidRewardState& reward,std::span<const o::PriestPlainCanonType> frames,const o::ChunkMap& chunks,
        std::span<std::uint8_t> players,o::RawBattleBroadcast& broadcast,RawCampaignConstructionState& state,
        o::PlayerGeyserBoundsState bounds,o::SquidPostPopList& recent,const o::SquidPostPopList& allGeysers,
        RawCampaignConstructionEffects effects,RawCampaignConstructionGameplay gameplay={});
    // 고정 주소를 참조하는 내부 callback을 복사하지 않는다.
    ~RawCampaignConstructionBindings();
    RawCampaignConstructionBindings(const RawCampaignConstructionBindings&)=delete; // 다른 월드 저장소로 복사하지 않는다.
    RawCampaignConstructionBindings& operator=(const RawCampaignConstructionBindings&)=delete; // 현재 callback의 고정 주소를 보존한다.
    // MayPlace를 통과한 일반 서버 요청을 원본 기준점 검사→SID 확보→송신→직접 통지 순서로 확정한다.
    bool Confirm(const o::ConstructionConfirmRequest& request) const;
    // LocalGeyser 전용 입구는 기존 noIsland/player0/나머지0 계약을 유지한다.
    bool ConfirmSupport(const o::ConstructionConfirmRequest& request) const;
    // 현재 Player9와 실제 PlayerBounds를 읽어 원본 로컬 생성 부모를 실행한다.
    void GenerateLocalGeysers() const;
    // 최근 생성/자원/풀의 실제 단일 저장소를 빌린다. 독립 간헐천 모형을 만들지 않는다.
    const o::RawLocalGeyser& LocalGeysers() const;
    const o::RawPlayerGeyserBounds& PlayerBounds() const;
    // 원본 임시512바이트 가운데 마지막 송신 길이만 빌린다. 다음 확정 때 같은 버퍼가 덮어써진다.
    std::span<const std::uint8_t> LastPacket() const;
    // 실제 noIsland VT의 Pre/Post만 기존 표면 수명으로 분배하고 종속/전송/선택은 현재 분배를 보존한다.
    o::SquidDestroyHooks DestroyHooks(o::SquidDestroyHooks fallback) const;
    // 예측 Clear가 요청한 실제 noIsland VT의 공통 가상 삭제를 같은 Pool/Graph/종속 수명에 연결한다.
    o::SquidAssetDestroy Destroy(o::SquidAssetDestroy fallback) const;
    // 같은 월드의 생성/삭제/공간을 공유하는지 상위 연결에서 확인한다.
    const o::SidPool& Pool() const;
private:
    struct Impl; // 실제 하위 모듈과 원본 임시 패킷의 고정 주소 소유자다.
    std::unique_ptr<Impl> impl_; // 외부 세션보다 먼저 해제하며 별도 Kernel 프로세스를 생성하지 않는다.
};
}
