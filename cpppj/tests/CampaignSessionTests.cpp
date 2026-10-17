#include "client/ClientMain.h"
#include "client/UserInput.h"
// 생산 세션과 GameWorld의 표시/선택/명령이 같은 실제 raw 월드를 사용하는지 검사한다.
#include "DudeSpawnSupport.h"
#include "client/GameWorld.h"
#include "client/AiTypeRequiredEnergy.h"
#include "client/RawCursorPlacement.h"
#include "client/RawCampaignCursor.h"
#include "client/RawCampaignCursorDisplay.h"
#include "client/CampaignCursorClientEffects.h"
#include "client/RawButtonControls.h"
#include "client/GumpVisual.h"
#include "o/RawMissionAiSettings.h"
#include "o/RawGeyserAiRegistration.h"
#include "o/RawSunAviaryProcess.h"
#include "o/RawCarrierSteal.h"
#include "o/RawCarrierPickup.h"
#include "o/RawCarrierMove.h"
#include "o/RawGemReward.h"
#include "zacket/LocalMissionTransport.h"
#include "client/RawShapeToBuffer.h"
#include "client/OwnerColorMaps.h"
#include <unordered_set>
#include <iostream>
#include <type_traits>
using namespace netstorm::client;
//BYTE변환의임시DWORD를원본공유owner로빌리는오류를거부한다.
static_assert(!std::is_constructible_v<netstorm::o::RawBattleBroadcast,netstorm::o::OriginalEdition,std::span<const std::uint8_t>,netstorm::o::BattlePacketPeers&,std::uint8_t&,const bool&>);
namespace {
// 장치/UI/송신은 명시 검사 경계이며 생산 세션의 게임 계산을 대체하지 않는다.
struct SessionServicesFixture {
    SoundList names{OriginalEdition::Patch1078}; // 실제 이름표의 opaque sound handle이다.
    std::unordered_set<SoundHandle> playing; // 장치 외부 입력의 현재 재생 상태다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 원본 부모의 명시 영어 번역 입력이다.
    std::vector<std::vector<std::uint8_t>> packets; // 실제 직렬화 송신 요청 전체바이트다.
    std::size_t notices{}; // 외부 UI/음향 요청의 관찰 개수다.
    std::size_t constructionConfigReads{},constructionRefreshes{}; // 명시 건설 요청만 선택해야 하는 실제 설정/표시 외부 경계다.
    // 같은 fixture가 세션보다 오래 살아야 하며 모든 실제 선택 경계를 명시한다.
    RawCampaignSessionServices Services() {
        names.Initialize();RawCampaignSessionServices s;s.unixSeconds=[] { return 946684800; };s.soundNames=&names;
        s.sound.wallSeconds=[] { return 100.0; };
        s.sound.playLoopAt=[this](float,float,SoundHandle current,SoundHandle sound) { const auto token=current ? current : sound;playing.insert(token);++notices;return token; };
        s.sound.playOnceAt=[this](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { ++notices;return sound; };
        s.sound.play=[this](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { playing.insert(sound);++notices;return sound; };
        s.sound.stop=[this](SoundHandle sound) { playing.erase(sound);++notices; };
        s.sound.isPlaying=[this](SoundHandle sound) { return playing.contains(sound) ? 1U : 0U; };
        s.priest.ownerNotice=[this](Sid) { ++notices; };s.priest.shield.noticeSound=[this](const PriestShieldNotice&) { ++notices; };
        s.priest.shield.translate=[](std::string_view key) { return std::string(key); };s.priest.shield.tell=[this](std::string_view) { ++notices; };
        s.ownership.refreshProduction=[this] { ++notices; };s.ownership.themeMode=[] { return 0U; };
        s.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };s.install.setConfig=[this](std::string_view,std::span<const std::uint8_t>) { ++notices; };
        s.install.refreshWindowTitle=[this] { ++notices; };s.install.resetCamera=[this] { ++notices; };
        s.assets.postPop.playImpactSound=[this](std::uint32_t) { ++notices; };
        s.assets.queueCommand=[this](std::uint32_t,std::int32_t,std::int32_t) { ++notices; };
        s.assets.sendGunPacket=s.pathPacket=[this](std::span<const std::uint8_t> bytes) { packets.emplace_back(bytes.begin(),bytes.end()); };
        s.archer.animation.common.sound=[this](Sid,std::string_view) { ++notices; };
        s.archer.missile.firingSound=s.archer.missile.impactSound=[this](float,float,std::uint32_t) { ++notices; };
        s.motion.path.stepSound=[this](Sid,float,float,std::uint32_t) { ++notices; };
        s.player.log=[this](std::uint32_t,std::string_view) { ++notices; };
        s.deletion.emit=[this](const SquidDestroyEvent& event) { if (event.effect!=SquidDestroyEffect::Transmit) throw std::logic_error("다른파생삭제미연결");++notices; };
        s.deletion.selected=[] { return Sid{}; };s.dependencyTransmit=[this](Sid,std::uint32_t) { ++notices; };
        s.construction.configInt=[this](std::string_view key,std::int32_t fallback) { CHECK(key=="predictableGeysers" && fallback==1);++constructionConfigReads;return 1; };
        s.construction.wallSeconds=[] { return 100.0; };s.construction.refresh=[this] { ++constructionRefreshes; };
        s.construction.negativeQuantity=[] { throw std::logic_error("실제 Session 검사에서 음수 생성수량 진단"); };
        s.construction.boundsAssertion=[](PlayerGeyserBoundsAssertion) { throw std::logic_error("실제 Session 검사에서 PlayerBounds 진단"); };
        return s; // 실제 부모 VT 통지와 Player AI 조회 래퍼를 세션이 공급한다.
    }
};
// 실제 원본 기본값 몸체로600바이트를 준비하고 명시 프로필만 덮어쓴다. 전략 프레임을 실행하는 입력은 아니다.
std::array<std::uint8_t,RawMissionAiSettings::kSize> SessionAiSettings(std::string_view name,std::string_view technology,std::uint32_t allies=0) {
    std::array<std::uint8_t,RawMissionAiSettings::kSize> profile;profile.fill(0xa7); // 생성자가 보존해야 할 미초기화 입력 바이트다.
    MissionAiSettingsState defaults;MissionAiSettingsHooks hooks; // Initialize에서는 설정 조회를 선택하지 않는다.
    hooks.get=[](std::string_view)->std::string { throw std::logic_error("AI 기본값 생성 중 선택하지 않은 설정 조회"); };
    RawMissionAiSettings settings(OriginalEdition::Patch1078,defaults,std::move(hooks));settings.Initialize(profile);
    std::copy(name.begin(),name.end(),profile.begin());profile[name.size()]=0;
    std::copy(technology.begin(),technology.end(),profile.begin()+30);profile[30+technology.size()]=0;
    Put(profile,0x22c,777);Put(profile,0x23c,allies);return profile;
}
// 실제 같은 지도의 빈3×3칸을 명시 배치 입력으로 선택한다. 확정 결과나 서버SID를 계산하지 않는다.
CellPoint EmptySessionSupport(const RawWorldCore& core) {
    // 가장자리 밖 원본 발자국을 피하고 실제 spot 하위6비트로만 입력 위치를 고른다.
    for (int y=4;y<248;++y) {
        // 같은 행의 왼쪽부터 자유로운 첫 정사각형을 찾는다.
        for (int x=4;x<248;++x) {
            bool empty=true;
            // 후보발자국의세행을검사하며막힌칸을찾은뒤에는다음후보로넘어간다.
            for (int dy=0;dy<3 && empty;++dy) {
                // 현재spot의실제점유비트를읽으며좌표/지형을합성하지않는다.
                for (int dx=0;dx<3;++dx) if (core.spots[(y+dy)*256+x+dx]&0x3fU) { empty=false;break; }
            }
            if (empty) return {x,y};
        }
    }
    throw std::logic_error("보호 Session의 빈3×3 받침 입력 위치 부재");
}
// 현재 목록 선택은 실제 BFS를 쓰되 기존40/Form/패킷을 유지하고 귀환 명령에서만 새 경로를 만든다.
TEST_CASE(CampaignCarrierChoice_ActualListsBfsHomeAndCurrentAi) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
    services.ai.participation.clearChat=[](std::uint32_t) {};services.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;
    const auto raw=core.pool.Slot(priest);const RawPathPoint point{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))};const auto golem=session.World().Create(133,2);session.World().Pop(golem,point.x,point.y,0);core.owner.Set(golem,1);
    auto* initial=session.StartCarrierMovement({golem,point,1,0,0,0,0});CHECK(initial);if (!initial) return;const auto form=initial->Form();const auto pid=core.kernel.Find(*initial);const auto packets=external.packets.size();
    SquidPostPopList candidates{{priest.value},1};CHECK(session.CarrierChoice().Evaluate(candidates,golem,golem)==priest);CHECK(core.processes.FromForm(form)==initial && core.kernel.Get(pid)==initial && external.packets.size()==packets);CHECK(&session.CarrierChoice().Pool()==&core.pool);
    const Sid home{static_cast<std::uint16_t>(Get(session.PlayerRecords(),0xb4+0x7c))};CHECK(home.value);const auto chosen=session.CarrierChoice().ChooseReturn(golem);CHECK(chosen==home);CHECK(core.processes.FromForm(form)==initial && external.packets.size()==packets);
    const auto returning=session.CarrierTarget().GoHome(golem);CHECK(returning);if (!returning) return;auto* path=reinterpret_cast<RawPathProcess*>(returning);CHECK(path->Parent()==golem && path->MovementType()==40 && path->Runtime().Count()>0);CHECK(external.packets.size()==packets+1);
    const auto selected=session.CarrierChoice().ChooseGeyser(golem);CHECK(selected.value && core.pool.Slot(selected)[10]==122);CHECK(core.processes.FromForm(path->Form())==path && external.packets.size()==packets+1);
    const auto beforeFailure=external.packets.size();CHECK(!session.CarrierTarget().GoGeyser(golem,selected));
    RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("후보 검사 조회에서 삭제 금지"); });CHECK(!lookup.Find(golem,40).value && external.packets.size()==beforeFailure); // 초기 지도의 끊긴 접근은 원본처럼 기존40을 지우고 새 경로를 만들지 않는다.
    const auto rectangle=core.neighbors.Footprint(selected);const auto collector=session.World().Create(133,2);session.World().Pop(collector,rectangle.right,rectangle.bottom,0);core.owner.Set(collector,1); // 간헐천 섬의 모서리를 초기 위치로 사용한다. 결과 경로는 실제 BFS가 계산한다.
    const auto mining=session.CarrierTarget().GoGeyser(collector,selected);
    if (!mining) {
        const auto current=core.pool.Slot(collector);const RawPathPoint source{std::bit_cast<float>(Get(current,14)),std::bit_cast<float>(Get(current,18))};BoardTargetState targetState;RawBoardTarget target(targetState);const auto destination=target.Nearest(core.neighbors.Footprint(selected),source,1);
        RawPathSearchState inspectState;RawPathSearch inspect(core.pool,core.hash,core.spots,core.types,core.frames,inspectState);RawPathRuntime probe(OriginalEdition::Patch1078);const auto reachable=inspect.Search(probe,collector,source,destination,0);
        std::cout<<"간헐천 접근 진단 owner="<<unsigned(current[34])<<" brain="<<session.Participation().Brain(current[34])<<" target="<<selected.value<<" state="<<unsigned(core.pool.Slot(selected)[11])<<" source="<<source.x<<','<<source.y<<" dest="<<destination.x<<','<<destination.y<<" graphs="<<unsigned(inspect.GraphAt(source).value_or(254))<<','<<unsigned(inspect.GraphAt(destination).value_or(254))<<" surface="<<inspect.SurfaceAt(source).value<<','<<inspect.SurfaceAt(destination).value<<" search="<<reachable<<" count="<<probe.Count()<<'\n';
    }
    CHECK(mining);if (!mining) return;auto* current=reinterpret_cast<RawPathProcess*>(mining);CHECK(current->Parent()==collector && current->MovementType()==40 && current->Runtime().Count()>1);const auto currentForm=current->Form();
    const auto ai=session.CreateAi(2,SessionAiSettings("Explicit","sunCannon"));CHECK(ai && session.Participation().Brain(2)==ai);const auto beforeAi=external.packets.size();core.owner.Set(collector,2);CHECK(session.CarrierTarget().GoReturn(collector,home)==0 && core.processes.FromForm(currentForm)==current && external.packets.size()==beforeAi);session.RemoveAi(2);CHECK(!session.Participation().Brain(2));core.owner.Set(collector,1);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제 보호 지도의 골렘 경로는 살아 있는 대상 확인 뒤 프레임을 진행하고 죽은 대상은 재선택한다.
TEST_CASE(CampaignSession_PathRetargetRunsRealCarrierFrames) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();const auto core=session.Core();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto source=core.pool.Slot(priest);
    const auto golem=session.World().Create(133,2);session.World().Pop(golem,std::bit_cast<float>(Get(source,14)),std::bit_cast<float>(Get(source,18)),0);core.owner.Set(golem,1);
    const auto geyser=session.CarrierChoice().ChooseGeyser(golem);CHECK(geyser.value);if (!geyser.value) return;const auto rectangle=core.neighbors.Footprint(geyser);session.World().Unpop(golem,0);session.World().Pop(golem,rectangle.right,rectangle.bottom,0);
    const auto started=session.CarrierTarget().GoGeyser(golem,geyser);CHECK(started);if (!started) return;auto* path=reinterpret_cast<RawPathProcess*>(started);
    CHECK(path->Runtime().Count()>1 && path->Runtime().TargetA()==geyser.value);const auto form=path->Form();const auto before=Get(core.pool.Slot(golem),14);const auto beforeY=Get(core.pool.Slot(golem),18);const auto packets=external.packets.size();
    core.processState.now=path->Runtime().Time(0)+(path->Runtime().Time(path->Runtime().Count()-1)-path->Runtime().Time(0))*0.9;
    const auto previous=path->Runtime().Current();path->RunFrame();CHECK(core.processes.FromForm(form)==path);if (core.processes.FromForm(form)!=path) return;
    CHECK(path->Runtime().Current()==previous+1);path->RunFrame();CHECK(core.processes.FromForm(form)==path);if (core.processes.FromForm(form)!=path) return;
    const auto& shape=core.shapes[133].frames.at(Get(core.pool.Slot(golem),36)); // 실제 보호 SHP의 한 걸음 임계값이다.
    CHECK(path->Runtime().Count()==2 && shape.cellHotspotX*shape.cellHotspotX+shape.cellHotspotY*shape.cellHotspotY>=1);
    CHECK(Get(core.pool.Slot(golem),14)==before && Get(core.pool.Slot(golem),18)==beforeY);CHECK(!path->Runtime().Finished() && path->Runtime().TargetA()==geyser.value && external.packets.size()==packets);
    std::cout<<"실제 골렘 경로 count="<<path->Runtime().Count()<<" current="<<path->Runtime().Current()<<" now="<<core.processState.now<<" end="<<path->Runtime().Time(1)<<" hotspot="<<shape.cellHotspotX<<','<<shape.cellHotspotY<<'\n';
    const auto oldState=core.pool.AllocatedBytes(geyser)[11];core.pool.AllocatedBytes(geyser)[11]|=2; // 사망 상태만 명시 입력한다. 삭제 트랜잭션을 대신한 것으로 계산하지 않는다.
    const auto replacement=session.CarrierChoice().ChooseGeyser(golem);CHECK(replacement!=geyser);path->RunFrame();core.pool.AllocatedBytes(geyser)[11]=oldState;
    RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("경로 재선택 검사 조회에서 삭제 금지"); });const auto currentForm=lookup.Find(golem,40);
    std::cout<<"사망 간헐천 "<<geyser.value<<" 재선택 "<<replacement.value<<" 현재40 "<<currentForm.value<<" 추가패킷 "<<external.packets.size()-packets<<'\n';
    if (currentForm.value) { auto* current=dynamic_cast<RawPathProcess*>(core.processes.FromForm(currentForm));CHECK(current && current->Runtime().TargetA()==replacement.value && current->Runtime().TargetA()!=geyser.value); }
    else CHECK(replacement.value!=0 && external.packets.size()==packets); // 실제 끊긴 섬의 접근 실패는 기존40만 삭제한다.
    CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 같은 실제 Host40이 목적지에 도착해서 내용물 생성→통지 송신→현재 수량 차감까지 실행한다.
TEST_CASE(CampaignSession_CarrierMiningNaturalArrivalUsesSameWorld) {
    // 실제 도착에서 잔량 유지와 마지막 너겟의 자연 고갈을 각각 새 월드로 실행한다.
    for (const auto initialQuantity:{400U,200U}) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();
    RawCampaignSession* live{};Sid golem{},geyser{};std::vector<std::vector<std::uint8_t>> mined;std::uint32_t quantityAtSend{},contentAtSend{}; // 실제 송신 경계에서 로컬 수량 차감 전 상태를 관찰한다.
    services.battlePeers[2]=[&](std::span<const std::uint8_t> packet) {
        if (packet.size()!=6 || packet[1]!=0x5b) return;CHECK(live && geyser.value && golem.value);if (!live) return;
        mined.emplace_back(packet.begin(),packet.end());quantityAtSend=Get(live->Core().pool.Slot(geyser),12,2);contentAtSend=live->Priests().Carrier().FindContained(golem,0x20,154,0).value;
    };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));live=&session;session.Load();const auto core=session.Core();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto priestRaw=core.pool.Slot(priest);
    golem=session.World().Create(133,2);session.World().Pop(golem,std::bit_cast<float>(Get(priestRaw,14)),std::bit_cast<float>(Get(priestRaw,18)),0);core.owner.Set(golem,1);geyser=session.CarrierChoice().ChooseGeyser(golem);CHECK(geyser.value);if (!geyser.value) return;
    Put(core.pool.AllocatedBytes(geyser),12,initialQuantity,2);CHECK(core.types[154].cost==200); // 한 번 또는 두 번의 채취분을 초기 입력한다. 너겟/차감 결과는 입력하지 않는다.
    const auto playerBytes=session.PlayerRecords();auto players=std::span<std::uint8_t>(const_cast<std::uint8_t*>(playerBytes.data()),playerBytes.size());players[2*0xb4+0x50]|=1; // 실제 접속 플래그와 실제 peer를 함께 준비한다.
    const auto rectangle=core.neighbors.Footprint(geyser);session.World().Unpop(golem,0);session.World().Pop(golem,rectangle.right,rectangle.bottom,0);const auto started=session.CarrierTarget().GoGeyser(golem,geyser);CHECK(started);if (!started) return;
    auto* path=reinterpret_cast<RawPathProcess*>(started);const auto form=path->Form();const auto limit=path->Runtime().Count()+2;core.processState.now=path->Runtime().Time(path->Runtime().Count()-1);const auto sp=session.Sp(1);
    // 실제 한 꼭짓점씩 진행하고 삭제된 Form을 다시 호출하지 않는다. 도착 좌표/수신을 직접 호출하지 않는다.
    for (int i=0;i<limit;++i) { auto* current=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));if (!current) break;current->RunFrame(); }
    const auto content=session.Priests().Carrier().FindContained(golem,0x20,154,0);CHECK(content.value && Get(core.pool.Slot(content),18,2)==154);
    CHECK(mined.size()==1 && mined.front()==std::vector<std::uint8_t>({1,0x5b,6,0,static_cast<std::uint8_t>(geyser.value),static_cast<std::uint8_t>(geyser.value>>8)}));CHECK(quantityAtSend==initialQuantity && contentAtSend==content.value);CHECK(session.Sp(1)==sp);
    if (initialQuantity==400) CHECK(Get(core.pool.Slot(geyser),12,2)==200);else CHECK((core.pool.Slot(geyser)[11]&3)!=0 || core.pool.Slot(geyser)[10]!=122);
    RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("채취 후 조회에서 삭제 금지"); });CHECK(!lookup.Find(golem,40).value);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    }
}
// 실제회피가같은Host40/BFS/직렬화로기존경로를바꾸고현재깊이3의추가요청은보존한다.
TEST_CASE(CampaignSession_CarrierEscapeUsesRealPathsAndDepthCap) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();const auto core=session.Core();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto raw=core.pool.Slot(priest);const RawPathPoint point{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))};
    const auto golem=session.World().Create(133,2);session.World().Pop(golem,point.x,point.y,0);core.owner.Set(golem,1);CHECK(session.StartCarrierMovement({golem,point,1,0,0,0,0}));
    RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("실제회피조회에서삭제금지"); });std::int32_t depth{};RawPathProcess* current{};
    // 현재같은초기위치에서실제새경로세개를계속요청한다. 경로/도착시각은입력하지않는다.
    for (const auto event:{0U,2U,4U}) {
        const auto packets=external.packets.size();CHECK(session.EscapeCarrier(golem,1,event)==1001);const auto form=lookup.Find(golem,40);CHECK(form.value);if (!form.value) return;current=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(current);if (!current) return;
        CHECK(current->Runtime().MoveAwayDepth()==++depth && current->Runtime().Count()>0 && current->Parent()==golem && external.packets.size()==packets+1);
    }
    const auto packets=external.packets.size();const auto form=current->Form();CHECK(session.EscapeCarrier(golem,2,0)==1000);CHECK(core.processes.FromForm(form)==current && current->Runtime().MoveAwayDepth()==3 && external.packets.size()==packets);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 장애물회피는현재지도surface후보를실제BFS/Prepare로평가한뒤새40에깊이를기록한다.
TEST_CASE(CampaignSession_CarrierEscapeSurfaceFallbackUsesRealBfs) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();const auto core=session.Core();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto raw=core.pool.Slot(priest);const RawPathPoint point{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))};const auto golem=session.World().Create(133,2);session.World().Pop(golem,point.x,point.y,0);core.owner.Set(golem,1);CHECK(session.StartCarrierMovement({golem,point,1,0,0,0,0}));
    const auto packets=external.packets.size();CHECK(session.EscapeCarrier(golem,5,0x1da1a)==1001);RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("실제회피조회에서삭제금지"); });const auto form=lookup.Find(golem,40);CHECK(form.value);if (!form.value) return;auto* current=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(current);if (!current) return;
    CHECK(current->Runtime().MoveAwayDepth()==1 && current->Runtime().Count()>1 && external.packets.size()==packets+1);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제Stage3가읽을전역과직접SP지정이동일Session의원문32×9분산배열에만영향을주어야한다.
TEST_CASE(CampaignSession_MissionInputsUseSameScrambledSpSlots) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());
    CHECK(session.LocalOwner()==1 && session.PlayerKnown()==1 && session.Editor()==0);session.Load();
    CHECK(session.LocalOwner()==1 && session.PlayerKnown()==1 && session.Editor()==0 && session.AiMemory().Allocations().empty());
    const auto& store=session.SpStore();const auto* address=store.Pools().data();const auto* const* firstAddress=&store.FirstPoolCurrent(); // 실제장기분산배열과첫주소전역의주소다.
    const auto offset=store.Offset();const auto tracking=store.TrackingOffsetCurrent();const auto permutation=store.Permutation();const auto original=store.Pools();
    const auto notices=external.notices;constexpr std::uint32_t owner=1; // 실제미션로컬SP슬롯에직접float비트를쓴다.
    session.SetSp(owner,123.25F);CHECK(std::bit_cast<std::uint32_t>(session.Sp(owner))==std::bit_cast<std::uint32_t>(123.25F));
    CHECK(&session.SpStore()==&store && store.Pools().data()==address && &store.FirstPoolCurrent()==firstAddress && store.FirstPoolCurrent()==address);
    CHECK(store.Offset()==offset && store.TrackingOffsetCurrent()==tracking && store.Permutation()==permutation && store.Pools()!=original);
    std::uint32_t decoded{}; // API구현을호출하지않고실제원문slot에서독립적으로조립할32비트다.
    // 실제순열이선택한각배열의owner1보존비트와다른모든owner의물리DWORD를확인한다.
    for (std::uint32_t bit=0;bit<32;++bit) {
        const auto pool=static_cast<std::size_t>(permutation[bit]);if (store.Pools()[pool*9+owner]&(1U<<((offset+bit)&31U))) decoded|=1U<<bit;
        // Set이선택한owner외에는난수잡음조차쓰지않아야한다.
        for (std::uint32_t other=0;other<9;++other) if (other!=owner) CHECK(store.Pools()[pool*9+other]==original[pool*9+other]);
    }
    CHECK(decoded==std::bit_cast<std::uint32_t>(123.25F));const auto written=store.Pools();static_cast<void>(session.Sp(owner));CHECK(store.Pools()==written);
    session.SetSp(owner,std::bit_cast<float>(0x80000000U));CHECK(std::bit_cast<std::uint32_t>(session.Sp(owner))==0x80000000U);
    CHECK(external.notices==notices && session.AiMemory().Allocations().empty()); // 직접Set은AddSp의UI/지갑/AI생성을실행하지않는다.
    bool rejected=false;const auto valid=store.Pools();try { session.SetSp(9,1.0F); } catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && store.Pools()==valid && store.Pools().data()==address);
}
// 첫객체전연결한참여조회가보호월드의실제사제목록/ownerState와동일한지확인한다.
TEST_CASE(CampaignSession_PriestParticipationUsesCurrentSharedState) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());
    auto& shared=session.Priests().Priests();CHECK(&session.PriestParticipation().Pool()==&session.Core().pool && &session.PriestParticipation().Priests()==&shared);
    CHECK(session.FindPriestParticipation(1)==0);shared.ownerState[0]=80;shared.ownerState[8]=88;session.Load();const auto core=session.Core();
    CHECK(RawPriestLookup(core.pool,shared).Find(8)==0 && shared.ownerState[0]==80 && shared.ownerState[8]==88); // Load에서없는소유자를자동2로고치지않는다.
    const auto live=RawPriestLookup(core.pool,shared).Find(1);CHECK(live!=0 && core.pool.Slot(Sid{static_cast<std::uint16_t>(live)})[10]==158);
    shared.ownerState.fill(77);const auto previous=shared.ownerState;CHECK(session.FindPriestParticipation(1)==live && shared.ownerState==previous);
    session.RunFrame(FrameTime{0,0,0,0},true);CHECK(shared.ownerState==previous); // 프레임도로그인타이머를대신하지않는다.
    session.RefreshPriestParticipation(1);CHECK(shared.ownerState[1]==0);session.RefreshPriestParticipation(8);CHECK(shared.ownerState[8]==2);
    const auto beforeInvalid=shared.ownerState;session.RefreshPriestParticipation(0);session.RefreshPriestParticipation(9);session.RefreshPriestParticipation(0xffffffff);
    CHECK(shared.ownerState==beforeInvalid && shared.ownerState[0]==77);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 보호188TYPE/실제Factory로만든Carrier/Content/Altar/Dais의현재raw와실제해시를읽는다.
