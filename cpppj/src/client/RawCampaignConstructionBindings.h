// 같은 캠페인 월드에서 로컬 간헐천의 noIsland 확정과 실제 생성 사슬을 연결한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "o/RawBattleBroadcast.h"
#include "o/RawConstructionNotice.h"
#include "o/RawConstructionLocal.h"
#include "o/RawConstructionCommand.h"
#include "o/RawConstructionPacket.h"
#include "o/RawLocalGeyser.h"
#include "o/RawPlayerGeyserBounds.h"
#include <memory>

namespace netstorm::client {
// 세션이 끝날 때까지 유지할 실제 건설 전역이다. 호출자가 현재 모드/소유자 값을 직접 갱신한다.
struct RawCampaignConstructionState {
    o::ConstructionRequestState request; // 전체 서버 MayPlace/충돌/받침 확정의 현재 전역이다.
    o::ConstructionCommandState command; // 실제0x3e 수신의 차단/비용/로컬 owner 전역이다.
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
    std::function<bool(std::int32_t)> islandExists; //일반 간헐천 제외 constructor가 읽는 같은 실제 IslandList다.
};
// 같은 Session의 기준점/실제 Builder/사제 경로/건설 시작을 연결하는 현재 게임 몸체다.
struct RawCampaignConstructionGameplay {
    std::function<void(o::Sid,std::uint32_t)> setOwner; // 원본 가상+74의 파생 소유자 몸체다. 없으면 공통 소유자를 사용한다.
    o::ConstructionRequestHooks request; // 실제 MayPlace/Displace/DropIsland이며 Confirm은 내부가 연결한다.
    o::ConstructionConfirmHooks confirm; // 기준점과 선택적 실제 기록기다. 생성/송신/직접 통지는 내부가 연결한다.
    o::ConstructionNoticeHooks notice; // 운반/사제 조회·접근·이동/시작이다. 정리/배치/환불/삭제/갱신은 내부가 연결한다.
};
// 건설이 실제 소비하는 같은13참조만 보관하며 World/Kernel/Form 소유권을 만들지 않는다.
struct RawConstructionCore {
    o::SidPool& pool; // 현재 생성/삭제의 실제 SID 풀이다.
    o::SquidHash& hash; // 현재 후보/표면 검색의 같은 해시다.
    std::span<std::uint8_t> spots; // 실제65536BYTE 점유 지도다.
    std::span<const o::RiftTypeRecord> types; // 같은 현재 TYPE 표다.
    std::span<const o::RiftTypeFrames> frames; // 같은 현재 프레임 표다.
    o::SquidPostPopState& bookkeeping; // HP/비용/생산과 공유하는 실제 장부다.
    o::SquidOwnerMode& ownerMode; // 같은 현재 battle/metamap bool 주소다.
    o::SquidFactory& factory; // 같은 실제 생성자 분배기다.
    o::SquidOwner& owner; // 같은 공통 소유자 지정 부모다.
    o::RawBuildingSpace& building; // 실제 표면 통지 부모다.
    o::RawSquidDestroyDispatch& virtualDestroy; // 같은 현재 가상10 삭제다.
    o::RawSurfaceLifecycle& surfaces; // 같은 실제 표면 Pre/Post 수명이다.
    o::BridgeConnectState& bridgeState; // 현재 다리/받침 TYPE 번호의 같은 주소다.
};
RawConstructionCore BindConstructionCore(const RawWorldCore& core); // 독립 World의 기존 실제 참조13개만 선택한다.
RawConstructionCore BindConstructionCore(RawSurfaceStorage& storage); // Native는 World 없이 같은 장기 부품을 직접 빌린다.
// 건설/간헐천의 실제 물리와 가상 생성·공간·삭제 부모다. World를 새로 만들거나 대여/reset하지 않는다.
struct RawConstructionRuntime {
    RawConstructionCore core; //같은 Pool/Hash/TYPE/spot/장부·Factory/Pop/Graph/Host를 참조하며 배열이나 상태 사본을 만들지 않는다.
    std::shared_ptr<RawSurfaceStorage> storage; //진단과 같은 장기 물리를 pin하며 짧은 UI/Session의 소유권을 합성하지 않는다.
    std::function<o::Sid(std::uint32_t,std::uint32_t)> create; //현재 실제 Factory 생성 부모이며 선택된 생성의 앞선 효과를 보존한다.
    std::function<void(o::Sid,float,float,std::uint32_t)> pop; //현재 실제 가상+90의 전체 파생 PostPop/전송까지 호출한다.
    std::function<bool(o::Sid,std::uint32_t)> destroy; //현재 실제 가상 삭제의 수락/거부 결과이며 목록 제거만으로 대신하지 않는다.
    std::function<std::span<std::uint8_t>()> players; // Native는 같은 ClientPlayerRecords의 현재base를 원문 선택 위치에서 조회한다. 독립 World는 기존 span 계약을 유지한다.
    std::function<void(std::span<std::uint8_t>)> broadcast; // Native는 Session 방송 객체 없이 같은 현재 Player/Source와 호출별 mutable 버퍼를 사용한다.
};
RawConstructionRuntime BindConstructionRuntime(RawSurfaceWorld& world); //기존 실제 World의 Core/Create/Pop/Destroy를 빌린다. World는 이 뷰와 결합한 모듈보다 오래 살아야 한다.
// 원본 일반 서버 확정/배치 통지와 로컬 간헐천이 같은 실제 월드를 사용한다.
class RawCampaignConstructionBindings {
public:
    // 모든 참조는 이 연결보다 오래 살아야 한다. recent는 전체 geysers 목록과 서로 다른 전용5칸 이상 저장소다.
    RawCampaignConstructionBindings(RawSurfaceWorld& world,o::SquidReward& hp,o::ScrambledSpStore& sp,
        o::SquidRewardState& reward,std::span<const o::PriestPlainCanonType> frames,const o::ChunkMap& chunks,
        std::span<std::uint8_t> players,o::RawBattleBroadcast& broadcast,RawCampaignConstructionState& state,
        o::PlayerGeyserBoundsState bounds,o::SquidPostPopList& recent,const o::SquidPostPopList& allGeysers,
        RawCampaignConstructionEffects effects,RawCampaignConstructionGameplay gameplay={});
    // 장기 실행 owner는 동일한 실제 Core/가상 부모를 전달한다. 이 overload는 World 생성/Acquire/모드 변경을 하지 않는다.
    RawCampaignConstructionBindings(RawConstructionRuntime runtime,o::SquidReward& hp,o::ScrambledSpStore& sp,
        o::SquidRewardState& reward,std::span<const o::PriestPlainCanonType> frames,const o::ChunkMap& chunks,
        std::span<std::uint8_t> players,o::RawBattleBroadcast& broadcast,RawCampaignConstructionState& state,
        o::PlayerGeyserBoundsState bounds,o::SquidPostPopList& recent,const o::SquidPostPopList& allGeysers,
        RawCampaignConstructionEffects effects,RawCampaignConstructionGameplay gameplay={});
    // Native 장기 장치는 현재 Player/방송 조회기를 필수로 제공하며 짧은 Player span/방송 객체를 빌리지 않는다.
    RawCampaignConstructionBindings(RawConstructionRuntime runtime,o::SquidReward& hp,o::ScrambledSpStore& sp,
        o::SquidRewardState& reward,std::span<const o::PriestPlainCanonType> frames,const o::ChunkMap& chunks,
        RawCampaignConstructionState& state,o::PlayerGeyserBoundsState bounds,o::SquidPostPopList& recent,
        const o::SquidPostPopList& allGeysers,RawCampaignConstructionEffects effects,RawCampaignConstructionGameplay gameplay={});
    // 고정 주소를 참조하는 내부 callback을 복사하지 않는다.
    ~RawCampaignConstructionBindings();
    RawCampaignConstructionBindings(const RawCampaignConstructionBindings&)=delete; // 다른 월드 저장소로 복사하지 않는다.
    RawCampaignConstructionBindings& operator=(const RawCampaignConstructionBindings&)=delete; // 현재 callback의 고정 주소를 보존한다.
    // MayPlace를 통과한 일반 서버 요청을 원본 기준점 검사→SID 확보→송신→직접 통지 순서로 확정한다.
    bool Confirm(const o::ConstructionConfirmRequest& request) const;
    // 로컬 예측도 같은 Factory/Place를 사용하며 나머지 Form/반환/송신 경계를 유지한다.
    o::ConstructionLocalHooks BindLocal(o::ConstructionLocalHooks hooks) const;
    // 일반 요청은 실제 MayPlace/abstract 충돌/밀어내기/받침/본 확정을 차례로 실행한다.
    bool Request(const o::ConstructionConfirmRequest& request) const;
    // 같은 실제0x3e 수신의 비용 선검사와 실패 환불을 실행한다.
    void Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const;
    void ReceiveNotice(std::span<const std::uint8_t> packet) const; //현재 전역을 준비한 caller와 실제3fh 등록이 같은 BYTE 통지 본체를 호출한다.
    void Refund(std::uint32_t type,std::uint32_t owner) const; // 다른 실제 서버 요청도 같은442a40 비용 환불/통지를 사용한다.
    void Reject(std::uint32_t type,std::uint32_t owner,float x,float y,std::uint32_t argument,std::uint32_t direction) const; //00442af0의현재환불/취소패킷수명을공유한다.
    void ChargeShortfall(std::uint32_t type,std::uint32_t owner,float x,float y,std::uint32_t argument,std::uint32_t direction) const; //00442b50의같은현재flags54/권한/안내와조건부Reject를사용한다.
    // 실제 전송 분배기에 같은 수신 몸체를 등록하며 prepare는 현재 Session 전역을 갱신한다.
    void Register(o::RawPacketDispatch& dispatch,std::function<void()> prepare);
    // LocalGeyser 전용 입구는 기존 noIsland/player0/나머지0 계약을 유지한다.
    bool ConfirmSupport(const o::ConstructionConfirmRequest& request) const;
    // 현재 Player9와 실제 PlayerBounds를 읽어 원본 로컬 생성 부모를 실행한다.
    void GenerateLocalGeysers() const;
    //전체 맵 부모도 같은 현재 Pool/자원/최근목록과 일반 제외·허용 인수를 사용한다.
    void PopulateGeysers(std::int32_t quantity,o::GameRandom& random,o::FortRandomRectangle rectangle,o::GeyserPlacementOptions options) const;
    // 최근 생성/자원/풀의 실제 단일 저장소를 빌린다. 독립 간헐천 모형을 만들지 않는다.
    const o::RawLocalGeyser& LocalGeysers() const;
    const o::RawPlayerGeyserBounds& PlayerBounds() const;
    // 마지막 방송 직후 진단 사본의 본문/SID 범위를 빌린다. 실제 선언WORD/전송 길이와 별개이며 다음 방송 때 덮어쓴다.
    std::span<const std::uint8_t> LastPacket() const;
    // 실제 표면/받침/종유석의 Pre/Post를 기존 표면 수명으로 분배하고 다른 파생 효과는 보존한다.
    o::SquidDestroyHooks DestroyHooks(o::SquidDestroyHooks fallback) const;
    // 예측 Clear와 실제DropIsland가 요청한 검증된 받침들의 삭제를 같은Pool/Graph/종속 수명에 연결한다.
    o::SquidAssetDestroy Destroy(o::SquidAssetDestroy fallback) const;
    // 같은 월드의 생성/삭제/공간을 공유하는지 상위 연결에서 확인한다.
    const o::SidPool& Pool() const;
private:
    struct Impl; // 실제 하위 모듈과 원본 임시 패킷의 고정 주소 소유자다.
    std::unique_ptr<Impl> impl_; // 외부 세션보다 먼저 해제하며 별도 Kernel 프로세스를 생성하지 않는다.
};
}
