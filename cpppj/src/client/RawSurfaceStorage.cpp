// Client의 실제 배열을 Session보다 오래 보관하고 같은 물리를 두 월드에 중복 설치하지 않는다.
#include "client/RawSurfaceWorld.h"
#include "o/RawVortexOwner.h" //장기신전가상74는같은원문VT표와실제영역owner로분배한다.
#include "client/NativeFortSaveVirtual.h" //완전 형식의 조회기 생성/반납을 Storage 구현에서만 수행한다.
#include "o/RawAssetRegular.h" //현재NativeGeyser의독립실제본문을같은Storage에서사용한다.
#include "o/RawBuriedPostPop.h" //실제오벨리스크전용후처리를같은장기저장소에서선택한다.
#include "o/RawBuriedRegular.h" //같은프레임/난수의기존사건본문을Native기본Host에연결한다.
#include "o/RawContainedLookup.h" //건설재시도의실제현재Form타입조회이며가짜존재값을반환하지않는다.
#include "o/RawDaisProcessData.h" //현재PID의실제6a writer를장기기본직렬화에연결한다.
#include "o/RawGunProcess.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawWindAviaryProcess.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawRainAviaryProcess.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawSunAviaryProcess.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawFlyerProcess.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawBombEffectTimer.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawPathProcess.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawGunAnimation.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawArcherAnimation.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawCrossbowAnimation.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawCannonFire.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawIceCannonFire.h" //실제현재ProcessForm파생writer와같은객체필드를공유한다.
#include "o/RawConstructionData.h" //현재건설객체의실제37 writer를같은공통버퍼에쓴다.
#include "client/GameAssets.h"
#include "client/PriestPlacementAssets.h" // 장기 Canon 소유자의 완전 형식은 구현 파일에서만 필요하다.
#include "o/RawGameAuxState.h"
#include "o/RawContentParentNotice.h" //공통종속의현재VT부모통지를직접선택한다.
#include "o/RawAssetOwner.h" //표면/거주구역등실제가상owner분배다.
#include <bit> //Repop은현재float좌표의원문비트를그대로읽는다.
#include "o/RawSquidPoolReset.h"
#include "client/SquidRenderer.h"
#include <algorithm>
#include <array> //원문baseRegular주소를가리키는고정VT16개의실제메타데이터다.
#include <stdexcept>
#include <utility>
#include <cmath> //공유난수float범위의원문넓은차이를계산한다.
#include <new> //실제Regular할당실패만원문NULL결과로돌린다.