TEST_CASE(CampaignSession_PriestParticipationFindsRealCapturedFallbacks) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();const auto core=session.Core();
    CHECK(RawPriestLookup(core.pool,session.Priests().Priests()).Find(8)==0 && session.FindPriestParticipation(8)==0);
    const auto carrier=session.World().Create(133,2),first=core.factory.CreateContained(2),second=core.factory.CreateContained(2); // 실제sunWalker/실제타입6생성자다.
    CHECK((core.types[133].flags2&0x30000U)!=0 && core.pool.Slot(first)[10]==6 && core.pool.Slot(second)[10]==6);
    auto firstRaw=core.pool.AllocatedBytes(first),secondRaw=core.pool.AllocatedBytes(second);Put(core.pool.AllocatedBytes(carrier),6,first.value,2);Put(firstRaw,4,second.value,2);Put(secondRaw,4,0,2);
    // 완성된포획체인의명시raw입력이다.실제포획트랜잭션이나부모통지를실행한것으로계산하지않는다.
    for (const auto form:{first,second}) { auto raw=core.pool.AllocatedBytes(form);Put(raw,16,carrier.value,2);Put(raw,18,158,2);Put(raw,20,0x20,2);raw[11]=8; }
    Put(firstRaw,12,1,2);Put(secondRaw,12,136,2);CHECK(session.Priests().Carrier().FindContained(carrier,0x20,158,0)==first);
    CHECK(session.FindPriestParticipation(8)==0); // 첫일치종속의원소유자가다르면같은Carrier의두번째종속을고르지않는다.
    Put(firstRaw,12,136,2);CHECK(session.FindPriestParticipation(8)==carrier.value);session.RefreshPriestParticipation(8);CHECK(session.Priests().Priests().ownerState[8]==1);
    const auto firstAltar=session.World().Create(164,2),firstDais=session.World().Create(165,2),lastAltar=session.World().Create(164,2),lastDais=session.World().Create(165,2);
    // 실제typed탐색에필요한현재좌표/버킷등록만입력한다.결과SID나포획bool을주입하지않는다.
    for (const auto pair:{std::array<Sid,2>{firstAltar,firstDais},std::array<Sid,2>{lastAltar,lastDais}}) {
        const float coordinate=pair[0]==firstAltar ? 100.75F : 110.75F;
        // 기준점과Dais는같은현재raw좌표를사용하며새등록의기존버킷꼬리를보존한다.
        for (const auto sid:pair) { auto raw=core.pool.AllocatedBytes(sid);Put(raw,14,std::bit_cast<std::uint32_t>(coordinate));Put(raw,18,std::bit_cast<std::uint32_t>(coordinate)); }
        Put(core.pool.AllocatedBytes(pair[0]),12,8,2);auto& head=core.hash.Bucket(1,coordinate,coordinate);auto daisRaw=core.pool.AllocatedBytes(pair[1]);Put(daisRaw,4,head,2);head=pair[1].value;daisRaw[33]=1;
    }
    CHECK(core.types[165].footX==7 && core.types[165].footY==7); // 보호dais.type의실제발자국은7×7이며독립합성fixture의3×3과구분한다.
    CHECK(session.FindPriestParticipation(8)==carrier.value);
    Put(firstRaw,12,1,2);CHECK(session.FindPriestParticipation(8)==lastAltar.value);session.RefreshPriestParticipation(8);CHECK(session.Priests().Priests().ownerState[8]==1);
    core.pool.AllocatedBytes(lastDais)[11]|=2;CHECK(session.FindPriestParticipation(8)==firstAltar.value);
    Put(core.pool.AllocatedBytes(firstAltar),12,264,2);CHECK(session.FindPriestParticipation(8)==0);session.RefreshPriestParticipation(8);CHECK(session.Priests().Priests().ownerState[8]==2);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제Session의첫객체전바인딩이같은서버확정/예측삭제/패킷/공간으로들어가야한다.
TEST_CASE(CampaignSession_ConstructionSupportUsesSameWorld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());
    const auto initial=session.Core().pool.FreeCount();bool rejected=false; // 설치전요청은실제공간쓰기이전에거부한다.
    try { session.ConfirmSupport(10,10); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && session.Core().pool.FreeCount()==initial && session.Construction().LastPacket().empty());
    session.Load();const auto core=session.Core();CHECK(&session.Construction().Pool()==&core.pool);
    CHECK(session.Construction().LastPacket().empty() && external.constructionConfigReads==0 && external.constructionRefreshes==0);
    const auto origin=EmptySessionSupport(core);const auto preview=session.World().Create(157,2); // 명시예측SID입력이다.
    const std::array<std::uint32_t,8> patterns{107,82,94,157,131,129,140,142}; // 실제기본Canon타입전역이며미리보기입력프레임선택에만쓴다.
    const auto decoder=DecodeCanonType(core.frames[157],0,patterns,{157,0,0,static_cast<float>(origin.x),static_cast<float>(origin.y),false},OriginalEdition::Patch1078);
    auto raw=core.pool.AllocatedBytes(preview);raw[40]|=1;Put(raw,36,std::bit_cast<std::uint32_t>(decoder.Frame()));
    CHECK(Get(raw,0)==kPatchNoIslandVtable); // 실제생성자VT로분배하며타입157만보고일반삭제를허용하지않는다.
    session.World().Pop(preview,static_cast<float>(origin.x),static_cast<float>(origin.y),2);
    CHECK(session.ConfirmSupport(static_cast<float>(origin.x),static_cast<float>(origin.y)));
    const auto bytes=session.Construction().LastPacket();CHECK(bytes.size()==45 && bytes[1]==0x3f && bytes[26]==9 && external.constructionRefreshes==1);
    CHECK((core.pool.Slot(preview)[11]&1U)!=0);
    // 동일wire에서읽은서버9SID가같은raw표면과spot에등록되어야한다.
    for (std::size_t index=0;index<bytes[26];++index) {
        const Sid sid{static_cast<std::uint16_t>(Get(bytes,27+2*index,2))};const auto placed=core.pool.Slot(sid);
        CHECK(sid.value>=core.pool.Layout().serverFirst && sid!=preview && placed[10]==157 && !(placed[11]&15U) && !(placed[40]&1U));
        const auto x=static_cast<int>(std::bit_cast<float>(Get(placed,14))),y=static_cast<int>(std::bit_cast<float>(Get(placed,18)));
        CHECK(core.hash.Entries(0)[y*256+x]==sid.value && (core.spots[y*256+x]&2U));
    }
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제Session의현재Player/실제Bounds/전용최근목록/같은보상pool/실제Geyser후처리를연결한다.
TEST_CASE(CampaignSession_LocalGeysersUseCurrentInputs) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();const auto core=session.Core();
    const auto& local=session.Construction().LocalGeysers();CHECK(&local.Pool()==&core.pool && local.Recent().count==0 && local.ResourcePool()==0);
    auto records=session.PlayerRecords();
    // 이전가짜참여표없이현재원문flags만명시입력으로설정하며실제로드한지역은유지한다.
    for (std::size_t owner=1;owner<9;++owner) records[owner*0xb4+0x50]=0;
    local.ResourcePool()=9000;session.GenerateLocalGeysers();CHECK(local.Recent().count==0 && local.ResourcePool()==9000);
    records[0xb4+0x50]=2;CHECK(Get(records,0xb4+0x78)!=127);
    session.GenerateLocalGeysers();CHECK(local.Recent().count==3 && local.ResourcePool()==3000 && external.constructionConfigReads==2);
    CHECK(session.Construction().LastPacket().size()==45 && external.constructionRefreshes!=0);
    // 최근목록의실제SID에최초수량2000과실제Regular가부착되어야한다.
    for (const auto value:local.Recent().Items()) {
        const Sid sid{static_cast<std::uint16_t>(value)};const auto raw=core.pool.Slot(sid);
        CHECK(raw[10]==122 && Get(raw,12,2)==2000 && !(raw[11]&15U));
        CHECK(core.processes.FindEvent(sid,0)!=nullptr);
    }
    records[0xb4+0x50]=0;session.GenerateLocalGeysers();CHECK(local.Recent().count==3 && local.ResourcePool()==3000);
    // 원본권한/요새guard는호출시점현재core모드를읽어생성을생략한다.
    records[0xb4+0x50]=2;core.ownerMode.fort=true;const auto free=core.pool.FreeCount();session.GenerateLocalGeysers();
    CHECK(core.pool.FreeCount()==free && local.Recent().count==3 && local.ResourcePool()==3000);core.ownerMode.fort=false;
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제CreateAi뒤간헐천등록은미복원AI하위를명시거부하며임시성공이나빈AI로대체하지않는다.
TEST_CASE(CampaignSession_LocalGeysersKeepNonnullAiBoundary) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
    services.ai.participation.clearChat=[&external](std::uint32_t) { ++external.notices; };
    services.ai.participation.addChat=[&external](std::uint32_t,std::string_view,std::uint32_t) { ++external.notices; };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();
    const auto token=session.CreateAi(2,SessionAiSettings("Explicit","sunCannon"));auto records=session.PlayerRecords();records[0xb4+0x50]=2;
    CHECK(token!=0 && session.Participation().Brain(2)==token);bool rejected=false;
    try { session.GenerateLocalGeysers(); } catch (const GeyserAiRegistrationBoundary& boundary) {
        rejected=true;CHECK(boundary.owner==2 && boundary.aiPointer==token && boundary.state==1);
        CHECK(boundary.sid>=session.Core().pool.Layout().serverFirst && session.Core().pool.Slot(Sid{static_cast<std::uint16_t>(boundary.sid)})[10]==122);
    }
    CHECK(rejected && session.Participation().Brain(2)==token && session.AiMemory().Contains(token));
    CHECK(session.Construction().LocalGeysers().Recent().count==0); // 원본순서에서실패한Pop뒤최근목록추가는실행하지않는다.
    session.RemoveAi(2);CHECK(session.Participation().Brain(2)==0);
}
// 같은 Session의 실제 본체/기술/동맹/현재 그래프 getter와 정상 목록 소멸을 함께 검사한다.
TEST_CASE(CampaignSession_AiUsesCurrentWorldAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();
    std::uint32_t chatClears=0,chatAdds=0,alliancePackets=0,nameReads=0; // 선택된 실제 외부 경계의 순서/인자를 관찰한다.
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xb6}); };
    services.ai.defaultName=[&] { ++nameReads;return std::string("Current AI"); };
    services.ai.participation.clearChat=[&](std::uint32_t channel) { CHECK(channel==0x40 || channel==0x80);++chatClears; };
    services.ai.participation.addChat=[&](std::uint32_t,std::string_view,std::uint32_t channel) { CHECK(channel==0x40 || channel==0x80);++chatAdds; };
    services.ai.participation.sendAlliance=[&](std::uint32_t server,std::uint32_t owner,std::uint32_t mask) { CHECK(server==9 && owner==1 && mask==6);++alliancePackets; };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto records=session.PlayerRecords();
    CHECK(session.Participation().Brain(2)==0 && session.AiMemory().Allocations().empty());
    const auto core=session.Core();CHECK(core.types[74].flags2==assets.game.TypeTable().Types()[74].flags2);
    core.bookkeeping.invalidGraph=252;const auto profile=SessionAiSettings("","sunWalker;sunCannon",2);const auto token=session.CreateAi(2,profile);
    CHECK(token!=0 && session.Participation().Brain(2)==token && session.AiMemory().Contains(token));
    const auto raw=session.AiMemory().Bytes(token);CHECK(raw.size()==RawAiBrainConstructor::kBytes && Get(raw,0x260)==2 && Get(raw,0x2d8)==777);
    CHECK(Get(raw,0x298)==252 && Get(raw,0x2ac)==252 && nameReads==1);
    CHECK(std::string(reinterpret_cast<const char*>(records.data()+2*0xb4+12))=="Current AI");
    CHECK(session.FindTechnology(2,assets.game.TypeTable().Find("sunCannon")).value!=0);
    CHECK((Get(records,2*0xb4+0x50)&0x20)!=0 && Get(records,2*0xb4+0xb0)==token && Get(records,2*0xb4+0x2c)==10002);
    CHECK(session.Alliances()[2*9+1]==1 && session.Alliances()[1*9+2]==1 && session.Alliances()[2*9+3]==0 && session.Alliances()[3*9+2]==0);
    CHECK(chatClears==2 && chatAdds>=1 && alliancePackets==1 && session.AiMemory().Allocations().size()==18);
    const auto unchanged=session.AiMemory().Bytes(token);const std::vector<std::uint8_t> before(unchanged.begin(),unchanged.end());
    const auto changed=SessionAiSettings("Ignored","windArcher");CHECK(session.CreateAi(2,changed)==token);
    CHECK(std::equal(before.begin(),before.end(),session.AiMemory().Bytes(token).begin()) && nameReads==1 && session.AiMemory().Allocations().size()==18);
    session.RemoveAi(2);CHECK(session.Participation().Brain(2)==0 && !session.AiMemory().Contains(token));
    CHECK(session.AiMemory().Frees().size()==18 && session.AiMemory().Frees().back()==token && (Get(records,2*0xb4+0x50)&0x20)!=0);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 언어/Chat/할당 누락을 생존 표식 성공으로 바꾸지 않고 선택 위치의 부분 효과와 실제 해제를 보존한다.
TEST_CASE(CampaignSession_AiMissingServicesKeepRealFailure) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession noAllocation(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());auto profile=SessionAiSettings("Explicit","sunCannon");
    bool rejected=false;try { noAllocation.CreateAi(2,profile); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && noAllocation.AiMemory().Allocations().empty());
    noAllocation.Load();rejected=false;try { noAllocation.CreateAi(2,profile); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && noAllocation.Participation().Brain(2)==0 && noAllocation.AiMemory().Allocations().empty());
    auto second=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x91}); };
    RawCampaignSession noChat(assets.game,assets.files,std::move(second),"thewarbegins",std::move(services));noChat.Load();rejected=false;
    try { noChat.CreateAi(2,profile); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && noChat.Participation().Brain(2)==0 && noChat.AiMemory().Allocations().size()==18 && noChat.AiMemory().Frees().size()==18);
    auto third=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());services=external.Services();
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x83}); };
    services.ai.participation.clearChat=[](std::uint32_t) {};services.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
    RawCampaignSession noLanguage(assets.game,assets.files,std::move(third),"thewarbegins",std::move(services));noLanguage.Load();profile=SessionAiSettings("","sunCannon");rejected=false;
    try { noLanguage.CreateAi(2,profile); } catch (const std::logic_error&) { rejected=true; }
    const auto attached=noLanguage.Participation().Brain(2);CHECK(rejected && attached!=0 && noLanguage.AiMemory().Contains(attached));
    noLanguage.RemoveAi(2);CHECK(noLanguage.Participation().Brain(2)==0 && !noLanguage.AiMemory().Contains(attached) && noLanguage.AiMemory().Frees().size()==18);
}
// Color는 기존 brain보다 먼저 같은 실제 색 표/섬 프레임을 바꾼 뒤 선택형 화면 소비자에 통지한다.
TEST_CASE(CampaignSession_AiColorPrecedesExistingBrain) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();std::uint32_t colors=0;
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x62}); };
    services.ai.participation.clearChat=[](std::uint32_t) {};services.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
    RawCampaignSession* current{};services.ai.changeColor=[&](std::uint32_t owner,std::uint32_t color) { CHECK(owner==2 && color==3 && current->Participation().Brain(owner)!=0 && current->Core().bridgeState.ownerColors[2]==3);++colors; };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));current=&session;session.Load();
    auto profile=SessionAiSettings("Explicit","sunCannon");const auto token=session.CreateAi(2,profile);Put(profile,0x250,3);
    CHECK(session.CreateAi(2,profile)==token && colors==1 && session.AiMemory().Allocations().size()==18);
    // 명시 RemoveAi 없이도 실제 세션 destructor가 살아 있는 현재 brain을 먼저 소멸/반납한다.
}
// 값형 기본값과 다른 현재 getter 및 Chat callback 이후 변경이 다음 원본 읽기에 반영되는지 확인한다.
TEST_CASE(CampaignSession_ParticipationUsesCurrentReads) {
    std::array<std::uint8_t,9*0xb4> players{};std::array<std::uint32_t,81> allies{};std::array<std::uint8_t,9> hidden{}; // 한 실제 기록/방향별표/숨김표다.
    PlayerParticipationState stale;stale.localOwner=8;stale.playerKnown=0; // 실제 getter가 이 값형 사본을 읽으면 검사가 실패한다.
    std::uint32_t owner=1,known=1,assigns=0,sends=0;bool battle=true,metamap=false,localTransport=true;std::vector<std::string> names; // 실제 외부 전역과 관찰 결과다.
    // 각 현재 Player에 이름/subscriber와 자기 동맹을 준비한다.
    for (std::uint32_t index=0;index<9;++index) {
        const auto name="P"+std::to_string(index);std::copy(name.begin(),name.end(),players.begin()+index*0xb4+12);Put(players,index*0xb4+0x2c,index+100);allies[index*9+index]=1;
    }
    allies[2*9+3]=1;PlayerParticipationHooks hooks;
    hooks.assignConnection=[&](std::int32_t,std::int32_t) { ++assigns; };
    hooks.clearChat=[&](std::uint32_t channel) { if (channel==0x40) owner=2; };
    hooks.addChat=[&](std::uint32_t,std::string_view name,std::uint32_t channel) { CHECK(channel==0x40);names.emplace_back(name);if (name=="P2") owner=3; };
    hooks.sendAlliance=[&](std::uint32_t server,std::uint32_t player,std::uint32_t mask) { CHECK(server==9 && player==1 && mask==4);++sends; };
    RawPlayerParticipation current(OriginalEdition::Patch1078,players,allies,hidden,stale,std::move(hooks));
    current.BindCurrentInputs({[&] { return owner; },[&] { return known; },[&] { return battle; },[&] { return metamap; },[&] { return localTransport; }});
    CHECK(current.CheckLogin(1,1) && assigns==0);current.SetAlliance(3,1U<<2);
    CHECK(names==std::vector<std::string>({"P2","P3"}) && owner==3);
    current.AddAlly(1,2);CHECK(sends==1 && allies[1*9+2]==1 && allies[2*9+1]==0);
    bool rejected=false;try { current.BindCurrentInputs({}); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    known=0;names.clear();current.SetAlliance(3,0);CHECK(names.empty());
}
// 실제 Session의 원문/장기 latch와 선택형 UI 경계를 같은 수명에서 검사한다. 원본 초기화 입력을 대신하지 않는다.
TEST_CASE(CampaignSession_MissionOutcomeSharesCurrentWorld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();
    RawCampaignSession* active{};std::uint32_t demo=0,blocked=0,close=0,notified=0;std::vector<std::string> sections;
    services.missionOutcome.demo=[&] { return demo; };services.missionOutcome.tellBlocked=[&] { return blocked; };
    // 첫 실제 UI callback이 같은 Player9/사제 상태를 바꾸면 다음 원본 조건은 즉시 현재 값을 읽어야 한다.
    services.missionOutcome.section=[&](std::string_view key) {
        sections.emplace_back(key);
        if (key!="ai1TempleHalfDead") return false;
        auto records=active->PlayerRecords();Put(records,0xb4+0x7c,0);records[2*0xb4+0x50]=0;
        active->Priests().Priests().ownerState[1]=1;demo=1;return true;
    };
    services.missionOutcome.closeTransient=[&] { ++close; };services.missionOutcome.onSection=[&] { ++notified; };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));active=&session;session.Load();
    CHECK(sections.empty() && close==0 && notified==0);auto records=session.PlayerRecords();auto alliances=session.Alliances();
    CHECK(records.size()==9*0xb4 && alliances.size()==81);
    // 이 검사는 명시 입력으로 현재 참여 조건을 설정한다. 실제 로그인/AI 초기화 구현의 증거로 합산하지 않는다.
    records[0xb4+0x50]=2;records[2*0xb4+0x50]=0x20;alliances[10]=7;alliances[20]=9;
    const Sid temple{static_cast<std::uint16_t>(Get(records,0xb4+0x7c))};CHECK(temple.value!=0);
    Put(session.Core().pool.AllocatedBytes(temple),26,0);session.CheckMissionOutcome();
    CHECK(sections.front()=="ai1TempleHalfDead");CHECK(close==1 && notified==1);
    CHECK(std::find(sections.begin(),sections.end(),"Demoai1TempleDead")!=sections.end());
    CHECK(std::find(sections.begin(),sections.end(),"Demoai1PriestCaptured")!=sections.end());
    CHECK(std::find(sections.begin(),sections.end(),"DemoBadTeamDead")!=sections.end());
    CHECK(std::none_of(sections.begin(),sections.end(),[](const auto& key) { return key.find("ai2")!=std::string::npos; }));
    const auto count=sections.size();session.CheckMissionOutcome();CHECK(sections.size()==count);
}
// 생산 세션의 실제 기술 생성/삭제가 같은 Player 체인과 Form 수명을 사용해야 한다.
TEST_CASE(CampaignSession_TechnologyUsesCurrentLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services();std::uint32_t uiLookups=0;
    services.playerContent.gump=[&](std::uint32_t) { ++uiLookups;return 0U; };
    services.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("없는 UI 대상 처리"); };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();
    const auto type=assets.game.TypeTable().Find("sunCannon");CHECK(type>=70);
    session.AddTechnology(1,"sunCannon");const auto first=session.FindTechnology(1,type);CHECK(first.value!=0);
    session.AddTechnology(1,"sunCannon;sunCannon");CHECK(session.FindTechnology(1,type)==first);
    const auto core=session.Core();CHECK(core.pool.Slot(first)[10]==6);
    session.ClearTechnology(1);CHECK(session.FindTechnology(1,type).value==0);
    CHECK((core.pool.Slot(first)[11]&1U)!=0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    CHECK(core.bookkeeping.depth==0 && uiLookups==0);
}
// 실제1-1의신전/Player/금지표/기술내용물이한생산저장소를사용하는지확인한다.
TEST_CASE(CampaignSession_ProductionUsesCurrentPlayerFactoriesAndRestrictions) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services();std::size_t allocations{},configReads{}; // 실제선택된할당과설정조회만관찰한다.
    services.production.allocationBytes=[&](std::span<std::uint8_t> bytes) { CHECK(bytes.size()==kProductionDeckBytes);++allocations;std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    services.production.effects.configInt=[&](std::string_view key) { CHECK(key=="getTeleportTechFree");++configReads;return 0; };
    services.production.effects.modeAssertion=[] { throw std::logic_error("설치된실제미션의Fort/Battle모드진단"); };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();
    const auto core=session.Core();const auto& production=session.Production();CHECK(&production.Pool()==&core.pool && allocations==0 && configReads==0);
    CHECK(core.bookkeeping.factories.entries.size()==500 && core.bookkeeping.factories.count==2);const auto factoryCount=core.bookkeeping.factories.count;
    const auto token=production.RefreshCurrent();CHECK(token!=0 && allocations==1 && configReads==1);
    CHECK(Get(session.PlayerRecords(),session.LocalOwner()*0xb4+0x68)==token && production.Deck().Player().Records().data()==session.PlayerRecords().data());
    CHECK(production.Contains(133) && production.Allowed(133));CHECK(production.RefreshCurrent()==token && allocations==1 && configReads==2);
    session.TotalMade().Allow(133,0);CHECK(!production.Allowed(133));CHECK(production.RefreshCurrent()==token && !production.Contains(133));
    session.TotalMade().Allow(133,1);production.RefreshCurrent();CHECK(production.Contains(133));
    core.bookkeeping.localSecondaryCounts[133]=17;core.bookkeeping.globalCounts[133]=23;
    CHECK(session.TotalMade().Built(133)==17 && session.TotalMade().Global(133)==23);
    const auto technology=assets.game.TypeTable().Find("sunCannon");production.IndicateTechnology(static_cast<std::uint32_t>(technology));
    const auto first=session.FindTechnology(static_cast<std::int32_t>(session.LocalOwner()),technology);CHECK(first.value!=0);
    production.IndicateTechnology(static_cast<std::uint32_t>(technology));CHECK(session.FindTechnology(static_cast<std::int32_t>(session.LocalOwner()),technology)==first);
    session.ClearTechnology(static_cast<std::int32_t>(session.LocalOwner()));CHECK(session.FindTechnology(static_cast<std::int32_t>(session.LocalOwner()),technology).value==0);
    CHECK(core.bookkeeping.factories.count==factoryCount && core.bookkeeping.depth==0);
}
// 실제 할당 입력이 없으면 원본 Player 포인터를 붙이거나 0채움 성공으로 진행하지 않아야 한다.
TEST_CASE(CampaignSession_ProductionMissingAllocationLeavesPlayerUnchanged) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());
    const std::vector<std::uint8_t> before(session.PlayerRecords().begin(),session.PlayerRecords().end());
    CHECK(Throws([&] { session.Production().RefreshCurrent(); }));CHECK(std::equal(before.begin(),before.end(),session.PlayerRecords().begin()));
}
// 같은 생산 월드의 empty84는 관찰 카운터 대신 실제 공통 삭제/반납으로 들어가야 한다.
TEST_CASE(CampaignSession_EmptyGeyserUsesCurrentDeletion) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();
    const auto core=session.Core();const auto free=core.pool.FreeCount();
    const auto empty=session.World().Create(84);session.World().Pop(empty,10,10,0);
    CHECK(session.World().Destroy(empty,0x400000));
    CHECK(core.pool.FreeCount()==free && (core.pool.Slot(empty)[11]&1U)!=0);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제1-1/1-2를 production GameWorld에 설치하고 같은 SID/프레임 표시를 확인한다.
