// 기존 원본 Confirm/Packet/Notice/Clear/Place와 PlayerBounds/LocalGeyser를 같은 월드에 조립한다.
#include "client/RawCampaignConstructionBindings.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kNoIslandUnsupportedFlags=0x8010,kDaisGenus=0x400000; // HP/운반/데이스는 일반 건설의 미복원 분기를 요구한다.
constexpr std::size_t kPacketBytes=512,kRecentMinimum=5,kPlayerBytes=9*0xb4; // 실제 임시 패킷/최근 목록/Player9의 물리 크기다.
// 선택되지 않은 일반 건설 경계를 빈 성공으로 대신하지 않고 필요한 하위 이름을 진단한다.
template<class Result,class... Arguments> std::function<Result(Arguments...)> ConstructionMissing(std::string name) {
    return [name=std::move(name)](Arguments...)->Result { throw std::logic_error("캠페인 일반 건설 미연결: "+name); };
}
// 원본 VT 기록DWORD를 호스트 포인터로 실행하지 않고 실제 little endian 값으로 읽는다.
std::uint32_t ConstructionVtable(std::span<const std::uint8_t> raw) {
    std::uint32_t value=0;
    // 실제첫네BYTE만조합하며객체의다른필드를읽지않는다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[index])<<(8*index);return value;
}
}
// 외부의 단일 상태/월드 주소를 빌리며 실제 계산/공간/삭제 몸체만 내부에 조립한다.
struct RawCampaignConstructionBindings::Impl {
    RawSurfaceWorld& world; // 모든 객체의 실제 생성/가상 삭제/Pop을 소유한다.
    RawWorldCore core; // 같은 Pool/TYPE/hash/spot/장부의 참조만 보관한다.
    RawCampaignConstructionState& state; // 현재 모드/타입/수량/이력을 프레임 사본 없이 읽는다.
    o::RawBattleBroadcast& broadcast; // 같은 실제 Player9/NULL 또는 연결 표의 원본 송신 분배기다.
    RawCampaignConstructionEffects effects; // 실제 설정/벽시각/표시/진단의 외부 소비자다.
    std::array<std::uint8_t,kPacketBytes> packet{}; // 원본 Confirm의512바이트 임시 패킷에 대응한다.
    std::size_t packetLength{}; // 최종화 이후의 현재 송신 길이며 최초 확정 전에는0이다.
    std::unique_ptr<o::RawConstructionPlace> place; // 같은 HP/SP/소유자와 실제 Pop을 사용한다.
    std::unique_ptr<o::RawConstructionClear> clear; // 같은 현재 hash/예측 조각을 실제 가상 삭제한다.
    std::unique_ptr<o::RawConstructionNotice> notice; // 원본 직접 수신 순서와 이력을 실행한다.
    std::unique_ptr<o::RawConstructionRequest> requestBody; // 같은 공간/MayPlace와 실제 서버 확정을 사용한다.
    std::unique_ptr<o::RawConstructionCommand> command; // 같은 실제 Place의 Money/부족/Reject를 사용하는0x3e 수신이다.
    std::unique_ptr<o::RawConstructionConfirm> confirm; // 실제 서버 SID를 확보하고 전송 뒤 직접 처리한다.
    std::unique_ptr<o::RawPlayerGeyserBounds> bounds; // 같은 Player9/ChunkMap/TYPE/프레임을 읽는다.
    std::unique_ptr<o::RawLocalGeyser> geysers; // 같은 보상 자원 pool과 전용 최근5목록을 사용한다.