namespace netstorm::client {
namespace {
constexpr std::uint32_t kCapacity=32768; // 기존 실제 월드의 SID 풀 크기이며 다음 대여에서 재확보하지 않는다.
constexpr std::size_t kCells=65536; // 원본256×256 spot 배열의 실제 바이트 수다.
}
// Client 수명의 현재 상세와 진단 장치만 빌리며 짧은 World/Session 포인터를 저장하지 않는다.
void RawSurfaceStorage::BindDisplayDiagnostics(std::optional<std::string>& detail,std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion) { displayAssertionDetail_=&detail;displayAssertion_=std::move(assertion); }
// 00436d20의 문구 복사를 같은 Client 상세 owner에 연결한다. 장치가 없는 독립 저장소는 선택 시 명시 실패한다.
void RawSurfaceStorage::RecordAssertionDetail(std::string_view text) {
    if (!displayAssertionDetail_) throw std::logic_error("오류 상세 Client owner 미연결");*displayAssertionDetail_=std::string(text);
}
//43d40의 현재 타입/shape와 세 인자를 같은 Client 상세 owner에 기록한다.
void RawSurfaceStorage::RecordDisplayFrameError(std::uint32_t type,std::int32_t frame,std::uint32_t abstract) {
    if (!displayAssertionDetail_) throw std::logic_error("표시 오류 상세 Client owner 미연결");*displayAssertionDetail_=o::BuildSquidFrameAssertionDetail(types,shapes,type,frame,abstract);
}
// 진단을 선택한 지점만 장치를 요구하며 복귀 뒤 다음 TYPE 조회를 막지 않는다.
void RawSurfaceStorage::AssertDisplay(std::string_view expression,std::string_view file,std::uint32_t line) const { const auto assertion=displayAssertion_;if (!assertion) throw std::logic_error("표시 원문 진단 장치 미연결");assertion(expression,file,line); } // 진단 안에서 현재 장치를 교체해도 선택된 호출의 수명을 보존한다.
// 실제 Client의 PATCH 선택만 저장하며 World 대여/반납은 이 장기 선택을 초기화하지 않는다.
void RawSurfaceStorage::EnablePatchDestroyDiagnostics() {
    if (pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("PATCH 공통 삭제 Client 판본 불일치");
    patchDestroyDiagnostics_=true;
}
// 현재 장치의 함수 사본을 고정하지 않고 각 삭제 효과가 같은 Client owner를 다시 조회하게 한다.
std::optional<o::SquidDestroyDiagnostics> RawSurfaceStorage::DestroyDiagnostics() const {
    if (!patchDestroyDiagnostics_) return std::nullopt;
    return o::SquidDestroyDiagnostics{
        [this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); },
        [this](auto level,auto text) { TraceGraph(level,text); },
        source_ ? source_->TypeTable().Types() : std::span<const o::RiftTypeRecord>{}}; // 실제 Client의 최초 비용 캐시는 hp.BindLoadedCosts와 같은 불변 GameAssets 표에서 읽는다.
}
// 장기 물리는 현재 Client 장치만 보관하며 실제 소진 전에는 덤프/대화상자를 실행하지 않는다.
void RawSurfaceStorage::BindAllocationDiagnostics(std::function<void()> dump,std::function<void(std::string_view)> tell) { allocationDump_=std::move(dump);allocationTell_=std::move(tell); }
// Client의 현재 로그 서비스를 장기 owner에 보관한다. World 대여마다 다른 Session 포인터로 교체하지 않는다.
void RawSurfaceStorage::BindGraphTrace(std::function<void(std::int32_t,std::string_view)> trace) { graphTrace_=std::move(trace); }
// 매 프레임 동일 게임 난수를 직접 소비하며 Session 또는 별도 난수 사본을 만들지 않는다.
void RawSurfaceStorage::UpdateAmbientWind(o::GameRandom& random) { o::StepAmbientWind(wind,random,pool.Edition(),AngleDiagnostics()); }
// 각도 부모는 같은 현재 checking과 같은 실제 Client 원문 진단 장치를 값 뷰로 보관한다.
o::BoardAngleDiagnostics RawSurfaceStorage::AngleDiagnostics() const { return {checking_,[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); }}; }
// 삭제 Graph의 두 시각 조회가 실제 현재 Client 시계를 읽게 한다.
void RawSurfaceStorage::BindGraphClock(std::function<double()> clock) { graphClock_=std::move(clock); }
// 독립 저장소에는 외부 시계 효과가 없다. 실제 Client는 GameSeconds를 호출한다.
double RawSurfaceStorage::ReadGraphClock() const { return graphClock_ ? graphClock_() : 0.0; }
// 독립 저장소는 기존 무출력 계약을 유지하고 실제 Client는 필터/FILE writer의 원문 계약을 적용한다.
void RawSurfaceStorage::TraceGraph(std::int32_t level,std::string_view text) const { const auto trace=graphTrace_;if (trace) trace(level,text); } // 현재 로그 장치 교체는 다음 호출에 반영하며 실행 중인 함수는 유지한다.
// 참조 배열을 원본 자산에서 최초 한 번만 복사해 이후 주소를 고정한다.
RawSurfaceStorage::RawSurfaceStorage(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> inputTypes,std::span<const o::RiftTypeFrames> inputFrames,std::span<const o::SquidDisplayShape> inputShapes,const GameAssets* source,std::shared_ptr<o::RawAuthorityState> authority,std::shared_ptr<o::RawGameAuxState> gameAux)
    :authority_(authority ? std::move(authority) : std::make_shared<o::RawAuthorityState>()),gameAux_(gameAux ? std::move(gameAux) : std::make_shared<o::RawGameAuxState>()),types(inputTypes.begin(),inputTypes.end()),frames(inputFrames.begin(),inputFrames.end()),shapes(inputShapes.begin(),inputShapes.end()),codes(types.size()),pool(edition,kCapacity,true,&authority_->server),spots(kCells),bridgeDecayScan{edition==o::OriginalEdition::Patch1078 ? -1 : 0,edition==o::OriginalEdition::Patch1078 ? -1 : 0,0,0.0},source_(source) {
    //PATCH의 초기 스캔 first/last는 실제 PE DWORD-1이며 cursor/기한만 BSS0이다. 다른 판본의 새 필드는 종전0을 유지한다.
    if (frames.size()!=types.size() || shapes.size()!=types.size()) throw std::invalid_argument("실제 월드 저장소의 자산 배열 크기 불일치");
    recentGeysers.entries.resize(5); //PE의최초BSS0인최근SID5칸을Client수명에한번확보한다. 실제Populate만count를0으로소비한다.
    o::FinishSquidPoolReset(pool,boardTargetState.invalid); //Pool 생성자가 이미 실행한 하위 Reset 뒤 원문 시작의 기록80DWORD·Y/X 꼬리를 한 번 잇는다.
    links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157; // 기존10.78 표면/초기 생산 경계의 실제 종류다. 월드 설치는 자기 판본의 실제 값을 직접 쓴다.
    for (std::size_t type=0;type<frames.size();++type) codes[type].assign(frames[type].Codes().begin(),frames[type].Codes().end()); // 같은 타입의 실제 프레임 코드만 최초 보관한다.
    pool.BindAllocation({&disallowSquidAllocation,&authority_->boss,&links.bridgeType,
        [this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); },
        [this](o::Sid sid) { Processes().ReclaimForAllocation(sid); },
        [this] { const auto dump=allocationDump_;if (!dump) throw std::logic_error("CD SID 할당의 현재 Client 객체 덤프 미연결");dump(); },
        [this](auto text) { RecordAssertionDetail(text); },
        [this](auto text) { const auto tell=allocationTell_;if (!tell) throw std::logic_error("CD SID 할당의 현재 Client Tell 미연결");tell(text); }}); //긴Pool은같은전역을읽고실제삭제만현재World의짧은대여에서선택한다.
}
// 같은 Template/Core 깊이 owner만 연결하고 현재 원문 DWORD를 새 기본값으로 덮지 않는다.
void RawSurfaceStorage::BindSerializationGlobals(std::shared_ptr<o::FortSerializationGlobals> globals) {
    if (!globals || (serializationGlobals_ && serializationGlobals_!=globals) || (borrowed_ && !serializationGlobals_)) throw std::invalid_argument("표면/Template의동일로딩깊이owner대여계약");serializationGlobals_=std::move(globals);
}
// 실제 Client는 같은005c89b8 주소를 사용하고 독립 표면 fixture만 종전초기1을 유지한다.
std::uint32_t& RawSurfaceStorage::LoadingDepth() { return serializationGlobals_ ? serializationGlobals_->loadingDepth:localLoadingDepth_; }
const std::uint32_t& RawSurfaceStorage::LoadingDepth() const { return serializationGlobals_ ? serializationGlobals_->loadingDepth:localLoadingDepth_; } //동기효과뒤같은현재값의참조다.
// 장기 process owner는 실제 Pool/TYPE가 살아 있는 동안 최종 소유권을 반납한다.
RawSurfaceStorage::~RawSurfaceStorage()=default;
void RawSurfaceStorage::BindChecking(o::RawCheckingFlag checking) { checking_=checking;RefreshChecking(); } //원문값은쓰지않고이미존재하는C++진단view만현재owner에연결한다.
void RawSurfaceStorage::RefreshChecking() {
    rewardState_.debugAsserts=Checking(); // 이미 구성한 장기 HP도 같은 현재 Client 진단 owner를 읽는다.
    if (placementAssets_) placementAssets_->geometry.decoderDiagnostics.checking=Checking(); // 기존 Canon도 바뀐 현재 진단 owner를 직접 읽는다.
    const auto assertion=[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); };
    if (graph_) graph_->BindDiagnostics({Checking(),assertion});if (building_) building_->BindNotifyDiagnostics(Checking(),assertion);
    if (factory_) factory_->BindDiagnostics({types,assertion,[this](auto text) { RecordAssertionDetail(text); },Checking()});if (postPop_) postPop_->BindManaDiagnostics({Checking(),assertion});
    if (unpop_) unpop_->BindDiagnostics({Checking(),types,assertion});if (pop_) pop_->BindDiagnostics({Checking(),types,{},[this] { return ownerMode.challenge; },[this](auto level,auto text) { TraceGraph(level,text); },assertion});
    if (containedUnpop_) containedUnpop_->BindChecking(Checking()); //새World없이도현재진단입력교체를모든기존실행기가읽는다.
}
void RawSurfaceStorage::NoteChanged() { if (const auto current=displayRuntime_.lock();current && current->changed) { const auto runtimePin=current->pin ? current->pin():std::shared_ptr<void>{};current->changed(); } } //기존World의뷰변경bool만설정한다. Native효과/성공값을대신하지않는다.
// 실제 Native 자산만 한 번 변환하며 Player/World/난수/표 초기화는 실행하지 않는다.
PriestPlacementAssets& RawSurfaceStorage::PlacementAssets() {
    if (!UsesNativeTransmissionDevices() || !Source()) throw std::logic_error("장기 Canon의 실제 Native 자산 owner 미연결");
    if (!placementAssets_) {
        auto assets=std::make_unique<PriestPlacementAssets>(*Source());assets->geometry.decoderDiagnostics.checking=Checking();
        assets->geometry.decoderDiagnostics.assertion=[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); }; // 세션 주소 없이 같은 실제 Client 진단으로 정상 복귀한다.
        placementAssets_=std::move(assets);
    }
    return *placementAssets_;
}
// 원문4e2e30의 Player→NULL Source→현재 로컬 제외·대상별 가상+4는 기존 실제 Client 장치가 수행한다.
void RawSurfaceStorage::BroadcastAssetPacket(std::span<std::uint8_t> packet) {
    const auto send=transmissionDevices_.broadcast;if (!send) throw std::logic_error("장기 건설의 실제 Native mutable 방송 미연결");send(packet);
}
void RawSurfaceStorage::BindPostPopDevices(o::SquidPostPopDevices devices) { if (pool.Edition()!=o::OriginalEdition::Patch1078 || !devices.aiNotice || !devices.localOwner || !devices.recheckSelection) throw std::invalid_argument("장기공통후처리의실제Client장치누락");const bool first=!postPopDevices_.aiNotice;if (first && (borrowed_ || postPop_)) throw std::logic_error("Native공통후처리장치는첫World/실행기전에필요");postPopDevices_=std::move(devices);if (first) { postPopState.graphsEnabled=true;postPopState.sharedSuppressed=&ownerMode.challenge; }if (postPop_) postPop_->BindDefaultPatchDevices(postPopDevices_); } //현재장치의함수만보관하며프레임/AI/입력을실행하지않는다.
void RawSurfaceStorage::BindSavedGame(bool& savedGame) { islandPostPopState.sharedSavedGame=&savedGame; } //현재Client주소만빌리며게임값은쓰지않는다.
void RawSurfaceStorage::BindContentCommand(std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> command) { if (!command) throw std::invalid_argument("공통종속의실제입력장치누락");contentCommand_=std::move(command); } //실제선택된UI통지에서만호출한다.
void RawSurfaceStorage::BindContainedUnpop(const std::shared_ptr<o::RawSquidContainedUnpop>& binding) { if (binding && &binding->Pool()!=&pool) throw std::invalid_argument("종속Unpop의같은Pool필요");currentContainedUnpop_=binding; } //Session수명을숨겨소유하지않는다.
o::RawSquidContainedUnpop& RawSurfaceStorage::ContainedUnpop() {
    if (!containedUnpop_) {
        o::SquidContainedUnpopHooks hooks;hooks.parent=[this](auto sid) { return o::CommonContainedParent(pool,sid); };hooks.previous=[this](auto sid) { return o::CommonContainedPrevious(pool,sid); };
        hooks.setPrevious=[this](auto sid,auto previous) { o::CommonContainedSetPrevious(pool,sid,previous); };hooks.parentNotice=[this](auto parent,auto form,auto reason) { o::RawContentParentNotice(pool).Notice(parent,form,reason); };
        hooks.command=[this](auto code,auto x,auto y) { const auto command=contentCommand_;if (!command) throw std::logic_error("공통종속의실제입력장치미연결");command(code,x,y); };hooks.assertion=[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); };
        containedUnpop_=std::make_unique<o::RawSquidContainedUnpop>(pool,o::SquidContainedUnpopState{containedType,containedGumpType,containedUiEvent,Checking(),[this] { return ownerMode.challenge; },[this] { return authority_->boss; }},std::move(hooks));
    }
    return *containedUnpop_;
}
o::SquidPostPop& RawSurfaceStorage::PostPop() {
    if (!postPop_) {
        auto post=std::make_unique<o::SquidPostPop>(pool,types,postPopState,&Graph(),[this](auto sid) { Connections().Connect(sid); });
        if (pool.Edition()==o::OriginalEdition::Patch1078) { post->BindCurrentTypes(types);if (Source()) post->BindLoadedCosts(Source()->TypeTable().Types()); }
        if (postPopDevices_.aiNotice && pool.Edition()==o::OriginalEdition::Patch1078) { post->BindDefaultPatchDevices(postPopDevices_);post->BindDefaultResourcePostPop([this](auto sid,auto flags) { DefaultResourcePostPop(sid,flags); }); }
        post->BindManaDiagnostics({Checking(),[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); }});
        post->SetIslandPrefix([this](auto sid,auto flags) { Connections().IslandPostPopPrefix(sid,flags); });post->SetSurfacePrefix([this](auto sid,auto flags) { Surface().Prefix(sid,flags); });postPop_=std::move(post);
    }
    return *postPop_;
}
o::SquidUnpop& RawSurfaceStorage::Unpop() {
    if (!unpop_) {
        auto unpop=std::make_unique<o::SquidUnpop>(pool,hash,spots,&Display());unpop->BindDiagnostics({Checking(),types,[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); }});
        unpop->BindContainedUnpop([this](auto sid) { if (const auto current=currentContainedUnpop_.lock()) current->Unpop(sid);else ContainedUnpop().Unpop(sid); });unpop->SetBuildingSpace(&Building());unpop_=std::move(unpop);
    }
    return *unpop_;
}
void RawSurfaceStorage::BindPopRuntime(const std::shared_ptr<const RawSurfacePopBindings>& runtime) { const auto current=popRuntime_.lock();if (!runtime || (current && current!=runtime) || !runtime->transmit) throw std::invalid_argument("동일실제Pop writer대여필요");popRuntime_=runtime; } //생성/전송/ACK를실행하지않는다.
void RawSurfaceStorage::BindResourceDevices(RawSurfaceResourceDevices devices) { //현재Client장치를최초보관하며공장/Host/원문값을조기구성하지않는다.
    if (!UsesNativeTransmissionDevices() || resourceDevices_.random || resourceCarrier_ || !devices.random || !devices.carrierCheck || !devices.geyserRegistration || !devices.damageableRegular || !devices.startConstruction || !devices.destroy) throw std::invalid_argument("장기자원PostPop의실제최초Client장치계약");resourceDevices_=std::move(devices);
}
o::RawCarrierPostPop& RawSurfaceStorage::ResourceCarrierPostPop() { //실제공통부모를World없이같은물리/현재전역으로구성한다.
    if (!resourceCarrier_) {
        if (!serializationGlobals_) throw std::logic_error("장기자원후처리의실제Template로딩owner미연결");resourceCarrierState_.sharedBoss=&authority_->boss;resourceCarrierState_.sharedLoadingDepth=&LoadingDepth();resourceCarrierState_.sharedGeyserType=&gameAux_->GeyserType();
        auto hooks=o::MakeCarrierPostPopHooks(pool,PostPop(),[this](auto sid) { const auto check=resourceDevices_.carrierCheck;if (!check) throw std::logic_error("장기Carrier의현재가상확인미연결");check(sid); },[this](auto x,auto y,auto owner) { Surface().SetSupportOwner(x,y,owner); });
        auto carrier=std::make_unique<o::RawCarrierPostPop>(pool,types,resourceCarrierState_,std::move(hooks));carrier->BindCurrentTypes(types);resourceCarrier_=std::move(carrier);
    }return *resourceCarrier_;
}
void RawSurfaceStorage::BindBuriedResource(std::shared_ptr<o::GameRandom> random,std::function<std::uint32_t(std::uint32_t)> choose) { //실제owner만처음연결하며게임상태를조회하거나초기화하지않는다.
    if (!UsesNativeTransmissionDevices() || !random || !choose || buriedRandom_ || buriedPostPop_ || buriedRegular_) throw std::invalid_argument("장기 오벨리스크의 실제 최초 난수/보상 owner 필요");buriedRandom_=std::move(random);buriedChoose_=std::move(choose);
}
o::RawBuriedPostPop& RawSurfaceStorage::BuriedPostPop() { //미선택추첨은요구하지않고두new의각성공/실패를실제Host로연결한다.
    if (!serializationGlobals_) throw std::logic_error("오벨리스크의 실제 장기 로딩 owner 미결합");
    if (!buriedPostPop_) {
        o::BuriedPostPopHooks hooks;hooks.chooseType=[this](auto value) { const auto choose=buriedChoose_;if (!choose) throw std::logic_error("오벨리스크의 실제 현재 보상 선택 미결합");return choose(value); };
        hooks.reserveRegular=[this] { return AllocateResourceRegular(); };hooks.addRegular=[this](auto sid,auto event,auto payload) { ConstructResourceRegular(sid,event,payload); };
        hooks.validateBase=[this](auto sid,auto flags,const auto* projected) { PostPop().ValidateBase(sid,flags,projected); };hooks.base=[this](auto sid,auto flags) { PostPop().PostPopBase(sid,flags); };
        hooks.notifySurface=[this](auto sid) { Building().NotifySurface(sid); };
        buriedPostPop_=std::make_unique<o::RawBuriedPostPop>(pool,authority_->boss,LoadingDepth(),std::move(hooks),links.bridgeType);
    }return *buriedPostPop_;
}
o::RawBuriedRegular& RawSurfaceStorage::BuriedRegular() { //다음사건도같은현재TYPE/Frame/난수를읽으며Session대여를요구하지않는다.
    if (!buriedRandom_) throw std::logic_error("오벨리스크의 실제 장기 난수 owner 미결합");
    if (!buriedRegular_) buriedRegular_=std::make_unique<o::RawBuriedRegular>(pool,frames,Frame(),*buriedRandom_);return *buriedRegular_;
}
bool RawSurfaceStorage::AllocateResourceRegular() { //같은new28의실패를그시점에서만반환한다.
    if (resourceRegular_) throw std::logic_error("장기자원Regular의확보메모리중복");try { resourceRegular_=std::make_unique<o::RegularProcess>(0,0.0F); }catch (const std::bad_alloc&) { return false; }return true;
}
void RawSurfaceStorage::ConstructResourceRegular(o::Sid sid,std::uint32_t event,float payload) { //두번째할당없이동일확보주소에서496e80과현재시각을소비한다.
    if (!resourceRegular_) throw std::logic_error("장기자원Regular의실제확보메모리누락");auto* reserved=resourceRegular_.get();std::destroy_at(reserved);std::construct_at(reserved,event,payload);auto* attached=Processes().Host().AttachRegular(std::move(resourceRegular_),sid);if (attached) attached->ResetTask(event,Processes().State().Now());
}
float RawSurfaceStorage::ResourceDamageableRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) { //일반사건의payload반환은원문496e70을직접실행하고큰세계효과를미리요구하지않는다.
    if (event==gameAux_->DamageRegularEvent()) { const auto hit=resourceDevices_.damageableRegular;if (!hit) throw std::logic_error("장기Damageable의실제피해사건미연결");return hit(sid,event,count,payload); }
    if (event==gameAux_->ConstructionRegularEvent()) {
        if (pool.Slot(sid)[40]&1U) { o::RawContainedLookup lookup(pool,[this](auto object,auto flags) { const auto destroy=resourceDevices_.destroy;if (!destroy) throw std::logic_error("장기Form의현재가상삭제미연결");destroy(object,flags); });
            if (!lookup.Find(sid,gameAux_->ConstructionProcessType()).value) { const auto start=resourceDevices_.startConstruction;if (!start) throw std::logic_error("장기Damageable의실제건설재시도미연결");start(sid); }
        }return 0.0F;
    }return payload;
}
o::RawNuggetRegular& RawSurfaceStorage::ResourceNuggetRegular() { //같은실제현재게임난수/프레임/공간과PostPop의확보메모리를공유한다.
    if (!resourceNuggetRegular_) { o::NuggetRegularHooks hooks;hooks.setFrame=[this](auto sid,auto frame,auto flags) { Frame().Set(sid,frame,flags); };hooks.refresh=[this](auto sid) { Display().Update(pool.Slot(sid)); };hooks.allocate=[this] { return AllocateResourceRegular(); };
        hooks.randomInteger=[this](auto low,auto high) { const auto read=resourceDevices_.random;if (!read) throw std::logic_error("장기너겟왕복의실제난수미연결");return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(low)+read().Next(static_cast<std::uint32_t>(high)-static_cast<std::uint32_t>(low))); };
        hooks.construct=[this](auto sid,auto event,auto payload) { ConstructResourceRegular(sid,event,payload); };hooks.destroy=[this](auto sid,auto flags) { DestroyAsset(sid,flags); };hooks.unpop=[this](auto sid,auto flags) { UnpopAsset(sid,flags); };hooks.pop=[this](auto sid,auto x,auto y,auto flags) { PopAsset(sid,x,y,flags); }; //실제너겟소진도현재World또는같은장기가상삭제를선택한다.
        hooks.damageable=[this](auto sid,auto event,auto count,auto payload) { return ResourceDamageableRegular(sid,event,count,payload); };hooks.aFrameCount=[this](auto type) { if (type>=frames.size()) throw std::out_of_range("장기너겟현재A프레임TYPE물리부족");return frames[type].Run('A').count; };resourceNuggetRegular_=std::make_unique<o::RawNuggetRegular>(pool,std::span<const std::int32_t>{},spots,authority_->boss,std::move(hooks));
    }return *resourceNuggetRegular_;
}
void RawSurfaceStorage::DefaultResourcePostPop(o::Sid sid,std::uint32_t flags) { //보호PE의실제현재+20만선택하고전용미복원을base성공으로넘기지않는다.
    const auto raw=pool.Slot(sid);std::uint32_t table{};for (std::size_t byte=0;byte<4;++byte) table|=std::uint32_t(raw[byte])<<(byte*8); //현재물리VT4BYTE만해석한다.
    if (table==0x503d80U || table==0x503e50U || table==0x50e568U) { ResourceCarrierPostPop().PostPop(sid,flags);return; }
    if (table==0x5040c0U || table==0x504198U) { BalloonBody().PostPop(sid,flags);return; } // 같은 실제 그림자 생성·표시와 Carrier/Damageable 부모다.
    if (table==0x50a820U || table==0x50a8e8U) { ResourceCarrierPostPop().DamageablePostPop(sid,flags);return; }
    if (table==0x50a748U) { BuriedPostPop().PostPop(sid,flags);return; } //실제현재+20=462170을선택하고공통성공으로넘기지않는다.
    if (table==0x50e888U) { IceTower().PostPop(sid,flags);return; } //성장형의실제48ead0은정기예약뒤같은공통PostPop만호출한다.
    if (table==0x509470U) { FencePostPop().PostPop(sid,flags);return; } //전체457560접두뒤같은Damageable부모를호출한다.
    if (table==0x50dc30U) {
        if (!resourceNugget_) { o::NuggetPostPopHooks hooks;hooks.hasRegular=[this](auto parent,auto event) { return Processes().Host().FindEvent(parent,event)!=nullptr; };hooks.allocate=[this] { return AllocateResourceRegular(); };
            hooks.random=[this](float low,float high) { const auto read=resourceDevices_.random;if (!read) throw std::logic_error("장기너겟의실제게임난수미연결");return static_cast<float>(static_cast<double>(read().Next(65536))*(1.0/65536.0)*std::abs(static_cast<double>(high)-low)+low); };
            hooks.construct=[this](auto parent,auto event,auto payload) { ConstructResourceRegular(parent,event,payload); };hooks.damageable=[this](auto parent,auto value) { ResourceCarrierPostPop().DamageablePostPop(parent,value); };resourceNugget_=std::make_unique<o::RawNuggetPostPop>(pool,std::move(hooks));
        }resourceNugget_->PostPop(sid,flags);return;
    }
    if (table==0x50da88U) {
        if (!resourceGeyser_) { resourceAssetState_.sharedGeysers=&sidLists.geysers;resourceAssetState_.currentBridgeType=&links.bridgeType;resourceAssetState_.currentIslandType=&links.islandType;resourceAssetState_.currentStalagType=&links.stalagType;
            o::AssetPostPopHooks hooks;hooks.reserveRegular=[this] { return AllocateResourceRegular(); };hooks.addRegular=[this](auto parent,auto event,auto payload) { ConstructResourceRegular(parent,event,payload); };
            hooks.geyserRegistration=[this](auto parent,auto operation) { const auto registration=resourceDevices_.geyserRegistration;if (!registration) throw std::logic_error("장기간헐천의현재PlayerAI등록미연결");registration(parent,operation); };hooks.notifySurface=[this](auto parent) { Building().NotifySurface(parent); };
            hooks.unpop=[this](auto parent,auto value) { UnpopAsset(parent,value); };hooks.repop=[this](auto parent,auto value) { RepopAsset(parent,value); };hooks.visitAtPosition=o::MakeAssetPostPopFinder(pool,hash,types,true);
            resourceGeyser_=std::make_unique<o::RawAssetPostPop>(pool,types,PostPop(),ResourceCarrierPostPop(),resourceAssetState_,std::move(hooks));
        }resourceGeyser_->GeyserPostPop(sid,flags);return;
    }
    throw std::logic_error("World없는현재VT의전용자원PostPop몸체미설치");
}
void RawSurfaceStorage::BindProcessDevices(RawSurfaceProcessDevices devices) { //실제장기선택장치만초기연결하며게임전역값은쓰지않는다.
    if (!UsesNativeTransmissionDevices() || borrowed_ || processes_ || baseDestroy_ || processDevices_.selected || !devices.selected || !devices.clearSelection) throw std::invalid_argument("장기Process기본선택장치의최초Native계약");processDevices_=std::move(devices);
}
std::uint32_t RawSurfaceStorage::CurrentProcessSelection() const { //앞선진단/전송효과뒤현재Client선택을잘림없이다시읽는다.
    const auto selected=processDevices_.selected;if (!selected) throw std::logic_error("장기Process삭제의현재선택조회미연결");return selected();
}
o::RawSquidDestroy& RawSurfaceStorage::BaseDestroy() { //Native의같은전역pre/post깊이와현재Pool/Unpop/TYPE를공유한다.
    if (!baseDestroy_) { auto destroy=std::make_unique<o::RawSquidDestroy>(pool,Unpop(),types);if (auto diagnostics=DestroyDiagnostics()) destroy->BindPatchDiagnostics(std::move(*diagnostics));baseDestroy_=std::move(destroy); }return *baseDestroy_;
}
o::SquidDestroyHooks RawSurfaceStorage::DefaultProcessAssetHooks() { //같은장기부모에서현재VT의실제삭제효과를선택한다.
    o::SquidDestroyHooks hooks;hooks.selectedRaw=[this] { return CurrentProcessSelection(); };hooks.unpopForm=[this](auto sid) { Factory().UnpopForm(sid); };
    hooks.emit=[this](const o::SquidDestroyEvent& event) { if (event.effect==o::SquidDestroyEffect::Transmit) { Transmission().Transmit(event.sid,-1,-2,0,event.flags);return; }if (event.effect==o::SquidDestroyEffect::ClearSelection) { const auto clear=processDevices_.clearSelection;if (!clear) throw std::logic_error("장기Process삭제의실제선택정리미연결");clear();return; }EmitDefaultAssetEffect(event); };return hooks;
}
o::SquidFormDispatch& RawSurfaceStorage::ProcessFormDispatch() { //현재World가없어도같은실제Host와공통/다리의가상10을결합한다.
    if (!processForms_) { auto forms=std::make_unique<o::SquidFormDispatch>(pool,BaseDestroy(),DefaultProcessAssetHooks(),[this](auto sid,auto flags,const auto& hooks) { return VirtualDestroy().Destroy(sid,flags,hooks); });forms->Bind(Processes().Host());processForms_=std::move(forms); }return *processForms_;
}
float RawSurfaceStorage::HandleDefaultRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) { //현재VT의실제+5c주소가base496e70인클래스만원문payload반환을실행한다.
    constexpr std::array<std::uint32_t,16> kBaseRegularVtables{0x00501dd0U,0x00503360U,0x00506d98U,0x00506e50U,0x00506f08U,0x005070c0U,0x005092d0U,0x00509388U,0x00509b98U,0x0050c3b0U,0x0050c9f0U,0x0050db78U,0x0050eb98U,0x00512098U,0x00512110U,0x00512188U}; //보호PE87VT를조회해+5c=00496e70인16개만보존한불변메타데이터다.
    const auto raw=pool.Slot(sid);std::uint32_t table{};for (std::size_t byte=0;byte<4;++byte) table|=std::uint32_t(raw[byte])<<(byte*8); //가상선택은현재VT4BYTE이며현재TYPE별추측으로바꾸지않는다.
    if (table==0x5034c8U) return BridgeRegular().Handle(sid,event,count,payload); // 실제422740은 World와 같은 장기 본문/현재 전역을 사용한다.
    if (table==0x50da88U) return o::RunNativeGeyserRegular(pool,types,sid,event,count,payload,[this](auto object,auto flags) { Display().Update(pool.Slot(object),flags); },[this](auto object,auto code,auto times,auto value) { return ResourceDamageableRegular(object,code,times,value); });
    if (table==0x50dc30U) return ResourceNuggetRegular().Handle(sid,event,count,payload);
    if (table==0x504198U) return BalloonRegular(sid,event,count,payload); // 사건3은 실제 프레임, 다른 사건은 현재 큰 Carrier 부모를 요구한다.
    if (table==0x50a748U) return BuriedRegular().Handle(sid,event,count,payload); //실제+5c=4622f0은사건1도원문payload를반환한다.
    if (table==0x50e888U) return IceTower().Handle(sid,event,count,payload); //실제48ebe0의성장사건/현재프레임/권한을같은장기본문으로보낸다.
    if (table==0x509470U) return FenceRegular().Handle(sid,event,count,payload); //같은장기본체가실제Host재진입도처리한다.
    constexpr std::array<std::uint32_t,14> kDamageableRegularVtables{0x00501ea0U,0x00509930U,0x005099e8U,0x00509aa0U,0x0050a1e8U,0x0050a388U,0x0050b6e8U,0x0050b7c0U,0x0050b888U,0x0050b9b8U,0x0050bad0U,0x0050e940U,0x0050ea08U,0x0050ead0U}; //보호PE의현재VT+5c=0044b900인14클래스만같은원문부모를사용한다.
    if (std::find(kDamageableRegularVtables.begin(),kDamageableRegularVtables.end(),table)!=kDamageableRegularVtables.end()) return ResourceDamageableRegular(sid,event,count,payload);
    if (std::find(kBaseRegularVtables.begin(),kBaseRegularVtables.end(),table)!=kBaseRegularVtables.end()) return payload; //496e70은event/count를읽지않는실제base본문이다.
    static_cast<void>(event);static_cast<void>(count);throw std::logic_error("World없는현재VT의전용Regular몸체미설치"); //Bridge/Geyser/Nugget전용을base반환으로가장하지않는다.
}
void RawSurfaceStorage::BindTransmissionDevices(RawSurfaceTransmissionDevices devices) { //실제장치를첫writer/World앞에서고정하며선택효과는발생시키지않는다.
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || borrowed_ || factory_ || processes_ || transmission_ || !devices.broadcast || !devices.connected || !devices.send) throw std::invalid_argument("장기Squid전송의실제Client장치계약");transmissionDevices_=std::move(devices);
}
o::RawSquidTransmission& RawSurfaceStorage::Transmission() { //같은현재TYPE/공장/진단과공통예약을빌리며Acquire/reset/권한변경을하지않는다.
    if (!transmission_) {
        o::RawSquidTransmissionHooks hooks;hooks.type=[this](auto number)->const o::RiftTypeRecord& { return types.at(number); };hooks.defaultType=[this] { return links.bridgeType; };hooks.maxHp=[this](auto number) { return Factory().ReadSelectedHitPoints(types.at(number),types); };
        hooks.trace=[this](auto level,auto text) { TraceGraph(level,text); };hooks.assertion=[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); };
        hooks.connected=[this](auto target) { if (transmissionDevices_.connected) return transmissionDevices_.connected(target);const auto current=popRuntime_.lock();if (!current || !current->connected) throw std::logic_error("독립Squid의현재Player장치미연결");const auto runtimeOwner=current->pin ? current->pin():std::shared_ptr<void>{};return current->connected(target); };
        if (transmissionDevices_.broadcast) { hooks.broadcastMutable=transmissionDevices_.broadcast;hooks.sendMutable=transmissionDevices_.send; } //실제Native만전체예약을observer에보내며기존typed const관측계약은유지한다.
        else { hooks.broadcast=[this](auto packet) { const auto current=popRuntime_.lock();if (!current || !current->broadcast) throw std::logic_error("독립Squid의실제방송관측미연결");const auto runtimeOwner=current->pin ? current->pin():std::shared_ptr<void>{};const auto send=current->broadcast;send(packet); };hooks.send=[this](auto target,auto packet) { const auto current=popRuntime_.lock();if (!current || !current->send) throw std::logic_error("독립Squid의실제대상송신미연결");const auto runtimeOwner=current->pin ? current->pin():std::shared_ptr<void>{};const auto send=current->send;send(target,packet); }; }
        hooks.processPacket=[this](auto form,auto packet,auto argument) { const auto current=popRuntime_.lock();if (current && current->processPacket) { const auto runtimeOwner=current->pin ? current->pin():std::shared_ptr<void>{};return current->processPacket(form,packet,argument); }return Processes().Host().WriteFormPacket(form,argument,packet,gameAux_->SquidTransmission()); }; //외부Kernel대여가없으면동일장기Host의실제PID를조회한다.
        transmission_=std::make_unique<o::RawSquidTransmission>(pool,gameAux_->SquidTransmission(),std::move(hooks));
    }return *transmission_;
}
void RawSurfaceStorage::BroadcastProcessForm(std::int32_t length) { //선언WORD/observer를미리수정하지않고원문양수반환뒤실제broadcast를선택한다.
    auto& buffer=gameAux_->SquidTransmission().buffer;if (length<0 || static_cast<std::size_t>(length)>buffer.size()) throw std::out_of_range("현재ProcessForm의공통송신예약범위");if (!transmissionDevices_.broadcast) throw std::logic_error("현재ProcessForm의실제Native방송장치미연결");if (length>0) { const auto send=transmissionDevices_.broadcast;send(buffer); }
}
void RawSurfaceStorage::TransmitPop(o::Sid sid,std::uint32_t flags) { const auto current=popRuntime_.lock();if (current && current->transmit) { const auto runtimeOwner=current->pin ? current->pin():std::shared_ptr<void>{};current->transmit(sid,flags); }else Transmission().Pop(sid,flags); } //공간/장부뒤실제writer를실행하고NULL대상은기존Registry선택에서자연반환한다.
std::int32_t RawSurfaceStorage::WriteDefaultForm(o::SquidProcess& process,bool parentChanged,std::uint32_t force) { return SerializeProcessForm(process,gameAux_->SquidTransmission().buffer,parentChanged,force); } //World없는현재Host도같은2048BYTE와실제공통분배를쓴다.
std::int32_t RawSurfaceStorage::SerializeProcessForm(o::SquidProcess& process,std::span<std::uint8_t> buffer,bool parentChanged,std::uint32_t force) { //현재원본헤더/실제PID객체/미작성tail을사용하고자체송신이나성공값을만들지않는다.
    if (auto* dais=dynamic_cast<o::DaisProcess*>(&process)) { const auto header=gameAux_->DaisProcessHeader();return dais->WritePacket(buffer,parentChanged,force!=0,header[1],header[0]); }
    if (auto* construction=dynamic_cast<o::ConstructionProcess*>(&process)) { const auto header=gameAux_->ConstructionHeader();return o::RawConstructionData::WriteProcess(*construction,buffer,parentChanged,force!=0,{header[1],header[0]}); }
    if (auto* gun=dynamic_cast<o::GunProcess*>(&process)) { const auto header=gameAux_->GunProcessHeader();return gun->WritePacket(buffer,parentChanged,force!=0,header[1],header[0]); }
    if (auto* current=dynamic_cast<o::WindAviaryProcess*>(&process)) { const auto header=gameAux_->WindAviaryHeader();return current->WritePacket(buffer,parentChanged,force!=0,header[1],header[0]); }
    if (auto* current=dynamic_cast<o::RainAviaryProcess*>(&process)) { const auto header=gameAux_->RainAviaryHeader();return current->WritePacket(buffer,parentChanged,force!=0,header[1],header[0]); }
    if (auto* current=dynamic_cast<o::SunAviaryProcess*>(&process)) { const auto header=gameAux_->SunAviaryHeader();return current->WritePacket(buffer,parentChanged,force!=0,header[1],header[0]); }
    if (auto* current=dynamic_cast<o::FlyerProcess*>(&process)) { const auto header=gameAux_->FlyerProcessHeader();return current->WritePacket(buffer,parentChanged,force!=0,header[1],header[0]); }
    if (auto* timer=dynamic_cast<o::BombEffectTimerProcess*>(&process)) { const auto header=gameAux_->BombTimerHeader();return static_cast<std::int32_t>(timer->WritePacket(buffer,gameAux_->BombTimerInitialLength(),header[1],header[0])); }
    if (auto* path=dynamic_cast<o::RawPathProcess*>(&process)) {
        if (&path->Pool()!=&pool) throw std::invalid_argument("장기Path writer의실제Pool정체성불일치"); //호스트소유권만검증하며Form/Parent값자격검사는추가하지않는다.
        o::RawPathDataWriteHooks hooks;hooks.parentGenus=[this](auto parent) { return types.at(pool.Slot(parent)[10]).flags2; };hooks.assertion=[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); };hooks.trace=[this](auto text) { TraceGraph(gameAux_->PathTraceLevel(),text); };
        return o::RawPathData(pool.Edition()).WriteCurrent(path->Runtime(),process.Form(),process.Parent(),gameAux_->PathPacketHeader(),buffer,hooks); //실제prefix뒤현재packet+6/TYPE+ec를읽으며force/parentChanged는원문처럼사용하지않는다.
    }
    std::span<std::uint8_t> animation; //동일outer접두를실제소유하는확인된다섯클래스만선택한다.
    if (auto* gunAnimation=dynamic_cast<o::GunAnimation*>(&process)) animation=gunAnimation->Fields().bytes;
    else if (auto* cannonFire=dynamic_cast<o::CannonFire*>(&process)) animation=cannonFire->Fields().bytes;
    else if (auto* archerAnimation=dynamic_cast<o::ArcherAnimation*>(&process)) animation=archerAnimation->Fields().bytes;
    else if (auto* crossbowAnimation=dynamic_cast<o::CrossbowAnimation*>(&process)) animation=crossbowAnimation->Fields().bytes;
    else if (auto* iceFire=dynamic_cast<o::IceCannonFire*>(&process)) animation=iceFire->Fields().common.bytes;
    if (!animation.empty()) { process.OnAttached();const auto header=o::GunAnimationUsesFirePacket(animation) ? gameAux_->FireAnimationHeader():gameAux_->RotateAnimationHeader();return o::WriteGunAnimationProcessData(animation,buffer,parentChanged,force!=0,header[0],header[1]); } //이들OnAttached는Base Form/Parent DWORD동기화만하며실제가상writer의현재입력을만든다.
    throw std::logic_error("선택된ProcessForm의실제파생writer미연결"); //모르는파생을빈writer로가장하지않는다.
}
o::SquidPop& RawSurfaceStorage::Pop() {
    if (!pop_) {
        auto pop=std::make_unique<o::SquidPop>(pool,hash,spots,&Display(),&PostPop());pop->BindDiagnostics({Checking(),types,{},[this] { return ownerMode.challenge; },[this](auto level,auto text) { TraceGraph(level,text); },[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); }});
        pop->SetBuildingSpace(&Building());if (pool.Edition()==o::OriginalEdition::Patch1078) pop->BindTransmission([this](auto sid,auto flags) { TransmitPop(sid,flags); });pop_=std::move(pop);
    }
    return *pop_;
}
o::SquidFrame& RawSurfaceStorage::Frame() { if (!frame_) { auto frame=std::make_unique<o::SquidFrame>(pool,types,o::MakeSquidFrameHooks(pool,types,Display(),Unpop(),Pop()));if (pool.Edition()==o::OriginalEdition::Patch1078) frame->BindCurrentTypes(types);frame_=std::move(frame); }return *frame_; } //같은세실행기를빌리며새공간상태는없다.
//장기물리조회뒤기존World와동일한공통Unpop을수행하며실제하위대여의pin은각실행기가선택한다.
void RawSurfaceStorage::UnpopAsset(o::Sid sid,std::uint32_t flags) { Unpop().Unpop(sid,types.at(pool.Slot(sid)[10]),flags);NoteChanged(); }
//기존World와같은가상owner부모를공유하여생산dirty·섬받침/종유석과공통접두분배를보존한다.
void RawSurfaceStorage::SetAssetOwner(o::Sid sid,std::uint32_t owner) {
    const auto set=o::MakeAssetOwnerDispatch(pool,links,postPopState.productionDirty,[this](auto object,auto player) { //표면특수부모뒤실제현재신전가상74를분배한다.
        if (UsesNativeTransmissionDevices() && o::HandlesRawVortexOwner(pool,object)) { const auto body=vortexOwner_;if (!body) throw std::logic_error("장기 신전의 실제 가상 소유자 미결합");body(object,player); }
        else Owner().Set(object,player);
    });set(sid,owner);NoteChanged();
}
o::RawPopResult RawSurfaceStorage::PopAsset(o::Sid sid,float x,float y,std::uint32_t flags) {
    if (pool.Edition()==o::OriginalEdition::Patch1078) { const auto result=Pop().PopCurrent(sid,x,y,flags);if (result!=o::RawPopResult::Unchanged) NoteChanged();return result; }
    const auto raw=pool.Slot(sid);const auto size=Display().FrameSize(pool.Edition(),raw,raw[34]);const auto result=Pop().Pop(sid,types[raw[10]],size[0],size[1],x,y,flags);if (result!=o::RawPopResult::Unchanged) NoteChanged();return result; //CD의기존BYTE프레임/사전크기계약이다.
}
void RawSurfaceStorage::RepopAsset(o::Sid sid,std::uint32_t flags) { const auto raw=pool.Slot(sid);const auto read=[raw](std::size_t offset) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(raw[offset+byte])<<(byte*8);return std::bit_cast<float>(value); };static_cast<void>(PopAsset(sid,read(14),read(18),flags)); } //현재슬롯의정확한두float만읽는다.
o::BridgeConnectHooks RawSurfaceStorage::SurfaceObjects() {
    o::BridgeConnectHooks hooks;hooks.create=[this](auto type,auto flags) { NoteChanged();return Factory().Create(type,flags); };
    const auto setOwner=o::MakeAssetOwnerDispatch(pool,links,postPopState.productionDirty,[this](auto sid,auto owner) { Owner().Set(sid,owner); });hooks.setOwner=[this,setOwner](auto sid,auto owner) { setOwner(sid,owner);NoteChanged(); };hooks.pop=[this](auto sid,auto x,auto y,auto flags) { static_cast<void>(PopAsset(sid,x,y,flags)); };
    hooks.setFrame=[this](auto sid,auto frame,auto flags) { Frame().Set(sid,frame,flags);NoteChanged(); };hooks.notifySurface=[this](auto sid) { Building().NotifySurface(sid); };return hooks;
}
o::RawBridgeConnect& RawSurfaceStorage::Connections() { if (!connections_) connections_=std::make_unique<o::RawBridgeConnect>(pool,Neighbors(),links,SurfaceObjects());return *connections_; } //현재모든연결/owner효과는같은Factory/Pop을사용한다.
o::RawIslandPostPop& RawSurfaceStorage::Surface() {
    if (!surface_) {
        o::IslandPostPopHooks hooks;hooks.objects=SurfaceObjects();hooks.connect=[this](auto sid) { Connections().Connect(sid); };hooks.findTypeAt=[this](auto x,auto y,auto type) { return Connections().FindTypeAt(x,y,type); };
        hooks.updateDisplay=[this](auto sid) { Display().Update(pool.Slot(sid)); };hooks.incompleteSupport=[](bool,bool,bool) { throw std::logic_error("현재표면의불완전한받침묶음"); };
        hooks.assertion=[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); };hooks.detail=[this](auto text) { RecordAssertionDetail(text); };hooks.typeWas=[this] { return gameAux_->SupportTypeWas(); };
        surface_=std::make_unique<o::RawIslandPostPop>(pool,hash.Entries(0),types,frames,links,islandPostPopState,std::move(hooks));
    }
    return *surface_;
}
// Graph의실제공유기록/스택을빌리며생성시현재배열을Reset하거나새월드를대여하지않는다.
o::RawGraph& RawSurfaceStorage::Graph() {
    if (!graph_) {
        auto runtimeGraph=std::make_unique<o::RawGraph>(pool,hash,spots,types,codes,this->graph,o::GraphRecovery::FullPool);
        if (pool.Edition()==o::OriginalEdition::Patch1078) runtimeGraph->BindCurrentTypes(types);
        runtimeGraph->BindDiagnostics({Checking(),[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); }});
        runtimeGraph->BindTrace([this](auto level,auto text) { TraceGraph(level,text); });runtimeGraph->BindDetachState(postPopState.graphsEnabled,deletionState.rebuildGraph);
        runtimeGraph->BindClock([this] { return ReadGraphClock(); });graph_=std::move(runtimeGraph);
    }
    return *graph_;
}
o::RawSquidNeighbors& RawSurfaceStorage::Neighbors() {
    if (!neighbors_) { auto neighbors=std::make_unique<o::RawSquidNeighbors>(pool,hash,spots,types,frames);if (pool.Edition()==o::OriginalEdition::Patch1078) neighbors->BindCurrentTypes(types);neighbors_=std::move(neighbors); } //실제현재발자국만빌리고별도spot/Hash를만들지않는다.
    return *neighbors_;
}
o::RawBuildingSpace& RawSurfaceStorage::Building() {
    if (!building_) {
        auto building=std::make_unique<o::RawBuildingSpace>(pool,hash,types,buildingSpaceState);
        if (UsesNativePostPopDevices()) building->BindCurrentIslandType(gameAux_->TerrainType()); //실제Client만현재005411dc를읽으며독립terrain인자계약은보존한다.
        building->BindCurrentBridgeType(links.bridgeType);building->BindNotifyDiagnostics(Checking(),[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); });building_=std::move(building);
    }
    return *building_;
}
o::SquidOwner& RawSurfaceStorage::Owner() {
    if (!owner_) { auto owner=std::make_unique<o::SquidOwner>(pool,types,postPopState,ownerMode);owner->BindDiagnostics({types,[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); }});owner_=std::move(owner); } //공통owner는실제Player장부/현재모드와같은주소만쓴다.
    return *owner_;
}
// 실제Client장치는현재owner를조회하는함수만보관하며이결합이화면/카메라를진행하지않는다.
void RawSurfaceStorage::BindDisplayDevices(RawSurfaceDisplayBindings devices) {
    if (!devices.view || !devices.suppressed || !devices.invalidate) throw std::invalid_argument("표면표시의현재Client장치누락");displayDevices_=std::move(devices);
}
void RawSurfaceStorage::BindDisplayRuntime(const std::shared_ptr<const RawSurfaceDisplayBindings>& runtime) { //실행중인같은대여가있으면새World가덮지못한다.
    const auto current=displayRuntime_.lock();if (!runtime || (current && current!=runtime) || !runtime->view || !runtime->suppressed || !runtime->invalidate) throw std::invalid_argument("표면표시의같은World대여필요");displayRuntime_=runtime;
}
o::SquidDisplayView RawSurfaceStorage::CurrentDisplayView() const { const auto current=displayRuntime_.lock();const auto runtimePin=current && current->pin ? current->pin():std::shared_ptr<void>{};const auto read=current ? current->view:displayDevices_.view;if (!read) throw std::logic_error("표면표시의현재카메라/지도장치미연결");return read(); } //실제getter를같은선택호출동안pin한다.
bool RawSurfaceStorage::Suppressed() const { const auto current=displayRuntime_.lock();const auto runtimePin=current && current->pin ? current->pin():std::shared_ptr<void>{};const auto read=current ? current->suppressed:displayDevices_.suppressed;if (!read) throw std::logic_error("표면표시의실제전체갱신관문미연결");return read(); }
void RawSurfaceStorage::Invalidate(o::SquidDisplayRect rectangle,std::uint32_t flags) { const auto current=displayRuntime_.lock();const auto runtimePin=current && current->pin ? current->pin():std::shared_ptr<void>{};const auto write=current ? current->invalidate:displayDevices_.invalidate;if (!write) throw std::logic_error("표면표시의실제dirty장치미연결");write(rectangle,flags); }
void RawSurfaceStorage::FrameError(std::uint32_t type,std::int32_t frame,std::uint32_t abstract) { RecordDisplayFrameError(type,frame,abstract); } //원문상세를같은장기owner에기록한다.
void RawSurfaceStorage::Assertion(std::string_view expression,std::string_view file,std::uint32_t line) { AssertDisplay(expression,file,line); } //현재진단정상복귀를기존표시부모에반환한다.
void RawSurfaceStorage::MetaRefresh(std::span<const std::uint8_t> bytes) { const auto current=displayRuntime_.lock();const bool selected=current && static_cast<bool>(current->metaRefresh);const auto runtimePin=selected && current->pin ? current->pin():std::shared_ptr<void>{};const auto update=selected ? current->metaRefresh:displayDevices_.metaRefresh;if (!update) throw std::logic_error("현재메타표시의실제파생갱신장치미연결");update(bytes); }
o::SquidDisplay& RawSurfaceStorage::Display() {
    if (!display_) { auto display=std::make_unique<o::SquidDisplay>(pool.Edition(),types,shapes,static_cast<o::SquidDisplaySink&>(*this),CurrentDisplayView());display->BindView([this] { return CurrentDisplayView(); });display_=std::move(display); } //첫실제표시사용에한실행기만구성하고World/Acquire를만들지않는다.
    return *display_;
}
// 원문의 실제 공통생성자를 같은 장기물리에 한 번 구성한다. 생성의 선택효과는 기존Factory가 결정한다.
// 현재삭제대여가있으면기존Client장치의실제Runtime pin을유지하고없으면같은장기VT분배를사용한다.
bool RawSurfaceStorage::DestroyAsset(o::Sid sid,std::uint32_t flags) {
    if (UsesNativeTransmissionDevices() && patchDestroyDiagnostics_ && deletionRuntime_.expired()) { const auto result=ProcessFormDispatch().Destroy(sid,flags);if (result) NoteChanged();return result; } //실제Client의PATCH삭제모드만장기본문을사용한다.독립typed장치계약은유지하고물리World잔류가아니라실제삭제대여의생존을기준으로선택한다.
    const auto destroy=resourceDevices_.destroy; //현재 장치를 호출이 끝날 때까지 보유하며 Session/World를 캡처하지 않는다.
    if (!destroy) throw std::logic_error("장기 자산의 현재 가상 삭제 장치 미연결");
    return destroy(sid,flags);
}
o::Sid RawSurfaceStorage::CreateAsset(std::uint32_t type,std::uint32_t flags) { NoteChanged();return Factory().Create(type,flags); } //실제 현재 표시 대여가 있을 때만 같은 changed bool을 알리고 Native 생성 순서는 보존한다.
NativeFortSaveVirtual& RawSurfaceStorage::FortSaveVirtual() {
    if (!fortSaveVirtual_) fortSaveVirtual_=std::make_unique<NativeFortSaveVirtual>(*this,*gameAux_);return *fortSaveVirtual_;
} //현재 장기 물리/보조 전역만 참조하며 Runtime을 확보하거나 초기화하지 않는다.
o::SquidFactory& RawSurfaceStorage::Factory() {
    if (!factory_) {
        auto factory=std::make_unique<o::SquidFactory>(pool,types); //새Pool/World를만들지않으며Create가현재Allocate/생성자/가상+28을직접실행한다.
        factory->BindDiagnostics({types,[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); },[this](auto text) { RecordAssertionDetail(text); },Checking()});
        factory->BindUnpop([this]()->o::SquidUnpop& { return Unpop(); });factory->BindFormDetach([this](auto sid) { if (const auto current=currentContainedUnpop_.lock()) current->Detach(sid);else ContainedUnpop().Detach(sid); });
        factory->BindFormPopDepth(postPopState.depth);factory->BindWeakenedMana(gameAux_->PriestQuarterHp());
        if (transmissionDevices_.broadcast) factory->BindFormAttach([this] { if (const auto current=currentContainedUnpop_.lock()) return current->ContentFormType();return ContainedUnpop().ContentFormType(); },[this](auto form,auto flags) { TransmitPop(form,flags); }); //Native Form도같은현재깊이/내용물번호와장기전송을사용한다.
        factory->BindHitPointTypes(&gameAux_->PriestType(),&gameAux_->DaisType());factory->BindFakeSurfaceType(gameAux_->FakeSurfaceType());
        factory_=std::move(factory); //모든실제현재owner결합이끝난뒤한주소만공개한다.
    }
    return *factory_;
}
RawProcessHostStorage& RawSurfaceStorage::Processes() {
    if (!processes_) {
        auto processes=std::make_unique<RawProcessHostStorage>(pool,types,authority_,std::shared_ptr<std::uint32_t>(GameAux(),&GameAux()->GeyserType()),GameAux());
        if (transmissionDevices_.broadcast) { //실제Client만기본writer를명시결합하며독립fixture의정책은유지한다.
            o::ProcessFormTransmission transmit;transmit.writeData=[this](auto& process,auto changed) { return WriteDefaultForm(process,changed,0); };transmit.writeFullData=[this](auto& process,auto force) { return WriteDefaultForm(process,false,force); };
            transmit.packet=[this](auto length)->std::span<const std::uint8_t> { const auto& buffer=gameAux_->SquidTransmission().buffer;if (length<0 || static_cast<std::size_t>(length)>buffer.size()) throw std::out_of_range("장기Form의실제작성크기범위");return std::span(buffer).first(static_cast<std::size_t>(length)); };
            transmit.transmit=[this](auto length) { BroadcastProcessForm(length); }; //양수관문뒤같은전체예약으로대상별observer를실행한다.
            processes->BindFormTransmissionDefaults(std::move(transmit),[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); });
            processes->BindFormFactoryDefaults([this]()->o::SquidFactory& { return Factory(); },[this] { return postPopState.depth; }); //실제장기공장/공통깊이만getter로연결하며메뉴에서생성/대여를발생시키지않는다.
            processes->BindLifecycleDefaults([this]()->o::RawSquidDestroy& { return BaseDestroy(); },[this](auto sid,auto event,auto count,auto payload) { return HandleDefaultRegular(sid,event,count,payload); },[this] { return ProcessFormDispatch().Hooks(); }); //현재World가없는경우만실제장기기본부모를선택한다.
        }processes_=std::move(processes);
    }return *processes_;
} // 한 번 만든 상태/Host 주소는 다음 World에서도 유지한다.
// 동시 설치를 거부한 뒤에만 이전 물리를 초기화한다. TYPE/삭제 기록과 배열 주소는 유지한다.
std::unique_ptr<RawSurfaceStorage::Use> RawSurfaceStorage::Acquire() {
    if (borrowed_) throw std::logic_error("실제 월드 저장소 중복 대여");
    auto use=std::unique_ptr<Use>(new Use(*this));
    gameAux_->ResetForGeneralWorld(); // 대여 거부 관문 뒤 기존 지역 Runtime의 fresh 입력만 호환한다.
    boardTargetState=o::BoardTargetState{}; //일반 새 World의 기존 실패점 기본값만 같은 주소에서 호환한다. prepared/반납에는 이 쓰기가 없다.
    authority_->server=true;authority_->boss=true; // 일반 설치의 기존 fresh 상태다. prepared 대여/반납에는 이 쓰기가 없다.
    if (generation_) { pool.Reset(0);hash.Reset();std::fill(spots.begin(),spots.end(),std::uint8_t{}); }
    priestOwnerState.fill(0); // 기존 일반 Session 지역9DWORD의 fresh0 호환만 유지한다. Native00492240의 지정1..8쓰기와 다른 부모다.
    graph=o::RawGraphStorage{};o::ResetRawGraphTable(graph); // 기존 일반 World의 fresh 지역 Graph 의미를 같은 주소에서 유지한다. Native 시작00462eb0/부분 cleanup의 실행으로 세지 않는다.
    std::fill(recentGeysers.entries.begin(),recentGeysers.entries.end(),0U);recentGeysers.count=0; //기존일반fresh Session의새최근목록호환만유지한다. Native prepared대여/반납에는쓰지않는다.
    sidLists.PrepareGeneralWorld(); //일반 설치만 기존 새 목록 의미를 유지한다. prepared/반납은 현재 SID/count/용량을 보존한다.
    postPopState=o::SquidPostPopState{.sharedTotalMade=&totalMade,.sharedTotalCost=&gameAux_->TotalCost(),.sharedProviders=&sidLists.providers,.sharedFactories=&sidLists.factories}; //기존일반fresh장부정책만같은주소에서보존한다. prepared/반납에는쓰지않는다.
    postPopState.sharedSuppressed=postPopDevices_.aiNotice ? &ownerMode.challenge:nullptr; //일반fresh후에도같은Native모드주소를다시결합할뿐값은쓰지않는다.
    deletionState=o::SquidDeletionState{.sharedLostX=&gameAux_->LostX(),.sharedLostY=&gameAux_->LostY()};if (deletionDevices_.unitLost) RestoreDeletionGlobals();islandPostPopState=o::IslandPostPopState{1,false}; //일반fresh지역상태호환이며실제공유loadingDWORD는변경하지않는다.
    totalMade=o::RawTotalMadeStorage{}; // 기존 일반 World의 새 장부3×256 전체0 의미만 유지한다. Native의 각255칸 부분 reset과 구별한다.
    islands=o::RawIslandStorage{};o::ResetRawIslandRecords(islands.records);chunks=o::ChunkMap{}; // 기존 일반 Session의 fresh 입력/Reset/기본 청크 의미를 같은 주소에서 유지한다. Native cleanup이나 최초 할당 복원으로 세지 않는다.
    borrowed_=true;++generation_;return use;
}
// 앞선 명시 정리가 끝난 실제 배열을 초기화 없이 대여하며 동시 사용은 같은 관문에서 거부한다.
std::unique_ptr<RawSurfaceStorage::Use> RawSurfaceStorage::AcquirePrepared() {
    if (borrowed_) throw std::logic_error("준비된 실제 월드 저장소 중복 대여");
    auto use=std::unique_ptr<Use>(new Use(*this));borrowed_=true;++generation_;return use;
}
// 대여 획득 전 생성한 반납 경계에는 별도 게임 효과가 없다.
RawSurfaceStorage::Use::Use(RawSurfaceStorage& storage):storage_(storage) {}
// 월드 모듈이 사라진 뒤에만 다음 월드가 같은 물리를 다시 사용할 수 있다.
RawSurfaceStorage::Use::~Use() { storage_.borrowed_=false; }
// 현재 대여 여부만 읽고 초기화나 할당을 추가하지 않는다.
bool RawSurfaceStorage::Borrowed() const { return borrowed_; }
// 호스트 검증용이며 게임 규칙의 프레임/세대로 사용하지 않는다.
std::uint64_t RawSurfaceStorage::Generation() const { return generation_; }
// 다른 자산의 뷰를 같은 실제 Client 저장소로 잘못 연결하지 않도록 한다.
const GameAssets* RawSurfaceStorage::Source() const { return source_; }
// 실제 TYPE70부터 파일 프레임을 같은 배열에 놓고 기존 SquidRenderer의 SHP 뷰를 사용한다.
std::shared_ptr<RawSurfaceStorage> MakeRawSurfaceStorage(const GameAssets& assets,std::shared_ptr<o::RawAuthorityState> authority,std::shared_ptr<o::RawGameAuxState> gameAux) {
    const auto& table=assets.TypeTable().Types();std::vector<o::RiftTypeFrames> frames(table.size(),o::RiftTypeFrames({})); // 누락된 내장 타입은 기존과 같은 빈 프레임 표다.
    for (const auto& type:assets.Types()) frames.at(o::kFirstAssetTypeNumber+type.block)=type.definition.FrameTable(); // 실제 archive 블록의 프레임 표를 보관한다.
    return std::make_shared<RawSurfaceStorage>(assets.Edition(),table,frames,SquidRenderer::Shapes(assets,assets.Edition()),&assets,std::move(authority),std::move(gameAux));
}
}