void CheckSessionWorld(std::string_view mission) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/"+std::string(mission)+".fort"),assets.game.TypeTable());
    auto services=external.Services();services.install.resetCamera={}; // 실제 GameWorld 카메라가 로더의 소비자를 공급해야 한다.
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},std::string(mission),std::move(services),{1024,768});world.Resize(1024,768);
    const auto core=world.Surfaces().Core();Sid priest{};
    // 실제 live 슬롯에서 내 사제를 선택한다. 별도 순번 객체를 만들지 않는다.
    for (const auto& object:world.Surfaces().Objects()) if (object.object.type==158 && object.object.owner==1) { priest=object.sid;break; }
    CHECK(priest.value>=15000 && world.Object(priest.value)!=nullptr);
    CHECK(world.Object(priest.value)->x==std::bit_cast<float>(Get(core.pool.Slot(priest),14)));
    const auto& type=core.types[core.pool.Slot(priest)[10]];const auto view=world.Object(priest.value); // 실제 초기 카메라는 내 사제의 전체 발자국 차감 지점을 중심으로 둔다.
    CHECK(world.ToScreen(view->x-type.footX,view->y-type.footY).x==512 && world.ToScreen(view->x-type.footX,view->y-type.footY).y==384);
    CHECK(!world.Sprites().empty());
    // 실제 캠페인의 레인 신전은 현재 물결만 제출하지 않고 기본0과 같은 항목으로 그린다.
    if (mission=="thewarbegins") {
        const auto& temple=assets.game.Find("rainVortex");bool found=false;
        // 화면에 제출된 실제 신전의 원본 Draw 순서와 전체 몸체 클릭 범위를 확인한다.
        for (const auto& sprite:world.Sprites()) if (sprite.block==temple.block) {
            CHECK(sprite.frame==0 && sprite.followingFrames==std::vector<std::size_t>{1});found=true;
            const auto point=world.ToScreen(89,116);CHECK(world.Pick({point.x-112,point.y-80})==15002);
        }
        CHECK(found);
    }
    // paused 프레임도 실제 절대 시각은 공유하고 Kernel은 진행하지 않는다.
    const auto kernel=core.kernel.Size();world.frameTime=[] { return FrameTime{0,1,0,1}; };world.paused=[] { return true; };world.RunFrame();
    CHECK(core.kernel.Size()==kernel && world.Object(priest.value)!=nullptr);
    world.frameTime={};world.paused={};world.Step(1.0/60.0);
    CHECK(core.bookkeeping.depth==0 && !world.Report().empty());
}
}
// 보호 첫 캠페인도 실제 세션이 같은 raw 지도/객체를 표시한다.
TEST_CASE(CampaignSession_ActualWarBeginsGameWorld) { CheckSessionWorld("thewarbegins"); }
// 보호 둘째 캠페인도 같은 production 소유자를 사용한다.
TEST_CASE(CampaignSession_ActualMasterGameWorld) { CheckSessionWorld("masterofwhirligigs"); }

// 같은 production GameWorld의 선택/좌클릭은 원본 BFS/Host54를 만들고 현재 슬롯을 이동한다.
// 실제 로그인 소비자를 선택하며 성공 플래그/Ready를 입력하지 않는다.
void UseWorldTestLogin(RawCampaignSessionServices& services) {
    services.ai.participation.clearChat=[](std::uint32_t channel) { CHECK(channel==0x40 || channel==0x80); };services.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t channel) { CHECK(channel==0x20 || channel==0x40 || channel==0x80); }; //로그인 Player/Alliance가 호출하는 실제 Chat 장치 경계를 관측한다.
    CampaignLoginServices login;login.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };login.wallSeconds=[] { return 100.25; };login.clock=[] { return 100250U; };
    login.refresh=[] {};login.playerUiBusy=[] { return false; };login.playerNotice=[](std::uint32_t kind,std::uint32_t owner,std::uint32_t) { CHECK((kind==3 || kind==5) && owner<9); };login.log=[](std::string_view) {};services.localLogin=std::move(login);
}
// 장치 효과는 관측하고 실제 게임/선택/강조/전송은 같은 제품 몸체가 수행한다.
std::shared_ptr<std::vector<std::uint32_t>> BindWorldTestInput(GameWorld& world) {
    const auto cursors=std::make_shared<std::vector<std::uint32_t>>();UserSelectionHooks selection;selection.paused=[&world] { return world.paused && world.paused() ? 1U : 0U; };
    selection.dump=[&world](Sid sid) { CHECK(world.Session().Core().pool.Slot(sid)[10]>=70); };selection.assertion=selection.previewAssertion=[] { throw std::logic_error("실제 월드 선택 검사 예상 밖 진단"); };
    UserHighlightHooks highlight;highlight.selectCursor=[cursors](std::uint32_t kind) { cursors->push_back(kind); };CarrierInputHooks carrier;carrier.selectCursor=highlight.selectCursor;carrier.assertion=[](std::uint32_t line) { throw std::logic_error("실제 월드 운반체 입력 진단 "+std::to_string(line)); };
    world.BindUserInput(std::move(selection),std::move(highlight),std::move(carrier));return cursors;
}
void CheckSessionMovement(bool actualBroadcast) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services();
    if (actualBroadcast) { services.assets.sendGunPacket={};services.pathPacket={}; }
    UseWorldTestLogin(services);
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);const auto cursors=BindWorldTestInput(world);CHECK(world.Session().LocalConnectionPhase()==14);
    auto core=world.Surfaces().Core();Sid priest{};
    // 현재 raw 내 사제만 선택한다.
    for (const auto& object:world.Surfaces().Objects()) if (object.object.type==158 && object.object.owner==1) { priest=object.sid;break; }
    CHECK(priest.value!=0);const auto initial=std::array<std::uint32_t,2>{Get(core.pool.Slot(priest),14),Get(core.pool.Slot(priest),18)};
    const int x=static_cast<int>(std::bit_cast<float>(initial[0])),y=static_cast<int>(std::bit_cast<float>(initial[1]));
    SquidPostPopList additional;ContainedFinderState finder;PlayerPlacementAnchorState anchorState;anchorState.graphReady=1;
    RawPlayerPlacementAnchor anchor(core.pool,core.types,core.hash.Entries(0),core.bookkeeping,additional,finder,anchorState);
    const auto graph=anchor.GraphFor(priest.value);CellPoint goal{};bool found=false;
    // 같은 실제 Graph의 자유 칸을 명시 클릭 입력으로 선택하며 경로 결과를 합성하지 않는다.
    for (int radius=4;radius<10 && !found;++radius) for (int dy=-radius;dy<=radius && !found;++dy) for (int dx=-radius;dx<=radius;++dx) {
        const int px=x+dx,py=y+dy;if (px<2 || py<2 || px>=254 || py>=254) continue;
        if ((core.spots[py*256+px]&7)==2 && anchor.GraphAt(static_cast<float>(px),static_cast<float>(py))==graph) { goal={px,py};found=true;break; }
    }
    CHECK(found);world.SetMousePoint(world.ToScreen(x,y));world.Select(priest.value);CHECK(world.InputMode()==5 && !cursors->empty());CHECK(world.MoveSelected(*world.Point("cell:"+std::to_string(goal.x)+','+std::to_string(goal.y))));
    bool path=false,moved=false;
    // 실제 현재 Kernel을 실행하고 화면 뷰도 같은 슬롯의 현재 위치를 따라가는지 확인한다.
    for (std::uint32_t frame=0;frame<480;++frame) {
        world.Step(1.0/60.0);
        // 현재 실제 내 사제의 부모 Form 체인에서 살아 있는54를 조회한다.
        for (auto form=static_cast<std::uint16_t>(Get(core.pool.Slot(priest),6,2));form;form=static_cast<std::uint16_t>(Get(core.pool.Slot(Sid{form}),4,2)))
            if (core.pool.Slot(Sid{form})[10]==54) path=true;
        const auto raw=core.pool.Slot(priest);if (Get(raw,14)!=initial[0] || Get(raw,18)!=initial[1]) moved=true;
        CHECK(world.Object(priest.value)->x==std::bit_cast<float>(Get(raw,14)) && world.Object(priest.value)->y==std::bit_cast<float>(Get(raw,18)));
    }
    CHECK(path && moved && core.bookkeeping.depth==0);
}

// 실제 GameWorld 선택/이동은 원본 경로를 사용한다.
TEST_CASE(CampaignSession_ActualWarBeginsClickMovesSamePriest) { CheckSessionMovement(false); }
// 단독 캠페인의 NULL 연결 표는 실제 Gun/Path 직렬화 뒤 원본 송신 분배에서 자연 반환한다.
TEST_CASE(CampaignSession_ActualSingleplayerBroadcastAndPath) { CheckSessionMovement(true); }

// 실제 생산 세션의 보호 TYPE 원문과 현재 재시드 상태에서 교전/HP1/감시 종료를 검사한다.
TEST_CASE(CampaignSession_ActualWarBeginsCombatToOne) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services(); // UI/장치는 명시 외부 입력이며 Form/삭제는 실제 writer를 사용한다.
    services.dependencyTransmit={};services.deletion={};
    services.formPacket=[&external](std::span<const std::uint8_t> bytes) { external.packets.emplace_back(bytes.begin(),bytes.end()); };
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});
    auto core=world.Surfaces().Core();Sid priest{};
    // 실제 설치된 내 사제를 선택하며 피해용 가짜 객체를 만들지 않는다.
    for (const auto& object:world.Surfaces().Objects()) if (object.object.type==158 && object.object.owner==1) { priest=object.sid;break; }
    CHECK(priest.value!=0);const auto before=world.Object(priest.value)->hitPoints;
    world.Surfaces().Unpop(priest,0);world.Surfaces().Pop(priest,130,114,0);
    bool impact=false;std::uint32_t frame=0,firstDamage=0;
    // 명시 재배치 입력 뒤 같은 Kernel만 실행하여 실제 피해/충돌/화면 HP를 관찰한다.
    for (;frame<3600 && world.Object(priest.value)->hitPoints>1;++frame) {
        world.Step(1.0/60.0);
        if (!firstDamage && world.Object(priest.value)->hitPoints<before) firstDamage=frame+1;
        // 현재 살아 있는 충돌 자산을 직접 관찰한다.
        for (const auto& object:world.Surfaces().Objects()) if (object.object.type==160) impact=true;
    }
    CHECK(firstDamage>0 && impact && world.Object(priest.value)->hitPoints==1);
    const auto low=frame;
    // HP1 이후에도 보호막/감시/실제 소리 프로세스와 투사체 종료를 계속 실행한다.
    for (;frame<low+720;++frame) world.Step(1.0/60.0);
    CHECK(world.Object(priest.value)->hitPoints==1 && !external.playing.empty());
    // 현재 모든 총은 HP1 사제 감시를 해제해야 한다.
    for (std::uint32_t id=5;id<core.pool.Capacity();++id) {
        const Sid sid{static_cast<std::uint16_t>(id)};const auto raw=core.pool.Slot(sid);if ((raw[11]&7) || raw[10]!=34) continue;
        auto* gun=dynamic_cast<GunProcess*>(core.processes.FromForm(sid));CHECK(gun!=nullptr);
        CHECK(gun->Fields().Target()!=priest && !core.dependencies.Has(priest,sid));
    }
    // 투사체와 충돌 효과는 실제 공통 삭제를 완료한다.
    for (const auto& object:world.Surfaces().Objects()) CHECK(object.object.type!=83 && object.object.type!=160);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    bool dependencyPacket=false,deletionPacket=false; // 같은 세션의 실제 감시/삭제 writer 송신 결과다.
    // 실제 교전 동안 생성/해제한 Form과 투사체의 패킷을 관찰한다. 별도 합성 writer로 대신하지 않는다.
    for (const auto& packet:external.packets) {
        if (packet.size()==13 && packet[1]==0x65) { dependencyPacket=true;CHECK(packet[0]==1 && packet[2]==13 && packet[3]==0 && (packet[8]&4)==0); }
        if (packet.size()==8 && packet[1]==0x2e) { deletionPacket=true;CHECK(packet[0]==1 && packet[2]==8 && packet[3]==0); }
    }
    CHECK(dependencyPacket && deletionPacket);
    std::printf("생산 세션 실제 교전: 최초피해 frame%u, HP1 frame%u, 종료 frame%u\n",firstDamage,low,frame);
}

// 실제 미션의 비행 자산을 파괴해 원본 캡처/픽셀/프레임과 감시 쌍의 dummy 삭제를 확인한다.
TEST_CASE(CampaignSession_ManowarExplosionPixelsAndDummyLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/DemoStormRages.fort"),assets.game.TypeTable());auto services=external.Services();
    services.deletion={};services.deletion.selected=[] { return Sid{}; }; // 실제 UI 선택이 없는 명시 입력이다. 삭제 계산/전송은 Session이 공급한다.
    services.dependencyTransmit={};services.formPacket=[&external](std::span<const std::uint8_t> bytes) { external.packets.emplace_back(bytes.begin(),bytes.end()); };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"DemoStormRages",std::move(services));session.Load();auto core=session.Core();Sid source;std::uint64_t frame=0;double time=0;
    // 미복원 경계232 이전의 실제 전체 Kernel에서 자연 생성된 현재 맨 오 워를 찾는다.
    for (;frame<230 && !source.value;++frame) {
        time=static_cast<double>(frame)*.025;session.RunFrame({time,time,frame ? .025 : 0,frame+1},false);
        // 별도 합성 객체 대신 현재 살아 있는 실제 미션 자산만 선택한다.
        for (const auto& object:session.World().Objects()) if (object.object.type==144 && !(core.pool.Slot(object.sid)[40]&8)) { source=object.sid;break; }
    }
    CHECK(source.value!=0);if (!source.value) return;const auto before=core.pool.Slot(source);const auto x=std::bit_cast<float>(Get(before,14)),y=std::bit_cast<float>(Get(before,18));const auto depth=std::bit_cast<std::int8_t>(before[35]);
    std::array<std::uint32_t,188> tokens{};tokens[144]=1;const auto maps=BuildStandardOwnerColorMaps(144,82);ShapeToBufferState state;state.addTypeHotspot=true;state.ownerTypeMaps=tokens;
    state.ownerColors=[&](std::uint32_t type,std::uint32_t owner)->std::span<const std::uint8_t> { CHECK(type==144);return maps.at(static_cast<std::size_t>(core.bridgeState.ownerColors.at(owner))); };
    RawShapeToBuffer capture(core.pool,core.types,core.shapes,assets.game.Shapes(),state);const auto expected=capture.Capture(source);
    Renderer renderer(1024,768);const auto cameraX=static_cast<int>(x*16)-128,cameraY=static_cast<int>(y*11)-128;
    session.SetParticleView({cameraX,cameraY,0,0,1024,768},{[&] { return renderer.FullRedrawPending(); },[&](SquidDisplayRect rect,std::uint32_t flags) { renderer.Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }});
    session.World().Destroy(source,0x100000);CHECK(core.pool.Slot(source)[11]&1);ShrapnelVisualProcess* effect=nullptr;
    // 같은 registry/Host의 실제 process16을 조회하며 별도 효과 목록을 만들지 않는다.
    for (const auto token:session.Particles().Rendering().Slots()) {
        const auto* fields=session.Particles().Rendering().Lookup(token);if (!fields) continue;auto* current=dynamic_cast<ShrapnelVisualProcess*>(core.processes.FromForm(Sid{static_cast<std::uint16_t>(fields->form)}));if (current) { CHECK(effect==nullptr);effect=current; }
    }
    CHECK(effect!=nullptr);if (!effect) return;const auto form=effect->Form(),dummy=effect->Parent();const auto token=effect->Fields().registration.token;
    CHECK(core.pool.Slot(dummy)[10]==110 && core.dependencies.Has(form,dummy));CHECK(effect->Fields().registration.zOrder==depth);
    CHECK(effect->Fields().image.pixels==expected.pixels && effect->Fields().image.width==expected.width && effect->Fields().image.hotspotX==expected.hotspotX && !effect->Fields().particles.empty());
    time+=.025;session.RunFrame({time,time,.025,frame+1},false);CHECK(effect->Fields().lastTime==time);
    auto sprites=session.ParticleSprites({0,0,1024,768});std::erase_if(sprites,[&](const auto& sprite) { return sprite.order.depth!=depth; });CHECK(!sprites.empty());renderer.SetScene(sprites);const auto image=renderer.SceneImage();CHECK(std::any_of(image.indices.begin(),image.indices.end(),[](auto color) { return color!=0; }));
    std::vector<std::uint8_t> padded(1028*768,0x7e);renderer.Draw(padded,1028);renderer.Present([](ScreenRect) {});
    // 실제 padded 표면의 본문 픽셀과 각 행의 화면 밖4바이트를 함께 확인한다.
    for (std::size_t row=0;row<768;++row) {
        CHECK(std::equal(image.indices.begin()+row*1024,image.indices.begin()+(row+1)*1024,padded.begin()+row*1028));
        // Renderer와 직접 픽셀 효과 모두 행 padding을 덮지 않아야 한다.
        for (std::size_t column=1024;column<1028;++column) CHECK(padded[row*1028+column]==0x7e);
    }
    const auto count=core.kernel.Size();core.processes.Kill(*effect,0);CHECK(core.kernel.Size()+1==count && !session.Particles().Rendering().Lookup(token));CHECK((core.pool.Slot(form)[11]&1) && (core.pool.Slot(dummy)[11]&1));
    renderer.SetScene(std::move(sprites));const auto removed=renderer.SceneImage();CHECK(std::all_of(removed.indices.begin(),removed.indices.end(),[](auto color) { return color==0; }));
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}

namespace {
// 실제 Session 메뉴의 GUI/번역/Tell/커서 소비자만 관찰한다. 게임 조회/생성은 교체하지 않는다.
struct SessionPriestMenuUi {
    struct Item { std::string label;std::uint32_t command{},parameter{};bool enabled{}; }; // 실제 작성 문자열과 전체DWORD·허용값이다.
    std::vector<Item> items; // 실제 사제 메뉴가 작성한 행이다.
    std::vector<std::string> messages; // 선택 관문이 요청한 원본 Tell 키다.
    Sid preview{};std::uint32_t flags{},takes{},menus{}; // 실제 생성Form과 커서 취득 경계에 전달한 값·횟수다.
    // 원본 고정 메뉴 서식의%s/%d/%%를 명시 UI 경계로 처리하며 게임 결과는 계산하지 않는다.
    static std::string Format(std::string_view key,std::span<const PriestMenuArgument> values) {
        std::string text;std::size_t argument=0;
        // 원문 서식의 지정자 순서를 유지한다.
        for (std::size_t i=0;i<key.size();++i) {
            if (key[i]!='%') { text+=key[i];continue; }if (++i>=key.size()) throw std::logic_error("메뉴 검사 서식 끝");
            if (key[i]=='%') { text+='%';continue; }if (argument>=values.size()) throw std::logic_error("메뉴 검사 인자 부족");
            if (key[i]=='s') text+=std::get<std::string>(values[argument++]);
            else if (key[i]=='d') text+=std::to_string(std::get<std::int32_t>(values[argument++]));
            else throw std::logic_error("메뉴 검사 미지원 서식");
        }
        CHECK(argument==values.size());return text;
    }
    // 그림/메뉴 작성과 실제 커서 취득의 외부 경계만 제공한다.
    PriestMenuHooks Hooks(std::span<const RiftTypeRecord> types) {
        PriestMenuHooks hooks;hooks.format=Format;hooks.description=[types](std::uint32_t type) { return types[type].description; };
        hooks.beginFancy=[] {};hooks.beginVertical=[] {};hooks.title=[](std::string_view) {};hooks.separator=[] {};
        hooks.takeMenu=[this] { return ++menus; };hooks.submenu=[](std::string_view,std::uint32_t,bool) {};
        hooks.item=[this](std::string_view label,std::uint32_t command,std::uint32_t parameter,bool enabled) { items.push_back({std::string(label),command,parameter,enabled}); };
        hooks.baseMenu=[](Sid) {}; // Carrier/객체 GUI 부모는 이 검사의 명시 외부 경계다.
        hooks.tell=[this](std::string_view key) { messages.emplace_back(key); };
        hooks.takePreview=[this](Sid form,std::uint32_t input) { preview=form;flags=input;++takes; };return hooks;
    }
};
}
// 실제 GameWorld가 표시하는 같은 세션의 사제/허용/SP/내용물 생성기를 메뉴와 건설 선택에서 사용한다.
TEST_CASE(CampaignSession_PriestMenuUsesActualWorldAndBuildSelection) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});
    auto& session=world.Session();auto core=session.Core();CHECK(&core.pool==&world.Surfaces().Core().pool);const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value!=0);
    SessionPriestMenuUi ui;auto hooks=ui.Hooks(core.types);hooks.allowed=[](std::uint32_t) { return false; };hooks.money=[](std::uint32_t) { return -999.0f; };
    auto menu=session.MakePriestMenu(std::move(hooks));auto& settings=session.PriestMenuSettings();settings.localOwner=8;settings.editor=true;settings.workshopMode=false;settings.chargeBlocked=false;
    menu->Open(priest);CHECK(!ui.items.empty());CHECK(settings.localOwnerRead()==session.LocalOwner() && !settings.editorRead() && settings.workshopModeRead()==core.ownerMode.battle && settings.chargeBlockedRead());
    const auto type=settings.temples[0];const auto cost=session.HitPoints().TypeCostUnits(type);CHECK(cost>0);
    if (core.types[type].flags2&0x4000) session.AddTechnology(1,core.types[type].assetName);
    session.SetSp(1,static_cast<float>(cost-1));CHECK(!menu->SelectBuild(type,priest));CHECK(ui.takes==0 && ui.messages==std::vector<std::string>({"BuildNoBucks"}));
    const float money=static_cast<float>(cost+1000);session.SetSp(1,money);CHECK(menu->SelectBuild(type,priest));CHECK(ui.takes==1 && ui.preview.value!=0 && ui.flags==0x160);
    const auto form=core.pool.Slot(ui.preview);CHECK(form[10]==6 && Get(form,18,2)==type);
    CHECK(session.Priests().Carrier().FindContained(priest,0x20,type,0)==ui.preview);CHECK(session.Sp(1)==money);
}
// 같은 메뉴 객체가 나중에 바뀐 실제 기술 체인과TYPE 제외 목록을 다시 읽는다.
TEST_CASE(CampaignSession_PriestMenuTracksTechnologyAndTypeExclusions) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();
    auto core=session.Core();SessionPriestMenuUi ui;auto menu=session.MakePriestMenu(ui.Hooks(core.types));const auto type=session.PriestMenuSettings().temples[0];
    session.ClearTechnology(1);CHECK(!menu->HasTheme(core.types[type].theme));session.AddTechnology(1,core.types[type].assetName);CHECK(menu->HasTheme(core.types[type].theme));
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};session.SetSp(1,100000);menu->AddBuild(type,false,priest,true);CHECK(ui.items.size()==1 && ui.items.back().enabled);
    auto& excluded=session.ConstructionExcludedTypes();excluded.entries={type};excluded.count=1;menu->AddBuild(type,false,priest,true);CHECK(ui.items.size()==2 && !ui.items.back().enabled);
    excluded.count=0;menu->AddBuild(type,false,priest,true);CHECK(ui.items.size()==3 && ui.items.back().enabled);CHECK(&excluded==&session.ConstructionExcludedTypes());
}
// 실제 Session 설치 전에 새 메뉴가 생성/게임 조회를 진행하지 않는 관문을 확인한다.
TEST_CASE(CampaignSession_PriestMenuRejectsUninstalledSession) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());CHECK(Throws([&] { static_cast<void>(session.MakePriestMenu({})); }));
}