    // 첫 생성 전에 같은 저장소/필수 서비스와 실제 일반 건설 경계를 구성한다.
    Impl(RawSurfaceWorld& source,o::SquidReward& hp,o::ScrambledSpStore& sp,o::SquidRewardState& reward,
        std::span<const o::PriestPlainCanonType> frames,const o::ChunkMap& chunks,std::span<std::uint8_t> players,
        o::RawBattleBroadcast& transmission,RawCampaignConstructionState& globals,o::PlayerGeyserBoundsState currentBounds,
        o::SquidPostPopList& recent,const o::SquidPostPopList& allGeysers,RawCampaignConstructionEffects input,RawCampaignConstructionGameplay gameplay)
        :world(source),core(source.Core()),state(globals),broadcast(transmission),effects(std::move(input)) {
        if (core.pool.Edition()!=o::OriginalEdition::Patch1078 || &hp.Pool()!=&core.pool || &hp.Bookkeeping()!=&core.bookkeeping ||
            frames.size()!=core.types.size() || players.size()!=kPlayerBytes || &recent==&allGeysers || recent.entries.size()<kRecentMinimum ||
            &currentBounds.battle!=&core.ownerMode.battle || &currentBounds.metamap!=&core.ownerMode.challenge ||
            !effects.configInt || !effects.wallSeconds || !effects.refresh || !effects.negativeQuantity || !effects.boundsAssertion)
            throw std::invalid_argument("캠페인 받침/간헐천의 같은 월드/현재 모드/전용 목록/실제 서비스 계약 오류");
        ValidateState();
        auto placeHooks=effects.presentation; // 현재 비용/UI 소비자를 사용하며 비어 있는 선택 경계는 명시 오류다.
        placeHooks.notifySurface=[this](o::Sid sid) { core.building.NotifySurface(sid); };
        placeHooks.pop=[this](o::Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        if (!placeHooks.sendMoney) placeHooks.sendMoney=ConstructionMissing<void,std::uint32_t,const o::ConstructionMoneyNotice&>("SP 통지");
        if (!placeHooks.format) placeHooks.format=ConstructionMissing<std::string,std::string_view,std::string_view>("비용 안내 서식");
        if (!placeHooks.tellOthers) placeHooks.tellOthers=ConstructionMissing<void,std::uint32_t,std::string_view>("다른 Player 비용 안내");
        if (!placeHooks.tellPlayer) placeHooks.tellPlayer=ConstructionMissing<void,std::uint32_t,std::string_view>("Player 비용 안내");
        if (!placeHooks.tell) placeHooks.tell=ConstructionMissing<void,std::string_view>("로컬 비용 안내");
        if (!placeHooks.sendReject) placeHooks.sendReject=ConstructionMissing<void,std::uint32_t,const o::ConstructionRejectNotice&>("배치 취소 통지");
        placeHooks=o::MakeConstructionPlaceHooks(core.pool,core.factory,core.owner,hp,&sp,state.place,std::move(placeHooks));
        if (gameplay.setOwner) placeHooks.setOwner=std::move(gameplay.setOwner); // 내려놓은 사제의 원래 소유자와 전투 표식을 가상 몸체가 복원한다.
        place=std::make_unique<o::RawConstructionPlace>(core.pool,core.types,frames,state.place,hp,std::move(placeHooks));
        place->BindPlayerRecords(players);
        o::ConstructionClearHooks clearHooks; // 삭제는 관찰/목록 제거가 아닌 같은 가상 수명 전체를 실행한다.
        clearHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { Destroy(sid,flags); };
        clear=std::make_unique<o::RawConstructionClear>(core.pool,core.hash,core.types,frames,state.clear,std::move(clearHooks));
        auto noticeHooks=std::move(gameplay.notice); // 같은 실제 Session의 운반/사제/시작을 선택 시 호출한다.
        if (!noticeHooks.moveBuilder) noticeHooks.moveBuilder=ConstructionMissing<void,std::uint32_t,o::Sid,std::uint32_t,std::uint32_t>("건설 운반 생성/이동");
        if (!noticeHooks.findPriest) noticeHooks.findPriest=ConstructionMissing<o::Sid,std::uint32_t>("소유 사제 조회");
        if (!noticeHooks.containedType) noticeHooks.containedType=ConstructionMissing<std::uint32_t,std::uint32_t>("사제 contained 타입");
        if (!noticeHooks.removeContained) noticeHooks.removeContained=ConstructionMissing<void,o::Sid,std::uint32_t>("사제 contained 삭제");
        if (!noticeHooks.immobile) noticeHooks.immobile=ConstructionMissing<bool,o::Sid>("사제 이동 불가");
        if (!noticeHooks.approach) noticeHooks.approach=ConstructionMissing<o::ConstructionNoticePoint,o::Sid,float,float,std::uint32_t>("사제 접근점");
        if (!noticeHooks.tryMove) noticeHooks.tryMove=ConstructionMissing<bool,o::Sid,float,float,o::Sid>("사제 건설 접근 이동");
        noticeHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { Destroy(sid,flags); };
        if (!noticeHooks.start) noticeHooks.start=ConstructionMissing<void,o::Sid,std::uint32_t>("건설 Process 시작");
        noticeHooks.refresh=effects.refresh;
        notice=std::make_unique<o::RawConstructionNotice>(core.pool,core.types,state.notice,
            o::MakeConstructionNoticeHooks(core.pool,*clear,*place,std::move(noticeHooks)));
        auto confirmHooks=std::move(gameplay.confirm); // 서버 SID와 실제 wire 전송 뒤 원본처럼 같은 통지를 직접 처리한다.
        if (!confirmHooks.findBuilder) confirmHooks.findBuilder=ConstructionMissing<o::Sid,std::uint32_t,std::uint32_t,float,float>("작업장/Graph 기준점");
        confirmHooks.broadcast=[this](const o::ConstructionNotice& value) {
            o::WriteConstructionPacket(value,state.packet,packet);o::FinalizeConstructionPacket(packet,true);
            packetLength=static_cast<std::size_t>(packet[2])|(static_cast<std::size_t>(packet[3])<<8);
            broadcast.Send(std::span<const std::uint8_t>(packet).first(packetLength));
        };
        if (!confirmHooks.recorderActive) confirmHooks.recorderActive=ConstructionMissing<bool>("기록기 활성 조회");
        if (!confirmHooks.record) confirmHooks.record=ConstructionMissing<void,o::Sid>("기록기 첫 SID 기록");
        confirmHooks=o::MakeConstructionNoticeConfirmHooks(core.pool,*notice,std::move(confirmHooks));
        confirm=std::make_unique<o::RawConstructionConfirm>(core.pool,core.types,frames,state.confirm,
            o::MakeConstructionConfirmHooks(core.pool,core.factory,std::move(confirmHooks)));
        auto requestHooks=std::move(gameplay.request); // 명시된 실제 Session 하위만 사용한다.
        if (!requestHooks.mayPlace) requestHooks.mayPlace=ConstructionMissing<bool,const o::CanonPlacementQuery&>("전체 MayPlace");
        if (!requestHooks.displace) requestHooks.displace=ConstructionMissing<bool,float,float,std::uint32_t,std::uint32_t>("발자국 유닛 밀어내기");
        if (!requestHooks.cancel) requestHooks.cancel=ConstructionMissing<void,float,float,std::uint32_t,std::uint32_t>("실패 섬 정리");
        requestHooks.confirm=[this](const o::ConstructionConfirmRequest& value) { return confirm->Confirm(value); };
        requestBody=std::make_unique<o::RawConstructionRequest>(core.pool,core.hash,core.types,frames,core.spots,state.request,std::move(requestHooks));
        o::ConstructionCommandHooks commandHooks; // 비용 선검사와 실패 환불도 같은 실제 Place다.
        commandHooks.money=[this](std::uint32_t owner) { return place->Money(owner); };
        commandHooks.shortfall=[this](const o::ConstructionConfirmRequest& value) { place->ChargeShortfall(value.type,value.player,value.x,value.y,value.argument,value.direction); };
        commandHooks.request=[this](const o::ConstructionConfirmRequest& value) { return requestBody->Request(value); };
        commandHooks.reject=[this](const o::ConstructionConfirmRequest& value) { place->Reject(value.type,value.player,value.x,value.y,value.argument,value.direction); };
        command=std::make_unique<o::RawConstructionCommand>(core.types,state.command,std::move(commandHooks));
        bounds=std::make_unique<o::RawPlayerGeyserBounds>(core.pool,chunks,core.types,core.frames,players,currentBounds,effects.boundsAssertion);
        o::LocalGeyserHooks geyserHooks; // factory/Pop/삭제/받침 모두 현재 같은 월드의 실제 몸체다.
        geyserHooks.configInt=effects.configInt;geyserHooks.wallSeconds=effects.wallSeconds;geyserHooks.negativeQuantity=effects.negativeQuantity;
        geyserHooks.regionExists=effects.islandExists;
        geyserHooks.playerBounds=[this](std::uint32_t player) { return bounds->Bounds(player); };
        geyserHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { Destroy(sid,flags); };
        geyserHooks.confirm=[this](const o::ConstructionConfirmRequest& request) { static_cast<void>(ConfirmSupport(request)); };
        geyserHooks.create=[this](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        geyserHooks.pop=[this](o::Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
        geysers=std::make_unique<o::RawLocalGeyser>(core.pool,core.hash,core.spots,core.types,core.frames,players,recent,state.geyser,std::move(geyserHooks));
        geysers->BindResourcePool(reward.GeyserPool());
    }
    // 실제 현재 전역의 불일치/일반 건설 분기를 생성 전에 거부한다. 지원 범위를 임의 성공으로 넓히지 않는다.
    void ValidateState() const {
        const auto type=state.place.noIslandType; // 동일받침전역과실제타입플래그를매요청다시읽는다.
        if (type>=core.types.size() || type!=core.bridgeState.noIslandType || type!=state.geyser.noIslandType ||
            type!=state.confirm.patternTypes[3] || state.confirm.patternTypes!=state.clear.patternTypes ||
            state.confirm.patternTypes!=state.place.patternTypes || state.place.bridgeType!=state.clear.bridgeType ||
            state.place.bridgeType!=core.bridgeState.bridgeType || state.place.localPlayer!=state.notice.localPlayer ||
            state.place.authority!=state.clear.authority || state.place.authority!=state.notice.authority ||
            state.place.authority!=state.geyser.boss || state.geyser.fort!=core.ownerMode.fort ||
            (core.types[type].flags1&kNoIslandUnsupportedFlags) || (core.types[type].flags2&kDaisGenus) ||
            type==state.notice.daisType)
            throw std::logic_error("캠페인 건설/간헐천의 현재 전역 불일치");
    }
    // 원본 LocalGeyser의 명시0인자만허용하여커서/SP/운반미연결을통과하지못하게한다.
    bool ConfirmSupport(const o::ConstructionConfirmRequest& request) const {
        ValidateState();
        if (request.type!=state.place.noIslandType || request.player || request.argument || request.direction || request.flags ||
            request.timeLow || request.timeHigh || request.quality)
            throw std::logic_error("캠페인 받침 확정은 LocalGeyser의 noIsland/Player0 요청만 지원합니다");
        return confirm->Confirm(request);
    }
    // 가상삭제거부를성공한예측정리/empty교체로간주하지않는다.
    void Destroy(o::Sid sid,std::uint32_t flags) const {
        if (!world.Destroy(sid,flags)) throw std::logic_error("캠페인 받침/간헐천의 실제 가상 삭제 거부");
    }
    // 실제 표면/받침/종유석의 확인된 가상 표만 표면 수명으로 분배한다. 살아 있는 받침도DropIsland가 정리한다.
    bool Support(o::Sid sid) const {
        const auto raw=core.pool.Slot(sid);const auto table=ConstructionVtable(raw);
        if (raw[10]==state.place.noIslandType && table==o::kPatchNoIslandVtable) return true;
        //94의 고유pre=442240·157의 고유post=442640은 기존SurfaceLifecycle이 구별한다.
        return (raw[10]==core.bridgeState.islandType && table==o::kPatchIslandVtable) ||
            (raw[10]==core.bridgeState.stalagType && table==o::kPatchStalagVtable) ||
            (raw[10]==96 && table==0x501dd0U); // 원본96 그루터기의 생성자0은 공통Squid 표/수명이다.
    }
};
// 실제세션의장기자료를복사하지않고같은주소의하위모듈을구성한다.
RawCampaignConstructionBindings::RawCampaignConstructionBindings(RawSurfaceWorld& world,o::SquidReward& hp,o::ScrambledSpStore& sp,
    o::SquidRewardState& reward,std::span<const o::PriestPlainCanonType> frames,const o::ChunkMap& chunks,
    std::span<std::uint8_t> players,o::RawBattleBroadcast& broadcast,RawCampaignConstructionState& state,
    o::PlayerGeyserBoundsState bounds,o::SquidPostPopList& recent,const o::SquidPostPopList& allGeysers,RawCampaignConstructionEffects effects,RawCampaignConstructionGameplay gameplay)
    :impl_(std::make_unique<Impl>(world,hp,sp,reward,frames,chunks,players,broadcast,state,bounds,recent,allGeysers,std::move(effects),std::move(gameplay))) {}
// 내부모듈은새Kernel프로세스를소유하지않으며현재배열보다먼저해제한다.
RawCampaignConstructionBindings::~RawCampaignConstructionBindings()=default;
// 일반 서버 요청도 원본 확정/통지/배치 몸체를 그대로 사용한다. 배치 가능 판정은 호출자의 선행 책임이다.
bool RawCampaignConstructionBindings::Confirm(const o::ConstructionConfirmRequest& request) const { impl_->ValidateState();return impl_->confirm->Confirm(request); }
// 로컬 예측은 서버 확정/통지와 같은 실제 배치기를 사용한다.
o::ConstructionLocalHooks RawCampaignConstructionBindings::BindLocal(o::ConstructionLocalHooks hooks) const { return o::MakeConstructionLocalHooks(impl_->core.pool,impl_->core.factory,*impl_->place,std::move(hooks)); }
// 전체 요청의 실제 공간/받침 검사와 현재 원본 확정만 실행한다.
bool RawCampaignConstructionBindings::Request(const o::ConstructionConfirmRequest& value) const { impl_->ValidateState();return impl_->requestBody->Request(value); }
// 원본 수신은 요청 실패 때 같은 실제 Place에서 환불/취소를 요청한다.
void RawCampaignConstructionBindings::Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const { impl_->ValidateState();impl_->command->Receive(sender,packet); }
// Cast/Drop의 잘못된 소유자도 기존 원본 환불 몸체를 사용한다.
void RawCampaignConstructionBindings::Refund(std::uint32_t type,std::uint32_t owner) const { impl_->ValidateState();impl_->place->Refund(type,owner); }
// 비용 부족 뒤 계속하는 서버 분기는 같은 원본 취소 통지를 요청한다.
void RawCampaignConstructionBindings::Reject(std::uint32_t type,std::uint32_t owner,float x,float y,std::uint32_t argument,std::uint32_t direction) const { impl_->ValidateState();impl_->place->Reject(type,owner,x,y,argument,direction); }
// 분배기/전송 계층의 실제 수신 지점에 현재 고정 주소 몸체를 연결한다.
void RawCampaignConstructionBindings::Register(o::RawPacketDispatch& dispatch,std::function<void()> prepare) { o::RegisterConstructionCommand(dispatch,*impl_->command,std::move(prepare)); }
// 로컬 간헐천 전용 입구는 기존 인자 제한을 보존한다.
bool RawCampaignConstructionBindings::ConfirmSupport(const o::ConstructionConfirmRequest& request) const { return impl_->ConfirmSupport(request); }
// 실제로그인/Player초기화뒤미션부모가지정한위치에서만호출한다.
void RawCampaignConstructionBindings::GenerateLocalGeysers() const { impl_->ValidateState();impl_->geysers->Generate(); }
//각 생성 요청 직전에 같은 실제 건설 전역의 정합성을 확인한다.
void RawCampaignConstructionBindings::PopulateGeysers(std::int32_t quantity,o::GameRandom& random,o::FortRandomRectangle rectangle,o::GeyserPlacementOptions options) const { impl_->ValidateState();impl_->geysers->Populate(quantity,random,rectangle,options); }
// 공유자원/최근목록의identity와현재값을상위에서검사할수있게한다.
const o::RawLocalGeyser& RawCampaignConstructionBindings::LocalGeysers() const { return *impl_->geysers; }
// 현재Player/청크지도의기존정밀몸체를공유한다.
const o::RawPlayerGeyserBounds& RawCampaignConstructionBindings::PlayerBounds() const { return *impl_->bounds; }
// 최종화이후실제송신에넘긴동일버퍼만노출한다.
std::span<const std::uint8_t> RawCampaignConstructionBindings::LastPacket() const { return std::span<const std::uint8_t>(impl_->packet).first(impl_->packetLength); }
// 실제 표면/받침/종유석의 pre/post는 같은 표면/공통 수명을 실행한다. 다른 효과는 기존 분배를 보존한다.
o::SquidDestroyHooks RawCampaignConstructionBindings::DestroyHooks(o::SquidDestroyHooks fallback) const {
    auto hooks=fallback;hooks.emit=[this,surfaces=impl_->core.surfaces.Hooks(),fallback=std::move(fallback)](const o::SquidDestroyEvent& event) {
        if (impl_->Support(event.sid) && (event.effect==o::SquidDestroyEffect::PreDestroy || event.effect==o::SquidDestroyEffect::PostDestroy)) {
            surfaces.emit(event);return;
        }
        if (!fallback.emit) throw std::logic_error("다른 캠페인 건설 자산 삭제 효과 미연결");fallback.emit(event);
    };return hooks;
}
// Form 분배의 현재 hooks로 받침들의 dead/종속/Unpop/깊이/반납을 모두 실행한다.
o::SquidAssetDestroy RawCampaignConstructionBindings::Destroy(o::SquidAssetDestroy fallback) const {
    return [this,fallback=std::move(fallback)](o::Sid sid,std::uint32_t flags,const o::SquidDestroyHooks& hooks) {
        if (impl_->Support(sid)) return impl_->core.virtualDestroy.Destroy(sid,flags,hooks);
        if (!fallback) throw std::logic_error("다른 캠페인 건설 자산 가상 삭제 미연결");return fallback(sid,flags,hooks);
    };
}
// 다른세션의풀을섞지않도록실제참조를제공한다.
const o::SidPool& RawCampaignConstructionBindings::Pool() const { return impl_->core.pool; }
}