// 실제 Session의 마나8개가 같은 Factory/표시/공간/ProcessForm/Kernel에서 생성·이동·숨김·삭제된다.
TEST_CASE(CampaignSession_CursorManaUsesActualWorldHostAndDisplay) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});
    auto& session=world.Session();auto core=session.Core();auto& surfaces=world.Surfaces();const auto baseline=core.kernel.Size();

    const auto parent=surfaces.Create(123,2);Put(core.pool.AllocatedBytes(parent),14,std::bit_cast<std::uint32_t>(100.0f));Put(core.pool.AllocatedBytes(parent),18,std::bit_cast<std::uint32_t>(110.0f));
    CursorLifecycleHooks hooks;session.BindCursorMana(hooks);const auto token=hooks.createMana(parent,0);CHECK(token!=0);session.CursorDropSettings().cursorGump=token;
    auto* process=dynamic_cast<CursorManaProcess*>(core.kernel.Get(token));CHECK(process && core.kernel.Size()==baseline+1 && core.processes.IsRegistered(token,process));
    CHECK(process->Parent()==parent && Get(process->Fields().bytes,0x84)==8 && Get(process->Fields().bytes,0x98)==0);
    std::array<Sid,8> markers{}; // 실제 프로세스가 보관한 월드 SID이며 별도 표시 사본이 아니다.
    // 생성된 같은 raw 슬롯에서 실제 TYPE/WORD/extra/프레임/공간 등록을 확인한다.
    for (std::size_t index=0;index<markers.size();++index) {
        markers[index]=Sid{static_cast<std::uint16_t>(Get(process->Fields().bytes,12+4*index))};const auto raw=core.pool.Slot(markers[index]);
        CHECK(raw[10]==103 && Get(raw,12,2)==1 && (raw[40]&8) && !(raw[11]&4));CHECK(Get(raw,36)==CursorManaThemeFrame(core.types[123].theme));
    }
    core.processState.now+=.125;process->RunFrame();CHECK(Get(core.pool.Slot(markers[0]),14)!=std::bit_cast<std::uint32_t>(1.0f));
    hooks.processEvent(token,1,1,0);
    // 실제 Unpop은 같은8개 객체를 void로 만들고 살아 있는 SID/프로세스를 유지한다.
    for (const auto marker:markers) CHECK(core.pool.Slot(marker)[11]&4);
    hooks.processEvent(token,1,0,0);
    // 실제 Repop은 같은 좌표/프레임으로 공간과 표시 등록을 복원한다.
    for (const auto marker:markers) CHECK(!(core.pool.Slot(marker)[11]&4));
    hooks.removeProcess(token,0);CHECK(!core.kernel.Get(token) && core.kernel.Size()==baseline && session.CursorDropSettings().cursorGump==0);
    // 실제 가상 삭제가 표시 객체와 ProcessForm의 종속 수명을 끝낸다.
    for (const auto marker:markers) CHECK(core.pool.Slot(marker)[11]&1);
    CHECK(Get(core.pool.Slot(parent),6,2)==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
    CHECK(Throws([&] { hooks.processEvent(token,1,1,0); }));CHECK(Throws([&] { hooks.removeProcess(token,0); }));
    const auto secondParent=surfaces.Create(103,2);Put(core.pool.AllocatedBytes(secondParent),14,std::bit_cast<std::uint32_t>(100.0f));Put(core.pool.AllocatedBytes(secondParent),18,std::bit_cast<std::uint32_t>(110.0f));
    const auto secondToken=hooks.createMana(secondParent,0);session.CursorDropSettings().cursorGump=secondToken;auto* second=dynamic_cast<CursorManaProcess*>(core.kernel.Get(secondToken));CHECK(second);
    std::array<Sid,8> dependentMarkers{}; // 부모 삭제가 정리해야 하는 실제 두 번째 표시 객체들이다.
    // 삭제 전에 번호를 저장하고 해제된 C++ 프로세스 포인터는 이후에 읽지 않는다.
    for (std::size_t index=0;index<dependentMarkers.size();++index) dependentMarkers[index]=Sid{static_cast<std::uint16_t>(Get(second->Fields().bytes,12+4*index))};
    CHECK(surfaces.Destroy(secondParent,0));CHECK(!core.kernel.Get(secondToken) && core.kernel.Size()==baseline && session.CursorDropSettings().cursorGump==0);
    // 실제 부모의 종속 ProcessForm 삭제가 Kernel 소멸자와8개 표시 삭제까지 이어진다.
    for (const auto marker:dependentMarkers) CHECK(core.pool.Slot(marker)[11]&1);
    CHECK(core.pool.Slot(secondParent)[11]&1);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
    // 실제 자기 삭제의 두 갈래를 같은 Kernel 소유 객체에서 실행한다. 마지막 포인터는 삭제 뒤 재사용하지 않는다.
    for (const bool rectangular:{false,true}) {
        const auto closingToken=hooks.createMana(parent,0);session.CursorDropSettings().cursorGump=closingToken;
        auto* closing=dynamic_cast<CursorManaProcess*>(core.kernel.Get(closingToken));CHECK(closing);std::array<Sid,8> closingMarkers{}; // 원본 종료가 반납할 실제 표시 목록이다.
        // 삭제 전에 목록을 저장하며 원형/사각형 두 프레임의 끝을 따로 관찰한다.
        for (std::size_t index=0;index<closingMarkers.size();++index) closingMarkers[index]=Sid{static_cast<std::uint16_t>(Get(closing->Fields().bytes,12+4*index))};
        if (rectangular) {
            // 사각형의 표시 객체가 모두 끝난 입력을 실제 월드 삭제로 준비한다.
            for (const auto marker:closingMarkers) CHECK(surfaces.Destroy(marker,0));
            // 원본 사각형 frame은 SID0 목록에서 Kill 후 마지막 시각을 계속 쓴다.
            for (std::size_t index=0;index<30;++index) Put(closing->Fields().bytes,12+4*index,0);
            Put(closing->Fields().bytes,0x98,1);Put(closing->Fields().bytes,0x88,5);
        } else hooks.processEvent(closingToken,3,0,0);
        closing->RunFrame();CHECK(!core.kernel.Get(closingToken) && core.kernel.Size()==baseline && session.CursorDropSettings().cursorGump==0);
        // 실제 자기 삭제가 끝난 뒤에는 SID와 Kernel 상태만 읽는다.
        for (const auto marker:closingMarkers) CHECK(core.pool.Slot(marker)[11]&1);
        CHECK(Get(core.pool.Slot(parent),6,2)==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
    }
}

// 실제 Session의 원형·직선·Wind 관측각을 같은 월드/총 자료/Host에 연결하고 표시 수명 전체를 검사한다.
TEST_CASE(CampaignSession_CursorRangeActualWorldAndSharedLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services();std::vector<std::uint32_t> diagnostics; // 실제 color29 진단 출력의 외부 경계다.
    services.cursorRangeDiagnostic=[&](std::uint32_t color,std::string_view text) { CHECK(text=="Dead circle range!");diagnostics.push_back(color); };
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});
    auto& session=world.Session();auto core=session.Core();auto& surfaces=session.World();const auto baseline=core.kernel.Size();CursorLifecycleHooks hooks;session.BindCursorRange(hooks);
    CHECK(&session.CursorRangeSettings().coordinates==&session.CursorManaSettings());
    // 썬 발사기/캐논/펜스와 Wind 크로스보우의 실제 타입 자료를 각각 사용한다.
    for (const std::uint32_t type:{71U,128U,136U,140U}) {
        const auto parent=surfaces.Create(type,2);auto parentRaw=core.pool.AllocatedBytes(parent);Put(parentRaw,14,std::bit_cast<std::uint32_t>(100.0f));Put(parentRaw,18,std::bit_cast<std::uint32_t>(110.0f));parentRaw[40]|=8;
        const auto token=hooks.createRange(parent,0);session.CursorDropSettings().cursorGump=token;auto* process=dynamic_cast<CursorRangeProcess*>(core.kernel.Get(token));CHECK(process && process->Parent()==parent && core.processes.IsRegistered(token,process));
        const bool rays=core.types[type].group==1 || core.types[type].group==6;CHECK(Get(process->Fields().bytes,12)==(rays ? 0U : 1U));
        std::vector<Sid> markers; // 실제 필드의 마커 SID이며 별도 표시 객체를 만들지 않는다.
        // 실제100칸에서 살아 있는 표시 객체와 프레임을 읽는다.
        for (std::size_t index=0;index<100;++index) {
            const auto value=Get(process->Fields().bytes,20+4*index);if (!value) continue;markers.push_back(Sid{static_cast<std::uint16_t>(value)});const auto raw=core.pool.Slot(markers.back());CHECK(raw[10]==104 && (raw[40]&8) && !(raw[11]&4));
        }
        CHECK(!markers.empty() && markers.size()==Get(process->Fields().bytes,0x1ac));core.processState.now+=.5;process->RunFrame();CHECK(Get(core.pool.Slot(markers.front()),14)!=std::bit_cast<std::uint32_t>(1.0f) || Get(core.pool.Slot(markers.front()),18)!=std::bit_cast<std::uint32_t>(1.0f));
        hooks.processEvent(token,1,1,0);
        // 같은 객체를 실제 Unpop한 결과를 확인한다.
        for (const auto marker:markers) CHECK(core.pool.Slot(marker)[11]&4);
        hooks.processEvent(token,1,0,0);
        // 같은 좌표와 프레임으로 실제 공간/표시를 복원한다.
        for (const auto marker:markers) CHECK(!(core.pool.Slot(marker)[11]&4));
        hooks.removeProcess(token,0);CHECK(!core.kernel.Get(token) && session.CursorDropSettings().cursorGump==0 && core.kernel.Size()==baseline);
        // 명시 제거가 원본의 고정100칸 가상 삭제로 이어진다.
        for (const auto marker:markers) CHECK(core.pool.Slot(marker)[11]&1);
        const auto mana=hooks.createMana(parent,0);const auto closingToken=hooks.createRange(parent,0);session.CursorDropSettings().cursorGump=mana;auto* closing=dynamic_cast<CursorRangeProcess*>(core.kernel.Get(closingToken));CHECK(closing);
        hooks.processEvent(closingToken,3,0,0);closing->RunFrame();CHECK(!core.kernel.Get(closingToken) && core.kernel.Get(mana) && session.CursorDropSettings().cursorGump==0);hooks.removeProcess(mana,0);CHECK(core.kernel.Size()==baseline);
        const auto dependentToken=hooks.createRange(parent,0);session.CursorDropSettings().cursorGump=dependentToken;CHECK(surfaces.Destroy(parent,0));CHECK(!core.kernel.Get(dependentToken) && core.kernel.Size()==baseline && session.CursorDropSettings().cursorGump==0);
        CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
    }
    CHECK(diagnostics.empty());
}

// 실제 미션의 공급 장부/TYPE mana/방향별 동맹/같은30칸 목록을 커서 판정에 연결한다.
TEST_CASE(CampaignSession_CursorEnergyActualProvidersAndSharedSources) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services();std::vector<Sid> lostSounds; // 실제 클라이언트의 unitLost.wav 장치 요청만 외부에서 관찰한다.
    services.unitLost=[&](Sid sid) { lostSounds.push_back(sid); };
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});
    auto& session=world.Session();auto core=session.Core();const auto before=core.bookkeeping.providers.count;const auto origin=EmptySessionSupport(core);const auto created=session.World().Create(76); // 실제 Rain 제네레이터이며 원본 Factory/Owner/Pop이 공급 장부를 등록한다.
    core.owner.Set(created,1);session.World().Pop(created,static_cast<float>(origin.x+2),static_cast<float>(origin.y+2),0);
    const auto providers=core.bookkeeping.providers.Items();CHECK(core.bookkeeping.providers.entries.size()==kEnergyProviderCapacity);CHECK(providers.size()==before+1 && providers.back()==created.value);
    const Sid source=created;const auto raw=core.pool.Slot(source);const auto& type=core.types[raw[10]];const auto owner=static_cast<std::uint32_t>(raw[34]);
    const float x=static_cast<float>(static_cast<double>(std::bit_cast<float>(Get(raw,14)))-type.footX*0.5),y=static_cast<float>(static_cast<double>(std::bit_cast<float>(Get(raw,18)))-type.footY*0.5);
    CHECK(&session.EnergySupply().Pool()==&core.pool && session.EnergySources().entries.size()==kEnergySourceCapacity);
    CHECK(session.EnergySupply().Covers(source,{},x,y,0,0));CHECK(!session.EnergySupply().Covers(source,{},x+22,y,0,0));CHECK(session.EnergySupply().Covers(source,{},x+22,y,0.001f,0));
    constexpr std::uint32_t target=75; // 실제 Wind 제네레이터의 명시 mana="s"이며 다른 원소 공급원 한 개로 충족한다.
    CursorPlacementHooks hooks;session.BindCursorEnergy(hooks);std::string label;CHECK(hooks.describe(label,x,y,target,owner)>0 && label=="S");
    auto& selected=session.EnergySources();CHECK(selected.count>0);const auto storage=selected.entries.data();const auto selectedSource=selected.entries.front();CHECK(session.EnergySupply().TakeSource()==selectedSource && selected.entries.front()==0 && selected.entries.data()==storage);
    auto alliances=session.Alliances();std::fill(alliances.begin(),alliances.end(),0);const auto other=(owner+1)%9;
    const auto contains=[&] { const auto current=selected.Items();return std::find(current.begin(),current.end(),source.value)!=current.end(); }; // 실제 마지막 선택 목록을 직접 읽는다.
    hooks.describe(label,x,y,target,other);CHECK(!contains());alliances[owner*9+other]=1;hooks.describe(label,x,y,target,other);CHECK(contains());
    // 반대 방향의 동맹을 대신 읽으면 이 검사가 실패한다.
    alliances[owner*9+other]=0;alliances[other*9+owner]=1;hooks.describe(label,x,y,target,other);CHECK(!contains());
    session.EnergySettings().allied=0;alliances[owner*9+other]=1;hooks.describe(label,x,y,target,other);CHECK(!contains());
    // 동일 실제 공급 설정을 다음 가상 범위 호출에서 즉시 읽는다.
    session.EnergySettings().radiusSquared=0;CHECK(!session.EnergySupply().Covers(source,{},x,y,0,0));session.EnergySettings().radiusSquared=484;
    CHECK(session.World().Destroy(created,0));CHECK(core.bookkeeping.providers.count==before);hooks.describe(label,x,y,target,owner);CHECK(!contains());
    CHECK(lostSounds.size()==1 && lostSounds.front()==created);
    CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
}

// 같은 실제 Session의 공급 선택 목록을 Builder가 복사 없이 소비하는 연결 계약이다.
TEST_CASE(CampaignSession_ConstructionBuilderSharesActualEnergySources) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();auto core=session.Core();
    const auto origin=EmptySessionSupport(core);const auto provider=session.World().Create(76);core.owner.Set(provider,1);session.World().Pop(provider,static_cast<float>(origin.x+2),static_cast<float>(origin.y+2),0);
    const auto raw=core.pool.Slot(provider);const auto& type=core.types[76];const float x=static_cast<float>(static_cast<double>(std::bit_cast<float>(Get(raw,14)))-type.footX*0.5),y=static_cast<float>(static_cast<double>(std::bit_cast<float>(Get(raw,18)))-type.footY*0.5);
    std::string demand;CHECK(session.EnergySupply().Evaluate(demand,x,y,75,1)>0);session.EnergySupply().SortSources(demand,x,y);CHECK(session.EnergySources().count>0);
    ConstructionBuilderState state;state.sources={{0xdeadbeefU},1};ConstructionBuilderHooks hooks;
    hooks.approach=[](const std::array<float,4>&,float,float,std::uint32_t)->ConstructionNoticePoint { throw std::logic_error("선택 소비 검사에서 접근점 실행"); };
    hooks.move=[](Sid,float,float)->ConstructionBuilderPath* { throw std::logic_error("선택 소비 검사에서 이동 실행"); };hooks.speed=[](ConstructionBuilderPath&,float,std::uint32_t) { throw std::logic_error("선택 소비 검사에서 속도 실행"); };
    auto builder=session.MakeConstructionBuilder(state,std::move(hooks));CHECK(&builder->Pool()==&core.pool);const auto first=session.EnergySources().entries.front();CHECK(first==provider.value);
    CHECK(builder->TakeSource()==first && session.EnergySources().entries.front()==0 && state.sources.entries.front()==0xdeadbeefU && state.sources.count==1);
    CHECK(builder->TakeSource()==0); // 이 요새의 현재 공급 범위에 있는 실제 선택을 모두 소비했다.
}
// 실제105 생성자/표시/Start39/20ms 이동/빈 대상 도착/공통 삭제를 같은 설치 월드에서 실행한다.
TEST_CASE(CampaignSession_ManaBoltUsesActualFlightAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();const auto origin=EmptySessionSupport(core);
    const auto bolt=session.World().Create(105,2);core.owner.Set(bolt,1);session.World().Pop(bolt,static_cast<float>(origin.x),static_cast<float>(origin.y),0);const auto before=external.notices;
    auto* path=session.StartConstructionMovement({bolt,{static_cast<float>(origin.x+2),static_cast<float>(origin.y)},1,0,0,0,0});CHECK(path!=nullptr);CHECK(path->MovementType()==39);CHECK(path->Runtime().Count()==2);const auto pid=core.kernel.Find(*path);
    // 같은 실제 시계를 전진시키며 등록이 살아 있는 경로만 호출한다. 다른 미완성 Regular 분배는 실행하지 않는다.
    for (int step=0;step<2000 && core.kernel.Get(pid);++step) { core.processState.now=step*0.02;path->RunFrame(); }
    CHECK(core.kernel.Get(pid)==nullptr);CHECK((core.pool.Slot(bolt)[11]&3)!=0);CHECK(external.notices>before);
}
// 실제134를 현재 사제와 같은 표면에서 생성하고 기존23 BFS/빈 대상 도착/삭제로 연결한다.
TEST_CASE(CampaignSession_ConstructionTeleportUsesActualSurfacePath) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();Sid priest{};
    // 설치된 실제 객체 중 살아 있고 공간 등록된 첫 사제 위치만 입력으로 고른다.
    for (std::uint32_t id=5;id<core.pool.Capacity();++id) { const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(id)});if (raw[10]==158 && !(raw[11]&7)) { priest=Sid{static_cast<std::uint16_t>(id)};break; } }
    CHECK(priest.value!=0);if (!priest.value) return;const auto raw=core.pool.Slot(priest);const float x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));
    const auto builder=session.World().Create(134,2);core.owner.Set(builder,raw[34]);session.World().Pop(builder,x,y,0);auto* path=session.StartConstructionMovement({builder,{x,y},1,0,0,0,0});CHECK(path!=nullptr);if (!path) return;
    CHECK(path->MovementType()==23);CHECK(path->Runtime().Count()>0);const auto pid=core.kernel.Find(*path);
    // 같은 실제 시계만 전진시키고 경로 자신이 삭제되면 즉시 호출을 끝낸다.
    for (int step=0;step<2000 && core.kernel.Get(pid);++step) { core.processState.now=step*0.02;path->RunFrame(); }
    CHECK(core.kernel.Get(pid)==nullptr);CHECK((core.pool.Slot(builder)[11]&3)!=0);
}
// 실제 타이머0→비행→도착의46/61 교체→타이머1 위치 조정→대상 완료 뒤 자기 삭제를 조립한다.
TEST_CASE(CampaignSession_ManaBoltRegularFollowsArrivalAndCurrentTarget) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();const auto origin=EmptySessionSupport(core);const float x=static_cast<float>(origin.x+2),y=static_cast<float>(origin.y+2);
    const auto target=session.World().Create(76);core.owner.Set(target,1);core.pool.AllocatedBytes(target)[40]|=1;session.World().Pop(target,x,y,0);
    const auto bolt=session.World().Create(105,2);core.owner.Set(bolt,1);session.World().Pop(bolt,x,y,0);const auto notices=external.notices;
    auto* initial=core.processes.AddRegular(bolt,0,0.05f);core.processes.AddSharedRegular(bolt,9,0.05f);
    core.processState.now=initial->Time();initial->RunFrame();CHECK(initial->Count()==1 && initial->Payload()==0.05f);CHECK(external.notices>notices);
    auto* path=session.StartConstructionMovement({bolt,{x,y},1,0,0,0,0});CHECK(path!=nullptr);if (!path) return;const auto pathId=core.kernel.Find(*path);const auto began=core.processState.now;
    // 실제 이동만 실행하여 같은 Target에서 도착의 Regular 교체를 확인한다.
    for (int step=0;step<2000 && core.kernel.Get(pathId);++step) { core.processState.now=began+step*0.02;path->RunFrame(); }
    CHECK(core.kernel.Get(pathId)==nullptr);CHECK(core.processes.FindEvent(bolt,0)==nullptr);CHECK(!FindAttachedProcess(core.pool,bolt,61).value);CHECK((core.pool.Slot(bolt)[11]&3)==0); // 반납한 Kernel 번호는 새 event1이 즉시 재사용할 수 있다.
    auto* arrived=core.processes.FindEvent(bolt,1);CHECK(arrived!=nullptr);if (!arrived) return;const auto arrivedId=core.kernel.Find(*arrived);
    core.display.SetView({319,127,65536,{0,0,1024,768}});session.SetParticleView({319,127,0,0,1024,768},{});core.processState.now=arrived->Time();arrived->RunFrame();
    CHECK(arrived->Count()==1 && arrived->Payload()==0.05f);CHECK(Get(core.pool.Slot(bolt),14)!=std::bit_cast<std::uint32_t>(x) || Get(core.pool.Slot(bolt),18)!=std::bit_cast<std::uint32_t>(y));
    core.pool.AllocatedBytes(target)[40]&=static_cast<std::uint8_t>(~1U);core.processState.now=arrived->Time();arrived->RunFrame();CHECK(core.kernel.Get(arrivedId)==nullptr);CHECK((core.pool.Slot(bolt)[11]&3)!=0);
}
// 공급원 선택과 접근점/실제23/39/현재 도착 시각/Prepare를 호출자 모형 없이 연결한다.
TEST_CASE(CampaignSession_ConstructionBuilderUsesActualMovementAndTime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    // 원본 Fort 내용물 형식의 준비된 windBattery 한 항목을 입력에 추가한다. 생산 완료 자체를 모형으로 대신하지 않는다.
    const auto prepareContents=[](FortChunkSection& section) {
        // 같은 저장 섹션의 청크를 원래 순서로 읽는다.
        for (auto& chunk:section.chunks) {
            // 기존 소유자1 Vortex의 실제 저장 내용물 로더를 사용한다.
            for (auto& object:chunk.objects) if (object.type==124) { if (!object.contents) object.contents=FortContentList{};FortContent ready;ready.type=75;ready.listFlags=0x10;ready.listFlagsStored=true;object.contents->items.push_back(ready); }
        }
    };
    // 저장 지역 섹션과 별도 Chaff를 같은 원본 로더가 읽게 한다.
    for (auto& section:fort.territories) prepareContents(section);prepareContents(fort.chaff);
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();
    const auto factories=core.bookkeeping.ownerFactories[1].Items();
    CHECK(!factories.empty());if (factories.empty()) return;const Sid factory{static_cast<std::uint16_t>(factories.front())};const auto rawFactory=core.pool.Slot(factory);
    const auto fx=static_cast<int>(std::bit_cast<float>(Get(rawFactory,14))+0.9999f),fy=static_cast<int>(std::bit_cast<float>(Get(rawFactory,18))+0.9999f);const auto surface=core.hash.Cell(0,fx,fy);CHECK(surface!=0);if (!surface) return;const auto graph=core.pool.Slot(Sid{surface})[30];CellPoint point{-1,-1};
    // 같은 실제 Graph의 비어 있는3×3 지면만 입력 위치로 고른다. 확정 결과나 경로는 합성하지 않는다.
    for (int y=4;y<248 && point.x<0;++y) {
        // 공장과 적당히 떨어진 곳을 골라 실제 BFS가 이동 시각을 만들게 한다.
        for (int x=4;x<248;++x) {
            const auto distance=std::abs(x-fx)+std::abs(y-fy);if (distance<5 || distance>15) continue;bool free=true;
            // 현재3×3 지면의 점유/Graph를 모두 확인한다.
            for (int dy=0;dy<3 && free;++dy) {
                // 각 칸은 같은 원본 surface 지도와 spot을 읽는다.
                for (int dx=0;dx<3;++dx) { const auto sid=core.hash.Cell(0,x+dx,y+dy);const auto spot=core.spots[(y+dy)*256+x+dx];if (!sid || core.pool.Slot(Sid{sid})[30]!=graph || !(spot&6) || (spot&0x38)) { free=false;break; } }
            }
            if (free) { point={x+2,y+2};break; }
        }
    }
    CHECK(point.x>=0);if (point.x<0) return;session.AddTechnology(1,core.types[75].assetName);
    const auto target=session.World().Create(75,2);core.owner.Set(target,1);core.pool.AllocatedBytes(target)[40]|=1;session.World().Pop(target,static_cast<float>(point.x),static_cast<float>(point.y),0);
    const auto source=session.World().Create(76);core.owner.Set(source,1);session.World().Pop(source,static_cast<float>(point.x+4),static_cast<float>(point.y+4),0);
    ConstructionBuilderState state;auto builder=session.MakeConstructionBuilder(state,{});std::vector<Sid> oldBuilders;
    // 새134의 실제 마지막 시각을 찾을 때만 현재 client 풀을 읽는다.
    const auto latest=[&]() -> RawPathProcess* {
        // 기존에 관찰한134는 다음 Move의 반환 경로와 구분한다.
        for (std::uint32_t id=5;id<core.pool.Layout().serverFirst;++id) { const Sid sid{static_cast<std::uint16_t>(id)};const auto raw=core.pool.Slot(sid);if (raw[10]==134 && !(raw[11]&7) && std::find(oldBuilders.begin(),oldBuilders.end(),sid)==oldBuilders.end()) { oldBuilders.push_back(sid);const auto form=FindAttachedProcess(core.pool,sid,23);return form.value ? dynamic_cast<RawPathProcess*>(core.processes.FromForm(form)) : nullptr; } }return nullptr;
    };
    const auto first=builder->Move(1,target,0,0);CHECK(first>=0);auto* initial=latest();CHECK(initial!=nullptr);if (!initial) return;CHECK(first==initial->Runtime().Time(initial->Runtime().Count()-1));
    const auto requested=std::bit_cast<std::uint64_t>(first+7.375);const auto adjusted=builder->Move(1,target,static_cast<std::uint32_t>(requested),static_cast<std::uint32_t>(requested>>32));auto* prepared=latest();CHECK(prepared!=nullptr);if (!prepared) return;CHECK(adjusted==prepared->Runtime().Time(prepared->Runtime().Count()-1));CHECK(adjusted!=first);
    std::size_t bolts=0;
    // 이번 두 Move가 실제 공급원에서 만든105/39/초기 Regular0를 같은 Host에서 확인한다.
    for (std::uint32_t id=5;id<core.pool.Layout().serverFirst;++id) { const Sid sid{static_cast<std::uint16_t>(id)};const auto raw=core.pool.Slot(sid);if (raw[10]==105 && !(raw[11]&7)) { CHECK(FindAttachedProcess(core.pool,sid,39).value!=0);CHECK(core.processes.FindEvent(sid,0)!=nullptr);++bolts; } }
    CHECK(bolts>0);
}
// 실제4910a0의9개 공장 용량을 첫 저장 객체 Pop 전의 같은 Session 상태와 비교한다.
TEST_CASE(CampaignSession_PlayerFactoryCapacityMatchesNativeInitialization) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());auto core=session.Core();std::ifstream file(NETSTORM_PLAYER_FACTORY_INIT_FIXTURE);CHECK(file.good());std::string line;std::size_t count=0;
    // 실제 원문이 할당한 capacity와 count0을9개 모두 확인한다.
    while (std::getline(file,line)) { if (line.empty() || line[0]=='#') continue;const auto row=netstorm::test::rawscene::Split(line,'\t');const auto owner=netstorm::test::rawscene::Number(row[0]);CHECK(core.bookkeeping.ownerFactories[owner].entries.size()==netstorm::test::rawscene::Number(row[2]));CHECK(core.bookkeeping.ownerFactories[owner].count==netstorm::test::rawscene::Number(row[3]));++count; }
    CHECK(count==9);
}

// 실제 서버 abstract/134 도착이30과17을 만들고 같은 시각으로 HP/완료/공간/표시 종료를 실행한다.
TEST_CASE(CampaignSession_ConstructionArrivalRunsActualProgressAndVisual) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();Sid priest{};
    // 현재 원본 지형에 설치된 살아 있는 사제 좌표를 동일 경로의 시작/도착 입력으로 쓴다.
    for (std::uint32_t id=5;id<core.pool.Capacity();++id) { const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(id)});if (raw[10]==158 && !(raw[11]&7) && raw[34]==1) { priest=Sid{static_cast<std::uint16_t>(id)};break; } }
    CHECK(priest.value!=0);if (!priest.value) return;const auto raw=core.pool.Slot(priest);const float x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));
    const auto target=session.World().Create(76);CHECK(target.value>=core.pool.Layout().serverFirst);CHECK((core.types[76].flags1&0x8000)!=0);core.owner.Set(target,1);core.pool.AllocatedBytes(target)[40]|=1;SetRawSquidHitPoints(core.pool,target,1);session.World().Pop(target,x,y,0);
    const auto builder=session.World().Create(134,2);core.owner.Set(builder,1);session.World().Pop(builder,x,y,0);auto* path=session.StartConstructionMovement({builder,{x,y},1,0,0,0,0});CHECK(path!=nullptr);if (!path) return;const auto pathId=core.kernel.Find(*path);
    // 실경로가 자기 삭제될 때까지 다른 미션 프로세스는 실행하지 않는다.
    for (int step=0;step<2000 && core.kernel.Get(pathId);++step) { core.processState.now=step*0.02;path->RunFrame(); }
    CHECK((core.pool.Slot(builder)[11]&3)!=0);const auto form=FindAttachedProcess(core.pool,target,30),visualForm=FindAttachedProcess(core.pool,target,17);CHECK(form.value!=0 && visualForm.value!=0);if (!form.value || !visualForm.value) return;
    auto* process=dynamic_cast<ConstructionProcess*>(core.processes.FromForm(form));auto* visual=dynamic_cast<ConstructionVisualProcess*>(core.processes.FromForm(visualForm));CHECK(process!=nullptr && visual!=nullptr);if (!process || !visual) return;
    const auto start=process->Fields().start,end=process->Fields().end;CHECK(end==start+static_cast<double>(0.6f));CHECK(!visual->Fields().visual.instant);CHECK(visual->Fields().visual.opaque>0);CHECK(!FindAttachedProcess(core.pool,target,47).value);
    const auto maximum=core.types[76].maxHitPoints;CHECK(maximum>1);const auto cameraX=static_cast<int>(x*16)-512,cameraY=static_cast<int>(y*11)-384;session.SetParticleView({cameraX,cameraY,0,0,1024,768},{});
    session.RunFrame({start,0,0,1},true);process->RunFrame();visual->RunFrame();CHECK(Get(core.pool.Slot(target),26)==0);CHECK((core.pool.Slot(target)[40]&1)!=0);
    const auto middle=start+(end-start)/2;session.RunFrame({middle,0,middle-start,2},true);process->RunFrame();visual->RunFrame();CHECK(Get(core.pool.Slot(target),26)==static_cast<std::uint32_t>(maximum/2));CHECK((core.pool.Slot(target)[40]&1)!=0);
    const auto& v=visual->Fields().visual;CHECK(v.selected>0);const ScreenRect cut{480,355,540,415};std::vector<std::uint8_t> pixels(1024*768,199),expected=pixels;
    const auto px=static_cast<int>(x*16-cameraX)-v.image.hotspotX,py=static_cast<int>(y*11-cameraY)-v.image.hotspotY;std::size_t painted{}; // 실제 캡처 버퍼의 위치/투명/clip을 독립적으로 대조한다.
    // 현재 출력 버퍼를 기대 표면에 옮기며 clip 밖과255 픽셀은 원래199를 유지한다.
    for (int row=0;row<v.image.height;++row) {
        // 출력 버퍼의 각 픽셀을 현재 정수 카메라/실제 hotspot 위치에 대조한다.
        for (int column=0;column<v.image.width;++column) { const auto color=v.output[static_cast<std::size_t>(row)*v.image.width+column];const auto sx=px+column,sy=py+row;if (color!=255 && sx>=cut.left && sx<cut.right && sy>=cut.top && sy<cut.bottom) { expected[static_cast<std::size_t>(sy)*1024+sx]=color;++painted; } }
    }
    const auto sprites=session.ParticleSprites({0,0,1024,768});CHECK(sprites.size()==1 && painted>0);
    // 수집한 실제 공통 kind3 표시를 현재8비트 표면에 직접 그린다.
    for (const auto& sprite:sprites) if (sprite.procedure) sprite.procedure->draw(pixels,1024,768,cut);
    CHECK(pixels==expected);const auto notices=external.notices;session.RunFrame({end+0.001,0,end-middle,3},true);process->RunFrame();
    CHECK(!FindAttachedProcess(core.pool,target,30).value);CHECK(Get(core.pool.Slot(target),26)==static_cast<std::uint32_t>(maximum));CHECK((core.pool.Slot(target)[40]&1)==0);CHECK((core.pool.Slot(target)[11]&7)==0);CHECK(external.notices>notices);
    visual->RunFrame();CHECK(!FindAttachedProcess(core.pool,target,17).value);CHECK(session.Particles().Rendering().Size()==0);
}
// duration0은 실제 즉시 표시 변형/기본10초와 반복 건설음47을 선택하고 완료 때 소리를 반납한다.
TEST_CASE(CampaignSession_ConstructionDefaultTimeOwnsActualSoundProcess) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();auto core=session.Core();const auto origin=EmptySessionSupport(core);
    const auto target=session.World().Create(76);core.owner.Set(target,1);core.pool.AllocatedBytes(target)[40]|=1;session.World().Pop(target,static_cast<float>(origin.x+2),static_cast<float>(origin.y+2),0);session.StartConstruction(target,0);
    const auto form=FindAttachedProcess(core.pool,target,30),sound=FindAttachedProcess(core.pool,target,47),visualForm=FindAttachedProcess(core.pool,target,17);CHECK(form.value && sound.value && visualForm.value);if (!form.value || !sound.value || !visualForm.value) return;
    auto* process=dynamic_cast<ConstructionProcess*>(core.processes.FromForm(form));auto* visual=dynamic_cast<ConstructionVisualProcess*>(core.processes.FromForm(visualForm));CHECK(process && visual);if (!process || !visual) return;
    CHECK(process->Fields().end==process->Fields().start+10);CHECK(process->Fields().sound==external.names.Lookup("jimBuild.wav"));CHECK(visual->Fields().visual.instant);CHECK(visual->Fields().visual.minimum==2 && visual->Fields().visual.maximum==9);
    session.RunFrame({process->Fields().start,0,0,1},true);core.processes.FromForm(sound)->RunFrame();CHECK(!external.playing.empty());const auto end=process->Fields().end;session.RunFrame({end+0.001,0,10,2},true);process->RunFrame();CHECK(!FindAttachedProcess(core.pool,target,47).value);CHECK(external.playing.empty());CHECK(!FindAttachedProcess(core.pool,target,30).value);CHECK((core.pool.Slot(target)[40]&1)==0);
    visual->RunFrame();CHECK(!FindAttachedProcess(core.pool,target,17).value);
}

namespace {
// 생산 완료를 대신하지 않고 복제한 Fort의 원본 저장 내용물 입력에 준비된75를 추가한다.
void ReadyConstructionInput(FortTemplate& fort,std::int32_t factoryType) {
    const auto add=[factoryType](FortChunkSection& section) { // 원본 저장 섹션의 공장 내용물만 바꾼다.
        // 같은 청크/객체 순서로 실제 저장 내용물 로더에 넘길 입력을 준비한다.
        for (auto& chunk:section.chunks) {
            // 목표 공장은 원본 thewarbegins의124(소유자1)/123(소유자2)다.
            for (auto& object:chunk.objects) if (object.type==factoryType) { if (!object.contents) object.contents=FortContentList{};FortContent ready;ready.type=75;ready.listFlags=0x10;ready.listFlagsStored=true;object.contents->items.push_back(ready); }
        }
    };
    // 지역 섹션과 별도 Chaff를 원래 설치 경로로 읽게 한다.
    for (auto& section:fort.territories) add(section);add(fort.chaff);
}
// 실제 소유자 공장과 같은 Graph의 빈3×3 지면을 찾으며 배치/경로 결과는 합성하지 않는다.
CellPoint GeneralConstructionPoint(const RawWorldCore& core,std::uint32_t owner) {
    const auto factories=core.bookkeeping.ownerFactories.at(owner).Items();if (factories.empty()) throw std::logic_error("일반 건설 검사의 실제 공장 부재");
    const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(factories.front())});const auto fx=static_cast<int>(std::bit_cast<float>(Get(raw,14))+0.9999f),fy=static_cast<int>(std::bit_cast<float>(Get(raw,18))+0.9999f);
    const auto surface=core.hash.Cell(0,fx,fy);if (!surface) throw std::logic_error("일반 건설 공장의 실제 표면 부재");const auto graph=core.pool.Slot(Sid{surface})[30];
    // 실제 공장과5~15칸 떨어진 같은 Graph의 지면을 원본 지도 순서로 찾는다.
    for (int y=4;y<248;++y) {
        // 각 후보의3×3 전체 지면과 점유 비트를 확인한다.
        for (int x=4;x<248;++x) { const auto distance=std::abs(x-fx)+std::abs(y-fy);if (distance<5 || distance>15) continue;bool free=true;
            // 원본 surface/spot을 같은 행 순서로 읽는다.
            for (int dy=0;dy<3 && free;++dy) {
                // 한 칸이라도 다른 Graph/점유면 다음 후보를 찾는다.
                for (int dx=0;dx<3;++dx) { const auto sid=core.hash.Cell(0,x+dx,y+dy);const auto spot=core.spots[(y+dy)*256+x+dx];if (!sid || core.pool.Slot(Sid{sid})[30]!=graph || !(spot&6) || (spot&0x38)) { free=false;break; } }
            }
            if (free) return {x+2,y+2};
        }
    }
    throw std::logic_error("일반 건설 검사의 같은 실제 Graph 빈 지면 부재");
}
// 현재 같은 풀에서 공간 등록된 실제 타입만 관찰한다.
std::vector<Sid> ConstructionLive(const RawWorldCore& core,std::uint32_t type) {
    std::vector<Sid> result; // 결과 SID는 관찰용이며 객체 소유권을 갖지 않는다.
    // client/server 번호를 모두 읽되 free/dead/void는 제외한다.
    for (std::uint32_t id=5;id<core.pool.Capacity();++id) { const Sid sid{static_cast<std::uint16_t>(id)};const auto raw=core.pool.Slot(sid);if (raw[10]==type && !(raw[11]&7)) result.push_back(sid); }return result;
}
// 같은 실제23의 도착과 새30/17을 끝까지 실행한다. 다른 미션 프로세스는 정지한다.
void FinishGeneralConstruction(RawCampaignSession& session,Sid builder,Sid target) {
    auto core=session.Core();const auto form=FindAttachedProcess(core.pool,builder,23);CHECK(form.value!=0);if (!form.value) return;
    auto* path=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(path!=nullptr);if (!path) return;const auto pid=core.kernel.Find(*path);const auto began=core.processState.now;
    // 도착이 경로를 지우거나 PID를 재사용하면 이전 주소를 더 읽지 않는다.
    for (int step=0;step<4000 && core.kernel.Get(pid)==path;++step) { core.processState.now=began+step*0.02;path->RunFrame(); }
    CHECK((core.pool.Slot(builder)[11]&3)!=0);const auto construction=FindAttachedProcess(core.pool,target,30),visualForm=FindAttachedProcess(core.pool,target,17);CHECK(construction.value && visualForm.value);if (!construction.value || !visualForm.value) return;
    auto* process=dynamic_cast<ConstructionProcess*>(core.processes.FromForm(construction));auto* visual=dynamic_cast<ConstructionVisualProcess*>(core.processes.FromForm(visualForm));CHECK(process && visual);if (!process || !visual) return;const auto end=process->Fields().end;
    session.RunFrame({end+0.001,0,0.601,1},true);process->RunFrame();CHECK((core.pool.Slot(target)[40]&1)==0);CHECK((core.pool.Slot(target)[11]&7)==0);CHECK(Get(core.pool.Slot(target),26)==static_cast<std::uint32_t>(session.HitPoints().MaxHitPoints(target)));CHECK(!FindAttachedProcess(core.pool,target,30).value);
    visual->RunFrame();CHECK(!FindAttachedProcess(core.pool,target,17).value);
}
}
// 실제 공장에 준비된 부품이 없으면 기준점 검사에서 실패하고 서버 SID/패킷/Builder를 만들지 않는다.
TEST_CASE(CampaignSession_GeneralConfirmRequiresActualReadyFactory) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();auto core=session.Core();const auto point=GeneralConstructionPoint(core,1);session.AddTechnology(1,core.types[75].assetName);const auto kernel=core.kernel.Size();const auto sp=session.Sp(1);const auto oldBatteries=ConstructionLive(core,75),oldBuilders=ConstructionLive(core,134);
    CHECK(!session.ConfirmConstruction({75,static_cast<float>(point.x),static_cast<float>(point.y),1,0,0,0,0,0,0}));CHECK(session.Construction().LastPacket().empty());CHECK(core.kernel.Size()==kernel && session.Sp(1)==sp);CHECK(ConstructionLive(core,75)==oldBatteries && ConstructionLive(core,134)==oldBuilders);
}
// 다른 Player의 확정은 실제 비용 차감/서버 조각/134 이동/30 완료까지 같은 월드에서 이어진다.
TEST_CASE(CampaignSession_GeneralConfirmBuildsForRemoteOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyConstructionInput(fort,123);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();const auto point=GeneralConstructionPoint(core,2);const auto parent=session.CreatePlayer(2);CHECK(core.pool.Slot(parent)[10]==152 && core.pool.Slot(parent)[34]==2);session.AddTechnology(2,core.types[75].assetName);session.SetSp(2,10000);
    CHECK(session.ConfirmConstruction({75,static_cast<float>(point.x),static_cast<float>(point.y),2,0,0,0,0,0,0}));const auto packet=session.Construction().LastPacket();CHECK(packet.size()==29 && packet[0]==1 && packet[1]==0x3f && packet[26]==1);if (packet.size()!=29) return;const Sid target{static_cast<std::uint16_t>(packet[27]|(std::uint16_t(packet[28])<<8))};const auto raw=core.pool.Slot(target);
    CHECK(target.value>=core.pool.Layout().serverFirst && raw[10]==75 && raw[34]==2 && (raw[40]&1));CHECK(Get(raw,26)==0);CHECK(session.Sp(2)==10000-core.types[75].cost);const auto builders=ConstructionLive(core,134);CHECK(builders.size()==1);if (builders.size()!=1) return;CHECK(core.pool.Slot(builders.front())[34]==2);FinishGeneralConstruction(session,builders.front(),target);
}
// 로컬 예측 조각 교체는 같은 기존134를 유지하며 서버 Notice가 두 번째 Builder를 만들지 않는다.
TEST_CASE(CampaignSession_GeneralConfirmReplacesLocalPredictionAndKeepsBuilder) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyConstructionInput(fort,124);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();const auto point=GeneralConstructionPoint(core,1);session.AddTechnology(1,core.types[75].assetName);session.SetSp(1,5000);
    const auto predicted=session.World().Create(75,2);core.owner.Set(predicted,1);core.pool.AllocatedBytes(predicted)[40]|=1;SetRawSquidHitPoints(core.pool,predicted,0);session.World().Pop(predicted,static_cast<float>(point.x),static_cast<float>(point.y),2);ConstructionBuilderState state;auto mover=session.MakeConstructionBuilder(state,{});const auto arrival=mover->Move(1,predicted,0,0);const auto before=ConstructionLive(core,134);CHECK(before.size()==1);if (before.size()!=1) return;const auto time=std::bit_cast<std::uint64_t>(arrival);
    CHECK(session.ConfirmConstruction({75,static_cast<float>(point.x),static_cast<float>(point.y),1,0,0,0,static_cast<std::uint32_t>(time),static_cast<std::uint32_t>(time>>32),0}));CHECK((core.pool.Slot(predicted)[11]&3)!=0);CHECK(ConstructionLive(core,134)==before);CHECK(session.Sp(1)==5000);const auto packet=session.Construction().LastPacket();CHECK(packet.size()==29);if (packet.size()!=29) return;CHECK(Get(packet,18)==static_cast<std::uint32_t>(time) && Get(packet,22)==static_cast<std::uint32_t>(time>>32));const Sid target{static_cast<std::uint16_t>(packet[27]|(std::uint16_t(packet[28])<<8))};CHECK(target!=predicted && target.value>=core.pool.Layout().serverFirst);FinishGeneralConstruction(session,before.front(),target);
}
// 비용 부족 보고가 실제 Player+54를 보존 갱신하고 이름/표시 비트의 외부 변경을 다음 요청에서 읽는다.
TEST_CASE(CampaignSession_GeneralConfirmShortfallUsesCurrentPlayerRecords) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::vector<std::string> names;std::size_t others{},direct{},money{},rejects{}; // 외부 안내/송신만 관찰하며 비용 계산은 실제 Place가 수행한다.
    services.construction.presentation.format=[&](std::string_view key,std::string_view name) { CHECK(key=="ClientCheated(%s)");names.emplace_back(name);return std::string(name); };
    services.construction.presentation.tellOthers=[&](std::uint32_t owner,std::string_view) { CHECK(owner==2);++others; };services.construction.presentation.tellPlayer=[&](std::uint32_t owner,std::string_view text) { CHECK(owner==2 && text=="ServerThinksYouCheated");++direct; };
    services.construction.presentation.sendMoney=[&](std::uint32_t owner,const ConstructionMoneyNotice& value) { CHECK(owner==2 && value.player==2 && value.source==2 && value.kind==1 && value.amount>0);++money; };services.construction.presentation.sendReject=[&](std::uint32_t owner,const ConstructionRejectNotice& value) { CHECK(owner==2 && value.type==75);++rejects; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();auto records=session.PlayerRecords();Put(records,2*0xb4+0x54,0x123);const std::string first="Alice",last="ABCDEFGHIJKLMNOPQRSTUVWXYZ12345"; // 같은 실제 원문 이름 입력이다.
    std::copy(first.begin(),first.end(),records.begin()+2*0xb4+0xc);records[2*0xb4+0xc+first.size()]=0;
    // 같은 실제 필드의 외부 변경/유지를 세 요청에서 구별한다. flags1은 원본 즉시 배치 입력이다.
    for (int pass=0;pass<3;++pass) {
        if (pass==2) { Put(records,2*0xb4+0x54,0x81);std::copy(last.begin(),last.end(),records.begin()+2*0xb4+0xc);records[2*0xb4+0xc+last.size()]=0; }
        session.SetSp(2,0);const auto origin=EmptySessionSupport(core);CHECK(session.ConfirmConstruction({75,static_cast<float>(origin.x+2),static_cast<float>(origin.y+2),2,0,0,1,0,0,0}));CHECK(Get(records,2*0xb4+0x54)==static_cast<std::uint32_t>(pass==2 ? 0x481 : 0x523));
    }
    CHECK(last.size()==31);CHECK(names==std::vector<std::string>({first,last}));CHECK(others==2 && direct==3 && money==3 && rejects==3);
}

namespace {
// 원본 전체 MayPlace가 실제 허용하는 같은 지면을 검사 입력으로 찾는다. force/권한 우회를 켜지 않는다.
CellPoint CommandPlacementPoint(RawCampaignSession& session,std::uint32_t owner) {
    const auto base=GeneralConstructionPoint(session.Core(),owner); // 같은 실제 공장 Graph의 시작 후보다.
    // 실제 파이프라인이 첫 허용을 반환한 좌표만 사용한다.
    for (int dy=-8;dy<=8;++dy) {
        // 주변의 각 후보를 실제 현재 MayPlace에 전달한다.
        for (int dx=-8;dx<=8;++dx) { const auto x=static_cast<float>(base.x+dx),y=static_cast<float>(base.y+dy);if (session.MayPlaceConstruction({75,0,x,y,0,owner,0})) return {base.x+dx,base.y+dy}; }
    }
    throw std::logic_error("실제 현재 MayPlace가 허용하는 건설 검사 지면 부재");
}
// 패킷 분배의 실제 시각/기록 소비자다. 게임 판단은 Session의 실제 수신기가 수행한다.
PacketDispatchHooks CommandDevice(std::vector<std::vector<std::uint8_t>>& packets) {
    PacketDispatchHooks hooks;hooks.now=[] { return 77U; };hooks.record=[&packets](PacketConnection&,std::span<const std::uint8_t> packet,std::uint16_t size) { packets.emplace_back(packet.begin(),packet.begin()+size); };
    hooks.terminate=[](PacketConnection&) { throw std::logic_error("정책0 요청의 예상 밖 연결 종료"); };hooks.close=[](PacketConnection&) { throw std::logic_error("정상 요청의 예상 밖 연결 닫기"); };hooks.release=[](PacketConnection&) { throw std::logic_error("정상 요청의 예상 밖 연결 반납"); };return hooks;
}
}
// 명시 로컬 예측/기존 Builder 입력에서 실제25바이트→분배→수신→MayPlace→서버 교체→완료를 잇는다.
TEST_CASE(CampaignSession_ConstructionCommandUsesActualRequestAndLocalPrediction) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyConstructionInput(fort,124);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();session.AddTechnology(1,core.types[75].assetName);session.SetSp(1,5000);const auto point=CommandPlacementPoint(session,1);
    const auto predicted=session.World().Create(75,2);core.owner.Set(predicted,1);core.pool.AllocatedBytes(predicted)[40]|=1;SetRawSquidHitPoints(core.pool,predicted,0);session.World().Pop(predicted,static_cast<float>(point.x),static_cast<float>(point.y),2);ConstructionBuilderState state;auto mover=session.MakeConstructionBuilder(state,{});static_cast<void>(mover->Move(1,predicted,0,0));const auto builders=ConstructionLive(core,134);CHECK(builders.size()==1);if (builders.size()!=1) return;
    ConstructionLocalMessage message;message.type=75;message.x=static_cast<float>(point.x);message.y=static_cast<float>(point.y);message.timeLow=0x89abcdef;message.timeHigh=0x7ff80000;std::array<std::uint8_t,25> packet{};WriteConstructionCommand(message,packet);std::vector<std::vector<std::uint8_t>> received;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(received));session.RegisterConstructionCommands(dispatch);PacketConnection connection;connection.sender=1;
    CHECK(dispatch.Dispatch(connection,packet));CHECK(received.size()==1 && received.front()==std::vector<std::uint8_t>(packet.begin(),packet.end()));CHECK((core.pool.Slot(predicted)[11]&3)!=0);CHECK(ConstructionLive(core,134)==builders);CHECK(session.Sp(1)==5000);const auto notice=session.Construction().LastPacket();CHECK(notice.size()==29);if (notice.size()!=29) return;CHECK(Get(notice,18)==0 && Get(notice,22)==0);const Sid target{static_cast<std::uint16_t>(notice[27]|(std::uint16_t(notice[28])<<8))};FinishGeneralConstruction(session,builders.front(),target);
}
// 다른 Player의 수신은 잔액 선검사 뒤 실제 Place에서 한 번만 차감하고 실제 Builder가 완성한다.
TEST_CASE(CampaignSession_ConstructionCommandChargesRemoteOnce) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyConstructionInput(fort,123);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();session.CreatePlayer(2);session.AddTechnology(2,core.types[75].assetName);session.SetSp(2,10000);const auto point=CommandPlacementPoint(session,2);
    ConstructionLocalMessage message;message.type=75;message.x=static_cast<float>(point.x);message.y=static_cast<float>(point.y);std::array<std::uint8_t,25> packet{};WriteConstructionCommand(message,packet);std::vector<std::vector<std::uint8_t>> received;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(received));session.RegisterConstructionCommands(dispatch);PacketConnection connection;connection.sender=2;CHECK(dispatch.Dispatch(connection,packet));CHECK(session.Sp(2)==10000-core.types[75].cost);const auto notice=session.Construction().LastPacket();CHECK(notice.size()==29);if (notice.size()!=29) return;const Sid target{static_cast<std::uint16_t>(notice[27]|(std::uint16_t(notice[28])<<8))};const auto builders=ConstructionLive(core,134);CHECK(builders.size()==1);if (builders.size()==1) FinishGeneralConstruction(session,builders.front(),target);
}
// 클라이언트 예측이 다른 소유자면 전체 요청에서 거부하고 원본 수신은 환불/취소만 요청한다.
TEST_CASE(CampaignSession_ConstructionCommandRejectsForeignPrediction) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::size_t money{},rejects{}; // 실제 송신 장치 경계의 논리 요청만 관찰한다.
    services.construction.presentation.sendMoney=[&](std::uint32_t owner,const ConstructionMoneyNotice& value) { CHECK(owner==1 && value.player==1 && value.kind==1);++money; };services.construction.presentation.sendReject=[&](std::uint32_t owner,const ConstructionRejectNotice& value) { CHECK(owner==1 && value.type==75);++rejects; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyConstructionInput(fort,124);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();session.AddTechnology(1,core.types[75].assetName);session.SetSp(1,5000);const auto point=CommandPlacementPoint(session,1);const auto predicted=session.World().Create(75,2);core.owner.Set(predicted,2);core.pool.AllocatedBytes(predicted)[40]|=1;session.World().Pop(predicted,static_cast<float>(point.x),static_cast<float>(point.y),2);
    ConstructionLocalMessage message;message.type=75;message.x=static_cast<float>(point.x);message.y=static_cast<float>(point.y);std::array<std::uint8_t,25> packet{};WriteConstructionCommand(message,packet);std::vector<std::vector<std::uint8_t>> received;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(received));session.RegisterConstructionCommands(dispatch);PacketConnection connection;connection.sender=1;CHECK(dispatch.Dispatch(connection,packet));CHECK(session.Construction().LastPacket().empty());CHECK(ConstructionLive(core,134).empty());CHECK(money==1 && rejects==1 && session.Sp(1)==5000);CHECK((core.pool.Slot(predicted)[11]&3)==0);
}

namespace {
// 외부 마우스/글자 치수/GUI/화면 갱신/송신만 명시하며 게임 판정은 같은 실제 몸체에 맡긴다.
struct SessionCursorUi {
    ScreenPoint mouse{}; // 현재 마우스는 검사에서 바뀌는 실제 장치 입력 경계다.
    std::size_t dirty{},highlights{},refreshes{},shown{}; // 장치/화면 요청 관찰 횟수다.
    std::vector<std::uint32_t> cursors; // 원본 하드웨어 커서 선택 순서다.
    std::vector<std::vector<std::uint8_t>> sent; // 실제 직렬화가 송신한25바이트다.
    std::function<void()> onSend; // 송신 경계 당시의 예측/비용 순서를 관찰한다.
    // 선택한 외부 경계만 공급하며 배치 허용/에너지/생성/삭제 결과를 덮어쓰지 않는다.
    CampaignCursorEffects Effects() {
        CampaignCursorEffects effects;effects.lifecycle.highlight=[this](std::uint32_t value) { CHECK(value==0);++highlights; };
        effects.lifecycle.selectCursor=[this](std::uint32_t value) { cursors.push_back(value); };
        effects.lifecycle.closePopups=[this] { ++refreshes; };
        effects.lifecycle.translate=[](std::string_view text) { return std::string(text); };
        effects.lifecycle.measure=[](std::string_view text,const std::array<std::uint32_t,8>&) { return std::array<std::int32_t,2>{static_cast<std::int32_t>(text.size()*8),16}; };
        effects.lifecycle.mouse=[this] { return mouse; };
        effects.placement.gumpAt=[](std::int32_t,std::int32_t) { return 0U; }; // 이 입력에는 화면 위 Gump가 없다.
        effects.placement.invalidate=[this](ScreenRect,std::uint32_t) { ++dirty; };
        effects.drop.showSp=[this](std::int32_t amount,std::int32_t,std::int32_t,std::uint32_t flags) { CHECK(amount>0 && flags==0);++shown; };
        effects.drop.battleRefresh=[this] { ++refreshes; };
        effects.drop.refresh=[this](std::uint32_t a,std::uint32_t b,std::uint32_t c) { CHECK(a==0 && b==0 && c==1);++refreshes; };
        effects.drop.closeGump=[this](std::uint32_t,std::uint32_t a,std::uint32_t b,std::uint32_t c) { CHECK(a==3 && b==0 && c==0);++refreshes; };
        effects.send=[this](std::uint32_t destination,std::span<const std::uint8_t> packet) { CHECK(destination==9 && packet.size()==25);if (onSend) onSend();sent.emplace_back(packet.begin(),packet.end()); };
        return effects;
    }
    // 실제 hotspot/그림 치수와 현재 카메라로 목표 정수 월드 칸에 대응하는 마우스 입력을 만든다.
    void Point(RawCampaignSession& session,CellPoint point,std::int32_t cameraX=0,std::int32_t cameraY=0,bool check=true) {
        const auto& placement=session.Cursor().Placement();mouse={point.x*16-cameraX-placement.hotspotX+placement.imageWidth/2,point.y*11-cameraY-placement.hotspotY+placement.imageHeight/2};
        session.Cursor().Update(mouse.x,mouse.y);
        if (check && !session.CursorDropSettings().legal) { const auto& p=session.Cursor().Placement();const auto& d=session.CursorDropSettings();throw std::logic_error("커서 검사 위치 거부: "+std::to_string(d.x)+","+std::to_string(d.y)+" argument="+std::to_string(d.argument)+" describe="+std::to_string(p.descriptionAllowed)+" place="+std::to_string(p.placementAllowed)+" source="+std::to_string(p.sourceAllowed)+" text="+p.description); }
    }
};
// 실제 준비75 입력은 부모의 containerListFlags10이 저장 BYTE보다 우선한다.
void ReadyCursorFactoryInput(FortTemplate& fort) { ReadyConstructionInput(fort,124); }
// 준비 부품은 저장 로더가 생성하고 GUI용 반환8 Form만 실제 공용 contained 생성기로 명시 공급한다.
Sid ReadyCursorFactoryForm(RawCampaignSession& session) {
    const auto core=session.Core();
    // 실제 현재 공장 목록에서 준비10 조각이 있는 부모만 사용한다.
    for (const auto factory:core.bookkeeping.ownerFactories[1].Items()) {
        const Sid parent{static_cast<std::uint16_t>(factory)};
        if (session.Priests().Carrier().FindContained(parent,0x10,75,0).value)
            return session.Cursor().Contents().Create(parent,75,0,8,0x10,3);
    }
    throw std::logic_error("실제 준비75 공장 입력 부재");
}
// 현재 실제 배치가 허용하는 지면에 명시 시작 상태의 완성76 공급원을 실제 생성/소유자/Pop으로 공급한다.
Sid CursorEnergyInput(RawCampaignSession& session) {
    const auto point=CommandPlacementPoint(session,1);auto core=session.Core();const auto before=core.bookkeeping.providers.count;
    const auto source=session.World().Create(76);core.owner.Set(source,1);session.World().Pop(source,static_cast<float>(point.x),static_cast<float>(point.y),0);
    CHECK(core.bookkeeping.providers.count==before+1);return source;
}
// 현재 커서의 실제 에너지/모드1 공간/SourceAllowed가 모두 허용하는 위치만 찾는다.
CellPoint CursorPlacementPoint(RawCampaignSession& session,SessionCursorUi& ui) {
    session.SetParticleView({0,0,0,0,1024,768},{}); // GameWorld의 초기 Home 카메라 대신 명시 장치 입력0을 사용한다.
    auto& drop=session.CursorDropSettings();std::string description;std::size_t energy{},space{},positions{}; // 실패 시 실제 각 판정의 통과 수를 남긴다.
    // 저장 입력에 준비 부품을 더하면 같은 원본 설치/소유권/공장 후보가 바뀔 수 있으므로 현재 월드를 다시 읽는다.
    for (int y=4;y<248;++y) {
        // 실제 판정을 통과한 후보만 마우스 위치 몸체에 넘기며 어떤 허용 비트도 덮어쓰지 않는다.
        for (int x=4;x<248;++x) {
            if (!session.EnergySupply().Evaluate(description,static_cast<float>(x),static_cast<float>(y),drop.type,drop.localPlayer)) continue;++energy;
            if (!session.MayPlaceConstruction({drop.type,drop.argument,static_cast<float>(x),static_cast<float>(y),drop.direction,drop.localPlayer,1})) continue;++space;
            ui.Point(session,{x,y},0,0,false);++positions;if (drop.legal) return {x,y};
        }
    }
    throw std::logic_error("현재 실제 커서 허용 위치 부재: energy="+std::to_string(energy)+" space="+std::to_string(space)+" positions="+std::to_string(positions)+" x="+std::to_string(drop.x)+" y="+std::to_string(drop.y));
}
// 실제 사제 메뉴의 게임 연결과 기본 커서 취득을 사용한다. GUI 작성기는 기존 명시 검사 경계다.
std::unique_ptr<RawPriestMenu> CursorMenu(RawCampaignSession& session,SessionPriestMenuUi& ui) {
    auto hooks=ui.Hooks(session.Core().types);hooks.takePreview={};return session.MakePriestMenu(std::move(hooks));
}
}
// 실제 메뉴 취득→현재 카메라 위치/Canon/SHP/에너지/전체 MayPlace→취소/중첩 삭제를 검사한다.
TEST_CASE(CampaignSession_CursorMenuUsesActualStateAndCancel) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();auto core=session.Core();SessionCursorUi ui;auto& cursor=session.Cursor();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;SessionPriestMenuUi menuUi;auto menu=CursorMenu(session,menuUi);const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};session.AddTechnology(1,core.types[75].assetName);session.SetSp(1,5000);
    CHECK(menu->SelectBuild(75,priest));auto& drop=session.CursorDropSettings();const auto form=drop.preview;CHECK(form.value && core.pool.Slot(form)[10]==6 && Get(core.pool.Slot(form),18,2)==75);CHECK(drop.lockDepth==0 && drop.costFlags==0x160 && drop.localPlayer==1);CHECK(cursor.Lifecycle().previewCount>0 && cursor.Lifecycle().previews[0].value);CHECK(cursor.Lifecycle().costImage!=0 && cursor.Lifecycle().costText==std::to_string(static_cast<int>(core.types[75].cost)));
    const auto previews=cursor.Lifecycle().previews;const auto point=CommandPlacementPoint(session,1);session.SetParticleView({137,91,0,0,1024,768},{});ui.Point(session,point,137,91);CHECK(drop.x==point.x && drop.y==point.y && drop.legal);CHECK(cursor.Placement().sourceAllowed && cursor.Placement().placementAllowed && cursor.Placement().descriptionAllowed);CHECK(session.Sp(1)==5000 && ui.sent.empty());
    cursor.Cancel();CHECK((core.pool.Slot(form)[11]&3)!=0);CHECK(!drop.preview.value && drop.lockDepth==1 && !drop.cursorGump);CHECK(cursor.Lifecycle().previewCount==0);CHECK(session.Sp(1)==5000);
    // 실제 생성된 임시 객체 전부가 같은 가상 삭제 사슬을 통해 반납되었는지 확인한다.
    for (const auto sid:previews) if (sid.value) CHECK((core.pool.Slot(sid)[11]&3)!=0);
    CHECK(ui.dirty>0 && ui.highlights>0 && ui.cursors.back()==1 && ui.sent.empty());
}
// 현재 잡힌 Form의 외부 삭제가 값형0 사본 대신 실제 커서를 찾아 잠긴 꼬리 정리까지 수행한다.
TEST_CASE(CampaignSession_CursorContentDestroyReadsCurrentPreview) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();auto core=session.Core();SessionCursorUi ui;auto& cursor=session.Cursor();cursor.SetEffects(ui.Effects());SessionPriestMenuUi menuUi;auto menu=CursorMenu(session,menuUi);session.AddTechnology(1,core.types[75].assetName);session.SetSp(1,5000);const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(menu->SelectBuild(75,priest));auto& drop=session.CursorDropSettings();const auto form=drop.preview;const auto previews=cursor.Lifecycle().previews;drop.lockDepth=3;const auto dirty=ui.dirty;
    CHECK(session.World().Destroy(form));CHECK((core.pool.Slot(form)[11]&3)!=0);CHECK(!drop.preview.value && drop.lockDepth==1 && !drop.cursorGump && cursor.Lifecycle().previewCount==0);CHECK(ui.dirty>dirty && session.Sp(1)==5000);
    // 잠금이 있어도 실제 Activate1이 미리보기를 모두 지워야 한다.
    for (const auto sid:previews) if (sid.value) CHECK((core.pool.Slot(sid)[11]&3)!=0);
    cursor.Cancel();CHECK(drop.lockDepth==1 && ui.sent.empty());
}
// 실제 생산 칸 Form 취득/내려놓기/로컬 예측/비용→25바이트→서버 전체 요청/교체→같은 Builder 완료를 검사한다.
TEST_CASE(CampaignSession_CursorDropPredictsSendsChargesAndCompletes) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[&](Sid) { ++external.notices; };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyCursorFactoryInput(fort);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();SessionCursorUi ui;auto& cursor=session.Cursor();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;session.AddTechnology(1,core.types[75].assetName);session.SetSp(1,5000);const auto source=CursorEnergyInput(session);CHECK(core.pool.Slot(source)[10]==76);const auto form=ReadyCursorFactoryForm(session);cursor.Take(form,0x41);const auto point=CursorPlacementPoint(session,ui);CHECK(session.CursorDropSettings().legal);
    const auto old=ConstructionLive(core,75);Sid prediction{};std::vector<Sid> builders;
    ui.onSend=[&] { CHECK(session.Sp(1)==5000);const auto current=ConstructionLive(core,75);CHECK(current.size()==old.size()+1);if (current.size()==old.size()+1) { /* 기존 서버 조각과 새 클라이언트 SID의 정렬 순서에 의존하지 않는다. */ for (const auto sid:current) if (std::find(old.begin(),old.end(),sid)==old.end()) prediction=sid; }builders=ConstructionLive(core,134);CHECK(builders.size()==1);CHECK(session.Construction().LastPacket().empty()); };
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";Client client(std::move(options));client.Configuration().Set("cursorSnapToGrid","1");UserInput input(client,world);input.UpdateCursor({InputCode::kLeftButton,ui.mouse.x,ui.mouse.y});input.Event({InputCode::kLeftButton,ui.mouse.x,ui.mouse.y});CHECK(ui.sent.size()==1 && prediction.value && builders.size()==1);CHECK(session.Sp(1)==5000-core.types[75].cost && ui.shown==1);CHECK((core.pool.Slot(form)[11]&3)!=0 && !session.CursorDropSettings().preview.value && cursor.Lifecycle().previewCount==0);if (ui.sent.size()!=1 || !prediction.value || builders.size()!=1) return;const auto& packet=ui.sent.front();CHECK(packet[0]==1 && packet[1]==0x3e && packet[4]==75);CHECK(Get(packet,5)==std::bit_cast<std::uint32_t>(static_cast<float>(point.x)) && Get(packet,9)==std::bit_cast<std::uint32_t>(static_cast<float>(point.y)));CHECK(Get(packet,17)==0 && Get(packet,21)==0);CHECK((core.pool.Slot(prediction)[40]&1)!=0 && prediction.value<core.pool.Layout().serverFirst);
    std::vector<std::vector<std::uint8_t>> received;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(received));session.RegisterConstructionCommands(dispatch);PacketConnection connection;connection.sender=1;CHECK(dispatch.Dispatch(connection,packet));CHECK((core.pool.Slot(prediction)[11]&3)!=0 && ConstructionLive(core,134)==builders);CHECK(session.Sp(1)==5000-core.types[75].cost);const auto notice=session.Construction().LastPacket();CHECK(notice.size()==29);if (notice.size()!=29) return;const Sid target{static_cast<std::uint16_t>(notice[27]|(std::uint16_t(notice[28])<<8))};FinishGeneralConstruction(session,builders.front(),target);
}
// 장치 누락은 도달한 dirty 경계에서 실패하고 이후 장치 교체가 같은 커서 몸체에 즉시 적용된다.
TEST_CASE(CampaignSession_CursorRequiresActualEffectsAndSupportsRebinding) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& cursor=world.Session().Cursor();bool failed=false;try { cursor.Cancel(); }catch (const std::logic_error& error) { failed=std::string(error.what()).find("화면 dirty")!=std::string::npos; }CHECK(failed);
    SessionCursorUi first,second;cursor.SetEffects(first.Effects());cursor.Cancel();CHECK(first.dirty>0);const auto count=first.dirty;cursor.SetEffects(second.Effects());cursor.Cancel();CHECK(first.dirty==count && second.dirty>0 && world.Session().CursorDropSettings().lockDepth==1);
}

// 저장된 준비 조각의 반환8 플래그가 실제 contained 생성과 같은 Kernel Regular로 이어진다.
TEST_CASE(CampaignSession_CursorReturnsFactoryPieceWithActualTimer) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyCursorFactoryInput(fort);
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();auto core=session.Core();SessionCursorUi ui;auto& cursor=session.Cursor();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;session.SetSp(1,5000);const auto source=CursorEnergyInput(session);CHECK(core.pool.Slot(source)[10]==76);const auto form=ReadyCursorFactoryForm(session);const auto parent=SquidForms(core.pool).Parent(form);
    cursor.ReturnSettings().lifetimeIndex=1;cursor.Take(form,0x41);const auto point=CursorPlacementPoint(session,ui);CHECK(session.CursorDropSettings().x==point.x);cursor.Drop();CHECK(ui.sent.size()==1 && ui.shown==1 && session.Sp(1)==5000-core.types[75].cost);const auto returned=session.Priests().Carrier().FindContained(parent,8,75,0);CHECK(returned.value);if (!returned.value) return;
    CHECK(core.pool.Slot(returned)[10]==6 && (Get(core.pool.Slot(returned),20,2)&0x108)==0x108);const auto timer=FindAttachedProcess(core.pool,returned,46);CHECK(timer.value);if (!timer.value) return;const auto* process=dynamic_cast<RegularProcess*>(core.processes.FromForm(timer));CHECK(process && process->Event()==0 && process->Payload()==5.0f && process->Time()==core.processState.now && core.pool.Slot(returned)[30]==2);CHECK(!session.CursorDropSettings().preview.value && cursor.Lifecycle().previewCount==0);
}

// 실제 커서 수집의 깊이/tail과 현재8비트 SHP/윤곽/파일 색표·카메라를 같은 표시기에서 검사한다.
TEST_CASE(CampaignSession_CursorDisplayUsesActualShapesCameraAndDirtyCollection) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyCursorFactoryInput(fort);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();auto core=session.Core();SessionCursorUi ui;auto& cursor=session.Cursor();auto& display=cursor.Display();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;session.SetSp(1,5000);CursorEnergyInput(session);cursor.Take(ReadyCursorFactoryForm(session),0x41);const auto point=CursorPlacementPoint(session,ui);
    const int cameraX=point.x*16-512,cameraY=point.y*11-384;session.SetParticleView({cameraX,cameraY,0,0,1024,768},{});ui.Point(session,point,cameraX,cameraY);CHECK(cursor.Placement().screenOrigin.x==cameraX && cursor.Placement().screenOrigin.y==cameraY);FontStore fonts(assets.files,"Arial");std::vector<std::string> translated;display.SetText(fonts,[&](std::string_view key) { translated.emplace_back(key);return std::string(key); });
    const ScreenRect clip{0,0,1024,768};auto sprites=display.Sprites(clip,0);CHECK(sprites.size()==2);if (sprites.size()!=2) return;CHECK(sprites[0].order.depth==core.types[75].zOrder && sprites[1].order.depth==core.types[75].zOrder-1);CHECK(sprites[0].order.x==static_cast<float>(point.x));CHECK(display.Sprites(clip,4).empty());
    const auto raw=core.pool.Slot(cursor.Lifecycle().previews[0]);const auto frame=Get(raw,36);const auto& type=core.types[75];const auto& asset=assets.game.Types()[75-kFirstAssetTypeNumber];const auto& picture=assets.game.Shapes().Blocks()[asset.block].frames[frame];const auto image=assets.game.Shapes().Decode(asset.block,frame);const auto x=static_cast<std::int16_t>(Get(raw,22,2))-cameraX,y=static_cast<std::int16_t>(Get(raw,24,2))-cameraY;const auto left=x-type.footX*16,top=y-type.footY*11;
    std::vector<std::uint8_t> actual(1024*768,199);IndexedImage expected{1024,768,actual,std::vector<std::uint8_t>(actual.size(),255)};const auto color=display.Settings().goodColor;
    const std::array<GumpLine,4> outline{{{{left,top},{x-1,top},color},{{left,y-1},{x-1,y-1},color},{{left,top},{left,y-1},color},{{x-1,top},{x-1,y-1},color}}};DrawGumpLines(expected,outline);DrawIndexedImage(expected.indices,1024,768,image,x-type.hotspotX+picture.rect.left,y-type.hotspotY+picture.rect.top,clip);sprites.front().procedure->draw(actual,1024,768,clip);CHECK(actual==expected.indices);CHECK(std::count_if(actual.begin(),actual.end(),[](std::uint8_t value) { return value!=199; })>100);
    session.SetSp(1,8001);session.RunFrame({12.5,0,0,1},true);display.Feedback().countdown=20;Renderer renderer(1024,768);std::size_t calls{};renderer.SetExtraCollection([&](ScreenRect cut,std::uint32_t flags) { ++calls;return display.Sprites(cut,flags); });renderer.SetScene({});renderer.Draw(actual,1024);renderer.Present([](ScreenRect) {});CHECK(calls==1 && display.Feedback().countdown==19 && display.Feedback().highlight==std::bit_cast<std::uint32_t>(12.5f));const auto countdown=display.Feedback().countdown;const auto snapshot=renderer.SceneImage();CHECK(display.Feedback().countdown==countdown);
    // 마지막 작은 dirty의 수집도 전체 스냅샷의 body/aux bounds를 잘라내지 않는다.
    renderer.Invalidate({0,0,2,2});renderer.Draw(actual,1024);renderer.Present([](ScreenRect) {});CHECK(calls==2 && display.Feedback().countdown==countdown-1);CHECK(renderer.SceneImage().indices==snapshot.indices);CHECK(cursor.Lifecycle().warningContext[2]==display.TextColors()[0] && cursor.Lifecycle().costContext[2]==display.TextColors()[1]);
    cursor.Cancel();renderer.InvalidateAll();renderer.Draw(actual,1024);renderer.Present([](ScreenRect) {});CHECK(calls==3 && display.Feedback().countdown==countdown-1);CHECK(display.Sprites(clip,0).empty());renderer.SetExtraCollection({});
}
// 불법 커서의 실제 빨강 변환과 원본 경고 키/글꼴 출력이 현재 dirty clip을 벗어나지 않는다.
TEST_CASE(CampaignSession_CursorDisplayIllegalAndActualWarningText) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyCursorFactoryInput(fort);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();SessionCursorUi ui;auto& cursor=session.Cursor();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;session.SetSp(1,5000);cursor.Take(ReadyCursorFactoryForm(session),0x41);
    session.SetParticleView({0,0,0,0,1024,768},{});ui.Point(session,{8,8},0,0,false);CHECK(!cursor.Placement().placementAllowed && !cursor.Placement().descriptionAllowed && !session.CursorDropSettings().legal);auto& display=cursor.Display();FontStore fonts(assets.files,"Arial");std::vector<std::string> translated;display.SetText(fonts,[&](std::string_view key) { translated.emplace_back(key);return std::string(key); });display.Settings().showLetters=1;const ScreenRect clip{30,20,400,250};auto sprites=display.Sprites(clip,0);CHECK(sprites.size()==2);std::vector<std::uint8_t> pixels(1024*768,199);
    // 원본 두 레이어를 동일 dirty 표면에 출력한다. 실제 native 키와 파일 글꼴을 소비한다.
    for (const auto& sprite:sprites) sprite.procedure->draw(pixels,1024,768,clip);
    CHECK(std::find(translated.begin(),translated.end(),"Energy not satisfied")!=translated.end());CHECK(std::count_if(pixels.begin(),pixels.end(),[](std::uint8_t value) { return value!=199; })>0);
    // 양 끝 제외 dirty 바깥의 픽셀은 글리프/격자/본체 어느 출력도 바꿔서는 안 된다.
    for (int y=0;y<768;++y) { /* 각 행의 clip 밖만 확인한다. */ for (int x=0;x<1024;++x) if (x<clip.left || x>=clip.right || y<clip.top || y>=clip.bottom) CHECK(pixels[static_cast<std::size_t>(y)*1024+x]==199); }
    cursor.Cancel();
}

// 초기 잠금1과 실제 목록 재생성/원본 ±2 입력/ESC가 같은 현재 커서에 연결된다.
TEST_CASE(CampaignSession_CursorActionsUseActualPreviewsAndInput) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyCursorFactoryInput(fort);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();SessionCursorUi ui;auto& cursor=session.Cursor();auto& drop=session.CursorDropSettings();CHECK(drop.lockDepth==1 && !cursor.HandleInput({InputCode::kLeftButton,0,0},0));cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;session.SetSp(1,5000);CursorEnergyInput(session);cursor.Take(ReadyCursorFactoryForm(session),0x41);const auto point=CursorPlacementPoint(session,ui);CHECK(drop.lockDepth==0 && drop.direction==0);
    const auto preview=cursor.Lifecycle().previews[0];cursor.Lock(0);CHECK(drop.lockDepth==1 && ui.cursors.back()==1);const auto count=ui.cursors.size();cursor.Lock(1);CHECK(drop.lockDepth==2 && ui.cursors.size()==count);cursor.Unlock();CHECK(drop.lockDepth==1 && ui.cursors.size()==count);cursor.Unlock();CHECK(drop.lockDepth==0 && cursor.Lifecycle().previews[0]==preview);
    CHECK(cursor.HandleInput({InputCode::kRightButton,ui.mouse.x,ui.mouse.y},0) && drop.direction==2);CHECK(cursor.HandleInput({0x250000|InputCode::kShift,0,0},0) && drop.direction==0);CHECK(cursor.HandleInput({InputCode::kRightButton,0,0},1) && drop.direction==6);CHECK(cursor.HandleInput({0x270000,0,0},0) && drop.direction==0);
    CHECK(!cursor.HandleInput({InputCode::kRightButton|InputCode::kRelease,0,0},0));CHECK(!cursor.HandleInput({InputCode::kRightButton|InputCode::kOutside,0,0},0));CHECK(drop.direction==0 && session.Sp(1)==5000 && ui.sent.empty());CHECK(drop.x==point.x && drop.y==point.y);
    const auto refreshed=ui.refreshes;CHECK(cursor.HandleInput({0x1b,0,0},0));CHECK(drop.lockDepth==1 && !drop.preview.value && cursor.Lifecycle().previewCount==0 && ui.refreshes==refreshed+1);CHECK(!cursor.HandleInput({0x270000,0,0},0));
}
// 실제 회전 조건의 Range 삭제/재생성은 같은51 Form/Kernel/표시 수명을 사용한다.
TEST_CASE(CampaignSession_CursorRotateUsesActualRangeLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();auto core=session.Core();SessionCursorUi ui;auto& cursor=session.Cursor();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;session.SetParticleView({0,0,0,0,1024,768},{});ui.mouse={1616,1221};cursor.Update(ui.mouse.x,ui.mouse.y);const auto priest=Sid{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};
    // 각 실제 원본 TYPE를 가진 Form은 명시 입력이며 메뉴 제공 행/생산 완료를 대신하지 않는다.
    for (const std::uint32_t type:{128U,140U}) {
        const auto form=cursor.Contents().Create(priest,type,0,0x20,0x10,0);cursor.Take(form,0x160);auto& drop=session.CursorDropSettings();const auto token=drop.cursorGump;auto* old=dynamic_cast<CursorRangeProcess*>(core.kernel.Get(token));CHECK(old && token);if (!old) continue;const auto oldForm=old->Form();const auto parent=old->Parent();CHECK(parent==cursor.Lifecycle().previews[0]);const auto baseline=core.kernel.Size();const auto rotations=core.processState.typeRotation;cursor.Rotate(2);CHECK(drop.direction==2 && drop.lockDepth==0);
        auto* current=dynamic_cast<CursorRangeProcess*>(core.kernel.Get(drop.cursorGump));CHECK(current && current->Parent()==cursor.Lifecycle().previews[0]);CHECK(core.kernel.Size()==baseline);
        if (core.types[type].group==1 || type==session.CursorRangeSettings().targeting.windArcherType) { CHECK(core.processState.typeRotation==rotations+1);CHECK(current && FindAttachedProcess(core.pool,cursor.Lifecycle().previews[0],51)==current->Form()); }
        else CHECK(drop.cursorGump==token && current && current->Form()==oldForm && core.processState.typeRotation==rotations);
        cursor.Cancel();CHECK(!session.CursorDropSettings().preview.value && !session.CursorDropSettings().cursorGump);
    }
}

// Run/창/장치를 열지 않고 실제 UserInput 진입점과 현재 설정/좌표·명령을 검증한다.
TEST_CASE(CampaignSession_CursorUserInputReadsCurrentSettingsAndMouse) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());ReadyCursorFactoryInput(fort);GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});auto& session=world.Session();SessionCursorUi ui;auto& cursor=session.Cursor();auto& drop=session.CursorDropSettings();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;session.SetSp(1,5000);CursorEnergyInput(session);cursor.Take(ReadyCursorFactoryForm(session),0x41);const auto point=CursorPlacementPoint(session,ui);
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";Client client(std::move(options));UserInput input(client,world);auto& config=client.Configuration();config.Set("cursorSnapToGrid","1");config.Set("cursorShowCannotPlace","1");
    ui.mouse.x+=16;input.UpdateCursor({InputCode::kLeftButton,ui.mouse.x,ui.mouse.y});CHECK(drop.x==point.x+1 && drop.y==point.y && cursor.SnapToGrid()==1 && cursor.Placement().placementFlags==1);
    config.Set("cursorSnapToGrid","0");config.Set("cursorShowCannotPlace","0");ui.mouse.x+=4;input.UpdateCursor({InputCode::kLeftButton,ui.mouse.x,ui.mouse.y});CHECK(drop.x==static_cast<float>(point.x)+1.25f && cursor.SnapToGrid()==0 && cursor.Placement().placementFlags==0);const auto x=drop.x;
    input.UpdateCursor({InputCode::kOutside,900,700});CHECK(drop.x==x);input.Event({InputCode::kRightButton,ui.mouse.x,ui.mouse.y});CHECK(drop.direction==2);config.Set("rotmode","1");input.Event({InputCode::kRightButton,ui.mouse.x,ui.mouse.y});CHECK(drop.direction==0);
    input.Event({0x270000,0,0});CHECK(drop.direction==2);input.Event({0x250000|InputCode::kShift,0,0});CHECK(drop.direction==0);input.Event({InputCode::kRightButton|InputCode::kRelease,0,0});CHECK(drop.direction==0 && ui.sent.empty() && session.Sp(1)==5000);
    input.Event({0x1b,0,0});CHECK(drop.lockDepth==1 && !drop.preview.value);input.UpdateCursor({0,1000,700});CHECK(drop.x==x);
}

// 내려놓기의+1c(3,0,0)는 같은 실제 Range 프로세스에 도달하며 UI 닫기 콜백으로 대체하지 않는다.
TEST_CASE(CampaignSession_CursorDropNotifiesActualRangeProcess) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});
    auto& session=world.Session();auto& cursor=session.Cursor();const auto core=session.Core();SessionCursorUi ui;
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};
    auto effects=ui.Effects();std::size_t received{},wrongClose{};std::uint32_t rangeToken{};bool closedObserved=false; // GUI 수신만 명시 입력이며 Range의 실제 몸체는 대체하지 않는다.
    effects.placement.invalidate=[&](ScreenRect,std::uint32_t) {
        ++ui.dirty;auto* actual=dynamic_cast<CursorRangeProcess*>(core.kernel.Get(rangeToken));
        if (actual && Get(actual->Fields().bytes,0x1b8)==1) closedObserved=true; // 마지막 실제 Prepare는 부모 삭제 전 Notify의 종료 필드를 읽는다.
    };
    effects.placement.gumpAt=[](std::int32_t,std::int32_t) { return 77U; };
    effects.placement.accept=[](std::uint32_t token,std::uint32_t event,std::uint32_t,std::uint32_t,std::uint32_t) { CHECK(event==0x0200003f);return token; };
    effects.placement.parent=effects.drop.guiParent=[priest](std::uint32_t token) { CHECK(token==77);return priest.value; };
    effects.drop.guiFlags=[](std::uint32_t token) { CHECK(token==77);return 0U; };
    effects.drop.guiReceive=[&](std::uint32_t token,Sid form) { CHECK(token==77 && form==session.CursorDropSettings().preview);++received; };
    effects.drop.closeGump=[&](auto,auto,auto,auto) { ++wrongClose; };
    cursor.SetEffects(std::move(effects));
    const auto form=cursor.Contents().Create(priest,128,0,0x20,0x10,0); // 명시 Form 입력이며 실제 생산 완료/메뉴 제공 행의 검증은 아니다.
    cursor.Take(form,0x100);auto& drop=session.CursorDropSettings();
    const auto token=drop.cursorGump;rangeToken=token;auto* range=dynamic_cast<CursorRangeProcess*>(core.kernel.Get(token));
    CHECK(range && token && drop.legalGump==77 && drop.legal);if (!range) return;
    CHECK(Get(range->Fields().bytes,0x1b8)==0);cursor.Drop();
    CHECK(received==1 && wrongClose==0 && drop.lockDepth==1 && !drop.preview.value && !drop.cursorGump);
    CHECK(closedObserved && core.kernel.Get(token)==nullptr); // 실제 종료 통지 뒤 부모 미리보기 삭제가 종속 Range를 반납한다.
    CHECK(SquidForms(core.pool).Parent(form)==priest); // GUI 수신은 실제 같은 ContentForm 부착을 사용한다.
}

// 강조 해제가 실제 월드의 현재 선택을 지우지 않고 폴링 좌표/마우스TYPE만 별도 상태로 공유한다.
TEST_CASE(CampaignSession_CursorHighlightSharesSelectionWithoutClearingIt) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});
    const auto boundCursors=BindWorldTestInput(world);const auto priest=world.Session().FindPriestParticipation(1);const auto* object=world.Object(priest);CHECK(object);world.SetMousePoint(world.ToScreen(object->x,object->y));world.Select(priest);auto state=world.HighlightSettings();CHECK(!boundCursors->empty());
    CHECK(state.selectedSid==priest && priest!=0);state.mode=5;state.mouseType=128;
    world.SetMousePoint({-33,91});CHECK(state.mouseX==-33 && state.mouseY==91);
    UserHighlightHooks hooks;std::vector<std::uint32_t> cursors;hooks.selectCursor=[&](std::uint32_t cursor) { cursors.push_back(cursor); };
    RawUserHighlight highlight(state,std::move(hooks));highlight.Highlight(0);
    CHECK(state.mode==0 && state.mouseType==0 && state.selectedSid==priest && cursors==std::vector<std::uint32_t>{1});
    highlight.Highlight(0);CHECK(cursors.size()==1);world.Select(0);CHECK(state.selectedSid==0);
}

// 마우스가 움직이지 않아도 UI 교체 뒤 수집은 새 토큰의 현재 깊이를 읽어야 한다.
TEST_CASE(CampaignSession_CursorRequeriesReplacedGumpsAtStationaryMouse) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",external.Services(),{1024,768});
    auto& session=world.Session();auto& cursor=session.Cursor();SessionCursorUi ui;RawButtonControls controls(OriginalEdition::Patch1078);
    const std::array<GumpControl,1> buttons{{{0,{-10000,-10000,10000,10000},false,true,false}}};
    const std::array<std::string,1> labels{{"Current"}};const RawButtonAssets background{90,0,20,16,{7,255,13}};
    controls.SetControls(buttons,labels,1024,768,background);
    auto effects=ui.Effects();BindCampaignCursorGumps(effects,controls);cursor.SetEffects(std::move(effects));
    cursor.Display().SetExternal([&](std::uint32_t token) { return controls.GumpDepth(token); },{});
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};
    const auto form=cursor.Contents().Create(priest,128,0,0x20,0x10,0);cursor.Take(form,0x100);
    const auto old=cursor.Lifecycle().currentGump;const auto x=session.CursorDropSettings().x,y=session.CursorDropSettings().y;
    CHECK(old && old==controls.PickGump({session.CursorDropSettings().screenX,session.CursorDropSettings().screenY}));
    controls.SetControls(buttons,labels,1024,768,background);const auto current=controls.PickGump({session.CursorDropSettings().screenX,session.CursorDropSettings().screenY});
    CHECK(current && current!=old);cursor.Update(ui.mouse.x,ui.mouse.y); // 원본446b50의 위치 불변 경로는 Query하지 않는다.
    CHECK(cursor.Lifecycle().currentGump==old);cursor.GumpsChanged();
    CHECK(cursor.Lifecycle().currentGump==current && session.CursorDropSettings().x==x && session.CursorDropSettings().y==y);
    cursor.Display().Sprites({0,0,1024,768},2); // 폐기 토큰 조회라면 여기서 out_of_range로 실패한다.
    controls.SetControls({},labels,1024,768,background);cursor.GumpsChanged();CHECK(cursor.Lifecycle().currentGump==0);
    cursor.Cancel();cursor.GumpsChanged();CHECK(session.CursorDropSettings().lockDepth==1);
}

// 실제5b 등록/분배가 현재 지면을 차감하고 고갈한 자산의 실제 삭제·공간·목록까지 갱신한다.
TEST_CASE(CampaignSession_CarrierMiningRegisteredReceiverExhaustsGeyser) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());
    std::vector<std::vector<std::uint8_t>> received;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(received));CHECK(Throws([&] { session.RegisterCarrierCommands(dispatch); }));session.Load();session.RegisterCarrierCommands(dispatch);const auto core=session.Core();
    const auto& registration=dispatch.Registration(0x5b);CHECK(registration.length==6 && registration.version==1 && registration.mismatch==0 && !registration.observer);
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto position=core.pool.Slot(priest);const auto golem=session.World().Create(133,2);session.World().Pop(golem,std::bit_cast<float>(Get(position,14)),std::bit_cast<float>(Get(position,18)),0);core.owner.Set(golem,1);
    const auto geyser=session.CarrierChoice().ChooseGeyser(golem);CHECK(geyser.value);if (!geyser.value) return;Put(core.pool.AllocatedBytes(geyser),12,200,2);CHECK(core.types[154].cost==200);
    const std::array<std::uint8_t,6> packet{1,0x5b,6,0,static_cast<std::uint8_t>(geyser.value),static_cast<std::uint8_t>(geyser.value>>8)};PacketConnection connection;connection.sender=7;
    auto geyserRaw=core.pool.AllocatedBytes(geyser);const auto oldState=geyserRaw[11];geyserRaw[11]|=1;const auto notices=external.notices;CHECK(dispatch.Dispatch(connection,packet));CHECK(Get(core.pool.Slot(geyser),12,2)==200 && external.notices==notices);geyserRaw[11]=oldState; // free 상태의 무효과를 확인한 뒤 원래 살아 있는 상태로 복구한다.
    CHECK(dispatch.Dispatch(connection,packet));CHECK(received.size()==2 && received[0]==std::vector<std::uint8_t>(packet.begin(),packet.end()) && connection.depth==0);
    CHECK((core.pool.Slot(geyser)[11]&3)!=0 || core.pool.Slot(geyser)[10]!=122);CHECK(session.CarrierChoice().ChooseGeyser(golem)!=geyser);CHECK(external.notices>notices);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
}

// 실제1-1의 내용물을 크랩으로 옮기고 원래 기증자의 실제40 경로와 송신 전 효과를 검사한다.
TEST_CASE(CampaignSession_CarrierStealUsesActualContentMovementAndReceiver) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();
    std::vector<std::array<std::uint32_t,6>> sounds; // 실제 장치 경계의 원본6인자다.
    services.sound.play=[&](SoundHandle sound,std::uint32_t loop,std::uint32_t volume,std::uint32_t pan,std::uint32_t priority,std::uint32_t limit) { sounds.push_back({sound,loop,volume,pan,priority,limit});return sound; };
    RawCampaignSession* current{};Sid crab{},donor{},content{};std::vector<std::vector<std::uint8_t>> sent;bool attachedAtSend{},soundAtSend{}; // 송신 시점의 같은 실제 체인/소리 상태다.
    services.battlePeers[2]=[&](std::span<const std::uint8_t> packet) { if (packet.size()==10 && packet[1]==0x60) { sent.emplace_back(packet.begin(),packet.end());attachedAtSend=SquidForms(current->Core().pool).Parent(content)==crab;soundAtSend=!sounds.empty() && sounds.back()[0]==external.names.Lookup("crabSteal.wav"); } };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));current=&session;CHECK(Throws([&] { static_cast<void>(session.CarrierSteal()); }));session.Load();auto core=session.Core();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto raw=core.pool.Slot(priest);const RawPathPoint point{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))};
    // 세번째owner에는 현재 신전/귀환 대상이 없다.42 경로 미복원을 우회하는 대체 경로를 공급하지 않는다.
    crab=session.World().Create(156,2);donor=session.World().Create(133,2);
    // 같은 실제 지면/해시에서 두 운반체를 생성·소유한다.
    for (const auto sid:{crab,donor}) { session.World().Pop(sid,point.x,point.y,0);core.owner.Set(sid,sid==crab ? 3U : 2U); }
    const auto* timer=core.processes.FindEvent(crab,0x492);CHECK(timer && timer->Payload()==1.0F && timer->Time()==core.processState.now);if (timer) CHECK(core.pool.Slot(timer->Form())[10]==46);
    CHECK(session.StartCarrierMovement({donor,point,1,0,0,0,0}));RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("포획 검사 조회에서 삭제 금지"); });const auto oldPath=lookup.Find(donor,40);CHECK(oldPath.value);
    content=session.CreateContained(donor,154,0,0x20,0,0);CHECK(SquidForms(core.pool).Parent(content)==donor);const auto before=session.Sp(3);
    // 실제 역방향만 적대다. 정방향의 동맹값은 포획을 금지하지 않는다.
    session.Alliances()[3*9+2]=1;session.Alliances()[2*9+3]=0;auto players=session.PlayerRecords();players[2*0xb4+0x50]|=1;
    auto* moving=dynamic_cast<RawPathProcess*>(core.processes.FromForm(oldPath));CHECK(moving && !moving->Runtime().Finished());if (!moving) return;
    moving->RunFrame(); // 실제40의Halt가포획을실행하며같은40이삭제된다.복귀뒤포인터를읽지않는다.
    CHECK(attachedAtSend && soundAtSend && sent.size()==1);CHECK(SquidForms(core.pool).Parent(content)==crab);CHECK(!lookup.Find(donor,40).value && !core.processes.FromForm(oldPath));CHECK(session.Priests().Carrier().FindContained(crab,0x20,0,0)==content);CHECK(!session.Priests().Carrier().FindContained(donor,0x20,0,0).value);CHECK(session.Sp(3)==before);
    CHECK(!sounds.empty() && (sounds.back()==std::array<std::uint32_t,6>{external.names.Lookup("crabSteal.wav"),0,0,0,1,0}));const auto* preview=core.processes.FindEvent(crab,0x491);CHECK(preview && preview->Event()==0x491);
    CHECK(session.CarrierSteal().Try(crab)==10 && sent.size()==1);CHECK(&session.CarrierSteal().Pool()==&core.pool);
    // 원본60h 수신은 기증자WORD를 무시하고 현재 두free 비트만 관문으로 사용한다.
    std::vector<std::vector<std::uint8_t>> recorded;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(recorded));session.RegisterCarrierCommands(dispatch);const auto& registration=dispatch.Registration(0x60);CHECK(registration.length==10 && registration.version==1 && registration.mismatch==0 && !registration.observer);
    std::array<std::uint8_t,10> packet{1,0x60,10,0};Put(packet,4,donor.value,2);Put(packet,6,content.value,2);Put(packet,8,0xffff,2);PacketConnection connection;connection.sender=8;auto bytes=core.pool.AllocatedBytes(content);const auto state=bytes[11];bytes[11]|=1;const auto count=sounds.size();CHECK(dispatch.Dispatch(connection,packet));CHECK(Get(core.pool.Slot(content),16,2)==crab.value && sounds.size()==count);bytes[11]=state;
    CHECK(dispatch.Dispatch(connection,packet));CHECK(SquidForms(core.pool).Parent(content)==donor && sounds.size()==count+1 && recorded.size()==2 && connection.depth==0);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
}

namespace {
// 명시적으로이미연결된단독미션의두실제끝을준비한다.로그인성공을재현한것으로계산하지않는다.
struct SessionPickupWire {
    std::vector<std::vector<std::uint8_t>> recorded; // 실제등록분배기의기록전체바이트다.
    netstorm::zacket::LocalMissionTransport transport; // 실제buffer/집합/서버분배큐다.
    // Session/현재Player기록은이연결보다오래살아야한다.
    explicit SessionPickupWire(RawCampaignSession& session):transport(session.PlayerRecords(),session.Core().processState.server,CommandDevice(recorded)) {
        transport.RegisterLogin([](std::uint32_t,PacketConnection&,std::span<const std::uint8_t>) { throw std::logic_error("포획검사에서로그인요청선택금지"); });
        transport.Open(6799);transport.AssignConnection(-1,1);session.PlayerRecords()[0xb4+0x50]|=1;session.RegisterCarrierCommands(transport.Dispatch());
        const auto& registration=transport.Dispatch().Registration(0x4f);CHECK(registration.length==9 && registration.version==1 && registration.mismatch==0 && !registration.observer);
    }
};
// 원본지도의실제한지점경로를시작하고끝시각의프레임을실행한다.도착결과는주입하지않는다.
void ArriveForPickup(RawCampaignSession& session,Sid parent,RawPathPoint point) {
    const auto core=session.Core();CHECK(session.StartCarrierMovement({parent,point,1,0,0,0,0}));RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("포획이동검사조회에서삭제금지"); });const auto form=lookup.Find(parent,40);CHECK(form.value);if (!form.value) return;auto* path=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(path && path->Runtime().Count()>0);if (!path || !path->Runtime().Count()) return;
    core.processState.now=path->Runtime().Time(path->Runtime().Count()-1)+0.01;path->RunFrame(); // 도착이경로를삭제할수있어복귀뒤포인터를읽지않는다.
}
}
// 실제도착은요청만버퍼에넣고Flush/Poll 뒤서버가너겟을같은내용물로변환한다.
TEST_CASE(CampaignSession_CarrierPickupNuggetThroughActualLocalTransport) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;InputQueue input;netstorm::zacket::LocalMissionTransport* wire{};auto services=external.Services();
    services.assets.queueCommand=[&](std::uint32_t command,std::int32_t first,std::int32_t second) { input.PushCommand(command,first,second); };
    services.carrierRequest=[&](std::int32_t owner,std::span<const std::uint8_t> packet) { CHECK(wire && owner==9);if (!wire) throw std::logic_error("포획실제로컬연결미설치");CHECK(wire->Send(owner,packet)); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.RequestCarrierPickup(Sid{50}); }));session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;
    const RawPathPoint point{98,122};const auto golem=session.World().Create(133,2),nugget=session.World().Create(154,2);session.World().Pop(golem,point.x,point.y,0);core.owner.Set(golem,1);session.World().Pop(nugget,point.x,point.y,0);const auto argument=Get(core.pool.Slot(nugget),12,2);const auto sp=session.Sp(1);
    // 로딩및원본부모가먼저남긴사건은소비하고이번도착의새명령만확인한다.
    while (!input.Empty()) static_cast<void>(input.Pop(false));
    ArriveForPickup(session,golem,point);CHECK(link.recorded.empty() && link.transport.QueuedFrames()==0);CHECK((core.pool.Slot(nugget)[11]&3)==0 && !session.Priests().Carrier().FindContained(golem,0x20,154,0).value);CHECK(input.Empty());
    link.transport.Flush();CHECK(link.transport.QueuedFrames()==1);CHECK(link.transport.Poll()==1 && link.transport.QueuedFrames()==0);const auto content=session.Priests().Carrier().FindContained(golem,0x20,154,0);CHECK(content.value);if (!content.value) return;
    CHECK(core.pool.Slot(content)[10]==6 && Get(core.pool.Slot(content),12,2)==argument && SquidForms(core.pool).Parent(content)==golem);CHECK((core.pool.Slot(nugget)[11]&1)!=0);CHECK(session.Sp(1)==sp);CHECK(link.recorded.size()==1 && link.recorded[0].size()==9 && Get(link.recorded[0],4,2)==golem.value && Get(link.recorded[0],6,2)==nugget.value && link.recorded[0][8]==154);
    const auto event=input.Pop(false);CHECK(event.code==0x030000b1 && event.x==nugget.value && event.y==golem.value && input.Empty());CHECK(link.transport.Server().depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);wire=nullptr;
}
// 실제저HP 사제와기술내용물을포획Form으로옮긴뒤원사제삭제/참여목록을확인한다.
TEST_CASE(CampaignSession_CarrierPickupPriestMovesNestedContents) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;InputQueue input;netstorm::zacket::LocalMissionTransport* wire{};auto services=external.Services();
    std::vector<std::array<std::uint32_t,6>> pickupSounds; // 실제사제포획이읽은현재TYPE 음향의6인자다.
    services.sound.play=[&](SoundHandle sound,std::uint32_t loop,std::uint32_t volume,std::uint32_t pan,std::uint32_t priority,std::uint32_t limit) { pickupSounds.push_back({sound,loop,volume,pan,priority,limit});return sound; };
    services.assets.queueCommand=[&](std::uint32_t command,std::int32_t first,std::int32_t second) { input.PushCommand(command,first,second); };
    services.carrierRequest=[&](std::int32_t owner,std::span<const std::uint8_t> packet) { if (!wire) throw std::logic_error("사제포획실제로컬연결미설치");CHECK(owner==9 && wire->Send(owner,packet)); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;
    const RawPathPoint point{98,122};const auto golem=session.World().Create(133,2),priest=session.World().Create(158,2);session.World().Pop(golem,point.x,point.y,0);core.owner.Set(golem,1);// 새 사제의 원래 소유자는 명시적인 전투 전 초기 입력으로 전용 가상 SetOwner에 전달하고 현재 전투 모드를 복구한다.
    const bool battle=core.ownerMode.battle;core.ownerMode.battle=false;session.Priests().SetOwner(priest,3);core.ownerMode.battle=battle;session.World().Pop(priest,point.x,point.y,0);CHECK((Get(core.pool.Slot(priest),12,2)&0x7f)==3);session.Priests().SetHitPoints(priest,session.HitPoints().MaxHitPoints(priest)/2-1);CHECK(session.Priests().PriestState().GroundImmobile(priest));
    const auto child=session.CreateContained(priest,75,0,4,0,0);const auto argument=Get(core.pool.Slot(priest),12,2);CHECK(SquidForms(core.pool).Parent(child)==priest && session.FindPriestParticipation(3)==priest.value);
    // 실제사제HP전환의표시사건을소비하고새포획통지를구별한다.
    while (!input.Empty()) static_cast<void>(input.Pop(false));
    ArriveForPickup(session,golem,point);CHECK(link.recorded.empty() && (core.pool.Slot(priest)[11]&3)==0);link.transport.Flush();CHECK(link.transport.Poll()==1);const auto captured=session.Priests().Carrier().FindContained(golem,0x20,158,0);CHECK(captured.value);if (!captured.value) return;
    CHECK(SquidForms(core.pool).Parent(captured)==golem && Get(core.pool.Slot(captured),12,2)==argument);CHECK(SquidForms(core.pool).Parent(child)==captured);CHECK(session.Priests().Carrier().FindContained(captured,4,75,0)==child);CHECK((core.pool.Slot(priest)[11]&1)!=0 && session.FindPriestParticipation(3)==golem.value);
    const auto livePriests=session.Priests().Priests().priests.Items();CHECK(std::find(livePriests.begin(),livePriests.end(),priest.value)==livePriests.end()); // 실제4919b0이같은사제등록목록에서도원객체를제거했다.CHECK(link.recorded.size()==1 && link.recorded[0][8]==158);
    const std::array<std::uint32_t,6> expectedSound{external.names.Lookup("golemPickup.wav"),0,0,0,1,0};CHECK(std::find(pickupSounds.begin(),pickupSounds.end(),expectedSound)!=pickupSounds.end());
    const auto event=input.Pop(false);CHECK(event.code==0x030000b1 && event.x==priest.value && event.y==golem.value);CHECK(link.transport.Server().depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);wire=nullptr;
}

// 실제원본TYPE 보상을새오벨리스크QA/비어있는QA/최초Pop추첨에서선택하고같은월드에서변환한다.
TEST_CASE(CampaignSession_GemRewardActualSelectionPopAndDestroy) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    // 서로새로운세션에서저장QA/변환시추첨/최초Pop추첨의실제세경로를분리한다.
    for (const std::uint32_t mode:{0U,1U,2U}) {
        SessionServicesFixture external;auto services=external.Services();std::size_t fills{},wallReads{}; // 실제스택입력과벽시각장치요청수다.
        services.production.allocationBytes=[&](std::span<std::uint8_t> raw) { CHECK(raw.size()==kProductionDeckBytes);++fills;std::fill(raw.begin(),raw.end(),std::uint8_t{0xa7}); };
        services.sound.wallSeconds=[&] { ++wallReads;return 12.5; };
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { static_cast<void>(session.GemReward()); }));session.Load();auto core=session.Core();std::uint32_t savedType{}; // 실제TYPE 표에서읽는명시저장QA입력이다.
        // 원본레벨0~3/genus100 후보를표순서대로선택하되결과객체/추첨결과는주입하지않는다.
        for (std::uint32_t type=70;type<core.types.size();++type) if ((core.types[type].flags2&0x100U) && core.types[type].level>=0 && core.types[type].level<=3) { savedType=type;break; }
        CHECK(savedType!=0);if (!savedType) continue;
        const auto buried=session.World().Create(112,2);if (mode==0) Put(core.pool.AllocatedBytes(buried),12,savedType,2);else Put(core.pool.AllocatedBytes(buried),12,0,2);const auto beforeFill=fills,beforeWall=wallReads;
        session.World().Pop(buried,98,122,mode==2 ? 1U : 0U);
        // 실제최초Pop은FirstPopFlags의1을추가해먼저추첨한다. 그뒤저장QA/빈QA라는변환시점초기입력만준비한다.
        if (mode!=2) Put(core.pool.AllocatedBytes(buried),12,mode==0 ? savedType : 0U,2);const auto qa=Get(core.pool.Slot(buried),12,2);
        if (mode==2) { CHECK(qa && (core.types[qa].flags2&0x100U));CHECK(fills==beforeFill+1 && wallReads==beforeWall+1);CHECK(core.processes.FindEvent(buried,0) && core.processes.FindEvent(buried,1)); }
        const auto conversionFill=fills,conversionWall=wallReads;const auto reward=session.GemReward().Reveal(buried);const auto type=core.pool.Slot(reward)[10];
        CHECK(type<core.types.size() && (core.types[type].flags2&0x100U));CHECK((core.pool.Slot(reward)[11]&7U)==0 && (core.pool.Slot(buried)[11]&1U));CHECK(session.World().Find(98,122,type)==reward);CHECK(&session.GemReward().Pool()==&core.pool);
        if (mode!=1) { CHECK(type==qa && fills==conversionFill && wallReads==conversionWall); }else { CHECK(fills==conversionFill+1 && wallReads==conversionWall+1); }
        CHECK(!core.processes.FindEvent(buried,0) && !core.processes.FindEvent(buried,1));CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
        std::cout<<"실제오벨리스크변환 mode"<<mode<<" TYPE"<<std::uint32_t(type)<<' '<<core.types[type].name<<'\n';
    }
}

// 실제도착/4f 로컬전송이보상내용물과원본번개·Sound47까지생성하고같은Kernel에서진행한다.
TEST_CASE(CampaignSession_LightningScreenPickupThroughActualLocalTransport) {
    // 서로새로운두실제세션에서저장QA와현재추첨의4f 끝경로를함께검사한다.
    for (const std::uint32_t savedQA:{113U,0U}) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();InputQueue input;netstorm::zacket::LocalMissionTransport* wire{};RawCampaignSession* current{};Sid buried{},golem{},reward{};std::uint32_t kind{};bool revealedAtQueue{}; // 실제연결과입력통지시점의동일객체다.
    double wall=100.123456789;std::vector<std::array<std::uint32_t,6>> globalSounds;std::vector<SoundHandle> atSounds; // 장치입력시각과실제재생인자만기록한다.
    services.production.allocationBytes=[](std::span<std::uint8_t> raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0xa7}); };services.sound.wallSeconds=[&] { return wall; };
    services.sound.play=[&](SoundHandle sound,std::uint32_t loop,std::uint32_t volume,std::uint32_t pan,std::uint32_t priority,std::uint32_t limit) { globalSounds.push_back({sound,loop,volume,pan,priority,limit});external.playing.insert(sound);return sound; };
    services.sound.playOnceAt=[&](float,float,SoundHandle sound,std::uint32_t loop,std::uint32_t priority) { CHECK(loop==0 && priority==0);atSounds.push_back(sound);return sound; };
    services.carrierRequest=[&](std::int32_t target,std::span<const std::uint8_t> packet) { CHECK(wire && target==9);if (!wire) throw std::logic_error("획득번개 실제로컬연결 미설치");CHECK(wire->Send(target,packet)); };
    services.assets.queueCommand=[&](std::uint32_t code,std::int32_t first,std::int32_t second) { input.PushCommand(code,first,second);if (code==0x030000b1) { reward=Sid{static_cast<std::uint16_t>(first)};const auto core=current->Core();kind=core.pool.Slot(reward)[10];revealedAtQueue=(core.pool.Slot(buried)[11]&1) && (core.types[kind].flags2&0x100U) && second==golem.value; } };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));current=&session;session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;
    Renderer renderer(1024,768);session.SetParticleView({1400,1000,0,0,1024,768},{[&] { return renderer.FullRedrawPending(); },[&](SquidDisplayRect rect,std::uint32_t flags) { renderer.Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }});
    const RawPathPoint point{98,122};golem=session.World().Create(133,2);buried=session.World().Create(112,2);session.World().Pop(golem,point.x,point.y,0);core.owner.Set(golem,1);session.World().Pop(buried,point.x,point.y,0);Put(core.pool.AllocatedBytes(buried),12,savedQA,2); // 실제최초Pop뒤저장Point Blast 또는빈QA라는변환시점초기입력만준비한다.
    while (!input.Empty()) static_cast<void>(input.Pop(false));const auto sp=session.Sp(1);ArriveForPickup(session,golem,point);CHECK(link.recorded.empty() && input.Empty());link.transport.Flush();CHECK(link.transport.Poll()==1 && revealedAtQueue);
    const auto content=session.Priests().Carrier().FindContained(golem,1,kind,0);CHECK(content.value && SquidForms(core.pool).Parent(content)==golem);CHECK((core.pool.Slot(reward)[11]&1) && session.Sp(1)==sp && link.recorded.size()==1 && link.recorded[0][8]==112);
    const auto event=input.Pop(false);CHECK(event.code==0x030000b1 && event.x==reward.value && event.y==golem.value);
    const auto boltForm=FindAttachedProcess(core.pool,golem,19),soundForm=FindAttachedProcess(core.pool,golem,47);auto* bolt=dynamic_cast<LightningVisualProcess*>(core.processes.FromForm(boltForm));auto* sound=dynamic_cast<SoundProcess*>(core.processes.FromForm(soundForm));CHECK(bolt && sound);if (!bolt || !sound) return;
    const auto token=bolt->Fields().registration.token;CHECK(bolt->Fields().particles.values.size()==120 && bolt->Fields().registration.zOrder==-25 && Get(bolt->Fields().path.bytes,32)==std::bit_cast<std::uint32_t>(5.0F));CHECK(Get(bolt->Fields().path.bytes,4)==std::bit_cast<std::uint32_t>(1000.0F) && Get(bolt->Fields().path.bytes,0)==Get(bolt->Fields().path.bytes,8));
    const auto name=assets.game.Types()[kind-70].definition.String("activeSound");const auto pickup=name && !name->empty() ? external.names.Lookup(*name) : SoundHandle{}; // 실제현재TYPE+6c를같은이름표로확인한다.CHECK(sound->Sound()==pickup && sound->Alternate()==0 && sound->Flags()==12 && sound->StartTime()==wall+1.5);CHECK(std::find(atSounds.begin(),atSounds.end(),external.names.Lookup("thunderCrack.wav"))!=atSounds.end());
    const auto game=core.processState.now,beginWall=wall;bool rendered{}; // 생성결과를주입하지않고실제전체Kernel과현재Renderer를진행한다.
    for (std::uint64_t frame=1;frame<=85;++frame) {
        wall=beginWall+static_cast<double>(frame)*.025;session.RunFrame({game+static_cast<double>(frame)*.025,wall,.025,frame},false);
        if (frame<=20 && session.Particles().Rendering().Lookup(token)) { auto sprites=session.ParticleSprites({0,0,1024,768});std::erase_if(sprites,[](const auto& sprite) { return sprite.order.depth!=-25; });renderer.SetScene(std::move(sprites));const auto image=renderer.SceneImage();rendered=rendered || std::any_of(image.indices.begin(),image.indices.end(),[](auto color) { return color!=0; }); }
    }
    const std::array<std::uint32_t,6> expected{pickup,0,0,0,8,0};CHECK(rendered && std::count(globalSounds.begin(),globalSounds.end(),expected)==1);CHECK(std::find(atSounds.begin(),atSounds.end(),external.names.Lookup("shortZap.wav"))!=atSounds.end());CHECK(!core.processes.FromForm(soundForm) && !core.processes.FromForm(boltForm) && !session.Particles().Rendering().Lookup(token));CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0 && link.transport.Server().depth==0);wire=nullptr;
    std::cout<<"실제4f 획득 QA"<<savedQA<<" TYPE"<<kind<<" 픽셀/지연재생/자연효과삭제 통과\n";
    }
}

// 같은 골렘의 두 실제 보상 획득에서 기존 내용물 삭제→보충 관문→새 내용물 부착을 검증한다.
TEST_CASE(CampaignSession_RepeatedRewardPickupThroughActualLocalTransport) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();InputQueue input;netstorm::zacket::LocalMissionTransport* wire{}; //이미 연결된 단독 미션을 명시 입력한다.
    services.production.allocationBytes=[](std::span<std::uint8_t> raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0xa7}); };
    services.carrierRequest=[&](std::int32_t target,std::span<const std::uint8_t> packet) { CHECK(wire && target==9);if (!wire) throw std::logic_error("반복 획득의 실제 로컬 연결 미설치");CHECK(wire->Send(target,packet)); };
    services.assets.queueCommand=[&](std::uint32_t code,std::int32_t first,std::int32_t second) { input.PushCommand(code,first,second); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;
    const RawPathPoint point{98,122};const auto golem=session.World().Create(133,2);session.World().Pop(golem,point.x,point.y,0);core.owner.Set(golem,1);const auto sp=session.Sp(1);Sid previous{};std::size_t index{}; //같은 위치의 실제 경로 도착을 두 번 수행한다.
    // 결과 내용물은 주입하지 않고 저장 QA만 실제 오벨리스크에 입력한다.
    for (const std::uint32_t qa:{113U,119U}) {
        const auto buried=session.World().Create(112,2);session.World().Pop(buried,point.x,point.y,0);Put(core.pool.AllocatedBytes(buried),12,qa,2);
        // 이전 획득의 통지를 모두 소비한 뒤 새 실제 입력 명령만 검사한다.
        while (!input.Empty()) static_cast<void>(input.Pop(false));const auto before=link.recorded.size();ArriveForPickup(session,golem,point);CHECK(link.recorded.size()==before && input.Empty());link.transport.Flush();CHECK(link.transport.Poll()==1);
        const auto event=input.Pop(false);CHECK(event.code==0x030000b1 && event.y==golem.value);CHECK(link.recorded.size()==before+1 && link.recorded.back()[8]==112);
        const auto content=session.Priests().Carrier().FindContained(golem,1,qa,0);CHECK(content.value && SquidForms(core.pool).Parent(content)==golem && session.Sp(1)==sp);
        CHECK(!session.Priests().Carrier().FindContained(golem,1,0,1).value); //생존하는 보상 내용물은 항상 하나다.
        if (index) { CHECK(!session.Priests().Carrier().FindContained(golem,1,113,0).value);CHECK(previous==content || (core.pool.Slot(previous)[11]&1)); }
        CHECK(FindAttachedProcess(core.pool,golem,19).value && FindAttachedProcess(core.pool,golem,47).value);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0 && link.transport.Server().depth==0);previous=content;++index;
    }
    CHECK(index==2);wire=nullptr;
}

// 실제 두 벌룬의 첫 그림자 Pop→42 이륙/이동/착륙→그림자 우선 삭제를 같은 Kernel로 진행한다.
TEST_CASE(CampaignSession_BalloonFlightShadowAndLandingThroughActualKernel) {
    // 같은 VT를 쓰는 원본 벌룬111과 에어 쉽141을 서로 새 세션에서 검증한다.
    for (const std::uint32_t kind:{111U,141U}) {
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();double wall=100;std::vector<SoundHandle> sounds;std::vector<Sid> lost; //같은 실제 음향 이름표와 장치 시각·전역소리 요청 입력이다.
        services.sound.wallSeconds=[&] { return wall; };services.sound.playOnceAt=[&](float,float,SoundHandle sound,std::uint32_t loop,std::uint32_t priority) { CHECK(!loop && !priority);sounds.push_back(sound);return sound; };
        services.unitLost=[&](Sid sid) { lost.push_back(sid); }; //실제 Client의 unitLost.wav 장치 경계다. 삭제 관문의 결과를 기록한다.
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto parent=session.World().Create(kind,2);session.World().Pop(parent,98,122,0);core.owner.Set(parent,1);
        const auto shadow=Sid{static_cast<std::uint16_t>(Get(core.pool.Slot(parent),8,2))};CHECK(shadow.value && (core.pool.Slot(shadow)[40]&8));if (!shadow.value) continue;CHECK(core.pool.Slot(shadow)[10]==kind && core.pool.Slot(shadow)[35]==0xec);
        const auto packets=external.packets.size();CHECK(session.StartCarrierMovement({parent,{102,122},1,0,0,0,0}));const auto form=FindAttachedProcess(core.pool,parent,42);auto* path=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(path && path->Runtime().Count()==4);if (!path) continue;CHECK(Get(path->Runtime().Bytes(),0)==0x50e2b8 && external.packets.size()>packets);
        const auto begin=core.processState.now,end=path->Runtime().Time(path->Runtime().Count()-1);bool rose{},landed{};std::size_t frames{};const auto sp=session.Sp(1); //끝 시각은 실제 Prepare가 계산한 값이다.
        // 현재 이동 시각을 연속 공급하며 전체 Kernel의 실제42/그림자 공간 후처리를 실행한다.
        for (std::uint64_t frame=0;frame<1000 && core.processState.now<end+0.25;++frame) {
            const auto now=begin+static_cast<double>(frame)*.025;wall=100+(now-begin);session.RunFrame({now,wall,frame ? .025 : 0.0,frame},false);++frames;
            const auto raw=core.pool.Slot(parent);if (raw[30]) { rose=true;CHECK(raw[35]==0xe2 && core.pool.Slot(shadow)[35]==0xe6); }
            if (!core.processes.FromForm(form)) { landed=true;break; }
        }
        CHECK(rose && landed && frames>2 && core.pool.Slot(parent)[30]==0);CHECK(std::bit_cast<float>(Get(core.pool.Slot(parent),14))==102.0F && std::bit_cast<float>(Get(core.pool.Slot(parent),18))==122.0F);CHECK(session.Sp(1)==sp && core.pool.Slot(shadow)[35]==0xec);
        const auto flame=external.names.Lookup("balloonFlame.wav");CHECK((std::find(sounds.begin(),sounds.end(),flame)!=sounds.end())==(kind==111));CHECK(Get(core.pool.Slot(shadow),36)==core.frames[kind].Codes().size()+Get(core.pool.Slot(parent),36));
        CHECK(session.World().Destroy(parent,0));CHECK((core.pool.Slot(parent)[11]&1) && (core.pool.Slot(shadow)[11]&1));CHECK(lost==std::vector<Sid>{parent});CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
        std::cout<<"실제 벌룬 TYPE"<<kind<<" 42/고도/그림자/착륙/삭제 "<<frames<<"프레임 통과\n";
    }
}

// 선택→18BYTE 송신→실제 buffer/Flush/Poll→서버40h→같은 Kernel 이동을 검사한다.
TEST_CASE(CampaignSession_CarrierMoveRequestThroughActualLocalTransportAndKernel) {
    // 같은 사제54/골렘40/벌룬42를 별개 실제1-1 Session에서 실행한다.
    for (const std::uint32_t kind:{158U,133U,111U}) {
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();netstorm::zacket::LocalMissionTransport* wire{};
        std::uint32_t selected{},mouseType{},mode{};std::array<std::uint32_t,2> remembered{};std::array<std::uint32_t,0> multiple{};std::int32_t count{},mouseX{},mouseY{};std::vector<Sid> dumps; //실제 UI 외부 관측과 단일 현재 선택 저장소다.
        services.deletion.selected=[&] { return Sid{static_cast<std::uint16_t>(selected)}; };
        services.carrierRequest=[&](std::int32_t owner,std::span<const std::uint8_t> packet) { CHECK(wire && owner==9);if (!wire) throw std::logic_error("이동 요청의 실제 전송 미설치");CHECK(wire->Send(owner,packet)); };
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.RequestCarrierMove(Sid{50},{90,108}); }));session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;
        const auto parent=kind==158 ? Sid{static_cast<std::uint16_t>(session.FindPriestParticipation(1))} : session.World().Create(kind,2);CHECK(parent.value);if (!parent.value) continue;
        if (kind!=158) { session.World().Pop(parent,kind==133 ? 94.0F : 98.0F,kind==133 ? 108.0F : 122.0F,0);core.owner.Set(parent,1); }
        const RawPathPoint destination=kind!=111 ? RawPathPoint{90,108} : RawPathPoint{102,122};const auto movement=session.Priests().MovementProcessType(kind);const auto initial=core.pool.Slot(parent);const RawPathPoint source{std::bit_cast<float>(Get(initial,14)),std::bit_cast<float>(Get(initial,18))};const auto sp=session.Sp(1); //현재 위치·SP·실제 경로 종류를 관측한다.
        RawUserHighlight highlight({mode,selected,mouseType,mouseX,mouseY},{});UserSelectionHooks hooks;hooks.paused=[] { return 0U; };hooks.enabled=[] { return 1U; };hooks.blocked=[] { return 0U; };hooks.dump=[&](Sid sid) { dumps.push_back(sid); };hooks.highlight=[&](std::uint32_t value) { highlight.Highlight(value); };hooks.assertion=hooks.previewAssertion=[] { throw std::logic_error("이동 선택 검사 예상 밖 진단"); };
        auto selection=session.MakeUserSelection({selected,mouseType,remembered,multiple,count},std::move(hooks));CHECK(selection->Select(parent.value,0,0)==parent.value && selected==parent.value && (core.pool.Slot(parent)[40]&4));CHECK(dumps==std::vector<Sid>{parent});CHECK(!FindAttachedProcess(core.pool,parent,movement).value);
        const auto& registration=link.transport.Dispatch().Registration(0x40);CHECK(registration.version==1 && !registration.mismatch && registration.length==18 && !registration.observer);session.RequestCarrierMove(parent,destination);CHECK(!selected && !(core.pool.Slot(parent)[40]&4));CHECK(link.recorded.empty() && !link.transport.QueuedFrames() && !FindAttachedProcess(core.pool,parent,movement).value);CHECK(Get(core.pool.Slot(parent),14)==std::bit_cast<std::uint32_t>(source.x) && Get(core.pool.Slot(parent),18)==std::bit_cast<std::uint32_t>(source.y));
        link.transport.Flush();CHECK(link.transport.QueuedFrames()==1);CHECK(link.transport.Poll()==1 && link.recorded.size()==1 && !link.transport.QueuedFrames());if (link.recorded.empty()) continue;const auto& packet=link.recorded.front();CHECK(packet.size()==18 && packet[0]==1 && packet[1]==0x40 && Get(packet,4,2)==parent.value && Get(packet,6)==std::bit_cast<std::uint32_t>(destination.x) && Get(packet,10)==std::bit_cast<std::uint32_t>(destination.y) && Get(packet,14)==0);
        const auto form=FindAttachedProcess(core.pool,parent,movement);auto* path=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(path && !path->Runtime().TargetA() && !path->Runtime().TargetB() && !path->Runtime().MoveAwayDepth());if (!path) continue;const auto begin=core.processState.now,end=path->Runtime().Time(path->Runtime().Count()-1); //원본 Prepare가 선택한 실제 끝 시각이다.
        const RawPathPoint storedDestination{std::bit_cast<float>(Get(path->Runtime().Bytes(),0x186dc)),std::bit_cast<float>(Get(path->Runtime().Bytes(),0x186e0))}; //원본에 저장된 두float를 그대로 중복 비교 입력으로 사용한다.
        session.RequestCarrierMove(parent,storedDestination);link.transport.Flush();CHECK(!link.transport.QueuedFrames() && link.recorded.size()==1 && FindAttachedProcess(core.pool,parent,movement)==form); //현재 같은 목적지는 새 명령을 보내지 않는다.
        std::size_t frames{};
        //경로/도착 결과를 쓰지 않고 같은 전체 Kernel을 연속 시각으로 진행한다.
        for (std::uint64_t frame=0;frame<3000 && core.processState.now<end+.25;++frame) { const auto now=begin+static_cast<double>(frame)*.025;session.RunFrame({now,100+(now-begin),frame ? .025 : 0.0,frame},false);++frames;if (!core.processes.FromForm(form)) break; }
        CHECK(!core.processes.FromForm(form) && frames>1);CHECK(std::bit_cast<float>(Get(core.pool.Slot(parent),14))==destination.x && std::bit_cast<float>(Get(core.pool.Slot(parent),18))==destination.y);CHECK(session.Sp(1)==sp && !link.transport.Server().depth && !core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);CHECK(&session.CarrierMove().Pool()==&core.pool);wire=nullptr;
        std::cout<<"실제40h 요청/수신 TYPE"<<kind<<" 경로"<<movement<<"/자연이동 "<<frames<<"프레임 통과\n";
    }
}


//성공flag를입력하지않고실제Zone→접속→Login→월드설치/응답→상태14/타이머를검사한다.
TEST_CASE(CampaignSession_LocalLoginActualHandshakeAndWorldLoad) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::vector<std::string> trace;std::vector<std::array<std::uint32_t,3>> notices;std::map<std::string,std::int32_t> config;std::size_t refresh{},chat{}; //실제장치/설정의명시입력과관측이다.
    CampaignLoginServices login;login.configInt=[&](std::string_view key,std::int32_t fallback) { const auto at=config.find(std::string(key));return at==config.end() ? fallback : at->second; };login.wallSeconds=[] { return 100.25; };login.clock=[] { return 100250U; };login.refresh=[&] { ++refresh; };login.playerUiBusy=[] { return false; };login.playerNotice=[&](std::uint32_t kind,std::uint32_t owner,std::uint32_t value) { notices.push_back({kind,owner,value}); };login.observePacket=[](std::uint32_t sender,bool boss,std::uint32_t local,std::span<const std::uint8_t> packet) { if (packet[1]==0x6e) std::cout<<"Ready 실제수신 sender"<<sender<<" boss"<<boss<<" local"<<local<<" mask"<<Get(packet,4)<<'\n'; };
    login.log=[&](std::string_view text) { trace.emplace_back(text); };services.localLogin=std::move(login);
    services.ai.participation.clearChat=[&](std::uint32_t channel) { CHECK(channel==0x40 || channel==0x80);++chat; };services.ai.participation.addChat=[&](std::uint32_t subscriber,std::string_view,std::uint32_t channel) { CHECK(channel==0x20 || channel==0x40 || channel==0x80);static_cast<void>(subscriber);++chat; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(!session.CurrentSubscriber() && !session.LocalConnectionPhase() && session.LocalConnectionPackets().empty());CHECK(!(session.PlayerRecords()[0xb4+0x50]&0x43));session.LoadLocalConnection();auto core=session.Core();CHECK(session.CurrentSubscriber()==1 && session.LocalOwner()==1 && session.PlayerKnown()==1 && session.LocalConnectionPhase()==14);CHECK((session.PlayerRecords()[0xb4+0x50]&0x43)==0x43 && session.FindPriestParticipation(1));
    const auto packets=session.LocalConnectionPackets();CHECK(packets.size()>8 && packets.front()==0x27 && std::find(packets.begin(),packets.end(),0x29)!=packets.end() && std::find(packets.begin(),packets.end(),0x28)!=packets.end() && packets.back()==0x6e);CHECK(refresh && chat && !notices.empty());
    std::vector<std::string> states; //원본상태로그만원래순서로읽는다.
    for (const auto& line:trace) { if (line.starts_with("로컬 미션 상태 ")) states.push_back(line);if (line.starts_with("Ready 송신") || line.starts_with("Timer broadcast")) std::cout<<line; }CHECK(states==std::vector<std::string>({"로컬 미션 상태 8\n","로컬 미션 상태 9\n","로컬 미션 상태 10\n","로컬 미션 상태 13\n","로컬 미션 상태 14\n"}));
    const auto before=packets.size();session.PumpLocalConnection();std::cout<<"로그인 실제수신 "<<before<<"→"<<session.LocalConnectionPackets().size()<<" 종류";for (const auto kind:session.LocalConnectionPackets()) std::cout<<' '<<std::uint32_t(kind);std::cout<<'\n';CHECK(session.LocalConnectionPackets().size()==before && session.LocalConnectionPhase()==14);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};const RawPathPoint destination{90,108};const auto sp=session.Sp(1);CHECK(!FindAttachedProcess(core.pool,priest,54).value);session.RequestCarrierMove(priest,destination);CHECK(!FindAttachedProcess(core.pool,priest,54).value);session.PumpLocalConnection();CHECK(session.LocalConnectionPackets().back()==0x40);const auto form=FindAttachedProcess(core.pool,priest,54);auto* path=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(path);if (!path) return;const auto start=core.processState.now,end=path->Runtime().Time(path->Runtime().Count()-1); //실제로그인된같은전송으로40h를소비한경로다.
    //기존Kernel이실제사제이동/도착/자연삭제를수행하도록연속시각만공급한다.
    for (std::uint64_t frame=0;frame<1000 && core.processState.now<end+.25;++frame) { const auto now=start+static_cast<double>(frame)*.025;session.PumpLocalConnection();session.RunFrame({now,100.25+(now-start),frame ? .025 : 0.0,frame},false);if (!core.processes.FromForm(form)) break; }
    CHECK(!core.processes.FromForm(form) && std::bit_cast<float>(Get(core.pool.Slot(priest),14))==90 && std::bit_cast<float>(Get(core.pool.Slot(priest),18))==108 && session.Sp(1)==sp);CHECK(Throws([&] { session.LoadLocalConnection(); }));
}

// 실제 월드 UI의 선택/호버/클릭이 같은 로그인 전송으로 사제·골렘·벌룬을 이동한다.
TEST_CASE(CampaignSession_WorldInputActualLoginThreeCarriersAndKernel) {
    // 세 종류는 별개 실제 미션에서 검사하여 선택/전송 수명이 섞이지 않게 한다.
    for (const std::uint32_t kind:{158U,133U,111U}) {
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);const auto cursors=BindWorldTestInput(world);auto& session=world.Session();auto core=session.Core();CHECK(session.LocalConnectionPhase()==14);
        const auto parent=kind==158 ? Sid{static_cast<std::uint16_t>(session.FindPriestParticipation(1))} : session.World().Create(kind,2);CHECK(parent.value);if (!parent.value) continue;
        if (kind!=158) { session.World().Pop(parent,kind==133 ? 94.0F : 98.0F,kind==133 ? 108.0F : 122.0F,0);core.owner.Set(parent,1); }world.Step(0);
        const auto raw=core.pool.Slot(parent);const RawPathPoint source{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))},destination=kind==111 ? RawPathPoint{102,122} : RawPathPoint{90,108};const auto sp=session.Sp(1);const auto movement=session.Priests().MovementProcessType(kind);const auto before=session.LocalConnectionPackets().size();
        world.SetMousePoint(world.ToScreen(source.x,source.y));world.Select(parent.value);CHECK(world.InputMode()==5 && world.HighlightSettings().selectedSid==parent.value && (core.pool.Slot(parent)[40]&4));CHECK(!FindAttachedProcess(core.pool,parent,movement).value);
        const auto point=world.Point("cell:"+std::to_string(static_cast<int>(destination.x))+','+std::to_string(static_cast<int>(destination.y)));CHECK(point.has_value());world.SetMousePoint(*point);world.UpdateMovementCursor();CHECK(!cursors->empty() && cursors->back()==18);CHECK(world.MoveSelected(*point));CHECK(!world.HighlightSettings().selectedSid && world.InputMode()==0 && !(core.pool.Slot(parent)[40]&4));CHECK(!FindAttachedProcess(core.pool,parent,movement).value && session.LocalConnectionPackets().size()==before);
        session.PumpLocalConnection();CHECK(session.LocalConnectionPackets().size()==before+1 && session.LocalConnectionPackets().back()==0x40);const auto form=FindAttachedProcess(core.pool,parent,movement);auto* path=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(path);if (!path) continue;const auto end=path->Runtime().Time(path->Runtime().Count()-1);std::size_t frames{};
        // 게임 시각만 전진시키고 실제 Kernel의 자연 도착/경로 반납을 기다린다.
        for (;frames<3000 && core.processState.now<end+.5;++frames) { world.Step(.025);if (!core.processes.FromForm(form)) break; }
        CHECK(!core.processes.FromForm(form) && frames>1 && std::bit_cast<float>(Get(core.pool.Slot(parent),14))==destination.x && std::bit_cast<float>(Get(core.pool.Slot(parent),18))==destination.y);CHECK(session.Sp(1)==sp && !core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
        std::cout<<"실제 월드 선택/호버/클릭 TYPE"<<kind<<" 로그인/40h/자연이동 "<<frames<<"프레임 관측\n";
    }
}
