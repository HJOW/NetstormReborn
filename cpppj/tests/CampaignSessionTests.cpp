#include "client/RawCampaignTutorialOneBindings.h"
#include "client/MissionGuideInput.h"
#include "client/CampaignTemplateStorage.h"
#include "client/RawProductionControls.h"
#include "o/RawBridgeDraw.h"
#include "client/RawMenuControls.h"
#include "client/ClientMain.h"
#include "client/RawCampaignMissionStartBindings.h"
#include "client/UserInput.h"
// 생산 세션과 GameWorld의 표시/선택/명령이 같은 실제 raw 월드를 사용하는지 검사한다.
#include "DudeSpawnSupport.h"
#include "client/GameWorld.h"
#include <cmath>
#include "client/RawCampaignFactoryDisplay.h"
#include "client/PaletteColors.h"
#include "o/RawBombEffectTimer.h"
#include "client/SpriteColorMaps.h"
#include "client/VFXMappedDraw.h"
#include "o/RawDaisDraw.h"
#include "o/RawAiCarrierActions.h"
#include "o/RawDudeBehavior.h"
#include "o/RawDudeTask.h"
#include "client/AiTypeRequiredEnergy.h"
#include "client/RawCursorPlacement.h"
#include "client/RawCampaignCursor.h"
#include "client/RawCampaignCursorDisplay.h"
#include "client/CampaignCursorClientEffects.h"
#include "client/RawButtonControls.h"
#include "client/GumpVisual.h"
#include "o/RawMissionAiSettings.h"
#include "o/RawMissionBattleOptions.h"
#include "o/RawGeyserAiRegistration.h"
#include "o/RawSunAviaryProcess.h"
#include "o/RawCarrierSteal.h"
#include "o/RawCarrierPickup.h"
#include "o/RawCarrierMove.h"
#include "o/RawTempleRitual.h"
#include "o/RawSpecialDamage.h"
#include "o/RawPlainWorkshopCapture.h"
#include "o/RawPlainAssetDraw.h"
#include "client/OwnerColorMaps.h"
#include "o/RawGemReward.h"
#include "o/RawBombEffect.h"
#include "client/RawBombBlastBindings.h"
#include "zacket/LocalMissionTransport.h"
#include "client/RawShapeToBuffer.h"
#include "client/OwnerColorMaps.h"
#include "client/SpriteColorMaps.h"
#include <unordered_set>
#include <iostream>
#include <set>
#include <filesystem>
#include "o/RawGunProcess.h"
#include "o/RawSunFlyerFrame.h"
#include "o/RawWindAviaryProcess.h"
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
    std::vector<std::array<std::uint32_t,3>> allianceRequests; // 로컬 로그인 연결을 선택하지 않은 검사의 실제 서버/owner/동맹mask 송신 요청이다.
    std::size_t notices{}; // 외부 UI/음향 요청의 관찰 개수다.
    std::size_t constructionConfigReads{},constructionRefreshes{}; // 명시 건설 요청만 선택해야 하는 실제 설정/표시 외부 경계다.
    // 같은 fixture가 세션보다 오래 살아야 하며 모든 실제 선택 경계를 명시한다.
    RawCampaignSessionServices Services() {
        names.Initialize();RawCampaignSessionServices s;s.unixSeconds=[] { return 946684800; };s.soundNames=&names;
        s.ai.bridgeScratchBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); }; //검사의명시caller초기입력이며본문진행/성공값을만들지않는다.
        s.ai.names.language=[] { return 1U; };s.ai.names.translate=s.ai.names.substitute=[](std::string_view text) { return std::string(text); }; //영어와치환변수없는명시장치입력으로실제객체이름몸체를실행한다.
        s.missionOutcome.demo=[] { return 0U; }; //공통Session검사는명시일반게임입력이며데모검사는자기게이트에연결한다.
        s.contentLifetime.localNotice=[this](std::uint32_t) { ++notices; };s.contentLifetime.sound=[this](float,float) { ++notices; }; //실제수입큐/음향요청만관측하고지갑/SP는Session의Reward가갱신한다.
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
        s.ai.participation.sendAlliance=[this](std::uint32_t server,std::uint32_t owner,std::uint32_t mask) { allianceRequests.push_back({server,owner,mask}); }; // 외부 송신만 관찰하며 Session의 실제 동맹 표를 대신 변경하지 않는다.
        s.deletion.emit=[this](const SquidDestroyEvent& event) { if (event.effect!=SquidDestroyEffect::Transmit) throw std::logic_error("다른파생삭제미연결");++notices; };
        s.deletion.selected=[] { return Sid{}; };s.dependencyTransmit=[this](Sid,std::uint32_t) { ++notices; };
        s.construction.configInt=[this](std::string_view key,std::int32_t fallback) { CHECK(key=="predictableGeysers" && fallback==1);++constructionConfigReads;return 1; };
        s.construction.wallSeconds=[] { return 100.0; };s.construction.refresh=[this] { ++constructionRefreshes; };
        s.construction.negativeQuantity=[] { throw std::logic_error("실제 Session 검사에서 음수 생성수량 진단"); };
        s.construction.boundsAssertion=[](PlayerGeyserBoundsAssertion) { throw std::logic_error("실제 Session 검사에서 PlayerBounds 진단"); };
        return s; // 실제 부모 VT 통지와 Player AI 조회 래퍼를 세션이 공급한다.
    }
};
// 실제 훨리기그 전용 ClearBomb가 같은 Session의 생성·공간·공통 삭제를 끝내는지 검사한다.
TEST_CASE(CampaignSession_SunFlyerBombClearsActualSessionLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");MissionScript script(config,"Tutorial5");CHECK(script.Loaded()); // 스크립트가 지정한 실제 fort 이름을 사용한다.
    auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();auto core=session.Core();
    const auto evidence=(std::filesystem::path(__FILE__).parent_path()/"fixtures/sun-bomb-lifetime-vtables.tsv").string(); // 보호 PE 정적 관측을 생성자/VT와 별개로 대조한다.
    const auto native=LoadFixture(evidence.c_str());CHECK(native.rows.size()==2 && native.rows[0][0]=="originals");CHECK(core.types[163].constructorAddress==std::stoul(native.rows[0][2],nullptr,16));
    const bool boss=true,paused=false;const double now=0;const std::uint32_t bombType=163; // 전용 ClearBomb는 이 시간/권한 값을 변경하지 않는다.
    std::vector<SunFlyerFrameType> types(core.types.size()); // ClearBomb에서 선택하지 않는 프레임 자료의 보관 영역이다.
    std::uint32_t deletes=0;SunFlyerFrameHooks hooks; // 삭제 몸체는 실제 Session에 맡기고 호출 인수만 관찰한다.
    hooks.robust=[&](Sid sid,bool diagnose) { CHECK(diagnose);return sid.value && !(core.pool.Slot(sid)[11]&3U); };
    hooks.destroy=[&](Sid sid,std::uint32_t flags) { CHECK(flags==0x10);++deletes;CHECK(session.World().Destroy(sid,flags)); };
    RawSunFlyerFrame frame(core.pool,core.shapes,types,{boss,paused,now,bombType},std::move(hooks));
    // 떨어지는 폭탄과 이미 Unpop된 폭탄, 부모 소멸 시의 정리를 같은 실제 수명으로 실행한다.
    for (const bool popped:{false,true}) {
        std::array<std::vector<std::uint16_t>,4> heads; // 생성 이전 공간 해시 전체를 비교할 기준이다.
        // 네 공간 단계의 원본 연결 목록을 각각 보존한다.
        for (int level=0;level<4;++level) heads[level].assign(core.hash.Entries(level).begin(),core.hash.Entries(level).end());
        const auto sp=session.Sp(1);const auto kernel=core.kernel.Size();const std::vector<std::uint8_t> spots(core.spots.begin(),core.spots.end());
        const auto bomb=session.World().Create(163,2);CHECK(Get(core.pool.Slot(bomb),0)==std::stoul(native.rows[0][3],nullptr,16));CHECK(core.pool.Slot(bomb)[10]==163);core.pool.AllocatedBytes(bomb)[40]|=8U;
        if (popped) session.World().Pop(bomb,136.0F,110.0F,0);
        FlyerProcessFields fields;fields.bytes.fill(0);fields.sunTail.fill(0);Put(fields.bytes,96,bomb.value); // 실제 전용+96의 현재 폭탄 SID 입력이다.
        frame.ClearBomb(fields);CHECK(Get(fields.bytes,96)==0 && (core.pool.Slot(bomb)[11]&3U)==3U);CHECK(session.Sp(1)==sp && core.kernel.Size()==kernel);
        CHECK(std::equal(spots.begin(),spots.end(),core.spots.begin()));
        // 삭제 후에는 어느 해시 단계에도 폭탄이나 끊어진 체인이 남지 않아야 한다.
        for (int level=0;level<4;++level) CHECK(std::equal(heads[level].begin(),heads[level].end(),core.hash.Entries(level).begin()));
        frame.ClearBomb(fields);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0 && core.bookkeeping.depth==0);
    }
    CHECK(deletes==2);
}

CellPoint EmptySessionSupport(const RawWorldCore& core); // 아래 공용 탐색기의 같은 실제 빈3×3 영역을 사용한다.
// 실제 확정된3×3 받침 위 일반 건물 삭제가9칸/받침/종유석·낙하 파편을 함께 정리하는지 검사한다.
TEST_CASE(CampaignSession_ActualDropIslandSupportAndParticles) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    std::vector<Sid> lost;auto services=external.Services();services.unitLost=[&](Sid sid) { lost.push_back(sid); }; // 실제 삭제가 요청하는 외부UI 통지를 관찰한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
    const auto point=EmptySessionSupport(core); // 같은 실제 지도에서 빈3×3 영역을 명시 입력으로 선택한다.
    CHECK(session.ConfirmSupport(static_cast<float>(point.x),static_cast<float>(point.y)));
    const auto packet=session.Construction().LastPacket();CHECK(packet.size()==45 && packet[26]==9);if (packet.size()!=45) return;
    float right=-10000,bottom=-10000; // 실제 확정된9칸에서 소유자 전파/섬 정리의 오른쪽 아래 기준점을 읽는다.
    // 요청 좌표와 표면 묶음 기준점이 같다고 가정하지 않는다.
    for (std::size_t index=0;index<9;++index) {
        const Sid sid{static_cast<std::uint16_t>(Get(packet,27+index*2,2))};const auto raw=core.pool.Slot(sid);right=std::max(right,std::bit_cast<float>(Get(raw,14)));bottom=std::max(bottom,std::bit_cast<float>(Get(raw,18)));
    }
    std::printf("받침 회귀 요청%d,%d / 실제 오른쪽아래%g,%g\n",point.x,point.y,right,bottom);
    session.World().SetSupportOwner(right,bottom,1);
    std::vector<Sid> family;std::size_t surfaces{},islands{},stalags{}; // 방금 확정한 현재 소유자의 실제 부품이다.
    // 생성/소유자 전파가 만든 실제 부품만 모으며 별도 합성 표면을 만들지 않는다.
    for (const auto& object:session.World().Objects()) {
        if (object.object.owner!=1 || object.object.x<right-2 || object.object.x>right || object.object.y<bottom-2 || object.object.y>bottom) continue;
        if (object.object.type==157) ++surfaces;else if (object.object.type==94) ++islands;else if (object.object.type==95) ++stalags;else continue;family.push_back(object.sid);
    }
    CHECK(surfaces==9 && islands==1 && stalags==1);if (family.size()!=11) return;
    const auto source=session.World().Create(75,0);CHECK(source.value>=core.pool.Layout().serverFirst && core.processState.server);core.owner.Set(source,1);session.World().Pop(source,right,bottom,0); // 실제 서버 삭제로 방송 관문까지 실행한다. extra80은 원본DropIsland 관문이 아니다.
    const auto sp=session.Sp(1);const auto notices=external.notices;CHECK(session.World().Destroy(source,0));CHECK((core.pool.Slot(source)[11]&3U)==3U);
    // 모든 부품의 실제 반납과 공통 삭제 깊이를 확인한다.
    for (const auto sid:family) CHECK((core.pool.Slot(sid)[11]&3U)==3U);
    std::size_t particles{}; // 같은 실제 표시 등록표에 존재하는 종류16을 센다.
    // 파편은 별도 관찰 배열이 아니라 현재Host에서 조회한다.
    for (const auto token:session.Particles().Rendering().Slots()) {
        const auto* fields=session.Particles().Rendering().Lookup(token);if (!fields) continue;
        if (dynamic_cast<ShrapnelVisualProcess*>(core.processes.FromForm(Sid{static_cast<std::uint16_t>(fields->form)}))) ++particles;
    }
    CHECK(particles==10 && session.Sp(1)==sp && external.notices>notices && std::find(lost.begin(),lost.end(),source)!=lost.end());CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
}
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
    CHECK(path->Runtime().Count()==2 && shape.walkStepX*shape.walkStepX+shape.walkStepY*shape.walkStepY<1);
    CHECK(Get(core.pool.Slot(golem),14)!=before || Get(core.pool.Slot(golem),18)!=beforeY);CHECK(!path->Runtime().Finished() && path->Runtime().TargetA()==geyser.value && external.packets.size()==packets);
    std::cout<<"실제 골렘 경로 count="<<path->Runtime().Count()<<" current="<<path->Runtime().Current()<<" now="<<core.processState.now<<" end="<<path->Runtime().Time(1)<<" 보행 간격="<<shape.walkStepX<<','<<shape.walkStepY<<'\n';
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
// 실제 AI 준비 뒤 최초 간헐천 Pop/삭제가 같은 본체의 다섯 후보 목록을 갱신한다. 작성·컴파일만 한다.
TEST_CASE(CampaignSession_LocalGeysersUseActualAiRegistration) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
    services.ai.participation.clearChat=[&external](std::uint32_t) { ++external.notices; };
    services.ai.participation.addChat=[&external](std::uint32_t,std::string_view,std::uint32_t) { ++external.notices; };
    services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();
    const auto token=session.CreateAi(2,SessionAiSettings("Explicit","sunCannon"));auto records=session.PlayerRecords();records[0xb4+0x50]=2;
    CHECK(token!=0 && session.Participation().Brain(2)==token && session.PrepareAi(2));const auto raw=session.AiMemory().Bytes(token);const auto count=Get(raw,0x32c);session.GenerateLocalGeysers();
    CHECK(records[0xb4+0x50]==2 && (records[2*0xb4+0x50]&0x22U)==0x20U && Get(records,0xb4+0x78)==0 && Get(records,2*0xb4+0x78)==1); // 실제1·2번 모두 활성 지역이 있어 원본 Generate가 각3개씩 생성한다.
    CHECK(session.Participation().Brain(2)==token && session.AiMemory().Contains(token) && Get(raw,0x32c)==count+6);CHECK(session.Construction().LocalGeysers().Recent().count==3); // 최근 목록은 마지막2번 Populate의3개만 보존한다.
    const auto created=session.Construction().LocalGeysers().Recent().entries[0];CHECK(session.Core().pool.Slot(Sid{static_cast<std::uint16_t>(created)})[10]==122);CHECK(session.World().Destroy(Sid{static_cast<std::uint16_t>(created)},0x200000));CHECK(Get(raw,0x32c)==count+5);
    // 실제 삭제된 간헐천은 다섯 활성 목록 어디에도 남지 않는다.
    for (const auto at:{0x324U,0x330U,0x33cU,0x348U,0x354U}) { const auto list=session.AiMemory().Bytes(Get(raw,at));for (std::uint32_t index=0;index<Get(raw,at+8);++index) CHECK(Get(list,index*4)!=created); } // 안쪽 반복문은 현재 활성 count만 방문한다.
    session.RemoveAi(2);CHECK(session.Participation().Brain(2)==0);
}
// 우리 편도 같은 준비 몸체로 기존 자산을 한 번만 등록하며 전체 전략 자동 실행과 구분한다.
TEST_CASE(CampaignSession_LocalAiPreparationSharesOriginalWorld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.PrepareAi(1); }));session.Load();CHECK(!session.PrepareAi(1));
    auto profile=SessionAiSettings("Local AI","sunWalker;sunCannon");Put(profile,0x22c,static_cast<std::uint32_t>(static_cast<std::int32_t>(session.Sp(1))));const auto token=session.CreateAi(1,profile);const auto raw=session.AiMemory().Bytes(token);CHECK(!Get(raw,0x25c) && !Get(raw,0x258));const auto before=session.Sp(1);CHECK(session.PrepareAi(1));CHECK(Get(raw,0x25c)==1 && Get(raw,0x264)==2 && Get(raw,0x294)!=0 && Get(raw,0x29c)==session.FindPriestParticipation(1) && session.Sp(1)==before);CHECK(!session.Core().bookkeeping.aiAttached[1]);
    const auto allocations=session.AiMemory().Allocations().size();const auto first=std::vector<std::uint8_t>(raw.begin(),raw.end());CHECK(session.PrepareAi(1));CHECK(session.AiMemory().Allocations().size()==allocations && std::equal(first.begin(),first.end(),raw.begin()));CHECK(!Get(raw,0x258)); // 14af0의 준비 완료 표식을 이 하위에서 가짜로 켜지 않는다.
    session.RemoveAi(1);CHECK(!session.PrepareAi(1));
}
// 초기 전략이 실제 내용물/기술부모/난수 세계를 사용하고 같은258으로 재호출을 막는지 검사한다.
TEST_CASE(CampaignSession_AiStrategyInitializationUsesActualContents) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.InitializeAi(1); }));session.Load();CHECK(!session.InitializeAi(1));auto profile=SessionAiSettings("Local AI","sunWalker;sunCannon");Put(profile,0x238,3);const auto token=session.CreateAi(1,profile);CHECK(session.InitializeAi(1));const auto raw=session.AiMemory().Bytes(token);CHECK(Get(raw,0x258)==1 && !Get(raw,0x25c) && Get(raw,0x264)==2 && Get(raw,0x628)>=3);const auto first=std::vector<std::uint8_t>(raw.begin(),raw.end());const auto allocated=session.AiMemory().Allocations().size();CHECK(session.InitializeAi(1) && session.AiMemory().Allocations().size()==allocated && std::equal(first.begin(),first.end(),raw.begin()));session.RemoveAi(1);CHECK(!session.InitializeAi(1));
}
// AI 경제 정리가 실제SP만 float로 저장하고 같은 본체의 정수 지갑에 재가산하지 않는지 확인한다.
TEST_CASE(CampaignSession_AiEconomySharesActualSpAndWallet) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.ReconcileAiEconomy(1); }));session.Load();CHECK(!session.ReconcileAiEconomy(1) && !session.RefreshAiBridges(1));auto profile=SessionAiSettings("Local AI","sunWalker;sunCannon");Put(profile,0x22c,16777217);Put(profile,0x24c,0);const auto token=session.CreateAi(1,profile);CHECK(session.InitializeAi(1));CHECK(session.ReconcileAiEconomy(1) && session.Sp(1)==16777216.0F && Get(session.AiMemory().Bytes(token),0x2d8)==16777217);CHECK(session.RefreshAiBridges(1));session.RemoveAi(1);CHECK(!session.ReconcileAiEconomy(1) && !session.RefreshAiBridges(1));
}
// 원격 AI의 실제 생산 조각/Regular/지갑과 합법 건설 요청을 같은 월드에서 연결한다.
TEST_CASE(CampaignSession_AiDeploymentUsesActualProductionAndConstruction) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.DeployAi(2,133,0,0); }));session.Load();CHECK(!session.DeployAi(2,133,0,0));const auto token=session.CreateAi(2,SessionAiSettings("Deploy AI","sunWalker;sunCannon"));CHECK(session.InitializeAi(2));auto core=session.Core();const auto priest=Sid{static_cast<std::uint16_t>(session.FindPriestParticipation(2))};CHECK(priest.value!=0);const auto raw=core.pool.Slot(priest);const auto originX=static_cast<std::int32_t>(std::bit_cast<float>(Get(raw,14))),originY=static_cast<std::int32_t>(std::bit_cast<float>(Get(raw,18)));bool found=false;float x{},y{};
    // 원본 전체 MayPlace로 빈 합법 후보를 찾고 배치 결과를 직접 만들지 않는다.
    for (int dy=-12;dy<=12 && !found;++dy) {
        // 같은 시작 섬 근처의 후보를 차례로 확인한다.
        for (int dx=-12;dx<=12;++dx) { x=static_cast<float>(originX+dx);y=static_cast<float>(originY+dy);if (session.MayPlaceConstruction({133,0,x,y,0,2,0})) { found=true;break; } }
    }
    CHECK(found);if (!found) return;const auto wallet=Get(session.AiMemory().Bytes(token),0x2d8);const auto sp=session.Sp(2);CHECK(session.DeployAi(2,133,x,y));CHECK(Get(session.AiMemory().Bytes(token),0x2d8)==wallet-static_cast<std::uint32_t>(core.types[133].cost) && session.Sp(2)==sp);const auto parent=Sid{static_cast<std::uint16_t>(Get(session.PlayerRecords(),2*180+0x9c))};RawContainedFinder finder(core.pool,session.ProductionFinder());Sid pending{};
    // 같은 실제 부모 체인에서 새 pending 생산을 찾고 그 Regular을 조회한다.
    for (auto form=finder.Begin(parent);form.value;form=finder.Next()) if (Get(core.pool.Slot(form),18,2)==133 && Get(core.pool.Slot(form),20,2)==0x108) { pending=form;break; }
    CHECK(pending.value!=0 && core.processes.FindEvent(pending,0)!=nullptr);session.RemoveAi(2);CHECK(!session.DeployAi(2,133,x,y));
}
// 원본 수집자 선택이 같은 실제 생산/MayPlace/배치/목록 통지까지 이어지는지 검사한다.
TEST_CASE(CampaignSession_AiCollectorSelectionUsesActualDeployment) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.EnsureAiCollectors(2); }));session.Load();CHECK(!session.EnsureAiCollectors(2));auto profile=SessionAiSettings("Collector AI","sunWalker;sunCannon");Put(profile,0x22c,5000);Put(profile,0x234,100);const auto token=session.CreateAi(2,profile);CHECK(session.InitializeAi(2) && session.PrepareAi(2));const auto before=Get(session.AiMemory().Bytes(token),0x2e4);CHECK(!Get(session.AiMemory().Bytes(token),0x2cc));
    // 최초3초 생각 기한 전에는 행동하지 않으며 실제 프레임의 현재 시각으로 기한에 도달한다.
    CHECK(session.EnsureAiCollectors(2) && Get(session.AiMemory().Bytes(token),0x2e4)==before);
    session.RunFrame({3.0,3.0,3.0,1},false);
    // 후보가 모두 막힌 경우 다음 정상 판단으로 재시도하며 조각/지갑/플래그를 직접 쓰지 않는다.
    for (std::size_t attempt=0;attempt<20 && Get(session.AiMemory().Bytes(token),0x2e4)==before;++attempt) CHECK(session.EnsureAiCollectors(2));
    auto core=session.Core();Sid building; //현재 실제 서버 배치의 미완료 수집자를 찾는다.
    // abstract 배치 중에는 AI 수집자 목록에 아직 등록되지 않아야 한다.
    for (std::uint32_t number=5;number<core.pool.Capacity();++number) { const Sid sid{static_cast<std::uint16_t>(number)};const auto object=core.pool.Slot(sid);if (!(object[11]&3U) && object[10]==133 && object[34]==2 && (object[40]&1U)) { building=sid;break; } }
    CHECK(building.value && Get(session.AiMemory().Bytes(token),0x2e4)==before);if (building.value) CHECK(Get(core.pool.Slot(building),26)==0);
    // 같은 Kernel의 건설 완료·실제 PostPop 통지가 목록에 도달할 때까지 정상 프레임만 진행한다.
    for (std::uint32_t frame=1;frame<=300 && Get(session.AiMemory().Bytes(token),0x2e4)==before;++frame) { const double game=3.0+static_cast<double>(frame)*0.05;session.RunFrame({game,game,0.05,frame+1},false); } //game은 이 프레임의 실제 초 단위 입력이다.
    CHECK(Get(session.AiMemory().Bytes(token),0x2e4)==before+1);session.RemoveAi(2);CHECK(!session.EnsureAiCollectors(2));
}
// 원본 MasterOfWhirligigs의 빈 원격 워크샾에 같은 기술/생산 내용물과 Deck 통지를 연결한다.
TEST_CASE(CampaignSession_AiFactoryProductionUsesOriginalWorkshops) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.production.effects.configInt=[](std::string_view) { return 0; };services.production.effects.modeAssertion=[] { throw std::logic_error("검사 외 생산 모드 진단"); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/masterofwhirligigs.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"masterofwhirligigs",std::move(services));CHECK(Throws([&] { session.ManageAiFactories(2); }));session.Load();CHECK(!session.ManageAiFactories(2));const auto token=session.CreateAi(2,SessionAiSettings("Factory AI","sunWalker;sunCannon"));CHECK(session.InitializeAi(2) && session.PrepareAi(2));const auto raw=session.AiMemory().Bytes(token);CHECK(Get(raw,0x314)==2);const auto list=session.AiMemory().Bytes(Get(raw,0x30c));const auto factory=Sid{static_cast<std::uint16_t>(Get(list,0))};std::uint32_t count{};
    // 전체AI의선행수집자행동도같은생각기한을사용하므로수동갱신을중복호출하지않고첫자연생산까지진행한다.
    for (std::uint64_t frame=0;frame<400 && !count;++frame) { const auto now=3.0+static_cast<double>(frame)*0.1;session.RunFrame({now,now,0.1,frame+1},false);count=static_cast<std::uint32_t>(CountContained(session.Core().pool,session.ProductionFinder(),factory,0x10,0)); }
    std::printf("자동AI 워크샾 첫생산%u 시각%g 수집자%u\n",count,session.Core().processState.now,Get(raw,0x2e4));CHECK(Get(raw,0x314)==2);
    CHECK(count==1);session.RemoveAi(2);CHECK(!session.ManageAiFactories(2));
}
// 같은58h 분배/분산SP/Kernel의 실제 SharedRegular을 요청과 수신 양쪽에서 확인한다. 진행 프레임은 실행하지 않는다.
TEST_CASE(CampaignSession_FactoryUpgradeCommandUsesActualHostAndSp) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::vector<std::vector<std::uint8_t>> packets;services.carrierRequest=[&](std::int32_t target,std::span<const std::uint8_t> packet) { CHECK(target==9);packets.emplace_back(packet.begin(),packet.end()); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/masterofwhirligigs.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"masterofwhirligigs",std::move(services));CHECK(Throws([&] { session.RequestFactoryUpgrade(Sid{1}); }));session.Load();auto core=session.Core();Sid factory{};
    // 원본 저장 맵에서 복원된 살아 있는 소유자2 Factory를 실제 슬롯/가상 표로 찾는다.
    for (std::uint32_t value=1;value<core.pool.Capacity();++value) { const auto sid=Sid{static_cast<std::uint16_t>(value)};const auto raw=core.pool.Slot(sid);if (!(raw[11]&3U) && raw[34]==2 && Get(raw,0)==0x509148U) { factory=sid;break; } }
    CHECK(factory.value!=0);if (!factory.value) return;session.RequestFactoryUpgrade(factory);CHECK(packets.size()==1);const auto client=core.processes.FindEvent(factory,4);CHECK(client && client->Payload()==20.0F);if (packets.empty()) return;const auto before=session.Sp(2);const auto cost=session.HitPoints().TypeCostUnits(core.pool.Slot(factory)[10]);PacketDispatchHooks hooks;hooks.now=[] { return 19U; };hooks.record=[](auto&,auto,auto) {};hooks.terminate=[](auto&) {};hooks.close=[](auto&) {};hooks.release=[](auto&) {};RawPacketDispatch dispatch(OriginalEdition::Patch1078,hooks);session.RegisterFactoryResearchCommands(dispatch);PacketConnection connection;connection.sender=0xffffffffU;CHECK(dispatch.Dispatch(connection,packets[0]));const auto server=core.processes.FindEvent(factory,3);CHECK(server && server->Payload()==15.0F && session.Sp(2)==before-static_cast<float>(cost));
}
// 같은 실제 Host의event3만명시호출하여예측그림/건설/감시쌍과완료의자기예약삭제를함께검사한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_FactoryUpgradePictureAndCompletionShareWorld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/masterofwhirligigs.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"masterofwhirligigs",external.Services());CHECK(Throws([&] { session.FactoryUpgrading(Sid{1}); }));session.Load();auto core=session.Core();Sid factory{};
    // 원본 Fort의레벨0/소유자2 Factory를같은실제풀에서찾는다.
    for (std::uint32_t value=5;value<core.pool.Capacity();++value) { const Sid candidate{static_cast<std::uint16_t>(value)};const auto raw=core.pool.Slot(candidate);if (!(raw[11]&3U) && raw[34]==2 && Get(raw,0)==0x509148U && !(raw[11]>>5)) { factory=candidate;break; } }
    CHECK(factory.value);if (!factory.value) return;auto* regular=core.processes.AddSharedRegular(factory,3,15);CHECK(regular && session.FactoryUpgrading(factory));if (!regular) return;regular->RunFrame();CHECK(regular->Count()==1);Sid image{};
    // 현재client영역에서같은TYPE/extra8 그림과실제의존쌍을찾는다.
    for (std::uint16_t value=5;value<15000;++value) { const Sid candidate{value};const auto raw=core.pool.Slot(candidate);if (!(raw[11]&3U) && (raw[40]&8U) && raw[10]==core.pool.Slot(factory)[10] && core.dependencies.Has(factory,candidate)) { image=candidate;break; } }
    CHECK(image.value && (core.pool.Slot(image)[11]>>5)==1);if (!image.value) return;bool construction{};
    CHECK(Get(core.pool.Slot(image),14)==Get(core.pool.Slot(factory),14)); // 그래프 칸 변환이 원래 그림의 float 위치를 바꾸지 않아야 한다.
    CHECK(std::bit_cast<float>(Get(core.pool.Slot(image),18))==static_cast<float>(static_cast<double>(std::bit_cast<float>(Get(core.pool.Slot(factory),18)))+static_cast<double>(0.0010000000474974513F))); // 원본509144의 깊이 오프셋을 정확히 보존한다.
    // 그림head의마지막Depend와실제TYPE30건설프로세스를구분한다.
    for (auto form=Get(core.pool.Slot(image),6,2);form;form=Get(core.pool.Slot(Sid{static_cast<std::uint16_t>(form)}),4,2)) { const Sid candidate{static_cast<std::uint16_t>(form)};if (core.pool.Slot(candidate)[10]==30) construction=core.processes.FromForm(candidate)!=nullptr; }
    CHECK((core.pool.Slot(image)[40]&9U)==9 && construction);core.processState.now=15;regular->RunFrame();CHECK((core.pool.Slot(factory)[11]>>5)==1 && Get(core.pool.Slot(factory),26)==static_cast<std::uint32_t>(session.HitPoints().MaxHitPoints(factory)));CHECK(!core.processes.FindEvent(factory,3) && !session.FactoryUpgrading(factory) && (core.pool.Slot(image)[11]&1U));
}
namespace {
// 원본MasterOfWhirligigs의살아있는소유자2 Factory를같은실제풀에서찾는다.
Sid InputFactory(RawCampaignSession& session) {
    auto core=session.Core();
    // 자유/죽음슬롯을제외하고실제Factory가상표를확인한다.
    for (std::uint32_t value=5;value<core.pool.Capacity();++value) { const Sid candidate{static_cast<std::uint16_t>(value)};const auto raw=core.pool.Slot(candidate);if (!(raw[11]&3U) && raw[34]==2 && Get(raw,0)==0x509148U) return candidate; }return {};
}
}
// 실제로컬SP/같은event4와58h송신을입력관문에연결한다.검사는작성·컴파일만한다.
TEST_CASE(CampaignSession_FactoryInputUsesActualSpAndPendingTimer) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::vector<std::string> tells;std::vector<std::vector<std::uint8_t>> packets;services.factoryTell=[&](std::string_view section) { tells.emplace_back(section); };services.carrierRequest=[&](std::int32_t target,std::span<const std::uint8_t> packet) { CHECK(target==9);packets.emplace_back(packet.begin(),packet.end()); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/masterofwhirligigs.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"masterofwhirligigs",std::move(services));CHECK(Throws([&] { session.CommandFactory(Sid{1},0x03000032U,0); }));session.Load();auto core=session.Core();const auto factory=InputFactory(session);CHECK(factory.value);if (!factory.value) return;const auto local=session.LocalOwner();const auto cost=session.HitPoints().TypeCostUnits(core.pool.Slot(factory)[10]);CHECK(cost>0 && core.pool.Slot(factory)[34]!=local);session.SetSp(local,0);CHECK(session.CommandFactory(factory,0xff000032U,factory.value)==1 && tells==std::vector<std::string>({"upgradeNoBucks"}) && packets.empty() && !session.FactoryUpgrading(factory));
    session.SetSp(local,static_cast<float>(cost));CHECK(session.CommandFactory(factory,0x03000032U,factory.value)==1 && packets.size()==1 && session.FactoryUpgrading(factory));const auto wait=core.processes.FindEvent(factory,4);CHECK(wait && wait->Payload()==20.0F && session.Sp(local)==static_cast<float>(cost));CHECK(session.CommandFactory(factory,0x03000032U,factory.value)==1 && tells==std::vector<std::string>({"upgradeNoBucks","alreadyUpgrading"}) && packets.size()==1 && core.processes.FindEvent(factory,4)==wait);
}
// 동일입력의상태순환/연구도움말/미처리부모위임이실제Session에합성된다.실행하지않는다.
TEST_CASE(CampaignSession_FactoryInputCycleHelpAndParent) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::uint32_t helps{};services.factoryHelp=[&](std::string_view section,std::uint32_t mode) { CHECK(section=="researchHelp" && mode==1);++helps; };auto fort=FortTemplate::Parse(assets.files.Read("d/masterofwhirligigs.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"masterofwhirligigs",std::move(services));session.Load();auto core=session.Core();const auto factory=InputFactory(session);CHECK(factory.value);if (!factory.value) return;
    // 0→1→2→0의실제level/상태프레임보정과기존SP/업그레이드예약보존을확인한다.
    for (std::uint32_t index=1;index<=3;++index) { CHECK(session.CommandFactory(factory,0xff000063U,0)==1);const auto raw=core.pool.Slot(factory);CHECK(static_cast<std::uint32_t>(raw[11]>>5)==index%3);CHECK(std::bit_cast<std::int32_t>(Get(raw,36))==core.frames[raw[10]].Run(static_cast<std::uint8_t>('A'+index%3)).first); }
    CHECK(session.SelectFactoryKnowledge(factory,0x03000080U,factory.value)==1 && helps==1);CHECK(session.CommandFactory(factory,0,0xabcd0000U|factory.value)==0 && !session.FactoryUpgrading(factory));
}
// 현재Player의실제DrySpot할당과같은에너지판정/AI소멸을함께검사한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_AiDrySpotsUseActualBrainEnergyAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.RecheckAiDrySpots(2); }));session.Load();CHECK(!session.RecordAiDrySpot(2,20,20,82) && session.RecheckAiDrySpots(2)==0);const auto token=session.CreateAi(2,SessionAiSettings("DrySpot AI","sunCannon"));auto core=session.Core();const auto requirement=BindAiTypeRequiredEnergy(assets.game,core.types);std::uint32_t type{};
    // 실제AST/현재TYPE에서요구에너지가없는첫타입을고르며판정결과를주입하지않는다.
    for (std::uint32_t candidate=70;candidate<core.types.size();++candidate) if (requirement(candidate).empty()) { type=candidate;break; }
    CHECK(type && token);if (!type || !token) return;const auto raw=session.AiMemory().Bytes(token);const auto backing=Get(raw,0xae4);CHECK(Get(raw,0xadc)==10 && Get(raw,0xae0)==0 && Get(raw,0xaec)==2);const auto before=session.Sp(2);CHECK(session.RecordAiDrySpot(2,20,20,type) && !session.RecordAiDrySpot(2,20,20,type));CHECK(Get(raw,0xae0)==1 && Get(session.AiMemory().Bytes(backing),8)==type);CHECK(session.RecheckAiDrySpots(2)==type && Get(raw,0xae0)==0 && session.Sp(2)==before);session.RemoveAi(2);CHECK(!session.AiMemory().Contains(backing) && !session.RecordAiDrySpot(2,20,20,type) && session.RecheckAiDrySpots(2)==0);
}
// AI명령이같은실제BFS/Host경로를만들고정지가그Form을삭제하는지검사한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_AiMovementUsesRealPathNamesAndStop) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.ai.names.language=[] { return 1U; };services.ai.names.translate=services.ai.names.substitute=[](std::string_view text) { return std::string(text); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.AiMovementForm(Sid{}); }));session.Load();auto core=session.Core();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(2))};CHECK(priest.value);if (!priest.value) return;CHECK(!session.MoveAiToPoint(2,priest,{20,20}));const auto token=session.CreateAi(2,SessionAiSettings("Move AI","sunWalker;sunCannon"));CHECK(token);const auto raw=core.pool.Slot(priest);const int x=static_cast<int>(std::bit_cast<float>(Get(raw,14))),y=static_cast<int>(std::bit_cast<float>(Get(raw,18)));CHECK(session.ObjectName(priest)==core.types[raw[10]].description);
    SquidPostPopList additional;ContainedFinderState finder;PlayerPlacementAnchorState state;state.graphReady=1;RawPlayerPlacementAnchor anchor(core.pool,core.types,core.hash.Entries(0),core.bookkeeping,additional,finder,state);const auto graph=anchor.GraphFor(priest.value);RawPathPoint destination{};bool found{};
    // 같은실제Graph의자유칸을입력으로고르며경로객체/성공값을주입하지않는다.
    for (int radius=4;radius<10 && !found;++radius) for (int dy=-radius;dy<=radius && !found;++dy) for (int dx=-radius;dx<=radius;++dx) { const auto px=x+dx,py=y+dy;if (px<2 || py<2 || px>=254 || py>=254) continue;if ((core.spots[py*256+px]&7U)==2 && anchor.GraphAt(static_cast<float>(px),static_cast<float>(py))==graph) { destination={static_cast<float>(px),static_cast<float>(py)};found=true;break; } } //안쪽두반복문도실제원본칸만방문한다.
    CHECK(found);if (!found) return;const auto before=session.Sp(2);CHECK(session.MoveAiToPoint(2,priest,destination));const auto form=session.AiMovementForm(priest);CHECK(form.value && core.pool.Slot(form)[10]==54 && core.processes.FromForm(form)!=nullptr);if (!form.value) return;session.StopAiMovement(priest);CHECK(!session.AiMovementForm(priest).value && core.processes.FromForm(form)==nullptr && session.Sp(2)==before);session.StopAiMovement(Sid{});session.RemoveAi(2);CHECK(!session.MoveAiToPoint(2,priest,destination));
}
// 현재실제간헐천목록/그래프와크리스털내용물·신전귀환을함께검사한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_AiResourceChoiceUsesActualGraphContentsAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };services.ai.names.language=[] { return 1U; };services.ai.names.translate=services.ai.names.substitute=[](std::string_view text) { return std::string(text); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.CollectAiOrReturnHome(1,Sid{}); }) && Throws([&] { session.RecheckAiGeysers(1); }));session.Load();CHECK(!session.RecheckAiGeysers(1) && !session.CollectAiOrReturnHome(1,Sid{}));const auto token=session.CreateAi(1,SessionAiSettings("Resource AI","sunWalker;sunCannon"));CHECK(token && session.PrepareAi(1));if (!token) return;auto core=session.Core();const auto raw=session.AiMemory().Bytes(token);const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto crystal=session.CreateContained(priest,154,0,0x20,0,0);CHECK(crystal.value && CountContained(core.pool,session.ProductionFinder(),priest,0x20,154)==1);const auto before=session.Sp(1);const auto registered=Get(raw,0x32c);CHECK(session.CollectAiOrReturnHome(1,priest));CHECK(session.RecheckAiGeysers(1) && Get(raw,0x32c)==registered && session.Sp(1)==before && SquidForms(core.pool).Parent(crystal)==priest);
    SquidPostPopList additional;ContainedFinderState finder;PlayerPlacementAnchorState state;state.graphReady=1;RawPlayerPlacementAnchor anchor(core.pool,core.types,core.hash.Entries(0),core.bookkeeping,additional,finder,state);const auto connected=session.AiMemory().Bytes(Get(raw,0x330));
    // 실제재분류후남은목록을실제GraphFor로읽으며기대그래프를주입하지않는다.
    for (std::uint32_t index=0;index<Get(raw,0x338);++index) CHECK(anchor.GraphFor(Get(connected,index*4))==Get(raw,0x298));
    session.RemoveAi(1);CHECK(!session.RecheckAiGeysers(1) && !session.CollectAiOrReturnHome(1,priest));
}
// 실제월드수집자/내용물genus/AI목록/경로Host와전체행동수명을함께검사한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_AiCarrierActionsUseActualWorldContentsAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };services.ai.names.language=[] { return 1U; };services.ai.names.translate=services.ai.names.substitute=[](std::string_view text) { return std::string(text); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.ManageAiCarriers(2); }));session.Load();CHECK(!session.ManageAiCarriers(2));auto core=session.Core();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(2))};CHECK(priest.value);if (!priest.value) return;const auto origin=core.pool.Slot(priest);const auto walker=session.World().Create(133,2);session.World().Pop(walker,std::bit_cast<float>(Get(origin,14)),std::bit_cast<float>(Get(origin,18)),0);core.owner.Set(walker,2);const auto token=session.CreateAi(2,SessionAiSettings("Carrier AI","sunWalker;sunCannon"));CHECK(token && session.PrepareAi(2));if (!token) return;const auto raw=session.AiMemory().Bytes(token);CHECK(Get(raw,0x2e4)==1 && Get(session.AiMemory().Bytes(Get(raw,0x2dc)),0)==walker.value);if (Get(raw,0x2e4)!=1) return;const auto captive=session.CreateContained(walker,158,3,0x20,0,0);CHECK(captive.value && FindContainedGenus(core.pool,session.ProductionFinder(),core.types,walker,0x20,0x200000,0)==captive);const auto before=session.Sp(2);CHECK(session.ManageAiCarriers(2) && Get(raw,0x6d0)==0 && Get(raw,0x2e4)==1 && session.Sp(2)==before && SquidForms(core.pool).Parent(captive)==walker);const auto form=session.AiMovementForm(walker);if (form.value) { CHECK(core.processes.FromForm(form)!=nullptr);session.StopAiMovement(walker);CHECK(!core.processes.FromForm(form)); }session.RemoveAi(2);CHECK(!session.ManageAiCarriers(2));
}
// 실제초기전략의자기경계/적기준좌표와같은spot·전체배치검사·벽시각을연결한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_AiBuildSearchUsesActualStatePlacementAndClock) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::size_t clocks{};services.construction.wallSeconds=[&] { ++clocks;return 100.0; };services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.StartAiBuildSearch(2,86); }));session.Load();CHECK(!session.StartAiBuildSearch(2,86) && !session.AdvanceAiBuildSearch(2,0));const auto token=session.CreateAi(2,SessionAiSettings("Build Search AI","sunWalker;sunCannon"));CHECK(token && session.PrepareAi(2) && session.InitializeAi(2));if (!token) return;auto core=session.Core();const auto raw=session.AiMemory().Bytes(token);CHECK(Get(raw,0x258)==1);if (Get(raw,0x258)!=1) return;const auto before=session.Sp(2);CHECK(session.StartAiBuildSearch(2,86) && Get(raw,0x574)==86 && Get(raw,0x578)==0 && Get(raw,0x57c)==0xc61c4000U && Get(raw,0x58c)!=0 && Get(raw,0x584)==Get(raw,0x2b0) && Get(raw,0x588)==Get(raw,0x2b4) && Get(raw,0x590)==2);const auto previousClocks=clocks;CHECK(session.AdvanceAiBuildSearch(2,0) && clocks>previousClocks && Get(raw,0x5a4)==Get(raw,0x5a0) && session.Sp(2)==before);
    if (std::bit_cast<float>(Get(raw,0x578))>0) { const auto x=static_cast<std::int32_t>(std::bit_cast<float>(Get(raw,0x57c))),y=static_cast<std::int32_t>(std::bit_cast<float>(Get(raw,0x580)));CHECK(x>=std::bit_cast<std::int32_t>(Get(raw,0x594)) && x<std::bit_cast<std::int32_t>(Get(raw,0x59c)) && y>=std::bit_cast<std::int32_t>(Get(raw,0x598)) && y<std::bit_cast<std::int32_t>(Get(raw,0x5a0)) && (core.spots[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]&0x3fU)==2); }
    session.RemoveAi(2);CHECK(!session.StartAiBuildSearch(2,86) && !session.AdvanceAiBuildSearch(2,0));
}
// 실제로컬SP/두Player의현재AI지갑과읽기전용건설TYPE/수명을연결한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_AiBuildChoiceUsesActualSpWalletAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.SelectAiBuildType(2); }) && Throws([&] { session.AiEnemyLowFunds(2); }));session.Load();CHECK(!session.SelectAiBuildType(2) && !session.AiEnemyLowFunds(2));const auto second=session.CreateAi(2,SessionAiSettings("Choice AI","sunWalker;sunCannon"));CHECK(second && session.PrepareAi(2) && session.InitializeAi(2));if (!second) return;const auto raw=session.AiMemory().Bytes(second);CHECK(Get(raw,0x264)==1 && session.PlayerKnown()==1 && session.LocalOwner()==1);session.SetSp(1,999.75F);CHECK(session.AiEnemyLowFunds(2));session.SetSp(1,1000.25F);CHECK(!session.AiEnemyLowFunds(2));const auto before=std::vector<std::uint8_t>(raw.begin(),raw.end());CHECK(session.SelectAiBuildType(2)<188 && std::equal(before.begin(),before.end(),raw.begin()));const auto first=session.CreateAi(1,SessionAiSettings("Local AI","sunWalker;sunCannon"));CHECK(first && session.PrepareAi(1));if (!first) return;CHECK(Get(session.AiMemory().Bytes(first),0x264)==2);const auto core=session.Core();CHECK(session.AiEnemyLowFunds(1)==(core.ownerMode.challenge || std::bit_cast<std::int32_t>(Get(raw,0x2d8))<1000));session.RemoveAi(2);CHECK(session.AiEnemyLowFunds(1) && !session.SelectAiBuildType(2) && !session.AiEnemyLowFunds(2));session.RemoveAi(1);CHECK(!session.AiEnemyLowFunds(1));
}
// 실제 사제/건설선택/동일 탐색 상태/이동 Host와 AI 수명을 연결한다. 작성·컴파일만 한다.
TEST_CASE(CampaignSession_AiPriestActionsUseActualSearchMovementAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services(); // 보호 자산의 실제 지도와 명시 외부 서비스 입력이다.
    services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };services.ai.names.language=[] { return 1U; };services.ai.names.translate=services.ai.names.substitute=[](std::string_view text) { return std::string(text); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.ManageAiPriest(2,Sid{}); }));session.Load();CHECK(!session.ManageAiPriest(2,Sid{}));auto profile=SessionAiSettings("Priest AI","sunWalker;sunCannon");Put(profile,0x22c,5000);const auto token=session.CreateAi(2,profile);CHECK(token && session.PrepareAi(2) && session.InitializeAi(2));if (!token) return;
    auto core=session.Core();const auto raw=session.AiMemory().Bytes(token);const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(2))};CHECK(priest.value && Get(raw,0x29c)==priest.value);if (!priest.value) return;core.processState.now=100;const auto type=session.SelectAiBuildType(2);CHECK(type && type<188);if (!type) return;const auto sp=session.Sp(2);const auto wallet=Get(raw,0x2d8);CHECK(session.StartAiBuildSearch(2,type) && Get(raw,0x574)==type);
    CHECK(session.ManageAiPriest(2,Sid{}) && Get(raw,0x5a4)==Get(raw,0x5a0) && Get(raw,0x574)==type && session.Sp(2)==sp && Get(raw,0x2d8)==wallet);CHECK(session.ManageAiPriest(2,priest));const auto form=session.AiMovementForm(priest);
    if (form.value) { CHECK(core.processes.FromForm(form)!=nullptr);session.StopAiMovement(priest);CHECK(!core.processes.FromForm(form)); } // 실제 이동이 선택된 경우 같은 Host를 정상 종료한다.
    session.RemoveAi(2);CHECK(!session.ManageAiPriest(2,priest) && !core.destroy.PreDepth() && !core.destroy.PostDepth());
}
// 실제 운반체/사제 내용물/현재 TYPE hpPerSec/같은 Gun 검색과 AI 수명을 연결한다. 작성·컴파일만 한다.
TEST_CASE(CampaignSession_AiDropTargetsUseActualContentsGunAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services(); // 보호 지도와 명시 장치 입력이다.
    services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));Sid result{9};CHECK(Throws([&] { session.ScoreAiTarget(2,Sid{}); }));session.Load();CHECK(!session.ScoreAiTarget(2,Sid{}) && !session.ScoreAiGun(2,128,20,30,2,result) && result==Sid{9});CHECK(!session.ScoreAiBlockers(2,78,20,30,0,result) && result==Sid{9});const auto token=session.CreateAi(2,SessionAiSettings("Drop Target AI","sunWalker;sunCannon"));CHECK(token && session.PrepareAi(2));if (!token) return;
    auto core=session.Core();const auto raw=session.AiMemory().Bytes(token);CHECK(Get(raw,0x264)==1);const auto walker=session.World().Create(133,2);core.owner.Set(walker,1);const auto before=session.ScoreAiTarget(2,walker);const auto sp=session.Sp(2);const auto wallet=Get(raw,0x2d8);const auto captive=session.CreateContained(walker,158,3,0x20,0,0);CHECK(captive.value);CHECK(session.ScoreAiTarget(2,walker)==before+400 && session.Sp(2)==sp && Get(raw,0x2d8)==wallet);
    const auto gunScore=session.ScoreAiGun(2,128,20,30,2,result);CHECK(gunScore==(result.value ? session.ScoreAiTarget(2,result) : 0));session.ScoreAiBlockers(2,78,20,30,0,result);CHECK(session.Sp(2)==sp && Get(raw,0x2d8)==wallet);session.RemoveAi(2);result=Sid{9};CHECK(!session.ScoreAiTarget(2,walker) && !session.ScoreAiGun(2,128,20,30,2,result) && !session.ScoreAiBlockers(2,78,20,30,0,result) && result==Sid{9});
}
// 실제 부족좌표/공급·지상검색/전체배치승인과같은AI수명을연결한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_AiDropEvaluationUsesActualManaGroundPlacementAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services(); // 실제 보호 지도와 명시 외부 장치 입력이다.
    services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));Sid target{9};RawPathPoint best{7,8};std::int32_t bestScore{};CHECK(Throws([&] { session.ScoreAiDrop(2,128,20,30,0,target); }) && Throws([&] { session.ConsiderAiDrop(2,128,20,30,0,best,bestScore,target); }));session.Load();CHECK(!session.ScoreAiDrop(2,128,20,30,0,target) && !session.ConsiderAiDrop(2,128,20,30,0,best,bestScore,target) && target==Sid{9} && best.x==7 && best.y==8);
    const auto token=session.CreateAi(2,SessionAiSettings("Drop Evaluation AI","sunWalker;sunCannon"));CHECK(token && session.PrepareAi(2));if (!token) return;auto core=session.Core();const auto raw=session.AiMemory().Bytes(token);std::uint32_t supplyType{},groundType{};
    // 실제 현재TYPE의분류로기존시설을선택하며합성flags나평가결과를주입하지않는다.
    for (std::uint32_t type=70;type<core.types.size();++type) { if (!supplyType && (core.types[type].flags2&0x800U)) supplyType=type;if (!groundType && core.types[type].group==3 && !(core.types[type].flags2&0x4000800U)) groundType=type; }
    CHECK(supplyType && groundType);if (!supplyType || !groundType) return;const auto sp=session.Sp(2);const auto wallet=Get(raw,0x2d8);CHECK(session.RecordAiDrySpot(2,21,30,supplyType));target=Sid{9};CHECK(session.ScoreAiDrop(2,supplyType,20,30,2,target)>=0 && target==Sid{9});const auto groundScore=session.ScoreAiDrop(2,groundType,20,30,2,target);CHECK(groundScore==(target.value ? session.ScoreAiTarget(2,target) : 0));
    const bool approved=session.ConsiderAiDrop(2,supplyType,20,30,2,best,bestScore,target);if (approved) CHECK(best.x==20 && best.y==30);CHECK(session.Sp(2)==sp && Get(raw,0x2d8)==wallet);session.RemoveAi(2);target=Sid{9};best={7,8};bestScore=0;CHECK(!session.ScoreAiDrop(2,supplyType,20,30,2,target) && !session.ConsiderAiDrop(2,supplyType,20,30,2,best,bestScore,target) && target==Sid{9} && best.x==7 && best.y==8 && !bestScore);
}
// 실제지도/다리frame·공용난수/현재본체스캔진행과AI수명을공유한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_AiDropScanUsesActualMapsStateAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services(); // 보호 Fort와 명시 장치 입력이다.
    services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.AdvanceAiDropScan(2,128,0); }));session.Load();CHECK(!session.AdvanceAiDropScan(2,128,0) && !session.RemoveAiDropChoice(2,0));const auto token=session.CreateAi(2,SessionAiSettings("Drop Scan AI","sunWalker;sunCannon"));CHECK(token && session.InitializeAi(2) && session.PrepareAi(2));if (!token) return;CHECK(session.SelectAiDropType(2));const auto raw=session.AiMemory().Bytes(token);const auto x=Get(raw,0xaf8),y=Get(raw,0xafc);const auto sp=session.Sp(2);const auto wallet=Get(raw,0x2d8);CHECK(session.AdvanceAiDropScan(2,128,0) && (Get(raw,0xaf8)!=x || Get(raw,0xafc)!=y) && Get(raw,0x570)<=30 && session.Sp(2)==sp && Get(raw,0x2d8)==wallet);session.RemoveAi(2);CHECK(!session.AdvanceAiDropScan(2,128,0) && !session.RemoveAiDropChoice(2,0));
}
// 실제 Player 기술부모/텔레포트 내용물·스캔/AI 수명으로 TYPE 선택을 연결한다.작성·컴파일만한다.
TEST_CASE(CampaignSession_AiDropSelectionUsesActualProductsScanAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services(); // 보호 Fort와 명시 외부 장치 입력이다.
    services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.SelectAiDropType(2); }));session.Load();CHECK(!session.SelectAiDropType(2));const auto token=session.CreateAi(2,SessionAiSettings("Drop Selection AI","sunWalker;sunCannon"));CHECK(token && session.PrepareAi(2));if (!token) return;auto core=session.Core();const auto known=session.FindTechnology(2,128);CHECK(known.value);if (!known.value) return;const auto parent=SquidForms(core.pool).Parent(known);CHECK(parent.value);if (!parent.value) return;const auto product=session.CreateContained(parent,128,0,8,0,0);CHECK(product.value);const auto raw=session.AiMemory().Bytes(token);const auto sp=session.Sp(2);const auto wallet=Get(raw,0x2d8);
    // 실제기술목록을순환하고운반체제외로0이반환된경우에만다음선택을요청한다.
    for (std::size_t step=0;step<8;++step) { CHECK(session.SelectAiDropType(2));if (Get(raw,0x3e0)) break; }
    const auto type=Get(raw,0x3e0);CHECK(type && type<188 && !Get(raw,0x570) && !Get(raw,0x3e8) && !Get(raw,0x3ec) && Get(raw,0xaf8)==10 && Get(raw,0xafc)==10);if (type) CHECK(session.AdvanceAiDropScan(2,type,std::bit_cast<std::int32_t>(Get(raw,0x3e4))));CHECK(session.Sp(2)==sp && Get(raw,0x2d8)==wallet);session.RemoveAi(2);CHECK(!session.SelectAiDropType(2));
}
// 실제 생산 조각/선택·스캔을 전체 배치 행동으로 연결하고 원래 지갑 관문/수명을 확인한다.작성·컴파일만 한다.
TEST_CASE(CampaignSession_AiDropActionsUseActualSelectionScanAndWallet) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services(); //보호 Fort와 명시 외부 장치 입력이다.
    services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.ManageAiDrops(2); }));session.Load();CHECK(!session.ManageAiDrops(2));auto profile=SessionAiSettings("Drop Actions AI","sunWalker;sunCannon");Put(profile,0x22c,1199);const auto token=session.CreateAi(2,profile);CHECK(token && session.PrepareAi(2));if (!token) return;auto core=session.Core();const auto known=session.FindTechnology(2,128);CHECK(known.value);if (!known.value) return;const auto parent=SquidForms(core.pool).Parent(known);CHECK(parent.value);if (!parent.value) return;const auto product=session.CreateContained(parent,128,0,8,0,0);CHECK(product.value);const auto raw=session.AiMemory().Bytes(token);const auto sp=session.Sp(2);CHECK(Get(raw,0x2d8)==1199);
    // 실제 기술 순환이 운반체TYPE를 제외하면 다음 정상 호출에서 다시 선택한다.
    for (std::size_t step=0;step<8;++step) { CHECK(session.ManageAiDrops(2));if (Get(raw,0x3e0)) break; }
    CHECK(Get(raw,0x3e0) && (Get(raw,0xaf8)!=10 || Get(raw,0xafc)!=10 || Get(raw,0x3ec)));CHECK(session.ManageAiDrops(2) && Get(raw,0x2d8)==1199 && session.Sp(2)==sp && !Get(raw,0x3f4) && !Get(raw,0x3fc) && !(core.pool.Slot(product)[11]&1U) && !(Get(core.pool.Slot(product),20,2)&0x100U));session.RemoveAi(2);CHECK(!session.ManageAiDrops(2));
}
// 실제 AI의 자기/비교 영역·fake TYPE와 같은 Dry Spot backing 및 소멸을 검사한다. 작성·컴파일만 한다.
TEST_CASE(CampaignSession_AiDrySeedUsesActualBrainListAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services(); // 보호 Fort와 명시 외부 장치 입력이다.
    services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.SeedAiDrySpot(2); }));session.Load();CHECK(!session.SeedAiDrySpot(2));const auto token=session.CreateAi(2,SessionAiSettings("Dry Seed AI","sunCannon"));CHECK(token && session.PrepareAi(2));if (!token) return;const auto raw=session.AiMemory().Bytes(token);const auto backing=Get(raw,0xae4),type=Get(raw,0x280),wallet=Get(raw,0x2d8);const auto sp=session.Sp(2);CHECK((Get(raw,0x248)&0x20) && !Get(raw,0xae0) && type);CHECK(session.SeedAiDrySpot(2) && Get(raw,0xae0)==1);const auto records=session.AiMemory().Bytes(backing);CHECK(Get(records,0)==Get(raw,0xaf0) && Get(records,4)==Get(raw,0xaf4) && Get(records,8)==type && Get(raw,0x2d8)==wallet && session.Sp(2)==sp);session.RemoveAi(2);CHECK(!session.AiMemory().Contains(backing) && !session.SeedAiDrySpot(2));
}
// 보호 Fort의 실제 지도에서 연속 빈칸을 찾아 같은 지도/경계의 원본 탐색을 연결한다. 작성·컴파일만 한다.
TEST_CASE(CampaignSession_AiDryGeometryUsesActualSurface) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());CHECK(Throws([&] { session.ComputeAiDryCandidate({30,40},{34,40}); }));session.Load();auto core=session.Core();std::int32_t chosenX{},chosenY{};
    // 실제 표면의 세로 네 빈칸을 고르고 탐색 결과/지도를 주입하지 않는다.
    for (std::int32_t y=20;y<200 && !chosenX;++y) { for (std::int32_t x=20;x<200;++x) { bool empty=true;for (std::int32_t row=0;row<4;++row) if (core.hash.Cell(0,x,y+row)) empty=false;if (empty) { chosenX=x;chosenY=y;break; } } } // 안쪽 반복은 실제 X 후보와 세로 네 칸을 순서대로 검사한다.
    CHECK(chosenX);if (!chosenX) return;const auto sp=session.Sp(1);const auto point=session.ComputeAiDryCandidate({static_cast<float>(chosenX-1),static_cast<float>(chosenY-1)},{static_cast<float>(chosenX+1),static_cast<float>(chosenY-1)});CHECK(point.x==chosenX && point.y==chosenY+3 && session.Sp(1)==sp);
}
// 실제 로컬 AI의 빈 부족목록/현재 신전·공급시설과 공용 Seed 수명을 연결한다.작성·컴파일만 한다.
TEST_CASE(CampaignSession_AiFakeDrySpotsUseActualStateAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services(); //보호 Fort와 명시 외부 장치 입력이다.
    services.ai.allocationBytes=[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };services.missionOutcome.demo=[] { return 0U; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.ExtendAiFakeDrySpots(1); }));session.Load();CHECK(!session.ExtendAiFakeDrySpots(1));const auto token=session.CreateAi(1,SessionAiSettings("Fake Dry AI","sunWalker;sunCannon"));CHECK(token && session.PrepareAi(1));if (!token) return;const auto raw=session.AiMemory().Bytes(token);const auto pointer=Get(raw,0xae4),type=Get(raw,0x280),suppliers=Get(raw,0x2fc),temple=Get(raw,0x294),wallet=Get(raw,0x2d8);const auto beforeX=Get(raw,0xaf0),beforeY=Get(raw,0xaf4);const auto sp=session.Sp(1);CHECK(type && temple && !Get(raw,0xae0) && (Get(raw,0x248)&0x20));CHECK(session.ExtendAiFakeDrySpots(1) && Get(raw,0xae0)==1);const auto record=session.AiMemory().Bytes(pointer);CHECK(Get(record,8)==type && session.Sp(1)==sp && Get(raw,0x2d8)==wallet);
    if (!suppliers) { const auto object=session.Core().pool.Slot(Sid{static_cast<std::uint16_t>(temple)});CHECK(Get(record,0)==Get(object,14) && Get(record,4)==Get(object,18) && Get(raw,0xaf0)==beforeX && Get(raw,0xaf4)==beforeY); }else CHECK(Get(record,0)==Get(raw,0xaf0) && Get(record,4)==Get(raw,0xaf4));session.RemoveAi(1);CHECK(!session.AiMemory().Contains(pointer) && !session.ExtendAiFakeDrySpots(1));
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
// 실제 서버 환불은 같은 SP/현재 Player brain에 가산하며 미연결 생명주기 부착 표를 켜지 않는다. 작성/컴파일만 수행한다.
TEST_CASE(CampaignSession_RefundCreditsCurrentActualAiWallet) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::size_t demoReads{},moneyNotices{},refreshes{}; // 원본 부모의 선택된 현재 UI 경계만 관측한다.
    services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };
    services.missionOutcome.demo=[&] { ++demoReads;return 0U; };services.refreshSp=[&] { ++refreshes; };services.construction.presentation.sendMoney=[&](auto owner,const auto& notice) { CHECK(owner==2 && notice.amount>0);++moneyNotices; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto type=assets.game.TypeTable().Find("sunCannon");const auto cost=static_cast<std::int32_t>(core.types[type].cost);
    const auto token=session.CreateAi(2,SessionAiSettings("Wallet AI","sunCannon"));CHECK(Get(session.AiMemory().Bytes(token),0x2d8)==777 && !core.bookkeeping.aiAttached[2]);const auto before=session.Sp(2);demoReads=moneyNotices=refreshes=0; // 로드/생성과 구분하여 명시 환불부터의 실제 통지만 계측한다.
    session.Construction().Refund(type,2);CHECK(session.Sp(2)==before+static_cast<float>(cost) && Get(session.AiMemory().Bytes(token),0x2d8)==777U+static_cast<std::uint32_t>(cost));CHECK(demoReads==1 && moneyNotices==1 && refreshes==1 && !core.bookkeeping.aiAttached[2]);
    core.ownerMode.challenge=true;session.Construction().Refund(type,2);CHECK(Get(session.AiMemory().Bytes(token),0x2d8)==777U+static_cast<std::uint32_t>(cost) && demoReads==1 && moneyNotices==2 && refreshes==2);core.ownerMode.challenge=false; // 같은00594fc8 비활성 조건은 SP 지급 뒤 지갑 단계만 건너뛴다.
    session.RemoveAi(2);session.Construction().Refund(type,2);CHECK(!session.Participation().Brain(2) && demoReads==1 && moneyNotices==3 && refreshes==3);
}
// 실제 새 자산의 Pop/삭제가 같은 brain 목록을 왕복하며 값형 부착 표 없이 통지한다. 작성·컴파일만 한다.
TEST_CASE(CampaignSession_ObjectNoticesUseActualPopAndDeletion) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();
    services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[](auto) {};services.ai.participation.addChat=[](auto,auto,auto) {};services.ai.localTransport=[] { return true; };services.missionOutcome.demo=[] { return 0U; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto point=EmptySessionSupport(core);
    CHECK(session.ConfirmSupport(static_cast<float>(point.x),static_cast<float>(point.y)));const auto packet=session.Construction().LastPacket();CHECK(packet.size()==45);if (packet.size()!=45) return;float right=-10000,bottom=-10000; // 실제 생성된9칸의 공통 기준점만 읽는다.
    // 받침 확정 결과의 실제 좌표를 사용하며 기대 목록을 결과에서 만들지 않는다.
    for (std::size_t index=0;index<9;++index) { const Sid part{static_cast<std::uint16_t>(Get(packet,27+index*2,2))};const auto raw=core.pool.Slot(part);right=std::max(right,std::bit_cast<float>(Get(raw,14)));bottom=std::max(bottom,std::bit_cast<float>(Get(raw,18))); }
    session.World().SetSupportOwner(right,bottom,2);const auto token=session.CreateAi(2,SessionAiSettings("Notice AI","windBattery"));const auto raw=session.AiMemory().Bytes(token);CHECK(!Get(raw,0x6d4+75*4) && !Get(raw,0x2fc) && !core.bookkeeping.aiAttached[2]);
    const auto source=session.World().Create(75,0);core.owner.Set(source,2);session.World().Pop(source,right,bottom,1);
    CHECK(Get(raw,0x6d4+75*4)==1 && Get(raw,0x2fc)==1 && Get(session.AiMemory().Bytes(Get(raw,0x2f4)),0)==source.value && !core.bookkeeping.aiAttached[2]);
    CHECK(session.World().Destroy(source,0x200000));CHECK(!Get(raw,0x6d4+75*4) && !Get(raw,0x2fc) && !Get(raw,0x2f0) && !core.bookkeeping.aiAttached[2]);CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
    session.RemoveAi(2);CHECK(!session.Participation().Brain(2) && !session.AiMemory().Contains(token));
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
    // 실제 캠페인의 레인 신전은 원본 수집/일반Draw procedure에서 본체·물결을 합성한다.
    if (mission=="thewarbegins") {
        bool found=false;
        // 실제 본체 procedure와 전체 몸체 클릭 범위를 확인한다. 픽셀 순서는 별도264원문 검사로 대조한다.
        for (const auto& sprite:world.Sprites()) if (sprite.procedure && sprite.order.x==89 && sprite.order.y==116) {
            found=true;
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

// 세 제네레이터의 원본 기준점과 본체/그림자 및 클릭 영역을 실제 GameWorld에서 확인한다.
TEST_CASE(CampaignSession_GeneratorBodyShadowAndPicking) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    std::array<std::uint32_t,256> logical{}; // 실제 파일 팔레트로 원본 그림자 색표를 준비한다.
    // 출력 장치의 BGR 순서와 구분한 논리 RGB 값이다.
    for (std::size_t index=0;index<logical.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); }
    const auto maps=BuildSpriteColorMaps(logical);
    // 이름이 다른 세 자산 모두 원본의 같은 생성자/가상 표와 기준점을 가진다.
    for (const auto number:{75U,76U,77U}) {
        SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();services.install.resetCamera={};
        GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);
        auto& session=world.Session();const auto core=session.Core();const auto origin=EmptySessionSupport(core);
        const float x=static_cast<float>(origin.x+2),y=static_cast<float>(origin.y+2); // 실제 spot으로 확인한 빈3×3 영역의 오른쪽 아래 기준점이다.
        const auto sid=session.World().Create(number,2);core.owner.Set(sid,1);session.World().Pop(sid,x,y,0);world.Scroll(-4096,-4096);
        CHECK(Get(core.pool.Slot(sid),0)==0x501ea0 && core.types[number].hotspotX==8 && core.types[number].hotspotY==4);
        world.frameTime=[] { return FrameTime{0,1,0,1}; };world.paused=[] { return true; };world.RunFrame(); // 같은 raw 슬롯을 실제 클릭 뷰에도 반영한다.
        // 정수 카메라 이동 뒤에도 보정이 중복되거나 사라지지 않아야 한다.
        for (const auto camera:std::array<ScreenPoint,2>{{{0,0},{7,9}}}) {
            world.Scroll(camera.x,camera.y);const auto point=world.ToScreen(x,y);std::vector<RenderSprite> pair;
            // 같은 건물의 원본 본체와 y-.1 그림자만 추린다. 다른 장면 객체는 검사 표면에 섞지 않는다.
            for (const auto& sprite:world.Sprites()) if (sprite.order.x==x && (sprite.order.y==y || sprite.order.y==static_cast<float>(y-.1))) pair.push_back(sprite);
            CHECK(pair.size()==2);if (pair.size()!=2) continue;
            // 원본 수집 순서의 각 레이어를 실제 RLE/색표로 독립 출력한 전체 표면과 비교한다.
            for (const auto& sprite:pair) {
                CHECK(sprite.procedure.has_value());if (!sprite.procedure) continue;
                const auto frame=sprite.order.y==y ? 0U : 2U;const auto& shape=assets.game.Shapes().Blocks()[number-70].frames[frame];const auto image=assets.game.Shapes().Decode(number-70,frame);
                std::vector<std::uint8_t> actual(1024*768,199),expected=actual;const ScreenRect clip{0,28,1024,768};
                sprite.procedure->draw(actual,1024,768,clip);
                DrawIndexedImage(expected,1024,768,image,point.x-8+shape.rect.left,point.y-4+shape.rect.top,clip,frame==2 ? &maps.shadow : nullptr);
                CHECK(actual==expected);CHECK(std::any_of(actual.begin(),actual.end(),[](auto value) { return value!=199; }));
            }
            const auto& shape=assets.game.Shapes().Blocks()[number-70].frames[0];
            CHECK(world.Pick({point.x-8+shape.rect.left+1,std::max(28,point.y-4+(shape.rect.top+shape.rect.bottom)/2)})==sid.value);
            auto raw=core.pool.AllocatedBytes(sid);raw[40]|=4; // 선택 괄호와 HP/자원 조회가 추가된 실제 일반 Draw 경로도 실행한다.
            Renderer selected(1024,768);selected.SetScene(pair);std::vector<std::uint8_t> selectedPixels(1024*768,199);selected.Draw(selectedPixels,1024,199);
            CHECK(std::any_of(selectedPixels.begin(),selectedPixels.end(),[](auto value) { return value!=199; }));raw[40]&=static_cast<std::uint8_t>(~4U);
        }
        CHECK(std::bit_cast<float>(Get(core.pool.Slot(sid),14))==x && std::bit_cast<float>(Get(core.pool.Slot(sid),18))==y);
    }
}

// 원본 캡처의37개 실제 그림 전체와 같은 화면을 두 카메라에서 비교해 준비 중 본체 소실을 잡는다.
TEST_CASE(CampaignSession_ThunderCannonBodyNative37Frames) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();services.install.resetCamera={};
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);
    const auto core=world.Session().Core();const Sid target{15002};auto raw=core.pool.AllocatedBytes(target);CHECK(raw[10]==124);
    raw[10]=131;Put(raw,0,0x50bad0U);raw[34]=1;raw[40]=2; // 같은 표시 슬롯에 명시한 원본 썬더 캐논 상태이며 자연 건설/전투 입력으로 세지 않는다.
    const float x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));
    world.frameTime=[] { return FrameTime{0,1,0,1}; };world.paused=[] { return true; };
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/thunder-cannon-capture-x86.tsv").string();std::size_t cases{},pixels{}; // 기존 독립 원본 픽셀은 재생성하지 않는다.
    // 일반 alive 추가 그림/owner1에 해당하는37행을 원본에서 고른다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(path.c_str()).rows) if (row[1]=="2" && row[2]=="1") {
        Put(raw,36,static_cast<std::uint32_t>(std::stoul(row[0])));world.RunFrame();const auto metadata=netstorm::test::rawscene::Split(row[3],',');
        const int width=std::stoi(metadata[0]),height=std::stoi(metadata[1]);const int hotX=std::stoi(metadata[2])+core.types[131].hotspotX+2,hotY=std::stoi(metadata[3])+core.types[131].hotspotY-3; // 원문TYPE(-2,3)에서 실제TYPE(2,4)로 메타 기준점만 환산한다.
        // 동일 raw 좌표를 유지하며 카메라의 정수 이동도 독립 확인한다.
        for (const auto camera:std::array<ScreenPoint,2>{{{0,0},{7,9}}}) {
            world.Scroll(camera.x,camera.y);const auto point=world.ToScreen(x,y);std::vector<RenderSprite> body;
            // 같은 객체의 실제 장면 항목만 렌더링하며 주변 지형의 가림은 섞지 않는다.
            for (const auto& sprite:world.Sprites()) if (sprite.order.x==x && sprite.order.y==y && (sprite.procedure || sprite.block==131-70)) body.push_back(sprite);
            CHECK(body.size()==1);if (body.size()!=1) return;
            Renderer renderer(1024,768);renderer.SetScene(body);std::vector<std::uint8_t> actual(1024*768,199);renderer.Draw(actual,1024,199);std::size_t mismatch{},painted{};
            // 원본 캡처의 불투명/투명 전체 영역을 화면 좌표로 옮겨 픽셀 단위로 대조한다.
            for (int index=0;index<width*height;++index) {
                const int dx=point.x-hotX+index%width,dy=point.y-hotY+index/width;if (dx<0 || dx>=1024 || dy<28 || dy>=768) continue;
                const auto expected=static_cast<std::uint8_t>(std::stoul(row[4].substr(static_cast<std::size_t>(index)*2,2),nullptr,16));
                if (actual[dy*1024+dx]!=(expected==255 ? 199 : expected)) ++mismatch;if (expected!=255) ++painted;++pixels;
            }
            if (mismatch) std::cout<<"썬더 캐논 현재 그림"<<row[0]<<" 원본 화면 픽셀 차이="<<mismatch<<'\n';
            CHECK(mismatch==0 && painted>0);world.Scroll(-camera.x,-camera.y);++cases;
        }
    }
    CHECK(cases==74 && pixels>400000);std::cout<<"썬더 캐논37프레임/두 카메라 원본 화면 대조 픽셀="<<pixels<<'\n';
}

namespace {
// 동결된 원본 전체 픽셀의 한 바이트를 읽으며 제품의 그림/프레임 계산은 재사용하지 않는다.
std::uint8_t DisplayNativeByte(std::string_view hex,std::size_t index) { return static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index*2,2)),nullptr,16)); }
// 원본 캡처를 실제 화면 위치에 옮겨 투명/불투명 전체 영역의 차이와 대조 픽셀 수를 반환한다.
std::pair<std::size_t,std::size_t> CompareDisplayNative(std::span<const std::uint8_t> actual,ScreenPoint origin,int width,int height,std::string_view hex) {
    std::size_t mismatch{},pixels{};
    // 화면과 메뉴 경계 안의 각 원본 픽셀만 대조한다.
    for (int index=0;index<width*height;++index) {
        const int x=origin.x+index%width,y=origin.y+index/width;if (x<0 || x>=1024 || y<28 || y>=768) continue;
        const auto expected=DisplayNativeByte(hex,static_cast<std::size_t>(index));if (actual[y*1024+x]!=(expected==255 ? 199 : expected)) ++mismatch;++pixels;
    }
    return {mismatch,pixels};
}
}

// 골렘의 북/남을 포함한64보행 프레임은 원본 발 기준점8,2와 전체 픽셀이 두 카메라에서 일치해야 한다.
TEST_CASE(CampaignSession_GolemNative64WalkingFrames) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();services.install.resetCamera={};
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);
    std::array<ScreenColor,256> palette{};
    for (std::size_t index=0;index<palette.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));palette[index]={color.blue,color.green,color.red,0}; } // 실제 원본 표시기에 파일 팔레트를 설치한다.
    world.SetPalette(palette);
    const auto core=world.Session().Core();const auto origin=EmptySessionSupport(core);
    const float x=static_cast<float>(origin.x)+2.25f,y=static_cast<float>(origin.y)+3.625f; // 다른 선택 가능한 지형의 간섭을 피한 소수 월드 좌표의 표시 전용 입력이다.
    const auto sid=world.Session().World().Create(133,2);world.Session().World().Pop(sid,x,y,0);auto raw=core.pool.AllocatedBytes(sid);raw[34]=0;raw[40]=128; // 원문과 같은 중립/현재프레임 표시이며 실제 플레이의 소유권 조작이 아니다.
    world.Scroll(-4096,-4096); // 첫 빈 영역이 화면 안에 들어오도록 실제 카메라를 원점으로 옮긴다.
    CHECK(core.types[133].hotspotX==8 && core.types[133].hotspotY==2);std::size_t cases{},pixels{},mismatches{};
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/golem-capture-x86.tsv").string();
    // 원본의8방향 각8프레임을 재생하며 빈/도움말 프레임은 사용자 보행 범위에서 제외한다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(path.c_str()).rows) {
        const auto frame=static_cast<std::uint32_t>(std::stoul(row[0]));if (frame>=64) continue;Put(raw,36,frame);
        const auto metadata=netstorm::test::rawscene::Split(row[1],',');const int width=std::stoi(metadata[0]),height=std::stoi(metadata[1]),hotX=std::stoi(metadata[2]),hotY=std::stoi(metadata[3]);
        // 같은 소수 월드 좌표를 유지하고 카메라 이동 전후의 표시만 비교한다.
        for (const auto camera:std::array<ScreenPoint,2>{{{0,0},{7,9}}}) {
            world.Scroll(camera.x,camera.y);const auto point=world.ToScreen(x,y);std::vector<RenderSprite> body;
            // 실제 골렘 본체만 분리해 지면/주변 유닛의 가림을 섞지 않는다.
            for (const auto& sprite:world.Sprites()) if (sprite.procedure && !sprite.shadow && sprite.order.x==x && sprite.order.y==static_cast<float>(static_cast<double>(y)+0.5)) body.push_back(sprite);
            CHECK(body.size()==1);if (body.size()!=1) return;
            Renderer renderer(1024,768);renderer.SetScene(body);std::vector<std::uint8_t> actual(1024*768,199);renderer.Draw(actual,1024,199);
            const auto compared=CompareDisplayNative(actual,{point.x-hotX,point.y-hotY},width,height,row[2]);mismatches+=compared.first;pixels+=compared.second;CHECK(compared.second>0);++cases;world.Scroll(-camera.x,-camera.y);
        }
    }
    CHECK(cases==128 && pixels>50000 && mismatches==0);std::cout<<"골렘64보행프레임/두 카메라 원본 대조 픽셀="<<pixels<<" 차이="<<mismatches<<'\n';
    Put(raw,36,5);world.frameTime=[] { return FrameTime{0,1,0,1}; };world.paused=[] { return true; };world.RunFrame();const auto* object=world.Object(sid.value);CHECK(object);if (!object) return;
    const auto point=world.ToScreen(object->x,object->y);const auto& rect=assets.game.Shapes().Blocks()[133-70].frames[object->frame].rect;
    const ScreenPoint leftEdge{point.x-8+rect.left,point.y-2+(rect.top+rect.bottom)/2}; // 수정 전8픽셀 오른쪽 클릭 영역의 여유4픽셀 밖에 있는 실제 본체 왼쪽 끝이다.
    const auto picked=world.Pick(leftEdge);if (picked!=sid.value) std::cout<<"골렘 클릭 대상="<<sid.value<<" 실제="<<picked<<" 화면="<<point.x<<','<<point.y<<" 왼쪽="<<leftEdge.x<<','<<leftEdge.y<<" frame="<<object->frame<<" selectable="<<object->selectable<<" TYPE="<<object->type<<" VT="<<Get(raw,0)<<'\n';
    CHECK(picked==sid.value);
}

// 실제 원본 간헐천51프레임의 고정 외형/현재 그림을 두 카메라의 GameWorld 전체 픽셀과 대조한다.
TEST_CASE(CampaignSession_GeyserBodyNative51Frames) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();services.install.resetCamera={};
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);
    const auto core=world.Session().Core();auto raw=core.pool.AllocatedBytes(Sid{15002});CHECK(raw[10]==124);raw[10]=122;Put(raw,0,0x50da88U);raw[34]=1;raw[40]=2; // 기존 표시 슬롯에 주는 명시 입력이며 자연 채집/건설 관측이 아니다.
    const float x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));std::size_t cases{},pixels{};
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/geyser-capture-x86.tsv").string();
    // 원본 alive 추가 그림/owner1의51행을 선택하고 기대값은 새로 계산하지 않는다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(path.c_str()).rows) if (row[1]=="2" && row[2]=="1") {
        Put(raw,36,static_cast<std::uint32_t>(std::stoul(row[0])));const auto metadata=netstorm::test::rawscene::Split(row[3],',');
        const int width=std::stoi(metadata[0]),height=std::stoi(metadata[1]),hotX=std::stoi(metadata[2])+core.types[122].hotspotX+2,hotY=std::stoi(metadata[3])+core.types[122].hotspotY-3; // 원문TYPE(-2,3)에서 현재TYPE(3,2)로 기준점만 환산한다.
        // 정수 카메라 이동에도 같은 본체/애니메이션 합성이 유지돼야 한다.
        for (const auto camera:std::array<ScreenPoint,2>{{{0,0},{7,9}}}) {
            world.Scroll(camera.x,camera.y);const auto point=world.ToScreen(x,y);std::vector<RenderSprite> body;
            // 그림자와 같은 좌표의 지면을 본체 비교에서 제외한다.
            for (const auto& sprite:world.Sprites()) if (sprite.order.x==x && sprite.order.y==y && (sprite.procedure || sprite.block==122-70)) body.push_back(sprite);
            CHECK(body.size()==1);if (body.size()!=1) return;
            Renderer renderer(1024,768);renderer.SetScene(body);std::vector<std::uint8_t> actual(1024*768,199);renderer.Draw(actual,1024,199);
            const auto compared=CompareDisplayNative(actual,{point.x-hotX,point.y-hotY},width,height,row[4]);CHECK(compared.first==0 && compared.second>0);pixels+=compared.second;++cases;world.Scroll(-camera.x,-camera.y);
        }
    }
    CHECK(cases==102 && pixels>100000);std::cout<<"간헐천51프레임/두 카메라 원본 대조 픽셀="<<pixels<<'\n';
}

// 실제 받침 윗면/하단의198개 원문을 두 카메라에서 대조해 소유자 색 누락과 하단 오염을 잡는다.
TEST_CASE(CampaignSession_SupportNative198FramesAndColors) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();services.install.resetCamera={};
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);
    const auto core=world.Session().Core();const auto origin=EmptySessionSupport(core);const float x=static_cast<float>(origin.x+2),y=static_cast<float>(origin.y+2);
    const auto top=world.Session().World().Create(94,2);world.Session().World().Pop(top,x,y,0);const auto bottom=world.Session().World().Find(x,y,95); // 실제 윗면 최초 Pop이 생성한 하단을 사용해 같은 하단을 중복 생성하지 않는다.
    CHECK(bottom.value);if (!bottom.value) return;world.Scroll(-4096,-4096);std::size_t cases{},pixels{};
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/support-color-capture-x86.tsv").string();
    // 새 독립 원문의 모든 입력을 재생하며 기대 픽셀을 제품의 색 계산으로 만들지 않는다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(path.c_str()).rows) {
        const auto type=static_cast<std::uint32_t>(std::stoul(row[0])),frame=static_cast<std::uint32_t>(std::stoul(row[1]));
        const auto owner=static_cast<std::uint8_t>(std::stoul(row[2]));auto raw=core.pool.AllocatedBytes(type==94 ? top : bottom);raw[34]=owner;raw[40]=128;Put(raw,36,frame);core.bridgeState.ownerColors[owner]=std::stoi(row[3]); // 소유자 번호와 현재 색 전역이 다른 경우도 원문대로 입력한다.
        const auto metadata=netstorm::test::rawscene::Split(row[4],',');
        const int width=std::stoi(metadata[0]),height=std::stoi(metadata[1]),hotX=std::stoi(metadata[2])+core.types[type].hotspotX+2,hotY=std::stoi(metadata[3])+core.types[type].hotspotY-3; // 명시 원문TYPE(-2,3)에서 실제TYPE 기준점만 환산한다.
        // 정수 카메라 이동 전후 같은 전체 픽셀을 비교한다.
        for (const auto camera:std::array<ScreenPoint,2>{{{0,0},{7,9}}}) {
            world.Scroll(camera.x,camera.y);const auto point=world.ToScreen(x,y);std::vector<RenderSprite> body;
            // 대상 받침 부품만 분리하며 본섬이나 반대 부품의 가림을 섞지 않는다.
            for (const auto& sprite:world.Sprites()) if (sprite.block==type-70 && sprite.order.x==x && sprite.order.y==y) body.push_back(sprite);
            CHECK(body.size()==1);if (body.size()!=1) { const auto& shape=assets.game.Shapes().Blocks()[type-70].frames[frame];std::cout<<"받침 대상 누락 TYPE="<<type<<" 실제TYPE="<<unsigned(raw[10])<<" frame="<<frame<<" owner="<<unsigned(owner)<<" 좌표="<<std::bit_cast<float>(Get(raw,14))<<','<<std::bit_cast<float>(Get(raw,18))<<" SID="<<(type==94 ? top.value : bottom.value)<<" flags="<<unsigned(raw[11])<<" special="<<shape.IsSpecial()<<" rect="<<shape.rect.left<<','<<shape.rect.top<<','<<shape.rect.right<<','<<shape.rect.bottom<<'\n';
                // 실패 시 실제 제출된 같은 받침 위치의 블록만 남긴다.
                for (const auto& sprite:world.Sprites()) if (sprite.order.x==x && sprite.order.y==y) std::cout<<"받침 위치 제출 block="<<sprite.block<<" frame="<<sprite.frame<<" procedure="<<sprite.procedure.has_value()<<'\n';return; }
            Renderer renderer(1024,768);renderer.SetScene(body);std::vector<std::uint8_t> actual(1024*768,199);renderer.Draw(actual,1024,199);
            const auto compared=CompareDisplayNative(actual,{point.x-hotX,point.y-hotY},width,height,row[5]);CHECK(compared.first==0 && compared.second>0);pixels+=compared.second;++cases;world.Scroll(-camera.x,-camera.y);
        }
    }
    CHECK(cases==396 && pixels>100000);std::cout<<"받침198원문/두 카메라 원본 대조 픽셀="<<pixels<<'\n';
}

// 중립 간헐천 받침에 실제 소유 다리를 접합하면 윗면/하단/아홉 논리 칸의 소유권과 그림이 갱신돼야 한다.
TEST_CASE(CampaignSession_GeyserSupportBridgeOwnershipAndColors) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();services.install.resetCamera={};
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);auto& session=world.Session();const auto core=session.Core();
    Sid geyser{},top{},bottom{}; // 저장된 중립 간헐천과 같은 좌표의 두 실제 받침을 찾는다.
    // 본섬 간헐천과 외부 받침의 간헐천을 구분한다.
    for (const auto& object:session.World().Objects()) if (object.object.type==122 && object.object.owner==0) {
        const auto candidate=session.World().Find(object.object.x,object.object.y,94);if (!candidate.value) continue;geyser=object.sid;top=candidate;bottom=session.World().Find(object.object.x,object.object.y,95);break;
    }
    CHECK(geyser.value && top.value && bottom.value);if (!geyser.value || !top.value || !bottom.value) return;
    const auto start=core.pool.Slot(top);const float x=std::bit_cast<float>(Get(start,14)),y=std::bit_cast<float>(Get(start,18));CHECK(start[34]==0 && Get(start,36)==8);
    core.bridgeState.ownerColors[1]=7; // 현재 소유자1의 실제 색 번호를 초기 MissionPlayers 사본과 다르게 둔다.
    const auto bridge=session.World().Create(82,2);core.owner.Set(bridge,1);session.World().Pop(bridge,x-3,y-1,0);session.World().BridgeConnections().Connect(bridge);
    CHECK(core.pool.Slot(top)[34]==1 && Get(core.pool.Slot(top),36)==6 && Get(core.pool.Slot(bottom),36)==6);
    // 실제 저장 받침의 논리 아홉 칸도 원본 전파로 같은 소유자가 돼야 한다.
    for (int dy=-2;dy<=0;++dy) {
        // 같은 행의 세 표면을 실제 해시에서 조회한다.
        for (int dx=-2;dx<=0;++dx) { const auto surface=core.hash.Cell(0,static_cast<int>(x)+dx,static_cast<int>(y)+dy);CHECK(surface && core.pool.Slot(Sid{surface})[34]==1); }
    }
    bool mapped=false; // 실제 다음 장면의 윗면 제출에 색표가 전달됐는지 확인한다.
    world.Scroll(-4096,-4096);world.Scroll(static_cast<int>(x*16)-512,static_cast<int>(y*11)-384); // 실제 검사 대상 받침을 화면 안에 둔 뒤 제출을 확인한다.
    // 이전 그림 캐시가 아니라 현재 raw 소유자의 장면을 조회한다.
    for (const auto& sprite:world.Sprites()) if (sprite.block==94-70 && sprite.order.x==x && sprite.order.y==y) mapped=sprite.colors.has_value();
    CHECK(mapped);
}

// 실제 다리60프레임/중립·아홉 색600원문의 좌표와 색을 두 카메라의 화면에서 대조한다.
TEST_CASE(CampaignSession_BridgeNative600FramesAndColors) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();services.install.resetCamera={};
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);
    const auto core=world.Session().Core();const auto origin=EmptySessionSupport(core);const float x=static_cast<float>(origin.x),y=static_cast<float>(origin.y);
    const auto sid=world.Session().World().Create(82,2);core.owner.Set(sid,1);world.Session().World().Pop(sid,x,y,0);world.Scroll(-4096,-4096);auto raw=core.pool.AllocatedBytes(sid);std::size_t cases{},pixels{};
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bridge-extra-capture-x86.tsv").string();CHECK(core.types[82].hotspotX==0 && core.types[82].hotspotY==2);
    // 같은 실제 원문은 재생성하지 않고 현재 소유자 색 전역만 명시 입력으로 바꾼다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(path.c_str()).rows) {
        const auto owner=static_cast<std::uint8_t>(std::stoul(row[1]));raw[34]=owner;core.bridgeState.ownerColors[owner]=std::stoi(row[2]);Put(raw,36,static_cast<std::uint32_t>(std::stoul(row[0])));
        std::array<std::uint8_t,16> metadata{};
        // 원문의 네 little endian DWORD를 그대로 읽는다.
        for (std::size_t index=0;index<metadata.size();++index) metadata[index]=DisplayNativeByte(row[3],index);
        const int width=static_cast<int>(Get(metadata,0)),height=static_cast<int>(Get(metadata,4)),hotX=std::bit_cast<std::int32_t>(Get(metadata,8)),hotY=std::bit_cast<std::int32_t>(Get(metadata,12))+2; // 원문TYPE(0,0)에 실제 다리의0,2만 더한다.
        // 각 원문을 두 정수 카메라에서 다시 표시한다.
        for (const auto camera:std::array<ScreenPoint,2>{{{0,0},{7,9}}}) {
            world.Scroll(camera.x,camera.y);const auto point=world.ToScreen(x,y);std::vector<RenderSprite> body;
            // 원본0단계의 실제 다리 한 개를 선택하며 연결 조각/다른 지면을 중복하지 않는다.
            for (const auto& sprite:world.Sprites()) if (sprite.block==82-70 && sprite.order.x==x && sprite.order.y==y) body.push_back(sprite);
            CHECK(body.size()==1);if (body.size()!=1) return;
            Renderer renderer(1024,768);renderer.SetScene(body);std::vector<std::uint8_t> actual(1024*768,199);renderer.Draw(actual,1024,199);
            const auto compared=CompareDisplayNative(actual,{point.x-hotX,point.y-hotY},width,height,row[4]);CHECK(compared.first==0 && compared.second>0);pixels+=compared.second;++cases;world.Scroll(-camera.x,-camera.y);
        }
    }
    CHECK(cases==1200 && pixels>100000);std::cout<<"다리600원문/두 카메라 원본 대조 픽셀="<<pixels<<'\n';
}

// 실제 raw 발사체/장식물의 원본 깊이와 겹친 픽셀이 GameWorld와 Renderer까지 보존돼야 한다.
TEST_CASE(CampaignSession_MissileFringeDepthAndOverlap) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services();services.install.resetCamera={};
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);
    auto& session=world.Session();auto core=session.Core();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};
    const float x=std::floor(std::bit_cast<float>(Get(core.pool.Slot(priest),14)))+0.5625f,y=std::floor(std::bit_cast<float>(Get(core.pool.Slot(priest),18)))+0.7490844727f; // 저장 실전 장면의 상대 픽셀 배치를 현재 시야로 평행 이동한 입력이다.
    const auto missile=session.World().Create(132,2),fringe=session.World().Create(102,2);
    Put(core.pool.AllocatedBytes(fringe),36,67); // 새 void 슬롯의 초기 프레임 입력이다. 일반 Set의 공간 재등록을 먼저 실행하지 않는다.
    session.World().Pop(missile,x,y,0);session.World().Pop(fringe,x+0.4375f,y+2.2509155273f,0);
    CHECK(std::bit_cast<std::int8_t>(core.pool.Slot(missile)[35])==-25 && core.pool.Slot(fringe)[35]==20);
    std::vector<RenderSprite> pair; // 다른 실제 그림의 가림을 제외하고 이 두 제출 항목의 순서를 확인한다.
    // 동일 블록과 현재 좌표로 실제 제출된 두 그림을 찾는다.
    for (const auto& sprite:world.Sprites()) {
        if ((sprite.block==132-70 && sprite.order.x==x && sprite.order.y==y) ||
            (sprite.block==102-70 && sprite.frame==67 && sprite.order.x==x+0.4375f && sprite.order.y==y+2.2509155273f)) pair.push_back(sprite);
    }
    CHECK(pair.size()==2);if (pair.size()!=2) return;
    const auto shot=std::find_if(pair.begin(),pair.end(),[](const auto& sprite) { return sprite.block==132-70; });
    const auto decoration=std::find_if(pair.begin(),pair.end(),[](const auto& sprite) { return sprite.block==102-70; });
    CHECK(shot->order.depth==-25 && decoration->order.depth==20 && CompareDrawOrder(decoration->order,shot->order)<0);
    Renderer renderer(1024,768);renderer.SetScene({*shot});const auto expected=renderer.SceneImage();renderer.SetScene(pair);const auto actual=renderer.SceneImage();
    const auto shotPixels=assets.game.Shapes().Decode(shot->block,shot->frame),decorPixels=assets.game.Shapes().Decode(decoration->block,decoration->frame);
    const auto shotRect=assets.game.Shapes().Blocks()[shot->block].frames[shot->frame].rect,decorRect=assets.game.Shapes().Blocks()[decoration->block].frames[decoration->frame].rect;
    std::size_t overlapping=0; // 양쪽 RLE에서 모두 불투명한 실제 화면 픽셀 수다.
    // 발사체의 불투명 픽셀을 실제 화면 좌표로 옮겨 장식물과 겹치는 부분을 검사한다.
    for (std::size_t row=0;row<shotPixels.height;++row) {
        // 행 안의 각 픽셀을 확인하며 투명 번호와 불투명 마스크를 구분한다.
        for (std::size_t column=0;column<shotPixels.width;++column) {
            if (!shotPixels.opacity[row*shotPixels.width+column]) continue;
            const int px=shot->x+shotRect.left+static_cast<int>(column),py=shot->y+shotRect.top+static_cast<int>(row);
            const int dx=px-decoration->x-decorRect.left,dy=py-decoration->y-decorRect.top;
            if (px<0 || px>=1024 || py<28 || py>=768 || dx<0 || dy<0 || dx>=static_cast<int>(decorPixels.width) || dy>=static_cast<int>(decorPixels.height)) continue;
            if (!decorPixels.opacity[dy*decorPixels.width+dx]) continue;
            ++overlapping;CHECK(actual.indices[py*1024+px]==expected.indices[py*1024+px]);
        }
    }
    CHECK(overlapping>0);std::cout<<"발사체/장식물 원본 깊이 -25/20, 겹침 픽셀="<<overlapping<<'\n';
}

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
    auto previousX=std::bit_cast<float>(initial[0]),previousY=std::bit_cast<float>(initial[1]); // 직전 실제 raw 좌표다.
    std::size_t movementUpdates=0;float largestStep=0; // 도착 때 한 번 이동하는 오류를 검출할 연속 관측이다.
    // 실제 현재 Kernel을 실행하고 화면 뷰도 같은 슬롯의 현재 위치를 따라가는지 확인한다.
    for (std::uint32_t frame=0;frame<480;++frame) {
        world.Step(1.0/60.0);
        // 현재 실제 내 사제의 부모 Form 체인에서 살아 있는54를 조회한다.
        for (auto form=static_cast<std::uint16_t>(Get(core.pool.Slot(priest),6,2));form;form=static_cast<std::uint16_t>(Get(core.pool.Slot(Sid{form}),4,2)))
            if (core.pool.Slot(Sid{form})[10]==54) path=true;
        const auto raw=core.pool.Slot(priest);if (Get(raw,14)!=initial[0] || Get(raw,18)!=initial[1]) moved=true;
        const auto currentX=std::bit_cast<float>(Get(raw,14)),currentY=std::bit_cast<float>(Get(raw,18)); // 같은 보고 시점의 위치다.
        if (currentX!=previousX || currentY!=previousY) {
            ++movementUpdates;largestStep=std::max(largestStep,std::hypot(currentX-previousX,currentY-previousY));previousX=currentX;previousY=currentY;
        }
        CHECK(world.Object(priest.value)->x==std::bit_cast<float>(Get(raw,14)) && world.Object(priest.value)->y==std::bit_cast<float>(Get(raw,18)));
    }
    CHECK(path && moved && core.bookkeeping.depth==0);
    CHECK(movementUpdates>10 && largestStep<0.75f);
    std::cout<<"실제 사제 이동 갱신="<<movementUpdates<<" 최대 한 걸음="<<largestStep<<'\n';
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
    // 벌룬/에어십과 전용 주기 애니메이션을 가진 클라우드 플로터를 각각 새 세션에서 검증한다.
    for (const std::uint32_t kind:{111U,141U,146U}) {
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


// 설치 전 Init 표와 설치 후 Session 소비자가 같은 배열을 읽고 쓴다. 복사한 표를 기대값으로 주입하지 않는다.
TEST_CASE(CampaignSession_InitSharedTablesBeforeInstall) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();
    auto options=std::make_shared<std::array<std::uint8_t,87>>();auto restrictions=std::make_shared<std::array<std::uint32_t,256>>(); // 현재 Client 전역과 같은 별도 소유 수명이다.
    ResetMissionBattleOptions(OriginalEdition::Patch1078,*options);(*options)[23]=0x79;(*restrictions)[75]=1;restrictions->back()=0x7391;services.missionOptions=options;services.missionRestrictions=restrictions;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(session.BattleOption(2)==2 && session.BattleOption(23)==0x79 && !session.TotalMade().Allowed(75));
    (*options)[2]=1;CHECK(session.BattleOption(2)==1);session.TotalMade().Allow(75,1);CHECK((*restrictions)[75]==0); // 양방향의 현재 참조를 확인한다.
    session.Load();CHECK(session.BattleOption(2)==1 && session.BattleOption(23)==0x79);session.TotalMade().AllowAll(0);CHECK((*restrictions)[75]==1 && restrictions->back()==0x7391);(*restrictions)[75]=0;CHECK(session.TotalMade().Allowed(75));
}

// Session이 해제돼도 Client 소유의 실제 Template/State는 명시 Init 정리 전까지 살아 있다.
TEST_CASE(CampaignSession_TemplateSurvivesUntilExplicitInitClear) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto storage=std::make_shared<CampaignTemplateStorage>();const std::uint8_t* stateAddress{}; // 실제 Client와 같은 외부 소유 수명이다.
    {
        auto services=external.Services();UseWorldTestLogin(services);services.templateStorage=storage;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.LoadLocalConnection();CHECK(session.LocalConnectionPhase()==14 && storage->OwnedImages()==1);
        stateAddress=storage->State(1).bytes.data();CHECK(stateAddress==storage->Template(1).Section("State").data() && (storage->Template(1).Section("State")[0]&2U));
    }
    CHECK(storage->OwnedImages()==1 && stateAddress==storage->Template(1).Section("State").data());storage->ClearAll();CHECK(!storage->OwnedImages() && Get(storage->Headers(),20)==0);
}

// Init에서 소유한 Zone의 subscriber7을 실제 로그인에 사용하고 설치 뒤 같은 이미지의 수명을 반납한다.
TEST_CASE(CampaignSession_InitRequestUsedByActualLogin) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);
    auto request=std::make_shared<MissionFortLoadRequest>();std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};std::uint32_t port=6799;MissionFortRequestHooks effects; // 실제 Client와 같은 단일 요청 소유권이다.
    effects.selectFort=[request](auto name) { request->fort=name; };effects.duplicate=[request](auto& packet) { return CaptureMissionFortZone(*request,packet); };WriteMissionFortRequest("thewarbegins",{request->next,request->sequence,address,port},effects);
    Put(request->zone,16,7);services.missionFortRequest=request;services.missionSubscriber=7; // 명시 입력 Zone의 현재 subscriber를 바꿔 새 Zone을 만들면 실제 접속이 실패하게 한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),request->fort,std::move(services));CHECK(!request->consumed && request->zone.size()==86);session.LoadLocalConnection();
    CHECK(session.CurrentSubscriber()==7 && session.LocalOwner()==1 && session.LocalConnectionPhase()==14 && Get(session.PlayerRecords(),0xb4+0x2c)==7);CHECK(request->consumed && request->zone.empty() && request->zone.capacity()==0 && request->next[0]==7 && request->sequence==1);
}

// 실제 다리 시작점/예산 검색이 자연 재고의 배치·소비·현재 재고 스캔까지 이어지는지 확인한다.
TEST_CASE(CampaignSession_AiBridgeProgressUsesActualWorldAndContents) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);
    services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.localTransport=[] { return true; };services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));RawPathPoint scratch{-10000,-10000};CHECK(Throws([&] { session.AdvanceAiBridge(2,{120,120},scratch); }));session.LoadLocalConnection();CHECK(!session.AdvanceAiBridge(2,{120,120},scratch));
    auto profile=SessionAiSettings("Bridge AI","sunWalker;sunCannon");Put(profile,0x238,4);const auto token=session.CreateAi(2,profile);CHECK(session.InitializeAi(2) && session.PrepareAi(2));auto core=session.Core();const auto body=session.AiMemory().Bytes(token);const auto before=Get(body,0x628),wallet=Get(body,0x2d8);const auto sp=session.Sp(2);CHECK(before==4 && Get(body,0x604)==4);
    const auto parent=Sid{static_cast<std::uint16_t>(Get(session.PlayerRecords(),2*180+0x9c))};RawContainedFinder finder(core.pool,session.ProductionFinder());std::vector<Sid> oldPieces;std::map<std::uint16_t,std::uint32_t> oldPatterns; //삭제 전 실제 패턴만 보관해 반납된 슬롯을 다시 읽지 않는다.
    for (auto form=finder.Begin(parent);form.value;form=finder.Next()) { const auto raw=core.pool.Slot(form);if ((Get(raw,20,2)&0x140)==0x40) { oldPieces.push_back(form);oldPatterns.emplace(form.value,Get(raw,12,2)); } } //현재 기술부모가 실제로 소유한 준비 조각만 관측한다.
    const auto enemy=Sid{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(enemy.value!=0);const auto enemyRaw=core.pool.Slot(enemy);const RawPathPoint target{std::bit_cast<float>(Get(enemyRaw,14)),std::bit_cast<float>(Get(enemyRaw,18))};
    const auto bridgeCells=[&] { std::size_t count{};for (const auto sid:core.hash.Entries(0)) { if (!sid) continue;const auto raw=core.pool.Slot(Sid{sid});if (raw[10]==82 && raw[34]==2) ++count; }return count; }; //현재실제hash0에서owner2다리칸을세며생성을강제하지않는다.
    const auto cellsBefore=bridgeCells();bool pending{},consumed{};std::size_t calls{};
    for (;calls<120 && !consumed;++calls) { const auto stage=Get(body,0x5f0);CHECK(session.AdvanceAiBridge(2,target,scratch));if (stage==3 && Get(body,0x5f0)==3) pending=true;consumed=Get(body,0x628)<before; } //실제상태전이와예산중단을재개해첫조각소비까지진행한다.
    CHECK(pending && consumed && Get(body,0x628)==before-1 && Get(body,0x5f0)==0);CHECK(Get(body,0x2d8)==wallet && session.Sp(2)==sp);
    std::vector<Sid> currentPieces;for (auto form=finder.Begin(parent);form.value;form=finder.Next()) if ((Get(core.pool.Slot(form),20,2)&0x140)==0x40) currentPieces.push_back(form); //배치뒤실제내용물체인을다시읽어첫일치소비를확인한다.
    CHECK(currentPieces.size()+1==oldPieces.size());const auto selected=Get(body,0x6b4);bool removedMatch{};
    for (const auto sid:oldPieces) { if (std::find(currentPieces.begin(),currentPieces.end(),sid)==currentPieces.end() && oldPatterns.at(sid.value)==selected) removedMatch=true; } //원본선택패턴의실제기존조각이체인에서제거됐는지관측한다.
    CHECK(removedMatch);session.RunFrame({0.1,0.1,0.1,1},false);const auto cellsAfter=bridgeCells();std::printf("실제AI다리 호출%zu 재고%u→%u 패턴%u 회전%u 배치(%g,%g) 끝점(%g,%g) 다리칸%zu→%zu\n",calls,before,Get(body,0x628),selected,Get(body,0x6b8),std::bit_cast<float>(Get(body,0x6ac)),std::bit_cast<float>(Get(body,0x6b0)),std::bit_cast<float>(Get(body,0x3d0)),std::bit_cast<float>(Get(body,0x3d4)),cellsBefore,cellsAfter);CHECK(cellsAfter>cellsBefore);
    CHECK(Throws([&] { session.AdvanceAiBridge(0,target,scratch); }));session.RemoveAi(2);CHECK(!session.AdvanceAiBridge(2,target,scratch));
}

// 수동AI행동API없이현재게임프레임만으로초기화/준비/전략/다리/수집자를실행한다.
TEST_CASE(CampaignSession_AutomaticAiFramesUseCurrentTimeAndWorld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.missionOutcome.demo=[] { return 0U; };services.ai.assertion=[](std::string_view text) { throw std::logic_error(std::string(text)); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.LoadLocalConnection();auto profile=SessionAiSettings("Automatic AI","sunWalker;sunCannon");Put(profile,0x22c,5000);Put(profile,0x238,4);const auto token=session.CreateAi(2,profile);const auto body=session.AiMemory().Bytes(token);CHECK(!Get(body,0x258) && !Get(body,0x25c));auto core=session.Core();
    const auto bridgeCells=[&] { std::size_t count{};for (const auto sid:core.hash.Entries(0)) { if (!sid) continue;const auto raw=core.pool.Slot(Sid{sid});if (raw[10]==82 && raw[34]==2) ++count; }return count; }; //실제현재지도에서AI다리칸을세며생성결과를직접만들지않는다.
    const auto before=bridgeCells();session.RunFrame({0.0,0.0,0.0,0},true);CHECK(!Get(body,0x258) && !Get(body,0x25c));
    for (int frame=1;frame<=240;++frame) { const auto now=frame*0.1;session.PumpLocalConnection();session.RunFrame({now,now,0.1,static_cast<std::uint64_t>(frame)},false); } //실제자동AI→같은월드Kernel을24초진행한다.
    const auto after=bridgeCells();std::printf("자동AI 초기화%u 준비%u 목표%u 재고%u 다리%zu→%zu 수집자%u SP%g 지갑%u\n",Get(body,0x258),Get(body,0x25c),Get(body,0x2d4),Get(body,0x628),before,after,Get(body,0x2e4),session.Sp(2),Get(body,0x2d8));CHECK(Get(body,0x258)==1 && Get(body,0x25c)==1 && Get(body,0x2d4)>=1 && Get(body,0x2d4)<=4);CHECK(after>before && Get(body,0x2e4)>0);CHECK(session.Sp(2)==static_cast<float>(std::bit_cast<std::int32_t>(Get(body,0x2d8))));
}
// 실제프로필종료값은정지중유지하고다음비정지프레임에서동일소유자의본문/목록만반납한다.
TEST_CASE(CampaignSession_AutomaticAiTerminationReleasesOwnedMemory) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.LoadLocalConnection();auto profile=SessionAiSettings("Terminating AI","sunWalker;sunCannon");Put(profile,0x254,1);const auto token=session.CreateAi(2,profile);const auto flags=session.PlayerRecords()[2*180+0x50];const auto before=session.AiMemory().Frees().size();session.RunFrame({0,0,0,0},true);CHECK(session.AiMemory().Contains(token) && session.Participation().Brain(2)==token);session.RunFrame({0.1,0.1,0.1,1},false);CHECK(!session.AiMemory().Contains(token) && !session.Participation().Brain(2) && session.PlayerRecords()[2*180+0x50]==flags && session.AiMemory().Frees().size()>before);
}

//성공flag를입력하지않고실제Zone→접속→Login→월드설치/응답→상태14/타이머를검사한다.
TEST_CASE(CampaignSession_LocalLoginActualHandshakeAndWorldLoad) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::vector<std::string> trace;std::vector<std::array<std::uint32_t,3>> notices;std::map<std::string,std::int32_t> config;std::size_t refresh{},chat{}; //실제장치/설정의명시입력과관측이다.
    CampaignLoginServices login;login.configInt=[&](std::string_view key,std::int32_t fallback) { const auto at=config.find(std::string(key));return at==config.end() ? fallback : at->second; };login.wallSeconds=[] { return 100.25; };login.clock=[] { return 100250U; };login.refresh=[&] { ++refresh; };login.playerUiBusy=[] { return false; };login.playerNotice=[&](std::uint32_t kind,std::uint32_t owner,std::uint32_t value) { notices.push_back({kind,owner,value}); };login.observePacket=[](std::uint32_t sender,bool boss,std::uint32_t local,std::span<const std::uint8_t> packet) { if (packet[1]==0x6e) std::cout<<"Ready 실제수신 sender"<<sender<<" boss"<<boss<<" local"<<local<<" mask"<<Get(packet,4)<<'\n'; };
    login.log=[&](std::string_view text) { trace.emplace_back(text); };services.localLogin=std::move(login);
    services.ai.participation.clearChat=[&](std::uint32_t channel) { CHECK(channel==0x40 || channel==0x80);++chat; };services.ai.participation.addChat=[&](std::uint32_t subscriber,std::string_view,std::uint32_t channel) { CHECK(channel==0x20 || channel==0x40 || channel==0x80);static_cast<void>(subscriber);++chat; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(!session.CurrentSubscriber() && !session.LocalConnectionPhase() && session.LocalConnectionPackets().empty());CHECK(!(session.PlayerRecords()[0xb4+0x50]&0x43));session.LoadLocalConnection();auto core=session.Core();CHECK(session.CurrentSubscriber()==1 && session.LocalOwner()==1 && session.PlayerKnown()==1 && session.LocalConnectionPhase()==14);CHECK((session.PlayerRecords()[0xb4+0x50]&0x43)==0x43 && session.FindPriestParticipation(1));
    const auto packets=session.LocalConnectionPackets();CHECK(packets.size()>8 && packets.front()==0x27 && std::find(packets.begin(),packets.end(),0x29)!=packets.end() && std::find(packets.begin(),packets.end(),0x28)!=packets.end() && packets.back()==0x6e);CHECK(refresh && chat && !notices.empty());
    // 첫 원본 옵션 초기화/파생 관측의 실제22개 기본값과 동일한0x67 수신 후 현재 저장소를 대조한다.
    constexpr std::array<std::uint8_t,22> originalDefaults{0,2,2,0,3,0,2,1,1,0,0,1,0,1,1,2,2,1,2,2,0,0};
    // 성공/값을 주입하지 않고 실제 Session 로그인/옵션 패킷이 남긴 바이트를 읽는다.
    for (std::size_t index=0;index<originalDefaults.size();++index) CHECK(session.BattleOption(index)==originalDefaults[index]);
    CHECK(session.EnergySettings().radiusSquared==900 && core.types[122].cost==3000); // 기본3번 범위38은 원본 최대30으로 제한되고 간헐천 비용은 옵션2의3000이다.
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

// 실제 로컬 로그인 뒤 Start의 현재 설정/자금/기술/AI와3d 동맹 송수신이 같은 Session을 사용하는지 확인한다.
TEST_CASE(CampaignSession_LocalLoginMissionStartAiAlliances) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    // 일반 미션과 StartDemo는 같은 보호 요새를 사용하지만 로컬 AI 포함 여부가 다르다.
    for (const std::uint32_t demo:{0U,1U}) {
        ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
        config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
        MissionScript script(config,"GuardMyBack");CHECK(script.Loaded());SessionServicesFixture external;auto services=external.Services();std::size_t receivedAlliances{},chat{},refresh{},commands{},cameras{},settings{}; // 실제 경계의 호출 수만 관찰한다.
        CampaignLoginServices login;login.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };login.wallSeconds=[] { return 100.25; };login.clock=[] { return 100250U; };login.refresh=[&] { ++refresh; };login.playerUiBusy=[] { return false; };login.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};login.log=[](std::string_view) {};
        login.observePacket=[&](std::uint32_t,bool,std::uint32_t,std::span<const std::uint8_t> packet) { if (packet[1]==0x3d) { CHECK(packet.size()==9 && packet[0]==1 && packet[2]==9);++receivedAlliances; } };
        services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),static_cast<std::uint8_t>(0xa7)); }; // 실제 생성자 미정 바이트의 명시 검사 입력이다.
        services.ai.defaultName=[] { return std::string("AI"); };services.localLogin=std::move(login);services.ai.participation.clearChat=[&](std::uint32_t) { ++chat; };services.ai.participation.addChat=[&](std::uint32_t,std::string_view,std::uint32_t) { ++chat; };
        auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.LoadLocalConnection();CHECK(session.LocalConnectionPhase()==14 && session.Sp(1)==0);
        MissionFrameGateState gate;gate.demo=demo;MissionStartState state;CampaignMissionStartEffects effects;
        effects.settingsBytes=[&](std::uint32_t,std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),static_cast<std::uint8_t>(0xa7));++settings; };effects.gameSeconds=[] { return 123.5; };effects.refreshSp=[&] { ++refresh; };effects.refreshCommandGump=[&] { ++commands; };effects.resetCamera=[&] { ++cameras; };effects.randomGeysers=[] { throw std::logic_error("보호 GuardMyBack에 없는 세계 랜덤 간헐천 입력"); };
        RawCampaignMissionStartBindings start(session,script,gate,state,session.MissionStartSettings(),session.MissionAiSettings(),std::move(effects));start.Start();
        CHECK(gate.started==1 && state.sequence==1 && gate.demo==demo && gate.startSeconds==123.5 && gate.demoEndSeconds==483.5 && commands==1 && cameras==1 && settings>=3);
        CHECK(session.Participation().Brain(2)!=0 && session.Participation().Brain(3)!=0 && session.Participation().Brain(4)!=0 && (session.Participation().Brain(1)!=0)==(demo!=0));
        CHECK(session.Sp(1)==1200 && session.MissionStartSettings().ordinaryGeysers==1 && session.FindTechnology(1,133).value!=0);CHECK(receivedAlliances==0); // 실제 송신은 아직 flush하지 않은 상태다.
        const auto before=session.LocalConnectionPackets().size();session.PumpLocalConnection();CHECK(session.LocalConnectionPhase()==14 && receivedAlliances>0 && session.LocalConnectionPackets().size()>before && chat>0);
        CHECK(session.Alliances()[2*9+2]==1 && session.Alliances()[3*9+3]==1 && session.Alliances()[4*9+4]==1 && session.Core().bookkeeping.depth==0);
    }
}

namespace {
// 같은 실제 로그인/건설/연구/생산으로 기본 회귀·제네레이터 연장·forward=true의 전진 공략을 검사한다.
void NaturalWorkshopPlacement(bool combat,bool burning=false,CellPoint cannonLocation={118,118},bool forward=false,bool reclaim=false) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();RawButtonControls* activeProductionUi{}; // 같은 실제 내용물 알림의 현재 UI다.
    std::vector<std::array<float,2>> bridgeSounds;auto previousSound=std::move(services.sound.playOnceAt); // 실제 위치 소리 요청을 기존 장치 경계와 함께 관찰한다.
    services.sound.playOnceAt=[&external,&bridgeSounds,previous=std::move(previousSound)](float x,float y,SoundHandle sound,std::uint32_t first,std::uint32_t second) {
        if (sound==external.names.Lookup("bridgeFall.wav")) { CHECK(first==0 && second==0);bridgeSounds.push_back({x,y}); }return previous(x,y,sound,first,second);
    };
    services.playerContent.gump=[&](auto id) { return activeProductionUi ? activeProductionUi->RegisteredGump(id) : 0U; };services.playerContent.handle=[&](auto token,auto event,auto form,auto reason) { if (!activeProductionUi) throw std::logic_error("생산 검사 UI 수명 오류");return activeProductionUi->HandleGump(token,event,form,reason); };services.gumpNotice=[&](auto id,auto parent) { if (activeProductionUi) activeProductionUi->ObjectNotice()(id,parent); };
    CampaignLoginServices login;login.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };login.wallSeconds=[] { return 100.25; };login.clock=[] { return 100250U; };login.refresh=[] {};login.playerUiBusy=[] { return false; };login.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};login.log=[](std::string_view) {};
    services.production.allocationBytes=[](std::span<std::uint8_t> bytes) { CHECK(bytes.size()==kProductionDeckBytes);std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); }; // 생성자가 초기화하지 않는 실제 할당 바이트의 명시 입력이다.
    services.production.effects.configInt=[](std::string_view key) { CHECK(key=="getTeleportTechFree");return 0; };services.production.effects.modeAssertion=[] { throw std::logic_error("실제 연구의 Fort/Battle 모드 진단"); };
    RawCampaignSession* activeSession{};std::vector<std::vector<std::uint8_t>> researchPackets; // 실제 송신과58h 수신 원문의 관측이다.
    services.carrierRequest=[&](std::int32_t target,std::span<const std::uint8_t> packet) { CHECK(activeSession && target==9 && ((packet.size()==16 && packet[1]==0x58) || (reclaim && packet.size()==8 && packet[1]==0x54)));if (!activeSession) throw std::logic_error("연구 실제 전송 미설치");activeSession->SendLocalPacket(target,packet); };
    login.observePacket=[&](std::uint32_t,bool,std::uint32_t,std::span<const std::uint8_t> packet) { if (packet[1]==0x58) researchPackets.emplace_back(packet.begin(),packet.end()); };
    services.localLogin=std::move(login);services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };services.ai.defaultName=[] { return std::string("AI"); };services.ai.participation.clearChat=[](std::uint32_t) {};services.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");MissionScript script(config,"TheWarBegins");
    auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));activeSession=&session;session.LoadLocalConnection();
    MissionFrameGateState gate;MissionStartState state;CampaignMissionStartEffects effects;effects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };effects.gameSeconds=[] { return 123.5; };effects.refreshSp=[] {};effects.refreshCommandGump=[] {};effects.resetCamera=[] {};effects.randomGeysers=[] { throw std::logic_error("보호1-1에없는세계랜덤간헐천"); };
    RawCampaignMissionStartBindings start(session,script,gate,state,session.MissionStartSettings(),session.MissionAiSettings(),std::move(effects));start.Start();CHECK(session.Sp(1)==3000 && gate.started==1);
    SessionCursorUi ui;auto cursorEffects=ui.Effects();cursorEffects.send=[&](std::uint32_t target,std::span<const std::uint8_t> packet) { CHECK(target==9 && packet.size()==25 && packet[1]==0x3e);session.SendLocalPacket(static_cast<std::int32_t>(target),packet); };session.Cursor().SetEffects(std::move(cursorEffects));session.Cursor().SnapToGrid()=1;SessionPriestMenuUi menuUi;auto menu=CursorMenu(session,menuUi);const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(menu->SelectBuild(85,priest));
    const auto point=combat ? CellPoint{109,107} : CursorPlacementPoint(session,ui);if (combat) { session.SetParticleView({0,0,0,0,1024,768},{});ui.Point(session,point,0,0,false); }std::cout<<"실제1-1 썬 워크샾 배치 허용점 "<<point.x<<','<<point.y<<"\n";CHECK(session.CursorDropSettings().legal && session.Sp(1)==3000);
    const auto before=session.LocalConnectionPackets().size();session.Cursor().Drop();CHECK(session.Sp(1)==2200 && ui.shown==1 && session.LocalConnectionPackets().size()==before);
    session.PumpLocalConnection();CHECK(session.LocalConnectionPhase()==14 && session.LocalConnectionPackets().size()>before && std::find(session.LocalConnectionPackets().begin()+static_cast<std::ptrdiff_t>(before),session.LocalConnectionPackets().end(),std::uint8_t{0x3e})!=session.LocalConnectionPackets().end());session.Cursor().Cancel();
    auto core=session.Core();Sid target{}; // 실제3e 수신이 만든 같은 소유자/위치의 서버 워크샾을 조회한다.
    // 풀의 실제 서버 영역을 읽으며 새 객체·완공 상태를 검사 코드에서 만들지 않는다.
    for (std::uint32_t number=core.pool.Layout().serverFirst;number<core.pool.Capacity();++number) {
        const Sid sid{static_cast<std::uint16_t>(number)};const auto raw=core.pool.Slot(sid);
        if (!(raw[11]&3) && raw[10]==85 && raw[34]==1 && std::bit_cast<float>(Get(raw,14))==point.x && std::bit_cast<float>(Get(raw,18))==point.y) { CHECK(!target.value);target=sid; }
    }
    CHECK(target.value && (core.pool.Slot(target)[40]&1));if (!target.value) return;
    RawCampaignFactoryDisplay display(core,assets.game,[&session](Sid sid,const auto& frame) { session.DrawTemple(sid,frame); });std::array<ScreenColor,256> palette{}; // 실제 Session의 같은 풀/SHP를 그리는 제품 표시기다.
    // 명시 파일 팔레트만 설치하며 HP/프레임/완공 필드는 쓰지 않는다.
    for (std::size_t i=0;i<palette.size();++i) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(i));palette[i]={color.blue,color.green,color.red,0}; }
    display.SetPalette(palette);CHECK(display.Handles(target) && display.Handles(priest)); // 사제도 정확한 TYPE/VT의 일반 원본 Draw에 포함된다.
    // 실제 공간 수집의 본체/그림자 두 항목과 dirty 출력이 현재 슬롯의 건설 상태를 따르는지 확인한다.
    const auto render=[&](bool dead) {
        const ScreenRect clip{0,28,1024,768};const int cameraX=static_cast<int>(point.x*16)-512,cameraY=static_cast<int>(point.y*11)-384;
        const auto sprites=display.Sprites(clip,cameraX,cameraY,session.LocalOwner());std::size_t count{};std::uint64_t hash=1469598103934665603ULL;
        // 같은 실제 배치점의 항목만 검사하고 다른 워크샾 출력과 혼합하지 않는다.
        for (const auto& sprite:sprites) if (sprite.order.x==point.x && (sprite.order.y==point.y || sprite.order.y==static_cast<float>(static_cast<double>(point.y)-0.1))) {
            CHECK(sprite.procedure && !sprite.database && sprite.order.depth==std::bit_cast<std::int8_t>(core.pool.Slot(target)[35]));if (!sprite.procedure) continue;
            const bool shadow=sprite.order.y!=point.y;std::vector<std::uint8_t> pixels(1024*768,255);sprite.procedure->draw(pixels,1024,768,clip);
            const auto changed=std::count_if(pixels.begin(),pixels.end(),[](auto pixel) { return pixel!=255; });CHECK((changed>0)==(!shadow || !dead));
            // 헤더의 메뉴 영역은 실제 dirty clip 밖이므로 어떤 상태에서도 덮지 않는다.
            CHECK(std::all_of(pixels.begin(),pixels.begin()+28*1024,[](auto pixel) { return pixel==255; }));
            // 자연 완공 전후의 실제 픽셀 관측만 비교하며 기대 그림을 계산하지 않는다.
            for (const auto pixel:pixels) { hash^=pixel;hash*=1099511628211ULL; }++count;
        }
        CHECK(count==2);return hash;
    };
    const auto placedPixels=render(true);bool renderedProgress{};
    bool started{},progress{},completed{};double began{},end{};Sid construction{}; // 실제 자연 과정에서 관찰한 필드다.
    const auto origin=core.processState.now;const auto maximum=session.HitPoints().MaxHitPoints(target); // 현재 실제 TYPE/HP 처리기다.
    // 시간만 전진시켜 같은 Kernel/Regular/경로/건설 몸체가 시작과 완료를 선택하도록 한다.
    for (std::uint64_t frame=1;frame<=7200 && !completed;++frame) {
        const auto now=origin+static_cast<double>(frame)/60;session.RunFrame({now,now,1.0/60,frame},false);
        const auto form=FindAttachedProcess(core.pool,target,30);
        if (form.value && !started) {
            auto* process=dynamic_cast<ConstructionProcess*>(core.processes.FromForm(form));CHECK(process);if (!process) return;
            started=true;construction=form;began=process->Fields().start;end=process->Fields().end;
            CHECK(end==began+10 && FindAttachedProcess(core.pool,target,17).value && FindAttachedProcess(core.pool,target,47).value);
        }
        if (started) {
            const auto hp=static_cast<std::int32_t>(Get(core.pool.Slot(target),26));if (hp>0 && hp<maximum) progress=true;
            if (progress && !renderedProgress) { CHECK(render(true)==placedPixels);renderedProgress=true; } // 본체는 원본 건설 색, 그림자는 원본 생략을 유지한다.
            completed=!(core.pool.Slot(target)[40]&1);
            if (completed) CHECK(now>=end && !form.value && !core.processes.FromForm(construction));
        }
    }
    CHECK(renderedProgress && render(false)!=placedPixels);
    CHECK(started && progress && completed && end>began);CHECK(Get(core.pool.Slot(target),26)==static_cast<std::uint32_t>(maximum));
    CHECK(!FindAttachedProcess(core.pool,target,30).value && !FindAttachedProcess(core.pool,target,47).value);
    CHECK((core.pool.Slot(target)[11]&7)==0 && session.Sp(1)==2200 && !core.bookkeeping.depth);
    CHECK(!session.Production().Contains(128));const auto packetCount=session.LocalConnectionPackets().size();
    const auto liveFlares=[&] {
        std::vector<Sid> result;
        // 실제 client 풀에서 살아 있는106만 조회하며 생성 수/수명을 쓰지 않는다.
        for (std::uint16_t number=5;number<15000;++number) { const Sid sid{number};const auto raw=core.pool.Slot(sid);if (!(raw[11]&3U) && raw[10]==106) result.push_back(sid); }return result;
    };
    CHECK(liveFlares().empty());CHECK(session.SelectFactoryKnowledge(target,0x03000080U,(128U<<16)|target.value)==1);
    const auto prediction=session.Priests().Carrier().FindContained(target,16,128,0);CHECK(prediction.value>=5 && prediction.value<15000 && session.Production().Contains(128));CHECK(liveFlares().size()==6 && session.LocalConnectionPackets().size()==packetCount && researchPackets.empty());
    session.PumpLocalConnection();const auto authoritative=session.Priests().Carrier().FindContained(target,16,128,0);CHECK(authoritative.value>=15000 && authoritative!=prediction && researchPackets.size()==1);
    if (!researchPackets.empty()) { const auto& packet=researchPackets.front();CHECK(packet.size()==16 && packet[0]==1 && packet[1]==0x58 && Get(packet,2,2)==16 && Get(packet,4,2)==target.value && packet[6]==128 && packet[7]==0); }
    CHECK((core.pool.Slot(prediction)[11]&1U)!=0 && session.Production().Contains(128) && session.Production().AppendFactoryProducts(target,0)==1 && liveFlares().size()==12);
    const auto dirty=core.bookkeeping.productionDirty;CHECK(dirty!=0);core.ownerMode.challenge=true;CHECK(!session.UpdateProductionContents() && core.bookkeeping.productionDirty==dirty);core.ownerMode.challenge=false;
    CHECK(session.UpdateProductionContents() && core.bookkeeping.productionDirty==0);const auto technologyParent=session.CurrentTechnologyParent();CHECK(technologyParent!=0);
    const auto teleport=session.Priests().Carrier().FindContained(Sid{static_cast<std::uint16_t>(technologyParent)},8,128,0);CHECK(teleport.value>=5 && teleport.value<15000 && Get(core.pool.Slot(teleport),20,2)==8 && core.pool.Slot(teleport)[30]==3);
    CHECK(!session.UpdateProductionContents() && session.Priests().Carrier().FindContained(Sid{static_cast<std::uint16_t>(technologyParent)},8,128,0)==teleport);
    std::cout<<"실제 생산dirty "<<dirty<<"→0/현재 기술부모"<<technologyParent<<"/teleport"<<teleport.value<<" kind128 flags8 quality3 통과\n";
    const auto initialFlares=liveFlares();if (initialFlares.empty()) return;std::size_t drawn{};
    // 실제 같은 수집/가상Draw가 초기 플레어의 signed깊이/현재 프레임을 그리는지 확인한다.
    for (const auto sid:initialFlares) { const auto raw=core.pool.Slot(sid);CHECK(display.Handles(sid) && raw[34]==0 && (raw[40]&8U) && raw[35]==static_cast<std::uint8_t>(core.pool.Slot(target)[35]-1U)); }
    const ScreenRect clip{0,28,1024,768};const auto sprites=display.Sprites(clip,static_cast<int>(point.x*16)-512,static_cast<int>(point.y*11)-384,1);
    // 플레어 깊이의 실제 장면 항목을 출력하고 빈 가상 효과로 바뀌지 않았는지 검사한다.
    for (const auto& sprite:sprites) if (sprite.order.depth==std::bit_cast<std::int8_t>(core.pool.Slot(initialFlares.front())[35]) && sprite.procedure) { std::vector<std::uint8_t> pixels(1024*768,0);sprite.procedure->draw(pixels,1024,768,clip);if (std::any_of(pixels.begin(),pixels.end(),[](auto pixel) { return pixel!=0; })) ++drawn; }
    const auto& firstFrame=assets.game.Shapes().Blocks()[106-70].frames[0];CHECK(firstFrame.IsSpecial() && drawn==0); // 보호 원본A01은 그림 없는 메타 레코드이므로 최초 출력이 비어 있어야 한다.
    const auto nextImage=assets.game.Shapes().Decode(106-70,1);CHECK(std::any_of(nextImage.opacity.begin(),nextImage.opacity.end(),[](auto value) { return value!=0; }));
    const auto researchTime=core.processState.now;std::size_t peak=initialFlares.size(),animated{};
    // 후속 생성·자기 삭제·Kernel 정리를 실제 주기만으로 진행한다.
    for (std::uint64_t frame=1;frame<=360;++frame) {
        const auto now=researchTime+static_cast<double>(frame)/60;session.RunFrame({now,now,1.0/60,10000+frame},false);peak=std::max(peak,liveFlares().size());
        if (frame==12) {
            const auto current=display.Sprites(clip,static_cast<int>(point.x*16)-512,static_cast<int>(point.y*11)-384,1);
            // 실제0.2초 시점의 원본 이미지 프레임을 같은 가상Draw로 출력한다.
            for (const auto& sprite:current) if (sprite.order.depth==std::bit_cast<std::int8_t>(core.pool.Slot(target)[35])-1 && sprite.procedure) { std::vector<std::uint8_t> pixels(1024*768,0);sprite.procedure->draw(pixels,1024,768,clip);if (std::any_of(pixels.begin(),pixels.end(),[](auto pixel) { return pixel!=0; })) ++animated; }
        }
    }
    CHECK(animated>=12);std::cout<<"플레어 원본 첫 메타프레임 출력"<<drawn<<"→0.2초 실제 출력"<<animated<<" 통과\n";
    CHECK(peak>12 && liveFlares().empty() && session.Sp(1)==2200 && session.Production().Contains(128));CHECK(session.Priests().Carrier().FindContained(target,16,128,0)==authoritative && !core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
    CHECK(session.SelectFactoryKnowledge(target,0x03000080U,(128U<<16)|target.value)==1);session.PumpLocalConnection();CHECK(researchPackets.size()==2 && liveFlares().empty() && session.Priests().Carrier().FindContained(target,16,128,0)==authoritative);
    std::cout<<"실제 연구58h/예측"<<prediction.value<<"→서버"<<authoritative.value<<"/현재Deck/플레어12→후속"<<peak<<"→0 통과\n";
    std::cout<<"실제1-1 썬 워크샾 자연 건설 SID"<<target.value<<" 시작"<<began<<" 완료"<<end<<" HP"<<maximum<<"\n";
    if (combat) {
        // 원본1-1 기술의 레인 제네레이터를 같은 지식 선택/서버 수신/내용물 갱신으로 연구한다.
        CHECK(session.SelectFactoryKnowledge(target,0x03000080U,(76U<<16)|target.value)==1);session.PumpLocalConnection();session.UpdateProductionContents();
        CHECK(session.Production().Contains(76) && session.Sp(1)==2200);
    }
    {
        FontStore fonts(assets.files,"Arial");double wall=100;bool productTimers{};std::size_t gets{}; // 실제 원본 글꼴/장치 입력 경계다.
        RawButtonControls controls(OriginalEdition::Patch1078,{[&] { return wall; },{}, {},[&](auto form,auto flags) { session.GumpForms().Destroy({static_cast<std::uint16_t>(form)},flags); }});activeProductionUi=&controls;
        RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,500,350}; };hooks.cursor=[](auto) {};
        hooks.objectForms=[&]() -> GumpFormHost& { return session.GumpForms(); };hooks.gameSeconds=[&] { return core.processState.now; };
        auto& menus=controls.EnableMenus(assets.game,fonts,std::move(hooks));std::array<std::uint32_t,256> logical{};
        // 같은 파일 팔레트의 실제 RGB 번호를 공급한다.
        for (std::size_t i=0;i<logical.size();++i) logical[i]=std::uint32_t(palette[i].red)|(std::uint32_t(palette[i].green)<<8)|(std::uint32_t(palette[i].blue)<<16);
        const auto colors=BuildPaletteColorTable(logical);menus.SetPalette(colors);const auto& fortGump=assets.game.Find("fortGump");const auto metrics=assets.game.Shapes().SquidMetrics(fortGump.block,0);
        const std::array<GumpControl,1> buttons{{{0,{84,4,160,26},false,true,false}}};const std::array<std::string,1> labels{{"Game"}};
        controls.SetControls(buttons,labels,1024,768,{static_cast<std::uint32_t>(fortGump.block+70),0,metrics.width,metrics.height,ButtonColors(colors)});
        auto& production=controls.EnableProduction(session,assets.game,{[&] { return wall; },[&](std::string_view name) { CHECK(name=="getPiece.wav");++gets; },[](auto) { throw std::logic_error("생산 검사 예상 밖 Tell"); },[&](std::string_view key,std::int32_t fallback) { return key=="useProductTimers" ? static_cast<std::int32_t>(productTimers) : fallback; },[](std::string_view text) { return std::string(text); }});production.SetPalette(palette);
        CHECK(controls.RegisteredGump(3) && production.Node(controls.RegisteredGump(3)));CHECK(production.Report().find("\t128\t")!=std::string::npos);
        std::vector<std::uint8_t> pixels(1024*768,0x5a);menus.Draw(pixels,1024,768,colors,BuildGumpShadeMaps(logical));CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto pixel) { return pixel!=0x5a; }));
        std::cout<<"실제 초기 에너지 공급원 "<<core.bookkeeping.providers.count<<"/Cannon level"<<core.types[128].level<<"/theme"<<core.types[128].theme<<"/요구"<<BindAiTypeRequiredEnergy(assets.game,core.types)(128)<<"\n";
        // 완공한 첫 제네레이터의 현재 공급을 이용해 두 번째 제네레이터까지 정상 건설한다.
        if (combat) for (std::uint32_t generatorIndex=0;generatorIndex<2;++generatorIndex) {
            const auto generatorClick=production.Point(76);CHECK(generatorClick.has_value());if (!generatorClick) return; // 실제 생산 클릭 좌표다.
            controls.Event({InputCode::kLeftButton,generatorClick->x,generatorClick->y});CHECK(session.CursorDropSettings().type==76 && gets==generatorIndex+1);
            session.SetParticleView({0,0,0,0,1024,768},{});std::optional<CellPoint> generatorPoint;double nearest=1e9; // 전방 캐논에 가장 가까운 실제 허용 후보를 선택한다.
            // 원본 공급/공간/커서 판정을 모두 통과하는 위치만 전략 후보로 쓴다.
            for (int x=90;x<=115;++x) {
                // 워크샾 위아래를 포함해 실제 지도에서 허용 여부를 질의한다.
                for (int y=90;y<=130;++y) {
                    const auto distance=(x-118.0)*(x-118.0)+(y-115.0)*(y-115.0);if (distance>=nearest) continue;
                    ui.Point(session,{x,y},0,0,false);if (session.CursorDropSettings().legal) { nearest=distance;generatorPoint=CellPoint{x,y}; }
                }
            }
            CHECK(generatorPoint.has_value());if (!generatorPoint) return;ui.Point(session,*generatorPoint);std::cout<<"실제 제네레이터 허용점 "<<session.CursorDropSettings().x<<","<<session.CursorDropSettings().y<<"\n";
            session.Cursor().Drop();session.PumpLocalConnection();session.Cursor().Cancel();CHECK(session.Sp(1)==2200-400*(generatorIndex+1));
            const auto generatorBegan=core.processState.now,generatorWall=wall; // 실제 건설 시작 시각이다.
            // 제네레이터의 HP와 완공은 같은 Kernel이 실제 시간으로 갱신한다.
            for (std::uint64_t frame=1;frame<=2400;++frame) { const auto now=generatorBegan+static_cast<double>(frame)/60;wall=generatorWall+(now-generatorBegan);session.RunFrame({now,wall,1.0/60,frame},false);controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame(); }
            const auto generators=ConstructionLive(core,76);CHECK(generators.size()==generatorIndex+1); // 실제 서버 건물이다.
            // 이 단계까지 생성한 모든 실제 제네레이터의 완공/HP를 읽는다.
            for (auto generator:generators) CHECK(!(core.pool.Slot(generator)[40]&1U) && Get(core.pool.Slot(generator),26)==700);
        }
        const auto click=production.Point(128);CHECK(click.has_value());
        if (click) { CHECK(controls.PickGump(*click)==controls.RegisteredGump(3));controls.Event({InputCode::kLeftButton,click->x,click->y});CHECK(session.CursorDropSettings().preview==teleport && session.CursorDropSettings().type==128 && session.CursorDropSettings().lockDepth==0 && gets==(combat ? 3U : 1U));
            const auto cannonPoint=combat ? cannonLocation : CursorPlacementPoint(session,ui);if (combat) {
                ui.Point(session,cannonPoint,0,0,false);const auto& placement=session.Cursor().Placement();const auto& drop=session.CursorDropSettings();std::string supply; // 같은 배치 거부 원인의 현재 진단이다.
                session.EnergySupply().Collect(supply,drop.x,drop.y,128,1);std::cout<<"캐논 실제좌표 "<<drop.x<<","<<drop.y<<" legal"<<drop.legal<<" 에너지"<<placement.descriptionAllowed<<" 공간"<<placement.placementAllowed<<" 공급문자"<<supply<<" 반경제곱"<<session.EnergySettings().radiusSquared<<"\n";
                // 현재 공급 장부의 실제 위치/소유자/가상 표를 읽는다.
                for (auto number:core.bookkeeping.providers.Items()) { const auto raw=core.pool.Slot({static_cast<std::uint16_t>(number)});std::cout<<"공급 "<<number<<" type"<<unsigned(raw[10])<<" owner"<<unsigned(raw[34])<<" xy"<<std::bit_cast<float>(Get(raw,14))<<","<<std::bit_cast<float>(Get(raw,18))<<" vt"<<Get(raw,0)<<"\n"; }
                CHECK(drop.legal);if (!drop.legal) return;
            }std::cout<<"실제 생산 썬 캐논 배치 허용점 "<<cannonPoint.x<<","<<cannonPoint.y<<"\n";
            session.Cursor().Drop();session.PumpLocalConnection();CHECK(session.Sp(1)==(combat ? 1000 : 1800));session.Cursor().Cancel();
            const auto cannonBegan=core.processState.now,cannonWall=wall;std::size_t completedCannons{}; // 실제 후속 Kernel이 완료한 캐논을 관찰한다.
            if (combat) {
                const auto cannons=ConstructionLive(core,128);CHECK(cannons.size()==1);if (cannons.size()!=1) return;const auto cannon=cannons.front(); // 실제 배치한 캐논이다.
                std::set<std::uint32_t> stages,targets,frames;bool missileSeen{},hitSeen{};std::size_t burnFrames{},burnPixels{};bool burnCaptured{},burnEmitted{};std::map<std::uint16_t,std::uint32_t> enemyHp; // 현재 실제 과정의 관측 집합이다.
                // 실제 표적이 발사기라고 미리 단정하지 않고 모든 살아 있는 적 피해 자산의 HP를 읽는다.
                for (std::uint32_t number=core.pool.Layout().serverFirst;number<core.pool.Capacity();++number) {
                    const Sid sid{static_cast<std::uint16_t>(number)};const auto raw=core.pool.Slot(sid);if (!(raw[11]&3U) && raw[10]>=70 && raw[34]==2 && (core.types[raw[10]].flags1&0x10U)) enemyHp.emplace(sid.value,Get(raw,26));
                }
                CHECK(enemyHp.size()>=5);
                if (burning) session.SetParticleView({1344,976,0,0,1024,768},{}); // 자연 전투의 현재 자산/연소 영역이 viewport에 들어오는 카메라 입력이다.
                // 기본 검사는 첫 적중, 연소 검사는 자연 저HP 생성 후240프레임의 실제 캡처/입자/출력까지 진행한다.
                for (std::uint64_t frame=1;frame<=7200 && (burning ? burnFrames<240 : !hitSeen);++frame) {
                    const auto now=cannonBegan+static_cast<double>(frame)/60;wall=cannonWall+(now-cannonBegan);session.RunFrame({now,wall,1.0/60,frame},false);controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame();
                    const auto form=FindAttachedProcess(core.pool,cannon,34);if (const auto* process=dynamic_cast<GunProcess*>(core.processes.FromForm(form))) { stages.insert(process->Fields().Stage());if (process->Fields().Target().value) targets.insert(process->Fields().Target().value); }
                    if (!(core.pool.Slot(cannon)[40]&1U)) frames.insert(Get(core.pool.Slot(cannon),36));
                    // 현재 살아 있는 소유자1 탄체의 실제 존재만 관찰한다.
                    for (auto sid:ConstructionLive(core,80)) if (core.pool.Slot(sid)[34]==1) missileSeen=true;
                    if (burning) {
                        bool present{}; // 같은 실제14 등록/Form의 현재 상태만 읽는다.
                        for (const auto token:session.Particles().Rendering().Slots()) { // 일반 표시 목록에서 자연 생성된 실제14만 관찰한다.
                            const auto* r=session.Particles().Rendering().Lookup(token);if (!r) continue;const auto* effect=dynamic_cast<BurningVisualProcess*>(core.processes.FromForm({static_cast<std::uint16_t>(r->form)}));if (!effect) continue;present=true;const auto& f=effect->Fields();CHECK(f.capacity==150 && f.mode==1);if (f.stage>=1 && !f.image.pixels.empty() && !f.rows.empty()) burnCaptured=true;
                            for (const auto& particle:f.particles) if (particle.life>0 && !particle.source) burnEmitted=true; // source가 아닌 실제 이동 입자다.
                        }
                        if (present) ++burnFrames;
                        if (present) { Renderer particles(1024,768);particles.SetScene(session.ParticleSprites({0,0,1024,768}));std::vector<std::uint8_t> burningPixels(1024*768,255);particles.Draw(burningPixels,1024,255);burnPixels+=static_cast<std::size_t>(std::count_if(burningPixels.begin(),burningPixels.end(),[](auto value) { return value!=255; })); }
                    }
                    // 삭제/HP 감소는 실제 슬롯의 결과에서 읽으며 결과를 쓰지 않는다.
                    for (const auto& [number,hp]:enemyHp) { const auto raw=core.pool.Slot({number});if ((raw[11]&3U) || Get(raw,26)<hp) { if (!hitSeen) std::cout<<"실제 피해 대상"<<number<<" TYPE"<<unsigned(raw[10])<<" HP"<<hp<<"→"<<Get(raw,26)<<"\n";hitSeen=true; } }
                }
                // 선택한 실제 표적 번호도 기록해 특정 적 종류의 사전 가정을 피한다.
                for (auto sid:targets) std::cout<<"실제 캐논 표적 SID"<<sid<<" TYPE"<<unsigned(core.pool.Slot({static_cast<std::uint16_t>(sid)})[10])<<"\n";
                CHECK(!targets.empty() && stages.count(2) && stages.count(5));CHECK(missileSeen && hitSeen && frames.size()>1);CHECK(session.Sp(1)==1000);
                std::cout<<"실제 제네레이터 연장/캐논 자연 첫 적중 표적"<<targets.size()<<"/단계"<<stages.size()<<"/완공 그림"<<frames.size()<<"/탄체"<<missileSeen<<"/적 HP 감소"<<hitSeen<<"\n";
                if (burning) { CHECK(burnFrames==240 && burnCaptured && burnEmitted && burnPixels>0);std::cout<<"자연 저HP Burning14: 실제 프레임"<<burnFrames<<"/캡처"<<burnCaptured<<"/이동입자"<<burnEmitted<<"/표시 픽셀"<<burnPixels<<"\n"; }
                if (forward) {
                    session.SetParticleView({0,0,0,0,1024,768},{});const Sid temple{15017};const auto initialTempleHp=Get(core.pool.Slot(temple),26); // 실제 남은 적 윈드 신전의 현재 HP를 읽는다.
                    const auto firingAt=[&](float x,float y) { const auto& type=core.types[128];return session.GunTargeting().MayTarget({x,y,static_cast<float>(x-type.footX*.5),static_cast<float>(y-type.footY*.5),128,0,1,0},temple); }; // 실제 창의 불가127,121과 자연 공략129,126을 같은 원본 ray로 구별한다.
                    CHECK(!firingAt(127,121) && firingAt(129,126));
                    const auto leavesCannonSpace=[&] { //사용자가계획한136,120의캐논발자국을비워두도록다리후보만선택한다.
                        const auto& previews=session.Cursor().Lifecycle();const auto& cannon=core.types[128];
                        for (std::int32_t index=0;index<previews.previewCount;++index) { // 실제 커서가 표시한 다리 셀만 읽고 공간/방향/원본 배치 결과를 바꾸지 않는다.
                            const auto raw=core.pool.Slot(previews.previews[static_cast<std::size_t>(index)]);if (raw[10]!=82) continue;
                            const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));if (x>=136-cannon.footX+1 && x<=136 && y>=120-cannon.footY+1 && y<=120) return false;
                        }
                        return true;
                    };
                    const auto plan=netstorm::test::rawscene::LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/campaign-forward-battle-north-plan.tsv").string().c_str());std::uint64_t forwardFrame=90000,battleFrames=0; //기존동결계획을보존한북쪽수동전술입력이며HP/SP/배치성공을주입하지않는다.
                    const auto tickForward=[&] { const auto now=core.processState.now+1.0/60;wall+=1.0/60;session.RunFrame({now,wall,1.0/60,++forwardFrame},false);controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame(); }; // 실제 시계/입력/메뉴 정리를 한 프레임씩 진행한다.
                    for (const auto& command:plan.rows) { // 같은 실제 생산/커서/서버 경로에 명시 전략 입력을 순서대로 적용한다.
                        CHECK(command.size()==5);if (command.size()!=5) return;const auto wait=std::stoul(command[4]);
                        if (command[0]=="battle-window") { battleFrames=wait;continue; } // 마지막 자연 공격의 최대 시간만 전술 입력으로 지정한다.
                        if (command[0]=="wait-dead") {
                            const Sid watched{static_cast<std::uint16_t>(std::stoul(command[1]))};
                            for (std::uint64_t frame=0;frame<wait && !(core.pool.Slot(watched)[11]&3U);++frame) tickForward(); // 표적 삭제는 Kernel의 자연 공격 결과로만 기다린다.
                            std::cout<<"전진 이전 자연 삭제 대기 SID"<<watched.value<<" time"<<core.processState.now<<" flags"<<unsigned(core.pool.Slot(watched)[11])<<"\n";CHECK(core.pool.Slot(watched)[11]&3U);continue;
                        }
                        CHECK(command[0]=="build" || command[0]=="build-fire");
                        const auto type=static_cast<std::uint32_t>(std::stoul(command[1]));const CellPoint preferred{std::stoi(command[2]),std::stoi(command[3])};const auto icon=production.Point(type);CHECK(icon.has_value());if (!icon) return;controls.Event({InputCode::kLeftButton,icon->x,icon->y});CHECK(session.CursorDropSettings().type==type);
                        std::optional<CellPoint> place;double nearest=1e9;std::uint32_t bestTurns{};std::size_t legalCandidates{},firingCandidates{};const auto orientations=type==82 ? 4U : 1U; // 다리는 실제 우클릭 네 방향을 비교하며 방문 후보의 배치/사격 관문을 구분해 센다.
                        constexpr int searchLeft=116,searchTop=102,searchRight=150,searchBottom=135; //AI가점유한앞길을북쪽으로우회할수있는실제사용자후보범위다.
                        std::array<std::string,searchBottom-searchTop+1> candidateMap; //실패때합법L/사격F/교집합B의실제후보위치를보존한다.
                        for (auto& row:candidateMap) row.assign(searchRight-searchLeft+1,'.'); //지정지도범위와같은칸수만초기화한다.
                        for (std::uint32_t turns=0;turns<orientations;++turns) { // 방향 필드를 직접 쓰지 않고 사용자 우클릭으로 회전한다.
                            for (int x=searchLeft;x<=searchRight;++x) { //적섬까지의실제전방지도칸이다.
                                for (int y=searchTop;y<=searchBottom;++y) { //북쪽우회후보를포함한발자국의공간/공급조건을실제미리보기로검사한다.
                                    const auto distance=(x-preferred.x)*(x-preferred.x)+(y-preferred.y)*(y-preferred.y);if (distance>=nearest) continue;ui.Point(session,{x,y},0,0,false);const bool fire=command[0]!="build-fire" || firingAt(session.CursorDropSettings().x,session.CursorDropSettings().y); // 현재 원본 사격 판정과 배치 판정은 독립 관측하며 결과를 쓰지 않는다.
                                    legalCandidates+=session.CursorDropSettings().legal ? 1U : 0U;firingCandidates+=fire ? 1U : 0U;if (session.CursorDropSettings().legal && fire && (type!=82 || leavesCannonSpace())) { nearest=distance;place=CellPoint{x,y};bestTurns=turns; }
                                    candidateMap[static_cast<std::size_t>(y-searchTop)][static_cast<std::size_t>(x-searchLeft)]=session.CursorDropSettings().legal ? (fire ? 'B' : 'L') : (fire ? 'F' : '.');
                                }
                            }
                            if (type==82) CHECK(session.Cursor().HandleInput({InputCode::kRightButton,0,0},0)); // 네 번 뒤 원래 방향으로 돌아와 선택한 입력 횟수만 다시 적용한다.
                        }
                        for (std::uint32_t turns=0;turns<bestTurns;++turns) CHECK(session.Cursor().HandleInput({InputCode::kRightButton,0,0},0)); // 합법 후보의 실제 사용자 회전 입력만 재현한다.
                        ui.Point(session,preferred,0,0,false);const auto& rejected=session.Cursor().Placement();std::cout<<"전진 목표 판정 TYPE"<<type<<" 방향"<<session.CursorDropSettings().direction<<" 위치"<<preferred.x<<","<<preferred.y<<" desc"<<rejected.descriptionAllowed<<" place"<<rejected.placementAllowed<<" source"<<rejected.sourceAllowed<<" text"<<rejected.description<<" 방문 합법"<<legalCandidates<<"/사격"<<firingCandidates<<"\n";CHECK(place.has_value());if (!place) {
                            for (std::size_t row=0;row<candidateMap.size();++row) std::cout<<"후보 지도 y"<<searchTop+row<<" x"<<searchLeft<<' '<<candidateMap[row]<<'\n'; //실패한현재후보집합만출력하며게임판정을대체하지않는다.
                            for (int y=123;y<=127;++y) { //원래사격후보129,126주변의실제표면/소유/연결/점유입력만실패자료로보존한다.
                                for (int x=126;x<=130;++x) { const auto sid=core.hash.Cell(0,x,y);const auto raw=core.pool.Slot(Sid{sid});std::printf("사격후보 지면(%d,%d) SID%u TYPE%u owner%u graph%u spot%02x\n",x,y,sid,raw[10],raw[34],raw[30],core.spots[y*256+x]); } //현재지도값을변경하지않는다.
                            }
                            const auto targetBounds=session.GunTargeting().Footprint(temple);std::printf("Temple bounds %g,%g,%g,%g\n",targetBounds.left,targetBounds.top,targetBounds.right,targetBounds.bottom); //현재표적발자국으로사격범위의원인을분리한다.
                            for (const auto probePoint:std::array<CellPoint,3>{{{123,124},{129,126},{136,120}}}) { //서로다른실제후보의네방향ray를읽고배치/사격결과를변경하지않는다.
                                const auto& cannonInfo=core.types[128];const GunTargetQuery query{static_cast<float>(probePoint.x),static_cast<float>(probePoint.y),static_cast<float>(probePoint.x-cannonInfo.footX*.5),static_cast<float>(probePoint.y-cannonInfo.footY*.5),128,0,1,0};
                                for (int direction=0;direction<8;direction+=2) { const auto ray=session.GunTargeting().CannonRay(query,direction);std::printf("Cannon(%d,%d) dir%d ray %g,%g,%g,%g\n",probePoint.x,probePoint.y,direction,ray.left,ray.top,ray.right,ray.bottom); } //원본허용짝수방향만관측한다.
                            }
                            return;
                        }ui.Point(session,*place);const auto money=session.Sp(1);std::cout<<"전진 실제 허용 TYPE"<<type<<" 위치"<<place->x<<","<<place->y<<" SP"<<money<<"\n";
                        session.Cursor().Drop();session.PumpLocalConnection();session.Cursor().Cancel();CHECK(session.Sp(1)==money-core.types[type].cost);
                        for (std::uint64_t frame=0;frame<wait;++frame) tickForward(); // 실제 반환/건설/전투 시간을 진행한다.
                        const auto live=ConstructionLive(core,type);std::cout<<"전진 건설 뒤 TYPE"<<type<<" 생존"<<live.size()<<" 신전HP"<<Get(core.pool.Slot(temple),26)<<"\n";for (auto built:live) std::cout<<"  내 자산"<<built.value<<" HP"<<Get(core.pool.Slot(built),26)<<" extra"<<unsigned(core.pool.Slot(built)[40])<<" xy"<<std::bit_cast<float>(Get(core.pool.Slot(built),14))<<","<<std::bit_cast<float>(Get(core.pool.Slot(built),18))<<"\n"; // 건설 중 피격/생존과 실제 좌표를 관찰한다.
                    }
                    std::set<std::uint32_t> forwardTargets; // 실제 추가 캐논들의 표적만 관찰한다.
                    for (std::uint64_t frame=1;frame<=battleFrames && !(core.pool.Slot(temple)[11]&3U);++frame) { // 지정한 최대 시간 동안 실제 자연 공격으로 신전 파괴를 기다린다.
                        tickForward();if (frame%6000==0) std::cout<<"전진 공격 경과 "<<frame/60<<"초 신전HP"<<Get(core.pool.Slot(temple),26)<<"\n"; // 긴 공략의 실제 피해 진행을 관찰한다.
                        for (const auto gun:ConstructionLive(core,128)) { // 가상 표적/HP 결과를 새 값으로 덮어쓰지 않는다.
                            const auto form=FindAttachedProcess(core.pool,gun,34);if (const auto* process=dynamic_cast<GunProcess*>(core.processes.FromForm(form))) if (process->Fields().Target().value) forwardTargets.insert(process->Fields().Target().value);
                        }
                    }
                    std::cout<<"전진 자연 공격 신전HP"<<initialTempleHp<<"→"<<Get(core.pool.Slot(temple),26)<<" flags"<<unsigned(core.pool.Slot(temple)[11])<<" 표적";for (auto observedTarget:forwardTargets) std::cout<<observedTarget<<",";std::cout<<"\n"; // 최종 자연 진행 상태와 표적을 기록한다.
                    CHECK(core.pool.Slot(temple)[11]&3U);
                }
                if (reclaim) {
                    const auto reclaimedCannons=ConstructionLive(core,128);CHECK(reclaimedCannons.size()==1);if (reclaimedCannons.size()!=1) return;const auto reclaimedCannon=reclaimedCannons.front(); // 앞의 실제 연구/생산/배치/자연 완공 결과만 회수한다.
                    CHECK(Get(core.pool.Slot(reclaimedCannon),26)==600 && core.types[128].cost==400);const auto money=session.Sp(1);const auto beforePackets=session.LocalConnectionPackets().size(); // 원래400비용/완전HP의 독립 회수 기대100이다.
                    session.RequestSalvage(reclaimedCannon.value);CHECK(session.Sp(1)==money && !(core.pool.Slot(reclaimedCannon)[11]&3U) && session.LocalConnectionPackets().size()==beforePackets); // 요청에서 삭제/환불 결과를 직접 만들지 않는다.
                    session.PumpLocalConnection();CHECK(session.Sp(1)==money+100 && (core.pool.Slot(reclaimedCannon)[11]&1U) && ConstructionLive(core,128).empty());
                    CHECK(std::count(session.LocalConnectionPackets().begin()+static_cast<std::ptrdiff_t>(beforePackets),session.LocalConnectionPackets().end(),std::uint8_t{0x54})==1);const auto afterMoney=session.Sp(1);session.RequestSalvage(reclaimedCannon.value);session.PumpLocalConnection();CHECK(session.Sp(1)==afterMoney); // 같은 반납SID의 두 번째 회수는 보상을 중복 지급하지 않는다.
                    for (std::uint64_t frame=1;frame<=360;++frame) { const auto now=core.processState.now+1.0/60;wall+=1.0/60;session.RunFrame({now,wall,1.0/60,120000+frame},false);controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame(); } // 같은 Kernel의 실제 붕괴와 종속 정리를 자연 진행한다.
                    CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);std::cout<<"자연 캐논"<<reclaimedCannon.value<<" 실제54h 회수 SP"<<money<<"→"<<session.Sp(1)<<"/반납·붕괴 정리 완료\n";
                }
                controls.DisableProduction();activeProductionUi=nullptr;return;
            }
            // 반환 내용물 Regular의 count0/10초/count1과 실제 캐논 건설을 자연 실행한다.
            for (std::uint64_t frame=1;frame<=2400;++frame) { const auto now=cannonBegan+static_cast<double>(frame)/60;session.RunFrame({now,cannonWall+(now-cannonBegan),1.0/60,frame},false);wall=cannonWall+(now-cannonBegan);controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame(); }
            // 현재 서버 풀에서 살아 있는 내 완공 캐논을 직접 읽는다.
            for (std::uint32_t number=core.pool.Layout().serverFirst;number<core.pool.Capacity();++number) { const auto raw=core.pool.Slot({static_cast<std::uint16_t>(number)});if (!(raw[11]&3) && raw[10]==128 && raw[34]==1 && !(raw[40]&1)) ++completedCannons; }
            CHECK(completedCannons==1);
            const auto bridgeClick=production.Point(82);CHECK(bridgeClick.has_value());
            if (bridgeClick) {
                // 실제 표시된 다리 아이콘을 클릭하며 생성/허용/SP 값을 검사에서 주입하지 않는다.
                controls.Event({InputCode::kLeftButton,bridgeClick->x,bridgeClick->y});const auto preview=session.CursorDropSettings().preview;
                CHECK(preview.value && session.CursorDropSettings().type==82 && session.CursorDropSettings().lockDepth==0 && gets==2);
                const auto bridgePoint=CursorPlacementPoint(session,ui);std::cout<<"실제 공급 다리 배치 허용점 "<<bridgePoint.x<<","<<bridgePoint.y<<"\n";
                const auto bridges=[&] { std::size_t total{};
                    // 서버의 실제 살아 있는 다리만 센다. 미리보기/내용물은 포함하지 않는다.
                    for (std::uint32_t number=core.pool.Layout().serverFirst;number<core.pool.Capacity();++number) { const auto raw=core.pool.Slot({static_cast<std::uint16_t>(number)});if (!(raw[11]&3) && raw[10]==82) ++total; }return total;
                };
                const auto bridgeCountBefore=bridges();session.Cursor().Drop();session.PumpLocalConnection();session.Cursor().Cancel();CHECK(bridges()>bridgeCountBefore && session.Sp(1)==1800);
                const auto bridgeBegan=core.processState.now,bridgeWall=wall; // 벽시각은 초기100초에서 단조 증가시킨다.
                // 사용한 칸이 실제1초 공급으로 보충되고6초 게임 시각 뒤 정상 품질이 되는 과정을 실행한다.
                for (std::uint64_t frame=1;frame<=480;++frame) { const auto now=bridgeBegan+static_cast<double>(frame)/60;session.RunFrame({now,bridgeWall+(now-bridgeBegan),1.0/60,frame},false);wall=bridgeWall+(now-bridgeBegan);controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame(); }
                RawContainedFinder supply(core.pool,session.ProductionFinder());std::size_t supplied{};
                // 같은 기술 체인의 모든 공급 다리를 원문으로 다시 확인한다.
                for (auto sid=supply.Begin({static_cast<std::uint16_t>(session.CurrentTechnologyParent())});sid.value;sid=supply.Next()) if (Get(core.pool.Slot(sid),20,2)&0x40) { CHECK(core.pool.Slot(sid)[30]==2);++supplied; }
                CHECK(supplied==static_cast<std::size_t>(session.BattleOption(1)*2+2));
                std::cout<<"실제 다리 클릭/Take/서버 배치/자동 재공급·품질 회복 통과\n";
                const auto placed=session.World().Find(static_cast<float>(bridgePoint.x),static_cast<float>(bridgePoint.y),82);CHECK(placed.value>=core.pool.Layout().serverFirst);
                if (placed.value) {
                    const auto original=core.pool.Slot(placed);const auto x=std::bit_cast<float>(Get(original,14)),y=std::bit_cast<float>(Get(original,18)); // 실제 배치 좌표를 소리/더미와 대조한다.
                    const auto maps=BuildStandardOwnerColorMaps(82,core.bridgeState.bridgeType);std::array<std::uint32_t,188> tokens{};tokens[82]=1;ShapeToBufferState captureState;captureState.addTypeHotspot=true;captureState.ownerTypeMaps=tokens;
                    captureState.ownerColors=[&](std::uint32_t type,std::uint32_t owner)->std::span<const std::uint8_t> { CHECK(type==82);return maps.at(static_cast<std::size_t>(core.bridgeState.ownerColors.at(owner))); };
                    BridgeDrawState drawState;RawBridgeDraw bridgeDraw(core.pool,drawState);captureState.mainFrame=[&](Sid sid) { return bridgeDraw.MainFrame(sid); };captureState.additional=[&](Sid sid,ConstructionVisualImage&,std::int32_t px,std::int32_t py) { bridgeDraw.Additional(sid,px,py); }; // 실제 extra2의 같은 검증된 가상 몸체다.
                    RawShapeToBuffer capture(core.pool,core.types,core.shapes,assets.game.Shapes(),captureState);const auto expected=capture.Capture(placed);const auto soundsBefore=bridgeSounds.size();
                    Renderer renderer(1024,768);session.SetParticleView({static_cast<int>(x*16)-256,static_cast<int>(y*11)-256,0,0,1024,768},{[&] { return renderer.FullRedrawPending(); },[&](SquidDisplayRect rect,std::uint32_t flags) { renderer.Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }});
                    std::vector<Sid> attachedLinks;
                    // 실제 가상 전처리가 정리할 연결 객체의 현재 두 참조를 파괴 전에 읽는다.
                    for (std::uint32_t number=5;number<core.pool.Capacity();++number) { const Sid sid{static_cast<std::uint16_t>(number)};const auto link=core.pool.Slot(sid);if (!(link[11]&3U) && link[10]==155 && (Get(link,8,2)==placed.value || Get(link,12,2)==placed.value)) attachedLinks.push_back(sid); }
                    CHECK(!attachedLinks.empty());
                    auto* request=core.processes.AddRegular(placed,kBridgeDestroyEvent,0);CHECK(request!=nullptr);const auto collapseStarted=core.processState.now,wallAt=wall; // 사건은 명시 검사 입력이며 처리/파괴/파편/후속 소진은 실제 전체 Kernel이다.
                    session.RunFrame({collapseStarted+1.0/60,wallAt+1.0/60,1.0/60,20001},false);CHECK((core.pool.Slot(placed)[11]&1U)!=0 && bridgeSounds.size()==soundsBefore+1 && (bridgeSounds.back()==std::array<float,2>{x,y}));
                    // 삭제된 현재 다리를 참조한 실제 연결 객체도 같은 재귀 삭제에서 반납돼야 한다.
                    for (const auto sid:attachedLinks) CHECK(core.pool.Slot(sid)[11]&1U);
                    std::cout<<"실제 다리 연결 객체 정리 "<<attachedLinks.size()<<"개\n";
                    ShrapnelVisualProcess* effect=nullptr;
                    // 실제 같은 표시 목록과 Host에서 새 process16을 찾는다.
                    for (const auto token:session.Particles().Rendering().Slots()) { const auto* fields=session.Particles().Rendering().Lookup(token);if (!fields) continue;auto* candidate=dynamic_cast<ShrapnelVisualProcess*>(core.processes.FromForm({static_cast<std::uint16_t>(fields->form)}));if (candidate) { CHECK(effect==nullptr);effect=candidate; } }
                    CHECK(effect!=nullptr);
                    if (effect) {
                        const auto& fields=effect->Fields();CHECK(fields.image.pixels==expected.pixels && fields.image.width==expected.width && fields.image.height==expected.height && fields.image.hotspotX==expected.hotspotX && fields.image.hotspotY==expected.hotspotY && std::bit_cast<std::uint32_t>(fields.gravity)==0x3f99999aU && fields.total>0);
                        const auto token=fields.registration.token;const Sid form{static_cast<std::uint16_t>(fields.registration.form)},dummy{static_cast<std::uint16_t>(fields.registration.parent)};CHECK(core.pool.Slot(dummy)[10]==110 && core.pool.Slot(form)[10]==16 && core.dependencies.Has(form,dummy) && std::bit_cast<float>(Get(core.pool.Slot(dummy),14))==x && std::bit_cast<float>(Get(core.pool.Slot(dummy),18))==y);
                        renderer.SetScene(session.ParticleSprites({0,0,1024,768}));const auto firstImage=renderer.SceneImage();CHECK(std::any_of(firstImage.indices.begin(),firstImage.indices.end(),[](auto color) { return color!=0; }));bool moved=false;
                        // 원본 그림의 입자가 실제 시각 갱신만으로 이동/소진하고 같은 상호 의존으로 해제되는지 확인한다.
                        for (std::uint64_t frame=2;frame<=360;++frame) { const auto now=collapseStarted+static_cast<double>(frame)/60;session.RunFrame({now,wallAt+(now-collapseStarted),1.0/60,20000+frame},false);wall=wallAt+(now-collapseStarted);controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame();if (frame==12) { renderer.SetScene(session.ParticleSprites({0,0,1024,768}));const auto next=renderer.SceneImage();moved=next.indices!=firstImage.indices && std::any_of(next.indices.begin(),next.indices.end(),[](auto color) { return color!=0; }); } }
                        CHECK(moved && !session.Particles().Rendering().Lookup(token) && (core.pool.Slot(form)[11]&1U) && (core.pool.Slot(dummy)[11]&1U));
                        CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth && session.Sp(1)==1800 && bridgeSounds.size()==soundsBefore+1);
                        std::cout<<"실제 서버 다리2691/가상 파괴/소유자 캡처·종류2 픽셀/위치 소리1회/자연 소진·더미 해제 통과\n";
                    }
                    session.SetParticleView({0,0,0,0,1024,768},{}); // 지역 Renderer가 해제되기 전에 빌린 장치 경계를 반납한다.
                }
            }
        }
        // 실제 취득/허용점/서버 배치로만 SP와 재고를 소비한다. 실패값을 주입하지 않는다.
        for (std::uint32_t extra=0;extra<4;++extra) {
            const auto current=production.Point(128);CHECK(current.has_value());if (!current) break;
            const auto priorMoney=session.Sp(1);const auto previousGets=gets;controls.Event({InputCode::kLeftButton,current->x,current->y});CHECK(gets==previousGets+1 && session.CursorDropSettings().lockDepth==0);
            CursorPlacementPoint(session,ui);session.Cursor().Drop();session.PumpLocalConnection();session.Cursor().Cancel();CHECK(session.Sp(1)==priorMoney-400);
            const auto beganMore=core.processState.now,wallMore=wall; // 실제 반환 내용물의 선택 잠금 Regular도 자연 진행한다.
            // 다음 클릭은 원본 반환 지연과 건설이 지난 실제 프레임에서 수행한다.
            for (std::uint64_t frame=1;frame<=720;++frame) { const auto now=beganMore+static_cast<double>(frame)/60;wall=wallMore+(now-beganMore);session.RunFrame({now,wall,1.0/60,frame},false);controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame(); }
            if (extra==1) {
                productTimers=true;const auto zero=production.Point(128);CHECK(zero.has_value());const auto noTake=gets;
                if (zero) controls.Event({InputCode::kLeftButton,zero->x,zero->y});
                CHECK(gets==noTake && session.Sp(1)==1000 && session.CursorDropSettings().lockDepth!=0);CHECK(production.Report().find("~0~w~ENone in inventory")!=std::string::npos);
                menus.Draw(pixels,1024,768,colors,BuildGumpShadeMaps(logical));wall+=3.999;controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame();CHECK(production.Report().find("raw_production_hint\t")!=std::string::npos);
                wall+=0.001;controls.Poll({InputCode::kRelease,500,350});menus.CleanupFrame();CHECK(production.Report().find("raw_production_hint\t")==std::string::npos);productTimers=false;
            }
        }
        CHECK(session.Sp(1)==200);const auto insufficient=production.Point(128);CHECK(insufficient.has_value());const auto noTake=gets;
        if (insufficient) controls.Event({InputCode::kLeftButton,insufficient->x,insufficient->y});CHECK(gets==noTake && session.Sp(1)==200 && session.CursorDropSettings().lockDepth!=0);
        CHECK(production.Report().find("~0~w~ENot enough Storm Power to build this")!=std::string::npos && production.Report().find("raw_production_budget_flash\t10\t")!=std::string::npos);
        menus.Draw(pixels,1024,768,colors,BuildGumpShadeMaps(logical));
        // 같은 종류 안내의 실제 교체와 루트 형제의 생산 종료 시 정리를 함께 확인한다.
        if (insufficient) controls.Event({InputCode::kLeftButton,insufficient->x,insufficient->y});menus.CleanupFrame();
        CHECK(production.Report().find("raw_production_hint\t")==production.Report().rfind("raw_production_hint\t"));
        controls.DisableProduction();CHECK(!controls.Production());CHECK(session.Sp(1)==200);activeProductionUi=nullptr;
    }

}

} // 자연 플레이 검사 경계다.
// 제네레이터를 추가하지 않는 기존 건설/재고/부족 안내 회귀를 유지한다.
TEST_CASE(CampaignSession_LocalLoginWorkshopHasActualPlacement) { NaturalWorkshopPlacement(false); }
// 전방 배치는 실제 레인 제네레이터를 연구/완공한 뒤 실행한다.
TEST_CASE(CampaignSession_LocalLoginGeneratorExtendsCannonFirstHit) { NaturalWorkshopPlacement(true); }

// 실제 표시 소비자에 세 템플의 명시 상태 입력264개를 공급하고 원본 순서/SHP의 불투명 픽셀을 대조한다.
// 썬더는 저장 미션/실전 관찰이 아니라 동일 지점의 합성 표시 입력이며 게임 프레임을 실행하지 않는다.
TEST_CASE(CampaignTempleDraw_Native264BodyPixels) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",external.Services());session.Load();const auto core=session.Core();const Sid temple{15002};CHECK(core.pool.Slot(temple)[10]==124);
    RawCampaignFactoryDisplay display(core,assets.game,[&](Sid sid,const auto& frame) { session.DrawTemple(sid,frame); });std::array<ScreenColor,256> palette{}; // 같은 풀/실제 SpriteDraw/파일 팔레트다.
    for (std::size_t index=0;index<palette.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));palette[index]={color.blue,color.green,color.red,0}; } // 현재 같은 파일 색을 사용한다.
    display.SetPalette(palette);std::size_t cases{},compared{}; // 실제 본체 픽셀만 세며 그림자 전체 일치로 세지 않는다.
    // 새 객체/생산/완공을 주입한 실전 검사로 주장하지 않고 현재 표시 raw만 명시적으로 교체한다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_TEMPLE_DRAW_FIXTURE).rows) {
        const auto& type=assets.game.Find(row[0]);auto raw=core.pool.AllocatedBytes(temple);const auto number=type.block+kFirstAssetTypeNumber;raw[10]=static_cast<std::uint8_t>(number);Put(raw,0,number==123 ? 0x519860 : number==124 ? 0x5196f0 : 0x519918);Put(raw,36,std::stoul(row[1]));Put(raw,12,std::stoul(row[2]),2);raw[40]=2;
        auto sprites=display.Sprites({0,0,512,400},1168,1036,1);const float worldX=std::bit_cast<float>(Get(raw,14)),worldY=std::bit_cast<float>(Get(raw,18)); // 이 원문은 현재 템플 본체만의 기대이며 주변 간헐천의 가림을 비교하지 않는다.
        std::erase_if(sprites,[worldX,worldY](const auto& sprite) { return sprite.order.x!=worldX || sprite.order.y!=worldY; });
        Renderer renderer(512,400);renderer.SetScene(sprites);std::vector<std::uint8_t> actual(512*400,7);renderer.Draw(actual,512,7);
        std::map<std::size_t,std::uint8_t> expected;std::vector<std::uint32_t> layers;const auto base=std::stoul(row[3]);if (base!=0xffffffffU) layers.push_back(static_cast<std::uint32_t>(base)); // 실제 원본+78 반환이다.
        // 원본 전용Draw가 제출한 F만 읽고 제품의 순서 계산을 재사용하지 않는다.
        for (const auto& event:netstorm::test::rawscene::Split(row[4],',')) if (!event.empty() && event[0]=='F') layers.push_back(static_cast<std::uint32_t>(std::stoul(event.substr(1))));
        // 기존 SHP 압축 해제에서 한 불투명 픽셀씩 직접 덮는다. 제품 Renderer 합성을 부르지 않는다.
        for (const auto frame:layers) {
            const auto& shape=assets.game.Shapes().Blocks()[type.block].frames[frame];if (shape.IsSpecial()) continue;const auto image=assets.game.Shapes().Decode(type.block,frame); // 레인27은 이미지가 아닌 특수 레코드이며 실제 픽셀 소비자도 건너뛴다.
            for (std::size_t y=0;y<image.height;++y) for (std::size_t x=0;x<image.width;++x) { const auto source=y*image.width+x;const auto dx=256-core.types[number].hotspotX+shape.rect.left+static_cast<int>(x),dy=240-core.types[number].hotspotY+shape.rect.top+static_cast<int>(y);if (image.opacity[source] && dx>=0 && dx<512 && dy>=0 && dy<400) expected[static_cast<std::size_t>(dy)*512+dx]=image.indices[source]; } // 기준점/투명/화면 clipping을 독립 적용한다.
        }
        std::size_t mismatch{};for (const auto& [at,color]:expected) if (actual[at]!=color) ++mismatch; // 그림자/배경 영역을 본체 기대에 섞지 않는다.
        if (mismatch) std::cerr<<"템플 픽셀 차이 "<<row[0]<<" frame"<<row[1]<<" word"<<row[2]<<" "<<mismatch<<'\n';CHECK(mismatch==0 && !expected.empty());compared+=expected.size();++cases;
    }
    // 세 템플의 선택 상태도 같은 일반Draw의 괄호/HP 경로를 실행하며 본체와 실제 그림자 항목을 수집한다.
    for (const auto name:{"windVortex","rainVortex","thunderVortex"}) {
        const auto& type=assets.game.Find(name);auto raw=core.pool.AllocatedBytes(temple);const auto number=type.block+kFirstAssetTypeNumber;raw[10]=static_cast<std::uint8_t>(number);Put(raw,0,number==123 ? 0x519860 : number==124 ? 0x5196f0 : 0x519918);Put(raw,36,type.definition.specialFrames.defaultFrame);Put(raw,12,0,2);raw[40]=6;
        const auto sprites=display.Sprites({0,0,512,400},1168,1036,1);bool body{},shadow{}; // 원본 수집 결과의 같은 기준점/그림자 y보정이다.
        for (const auto& sprite:sprites) if (sprite.order.x==89) { if (sprite.order.y==116) body=true;if (sprite.order.y==static_cast<float>(116.0-.1f)) shadow=true; } // 현재 같은 템플의 두 항목만 확인한다.
        CHECK(body && shadow);Renderer selected(512,400);selected.SetScene(sprites);std::vector<std::uint8_t> pixels(512*400,7);selected.Draw(pixels,512,7);CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=7; }));
    }
    CHECK(cases==264 && compared>1000000);std::cerr<<"실제 템플 표시 소비자: 합성 입력"<<cases<<"개 본체"<<compared<<"픽셀 일치. 썬더 실전 창 검증 별도\n";
}

// 본체를 빼고 동작 프레임만 그리던 실제 화면 연결을 검사한다. raw 교체는 표시 검사 입력이며 자연 생산 검사가 아니다.
TEST_CASE(CampaignCombatDraw_BodiesAndSixFlyingShadows) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external; // 동일한 실제 자산/장치 경계다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",external.Services());session.Load();auto core=session.Core();const Sid object{15002}; // 기존 슬롯을 표시 입력으로만 바꾼다.
    RawCampaignFactoryDisplay display(core,assets.game);std::array<ScreenColor,256> palette{};std::array<std::uint32_t,256> logical{}; // 파일 팔레트와 원본 그림자 치환 표 입력이다.
    // 장치 BGR과 원본 R/G/B DWORD를 같은 팔레트에서 만든다.
    for (std::size_t i=0;i<palette.size();++i) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(i));palette[i]={color.blue,color.green,color.red,0};logical[i]=std::uint32_t(color.red)|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); }
    display.SetPalette(palette);const auto colors=BuildSpriteColorMaps(logical);std::size_t compared{},cases{}; // 전체 배경 대신 실제 원본의 불투명 픽셀을 비교한다.
    const auto render=[&](std::uint32_t type,std::uint32_t table,std::uint32_t frame,std::uint8_t extra,std::uint16_t word) {
        auto raw=core.pool.AllocatedBytes(object);raw[10]=static_cast<std::uint8_t>(type);Put(raw,0,table);Put(raw,36,frame);Put(raw,12,word,2);raw[40]=extra;raw[34]=1; // 게임 프레임/경로를 실행하지 않는 명시 표시 상태다.
        CHECK(display.Handles(object));std::vector<std::uint8_t> pixels(512*400,255);const auto sprites=display.Sprites({0,0,512,400},1168,1036,1); // 현재 슬롯의 실제 수집/일반 Draw를 거친다.
        // 동일 객체 본체 항목만 검사한다. 별도 shadow 레이어는 아래에서 독립 비교한다.
        for (const auto& sprite:sprites) if (sprite.order.x==89 && sprite.order.y==116 && sprite.procedure) sprite.procedure->draw(pixels,512,400,{0,0,512,400});
        return pixels;
    };
    // 이미 동결한 원본 훨리베이스 캡처의 실제32개 동작 프레임을 화면 소비자와 대조한다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/sun-aviary-capture-x86.tsv").string().c_str()).rows) {
        if (row[1]!="2" || row[2]!="1") continue;const auto metadata=netstorm::test::rawscene::Split(row[3],',');const int width=std::stoi(metadata[0]),height=std::stoi(metadata[1]); // 원본 캡처는 TYPE hotspot(-2,3)을 추가한 값이다.
        const int left=256-core.types[72].hotspotX-std::stoi(metadata[2])-2,top=240-core.types[72].hotspotY-std::stoi(metadata[3])+3;const auto actual=render(72,0x509930U,static_cast<std::uint32_t>(std::stoul(row[0])),2,0); // 원본 이미지 기준점으로 역변환한다.
        // 기대 픽셀은 제품 합성으로 만들지 않고 원본 격리 캡처의 전체 바이트를 읽는다.
        for (int y=0;y<height;++y) for (int x=0;x<width;++x) { const auto index=static_cast<std::size_t>(y*width+x);const auto value=static_cast<std::uint8_t>(std::stoul(row[4].substr(index*2,2),nullptr,16));if (value==255 || x+left<0 || x+left>=512 || y+top<0 || y+top>=400) continue;CHECK(actual[static_cast<std::size_t>((top+y)*512+left+x)]==value);++compared; }
        ++cases;
    }
    // 썬 디스크 발사기는 고정 본체32 뒤 현재 동작/포구 디스크를 그린다. 캐논128은 추가 그림 없이 현재 전체 그림을 쓴다.
    for (const auto frame:{0U,1U,17U,31U,32U}) for (const auto word:{0U,1U}) {
        const auto actual=render(71,0x50b888U,frame,2,static_cast<std::uint16_t>(word));const auto& asset=assets.game.Find("sunArcher");std::vector<std::uint8_t> expected(512*400,255); // 본체가 빠지면 이 기대와 달라진다.
        const int x=256-core.types[71].hotspotX,y=240-core.types[71].hotspotY; // 원본 일반 Draw의 TYPE 발 기준점이다.
        static_cast<void>(DrawShape(expected,512,400,assets.game.Shapes(),asset.block,32,x,y,{0,0,511,399}));
        if (frame!=32) { static_cast<void>(DrawShape(expected,512,400,assets.game.Shapes(),asset.block,frame,x,y,{0,0,511,399}));if (!word) { const auto& header=core.shapes[71].frames[frame];static_cast<void>(DrawShape(expected,512,400,assets.game.Shapes(),83-70,0,x+header.muzzleX,y+header.muzzleY,{0,0,511,399})); } }
        CHECK(actual==expected);++cases;
    }
    // 별도 그림자 SID를 쓰는 여섯 타입의 첫/마지막 그림자를 모두 같은 원본 소스 색 치환과 기준점으로 대조한다.
    for (const auto [type,table]:std::array<std::array<std::uint32_t,2>,6>{{{73,0x50a1e8U},{111,0x5040c0U},{138,0x50a050U},{141,0x5040c0U},{144,0x50a388U},{146,0x504198U}}}) for (const auto last:{false,true}) {
        const auto count=static_cast<std::uint32_t>(core.shapes[type].frameCount),frame=last ? count*2-1 : count;const auto actual=render(type,table,frame,8,0); // 원본 여섯 생성자 VT와 물리 그림자 번호다.
        std::vector<std::uint8_t> expected(512*400,255);static_cast<void>(DrawMappedShape(expected,512,400,assets.game.Shapes(),type-70,frame,256-core.types[type].hotspotX,240-core.types[type].hotspotY,{0,0,511,399},colors.shadow)); // 일반 원색 출력이면 일치하지 않는다.
        CHECK(actual==expected);++cases;
    }
    CHECK(cases==54 && compared>10000);
}

// 기존 로그인/생산 클릭/허용점/두 제네레이터/캐논 전투에서 HP 주입 없이 실제14를 관찰한다.
TEST_CASE(CampaignBurning_NaturalGeneratorCannonCombat) { NaturalWorkshopPlacement(true,true); }
// 수명 검사는 HP 감소/피해자 삭제를 명시 입력으로 구분하며 자연 전투 완주로 세지 않는다.
TEST_CASE(CampaignBurning_ActualHostRecoveryAndDeletion) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",external.Services());session.Load();auto core=session.Core();const Sid temple{15002};auto raw=core.pool.AllocatedBytes(temple);CHECK(raw[10]==124);const auto maximum=Get(raw,26);Put(raw,26,maximum/4); // 회복·삭제 경로의 명시 HP 입력이다.
    DamageableBurningState burningState;DamageableBurningHooks hooks; // 회복 몸체에 같은 실제 Form/Host 경계를 연결하는 명시 검사 조립이다.
    hooks.find=[&](Sid victim,std::uint32_t type,std::uint32_t index) { CHECK(index==0);return FindAttachedProcess(core.pool,victim,type); };
    hooks.create=[&](Sid victim) { const auto* effect=const_cast<RawCampaignParticles&>(session.Particles()).AddBurning(victim,1);return effect ? effect->Form() : Sid{}; };hooks.destroy=[&](Sid form,std::uint32_t flags) { session.World().Destroy(form,flags); };
    RawDamageableBurning burning(core.pool,core.types,session.HitPoints(),burningState,std::move(hooks));session.SetParticleView({1168,1036,0,0,1024,768},{});const auto form=burning.Refresh(temple);auto* effect=dynamic_cast<BurningVisualProcess*>(core.processes.FromForm(form));CHECK(effect);if (!effect) return;const auto token=effect->Fields().registration.token;std::size_t drawn{};
    for (std::uint64_t frame=1;frame<=180;++frame) { // 실제 Kernel의 gate/초기 캡처/연소점/입자/Renderer를 함께 실행한다.
        session.RunFrame({frame/40.0,100+frame/40.0,.025,frame},false);Renderer renderer(1024,768);renderer.SetScene(session.ParticleSprites({0,0,1024,768}));std::vector<std::uint8_t> pixels(1024*768,255);renderer.Draw(pixels,1024,255);drawn+=static_cast<std::size_t>(std::count_if(pixels.begin(),pixels.end(),[](auto color) { return color!=255; }));
    }
    CHECK(effect->Fields().stage==2 && effect->Fields().total>0 && drawn>0);Put(raw,26,maximum);CHECK(!burning.Refresh(temple).value); // 실제 회복 판정이 기존14를 바로 삭제한다. source의 최소1 규칙을 임의0으로 바꾸지 않는다.
    CHECK(!session.Particles().Rendering().Lookup(token) && !core.processes.FromForm(form));Put(raw,26,maximum/4);const auto second=burning.Refresh(temple);auto* next=dynamic_cast<BurningVisualProcess*>(core.processes.FromForm(second));CHECK(next);if (!next) return;const auto nextToken=next->Fields().registration.token;
    session.World().Destroy(temple,0x800);CHECK(!session.Particles().Rendering().Lookup(nextToken) && !core.processes.FromForm(second));CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth());std::cout<<"실제14 캡처/표시"<<drawn<<"픽셀·회복 삭제·피해 부모 삭제 정리 통과\n";
}

// 원본181입력의 캡처/모든 픽셀을 실제 Session의 파편16 생성 경로에서 대조한다.
// raw/TYPE 기준점만 명시 입력이며 실제 미션 진행이나 썬더 실전 장면 검증으로 세지 않는다.
TEST_CASE(CampaignTempleShrapnel_Native181CapturePixels) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;std::size_t captureSounds{};auto services=external.Services();auto play=services.sound.play;services.sound.play=[&captureSounds,play](auto... args) { ++captureSounds;return play(args...); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));session.Load();auto core=session.Core();const Sid source{15002};auto raw=core.pool.AllocatedBytes(source);const std::vector<std::uint8_t> saved(raw.begin(),raw.end());std::size_t cases{},pixels{};
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_TEMPLE_CAPTURE_FIXTURE).rows) { // 원본 기대 열은 실행 뒤 비교에만 쓴다.
        CHECK(row.size()==10);const auto type=assets.game.TypeTable().Find(row[0]);CHECK(type>=70);std::uint32_t table{}; // 동결 관측의 실제 다섯VT 입력이다.
        if (type==123) table=0x519860;else if (type==124) table=0x5196f0;else if (type==125) table=0x519918;else if (type==71) table=0x50b888;else if (type==128) table=0x50b9b8;else { CHECK(false);return; }
        auto& definition=const_cast<RiftTypeRecord&>(core.types[type]);const auto hotX=definition.hotspotX,hotY=definition.hotspotY;definition.hotspotX=-2;definition.hotspotY=3; // 이 세션만 소유하는 TYPE의 명시 native 입력이다.
        Put(raw,0,table);raw[10]=static_cast<std::uint8_t>(type);raw[11]=0;Put(raw,12,std::stoul(row[2]),2);raw[34]=static_cast<std::uint8_t>(std::stoul(row[4]));raw[40]=static_cast<std::uint8_t>(std::stoul(row[3]));Put(raw,36,std::stoul(row[1]));const auto notices=captureSounds;
        auto* effect=const_cast<RawCampaignParticles&>(session.Particles()).AddShrapnel(source,ShrapnelKind::Explosion);CHECK(effect);if (!effect) return;const auto& image=effect->Fields().image;
        const auto metadata=std::to_string(image.width)+','+std::to_string(image.height)+','+std::to_string(image.hotspotX)+','+std::to_string(image.hotspotY);const bool same=metadata==row[5] && netstorm::test::rawscene::Hex(image.pixels)==row[6] && image.pixels.size()==std::stoul(row[7]);
        if (!same) std::cerr<<"실제 파편 캡처 차이 "<<row[0]<<" frame"<<row[1]<<" word"<<row[2]<<" extra"<<row[3]<<" meta"<<metadata<<" 원본"<<row[5]<<'\n';CHECK(same && row[8]=="1" && captureSounds-notices==std::stoul(row[9]));pixels+=image.pixels.size();++cases;
        const auto form=effect->Form(),dummy=effect->Parent();const auto token=effect->Fields().registration.token;CHECK(core.dependencies.Has(form,dummy));core.processes.Kill(*effect,0);CHECK(!session.Particles().Rendering().Lookup(token) && (core.pool.Slot(dummy)[11]&1U));definition.hotspotX=hotX;definition.hotspotY=hotY;std::copy(saved.begin(),saved.end(),raw.begin());
    }
    CHECK(cases==181 && pixels==2684495);std::cout<<"실제 Session 파편 캡처: 합성"<<cases<<"입력/원본"<<pixels<<"픽셀 일치·모든 의존 더미 정리\n";
}
// 저장된 실제 윈드/레인 템플의 폭발 삭제와 같은 Kernel의 자연 파편 소진을 확인한다.
TEST_CASE(CampaignTempleShrapnel_ActualWindRainExplosionLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto mission:{"tutorial5","thewarbegins"}) { // 썬더는 위 명시 캡처 입력이며 이 실제 장면 둘에 포함하지 않는다.
        SessionServicesFixture external;auto services=external.Services();services.deletion={};services.deletion.selected=[] { return Sid{}; };services.dependencyTransmit={};services.formPacket=[&](std::span<const std::uint8_t> packet) { external.packets.emplace_back(packet.begin(),packet.end()); };
        ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");MissionScript script(config,mission); // 미션 이름과 저장fort 이름을 동일시하지 않고 실제 지정을 읽는다.
        auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.Load();auto core=session.Core();Sid source; // 실제 보호 미션의 현재 owner1 템플이다.
        for (const auto& object:session.World().Objects()) if (object.object.owner==1 && object.object.type==(std::string_view(mission)=="tutorial5" ? 123U : 124U)) { CHECK(!source.value);source=object.sid; } // 검사에서 템플을 만들지 않는다.
        CHECK(source.value);if (!source.value) return;const auto before=core.pool.Slot(source);const auto x=std::bit_cast<float>(Get(before,14)),y=std::bit_cast<float>(Get(before,18));const auto depth=std::bit_cast<std::int8_t>(before[35]);Renderer renderer(1024,768);session.SetParticleView({static_cast<int>(x*16)-512,static_cast<int>(y*11)-384,0,0,1024,768},{[&] { return renderer.FullRedrawPending(); },[&](SquidDisplayRect rect,std::uint32_t flags) { renderer.Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }});
        session.World().Destroy(source,0x100000);CHECK(core.pool.Slot(source)[11]&1U);ShrapnelVisualProcess* effect{};
        for (const auto token:session.Particles().Rendering().Slots()) { const auto* f=session.Particles().Rendering().Lookup(token);if (!f) continue;auto* candidate=dynamic_cast<ShrapnelVisualProcess*>(core.processes.FromForm({static_cast<std::uint16_t>(f->form)}));if (candidate && f->zOrder==depth) { CHECK(!effect);effect=candidate; } } // 실제16/더미 쌍만 선택한다.
        CHECK(effect);if (!effect) return;const auto form=effect->Form(),dummy=effect->Parent();const auto token=effect->Fields().registration.token;CHECK(core.dependencies.Has(form,dummy) && effect->Fields().total>0);std::size_t drawn{};
        for (std::uint64_t frame=1;frame<=160;++frame) { // 실제4초 동안 중력/색 출력/소진/의존 더미 삭제까지 진행한다.
            session.RunFrame({frame*.025,100+frame*.025,.025,frame},false);renderer.SetScene(session.ParticleSprites({0,0,1024,768}));std::vector<std::uint8_t> pixels(1024*768,255);renderer.Draw(pixels,1024,255);renderer.Present([](ScreenRect) {});drawn+=static_cast<std::size_t>(std::count_if(pixels.begin(),pixels.end(),[](auto color) { return color!=255; }));
        }
        CHECK(drawn>0 && !session.Particles().Rendering().Lookup(token) && !core.processes.FromForm(form) && (core.pool.Slot(dummy)[11]&1U));CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);session.SetParticleView({0,0,0,0,1024,768},{});std::cout<<mission<<" 실제 템플 폭발/파편 출력"<<drawn<<"픽셀·자연 소진/더미 정리 통과\n";
    }
}

namespace {
// 실제 원본 데이터에서 생성한 분배 표를 검사에도 같은 현재 상수로 읽는다.
#include "client/CampaignDamageableRegularTables.inc"
}
// 새 함수 실행 수에 넣지 않는 실제83VT 데이터와 세 현재 C++ 분배 범위를 전수 대조한다.
TEST_CASE(CampaignTempleShrapnel_Native83RegularTables) {
    std::size_t count{};
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_DAMAGEABLE_REGULAR_TABLES_FIXTURE).rows) { // 기대 주소는 실제 PE 데이터에서만 읽는다.
        const auto table=static_cast<std::uint32_t>(std::stoul(row[0])),hit=static_cast<std::uint32_t>(std::stoul(row[2]));CHECK((std::find(kCommonDamageableRegularTables.begin(),kCommonDamageableRegularTables.end(),table)!=kCommonDamageableRegularTables.end())==(row[3]=="1"));CHECK((std::find(kGeneralDamageableHitTables.begin(),kGeneralDamageableHitTables.end(),table)!=kGeneralDamageableHitTables.end())==(hit==0x44c8e0U));CHECK((std::find(kBaseDamageableHitTables.begin(),kBaseDamageableHitTables.end(),table)!=kBaseDamageableHitTables.end())==(hit==0x4add10U));++count;
    }
    CHECK(count==83);
}
// 폭발이 예약한6699/count의 실제 공통 Handle→지면 효과→가상98→HP/연소를 같은 Kernel에서 검증한다.
TEST_CASE(CampaignTempleShrapnel_QueuedDamageOwnsActualHpAndBurning) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",external.Services());session.Load();auto core=session.Core();const Sid target{15006};CHECK(core.pool.Slot(target)[10]==71 && Get(core.pool.Slot(target),26)==300);
    auto* request=core.processes.AddRegular(target,6699,0);CHECK(request);if (!request) return;request->SetInitialCount(65687); // 실제 창에서 관찰한owner1/피해151의 명시 예약 입력이다. 결과 HP는 쓰지 않는다.
    session.RunFrame({.1,100.1,.1,1},false);CHECK(Get(core.pool.Slot(target),26)==149);const auto form=FindAttachedProcess(core.pool,target,14);CHECK(form.value);const auto* burning=dynamic_cast<BurningVisualProcess*>(core.processes.FromForm(form));CHECK(burning && burning->Fields().victim==target && burning->Fields().mode==1);std::cout<<"실제6699 예약: HP300→149/모드1 연소14 생성 통과\n";
}
// 같은 두 제네레이터 뒤 적 발사기 사정거리 바깥의 합법 캐논 후보를 자연 건설/전투로 확인한다.
TEST_CASE(CampaignTempleShrapnel_NaturalSafeCannonCombat) { NaturalWorkshopPlacement(true,true,{116,118}); }

// 실제 파편 생성이 반환한 세 제네레이터의 모든 픽셀을 제품 전에 동결한180입력과 대조한다.
TEST_CASE(CampaignGeneratorShrapnel_Native180CapturePixels) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::size_t soundCalls{};auto play=services.sound.play;services.sound.play=[&soundCalls,play](auto... args) { ++soundCalls;return play(args...); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));session.Load();auto core=session.Core();const Sid source{15002};auto raw=core.pool.AllocatedBytes(source);const std::vector<std::uint8_t> saved(raw.begin(),raw.end());std::size_t cases{},pixels{};
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_GENERATOR_CAPTURE_FIXTURE).rows) { // 기대 픽셀은 실행 뒤 비교에만 사용하며 현재 결과를 주입하지 않는다.
        CHECK(row.size()==9);const auto type=assets.game.TypeTable().Find(row[0]);CHECK(type>=75 && type<=77 && assets.game.Types()[type-70].definition.specialFrames.baseFrame==-1);
        auto& definition=const_cast<RiftTypeRecord&>(core.types[type]);const auto hotX=definition.hotspotX,hotY=definition.hotspotY;definition.hotspotX=-2;definition.hotspotY=3; // 동결 원본과 같은 이 세션 소유 TYPE의 명시 기준점이다.
        Put(raw,0,0x501ea0U);raw[10]=static_cast<std::uint8_t>(type);raw[11]=0;Put(raw,12,std::stoul(row[2]),2);raw[34]=static_cast<std::uint8_t>(std::stoul(row[4]));raw[40]=static_cast<std::uint8_t>(std::stoul(row[3]));Put(raw,36,std::stoul(row[1]));const auto notices=soundCalls;
        auto* effect=const_cast<RawCampaignParticles&>(session.Particles()).AddShrapnel(source,ShrapnelKind::Explosion);CHECK(effect);if (!effect) return;const auto& image=effect->Fields().image;
        const auto metadata=std::to_string(image.width)+','+std::to_string(image.height)+','+std::to_string(image.hotspotX)+','+std::to_string(image.hotspotY);CHECK(metadata==row[5] && netstorm::test::rawscene::Hex(image.pixels)==row[6] && image.pixels.size()==std::stoul(row[7]) && row[8]=="1" && notices==soundCalls);pixels+=image.pixels.size();++cases;
        const auto form=effect->Form(),dummy=effect->Parent();const auto token=effect->Fields().registration.token;CHECK(core.dependencies.Has(form,dummy));core.processes.Kill(*effect,0);CHECK(!session.Particles().Rendering().Lookup(token) && (core.pool.Slot(dummy)[11]&1U));definition.hotspotX=hotX;definition.hotspotY=hotY;std::copy(saved.begin(),saved.end(),raw.begin());
    }
    CHECK(cases==180 && pixels==1844370);std::cout<<"실제 제네레이터 파편 캡처: 합성"<<cases<<"입력/원본"<<pixels<<"픽셀 일치·모든 의존 더미 정리\n";
}

// 실제1-1 저장 윈드 제네레이터에 명시 폭발을 요청하고 같은 Kernel의 파편 소진/후처리를 검사한다.
TEST_CASE(CampaignGeneratorShrapnel_ActualSavedWindExplosionLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.deletion={};services.deletion.selected=[] { return Sid{}; };services.dependencyTransmit={};services.formPacket=[&](std::span<const std::uint8_t> packet) { external.packets.emplace_back(packet.begin(),packet.end()); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));session.Load();auto core=session.Core();const Sid source{15014};const auto before=core.pool.Slot(source);CHECK(before[10]==75 && before[34]==2 && Get(before,26)==700);
    const auto x=std::bit_cast<float>(Get(before,14)),y=std::bit_cast<float>(Get(before,18));Renderer renderer(1024,768);session.SetParticleView({static_cast<int>(x*16)-512,static_cast<int>(y*11)-384,0,0,1024,768},{[&] { return renderer.FullRedrawPending(); },[&](SquidDisplayRect rect,std::uint32_t flags) { renderer.Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }});
    session.World().Destroy(source,0x100000);CHECK(core.pool.Slot(source)[11]&1U);ShrapnelVisualProcess* effect{};
    for (const auto token:session.Particles().Rendering().Slots()) { // 파괴 직후 생성된 실제16/110 쌍만 선택한다.
        const auto* fields=session.Particles().Rendering().Lookup(token);if (!fields) continue;auto* candidate=dynamic_cast<ShrapnelVisualProcess*>(core.processes.FromForm({static_cast<std::uint16_t>(fields->form)}));if (candidate) { CHECK(!effect);effect=candidate; }
    }
    CHECK(effect);if (!effect) return;const auto form=effect->Form(),dummy=effect->Parent();const auto token=effect->Fields().registration.token;CHECK(effect->Fields().total>0 && core.dependencies.Has(form,dummy));std::size_t drawn{};
    for (std::uint64_t frame=1;frame<=160;++frame) { // 실제4초 동안 중력/출력/자연 소진과 의존 더미 삭제를 진행한다.
        session.RunFrame({frame*.025,100+frame*.025,.025,frame},false);renderer.SetScene(session.ParticleSprites({0,0,1024,768}));std::vector<std::uint8_t> pixels(1024*768,255);renderer.Draw(pixels,1024,255);renderer.Present([](ScreenRect) {});drawn+=static_cast<std::size_t>(std::count_if(pixels.begin(),pixels.end(),[](auto color) { return color!=255; }));
    }
    CHECK(drawn>0 && !session.Particles().Rendering().Lookup(token) && !core.processes.FromForm(form) && (core.pool.Slot(dummy)[11]&1U));CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);session.SetParticleView({0,0,0,0,1024,768},{});std::cout<<"실제 저장 윈드 제네레이터 폭발: 전체 입자"<<drawn<<"픽셀·자연 소진/의존 더미 정리 통과\n";
}

// 같은 실제 초기 자원/연구/생산으로 전방 공급과 두 번째 캐논을 건설해 적 신전까지 자연 공략한다.
TEST_CASE(CampaignForwardBattle_NaturalTempleAssault) { NaturalWorkshopPlacement(true,true,{116,118},true); }

// 실제 자금/연구/제네레이터·캐논 건설 뒤 같은 로컬54h가 환불/붕괴/삭제를 실행한다.
TEST_CASE(CampaignSalvage_NaturalCannonActualTransportRefundCollapse) { NaturalWorkshopPlacement(true,false,{118,118},false,true); }

// 실제 파편 생성이 반환한 스톤/아이스 타워의 모든 픽셀을 제품 전에 동결한120입력과 대조한다.
TEST_CASE(CampaignForwardBattle_Native120CapturePixels) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::size_t soundCalls{};auto play=services.sound.play;services.sound.play=[&soundCalls,play](auto... args) { ++soundCalls;return play(args...); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));session.Load();auto core=session.Core();const Sid source{15002};auto raw=core.pool.AllocatedBytes(source);const std::vector<std::uint8_t> saved(raw.begin(),raw.end());std::size_t cases{},pixels{};
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_BLOCKER_CAPTURE_FIXTURE).rows) { // 기대 픽셀은 실행 뒤 비교에만 사용하며 현재 결과를 주입하지 않는다.
        CHECK(row.size()==9);const auto type=assets.game.TypeTable().Find(row[0]);CHECK((type==78 || type==147) && assets.game.Types()[type-70].definition.specialFrames.baseFrame==-1);
        auto& definition=const_cast<RiftTypeRecord&>(core.types[type]);const auto hotX=definition.hotspotX,hotY=definition.hotspotY;definition.hotspotX=-2;definition.hotspotY=3; // 동결 원본과 같은 이 세션 소유 TYPE의 명시 기준점이다.
        Put(raw,0,0x50e940U);raw[10]=static_cast<std::uint8_t>(type);raw[11]=0;Put(raw,12,std::stoul(row[2]),2);raw[34]=static_cast<std::uint8_t>(std::stoul(row[4]));raw[40]=static_cast<std::uint8_t>(std::stoul(row[3]));Put(raw,36,std::stoul(row[1]));const auto notices=soundCalls;
        auto* effect=const_cast<RawCampaignParticles&>(session.Particles()).AddShrapnel(source,ShrapnelKind::Explosion);CHECK(effect);if (!effect) return;const auto& image=effect->Fields().image;
        const auto metadata=std::to_string(image.width)+','+std::to_string(image.height)+','+std::to_string(image.hotspotX)+','+std::to_string(image.hotspotY);CHECK(metadata==row[5] && netstorm::test::rawscene::Hex(image.pixels)==row[6] && image.pixels.size()==std::stoul(row[7]) && row[8]=="1" && notices==soundCalls);pixels+=image.pixels.size();++cases;
        const auto form=effect->Form(),dummy=effect->Parent();const auto token=effect->Fields().registration.token;CHECK(core.dependencies.Has(form,dummy));core.processes.Kill(*effect,0);CHECK(!session.Particles().Rendering().Lookup(token) && (core.pool.Slot(dummy)[11]&1U));definition.hotspotX=hotX;definition.hotspotY=hotY;std::copy(saved.begin(),saved.end(),raw.begin());
    }
    CHECK(cases==120 && pixels==1317660);std::cout<<"실제 타워 파편 캡처: 합성"<<cases<<"입력/원본"<<pixels<<"픽셀 일치·모든 의존 더미 정리\n";
}


// 실제1-1 저장 스톤 타워에 명시 폭발을 요청하고 같은 Kernel의 파편 소진/후처리를 검사한다.
TEST_CASE(CampaignForwardBattle_ActualSavedStoneExplosionLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.deletion={};services.deletion.selected=[] { return Sid{}; };services.dependencyTransmit={};services.formPacket=[&](std::span<const std::uint8_t> packet) { external.packets.emplace_back(packet.begin(),packet.end()); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));session.Load();auto core=session.Core();const Sid source{15019};const auto before=core.pool.Slot(source);CHECK(before[10]==78 && before[34]==2 && Get(before,26)==2000);
    const auto x=std::bit_cast<float>(Get(before,14)),y=std::bit_cast<float>(Get(before,18));Renderer renderer(1024,768);session.SetParticleView({static_cast<int>(x*16)-512,static_cast<int>(y*11)-384,0,0,1024,768},{[&] { return renderer.FullRedrawPending(); },[&](SquidDisplayRect rect,std::uint32_t flags) { renderer.Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }});
    session.World().Destroy(source,0x100000);CHECK(core.pool.Slot(source)[11]&1U);ShrapnelVisualProcess* effect{};
    for (const auto token:session.Particles().Rendering().Slots()) { // 파괴 직후 생성된 실제16/110 쌍만 선택한다.
        const auto* fields=session.Particles().Rendering().Lookup(token);if (!fields) continue;auto* candidate=dynamic_cast<ShrapnelVisualProcess*>(core.processes.FromForm({static_cast<std::uint16_t>(fields->form)}));if (candidate) { CHECK(!effect);effect=candidate; }
    }
    CHECK(effect);if (!effect) return;const auto form=effect->Form(),dummy=effect->Parent();const auto token=effect->Fields().registration.token;CHECK(effect->Fields().total>0 && core.dependencies.Has(form,dummy));std::size_t drawn{};
    for (std::uint64_t frame=1;frame<=160;++frame) { // 실제4초 동안 중력/출력/자연 소진과 의존 더미 삭제를 진행한다.
        session.RunFrame({frame*.025,100+frame*.025,.025,frame},false);renderer.SetScene(session.ParticleSprites({0,0,1024,768}));std::vector<std::uint8_t> pixels(1024*768,255);renderer.Draw(pixels,1024,255);renderer.Present([](ScreenRect) {});drawn+=static_cast<std::size_t>(std::count_if(pixels.begin(),pixels.end(),[](auto color) { return color!=255; }));
    }
    CHECK(drawn>0 && !session.Particles().Rendering().Lookup(token) && !core.processes.FromForm(form) && (core.pool.Slot(dummy)[11]&1U));CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);session.SetParticleView({0,0,0,0,1024,768},{});std::cout<<"실제 저장 스톤 타워 폭발: 전체 입자"<<drawn<<"픽셀·자연 소진/의존 더미 정리 통과\n";
}

// 실제 자산의 명시 Dais/제단/포획 Form 입력 뒤 UI 자격과18BYTE→서버40h→같은54 경로를 확인한다. 자연 포획/최초Pop/희생 완료 검사는 아니다.
namespace {
// 같은 명시 상태의 초기 의식 회귀와 후속 Kernel 진행을 구분한다. false는 기존 입력/종료점을 그대로 사용한다.
void ActualSacrificeSession(bool followup) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();netstorm::zacket::LocalMissionTransport* wire{};std::vector<std::string> music;std::vector<std::uint32_t> income;std::size_t incomeSounds{};double wall=100;std::vector<SoundHandle> globalSounds;std::unique_ptr<Renderer> visual;std::vector<std::uint8_t> visualPixels;std::size_t drawn{}; // 실제 자산과 명시 장치/전송 관찰이다.
    services.deletion.selected=[] { return Sid{}; };services.carrierRequest=[&](std::int32_t owner,std::span<const std::uint8_t> packet) { CHECK(wire);wire->Send(owner,packet); };services.requestMusic=[&](std::string_view name) { music.emplace_back(name); };
    services.contentLifetime.localNotice=[&](std::uint32_t command) { income.push_back(command); };services.contentLifetime.sound=[&](float x,float y) { CHECK(std::isfinite(x) && std::isfinite(y));++incomeSounds; }; // 실제 클라이언트의 입력 큐/장치에 해당하는 명시 관찰이다. 지갑/SP는 실제 Reward를 사용한다.
    if (followup) { services.sound.wallSeconds=[&] { return wall; };const auto original=services.sound.play;services.sound.play=[&,original](SoundHandle sound,std::uint32_t loop,std::uint32_t volume,std::uint32_t pan,std::uint32_t priority,std::uint32_t limit) { globalSounds.push_back(sound);return original(sound,loop,volume,pan,priority,limit); }; } // 지연 음향의 실제 장치 요청/시계만 관찰한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto priestRaw=core.pool.Slot(priest);const auto& daisHotspot=core.shapes[165].frames.at(3);const auto& runeHotspot=core.shapes[166].frames.at(0); // 실제0단계 좌표를 결정하는 두 SHP 헤더다.
    const auto graphAt=[&](float x,float y) { const auto sid=core.hash.Entries(0)[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)];return sid ? std::uint32_t(core.pool.Slot(Sid{sid})[30]) : 254U; }; // 현재 원본 surface/Graph 입력만 읽는다.
    const auto sourceGraph=graphAt(std::bit_cast<float>(Get(priestRaw,14)),std::bit_cast<float>(Get(priestRaw,18)));RawPathPoint point{};double nearest=1e9; // 같은 현재 Graph에서 장소와0단계 목표점이 모두 있는 입력을 고른다.
    for (int y=90;y<120;++y) { // 원래 사제의 시작 섬 주변을 검사한다.
        for (int x=85;x<120;++x) { // 길/배치 성공을 주입하지 않고 실제 지면/Graph로 입력 위치만 고른다.
            const float tx=static_cast<float>(double(x)+double(daisHotspot.muzzleX)/16.0+double(runeHotspot.muzzleX)/16.0),ty=static_cast<float>(double(y)+double(daisHotspot.muzzleY)/11.0+double(runeHotspot.muzzleY)/11.0);const auto cell=static_cast<std::size_t>(ty)*256+static_cast<std::size_t>(tx); // 실제 단계0 SHP 목표점의 입력 기하다.
            if (graphAt(float(x),float(y))!=sourceGraph || graphAt(tx,ty)!=sourceGraph || !(core.spots[y*256+x]&6U) || (core.spots[y*256+x]&8U) || !(core.spots[cell]&6U) || (core.spots[cell]&8U)) continue;
            const double dx=double(tx)-std::bit_cast<float>(Get(priestRaw,14)),dy=double(ty)-std::bit_cast<float>(Get(priestRaw,18)),distance=dx*dx+dy*dy;if (distance<1 || distance>=nearest) continue;point={float(x),float(y)};nearest=distance;
        }
    }
    CHECK(nearest<1e9);if (nearest>=1e9) return;const auto sp=session.Sp(1); // 현재 결과 HP/자원/길은 바꾸지 않는다.
    std::cout<<"의식 입력: 사제"<<std::bit_cast<float>(Get(priestRaw,14))<<","<<std::bit_cast<float>(Get(priestRaw,18))<<"/Dais"<<point.x<<","<<point.y<<"/Graph"<<sourceGraph<<"\n";
    const auto input=[&](std::uint32_t type,std::int32_t frame) { // 최초Pop 미연결과 구별하는 합성 현재 raw/해시 입력이다. 원본 자산 표는 수정하지 않는다.
        const auto sid=core.factory.Create(type,0);auto raw=core.pool.AllocatedBytes(sid);raw[11]&=static_cast<std::uint8_t>(~4U);raw[34]=1;Put(raw,14,std::bit_cast<std::uint32_t>(point.x));Put(raw,18,std::bit_cast<std::uint32_t>(point.y));Put(raw,36,static_cast<std::uint32_t>(frame));const auto& header=core.shapes[type].frames.at(static_cast<std::size_t>(frame));const auto level=SquidHash::ObjectLevel(core.types[type].flags2,header.cellWidth,header.cellHeight);raw[33]=static_cast<std::uint8_t>(level);auto& head=core.hash.Bucket(level,point.x,point.y);Put(raw,4,head,2);head=sid.value;return sid;
    };
    std::cout<<"제단 실제 프레임: 개수"<<core.frames[164].Codes().size()<<" P"<<core.frames[164].Run('P').first<<" K"<<core.frames[164].Run('K').first<<" L"<<core.frames[164].Run('L').first<<"\n"; // 실제 자산의 입력 프레임 구성 진단이다.
    const auto dais=input(165,3),altar=input(164,0);const auto captured=core.factory.CreateContained(0);auto capturedRaw=core.pool.AllocatedBytes(captured);capturedRaw[11]=0x10;Put(capturedRaw,12,2,2);Put(capturedRaw,16,dais.value,2);Put(capturedRaw,18,158,2);Put(capturedRaw,20,32,2);Put(core.pool.AllocatedBytes(dais),6,captured.value,2); // 포획 내용물의 원래owner2/kind158/mask32 명시 입력이다.
    if (followup) { Put(core.pool.AllocatedBytes(altar),12,2,2);Put(core.pool.AllocatedBytes(altar),36,static_cast<std::uint32_t>(core.frames[164].Run('O').first)); } // 후속 입력은 실제 포획 통지 뒤의 원래owner2/제단O 상태를 명시한다.
    auto ui=session.MakeCarrierInput({});auto target=point;CHECK(ui->Evaluate(priest,target)==0x11 && target.x==point.x && target.y==point.y);
    Put(capturedRaw,20,0,2);target=point;CHECK(ui->Evaluate(priest,target)!=0x11);Put(capturedRaw,20,32,2); // 포획 대상 없이는 의식 자격이 나지 않는다.
    const auto ready=Get(core.pool.Slot(altar),36);Put(core.pool.AllocatedBytes(altar),36,static_cast<std::uint32_t>(core.frames[164].Run('L').first));target=point;CHECK(ui->Evaluate(priest,target)!=0x11);Put(core.pool.AllocatedBytes(altar),36,ready); // 실제 존재하는L 구간의 제단도 거부한다. K 구간은 이 실제 자산에 없다.
    target=point;CHECK(ui->Evaluate(priest,target)==0x11);const auto nugget=session.CreateContained(priest,154,0,32,0x10,0);CHECK(nugget.value);session.RequestCarrierMove(priest,target,1);CHECK(link.recorded.empty() && music.empty() && !FindAttachedProcess(core.pool,priest,54).value);link.transport.Flush();CHECK(link.transport.Poll()==1 && link.recorded.size()==1);
    if (!link.recorded.empty()) { const auto& packet=link.recorded.front();CHECK(packet.size()==18 && packet[1]==0x40 && Get(packet,14)==1); }
    const auto form=FindAttachedProcess(core.pool,priest,54);auto* path=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(path);if (path) { CHECK(path->Runtime().MoveAwayDepth()==1 && Get(path->Runtime().Bytes(),0x186e4)==dais.value && Get(path->Runtime().Bytes(),0x186e8)==0);CHECK(path->Runtime().Count()>0);std::cout<<"실제 희생 요청: UI17/40h/54 경로 깊이1·목표Dais"<<dais.value<<"/"<<path->Runtime().Count()<<"점\n"; }
    CHECK((core.pool.Slot(nugget)[11]&3U)!=0); // wrapper가 같은 실제 내용물 조회/삭제로 너겟 Form을 제거한다.
    if (path) { // 결과 좌표를 쓰지 않고 같은 Kernel을25ms씩 진행해 도착/PathCleanup→의식을 실행한다.
        const auto began=core.processState.now;std::uint64_t steps{}; // 시작 명령의 현재 시각과 실제 실행 횟수다.
        for (std::uint64_t step=1;step<=4000 && core.processes.FromForm(form)==path;++step) { // 예상 끝 시각으로 한 번 건너뛰면 아직 걷기/회전이 끝나지 않을 수 있다.
            const auto now=began+double(step)*.025;session.RunFrame({now,100+now,.025,step},false);steps=step;
        }
        CHECK(!core.processes.FromForm(form));CHECK(core.processes.FindEvent(dais,0x307));CHECK(Get(core.pool.Slot(priest),36)==static_cast<std::uint32_t>(core.frames[158].Run('K').first));std::cout<<"명시 제단 입력 뒤 도착 관측: "<<steps<<"프레임/사제frame"<<Get(core.pool.Slot(priest),36)<<"/307:"<<(core.processes.FindEvent(dais,0x307)!=nullptr)<<"/DaisExtra"<<std::uint32_t(core.pool.Slot(dais)[40])<<"/DaisHash"<<std::uint32_t(core.pool.Slot(dais)[33])<<"/사제위치"<<std::bit_cast<float>(Get(core.pool.Slot(priest),14))<<","<<std::bit_cast<float>(Get(core.pool.Slot(priest),18))<<"\n";
    }
    CHECK(music==std::vector<std::string>{"sacrifice.mus"} && session.Sp(1)==sp+core.types[154].cost && income==std::vector<std::uint32_t>{0x020000b4} && incomeSounds==1);CHECK((core.pool.Slot(priest)[11]&3U)==0 && !link.transport.Server().depth);
    if (followup) {
        visual=std::make_unique<Renderer>(1024,768);visualPixels.resize(1024*768);session.SetParticleView({static_cast<int>(point.x*16)-512,static_cast<int>(point.y*11)-384,0,0,1024,768},{[&] { return visual->FullRedrawPending(); },[&](SquidDisplayRect rect,std::uint32_t flags) { visual->Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }}); // Rune 건설/효과가 실제 캡처·렌더러까지 연결되는지 검사한다.
        const auto began=core.processState.now;auto lastFrame=Get(core.pool.Slot(dais),36);std::size_t born{},retired{};bool live{};std::array<bool,5> bands{};double readyAt{};std::array<Sid,2> delayed{}; // 실제 Rune 수명/의식 코드/완료 지연음을 추적한다.
        RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("소리 관찰에서 Form 삭제 요청"); });
        for (std::uint64_t step=1;step<=8000;++step) { // 최대200초를 같은 Kernel에서 진행하며 HP/단계/음향 결과를 쓰지 않는다.
            const auto now=began+double(step)*.025;wall=100+now;
            try { session.RunFrame({now,wall,.025,5000+step},false); }
            catch (...) { std::cout<<"의식 후속 오류: step"<<step<<"/game"<<now<<"/Daisframe"<<Get(core.pool.Slot(dais),36)<<"/사제frame"<<Get(core.pool.Slot(priest),36)<<"\n";throw; }
            visual->SetScene(session.ParticleSprites({0,0,1024,768}));std::fill(visualPixels.begin(),visualPixels.end(),std::uint8_t{255});visual->Draw(visualPixels,1024,255);visual->Present([](ScreenRect) {});drawn+=static_cast<std::size_t>(std::count_if(visualPixels.begin(),visualPixels.end(),[](auto color) { return color!=255; })); // 전체 입자 표면 출력이며 Rune만의 원본 픽셀 일치 판정은 아니다.
            const auto frame=Get(core.pool.Slot(dais),36);const auto runes=ConstructionLive(core,166);const bool current=!runes.empty();if (current && !live) ++born;if (!current && live) ++retired;live=current; // SID 재사용과 별개로 생성/삭제 전이를 센다.
            const auto direction=RawPathAnimation::Direction(core.frames[158],static_cast<std::int32_t>(Get(core.pool.Slot(priest),36)));if (current && direction>=10 && direction<=14) bands[static_cast<std::size_t>(direction-10)]=true;
            if (frame!=lastFrame || (step%20==0 && current)) std::cout<<"의식 후속 game"<<now<<"/Dais"<<frame<<"/Rune"<<runes.size()<<"/생성"<<born<<"/삭제"<<retired<<"/사제"<<Get(core.pool.Slot(priest),36)<<"\n";lastFrame=frame;
            if (!readyAt && born==5 && retired==5) {
                CHECK(frame==static_cast<std::uint32_t>(core.frames[165].Run('N').first) && std::all_of(bands.begin(),bands.end(),[](bool seen) { return seen; }) && !core.processes.FindEvent(priest,604));readyAt=now;
                delayed={lookup.Find(dais,47,0),lookup.Find(dais,47,1)};CHECK(delayed[0].value && delayed[1].value && !lookup.Find(dais,47,2).value);std::vector<double> starts;
                for (const auto soundForm:delayed) { // 실제 붙은47의 두 절대 시작 시각과 flags를 확인한다.
                    auto* sound=dynamic_cast<SoundProcess*>(core.processes.FromForm(soundForm));CHECK(sound && sound->Parent()==dais && sound->Flags()==0xc && !sound->Current());if (sound) starts.push_back(sound->StartTime());
                }
                std::sort(starts.begin(),starts.end());CHECK(starts.size()==2);if (starts.size()==2) { CHECK_NEAR(starts[0],wall+2.799999952316284,1e-12);CHECK_NEAR(starts[1],wall+4.0,1e-12); }
                std::cout<<"실제 Rune5 생성/완공·삭제/K..O/미션 제단이탈N·두 지연음 예약 game"<<now<<"\n";
            }
            if (readyAt && now>readyAt+4.5) { // 실제 벽시계 이후 한 번 재생과 Sound47 자연 삭제까지 관찰한다.
                CHECK(!core.processes.FromForm(delayed[0]) && !core.processes.FromForm(delayed[1]));CHECK(std::count(globalSounds.begin(),globalSounds.end(),external.names.Lookup("priestSacrifice2.wav"))==1);CHECK(std::count(globalSounds.begin(),globalSounds.end(),external.names.Lookup("thunderCrack.wav"))==1);CHECK(Get(core.pool.Slot(dais),36)==static_cast<std::uint32_t>(core.frames[165].Run('N').first+2) && (core.pool.Slot(captured)[11]&3U)==0);CHECK(drawn>0);std::cout<<"의식 룬 단계 완료/두 지연음 재생·자연 소진/전체 입자 표면"<<drawn<<"픽셀, 최종 희생 완료는 별도 실제 건설 검사\n";session.SetParticleView({0,0,0,0,1024,768},{});return;
            }
        }
        CHECK(readyAt>0);
    }
}
}
// 기존 초기 의식 회귀의 입력/종료점은 유지한다.
TEST_CASE(CampaignSacrifice_ActualUiAndLocalPacketToRitualPath) { ActualSacrificeSession(false); }
// 동일한 실제 명령/Kernel 뒤307 지연부터 Rune/단계 진행을 검사한다. 자연 포획/최초Pop 완료는 아니다.
TEST_CASE(CampaignRitualProgress_Actual307RuneConstruction) { ActualSacrificeSession(true); }

// 실제 보호 SHP의 표시 기준점과 다른 의식 목표점으로 독립 원본30입력/Anchor·Point120반환을 대조한다.
TEST_CASE(CampaignSacrifice_Native30ActualShapeOffsets) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();auto core=session.Core();const auto dais=core.factory.Create(165,0);auto raw=core.pool.AllocatedBytes(dais);raw[34]=1;Put(raw,36,3); // 원본 관측과 같은 명시 count1/frame3 부모 입력이다.
    PriestRitualActionHooks action; // 이 좌표 조회에서 프레임/생성/장치 요청은 허용하지 않는다.
    action.setFrame=[](Sid,std::int32_t,std::uint32_t) { throw std::logic_error("좌표 검사에서 의식 동작 요청"); };action.reserveShared=[]()->bool { throw std::logic_error("좌표 검사에서 예약 요청"); };action.addShared=[](Sid,std::uint32_t,float) { throw std::logic_error("좌표 검사에서 부착 요청"); };RawPriestRitualActions actions(core.pool,core.frames,std::move(action));
    TempleRitualHooks hooks; // 순수 Anchor/Point 실행 이외 하위 효과가 호출되면 실패한다.
    hooks.create=[](std::uint32_t,std::uint32_t)->Sid { throw std::logic_error("좌표 검사에서 생성 요청"); };hooks.setHitPoints=[](Sid,std::int32_t) { throw std::logic_error("좌표 검사에서 HP 요청"); };hooks.setOwner=[](Sid,std::uint32_t) { throw std::logic_error("좌표 검사에서 owner 요청"); };hooks.pop=[](Sid,float,float,std::uint32_t) { throw std::logic_error("좌표 검사에서 Pop 요청"); };hooks.playAt=[](float,float,std::string_view,std::uint32_t) { throw std::logic_error("좌표 검사에서 장치 요청"); };hooks.reserveShared=[]()->bool { throw std::logic_error("좌표 검사에서 예약 요청"); };hooks.addShared=[](Sid,std::uint32_t,float) { throw std::logic_error("좌표 검사에서 부착 요청"); };
    TempleRitualState state;std::vector<std::uint8_t> spots(65536);RawTempleRitual ritual(core.pool,core.hash,core.types,core.frames,core.shapes,spots,core.neighbors,session.HitPoints(),actions,state,std::move(hooks));std::size_t count{}; // 실제 원본 헤더/TYPE와 명시 균일 지면 입력을 사용한다.
    const auto bytes=[](TempleRitualPoint point) { std::array<std::uint8_t,8> result{};Put(result,0,std::bit_cast<std::uint32_t>(point.x));Put(result,4,std::bit_cast<std::uint32_t>(point.y));return netstorm::test::rawscene::Hex(result); }; // x87의 실제 단정도 저장 바이트를 비교한다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_RITUAL_SHAPE_OFFSETS_FIXTURE).rows) { // 기대 좌표는 원본 fixture에서만 읽는다.
        CHECK(row.size()==6);if (row.size()!=6) continue;const auto stage=netstorm::test::rawscene::Number(row[0]);std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(netstorm::test::rawscene::Number(row[1])));Put(raw,14,netstorm::test::rawscene::Number(row[2]));Put(raw,18,netstorm::test::rawscene::Number(row[3]));CHECK(bytes(ritual.StageAnchor(dais,stage))==row[4]);CHECK(bytes(ritual.StagePoint(dais,stage))==row[5]);++count;
    }
    CHECK(count==30);CHECK(core.shapes[165].frames[3].hotspotX==107 && core.shapes[165].frames[3].muzzleX==-89);std::cout<<"실제 의식 헤더30입력/Anchor·Point 전수 일치: 표시107,96과 목표-89,-46 구별\n";
}

// 저장 다이스를 첫 로드에서 실제 파생 Pop으로 등록한다. 원본 파일 대신 메모리상의 거주 구역 레코드 하나만 바꾼다.
TEST_CASE(CampaignDais_SavedRecordFirstPopRegistersAltar) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());bool replaced{}; // 검사 입력으로 바꾼 저장 레코드의 존재 여부다.
    // 원래 선택 청크/좌표/순서를 보존하여 일반 저장 로더로만 다이스를 생성한다.
    for (auto& section:fort.territories) {
        // 청크 순서와 다른 저장 객체를 그대로 유지한다.
        for (auto& chunk:section.chunks) {
            // 실제 Terr00의 로컬 거주 구역89 한 개를 저장 다이스로 치환하며 나머지는 원본 로드 입력이다.
            for (auto& object:chunk.objects) if (!replaced && object.type==89 && object.owner.value_or(1)==1) {
                object.type=165;object.storedType=165;object.frame=std::uint8_t{0};object.quantityByte=std::uint8_t{0};object.quantityWord=std::int16_t{0};
                object.owner=std::uint8_t{1};object.contents=FortContentList{};replaced=true;
            }
        }
    }
    CHECK(replaced);if (!replaced) return;
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();auto core=session.Core();
    const auto daises=ConstructionLive(core,165),altars=ConstructionLive(core,164);CHECK(daises.size()==1 && altars.size()==1);
    if (daises.size()!=1 || altars.size()!=1) return;const auto dais=daises.front(),altar=altars.front(); // 실제 로더/파생 Pop이 만든 두 객체다.
    CHECK(core.postPop.HandlesDais(dais) && core.postPop.HandlesDais(altar));
    CHECK(Get(session.PlayerRecords(),0xb4+0x8c)==dais.value && Get(session.PlayerRecords(),0xb4+0x90)==dais.value);
    CHECK(core.pool.Slot(altar)[34]==1 && core.processes.FindEvent(altar,21));
    CHECK(Get(core.pool.Slot(dais),14)==Get(core.pool.Slot(altar),14) && Get(core.pool.Slot(dais),18)==Get(core.pool.Slot(altar),18));
    CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
}

// 같은 소유자의 포획 사제는 원본 제단21에서 자동 해방된다. 가상 owner0이 원래 WORD의 소유자로 복귀해야 한다.
TEST_CASE(CampaignAltar_AlliedReleaseUsesVirtualPriestOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();auto core=session.Core();
    const auto dais=session.World().Create(165);core.owner.Set(dais,1);session.World().Pop(dais,100,111,0);
    const auto altars=ConstructionLive(core,164);CHECK(altars.size()==1);if (altars.size()!=1) return;const auto altar=altars.front(); // 실제 다이스 Pop이 만든 제단이다.
    const auto before=ConstructionLive(core,158);const auto captured=session.CreateContained(dais,158,0x81,32,0,0); // 원래 소유자1인 명시 포획 입력이며 기존 사제는 바꾸지 않는다.
    CHECK(Get(core.pool.Slot(altar),12,2)==1);auto* regular=core.processes.FindEvent(altar,21);CHECK(regular);if (!regular) return;
    const auto originalForm=regular->Form();const auto originalPayload=regular->Payload();const auto originalTime=regular->Time();core.processState.now=originalTime;regular->RunFrame(); // 같은 방향 동맹1→1 해방의 반환-1은 원래21의 시각/count를 그대로 보존한다.
    Sid released{};
    // 기존 사제 목록의 결과를 덮어쓰지 않고 이번 해방이 생성한 SID만 찾는다.
    for (const auto sid:ConstructionLive(core,158)) if (std::find(before.begin(),before.end(),sid)==before.end()) { CHECK(!released.value);released=sid; }
    CHECK(released.value);if (!released.value) return;const auto raw=core.pool.Slot(released); // 사제 가상 Owner/PostPop이 실제로 기록한 현재 raw 상태다.
    CHECK(raw[34]==1 && (Get(raw,12,2)&0x7fU)==1 && (raw[12]&0x80U));
    CHECK((core.pool.Slot(captured)[11]&3U)==3U && !session.Priests().Carrier().FindContained(dais,0,158,0).value);
    auto* preserved=core.processes.FindEvent(altar,21);CHECK(preserved==regular && core.processes.FromForm(originalForm)==preserved && preserved->Count()==0 && preserved->Payload()==originalPayload && preserved->Time()==originalTime); // 원본 Regular의 음수 반환을 삭제나 새 예약으로 해석하지 않는다.
    CHECK(Get(core.pool.Slot(altar),36)==static_cast<std::uint32_t>(core.frames[164].Run('P').first));
    if (preserved) { preserved->RunFrame();CHECK(core.processes.FindEvent(altar,21)==preserved && preserved->Form()==originalForm && preserved->Count()==1 && preserved->Time()==originalTime+static_cast<double>(originalPayload)); } // 다음 P방향 호출은 양수 payload를 반환해 같은21을3초 뒤로 재예약한다.
    CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
}

// 실제 다이스 별도 그림과 파괴 캡처를 동일 Session/Renderer/프로세스에 연결한다. 원본 전체 픽셀 관측 검사는 아니다.
TEST_CASE(CampaignDais_AdditionalDisplayAndShrapnelCapture) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();auto core=session.Core();
    const auto dais=session.World().Create(165);core.owner.Set(dais,1);session.World().Pop(dais,100,111,0);core.frame.Set(dais,47,0);core.pool.AllocatedBytes(dais)[40]|=2U; // 실제 의식의 추가 그림 상태만 명시 표시 입력으로 준다.
    RawCampaignFactoryDisplay display(core,assets.game);CHECK(display.Handles(dais));std::array<ScreenColor,256> palette{}; // 실제 파일 팔레트를 표시기의 색표에 설치한다.
    // 원본 팔레트 인덱스와 BGRX 성분을 같은 순서로 유지한다.
    for (std::size_t index=0;index<palette.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));palette[index]={color.blue,color.green,color.red,0}; }
    display.SetPalette(palette);const auto depth=std::bit_cast<std::int8_t>(core.pool.Slot(dais)[35]);const ScreenRect clip{0,0,1024,768};
    auto sprites=display.Sprites(clip,1400,1000,1);std::erase_if(sprites,[&](const auto& sprite) { return sprite.order.depth!=depth || sprite.order.x!=100 || sprite.order.y!=111; });CHECK(!sprites.empty());
    Renderer renderer(1024,768);renderer.SetScene(sprites);const auto shown=renderer.SceneImage();CHECK(std::any_of(shown.indices.begin(),shown.indices.end(),[](auto color) { return color!=0; }));
    // 파편 검사는 원본 Damageable preDestroy의 붕괴 비트200000h를 명시 요청해야 한다.
    CHECK(session.World().Destroy(dais,0x200000U));CHECK((core.pool.Slot(dais)[11]&3U)==3U);bool captured{};
    // 파괴가 만든 실제 파편 프로세스에서 다이스의 등록 깊이와 비어 있지 않은 이미지가 보존돼야 한다.
    for (const auto token:session.Particles().Rendering().Slots()) {
        const auto* fields=session.Particles().Rendering().Lookup(token);if (!fields || fields->zOrder!=depth) continue;
        const auto* process=dynamic_cast<ShrapnelVisualProcess*>(core.processes.FromForm(Sid{static_cast<std::uint16_t>(fields->form)}));
        if (process && std::any_of(process->Fields().image.pixels.begin(),process->Fields().image.pixels.end(),[](auto color) { return color!=255; })) captured=true;
    }
    CHECK(captured);CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
}

// 실제 Dude 최초Pop→부모 도착→공통/고유 삭제가 같은 월드를 정리해야 한다. 자연 생성/대피 검사는 아니다.
TEST_CASE(CampaignDude_ActualArrivalUsesPopulationLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();auto core=session.Core();
    const auto dude=session.World().Create(70);core.owner.Set(dude,1);session.World().Pop(dude,100,111,0);CHECK(session.DudeBehavior().Handles(dude));CHECK(Get(core.pool.Slot(dude),12,2)==0x5101);
    CHECK(session.DudeBehavior().Arrival(dude)==1001 && (core.pool.Slot(dude)[11]&3U)==3U);
    CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
}

// 실제30칸 등록과 Kernel 분배의poll/FALL→프레임·공간·102회 삭제를 확인한다. task 입력은 명시한다.
TEST_CASE(CampaignDude_ActualRegularFallAndPopulation30) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();auto core=session.Core();const auto& population=session.DudeBehavior().Population();const auto before=population.count;
    CHECK(population.entries.size()==30 && before<30);const auto dude=session.World().Create(70);core.owner.Set(dude,1);session.World().Pop(dude,100,111,0);CHECK(population.count==before+1 && Get(core.pool.Slot(dude),12,2)==0x5101);
    auto* poll=core.processes.AddRegular(dude,0x1a2b,5.0F,0x50);CHECK(poll);if (!poll) return;const auto pollForm=poll->Form();poll->RunFrame();CHECK(!core.processes.FromForm(pollForm) && !(core.pool.Slot(dude)[11]&3U) && population.count==before+1);
    DudeTaskHooks hooks;hooks.notifySurface=[&](Sid sid) { core.building.NotifySurface(sid); };RawDudeTask task(core.pool,{core.processState,core.bridgeState.bridgeType},MakeDudeTaskHostHooks(core.pool,core.processes,std::move(hooks)));task.Set(dude,0x5105,0);
    auto* fall=core.processes.FindEvent(dude,0x5105);CHECK(fall);if (!fall) return;const auto form=fall->Form();std::size_t calls{};
    // 예약 시각만 전진하고 실제 Regular/부모/프레임/공통 삭제를 실행한다. 전체 미션 Kernel 검사는 아니다.
    while (calls<103) { auto* current=dynamic_cast<RegularProcess*>(core.processes.FromForm(form));if (!current) break;core.processState.now=current->Time();current->RunFrame();++calls; }
    CHECK(calls==102 && !core.processes.FromForm(form) && (core.pool.Slot(dude)[11]&3U)==3U && population.count==before);CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
}

// 명시 Dude에서 실제 BFS/현재24 조회/임시2배 속도/부모 도착 수명을 확인한다. 전체 Kernel 검사는 아니다.
TEST_CASE(CampaignDude_ActualPathNoticeSpeedAndArrival) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();auto core=session.Core();RawPathPoint source{};bool found{};
    // 원래 지도에서 점유되지 않은 연속 표면5칸만 고르며 spot/hash를 검사에서 만들지 않는다.
    for (int y=32;y<220 && !found;++y) {
        // 같은 줄의 실제 표면과 점유를 원본 지도에서 읽는다.
        for (int x=32;x<215 && !found;++x) {
            bool clear=true;
            // 시작·목적지 사이 모든 셀이 같은 surface 위의 통행 가능한 칸이어야 한다.
            for (int dx=0;dx<5;++dx) if (!(core.spots[y*256+x+dx]&6U) || (core.spots[y*256+x+dx]&8U) || !core.hash.Cell(0,x+dx,y)) clear=false;
            if (clear) { source={static_cast<float>(x),static_cast<float>(y)};found=true; }
        }
    }
    CHECK(found);if (!found) return;const auto dude=session.World().Create(70);core.owner.Set(dude,1);session.World().Pop(dude,source.x,source.y,0);const std::array<float,2> target{source.x+3,source.y};const auto packets=external.packets.size();
    CHECK(session.Dudes().Move(dude,target,false));const auto form=FindAttachedProcess(core.pool,dude,24);auto* slow=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(slow && form.value<core.pool.Layout().serverFirst);if (!slow) return;
    CHECK(slow->MovementType()==24 && Get(slow->Runtime().Bytes(),0)==0x50e3d8U && Get(core.pool.Slot(dude),12,2)==0x5101U);const auto before=std::vector<std::uint8_t>(slow->Runtime().Bytes().begin(),slow->Runtime().Bytes().end());
    CHECK(session.Dudes().Move(dude,target,true) && core.processes.FromForm(form)==slow && std::equal(before.begin(),before.end(),slow->Runtime().Bytes().begin())); // 같은 끝점은 기존 경로/시간표를 그대로 유지한다.
    std::vector<double> times;std::vector<RawPathPoint> points;std::vector<std::int32_t> directions;
    // 실제 BFS가 고른 시간표만 보존하여 새로 만든 기대 경로로 대체하지 않는다.
    for (std::int32_t index=0;index<slow->Runtime().Count();++index) { times.push_back(slow->Runtime().Time(index));points.push_back(slow->Runtime().Point(index));directions.push_back(slow->Runtime().Direction(index)); }
    core.processes.Kill(*slow,0);CHECK(session.Dudes().Move(dude,target,true));const auto fastForm=FindAttachedProcess(core.pool,dude,24);auto* fast=dynamic_cast<RawPathProcess*>(core.processes.FromForm(fastForm));CHECK(fast);if (!fast) return;CHECK(fast->Runtime().Count()==static_cast<std::int32_t>(times.size()));std::size_t movingSegments{};
    constexpr double kFirstUnitSeconds=0.6666666865348816,kNextQuickUnitSeconds=0.5; // 원본 float 역수: 최초속도1.5의3f2aaaab과 정수1 복원 뒤 빠른속도2의3f000000이다.
    CHECK(assets.game.Types()[0].definition.Number("speed").value()==1.5); // 최초 실제 Dude TYPE 입력을 독립 자산값과 함께 확인한다.
    // 첫 요청 뒤 원본 정수 복원으로1.5→1이 되므로 다음 빠른 요청은2이며 최초 요청의 정확한 절반이 아니다.
    for (std::int32_t index=1;index<fast->Runtime().Count() && index<static_cast<std::int32_t>(times.size());++index) {
        const auto point=fast->Runtime().Point(index);CHECK(point.x==points[index].x && point.y==points[index].y && fast->Runtime().Direction(index)==directions[index]);
        if ((point.x!=points[index-1].x || point.y!=points[index-1].y) && directions[index]==directions[index-1]) {
            const double dx=static_cast<double>(point.x)-points[index-1].x,dy=static_cast<double>(point.y)-points[index-1].y;const auto distance=std::sqrt(dx*dx+dy*dy); // 실제 BFS 구간 길이만 읽고 경로나 결과 시각을 수정하지 않는다.
            CHECK_NEAR(times[index]-times[index-1],distance*kFirstUnitSeconds,1e-10);CHECK_NEAR(fast->Runtime().Time(index)-fast->Runtime().Time(index-1),distance*kNextQuickUnitSeconds,1e-10);++movingSegments;
        }
    }
    CHECK(movingSegments && external.packets.size()==packets);CHECK(!core.processes.FromForm(form) || fastForm==form);CHECK(Throws([&] { static_cast<void>(session.StartDudeMovement({Sid{static_cast<std::uint16_t>(session.FindPriestParticipation(1))},source})); }));
    Put(core.pool.AllocatedBytes(dude),12,0x5107,2);CHECK(core.processes.AddRegular(dude,0x5107,5.0F,0x50)); // 최초Pop은Regular를 만들지 않는다. 최종 삭제 task/Regular만 명시 입력이며 본문은 후속 단위다.
    const auto began=core.processState.now;std::size_t steps{};
    // 전체 Kernel 대신 현재 Path만 진행하며 Arrive가 부모와 자신을 지운 뒤 포인터를 다시 읽지 않는다.
    while (core.processes.FromForm(fastForm) && steps<2000) { core.processState.now=began+static_cast<double>(++steps)*0.025;auto* current=core.processes.FromForm(fastForm);current->RunFrame(); }
    CHECK(steps>0 && steps<2000 && !core.processes.FromForm(fastForm) && (core.pool.Slot(dude)[11]&3U)==3U);CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
}

// 실제 Factory/전체Pop의 제단 생성·Player 등록과 부착 통지/제단 저항을 확인한다. 자연 포획 입력은 아니다.
TEST_CASE(CampaignDais_ActualFactoryPopAndCapturedNotice) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();InputQueue commands;netstorm::zacket::LocalMissionTransport* wire{};std::vector<std::string> music;services.carrierRequest=[&](std::int32_t owner,std::span<const std::uint8_t> packet) { CHECK(wire);if (wire) wire->Send(owner,packet); };services.requestMusic=[&](std::string_view name) { music.emplace_back(name); };services.assets.queueCommand=[&](std::uint32_t command,std::int32_t x,std::int32_t y) { commands.PushCommand(command,x,y); }; // 실제 입력 큐와 자산을 쓴다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;const auto dais=session.World().Create(165);core.owner.Set(dais,1);session.World().Pop(dais,100,111,0); // 합법 배치/UI 건설 검사가 아닌 명시 생성 위치다.
    const auto altars=ConstructionLive(core,164);CHECK(altars.size()==1);if (altars.size()!=1) return;const auto altar=altars[0];CHECK(Get(core.pool.Slot(dais),36)==0 && Get(core.pool.Slot(altar),36)==0);CHECK(core.pool.Slot(altar)[34]==1 && core.processes.FindEvent(altar,21));CHECK(Get(session.PlayerRecords(),0xb4+0x8c)==dais.value && Get(session.PlayerRecords(),0xb4+0x90)==dais.value); // 전체Pop이 실제Player와 같은 풀을 바꿨는지 확인한다.
    while (!commands.Empty()) static_cast<void>(commands.Pop(false)); // 기존 로드 명령과 이번 부착 명령을 구분한다.
    const auto captured=session.CreateContained(dais,158,0x82,32,0,0);CHECK(Get(core.pool.Slot(altar),12,2)==2);CHECK(Get(core.pool.Slot(altar),36)==static_cast<std::uint32_t>(core.frames[164].Run('O').first));CHECK((core.pool.Slot(captured)[11]&3U)==0); // 원래owner의 하위7비트와 O프레임은 실제 부모 통지가 쓴다.
    const auto notice=commands.Pop(false);CHECK(notice.code==0x020000d7 && notice.x==0 && notice.y==0); // PE00542a50의 실제 명령을 음향 정지로 바꾸지 않는다.
    const auto began=core.processState.now;for (std::uint64_t step=1;step<=160;++step) session.RunFrame({began+double(step)*.025,100+double(step)*.025,.025,1000+step},false); // 같은 Kernel에서 제단21/20 또는22를 진행한다.
    CHECK(core.processes.FindEvent(altar,21));
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};const RawPathPoint point{100,111};auto input=session.MakeCarrierInput({});auto target=point;CHECK(input->Evaluate(priest,target)==0x11);session.RequestCarrierMove(priest,target,1);link.transport.Flush();CHECK(link.transport.Poll()==1 && link.recorded.size()==1 && music==std::vector<std::string>{"sacrifice.mus"}); // 실제 공간의 Dais를 사용자 이동 명령으로 선택한다.
    const auto ritualBegan=core.processState.now;std::size_t born{},retired{};bool live{}; // 결과를 쓰지 않고 실제 룬 수명의 전이를 센다.
    for (std::uint64_t step=1;step<=8000 && retired<5;++step) { // 최대200초 동안 경로 도착과 다섯 룬을 같은 Kernel에서 진행한다.
        const auto now=ritualBegan+double(step)*.025;session.RunFrame({now,100+now,.025,2000+step},false);const auto runes=ConstructionLive(core,166);const bool current=!runes.empty();if (current && !live) ++born;if (!current && live) ++retired;live=current;
    }
    CHECK(born==5 && retired==5 && Get(core.pool.Slot(dais),36)==static_cast<std::uint32_t>(core.frames[165].Run('N').first) && !core.processes.FindEvent(priest,604));CHECK((core.pool.Slot(captured)[11]&3U)==0); // 보상 선택/희생 완료를 주입하지 않는다.
    std::cout<<"전체Pop 실제 Dais 해시"<<std::uint32_t(core.pool.Slot(dais)[33])<<"/UI17→40h→경로→룬5 자연 진행\n";
    std::cout<<"실제 Dais"<<dais.value<<"/제단"<<altar.value<<" 생성·Player 등록·내용물O/owner2 통지·4초 저항 진행\n";
}

namespace {
// 원래1-1의 실제 건설/운반 입력을 공유한다. finish가 참이면 같은 현재 미션에서 클릭/희생 완료까지 진행한다.
void ActualBuiltDaisDelivery(bool finish) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;RawCampaignSession* currentSession{};Sid observedGolem{};std::array<float,2> noticePoint{};std::size_t deliveryNotices{};std::vector<std::string> ritualTells,ritualMusic;std::size_t spRefreshes{};auto services=external.Services(); // 같은 실제 장치/Tell 요청만 관찰한다.
    services.refreshSp=[&] { ++spRefreshes; };services.priest.shield.tell=[&](std::string_view key) { ritualTells.emplace_back(key); };services.requestMusic=[&](std::string_view name) { ritualMusic.emplace_back(name); };const auto queue=services.assets.queueCommand;services.assets.queueCommand=[&](std::uint32_t command,std::int32_t x,std::int32_t y) { queue(command,x,y);if (command==0x020000d7 && currentSession && observedGolem.value) { const auto raw=currentSession->Core().pool.Slot(observedGolem);noticePoint={std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))};++deliveryNotices; } };UseWorldTestLogin(services);services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };services.ai.defaultName=[] { return std::string("AI"); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();currentSession=&session;auto core=session.Core();SessionCursorUi ui;auto& cursor=session.Cursor();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1; // 장치/입력 관찰만 대체하며 계산은 같은 세션이다.
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");MissionScript script(config,"TheWarBegins");CHECK(script.Loaded());MissionFrameGateState gate;MissionStartState startState;CampaignMissionStartEffects startEffects; // 원래 영어 스크립트의 실제 Start로 자금/기술/AI를 적용한다.
    startEffects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };startEffects.gameSeconds=[&] { return core.processState.now; };startEffects.refreshSp=[] {};startEffects.refreshCommandGump=[] {};startEffects.resetCamera=[] {};startEffects.randomGeysers=[] { throw std::logic_error("원래1-1에 없는 세계 랜덤 간헐천 요청"); };
    RawCampaignMissionStartBindings start(session,script,gate,startState,session.MissionStartSettings(),session.MissionAiSettings(),std::move(startEffects));start.Start();CHECK(gate.started==1); // 별도 SetSp/기술 주입을 하지 않는다.
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};SessionPriestMenuUi menuUi;auto hooks=menuUi.Hooks(core.types);hooks.takePreview=[&](Sid form,std::uint32_t flags) { menuUi.preview=form;menuUi.flags=flags;++menuUi.takes;cursor.Take(form,flags); };auto menu=session.MakePriestMenu(std::move(hooks));const auto before=session.Sp(1);std::cout<<"실제 Dais 건설 시작 SP"<<before<<"/비용"<<core.types[165].cost<<"\n";CHECK(menu->SelectBuild(165,priest));if (!menuUi.takes) return;
    const auto point=CursorPlacementPoint(session,ui);CHECK(session.CursorDropSettings().legal);std::cout<<"실제 다이스 커서 허용 위치"<<point.x<<","<<point.y<<"\n";cursor.Drop();CHECK(ui.sent.size()==1);if (ui.sent.size()!=1) return;CHECK(ui.sent[0][1]==0x3e && ui.sent[0][4]==165); // 실제25바이트 요청이며 서버가 전체 최초Pop을 수행한다.
    std::vector<std::vector<std::uint8_t>> received;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(received));session.RegisterConstructionCommands(dispatch);PacketConnection connection;connection.sender=1;CHECK(dispatch.Dispatch(connection,ui.sent[0]));const auto packet=session.Construction().LastPacket();CHECK(packet.size()==29);if (packet.size()!=29) return;const Sid target{static_cast<std::uint16_t>(packet[27]|(std::uint16_t(packet[28])<<8))};CHECK(core.pool.Slot(target)[10]==165);CHECK(session.Sp(1)==before-core.types[165].cost);CHECK(ConstructionLive(core,134).empty());const auto priestPath=FindAttachedProcess(core.pool,priest,54);CHECK(priestPath.value);std::cout<<"다이스 건설은 실제 사제54 경로/대상Extra"<<std::uint32_t(core.pool.Slot(target)[40])<<"\n";
    const auto began=core.processState.now;for (std::uint64_t step=1;step<=8000 && (core.pool.Slot(target)[40]&1);++step) session.RunFrame({began+double(step)*.025,100+double(step)*.025,.025,2000+step},false); // 결과 시각을 덮어쓰지 않고 같은 Kernel에서 건설을 끝낸다.
    CHECK((core.pool.Slot(target)[11]&7U)==0 && !(core.pool.Slot(target)[40]&1));CHECK(!session.HitPoints().HasHitPoints(target));CHECK((core.pool.Slot(priest)[11]&3U)==0);const auto altars=ConstructionLive(core,164);CHECK(altars.size()==1);if (altars.size()!=1) return;std::cout<<"실제 사제 메뉴/허용 커서/패킷/사제54→다이스"<<target.value<<" 완공·제단"<<altars.size()<<"개\n";
    const auto golem=session.World().Create(133,2);core.owner.Set(golem,1);session.World().Pop(golem,static_cast<float>(point.x+1),static_cast<float>(point.y),0);const auto held=session.CreateContained(golem,158,0x82,32,0,0); // 포획 이전을 대신 증명하지 않는 명시 운반 시작 입력이다. 다이스는 앞의 사용자 건설 결과를 그대로 쓴다.
    observedGolem=golem; // 실제 제단 통지 시점의 운반체 좌표를 관찰한다. 도착 뒤 회피는 같은 프레임에 위치를 바꿀 수 있다.
    auto carrierInput=session.MakeCarrierInput({});RawPathPoint destination{static_cast<float>(point.x),static_cast<float>(point.y)};CHECK(carrierInput->Evaluate(golem,destination)==3);CHECK(destination.x==point.x-3 && destination.y==point.y-3);session.RequestCarrierMove(golem,destination,0);session.PumpLocalConnection(); // 실제 커서가 제단 중심으로 보정한 좌표를 로컬40h에 넘긴다.
    const auto deliveryBegan=core.processState.now;Sid delivered{};for (std::uint64_t step=1;step<=8000 && !delivered.value;++step) { // 같은 이동/도착이 새 제단 내용물을 만들 때까지 진행한다.
        const auto now=deliveryBegan+double(step)*.025;session.PumpLocalConnection();session.RunFrame({now,100+now,.025,12000+step},false);delivered=session.Priests().Carrier().FindContained(target,32,158,0);
    }
    std::cout<<"제단 전달 통지 시점"<<noticePoint[0]<<","<<noticePoint[1]<<"/현재 자기동맹"<<session.Alliances()[10]<<"/전달"<<delivered.value<<"\n";CHECK(deliveryNotices==1 && noticePoint[0]==point.x-3 && noticePoint[1]==point.y-3 && session.Alliances()[10]==1);
    CHECK(delivered.value && delivered!=held);CHECK((core.pool.Slot(held)[11]&3U)!=0 && !session.Priests().Carrier().FindContained(golem,32,158,0).value);if (!delivered.value) return;CHECK(Get(core.pool.Slot(delivered),12,2)==0x82);CHECK(Get(core.pool.Slot(altars[0]),12,2)==2 && RawPathAnimation::Direction(core.frames[164],static_cast<std::int32_t>(Get(core.pool.Slot(altars[0]),36)))==14); // 전달 결과/제단O는 실제 도착과 부모 통지가 쓴다.
    std::cout<<"명시 포획 운반 입력→실제 커서3/40h/이동/제단 전달"<<delivered.value<<"/제단O·owner2 갱신\n";
    if (!finish) return;
    const auto beforeFinish=session.Sp(1);const auto refreshBefore=spRefreshes;CarrierInputHooks ritualHooks;std::size_t menus{}; // 현재 실제 자금과 보상창 호출을 관찰하며 완료값을 쓰지 않는다.
    ritualHooks.showSacrifice=[&](std::uint32_t) { ++menus;throw std::logic_error("캠페인에 없는 멀티플레이 보상창"); };ritualHooks.assertion=[](std::uint32_t line) { throw std::logic_error("실제 희생 클릭 진단 "+std::to_string(line)); };
    auto ritualInput=session.MakeCarrierInput(std::move(ritualHooks));RawPathPoint altarPoint{static_cast<float>(point.x),static_cast<float>(point.y)};CHECK(ritualInput->Evaluate(priest,altarPoint)==0x11);ritualInput->Commit(priest.value,altarPoint);session.PumpLocalConnection();CHECK(menus==0 && ritualMusic==std::vector<std::string>{"sacrifice.mus"});
    const auto finishBegan=core.processState.now;std::size_t born{},retired{};bool wasLive{}; // 실제 룬/다이스 수명의 전이를 읽는다.
    for (std::uint64_t step=1;step<=12000 && !(core.pool.Slot(target)[11]&3U);++step) { // 같은Kernel/로컬전송으로 최대300초 진행한다.
        const auto now=finishBegan+double(step)*.025;session.PumpLocalConnection();session.RunFrame({now,100+now,.025,20000+step},false);const bool live=!ConstructionLive(core,166).empty();if (live && !wasLive) ++born;if (!live && wasLive) ++retired;wasLive=live;
    }
    CHECK(born==5 && retired==5 && menus==0);CHECK((core.pool.Slot(target)[11]&3U)!=0 && (core.pool.Slot(altars[0])[11]&3U)!=0 && (core.pool.Slot(delivered)[11]&3U)!=0);CHECK(session.Sp(1)==beforeFinish+5000 && spRefreshes>=refreshBefore+1);CHECK(ritualTells==std::vector<std::string>{"SacrificeForStormpower"}); //전체AI의다른SP표시갱신도같은장치를사용하므로총호출을한번으로제한하지않는다.
    CHECK(session.Priests().Priests().ownerState[1]==0); // 원래 적 사제가 살아 있는 명시 포획 입력이므로 승리를 주장하지 않는다.
    for (std::uint32_t owner=1;owner<=8;++owner) { // Finish의8명갱신이같은현재조회/저장소를소비했는지확인한다.
        const auto found=session.FindPriestParticipation(owner);const auto expected=!found ? 2U : core.pool.Slot(Sid{static_cast<std::uint16_t>(found)})[10]==158 ? 0U : 1U;CHECK(session.Priests().Priests().ownerState[owner]==expected);
    }
    CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);std::cout<<"실제 캠페인 희생 완료: 룬5/메뉴0/다이스·제단·포획 삭제/SP"<<beforeFinish<<"→"<<session.Sp(1)<<"/전체 표시"<<spRefreshes-refreshBefore<<"/Tell1/참여8명 갱신\n";
}
}
// 기존 실제 메뉴/건설/명시 운반 회귀를 동일 입력과 종료점에서 실행한다.
TEST_CASE(CampaignDais_ActualPriestMenuCursorConstruction) { ActualBuiltDaisDelivery(false); }
// 같은 사용자 건설 결과와 명시 운반 입력 뒤 실제 희생 클릭/완료·SP를 확인한다. 자연 포획/미션 승리를 주입하지 않는다.
TEST_CASE(CampaignDais_ActualCampaignSacrificeFinish) { ActualBuiltDaisDelivery(true); }

// 캠페인의 실제Commit은 멀티플레이 보상창을 호출하지 않고 같은40h/의식 경로를 계속 사용한다. 자연 포획을 증명하는 검사는 아니다.
TEST_CASE(CampaignSacrifice_ClickSkipsMultiplayerMenuAndKeepsRitualRequest) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();netstorm::zacket::LocalMissionTransport* wire{};std::vector<std::string> music;std::size_t menus{}; // 실제 자산/전송을 사용하고 보상창 호출만 센다.
    services.carrierRequest=[&](std::int32_t owner,std::span<const std::uint8_t> packet) { CHECK(wire);if (wire) wire->Send(owner,packet); };services.requestMusic=[&](std::string_view name) { music.emplace_back(name); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;
    const auto dais=session.World().Create(165);core.owner.Set(dais,1);session.World().Pop(dais,100,111,0);const auto captured=session.CreateContained(dais,158,0x82,32,0,0); // 메뉴 관문을 확인하는 명시 제단/포획 입력이며 건설·포획 완료값을 대신하지 않는다.
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;CarrierInputHooks hooks;hooks.showSacrifice=[&](std::uint32_t) { ++menus;throw std::logic_error("캠페인에서 호출되면 안 되는 멀티플레이 보상 메뉴"); };hooks.assertion=[](std::uint32_t line) { throw std::logic_error("실제 의식 클릭 진단 "+std::to_string(line)); };
    auto input=session.MakeCarrierInput(std::move(hooks));RawPathPoint target{100,111};CHECK(input->Evaluate(priest,target)==0x11);const auto sp=session.Sp(1);input->Commit(priest.value,{100,111});CHECK(menus==0); // 실제 사용자Commit을 직접 실행하며 UI 오류를 숨기는 예외 처리를 하지 않는다.
    link.transport.Flush();CHECK(link.transport.Poll()==1 && link.recorded.size()==1);CHECK(music==std::vector<std::string>{"sacrifice.mus"} && session.Sp(1)==sp);CHECK(FindAttachedProcess(core.pool,priest,54).value); // 보상 선택 생략과 의식 이동/음악 요청을 구별한다.
    const auto began=core.processState.now;std::size_t born{},retired{};bool wasLive{}; // 같은Kernel의 실제 룬 생성/삭제 전이를 센다.
    for (std::uint64_t step=1;step<=8000 && retired<5;++step) { // 기존 요청 뒤 실제 다섯 룬 진행이 유지되는지 확인한다.
        const auto now=began+double(step)*.025;session.RunFrame({now,100+now,.025,step},false);const bool live=!ConstructionLive(core,166).empty();if (live && !wasLive) ++born;if (!live && wasLive) ++retired;wasLive=live;
    }
    CHECK(born==5 && retired==5 && menus==0 && Get(core.pool.Slot(dais),36)==static_cast<std::uint32_t>(core.frames[165].Run('N').first));CHECK((core.pool.Slot(captured)[11]&3U)==0 && !core.processes.FindEvent(priest,604)); // 캠페인 보상·희생 완료는 별도 원본 경로이며 여기서 주입하지 않는다.
    std::cout<<"실제 캠페인 클릭: 멀티플레이 보상창0/40h1/의식 음악1/룬5 생성·소진, 희생 완료는 별도\n";
}


// 실제 Early1 로그인/영어 Start와 같은 판정기가10호출 안내·7호출 재개·F4 단계 예약을 공유하는지 검사한다.
TEST_CASE(CampaignTutorialOne_ActualSessionFrameAndInputPauseOwnership) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
    MissionScript script(config,"Tutorial1");CHECK(script.Loaded());bool dialog{};std::uint32_t depth{};double now=10;std::vector<std::string> pages;std::vector<bool> closePaused;RawCampaignTutorialOneBindings* active{}; // 실제 UI/시계와 팝업 정리 시점의 정지 소유권을 관측한다.
    services.missionOutcome.demo=[] { return 0U; };services.missionOutcome.tellBlocked=[&] { return dialog ? 1U : 0U; };
    services.missionOutcome.section=[&](std::string_view name) { CHECK(!script.Section(name));return false; }; // 이 보호 Early1에는 승패 사건 섹션이 없다.
    services.missionOutcome.closeTransient=[] { throw std::logic_error("보호 Early1에 없는 승패 창 정리"); };
    services.missionOutcome.onSection=[] { throw std::logic_error("보호 Early1에 없는 승패 창 정지"); };
    CampaignLoginServices login;login.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };login.wallSeconds=[] { return 100.25; };login.clock=[] { return 100250U; };login.refresh=[] {};login.playerUiBusy=[] { return false; };login.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};login.log=[](std::string_view) {};
    services.ai.participation.addChat=[&external](std::uint32_t,std::string_view,std::uint32_t) { ++external.notices; };services.ai.participation.clearChat=[&external](std::uint32_t) { ++external.notices; }; // 로그인 패킷이 요청하는 실제 채팅 UI 경계를 명시한다.
    services.localLogin=std::move(login);auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.LoadLocalConnection();
    CHECK(session.LocalConnectionPhase()==14 && session.LocalOwner()==1 && session.PlayerKnown()==1 && session.Sp(1)==0 && !session.MissionPriestDead(1));
    MissionFrameGateState gate;gate.tutorialNumber=1;MissionStartState startState;CampaignMissionStartEffects startEffects; // 단계/자금/완료 결과를 검사에서 지정하지 않는다.
    startEffects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t>) { throw std::logic_error("AI off Early1의 AI 생성 도달"); };startEffects.gameSeconds=[&] { return now; };startEffects.refreshSp=[] {};startEffects.refreshCommandGump=[] {};startEffects.resetCamera=[] {};startEffects.randomGeysers=[] { throw std::logic_error("Early1 Start의 세계 랜덤 간헐천"); };
    RawCampaignMissionStartBindings start(session,script,gate,startState,session.MissionStartSettings(),session.MissionAiSettings(),std::move(startEffects));
    CampaignTutorialOneEffects effects;effects.frame.gameSeconds=[&] { return now; };effects.frame.paused=[&] { return depth!=0; };effects.frame.dialogsClear=[&] { return !dialog; };effects.frame.knowledgeOpen=[] { return false; };effects.frame.guideOpen=[] { return false; };effects.frame.blockingGumpOpen=[] { return false; };
    effects.frame.pause=[&] { ++depth; };effects.frame.resume=[&] { CHECK(depth>0);--depth; };effects.frame.endDemo=[] { throw std::logic_error("Early1의 데모 종료 도달"); };
    effects.tell=[&](std::string_view name) { if (!script.Section(name)) return false;dialog=true;pages.emplace_back(name);return true; };
    effects.closeTransient=[&] { CHECK(dialog && active);closePaused.push_back(gate.scriptPaused!=0); }; // 최초/새 단계는 Tell→정리→정지이며 이미 열린 안내의 First 재표시는 기존 자기 정지를 보존한다.
    RawCampaignTutorialOneBindings runtime(session,script,start,gate,startState,std::move(effects));active=&runtime;
    runtime.Frame();CHECK(!gate.started && pages.empty() && gate.lockFrames==10);start.Start();CHECK(gate.started && session.Sp(1)==0 && session.TotalMade().BuiltForGenus(4)==0);
    // 입력이 없는 실제 화면 호출9회에는 초기 안내가 없고10회에만 A를 표시한다.
    for (int frame=0;frame<9;++frame) runtime.Frame();
    CHECK(pages.empty() && gate.lockFrames==1 && !depth);runtime.Frame();CHECK(pages==std::vector<std::string>{"A."} && dialog && depth==1 && gate.scriptPaused==1 && !gate.lockFrames && runtime.StageShown()==1);
    runtime.Frame();CHECK(depth==1 && gate.clearFrames==0 && gate.stage=='A');dialog=false; // 사용자가 대화상자를 닫는 실제 UI 경계다.
    // 연속 빈 호출6회에는 자기 정지가 남고7번째에 한 깊이만 반납한다.
    for (int frame=0;frame<6;++frame) runtime.Frame();
    CHECK(depth==1 && gate.clearFrames==6 && gate.stage=='A');runtime.Frame();CHECK(!depth && !gate.scriptPaused && !gate.clearFrames);
    CHECK(runtime.Event({0x02000008U,0,0})==1 && gate.stage=='A'); // A에서 다리 취득 명령은 소비하며 진행값을 만들지 않는다.
    CHECK(runtime.Event({0x020000aeU,0,0})==0 && gate.stage=='B' && gate.lockFrames==10 && gate.nextStageSeconds==10.5); // 실제 홈 명령이 원본 판정의 Advance를 호출한다.
    now=10.499;runtime.Frame();CHECK(gate.lockFrames==10 && pages.size()==1);now=10.5;
    // 다음 안내도 같은10호출 카운터를 사용한다.
    for (int frame=0;frame<9;++frame) runtime.Frame();
    CHECK(gate.lockFrames==1 && pages.size()==1);runtime.Frame();CHECK(gate.stage=='B' && pages==std::vector<std::string>({"A.","B."}) && depth==1 && dialog);
    CHECK(runtime.Event({0x020000d3U,0,0})==1 && gate.stage=='B' && depth==1); // 정지 중 수동 단계 명령도 같은 원본 관문에서 거부한다.
    InputQueue replay;const auto sequence=startState.sequence,flags=startState.flags; // 같은 실제 Start 상태를 F8 재표시가 진행/복제하지 않는지 본다.
    CHECK(QueueMissionFirstKey({0x770000U,0,0},replay,true) && pages.size()==2);CHECK(runtime.Event(replay.Pop(false))==1);
    CHECK(pages==std::vector<std::string>({"A.","B.","B."}) && depth==1 && gate.scriptPaused && gate.stage=='B' && !gate.lockFrames);
    CHECK(runtime.Event({InputCode::kRelease|kMissionFirstCommand,0,0})==1 && pages.size()==4 && depth==1);
    CHECK(startState.sequence==sequence && startState.flags==flags && runtime.StageShown()==1); // 명시 First는 Guide가 release 이전에 소비한다.
    CHECK(closePaused==std::vector<bool>({false,false,true,true})); // A/B 최초 표시는 정지 전이고 F8/명시 First 재표시는 이미 보유한 한 정지 안에서 정리한다.
    CHECK(session.Sp(1)==0 && session.TotalMade().BuiltForGenus(4)==0 && session.Core().bookkeeping.depth==0 && session.Core().destroy.PreDepth()==0 && session.Core().destroy.PostDepth()==0);
}


// 실제 영어 Start의10000SP와 같은 패널/예산/등록ID가 모달 페이지 교체와 화면 변경을 견디는지 확인한다.
TEST_CASE(CampaignProduction_ModalPagesPreservePanelBudgetAndResize) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();RawButtonControls* active{};double wall=100; // 실제 UI/벽시계의 외부 경계다.
    services.production.allocationBytes=[](std::span<std::uint8_t> bytes) { CHECK(bytes.size()==kProductionDeckBytes);std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); }; // 원본 생성자가 보존하는 할당 바이트의 명시 입력이다.
    services.production.effects.configInt=[](std::string_view key) { CHECK(key=="getTeleportTechFree");return 0; };services.production.effects.modeAssertion=[] { throw std::logic_error("실제 튜토리얼 생산 모드 진단"); };services.sound.wallSeconds=[&] { return wall; };
    services.playerContent.gump=[&](auto id) { return active ? active->RegisteredGump(id) : 0U; };services.playerContent.handle=[&](auto token,auto event,auto first,auto second) { CHECK(active);return active->HandleGump(token,event,first,second); };
    services.ai.participation.addChat=[&](std::uint32_t,std::string_view,std::uint32_t) { ++external.notices; };services.ai.participation.clearChat=[&](std::uint32_t) { ++external.notices; };
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");MissionScript script(config,"Tutorial2");CHECK(script.Loaded());
    CampaignLoginServices login;login.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };login.wallSeconds=[&] { return wall; };login.clock=[] { return 100000U; };login.refresh=[] {};login.playerUiBusy=[] { return false; };login.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};login.log=[](std::string_view) {};
    services.localLogin=std::move(login);auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.LoadLocalConnection();CHECK(session.LocalConnectionPhase()==14);
    MissionFrameGateState gate;MissionStartState startState;CampaignMissionStartEffects startEffects; // 자금은 검사 setter가 아닌 원래 영어 Start가 지정한다.
    startEffects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t>) { throw std::logic_error("AI off Tutorial2의 AI 생성 도달"); };startEffects.gameSeconds=[] { return 0.0; };startEffects.refreshSp=[] {};startEffects.refreshCommandGump=[] {};startEffects.resetCamera=[] {};startEffects.randomGeysers=[] { throw std::logic_error("보호 Tutorial2 Start의 세계 간헐천 생성"); };
    RawCampaignMissionStartBindings start(session,script,gate,startState,session.MissionStartSettings(),session.MissionAiSettings(),std::move(startEffects));start.Start();CHECK(session.Sp(1)==10000 && session.PlayerKnown()==1);
    FontStore fonts(assets.files,"Arial");RawButtonControls controls(OriginalEdition::Patch1078,{[&] { return wall; },{},{},[&](auto form,auto flags) { session.GumpForms().Destroy({static_cast<std::uint16_t>(form)},flags); }});active=&controls;
    RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t&) {};hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,500,350}; };hooks.cursor=[](auto) {};hooks.objectForms=[&]() -> GumpFormHost& { return session.GumpForms(); };hooks.gameSeconds=[] { return 0.0; };
    controls.EnableMenus(assets.game,fonts,std::move(hooks));const auto& texture=assets.game.Find("fortGump");const auto metrics=assets.game.Shapes().SquidMetrics(texture.block,0);const RawButtonAssets buttonAssets{static_cast<std::uint32_t>(texture.block+70),0,metrics.width,metrics.height,{}}; // 실제 버튼 텍스처와 공통 트리다.
    const std::array<GumpControl,1> game{{{0,{84,4,160,26},false,true,false}}},dialog{{{0,{470,400,550,423},false,true,false}}};const std::array<std::string,1> gameLabels{{"Game"}},dialogLabels{{"OK"}}; // 현재 페이지의 실제 버튼들이다.
    controls.SetControls(game,gameLabels,1024,768,buttonAssets);
    auto& panel=controls.EnableProduction(session,assets.game,{[&] { return wall; },[](std::string_view) {},[](std::string_view) { throw std::logic_error("모달 수명 검사에서 예기치 않은 Tell"); },[](std::string_view key,std::int32_t fallback) { return key=="useProductTimers" ? 0 : fallback; }});
    const auto summary=[&] { const auto text=controls.Production()->Report();return text.substr(0,text.find('\n')); }; // 같은 실제 root/id/크기/예산만 읽는다.
    // 게임 프레임을 실행하지 않고 실제 UI idle만 호출해 원본 예산 애니메이션을 수렴시킨다.
    for (int frame=0;frame<200;++frame) { wall+=.025;controls.Poll({InputCode::kRelease,500,350}); }
    const auto before=summary();CHECK(before.ends_with("\t10000"));const auto* identity=&panel; // 현재 패널 주소와 실제 예산을 보존한다.
    controls.SetControls(dialog,dialogLabels,1024,768,buttonAssets,true);CHECK(controls.Production()==identity && summary()==before);
    // 같은 모달 페이지를 반복 생성해도 패널이나 예산/등록ID를 재생성하지 않는다.
    for (int page=0;page<4;++page) { controls.SetControls(dialog,dialogLabels,1024,768,buttonAssets,true);wall+=.025;controls.Poll({InputCode::kRelease,500,350});CHECK(controls.Production()==identity && summary()==before); }
    controls.SetControls(game,gameLabels,1024,600,buttonAssets,true);panel.Resize(600);CHECK(controls.Production()==identity && summary().ends_with("\t80\t520\t10000"));const auto resized=summary();panel.Resize(600);CHECK(summary()==resized);
    controls.SetControls(game,gameLabels,1024,600,buttonAssets);CHECK(controls.Production()==nullptr);active=nullptr;CHECK(session.Core().bookkeeping.depth==0 && session.Core().destroy.PreDepth()==0 && session.Core().destroy.PostDepth()==0); // 기존 전체 반납 경로도 유지한다.
}

// 실제 Early2 로그인/영어 Start와 같은 판정기가 현재 옵션/공급 범위/제작 장부를 공유한다.
TEST_CASE(CampaignTutorialTwo_ActualSessionOptionsAndInitialFrame) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
    MissionScript script(config,"Tutorial2");CHECK(script.Loaded());bool dialog{};std::uint32_t depth{};double now=10;std::vector<std::string> pages;RawCampaignTutorialOneBindings* active{}; // 실제 UI/시계의 명시 외부 입력만 공급한다.
    services.missionOutcome.demo=[] { return 0U; };services.missionOutcome.tellBlocked=[&] { return dialog ? 1U : 0U; };
    services.missionOutcome.section=[&](std::string_view name) { CHECK(!script.Section(name));return false; }; // 이 보호 Early2에는 승패 사건 섹션이 없다.
    services.missionOutcome.closeTransient=[] { throw std::logic_error("보호 Early2에 없는 승패 창 정리"); };
    services.missionOutcome.onSection=[] { throw std::logic_error("보호 Early2에 없는 승패 창 정지"); };
    CampaignLoginServices login;login.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };login.wallSeconds=[] { return 100.25; };login.clock=[] { return 100250U; };login.refresh=[] {};login.playerUiBusy=[] { return false; };login.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};login.log=[](std::string_view) {};
    services.ai.participation.addChat=[&external](std::uint32_t,std::string_view,std::uint32_t) { ++external.notices; };services.ai.participation.clearChat=[&external](std::uint32_t) { ++external.notices; }; // 로그인 패킷이 요청하는 실제 채팅 UI 경계를 명시한다.
    services.localLogin=std::move(login);auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.LoadLocalConnection();
    CHECK(session.LocalConnectionPhase()==14 && session.LocalOwner()==1 && session.PlayerKnown()==1 && !session.MissionPriestDead(1));
    MissionFrameGateState gate;gate.tutorialNumber=2;MissionStartState startState;CampaignMissionStartEffects startEffects; // 단계/자금/완료 결과를 검사에서 지정하지 않는다.
    startEffects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t>) { throw std::logic_error("AI off Early2의 AI 생성 도달"); };startEffects.gameSeconds=[&] { return now; };startEffects.refreshSp=[] {};startEffects.refreshCommandGump=[] {};startEffects.resetCamera=[] {};startEffects.randomGeysers=[] { throw std::logic_error("Early2 Start의 세계 랜덤 간헐천"); };
    RawCampaignMissionStartBindings start(session,script,gate,startState,session.MissionStartSettings(),session.MissionAiSettings(),std::move(startEffects));
    CampaignTutorialOneEffects effects;effects.frame.gameSeconds=[&] { return now; };effects.frame.paused=[&] { return depth!=0; };effects.frame.dialogsClear=[&] { return !dialog; };effects.frame.knowledgeOpen=[] { return false; };effects.frame.guideOpen=[] { return false; };effects.frame.blockingGumpOpen=[] { return false; };
    effects.frame.pause=[&] { ++depth; };effects.frame.resume=[&] { CHECK(depth>0);--depth; };effects.frame.endDemo=[] { throw std::logic_error("Early2의 데모 종료 도달"); };
    effects.tell=[&](std::string_view name) { if (!script.Section(name)) return false;dialog=true;pages.emplace_back(name);return true; };
    effects.closeTransient=[&] { CHECK(dialog && active && !gate.scriptPaused); }; // Tell→팝업 정리→자기 정지의 순서까지 관찰한다.
    effects.selected=[] { return 0U; };effects.resetSelection=[] { throw std::logic_error("Early2 초기A에서 선택 초기화 도달"); };effects.clearSelection=[] { throw std::logic_error("Early2 초기A에서 선택 해제 도달"); }; // 이 회귀는 선택 효과가 없는 실제 A만 실행한다.
    RawCampaignTutorialOneBindings runtime(session,script,start,gate,startState,std::move(effects));active=&runtime;
    runtime.Frame();CHECK(!gate.started && pages.empty() && gate.lockFrames==10);start.Start();CHECK(gate.started && session.Sp(1)==10000 && session.MissionStartSettings().denySalvage==1 && session.TotalMade().BuiltForGenus(0x200)==0);
    // 입력이 없는 실제 화면 호출9회에는 초기 안내가 없고10회에만 A를 표시한다.
    for (int frame=0;frame<9;++frame) runtime.Frame();
    CHECK(pages.empty() && gate.lockFrames==1 && !depth);runtime.Frame();CHECK(pages==std::vector<std::string>{"A."} && dialog && depth==1 && gate.scriptPaused==1 && !gate.lockFrames && runtime.StageShown()==1);
    runtime.Frame();CHECK(depth==1 && gate.clearFrames==0 && gate.stage=='A');dialog=false; // 사용자가 대화상자를 닫는 실제 UI 경계다.
    // 연속 빈 호출6회에는 자기 정지가 남고7번째에 한 깊이만 반납한다.
    for (int frame=0;frame<6;++frame) runtime.Frame();
    CHECK(depth==1 && gate.clearFrames==6 && gate.stage=='A');runtime.Frame();CHECK(!depth && !gate.scriptPaused && !gate.clearFrames);
    CHECK(gate.stage=='A' && session.BattleOption(2)==2 && session.BattleOption(4)==0 && session.EnergySettings().radiusSquared==196);
    CHECK(session.Sp(1)==10000 && session.TotalMade().BuiltForGenus(0x200)==0 && session.MissionStartSettings().denySalvage==1);
    CHECK(!session.Core().bookkeeping.depth && !session.Core().destroy.PreDepth() && !session.Core().destroy.PostDepth());
}

// 실제 네 미션 로그인/Start와 원문 A안내를 실행하며 Early5의 썬더 투사체→스톤 타워 피격 사건 두 인자가 손실되지 않는지 확인한다.
TEST_CASE(CampaignTutorialLater_ActualSessionsAndThreeDwordEvents) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    const std::array<float,4> money{2000,2000,4000,7500}; // 보호 영어 Header에서 읽어 별도로 고정한 초기 SP다.
    // 미션마다 실제 월드/로그인/Start/정지 저장소를 새로 만들며 앞 미션의 상태를 재사용하지 않는다.
    for (std::uint32_t number=3;number<=6;++number) {
        SessionServicesFixture external;auto services=external.Services();bool dialog{},nestedTrigger{},nestedObserved{};std::uint32_t depth{};double now=10;std::vector<std::string> pages;RawCampaignTutorialOneBindings* active{}; // 장치/대화상자/시계와 명시 재진입만 검사 외부 경계다.
        ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
        config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
        MissionScript script(config,"Tutorial"+std::to_string(number));CHECK(script.Loaded());
        const auto show=[&](std::string_view name) { if (!script.Section(name)) return false;dialog=true;pages.emplace_back(name);return true; }; // 실제 현재 원문에 있는 섹션만 창으로 반환한다.
        services.missionOutcome.demo=[] { return 0U; };services.missionOutcome.tellBlocked=[&] { return dialog ? 1U : 0U; };
        services.missionOutcome.section=show;services.missionOutcome.closeTransient=[] {};services.missionOutcome.onSection=[&] { CHECK(active);active->Gate().PauseForDialog(); };
        services.ai.participation.addChat=[&](std::uint32_t,std::string_view,std::uint32_t) { ++external.notices; };services.ai.participation.clearChat=[&](std::uint32_t) { ++external.notices; };
        services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xb3}); }; // aiOff도 현재 사제의 실제 AI 생성/설정 사슬을 방문한다.
        services.ai.defaultName=[] { return std::string("Default AI"); };services.ai.localTransport=[] { return false; };
        services.ai.participation.sendAlliance=[&](std::uint32_t,std::uint32_t,std::uint32_t) { ++external.notices; };
        services.ai.participation.assertion=[](PlayerParticipationAssertion) { throw std::logic_error("Early3~6 실제 시작의 예상하지 않은 참여 진단"); };
        services.production.allocationBytes=[](std::span<std::uint8_t> bytes) { CHECK(bytes.size()==kProductionDeckBytes);std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); }; // 실제 Deck 생성자가 보존할 할당 입력만 공급한다.
        services.production.effects.configInt=[](std::string_view key) { CHECK(key=="getTeleportTechFree");return 0; };
        services.production.effects.modeAssertion=[] { throw std::logic_error("Early3~6 실제 생산의 Fort/Battle 모드 진단"); };
        CampaignLoginServices login;login.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };login.wallSeconds=[] { return 100.25; };login.clock=[] { return 100250U; };login.refresh=[] {};login.playerUiBusy=[] { return false; };login.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};login.log=[](std::string_view) {};
        services.localLogin=std::move(login);auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.LoadLocalConnection();
        CHECK(session.LocalConnectionPhase()==14 && session.LocalOwner()==1 && !session.MissionPriestDead(1));
        MissionFrameGateState gate;gate.tutorialNumber=number;MissionStartState startState;CampaignMissionStartEffects startEffects; // 진행 단계와 SP는 검사에서 쓰지 않는다.
        startEffects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); }; // 실제600BYTE 기본값 생성 전에 미초기화 입력만 공급한다. aiOff 판정은 원본 Start/AI 몸체가 수행한다.
        startEffects.gameSeconds=[&] { return now; };startEffects.refreshSp=[] {};startEffects.refreshCommandGump=[] {};startEffects.resetCamera=[] {};startEffects.randomGeysers=[] { throw std::logic_error("Early3~6 Start의 세계 랜덤 간헐천 도달"); };
        RawCampaignMissionStartBindings start(session,script,gate,startState,session.MissionStartSettings(),session.MissionAiSettings(),std::move(startEffects));
        CampaignTutorialOneEffects effects;effects.frame.gameSeconds=[&] {
            if (nestedTrigger) { nestedTrigger=false;nestedObserved=true;CHECK(active->Event({0x020000b5U,131,78})==0 && gate.stage=='A'); } // 중첩 사건이 바깥 사건의 TYPE 두 칸을 덮어쓰지 않아야 한다.
            return now;
        };effects.frame.paused=[&] { return depth!=0; };effects.frame.dialogsClear=[&] { return !dialog; };effects.frame.knowledgeOpen=[] { return false; };effects.frame.guideOpen=[] { return false; };effects.frame.blockingGumpOpen=[] { return false; };
        effects.frame.pause=[&] { ++depth; };effects.frame.resume=[&] { CHECK(depth>0);--depth; };effects.frame.endDemo=[] { throw std::logic_error("Early3~6의 데모 종료 도달"); };
        effects.tell=show;effects.closeTransient=[] {};effects.selected=[] { return 0U; };effects.resetSelection=[] {};effects.clearSelection=[] {};
        RawCampaignTutorialOneBindings runtime(session,script,start,gate,startState,std::move(effects));active=&runtime;start.Start();CHECK(session.Sp(1)==money[number-3]);
        // 입력 없는 열 번째 화면 호출에서만 실제 A안내를 표시하고 자기 정지 한 깊이를 보유한다.
        for (int frame=0;frame<9;++frame) runtime.Frame();
        CHECK(pages.empty() && gate.lockFrames==1);runtime.Frame();CHECK(pages==std::vector<std::string>{"A."} && depth==1 && dialog && runtime.StageShown()==1);
        dialog=false;
        // 현재 초기 자료로는 A판정의 조건이 충족되지 않으며 일곱 번째 빈 화면에서만 재개한다.
        for (int frame=0;frame<7;++frame) runtime.Frame();
        CHECK(!depth && !gate.scriptPaused && gate.stage=='A' && session.BattleOption(2)==2);
        if (number==3) {
            CHECK(runtime.Event({0x220000b4U,123,456})==0 && gate.stage=='B'); // 실제 수입 사건과 modifier를 함께 전달한다.
            now=10.5;
            // 같은 현재 Graph가 다르면 안내 뒤에도 B를 유지한다. 검사에서 다리/진행을 주입하지 않는다.
            for (int frame=0;frame<10;++frame) runtime.Frame();
            CHECK(pages.back()=="B." && dialog && depth==1);
        } else if (number==5) {
            CHECK(runtime.Event({0x020000b5U,132,77})==0 && gate.stage=='A'); // 잘못된 두 번째 TYPE은 거부한다.
            CHECK(runtime.Event({0x020000b5U,131,78})==0 && gate.stage=='A'); // 잘못된 첫 번째 TYPE도 거부한다.
            nestedTrigger=true;
            CHECK(runtime.Event({0x220000b5U,132,78})==0 && nestedObserved && gate.stage=='B' && gate.lockFrames==10); // 중첩 입력 뒤에도 실제 두 TYPE/명령 modifier를 그대로 써야만 예약한다.
        }
        CHECK(session.Sp(1)==money[number-3] && !session.Core().bookkeeping.depth && !session.Core().destroy.PreDepth() && !session.Core().destroy.PostDepth());
    }
}

// 실제 메뉴→427070→로컬전송→444bc0→0x5c가 같은 경제/내용물/공간/프로세스를 사용하는지 확인한다.
TEST_CASE(CampaignCarrierDrop_ActualCastRequestAndTransport) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();netstorm::zacket::LocalMissionTransport* wire{};std::vector<std::array<std::uint32_t,3>> queued;std::vector<std::array<std::int32_t,4>> popups;
    services.carrierRequest=[&](std::int32_t target,std::span<const std::uint8_t> packet) { CHECK(wire && target==9);if (wire) wire->Send(target,packet); };
    services.carrierDropPopup=[&](std::int32_t cost,std::int32_t x,std::int32_t y,std::uint32_t reference) { popups.push_back({cost,x,y,static_cast<std::int32_t>(reference)}); };
    services.assets.queueCommand=[&](std::uint32_t command,std::int32_t x,std::int32_t y) { queued.push_back({command,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)}); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { session.RequestCarrierDrop(Sid{50}); }));session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto raw=core.pool.Slot(priest);const RawPathPoint point{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))};
    const auto caster=session.World().Create(133,2);session.World().Pop(caster,point.x,point.y,0);core.owner.Set(caster,1);const auto prayer=session.CreateContained(caster,168,0,1,0,0);CHECK(SquidForms(core.pool).Parent(prayer)==caster);session.SetSp(1,10000);const auto before=session.Sp(1);const auto cost=core.types[168].cost;
    std::uint32_t selected{},mouseType{},mode{};std::int32_t mouseX{},mouseY{};std::array<std::uint32_t,2> remembered{};std::array<std::uint32_t,1> multiple{};std::int32_t count{};RawUserHighlight highlight({mode,selected,mouseType,mouseX,mouseY},{});UserSelectionHooks select;select.paused=[] { return 0U; };select.enabled=[] { return 1U; };select.blocked=[] { return 0U; };select.dump=[](Sid) {};select.highlight=[&](std::uint32_t value) { highlight.Highlight(value); };select.assertion=select.previewAssertion=[] { throw std::logic_error("시전 선택 예상 밖 진단"); };
    auto selection=session.MakeUserSelection({selected,mouseType,remembered,multiple,count},std::move(select));CHECK(selection->Select(caster.value,0,0)==caster.value);const auto initialPosition=std::array<std::uint32_t,2>{Get(core.pool.Slot(caster),14),Get(core.pool.Slot(caster),18)};
    const auto& registration=link.transport.Dispatch().Registration(0x50);CHECK(registration.length==9 && registration.version==1 && !registration.mismatch && !registration.observer);
    CHECK(session.CommandCarrierDrop(caster,0x0300011a,caster)==1);CHECK(session.Sp(1)==before-cost && selected==0 && popups.size()==1);if (!popups.empty()) CHECK(popups.front()[0]==static_cast<std::int32_t>(cost) && popups.front()[3]==0);
    const auto* first=core.processes.FindEvent(caster,0x493);CHECK(first && core.pool.Slot(first->Form())[10]==61);const auto firstForm=first ? first->Form() : Sid{};
    CHECK(link.recorded.empty() && !link.transport.QueuedFrames() && ConstructionLive(core,168).empty());link.transport.Flush();CHECK(link.transport.QueuedFrames()==1);CHECK(link.transport.Poll()==1 && link.recorded.size()==1);
    if (!link.recorded.empty()) { const auto& packet=link.recorded.front();CHECK(packet.size()==9 && packet[0]==1 && packet[1]==0x50 && Get(packet,4,2)==caster.value && Get(packet,6,2)==prayer.value); }
    const auto spells=ConstructionLive(core,168);CHECK(spells.size()==1);if (spells.size()==1) { const auto bomb=spells.front();CHECK(Get(core.pool.Slot(bomb),0)==0x502370U && Get(core.pool.Slot(bomb),12,2)==caster.value && core.pool.Slot(bomb)[34]==1);CHECK(core.processes.FindEvent(bomb,0)!=nullptr); }
    const auto* second=core.processes.FindEvent(caster,0x493);CHECK(second && second->Form()!=firstForm && core.processes.FromForm(firstForm));CHECK(Get(core.pool.Slot(caster),14)==initialPosition[0] && Get(core.pool.Slot(caster),18)==initialPosition[1] && !(core.pool.Slot(caster)[11]&7U));CHECK(SquidForms(core.pool).Parent(prayer)==caster && !(core.pool.Slot(prayer)[11]&3U) && session.Sp(1)==before-cost);
    CHECK(std::find(queued.begin(),queued.end(),std::array<std::uint32_t,3>{0x030000b2,168,0})!=queued.end());CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());wire=nullptr;
}

// SP가 부족하면 실제 요청 단계에서 팝업/493/송신을 만들지 않으며 내용물을 유지한다.
TEST_CASE(CampaignCarrierDrop_ActualInsufficientFunds) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::size_t sent{},popups{};
    services.carrierRequest=[&](std::int32_t,std::span<const std::uint8_t>) { ++sent; };services.carrierDropPopup=[&](std::int32_t,std::int32_t,std::int32_t,std::uint32_t) { ++popups; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;
    const auto raw=core.pool.Slot(priest);const auto caster=session.World().Create(133,2);session.World().Pop(caster,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),0);core.owner.Set(caster,1);const auto prayer=session.CreateContained(caster,168,0,1,0,0);session.SetSp(1,0);session.RequestCarrierDrop(caster,prayer);CHECK(!sent && !popups && session.Sp(1)==0 && !core.processes.FindEvent(caster,0x493));CHECK(SquidForms(core.pool).Parent(prayer)==caster && !(core.pool.Slot(prayer)[11]&3U));
}

// 공통 명령 위임이 미처리0과 현재 사제 가상 소유자 변경을 같은 Session에서 수행하는지 확인한다.
TEST_CASE(CampaignObjectCommand_ActualCarrierParentAndPriestOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
    MissionScript script(config,"Tutorial5");CHECK(script.Loaded());
    auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();
    Sid enemy{}; // 실제 저장 미션에 존재하는 적 사제만 사용한다.
    // 소유자/HP를 검사에서 덮어쓰지 않고 원본 로드 결과로 대상을 고른다.
    for (const auto& object:session.World().Objects()) if (object.object.type==158 && object.object.owner==2) { enemy=object.sid;break; }
    CHECK(enemy.value);if (!enemy.value) return;
    const auto before=session.Sp(1);const auto packets=external.packets.size();const auto hp=Get(session.Core().pool.Slot(enemy),26);const auto originalOwner=Get(session.Core().pool.Slot(enemy),12,2); // 전투의 사제 가상 Owner는 원래 소유자WORD를 사용한다.
    CHECK(session.CommandObject(Sid{},0xffffffffU)==0);
    CHECK(session.CommandCarrierDrop(enemy,0x030003feU,enemy)==0);
    CHECK(session.CommandCarrierDrop(enemy,0xff0000afU,enemy)==1);
    CHECK(session.Core().pool.Slot(enemy)[34]==2 && (Get(session.Core().pool.Slot(enemy),12,2)&0x7fU)==(originalOwner&0x7fU) && !(session.Core().pool.Slot(enemy)[12]&0x80U)); // 전투 중 로컬 소유자 인자는 원래 소유자를 바꾸지 않으며 로컬 표식도 켜지지 않는다.
    CHECK(Get(session.Core().pool.Slot(enemy),26)==hp && session.Sp(1)==before && external.packets.size()==packets);
}

// 원본 Early5의 적 사제를 실제 검색/피해/포획/내려놓기로 처리한다. 시전자/내용물/잔액만 명시 검사 입력이다.
TEST_CASE(CampaignDevastation_ActualEnemyCaptureAndDropFromCast) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();netstorm::zacket::LocalMissionTransport* wire{};
    services.carrierRequest=[&](std::int32_t target,std::span<const std::uint8_t> packet) { CHECK(wire && target==9);if (wire) wire->Send(target,packet); };
    services.carrierDropPopup=[&](std::int32_t,std::int32_t,std::int32_t,std::uint32_t) { ++external.notices; };services.unitLost=[&](Sid) { ++external.notices; };
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");MissionScript script(config,"Tutorial5");CHECK(script.Loaded());
    auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.Load();auto core=session.Core();SessionPickupWire link(session);wire=&link.transport;
    Sid enemy{},temple{}; // 원본에서 로드한 적 사제와 신전만 보관한다.
    // 저장 미션의 실제 소유자와 TYPE로 대상을 고르고 HP/좌표는 바꾸지 않는다.
    for (const auto& object:session.World().Objects()) {
        if (object.object.type==158 && object.object.owner==2) enemy=object.sid;
        if (object.object.type==125 && object.object.owner==2) temple=object.sid;
    }
    CHECK(enemy.value && temple.value);if (!enemy.value || !temple.value) return;
    const auto victim=core.pool.Slot(enemy);const auto x=std::bit_cast<float>(Get(victim,14)),y=std::bit_cast<float>(Get(victim,18));const auto initialHealth=Get(victim,26,2),templeHealth=Get(core.pool.Slot(temple),26,2);
    const auto caster=session.World().Create(133,2);core.owner.Set(caster,1);session.World().Pop(caster,x-1,y-1,0);const auto prayer=session.CreateContained(caster,168,0,1,0,0);session.SetSp(1,10000);
    std::uint32_t selected{},mouseType{},mode{};std::int32_t mouseX{},mouseY{};std::array<std::uint32_t,2> remembered{};std::array<std::uint32_t,1> multiple{};std::int32_t count{};RawUserHighlight highlight({mode,selected,mouseType,mouseX,mouseY},{});
    UserSelectionHooks select;select.paused=[] { return 0U; };select.enabled=[] { return 1U; };select.blocked=[] { return 0U; };select.dump=[](Sid) {};select.highlight=[&](std::uint32_t value) { highlight.Highlight(value); };select.assertion=select.previewAssertion=[] { throw std::logic_error("적/지형 시전 선택의 예상 밖 진단"); };
    auto selection=session.MakeUserSelection({selected,mouseType,remembered,multiple,count},std::move(select));CHECK(selection->Select(caster.value,0,0)==caster.value);
    const auto money=session.Sp(1);session.RequestCarrierDrop(caster,prayer);CHECK(session.Sp(1)==money-core.types[168].cost && Get(core.pool.Slot(enemy),26,2)==initialHealth);link.transport.Flush();CHECK(link.transport.Poll()==1);
    const auto spells=ConstructionLive(core,168);CHECK(spells.size()==1);if (spells.size()!=1) return;const auto bomb=spells.front();const auto* initial=core.processes.FindEvent(bomb,0);CHECK(initial);if (!initial) return;const auto form=initial->Form();std::size_t frames{};
    // 실제 시전 예약만 진행한다. 검색 목록·대상 HP·지형·시전 시간은 검사에서 쓰지 않는다.
    while (!(core.pool.Slot(bomb)[11]&3U) && frames<4096) {
        auto* current=dynamic_cast<RegularProcess*>(core.processes.FromForm(form));CHECK(current);if (!current) break;core.processState.now=current->Time();current->RunFrame();++frames;
    }
    CHECK(frames>1 && frames<4096 && (core.pool.Slot(bomb)[11]&3U)==3U && !core.processes.FromForm(form));CHECK(Get(core.pool.Slot(enemy),26,2)==1 && initialHealth==100);CHECK(core.pool.Slot(enemy)[34]==2 && !(core.pool.Slot(enemy)[11]&3U));
    CHECK(Get(core.pool.Slot(temple),26,2)==templeHealth);CHECK(Get(core.pool.Slot(caster),26,2)==50 && !core.processes.FindEvent(caster,0x493));CHECK(SquidForms(core.pool).Parent(prayer)==caster && !(core.pool.Slot(prayer)[11]&3U));CHECK(session.Sp(1)==money-core.types[168].cost);
    // 피해로 행동 불가가 된 실제 적 사제에게 경로로 도착하여 원래 포획 요청을 발생시킨다.
    const auto argument=Get(core.pool.Slot(enemy),12,2);auto* movement=session.StartCarrierMovement({caster,{x,y},1,0,0,0,0});CHECK(movement);if (!movement) return;const auto movementForm=movement->Form();const auto arrivalTime=movement->Runtime().Time(movement->Runtime().Count()-1)+0.01;
    // 경로는 한 호출에 한 꼭짓점만 소비하므로 마지막 시각에서도 실제 프레임을 반복한다.
    for (std::size_t step=0;step<1024;++step) {
        auto* current=dynamic_cast<RawPathProcess*>(core.processes.FromForm(movementForm));if (!current) break;core.processState.now=arrivalTime;current->RunFrame();
    }
    CHECK(std::bit_cast<float>(Get(core.pool.Slot(caster),14))==x && std::bit_cast<float>(Get(core.pool.Slot(caster),18))==y && !core.processes.FromForm(movementForm));
    link.transport.Flush();CHECK(link.transport.Poll()==1);
    const auto captured=session.Priests().Carrier().FindContained(caster,0x20,158,0);CHECK(captured.value);if (!captured.value) return;
    CHECK((core.pool.Slot(enemy)[11]&1U)!=0 && SquidForms(core.pool).Parent(captured)==caster && Get(core.pool.Slot(captured),12,2)==argument);CHECK(session.FindPriestParticipation(2)==caster.value);
    // 명시적으로 선택한 일반 포로 내용물을 실제0x50과 건설 확정 경로로 내려놓는다.
    const auto beforeDrop=session.Sp(1);session.RequestCarrierDrop(caster,captured);CHECK(!(core.pool.Slot(captured)[11]&3U));link.transport.Flush();CHECK(link.transport.Poll()==1);
    const Sid restored{static_cast<std::uint16_t>(session.FindPriestParticipation(2))};CHECK(restored.value && restored!=caster);if (restored.value && restored!=caster) CHECK(core.pool.Slot(restored)[10]==158 && (Get(core.pool.Slot(restored),12,2)&0x7fU)==2 && !(core.pool.Slot(restored)[11]&3U));
    CHECK((core.pool.Slot(captured)[11]&3U)==3U && !session.Priests().Carrier().FindContained(caster,0x20,158,0).value);CHECK(session.Sp(1)==beforeDrop && SquidForms(core.pool).Parent(prayer)==caster && !(core.pool.Slot(prayer)[11]&3U));
    CHECK(link.recorded.size()==3 && link.recorded[0][1]==0x50 && link.recorded[1][1]==0x4f && link.recorded[2][1]==0x50);CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());wire=nullptr;
}

// 최신 사용자 TEST02 원문/실제 로그인/시나리오 시작 뒤 기존 Bulf를 선택하고 자연41 경로를 끝낸다.
TEST_CASE(CampaignTest02_ActualStoredBulfSelectionAndNaturalMovement) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto services=external.Services();UseWorldTestLogin(services);std::size_t footsteps{};
    services.production.allocationBytes=[](std::span<std::uint8_t> bytes) { CHECK(bytes.size()==kProductionDeckBytes);std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); }; // 이제실제로진행하는AI생산의힙입력만공급한다.
    services.production.effects.configInt=[](std::string_view key) { CHECK(key=="getTeleportTechFree");return 0; }; // 기존튜토리얼생산검사와동일한설정입력이다.
    services.production.effects.modeAssertion=[] { throw std::logic_error("TEST02 실제생산모드진단"); }; // 잘못된모드를성공으로넘기지않는다.
    const auto sound=services.sound.playOnceAt;
    services.sound.playOnceAt=[&](float x,float y,SoundHandle id,std::uint32_t first,std::uint32_t second) {
        // 실제 장치 요청만 세며 공용 난수나 선택 파일 번호는 바꾸지 않는다.
        for (int variant=1;variant<=5;++variant) if (id==external.names.Lookup("bulfWalk"+std::to_string(variant)+".wav")) { CHECK(first==0 && second==0);++footsteps; }
        return sound(x,y,id,first,second);
    };
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    services.ai.defaultName=[] { return std::string("AI"); };
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
    MissionScript script(config,"TEST02");CHECK(script.Loaded() && script.MissionType()=="Tutorial");
    auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},script.FortName(),std::move(services),{1024,768});
    auto& session=world.Session();auto core=session.Core();CHECK(session.LocalConnectionPhase()==14);
    const auto cursors=BindWorldTestInput(world);MissionFrameGateState gate;MissionStartState state;CampaignMissionStartEffects effects;
    effects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    effects.gameSeconds=[&] { return core.processState.now; };effects.refreshSp=[] {};effects.refreshCommandGump=[] {};effects.resetCamera=[] {};
    effects.randomGeysers=[] { throw std::logic_error("TEST02 moreGeysers0에서 무작위 간헐천 도달"); };
    RawCampaignMissionStartBindings start(session,script,gate,state,session.MissionStartSettings(),session.MissionAiSettings(),std::move(effects));
    start.Start();session.PumpLocalConnection();CHECK(gate.started==1 && session.Sp(1)==500000 && session.Sp(2)==0 && session.Sp(3)==0);
    CHECK(session.Participation().Brain(2) && session.Participation().Brain(3));Sid bulf{};
    // 사용자 맵에서 로드한 기존 객체만 사용하며 운반체/사제/좌표를 새로 만들지 않는다.
    for (const auto& object:session.World().Objects()) if (object.object.type==135 && object.object.owner==1) { CHECK(!bulf.value);bulf=object.sid; }
    CHECK(bulf.value);if (!bulf.value) return;
    const auto raw=core.pool.Slot(bulf);CHECK(Get(raw,0)==0x503e50U && session.Priests().MovementProcessType(135)==41);
    const RawPathPoint source{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))};
    world.SetMousePoint(world.ToScreen(source.x,source.y));world.Select(bulf.value);CHECK(world.InputMode()==5 && world.HighlightSettings().selectedSid==bulf.value);
    RawPathPoint destination{};bool found{};std::optional<ScreenPoint> target;
    // 같은 실제 본섬의 충분히 떨어진 빈 칸만 골라 원본 BFS가 경로를 결정하게 한다.
    for (int y=static_cast<int>(source.y)-7;y<=static_cast<int>(source.y)+7 && !found;++y) {
        // 점유와 실제 표면을 모두 읽으며 테스트에서 지형/다리를 추가하지 않는다.
        for (int x=static_cast<int>(source.x)-7;x<=static_cast<int>(source.x)+7 && !found;++x) {
            if (x<1 || y<1 || x>=255 || y>=255 || std::abs(x-source.x)+std::abs(y-source.y)<8) continue;
            const auto cell=static_cast<std::size_t>(y*256+x);
            if ((core.spots[cell]&6U) && !(core.spots[cell]&8U) && core.hash.Cell(0,x,y)) {
                // 서버는귀환장소를별도로조회하므로표면spot만빈칸으로판단하지않는다. 현재일반finder로주변자산도확인한다.
                RawSquidFinder empty(core.pool,core.hash,core.types);if (empty.Begin({x-1,y-1,x+1,y+1},1).value) continue;
                const auto probe=world.Point("cell:"+std::to_string(x)+','+std::to_string(y));if (!probe) continue;
                world.SetMousePoint(*probe);world.UpdateMovementCursor();
                if (!cursors->empty() && cursors->back()==18) { destination={static_cast<float>(x),static_cast<float>(y)};target=probe;found=true; }
            }
        }
    }
    CHECK(found);if (!found) return;
    // 표시경계점이아닌칸내부이며실제이동커서18인후보만클릭한다. 신전귀환/운반명령과구분한다.
    CHECK(target.has_value() && cursors->back()==18);if (!target) return;
    CHECK(world.MoveSelected(*target));session.PumpLocalConnection();const auto form=FindAttachedProcess(core.pool,bulf,41);
    auto* path=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(path && path->MovementType()==41);if (!path) return;
    CHECK(Get(path->Runtime().Bytes(),0)==0x50e354U);const auto end=path->Runtime().Time(path->Runtime().Count()-1);const auto initialSp=session.Sp(1);
    const auto endpoint=path->Runtime().Point(path->Runtime().Count()-1);std::cout<<"TEST02 요청"<<destination.x<<","<<destination.y<<"/실제경로끝"<<endpoint.x<<","<<endpoint.y<<"/커서"<<cursors->back()<<'\n';
    // 원본41 Init은 cardinal 경로를 선택하며 대각 방향을 만들어 넣지 않는다.
    for (std::int32_t index=0;index<path->Runtime().Count();++index) CHECK((path->Runtime().Direction(index)&1)==0);
    std::size_t frames{};bool turning{};
    // AI와 같은 Kernel을 시간 순서대로 실행하며 도착/경로 제거 결과를 쓰지 않는다.
    for (;frames<6000 && core.processes.FromForm(form);++frames) {
        world.Step(.025);const auto direction=RawPathAnimation::Direction(core.frames[135],static_cast<std::int32_t>(Get(core.pool.Slot(bulf),36)));
        turning|=direction==4 || direction==5;
        if (core.processState.now>end+5) break;
    }
    CHECK(!core.processes.FromForm(form) && frames>1 && turning && footsteps>0);
    std::cout<<"TEST02 도착현재좌표"<<std::bit_cast<float>(Get(raw,14))<<","<<std::bit_cast<float>(Get(raw,18))<<'\n';
    CHECK(std::bit_cast<float>(Get(raw,14))==destination.x && std::bit_cast<float>(Get(raw,18))==destination.y && session.Sp(1)==initialSp);
    CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
    std::cout<<"TEST02 실제 기존 Bulf"<<bulf.value<<" 선택/40h/41: "<<source.x<<","<<source.y<<"→"<<destination.x<<","<<destination.y<<"/자연 프레임"<<frames<<"/회전"<<turning<<"/발소리"<<footsteps<<"/SP"<<session.Sp(1)<<'\n';
}

// 원본TEST02 작업본의 실제14자산 슬롯과 동일 Fort/시나리오를 읽은 생산 세션의 시작 상태를 비교한다.
TEST_CASE(CampaignTest02_OriginalRuntimeStartupAssetParity) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto services=external.Services();UseWorldTestLogin(services);
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    services.ai.defaultName=[] { return std::string("AI"); };
    const auto directory=std::filesystem::path(__FILE__).parent_path()/"fixtures";
    const auto original=directory.parent_path().parent_path().parent_path()/"originals";
    BaseFileSystem files(directory/"test02-runtime",original);files.RegisterArchive(original/"netstorm.tarc");
    std::ifstream input(directory/"test02-runtime/d/TEST02.fort",std::ios::binary);
    const std::vector<std::uint8_t> image{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};CHECK(image.size()==2150);
    services.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(image);
    auto fort=FortTemplate::Parse(image,assets.game.TypeTable());
    GameWorld world(assets.game,files,std::move(fort),MissionPlayers{},"TEST02",std::move(services),{1024,768});
    auto& session=world.Session();auto core=session.Core();
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
    MissionScript script(config,"TEST02");MissionFrameGateState gate;MissionStartState state;CampaignMissionStartEffects effects;
    effects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    effects.gameSeconds=[&] { return core.processState.now; };effects.refreshSp=[] {};effects.refreshCommandGump=[] {};effects.resetCamera=[] {};
    effects.randomGeysers=[] { throw std::logic_error("원본TEST02 moreGeysers0에서 무작위 간헐천 도달"); };
    RawCampaignMissionStartBindings start(session,script,gate,state,session.MissionStartSettings(),session.MissionAiSettings(),std::move(effects));
    start.Start();session.PumpLocalConnection();CHECK(gate.started==1 && session.Sp(1)==500000);
    world.Step(.025); // 원본 Go 뒤처럼 현재 시각의 초기 Regular를 같은 Kernel로 처리한다.
    const auto path=(directory/"test02-original-runtime.tsv").string();std::size_t count{};
    // 기대값은원본프로세스에서읽은type/owner/QA/좌표/HP이며검사에서결과를쓰지않는다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==7);if (row.size()!=7) continue;const auto sid=Sid{static_cast<std::uint16_t>(Number(row[0]))};const auto raw=core.pool.Slot(sid);
        std::array<std::uint32_t,6> actual{raw[10],raw[34],Get(raw,12,2),Get(raw,14),Get(raw,18),Get(raw,26)};
        // 같은 원문에서 읽은 각 필드를 순서대로 비교한다.
        for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=Number(row[index+1])) {
            std::cout<<"TEST02 원본 시작 SID"<<sid.value<<" 필드"<<index<<" C++"<<actual[index]<<" 원본"<<row[index+1]<<'\n';CHECK(false);
        }
        ++count;
    }
    CHECK(count==14);
}

// 실제40aca0의 독립909입력과 송신하지 않는 관문의 바이트 보존을 확인한다.
TEST_CASE(CampaignDevastation_PacketLengthNative909) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-packet-x86.tsv").string();std::size_t rows{};
    // 기대값을 제품 수식으로 만들지 않고 보호PE 실행의 전체210바이트와 비교한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==3);if (row.size()!=3) continue;const auto input=Split(row[0],',');CHECK(input.size()==3);if (input.size()!=3) continue;
        std::array<std::uint8_t,210> packet;packet.fill(0xa7);Put(packet,0,1,1);Put(packet,1,0x5f,1);Put(packet,2,Number(input[2]),2);Put(packet,4,15000,2);Put(packet,6,Number(input[0]));
        FinalizeBombEffectPacket(packet,Number(input[1])!=0);CHECK(Hex(packet)==row[1]);CHECK(row[1]==row[2]);++rows;
    }
    CHECK(rows==909);FinalizeBombEffectPacket({},false);CHECK(Throws([] { FinalizeBombEffectPacket({},true); }));
    std::array<std::uint8_t,10> shortPacket{};Put(shortPacket,6,1);CHECK(Throws([&] { FinalizeBombEffectPacket(shortPacket,true); }));
}

// 실제41e3d0의 독립32입력으로 현재 TYPE/프레임/좌표의 평문 그림 요청을 비교한다.
TEST_CASE(CampaignDevastation_AdditionalDrawNative32) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/devastation-draw-x86.tsv").string();SidPool pool(OriginalEdition::Patch1078,24000,true);const auto slot=pool.Slot(Sid{50});std::span<std::uint8_t> raw(const_cast<std::uint8_t*>(slot.data()),slot.size());std::size_t rows{};
    // SHEET 토큰은 같은TYPE가 가리키는 실제 그림 묶음의 독립 입력이며 제품은 현재TYPE를 전달한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        const auto input=Split(row[0],',');CHECK(input.size()==4 && row.size()==3);if (input.size()!=4 || row.size()!=3) continue;
        Put(raw,0,0x502370);raw[10]=168;Put(raw,36,Number(input[0]));std::string trace;
        DrawDevastationAdditional(pool,Sid{50},std::bit_cast<std::int32_t>(Number(input[1])),std::bit_cast<std::int32_t>(Number(input[2])),[&](std::uint32_t type,std::int32_t frame,std::int32_t x,std::int32_t y) { CHECK(type==168);trace="draw:"+input[3]+':'+std::to_string(std::bit_cast<std::uint32_t>(frame))+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y)); });
        CHECK(trace==row[1] && row[1]==row[2]);++rows;
    }
    CHECK(rows==32);
}

// 실제 Session의0x5c→SharedRegular/0x5f→493종료·효과음·공간/프로세스 삭제를 확인한다.
TEST_CASE(CampaignDevastation_ActualActivationEffectLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");MissionScript script(config,"Tutorial5");CHECK(script.Loaded());
    auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),external.Services());session.Load();auto core=session.Core();
    Sid caster{};
    // 실제 로드한 로컬 사제를 찾으며 검사 전용 부모나 진행 상태를 만들지 않는다.
    for (std::uint32_t index=1;index<core.pool.Capacity();++index) { const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(index)});if (!(raw[11]&3U) && raw[10]==158 && raw[34]==1) { caster=Sid{static_cast<std::uint16_t>(index)};break; } }
    CHECK(caster.value);if (!caster.value) return;
    const auto first=core.processes.AddSharedRegular(caster,0x493,0.1F)->Form(),second=core.processes.AddSharedRegular(caster,0x493,0.1F)->Form();const auto other=core.processes.AddRegular(caster,0x492,0.1F)->Form();
    const auto bomb=session.World().Create(168,2);CHECK(Get(core.pool.Slot(bomb),0)==0x502370U);core.owner.Set(bomb,1);Put(core.pool.AllocatedBytes(bomb),12,caster.value,2);session.World().Pop(bomb,200,200,0);
    std::vector<std::vector<std::uint8_t>> received;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(received));session.RegisterCarrierCommands(dispatch);PacketConnection connection;connection.sender=1;
    CHECK(dispatch.Registration(0x5c).length==6 && dispatch.Registration(0x5c).version==1 && dispatch.Registration(0x5f).length==0xffff && dispatch.Registration(0x5f).version==1);
    std::array<std::uint8_t,6> activation{1,0x5c,6,0,0,0};Put(activation,4,bomb.value,2);const auto before=session.Sp(1);CHECK(dispatch.Dispatch(connection,activation));
    auto* process=core.processes.FindEvent(bomb,0);CHECK(process && process->Count()==0 && process->Payload()==0.1F);if (!process) return;const auto form=process->Form();CHECK(core.pool.Slot(form)[10]==61 && form.value<core.pool.Layout().serverFirst);
    std::array<std::uint8_t,10> effect{1,0x5f,10,0,0,0,0,0,0,0};Put(effect,4,bomb.value,2);const auto notices=external.notices;CHECK(dispatch.Dispatch(connection,effect));
    CHECK(!core.processes.FindEvent(caster,0x493) && !core.processes.FromForm(first) && !core.processes.FromForm(second));CHECK(core.processes.FromForm(other)!=nullptr && !core.processes.FromForm(form));CHECK((core.pool.Slot(bomb)[11]&3U)==3U && session.Sp(1)==before && external.notices>notices);
    const auto after=external.notices;CHECK(dispatch.Dispatch(connection,effect));CHECK(external.notices==after); // 이미 죽은 주문의 재수신은 원본 조기 반환이다.
    // 두 번째 주문은 효과 패킷을 주입하지 않고 실제 SharedRegular의 시계·프레임·검색·로컬 수신으로 종료시킨다.
    const auto timed=session.World().Create(168,2);core.owner.Set(timed,1);Put(core.pool.AllocatedBytes(timed),12,caster.value,2);session.World().Pop(timed,200,200,0);Put(activation,4,timed.value,2);CHECK(dispatch.Dispatch(connection,activation));const auto timedForm=core.processes.FindEvent(timed,0)->Form();std::size_t frames{};
    // 현재 예약 시각으로만 시계를 전진시키며 casttime/count/HP/대상 목록은 검사에서 쓰지 않는다.
    while (!(core.pool.Slot(timed)[11]&3U) && frames<4096) {
        auto* current=dynamic_cast<RegularProcess*>(core.processes.FromForm(timedForm));CHECK(current);if (!current) break;core.processState.now=current->Time();current->RunFrame();++frames;
    }
    CHECK(frames>1 && frames<4096 && (core.pool.Slot(timed)[11]&3U)==3U && !core.processes.FromForm(timedForm));CHECK(session.Sp(1)==before);
    CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
}

// 사용자 TEST02의 저장 데빌메이커를 실제 Kernel로 생산·비행·주기 수명까지 진행한다.
TEST_CASE(CampaignWindFlyer_ActualTest02NaturalProductionAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto services=external.Services();services.formPacket=[&](std::span<const std::uint8_t> packet) { external.packets.emplace_back(packet.begin(),packet.end()); }; // 실제Form송신요청만기록하고상태쓰기를대체하지않는다.
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
    MissionScript script(config,"TEST02");CHECK(script.Loaded());
    auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.Load();auto core=session.Core();
    Sid home{},body{};std::uint32_t born{},flying{},dead{};std::set<std::uint32_t> animation; // 현재 실제 객체와 수명만 관찰한다.
    for (const auto& object:session.World().Objects()) if (object.object.type==139 && !home.value) home=object.sid; // 사용자 원문에서 읽은 첫 데빌메이커를 쓴다.
    CHECK(home.value);if (!home.value) return;
    const auto form=FindAttachedProcess(core.pool,home,59);auto* maker=dynamic_cast<WindAviaryProcess*>(core.processes.FromForm(form));
    CHECK(maker && maker->Parent()==home && Get(maker->Fields().bytes,24)==1);if (!maker) return;
    CHECK(std::any_of(external.packets.begin(),external.packets.end(),[](const auto& packet) { return packet.size()>8 && packet[0]==1 && packet[1]==0x74; })); // 실제 Host 송신도 같은 전용 writer를 거친다.
    GameClock clock;
    for (std::uint32_t frame=1;frame<=1120;++frame) { // 현재 시계만 진행하며 생산 단계/표적/HP/결과는 쓰지 않는다.
        session.RunFrame(clock.Capture(frame*25),false);
        if (!body.value) {
            const Sid temporary{static_cast<std::uint16_t>(Get(maker->Fields().bytes,48))}; // 최초Pop은+12를압축피해시각으로바꾸므로생산기의실제임시SID를읽는다.
            if (temporary.value) { const auto raw=core.pool.Slot(temporary);CHECK(raw[10]==138 && !(raw[11]&7U));body=temporary;born=frame; }
        }
        if (!body.value) continue;
        const auto raw=core.pool.Slot(body);if (raw[11]&1U) { dead=frame;break; }
        CHECK(Get(raw,0)==0x50a050U && raw[34]==core.pool.Slot(home)[34]);animation.insert(Get(raw,36));
        const auto flyerForm=FindAttachedProcess(core.pool,body,31);
        if (flyerForm.value) {
            const auto* process=dynamic_cast<const FlyerProcess*>(core.processes.FromForm(flyerForm));CHECK(process);if (!process) return;
            CHECK(Get(process->Fields().bytes,0)==0x509f98U && Get(process->Fields().bytes,48)==0x509f78U && process->Fields().Home()==home);
            if (!flying) flying=frame;
        }
    }
    CHECK(born>0 && flying>born && dead>flying && animation.size()>2);
    if (body.value) CHECK(!FindAttachedProcess(core.pool,body,31).value && !core.processes.FindEvent(body,0));
    CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
    std::cout<<"TEST02 실제 데빌메이커"<<home.value<<"/더스트 데빌"<<body.value<<"/표시"<<born<<"/비행"<<flying<<"/삭제"<<dead<<"/그림"<<animation.size()<<'\n';
}

// 실제 원본 지형에서 두 새 유닛의 UI 선택/40h/38/자연 도착과 부모 삭제를 확인한다.
TEST_CASE(CampaignWalkerPath_ActualSelectionAndNaturalMovement) {
    // 두 부모는 같은 경로38을 쓰되 서로 다른 애니메이션/포획 Halt를 실행한다.
    for (const auto type:{137U,156U}) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto services=external.Services();UseWorldTestLogin(services);// 실제 로컬40h 명령을 사용한다.
    std::vector<Sid> losses;services.unitLost=[&](Sid sid) { losses.push_back(sid); }; // 삭제가 요청한 실제 UI 통지를 관찰한다.
    services.production.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    services.production.effects.configInt=[](std::string_view) { return 0; };
    services.production.effects.modeAssertion=[] { throw std::logic_error("Walker 실제 생산모드 진단"); };
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    services.ai.defaultName=[] { return std::string("AI"); };
    ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}\\{local.1}.fort\"\n[END]\n");
    MissionScript script(config,"TEST02");CHECK(script.Loaded() && script.MissionType()=="Tutorial");
    auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},script.FortName(),std::move(services),{1024,768});
    auto& session=world.Session();auto core=session.Core();CHECK(session.LocalConnectionPhase()==14);
    const auto cursors=BindWorldTestInput(world);MissionFrameGateState gate;MissionStartState state;CampaignMissionStartEffects effects;
    effects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    effects.gameSeconds=[&] { return core.processState.now; };effects.refreshSp=[] {};effects.refreshCommandGump=[] {};effects.resetCamera=[] {};
    effects.randomGeysers=[] { throw std::logic_error("TEST02 moreGeysers0에서 무작위 간헐천 도달"); };
    RawCampaignMissionStartBindings start(session,script,gate,state,session.MissionStartSettings(),session.MissionAiSettings(),std::move(effects));
    start.Start();session.PumpLocalConnection();CHECK(gate.started==1 && session.Sp(1)==500000 && session.Sp(2)==0 && session.Sp(3)==0);
    Sid anchor{};
    // 저장된 Bulf의 실제 섬 좌표를 입력으로만 빌리고 지형/다리는 바꾸지 않는다.
    for (const auto& object:session.World().Objects()) if (object.object.type==135 && object.object.owner==1) anchor=object.sid;
    CHECK(anchor.value);if (!anchor.value) return;
    const auto anchorRaw=core.pool.Slot(anchor);const RawPathPoint source{std::bit_cast<float>(Get(anchorRaw,14)),std::bit_cast<float>(Get(anchorRaw,18))};
    const auto walker=session.World().Create(type,2);core.owner.Set(walker,1);session.World().Pop(walker,source.x,source.y,0);
    const auto raw=core.pool.Slot(walker);CHECK(Get(raw,0)==(type==137 ? 0x503d80U : 0x503f20U) && session.Priests().MovementProcessType(type)==38);
    world.SetMousePoint(world.ToScreen(source.x,source.y));world.Select(walker.value);CHECK(world.InputMode()==5 && world.HighlightSettings().selectedSid==walker.value);
    RawPathPoint destination{};bool found{};std::optional<ScreenPoint> target;
    // 같은 실제 본섬의 충분히 떨어진 빈 칸만 골라 원본 BFS가 경로를 결정하게 한다.
    for (int y=static_cast<int>(source.y)-7;y<=static_cast<int>(source.y)+7 && !found;++y) {
        // 점유와 실제 표면을 모두 읽으며 테스트에서 지형/다리를 추가하지 않는다.
        for (int x=static_cast<int>(source.x)-7;x<=static_cast<int>(source.x)+7 && !found;++x) {
            if (x<1 || y<1 || x>=255 || y>=255 || std::abs(x-source.x)+std::abs(y-source.y)<8) continue;
            const auto cell=static_cast<std::size_t>(y*256+x);
            if ((core.spots[cell]&6U) && !(core.spots[cell]&8U) && core.hash.Cell(0,x,y)) {
                // 서버는귀환장소를별도로조회하므로표면spot만빈칸으로판단하지않는다. 현재일반finder로주변자산도확인한다.
                RawSquidFinder empty(core.pool,core.hash,core.types);if (empty.Begin({x-1,y-1,x+1,y+1},1).value) continue;
                const auto probe=world.Point("cell:"+std::to_string(x)+','+std::to_string(y));if (!probe) continue;
                world.SetMousePoint(*probe);world.UpdateMovementCursor();
                if (!cursors->empty() && cursors->back()==18) { destination={static_cast<float>(x),static_cast<float>(y)};target=probe;found=true; }
            }
        }
    }
    CHECK(found);if (!found) return;
    // 표시경계점이아닌칸내부이며실제이동커서18인후보만클릭한다. 신전귀환/운반명령과구분한다.
    CHECK(target.has_value() && cursors->back()==18);if (!target) return;
    CHECK(world.MoveSelected(*target));session.PumpLocalConnection();const auto form=FindAttachedProcess(core.pool,walker,38);
    auto* path=dynamic_cast<RawPathProcess*>(core.processes.FromForm(form));CHECK(path && path->MovementType()==38);if (!path) return;
    CHECK(Get(path->Runtime().Bytes(),0)==0x50e320U);const auto end=path->Runtime().Time(path->Runtime().Count()-1);const auto initialSp=session.Sp(1);
    const auto endpoint=path->Runtime().Point(path->Runtime().Count()-1);std::cout<<"TEST02 요청"<<destination.x<<","<<destination.y<<"/실제경로끝"<<endpoint.x<<","<<endpoint.y<<"/커서"<<cursors->back()<<'\n';
    // 원본38 Init은 cardinal 경로를 선택하며 대각 방향을 만들어 넣지 않는다.
    for (std::int32_t index=0;index<path->Runtime().Count();++index) CHECK((path->Runtime().Direction(index)&1)==0);
    std::size_t frames{};std::set<std::uint32_t> animation;
    // AI와 같은 Kernel을 시간 순서대로 실행하며 도착/경로 제거 결과를 쓰지 않는다.
    for (;frames<6000 && core.processes.FromForm(form);++frames) {
        world.Step(.025);animation.insert(Get(core.pool.Slot(walker),36));
        if (core.processState.now>end+5) break;
    }
    CHECK(!core.processes.FromForm(form) && frames>1 && animation.size()>2);
    std::cout<<"TEST02 도착현재좌표"<<std::bit_cast<float>(Get(raw,14))<<","<<std::bit_cast<float>(Get(raw,18))<<'\n';
    CHECK(std::bit_cast<float>(Get(raw,14))==destination.x && std::bit_cast<float>(Get(raw,18))==destination.y && session.Sp(1)==initialSp);
    CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
    std::cout<<"실제 Walker TYPE"<<type<<"/SID"<<walker.value<<"/선택·40h·38·도착 "<<frames<<"프레임/그림"<<animation.size()<<"/SP"<<session.Sp(1)<<'\n';
    // 완료 후 다시 이동시킨 부모를 삭제하면 같은38 Form도 즉시 반납돼야 한다.
    CHECK(session.StartCarrierMovement({walker,source,1,0,0,0,0}));const auto moving=FindAttachedProcess(core.pool,walker,38);CHECK(moving.value);
    session.World().Destroy(walker,0);CHECK((core.pool.Slot(walker)[11]&1U) && !core.processes.FromForm(moving));
    CHECK(std::find(losses.begin(),losses.end(),walker)!=losses.end());
    CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
    }
}

// 두 실제149와 적 지상 건물의 Pop/Kernel/공간 검색/연쇄 빔/피해/삭제를 자연 진행한다.
TEST_CASE(CampaignArcSpire_ActualKernelAttackAndDeletion) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto services=external.Services();std::size_t zaps{},lost{};const auto sound=services.sound.playOnceAt;
    services.sound.playOnceAt=[&](float x,float y,SoundHandle token,std::uint32_t first,std::uint32_t second) {
        if (token==external.names.Lookup("shortZap.wav")) ++zaps;
        return sound(x,y,token,first,second);
    }; // 실제 빔 생성의 위치 음향 요청만 관찰한다.
    services.unitLost=[&](Sid) { ++lost; }; //실제 삭제가 요청하는 소실 음향 장치다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
    CHECK(session.Alliances()[10]!=0 && !session.Alliances()[11]);
    const auto source=session.World().Create(149,2),other=session.World().Create(149,2),victim=session.World().Create(78,2);
    const auto funds=session.Sp(1);core.owner.Set(source,1);core.owner.Set(other,1);core.owner.Set(victim,2);
    // 표면은 원본 Pop 후처리가 생성하며 지형/Spot/피해 결과를 검사에서 직접 쓰지 않는다.
    session.World().Pop(source,200,200,0);session.World().Pop(other,212,200,0);session.World().Pop(victim,206,200,0);
    CHECK(Get(core.pool.Slot(source),0)==0x50e668U && Get(core.pool.Slot(other),0)==0x50e668U);
    const auto* timer=core.processes.FindEvent(source,0);CHECK(timer && timer->Payload()==1.0F && timer->Count()==0);
    CHECK(core.postPop.HandlesAsset(source));const auto hp=Get(core.pool.Slot(victim),26);CHECK(hp>90);
    GameClock clock;
    for (std::uint32_t frame=1;frame<=84;++frame) session.RunFrame(clock.Capture(frame*25),false); //2.1초의 실제 Kernel 예약만 진행한다.
    const auto after=Get(core.pool.Slot(victim),26);CHECK(after<hp && after>0 && zaps>0 && session.Sp(1)==funds);
    CHECK(core.processes.FindEvent(source,0) && core.processes.FindEvent(source,0)->Count()>=2);
    // 현재 공통 삭제/파편 캡처와 종속 Regular 반납도 실제 객체에 적용한다.
    session.World().Destroy(source,0);CHECK((core.pool.Slot(source)[11]&1U) && !FindAttachedProcess(core.pool,source,46).value && lost>0);
    CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
    std::cout<<"아크 스파이어 실제 Kernel: HP"<<hp<<"→"<<after<<"/빔 소리"<<zaps<<"/부모 삭제·예약 반납/SP"<<funds<<'\n';
}

// 원본0x61 수신은 폴TYPE를 검사하지 않는다. 두78은 수신/표시 검사의 명시 참조이며145의자연포획검사와구분한다.
TEST_CASE(CampaignAcidZap_ActualPacketCapturePixelsAndNaturalExpiry) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);std::size_t cases{},totalPixels{},sounds{},lost{};
    for (const auto type:{133U,135U,137U,111U,141U,146U,156U,73U,144U,138U}) { //실제 운반·비행 유닛의 일반/전용 소유자 색표를 포함한다.
        for (const auto orientation:{0,1}) { //각 장면은 새 Session으로 격리하여 다른 홈의 후속 생산을 섞지 않는다.
            SessionServicesFixture external;auto services=external.Services();const auto once=services.sound.playOnceAt;
            services.sound.playOnceAt=[&](float x,float y,SoundHandle token,std::uint32_t first,std::uint32_t second) { if (token==external.names.Lookup("dissolve.wav")) ++sounds;return once(x,y,token,first,second); }; //실제 단발 위치 음향을 관찰한다.
            services.unitLost=[&](Sid) { ++lost; };services.formPacket=[&](std::span<const std::uint8_t> packet) { external.packets.emplace_back(packet.begin(),packet.end()); }; //생산 홈/비행체의 실제 Form 직렬화도 소비한다.
            auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
            std::vector<std::vector<std::uint8_t>> received;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(received));session.RegisterCarrierCommands(dispatch);const auto& registration=dispatch.Registration(0x61);CHECK(registration.version==1 && registration.length==11 && registration.mismatch==0 && !registration.observer);
            PacketConnection connection;connection.sender=8;const auto first=session.World().Create(78,2),second=session.World().Create(78,2);core.owner.Set(first,1);core.owner.Set(second,1);session.World().Pop(first,195,195,0);session.World().Pop(second,215,215,0);
            constexpr SoundView kView{2768,1871,0,0,1024,768}; //205,205를 중앙에 두는 같은16·11 카메라다.
            session.SetParticleView(kView,{});std::uint64_t frame{};const auto tick=[&] { ++frame;session.RunFrame({frame*.025,100+frame*.025,.025,frame},false); }; //현재 시각만 진행하며 생산 단계/피해 결과를 쓰지 않는다.
            Sid victim{};
            if (type==73 || type==144 || type==138) {
                const auto homeType=type==73 ? 72U : type==144 ? 143U : 139U;const auto home=session.World().Create(homeType,2);core.owner.Set(home,2);session.World().Pop(home,206,208,0);
                const auto makerForm=FindAttachedProcess(core.pool,home,59);auto* maker=type==138 ? dynamic_cast<WindAviaryProcess*>(core.processes.FromForm(makerForm)) : nullptr;
                if (type==138) CHECK(maker);
                Sid emerging{}; //더스트 데빌의첫몸체는31부착시임시48에서빠지므로후속표시번호로덮어쓰지않는다.
                for (std::uint32_t step=0;step<1600 && !victim.value;++step) { //실제 홈이 만들고31을 붙인 비행체만 수신 표적으로 사용한다.
                    tick();const auto value=type==138 && maker ? Get(maker->Fields().bytes,48) : Get(core.pool.Slot(home),12,2);
                    if (!emerging.value && value && value<core.pool.Capacity()) emerging=Sid{static_cast<std::uint16_t>(value)};
                    if (emerging.value) { const auto raw=core.pool.Slot(emerging);if (raw[10]==type && !(raw[11]&7) && !(raw[40]&8) && FindAttachedProcess(core.pool,emerging,31).value) victim=emerging; }
                }
                CHECK(victim.value);if (!victim.value) return;
            } else { victim=session.World().Create(type,2);core.owner.Set(victim,2);session.World().Pop(victim,205,205,0); }
            const auto money=session.Sp(1);const auto refund=session.HitPoints().Plan(victim,1,false).refund; //기존 원문 대조된 삭제 보상 계획과 실제 수신 지급을 비교한다.
            std::array<std::uint8_t,11> packet{1,0x61,11,0};Put(packet,4,victim.value,2);Put(packet,6,first.value,2);Put(packet,8,second.value,2);packet[10]=static_cast<std::uint8_t>(orientation);const auto priorSounds=sounds;
            CHECK(dispatch.Dispatch(connection,packet));CHECK((core.pool.Slot(victim)[11]&1) && sounds==priorSounds+1);CHECK(session.Sp(1)==money+refund);
            const auto forms=ConstructionLive(core,18);CHECK(forms.size()==1);if (forms.size()!=1) return;const auto form=forms.front();auto* effect=dynamic_cast<DissolveVisualProcess*>(core.processes.FromForm(form));CHECK(effect);if (!effect) return;
            const auto dummy=effect->Parent();const auto token=effect->Fields().common.registration.token;CHECK(core.dependencies.Has(form,dummy));const auto& image=effect->Fields().common.image;
            CHECK(image.width>1 && image.height>1 && image.pixels.size()==static_cast<std::size_t>(image.width*image.height));const auto opaque=std::count_if(image.pixels.begin(),image.pixels.end(),[](auto color) { return color!=255; });CHECK(opaque>0 && effect->Fields().opaqueCount==opaque);
            CHECK(dispatch.Dispatch(connection,packet) && sounds==priorSounds+1 && session.Sp(1)==money+refund); //같은free대상은효과/보상을반복하지않는다.
            tick();effect=dynamic_cast<DissolveVisualProcess*>(core.processes.FromForm(form));CHECK(effect);if (!effect) return;
            CHECK(effect->Fields().clockStarted && effect->Fields().common.delta==0);const auto bounds=RenderProcessRemovalBounds(effect->Fields().common.registration,static_cast<float>(kView.cameraX),static_cast<float>(kView.cameraY));
            auto sprites=session.ParticleSprites({0,0,1024,768});std::erase_if(sprites,[&](const RenderSprite& sprite) { if (!sprite.procedure) return true;const auto area=sprite.procedure->bounds;return area.left!=bounds[0] || area.top!=bounds[1] || area.right!=bounds[2] || area.bottom!=bounds[3]; });CHECK(sprites.size()==1);
            Renderer renderer(1024,768);renderer.SetScene(std::move(sprites));std::vector<std::uint8_t> pixels(1024*768,255);renderer.Draw(pixels,1024,255);const auto visible=std::count_if(pixels.begin(),pixels.end(),[](auto color) { return color!=255; });CHECK(visible>0);totalPixels+=static_cast<std::size_t>(visible);
            for (std::uint32_t step=0;step<240 && core.processes.FromForm(form);++step) tick(); //같은 실제 Kernel만으로 저장 수명을 소진한다.
            CHECK(!core.processes.FromForm(form) && !session.Particles().Rendering().Lookup(token) && (core.pool.Slot(dummy)[11]&3)==3);CHECK(session.Sp(1)==money+refund && !core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);++cases;
            std::cout<<"애시드 실제 수신 TYPE"<<type<<"/방향"<<orientation<<"/픽셀"<<visible<<"/삭제 보상"<<refund<<"/전체 frame"<<frame<<'\n';
        }
    }
    CHECK(cases==20 && totalPixels>0);std::cout<<"애시드 실제 수신/캡처/자연소진:"<<cases<<"건/분해 표면"<<totalPixels<<"픽셀/소리"<<sounds<<"/소실"<<lost<<'\n';
}

// 두 실제145의 첫0.3F 주기가 양축finder부터 패킷/분해/삭제와 예약 정리까지 실행해야 한다.
TEST_CASE(CampaignAcidFence_ActualKernelAutomaticCapture) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto orientation:{0,1}) { //같은행/열의두장면을각각새세계에서관찰한다.
        SessionServicesFixture external;auto services=external.Services();std::size_t sounds{},lost{};const auto once=services.sound.playOnceAt;
        services.sound.playOnceAt=[&](float x,float y,SoundHandle token,std::uint32_t first,std::uint32_t second) { if (token==external.names.Lookup("dissolve.wav")) ++sounds;return once(x,y,token,first,second); }; //실제세계음향요청이다.
        services.unitLost=[&](Sid) { ++lost; };
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
        const auto source=session.World().Create(145,2),other=session.World().Create(145,2),victim=session.World().Create(133,2),ally=session.World().Create(135,2);
        core.owner.Set(source,1);core.owner.Set(other,1);core.owner.Set(victim,2);core.owner.Set(ally,1);
        session.World().Pop(source,200,200,0);session.World().Pop(other,orientation ? 206.0F : 200.0F,orientation ? 200.0F : 206.0F,0);
        session.World().Pop(victim,orientation ? 203.0F : 199.0F,orientation ? 199.0F : 203.0F,0);session.World().Pop(ally,orientation ? 204.0F : 199.0F,orientation ? 199.0F : 204.0F,0);
        CHECK(Get(core.pool.Slot(source),0)==0x50e738U && Get(core.pool.Slot(other),0)==0x50e738U);const auto* initial=core.processes.FindEvent(source,0);CHECK(initial && initial->Payload()==.3F && initial->Count()==0);
        const auto funds=session.Sp(1);const auto refund=session.HitPoints().Plan(victim,1,false).refund;std::uint64_t frame{};const auto tick=[&] { ++frame;session.RunFrame({frame*.025,100+frame*.025,.025,frame},false); }; //원본의 현재 시각만 진행한다.
        for (std::uint32_t step=0;step<20 && !(core.pool.Slot(victim)[11]&1);++step) tick(); //검사에서 HP/결과/후보를 바꾸지 않는다.
        const auto captureFrame=frame; //수명소진까지진행한전체프레임과첫자동포획시각을구분한다.
        CHECK((core.pool.Slot(victim)[11]&1) && !(core.pool.Slot(ally)[11]&7) && sounds==1 && session.Sp(1)==funds+refund);
        const auto effects=ConstructionLive(core,18);CHECK(effects.size()==1);if (effects.size()!=1) return;const auto form=effects.front();const auto* dissolve=dynamic_cast<const DissolveVisualProcess*>(core.processes.FromForm(form));CHECK(dissolve && dissolve->Fields().opaqueCount>0);if (!dissolve) return;
        const auto dummy=dissolve->Parent();const auto token=dissolve->Fields().common.registration.token;
        CHECK(core.processes.FindEvent(source,0) && core.processes.FindEvent(source,1) && core.processes.FindEvent(other,1));
        session.World().Destroy(source,0);session.World().Destroy(other,0);CHECK((core.pool.Slot(source)[11]&1) && !FindAttachedProcess(core.pool,source,46).value && !FindAttachedProcess(core.pool,other,46).value && lost>0);
        for (std::uint32_t step=0;step<240 && core.processes.FromForm(form);++step) tick(); //같은 Kernel의 자연 수명으로 분해를 마친다.
        CHECK(!core.processes.FromForm(form) && !session.Particles().Rendering().Lookup(token) && (core.pool.Slot(dummy)[11]&3)==3);CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
        std::cout<<"애시드 실제 자동포획/방향"<<orientation<<"/포획frame"<<captureFrame<<"/전체frame"<<frame<<"/소리"<<sounds<<"/보상"<<refund<<'\n';
    }
}

// 실제받침확정경로가만든9표면/윗면/하단의기준점을읽으며검사에서직접조립하지않는다.
std::array<float,2> IceSupport(RawCampaignSession& session,Sid& island) {
    auto core=session.Core();const auto point=EmptySessionSupport(core);CHECK(session.ConfirmSupport(static_cast<float>(point.x),static_cast<float>(point.y)));const auto packet=session.Construction().LastPacket();CHECK(packet.size()==45 && packet[26]==9);if (packet.size()!=45) throw std::logic_error("아이스 받침 확정 패킷 부재");
    float x=-10000,y=-10000;
    for (std::size_t index=0;index<9;++index) { const Sid sid{static_cast<std::uint16_t>(Get(packet,27+index*2,2))};const auto raw=core.pool.Slot(sid);x=std::max(x,std::bit_cast<float>(Get(raw,14)));y=std::max(y,std::bit_cast<float>(Get(raw,18))); } //현재생성된오른쪽/아래기준점이다.
    session.World().SetSupportOwner(x,y,1);island=session.World().Find(x,y,94);CHECK(island.value);return {x,y};
}
// 실제 섬/147을 배치하고 파괴→150→원래147과 반복재성장을 같은 Kernel만으로 진행한다.
TEST_CASE(CampaignIceTower_ActualKernelRegrowthAndRepeat) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();std::size_t lost{};services.unitLost=[&](Sid) { ++lost; }; //외부소실통지만관찰한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
    Sid island{};const auto point=IceSupport(session,island);const auto x=point[0],y=point[1];const Sid surface{core.hash.Cell(0,static_cast<int>(x),static_cast<int>(y))};CHECK(surface.value && core.pool.Slot(surface)[10]==157);
    const auto source=session.World().Create(147,2);core.owner.Set(source,1);session.World().Pop(source,x,y,0);const auto raw=core.pool.Slot(source);const auto word=Get(raw,12,2);const auto depth=raw[35];const auto funds=session.Sp(1);
    CHECK(Get(raw,0)==0x50e940U && session.World().Destroy(source,0) && (core.pool.Slot(source)[11]&1));
    auto growing=ConstructionLive(core,150);CHECK(growing.size()==1);if (growing.size()!=1) return;const auto sprout=growing.front();const auto sproutRaw=core.pool.Slot(sprout);
    CHECK(Get(sproutRaw,0)==0x50e888U && !(sproutRaw[11]&7) && sproutRaw[34]==1 && sproutRaw[35]==depth && Get(sproutRaw,12,2)==word && Get(sproutRaw,36)==0);
    const auto total=core.frames[150].Codes().size();CHECK(total>3);const auto* initial=core.processes.FindEvent(sprout,0x3a6);CHECK(initial && initial->Payload()==40.0F/static_cast<float>(total) && initial->Count()==0);
    CHECK(core.hash.Cell(0,static_cast<int>(x),static_cast<int>(y))==surface.value && !(core.pool.Slot(island)[11]&7));const auto form=FindAttachedProcess(core.pool,sprout,46);CHECK(form.value);
    std::uint64_t frame{};std::set<std::uint32_t> animation;Sid rebuilt{};const auto tick=[&] { ++frame;session.RunFrame({frame*.025,100+frame*.025,.025,frame},false); }; //시각만진행하며프레임/생성단계/HP를바꾸지않는다.
    for (std::uint32_t step=0;step<2400 && !rebuilt.value;++step) { //전용주기가자연성장/자기삭제/최종147Pop을수행한다.
        tick();const auto current=core.pool.Slot(sprout);if (!(current[11]&3)) animation.insert(Get(current,36));
        if (current[11]&1) rebuilt=session.World().Find(x,y,147);
    }
    CHECK(rebuilt.value && animation.size()>2 && !core.processes.FromForm(form));if (!rebuilt.value) return;const auto complete=core.pool.Slot(rebuilt);
    CHECK(complete[34]==1 && !(complete[11]&7) && Get(complete,26)==static_cast<std::uint32_t>(session.HitPoints().MaxHitPoints(rebuilt)) && session.Sp(1)==funds);
    CHECK(core.hash.Cell(0,static_cast<int>(x),static_cast<int>(y))==surface.value && !(core.pool.Slot(island)[11]&7));const auto completeFrame=frame;
    CHECK(session.World().Destroy(rebuilt,0));growing=ConstructionLive(core,150);CHECK(growing.size()==1);if (growing.size()!=1) return;const auto again=growing.front();CHECK(core.pool.Slot(again)[34]==1 && Get(core.pool.Slot(again),36)==0 && core.processes.FindEvent(again,0x3a6));
    // 성장형의실제받침정리비트로삭제하면섬/하단/9표면과자기예약을함께반납한다.
    CHECK(session.World().Destroy(again,0x8000000U));CHECK((core.pool.Slot(again)[11]&1) && !FindAttachedProcess(core.pool,again,46).value && (core.pool.Slot(island)[11]&1) && !core.hash.Cell(0,static_cast<int>(x),static_cast<int>(y)) && ConstructionLive(core,150).empty());
    CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
    std::cout<<"아이스 실제 재성장: TYPE150 프레임"<<animation.size()<<"/완성frame"<<completeFrame<<"/완성시간"<<completeFrame*.025<<"/반복생성·받침정리/소실"<<lost<<'\n';
}

// 완성형의회수/받침낙하/로딩과성장형삭제가원본의별도관문을따라야한다.
TEST_CASE(CampaignIceTower_ActualDeletionGates) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto mode:{0,1,2,3,4}) { //회수/받침낙하/로딩과같은삭제몸체의142/79를각각새세계로분리한다.
        SessionServicesFixture external;auto services=external.Services();services.unitLost=[](Sid) {};
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
        Sid island{};const auto point=IceSupport(session,island);const auto x=point[0],y=point[1];const auto source=session.World().Create(mode==3 ? 142U : mode==4 ? 79U : 147U,2);core.owner.Set(source,1);session.World().Pop(source,x,y,0);
        if (mode==2) session.World().SetLoadingDepth(1);const auto flags=mode==0 ? 0x200000U : mode==1 ? 0x8000000U : 0U;
        CHECK(session.World().Destroy(source,flags) && (core.pool.Slot(source)[11]&1) && ConstructionLive(core,150).empty());if (mode==2) session.World().SetLoadingDepth(0);
        CHECK((core.hash.Cell(0,static_cast<int>(x),static_cast<int>(y))==0)==(mode!=2));CHECK(((core.pool.Slot(island)[11]&1)!=0)==(mode!=2));CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
        std::cout<<"아이스 실제 삭제관문"<<mode<<"/성장형없음/받침존재"<<((core.pool.Slot(island)[11]&1)==0)<<'\n';
    }
}

// 실제 가상98의 지상 아크 피해와6699의 공격자0 피해가 모두 불워크 HP에 도달한다.
TEST_CASE(CampaignBulwark_ActualGroundAndQueuedDamage) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[](Sid) {};
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
    const auto first=session.World().Create(149),second=session.World().Create(149),victim=session.World().Create(79);core.owner.Set(first,1);core.owner.Set(second,1);core.owner.Set(victim,2);
    session.World().Pop(first,200,200,0);session.World().Pop(second,212,200,0);session.World().Pop(victim,206,200,0);const auto hp=Get(core.pool.Slot(victim),26);CHECK(Get(core.pool.Slot(victim),0)==0x50ea08U);
    for (std::uint64_t frame=1;frame<=84;++frame) session.RunFrame({frame*.025,100+frame*.025,.025,frame},false); // 실제 검색/빔/가상98만으로 피해를 발생시킨다.
    const auto after=Get(core.pool.Slot(victim),26);CHECK(after<hp && after>151);
    auto* request=core.processes.AddRegular(victim,6699,0);CHECK(request);if (!request) return;request->SetInitialCount(65687); // owner1/피해151의 외부 예약 명령이며 HP 결과는 쓰지 않는다.
    session.RunFrame({2.125,102.125,.025,85},false);CHECK(Get(core.pool.Slot(victim),26)==after-151);
    std::cout<<"불워크 실제 지상 빔 HP"<<hp<<"→"<<after<<"/공격자0 예약→"<<Get(core.pool.Slot(victim),26)<<'\n';
}

// 같은 실제 더스트 데빌/방어자 배치를 두 장면에서 비교해 비행 공격 피해만 거부하는지 확인한다.
TEST_CASE(CampaignBulwark_ActualAirAttackImmunity) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const std::uint32_t type:{78U,79U,142U}) { // 같은 공격 입력의 대조와 윈드타워의 비행 공격 예외도 함께 확인한다.
        SessionServicesFixture external;auto services=external.Services();services.unitLost=[](Sid) {};auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
        const auto victim=session.World().Create(type),flyer=session.World().Create(138);core.owner.Set(victim,2);core.owner.Set(flyer,1);session.World().Pop(victim,200,200,0);session.World().Pop(flyer,199.5F,199.5F,1); // 실제 서버 객체/최초Pop의 공격 예약을 사용하며 생산/이동 완료를 주장하지 않는다.
        const auto hp=Get(core.pool.Slot(victim),26);CHECK(core.types[138].flags2&0x100000U);CHECK(victim.value>=core.pool.Layout().serverFirst && core.processes.FindEvent(flyer,0));
        for (std::uint64_t frame=1;frame<=40;++frame) session.RunFrame({frame*.025,100+frame*.025,.025,frame},false); // 실제 event0의 영역 검색/피해 처리만 진행한다.
        const auto after=Get(core.pool.Slot(victim),26);CHECK(type==79 ? after==hp : after<hp);std::cout<<"실제 비행체"<<flyer.value<<"/방어자TYPE"<<type<<"/HP"<<hp<<"→"<<after<<'\n';
    }
}

// 실제 우클릭→25바이트 건설 명령→서버 배치/완공의 방향이 네 면의 원본 방어와 일치한다.
TEST_CASE(CampaignWindTower_RightClickConstructionAndFourFaces) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const std::uint32_t direction:{0U,2U,4U,6U}) { // 방향별로 독립 시작 상태를 사용한다.
        SessionServicesFixture external;auto services=external.Services();services.unitLost=[](Sid) {};auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
        const auto ready=[](FortChunkSection& section) { for (auto& chunk:section.chunks) { /* 저장 순서와 공장 소유자는 유지한다. */ for (auto& object:chunk.objects) if (object.type==124) { if (!object.contents) object.contents=FortContentList{};FortContent part;part.type=142;part.listFlags=0x10;part.listFlagsStored=true;object.contents->items.push_back(part); } } }; // 실제 로더에 준비142 부품을 명시 공급한다.
        for (auto& section:fort.territories) ready(section);ready(fort.chaff); // 모든 저장 섹션을 원래 로드 경로로 보낸다.
        GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();auto core=session.Core();SessionCursorUi ui;auto& cursor=session.Cursor();cursor.SetEffects(ui.Effects());cursor.SnapToGrid()=1;session.SetSp(1,5000);session.AddTechnology(1,core.types[142].assetName);CursorEnergyInput(session);
        const auto windPoint=CommandPlacementPoint(session,1);const auto wind=session.World().Create(75,2);core.owner.Set(wind,1);session.World().Pop(wind,static_cast<float>(windPoint.x),static_cast<float>(windPoint.y),0); // 윈드타워의 실제 원소 공급을 제공하며 요구 조건을 우회하지 않는다.
        const Sid factory{static_cast<std::uint16_t>(core.bookkeeping.ownerFactories[1].Items().front())};const auto form=cursor.Contents().Create(factory,142,0,8,0x10,3);cursor.Take(form,0x41);const auto point=CursorPlacementPoint(session,ui);
        ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";Client client(std::move(options));client.Configuration().Set("cursorSnapToGrid","1");client.Configuration().Set("rotmode","0");UserInput input(client,world);
        for (std::uint32_t turn=0;turn<direction/2;++turn) input.Event({InputCode::kRightButton,ui.mouse.x,ui.mouse.y}); // 실제 UserInput이 현재 커서 회전을 선택한다.
        CHECK(session.CursorDropSettings().direction==direction);input.UpdateCursor({InputCode::kLeftButton,ui.mouse.x,ui.mouse.y});input.Event({InputCode::kLeftButton,ui.mouse.x,ui.mouse.y});CHECK(ui.sent.size()==1);if (ui.sent.size()!=1) return;
        const auto& sent=ui.sent.front();CHECK(sent[0]==1 && sent[1]==0x3e && sent[4]==142 && sent[15]==direction && Get(sent,5)==std::bit_cast<std::uint32_t>(static_cast<float>(point.x)) && Get(sent,9)==std::bit_cast<std::uint32_t>(static_cast<float>(point.y)));
        std::vector<std::vector<std::uint8_t>> received;RawPacketDispatch dispatch(OriginalEdition::Patch1078,CommandDevice(received));session.RegisterConstructionCommands(dispatch);PacketConnection connection;connection.sender=1;CHECK(dispatch.Dispatch(connection,ui.sent.front()));const auto packet=session.Construction().LastPacket();CHECK(packet.size()==29);if (packet.size()!=29) return;const Sid target{static_cast<std::uint16_t>(packet[27]|(std::uint16_t(packet[28])<<8))};
        const auto builders=ConstructionLive(core,134);CHECK(builders.size()==1);if (builders.size()!=1) return;FinishGeneralConstruction(session,builders.front(),target);const auto raw=core.pool.Slot(target);CHECK(raw[10]==142 && Get(raw,0)==0x50ead0U);const auto code=core.frames[142].Codes()[Get(raw,36)].side;CHECK(code=='L'+direction/2);
        const auto source=session.World().Create(133,2);core.owner.Set(source,2); // 실제 지상 객체 생성자와 현재 발자국을 사용한다.
        for (const std::uint32_t incoming:{0U,2U,4U,6U}) { // 네 면 각각을 실제 좌표/프레임 기반 방어 몸체에 전달한다.
            const float x=static_cast<float>(point.x)+(incoming==2 ? 8.F : incoming==6 ? -8.F : 0.F),y=static_cast<float>(point.y)+(incoming==4 ? 8.F : incoming==0 ? -8.F : 0.F);session.World().Pop(source,x,y,0);CHECK(RawWindTowerDefense(core.pool,core.types,core.frames,target,source)==(incoming==direction));session.World().Unpop(source,0);
        }
        session.World().Destroy(source,0);std::cout<<"윈드타워 우클릭방향"<<direction<<"/건설"<<target.value<<"/완공코드"<<static_cast<char>(code)<<"/4면 방어 통과\n";
    }
}

// 실제 생산의 최초/임시 표시/비행 시작 단계에서 Form 삭제와 홈 파괴를 각각 진행한다.
TEST_CASE(CampaignWindAviaryDestroy_ActualProductionAndFormLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const bool homeDeath:{false,true}) { // 직접 Form 삭제와 부모 삭제의 종속 경로를 구분한다.
        for (const std::uint32_t phase:{0U,1U,2U}) { // 생산 단계를 검사 코드에서 쓰지 않고 실제 시각으로 기다린다.
            SessionServicesFixture external;auto services=external.Services();std::vector<Sid> lost;services.unitLost=[&](Sid sid) { lost.push_back(sid); };services.formPacket=[&](std::span<const std::uint8_t> packet) { external.packets.emplace_back(packet.begin(),packet.end()); };
            auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
            const auto home=session.World().Create(139);core.owner.Set(home,1);session.World().Pop(home,206,208,1);const auto form=FindAttachedProcess(core.pool,home,59);auto* maker=dynamic_cast<WindAviaryProcess*>(core.processes.FromForm(form));CHECK(maker);if (!maker) return;const auto pid=core.kernel.Find(*maker);Sid body{};std::uint64_t frame{};
            if (!phase) { frame=1;session.RunFrame({.025,100.025,.025,frame},false);CHECK(Get(maker->Fields().bytes,48)==0); } // 원본 생성자가 남긴 미정+48은 최초 실제 Frame이 초기화한다.
            if (phase) {
                for (frame=1;frame<=320;++frame) { // 생산기의 처음 임시 SID를 보존하고 이후 필드0으로 덮어쓰지 않는다.
                    session.RunFrame({frame*.025,100+frame*.025,.025,frame},false);if (!body.value) body=Sid{static_cast<std::uint16_t>(Get(maker->Fields().bytes,48))};
                    if (body.value && (phase==1 || FindAttachedProcess(core.pool,body,31).value)) break;
                }
                CHECK(body.value && frame<=320);if (!body.value || frame>320) return;CHECK(core.pool.Slot(body)[10]==138 && !(core.pool.Slot(body)[11]&7U));
            }
            const Sid temporary{static_cast<std::uint16_t>(Get(maker->Fields().bytes,48))}; // 발사 직후 같은 Frame이 새로 만든 두 번째 임시 개체도 현재 원문에서 읽는다.
            if (temporary.value) CHECK(core.pool.Slot(temporary)[10]==138 && !FindAttachedProcess(core.pool,temporary,31).value);if (phase==1) CHECK(temporary==body);
            if (homeDeath) CHECK(session.World().Destroy(home,0x100000));else core.processes.Kill(*maker,0x200000); // 두 실제 삭제 경로가 같은 +0c에 도달한다.
            CHECK(core.pool.Slot(form)[11]&1U);CHECK(!core.processes.IsRegistered(pid,maker));CHECK(!FindAttachedProcess(core.pool,home,59).value);CHECK(((core.pool.Slot(home)[11]&1U)!=0)==homeDeath); // 홈 파편이 같은 PID를 즉시 재사용할 수 있다.
            if (body.value) { CHECK(((core.pool.Slot(body)[11]&1U)!=0)==(phase==1));CHECK((FindAttachedProcess(core.pool,body,31).value!=0)==(phase==2));if (phase==1) CHECK(!core.processes.FindEvent(body,0)); }
            if (temporary.value) CHECK(core.pool.Slot(temporary)[11]&1U); // 원본이 참조하는 현재 임시 개체만 Form과 함께 정리한다.
            CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
            std::cout<<"실제 Wind Form삭제/홈파괴"<<homeDeath<<"/단계"<<phase<<"/frame"<<frame<<"/첫비행체"<<body.value<<"/현재임시"<<temporary.value<<"/첫비행체삭제"<<(body.value ? bool(core.pool.Slot(body)[11]&1U) : false)<<"/통지"<<lost.size()<<'\n';
        }
    }
}

// 실제139의 여섯 생산 프레임/owner/추가·추상 플래그 캡처54건을 보호PE 전체 픽셀과 대조한다.
TEST_CASE(CampaignWindAviaryDestroy_Native54CapturePixels) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();services.unitLost=[](Sid) {};services.formPacket=[](std::span<const std::uint8_t>) {};
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto home=session.World().Create(139);core.owner.Set(home,1);session.World().Pop(home,206,208,0);session.RunFrame({.025,100.025,.025,1},false);
    auto raw=core.pool.AllocatedBytes(home);const std::vector<std::uint8_t> saved(raw.begin(),raw.end());auto& type=const_cast<RiftTypeRecord&>(core.types[139]);const auto hotX=type.hotspotX,hotY=type.hotspotY;type.hotspotX=-2;type.hotspotY=3;std::size_t rows{},pixels{};const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/wind-aviary-capture-x86.tsv").string();
    for (const auto& row:netstorm::test::rawscene::LoadFixture(path.c_str()).rows) { // 생성자/생산 검사를 대체하지 않는 명시 캡처 입력이다.
        CHECK(row.size()==7);if (row.size()!=7) continue;Put(raw,36,static_cast<std::uint32_t>(std::stoul(row[0])));raw[40]=static_cast<std::uint8_t>(std::stoul(row[1]));raw[34]=static_cast<std::uint8_t>(std::stoul(row[2]));Put(raw,12,0,2);
        auto* effect=const_cast<RawCampaignParticles&>(session.Particles()).AddShrapnel(home,ShrapnelKind::Explosion);CHECK(effect);if (!effect) return;const auto& image=effect->Fields().image;const auto metadata=std::to_string(image.width)+','+std::to_string(image.height)+','+std::to_string(image.hotspotX)+','+std::to_string(image.hotspotY);
        CHECK(metadata==row[3] && netstorm::test::rawscene::Hex(image.pixels)==row[4] && image.pixels.size()==std::stoul(row[5]) && row[6]=="1");pixels+=image.pixels.size();++rows;const auto dummy=effect->Parent();const auto token=effect->Fields().registration.token;core.processes.Kill(*effect,0);CHECK(!session.Particles().Rendering().Lookup(token) && (core.pool.Slot(dummy)[11]&1U));
    }
    type.hotspotX=hotX;type.hotspotY=hotY;std::copy(saved.begin(),saved.end(),raw.begin());CHECK(rows==54 && pixels==64152);std::cout<<"Wind 실제 파편 캡처"<<rows<<"행/전체"<<pixels<<"픽셀 통과\n";
}

// 원본이 공유하는128/129 가상 표도 타입과 함께 대조하며 미복원 파생을 일반화하지 않는다.
TEST_CASE(CampaignPlainWorkshopCapture_ExactTypeAndVtable) {
    std::array<std::uint8_t,50> raw{};
    for (const auto [type,table]:{std::pair{129U,0x50b9b8U},std::pair{136U,0x509470U},std::pair{140U,0x50b6e8U},std::pair{143U,0x5099e8U},std::pair{148U,0x50b7c0U}}) { // 다섯 실제 쌍과 각 필드의 단독 불일치를 확인한다.
        raw[10]=static_cast<std::uint8_t>(type);Put(raw,0,table);CHECK(IsPlainWorkshopCapture(raw));Put(raw,0,table+4);CHECK(!IsPlainWorkshopCapture(raw));Put(raw,0,table);raw[10]=128;CHECK(!IsPlainWorkshopCapture(raw));
    }
    CHECK(!IsPlainWorkshopCapture(std::span<const std::uint8_t>(raw).first(49)));
}

// 모든141클러스터×5상태×3소유자의 실제 캡처를 보호PE 2115행 전체 픽셀과 비교한다.
TEST_CASE(CampaignPlainWorkshopCapture_Native2115Pixels) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/plain-workshop-capture-x86.tsv").string();const auto fixture=netstorm::test::rawscene::LoadFixture(path.c_str());std::size_t rows{},pixels{};
    for (const std::uint32_t number:{129U,136U,140U,143U,148U}) { // 각 실제 자산은 독립 세션의 원래 생성자/저장 Pop으로 준비한다.
        SessionServicesFixture external;auto services=external.Services();services.unitLost=[](Sid) {};services.formPacket=[](std::span<const std::uint8_t>) {};
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();Sid island{};const auto point=IceSupport(session,island);const auto source=session.World().Create(number);core.owner.Set(source,1);session.World().Pop(source,point[0],point[1],9);session.RunFrame({.025,100.025,.025,1},false); // flags9는 실제 Fort 로더의 저장 객체 Pop이다.
        auto raw=core.pool.AllocatedBytes(source);const std::vector<std::uint8_t> saved(raw.begin(),raw.end());auto& type=const_cast<RiftTypeRecord&>(core.types[number]);const auto hotX=type.hotspotX,hotY=type.hotspotY;type.hotspotX=-2;type.hotspotY=3;std::size_t current{};
        for (const auto& row:fixture.rows) { // 독립 원문이 공급한 현재 그림/추가·추상 상태/owner만 명시 입력으로 쓴다.
            CHECK(row.size()==8);if (row.size()!=8 || std::stoul(row[0])!=number) continue;Put(raw,36,static_cast<std::uint32_t>(std::stoul(row[1])));raw[40]=static_cast<std::uint8_t>(std::stoul(row[2]));raw[34]=static_cast<std::uint8_t>(std::stoul(row[3]));Put(raw,12,0,2);CHECK(IsPlainWorkshopCapture(raw));
            auto* effect=const_cast<RawCampaignParticles&>(session.Particles()).AddShrapnel(source,ShrapnelKind::Explosion);CHECK(effect);if (!effect) return;const auto& image=effect->Fields().image;const auto metadata=std::to_string(image.width)+','+std::to_string(image.height)+','+std::to_string(image.hotspotX)+','+std::to_string(image.hotspotY);
            if (metadata!=row[4] || netstorm::test::rawscene::Hex(image.pixels)!=row[5] || image.pixels.size()!=std::stoul(row[6]) || row[7]!="1") { std::cout<<"워크샾 캡처 차이 TYPE"<<number<<"/그림"<<row[1]<<"/extra"<<row[2]<<"/owner"<<row[3]<<'\n';CHECK(false); }
            pixels+=image.pixels.size();++rows;++current;const auto dummy=effect->Parent();const auto token=effect->Fields().registration.token;core.processes.Kill(*effect,0);CHECK(!session.Particles().Rendering().Lookup(token) && (core.pool.Slot(dummy)[11]&1U));
        }
        type.hotspotX=hotX;type.hotspotY=hotY;std::copy(saved.begin(),saved.end(),raw.begin());std::cout<<"워크샾 실제 캡처 TYPE"<<number<<"/전체"<<current<<"행 통과\n";
    }
    CHECK(rows==2115);std::cout<<"다섯 워크샾 캡처"<<rows<<"행/전체"<<pixels<<"픽셀 통과\n";
}

// 실제6699→HP 절반 미만→연소14 캡처/입자→치명 피해/파편과 Form 정리를 끝까지 진행한다.
TEST_CASE(CampaignPlainWorkshopCapture_ActualDamageBurningAndDeletion) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const std::uint32_t number:{129U,136U,140U,143U,148U}) { // 정상 상태에서 각 자산의 전용 프로세스와 함께 실제 피해를 전달한다.
        SessionServicesFixture external;auto services=external.Services();std::size_t lost{};services.unitLost=[&](Sid) { ++lost; };services.formPacket=[](std::span<const std::uint8_t>) {};
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();Sid island{};const auto point=IceSupport(session,island);const auto target=session.World().Create(number);core.owner.Set(target,1);session.World().Pop(target,point[0],point[1],9);
        const auto maximum=Get(core.pool.Slot(target),26),damage=maximum/2+1;CHECK(maximum==static_cast<std::uint32_t>(session.HitPoints().MaxHitPoints(target)) && maximum>1 && damage<65536 && core.types[number].damageEffect);
        if (number==143) CHECK(FindAttachedProcess(core.pool,target,60).value);else if (number!=136) CHECK(FindAttachedProcess(core.pool,target,34).value); // 실제60 생산기/34 공격 프로세스가 생성된 상태에서 삭제 정리를 확인한다.
        auto* request=core.processes.AddRegular(target,6699,0);CHECK(request);if (!request) return;request->SetInitialCount((2U<<16)|damage);std::uint64_t frame=1;session.RunFrame({frame*.025,100+frame*.025,.025,frame},false);CHECK(Get(core.pool.Slot(target),26)==maximum-damage);
        const auto form=FindAttachedProcess(core.pool,target,14);auto* burning=dynamic_cast<BurningVisualProcess*>(core.processes.FromForm(form));CHECK(burning && burning->Fields().mode==1 && burning->Fields().victim==target);if (!burning) return;
        for (++frame;frame<=120 && burning->Fields().image.pixels.empty();++frame) session.RunFrame({frame*.025,100+frame*.025,.025,frame},false); // 실제 표시 프로세스가 현재 그림을 캡처할 때까지 시각만 진행한다.
        CHECK(!burning->Fields().image.pixels.empty());const auto token=burning->Fields().registration.token;const auto imagePixels=burning->Fields().image.pixels.size();request=core.processes.AddRegular(target,6699,0);CHECK(request);if (!request) return;request->SetInitialCount((2U<<16)|maximum);++frame;session.RunFrame({frame*.025,100+frame*.025,.025,frame},false);
        CHECK(core.pool.Slot(target)[11]&1U);CHECK(!FindAttachedProcess(core.pool,target,14).value && !FindAttachedProcess(core.pool,target,34).value && !FindAttachedProcess(core.pool,target,60).value);CHECK(!core.processes.FindEvent(target,6699) && !session.Particles().Rendering().Lookup(token));CHECK(lost>0 && session.Particles().Rendering().Size()>0);CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);
        std::cout<<"워크샾 실제 피해/연소/삭제 TYPE"<<number<<"/HP"<<maximum<<"→"<<maximum-damage<<"/연소픽셀"<<imagePixels<<"/파편표시"<<session.Particles().Rendering().Size()<<'\n';
    }
}

namespace {
constexpr std::array<std::uint32_t,25> kWorkshopDisplayTypes{71,72,78,111,133,136,76,129,143,145,146,147,156,75,137,139,140,141,142,77,79,131,135,148,149}; // 사용자 범위인 썬6/레인7/윈드6/썬더6이다.
// 실제 파일 팔레트를 원본 표시기의 RGBX 입력으로 변환한다.
std::array<ScreenColor,256> WorkshopDisplayPalette(const GameAssets& assets) {
    std::array<ScreenColor,256> palette{};for (std::size_t index=0;index<palette.size();++index) { const auto color=assets.Palette().Color(static_cast<std::uint8_t>(index));palette[index]={color.blue,color.green,color.red,0}; } // 원래256색 번호는 바꾸지 않는다.
    return palette;
}
// 검사 카메라 안쪽의 명시 시작 상태도 원래 받침 확정 패킷으로 생성한다.
std::array<float,2> WorkshopDisplaySupport(RawCampaignSession& session) {
    auto core=session.Core();CHECK(session.ConfirmSupport(200,200));const auto packet=session.Construction().LastPacket();CHECK(packet.size()==45);if (packet.size()!=45) throw std::logic_error("워크샾 표시 받침 패킷 부재");float x{},y{};
    for (std::size_t index=0;index<9;++index) { const auto sid=Sid{static_cast<std::uint16_t>(Get(packet,27+index*2,2))};x=std::max(x,std::bit_cast<float>(Get(core.pool.Slot(sid),14)));y=std::max(y,std::bit_cast<float>(Get(core.pool.Slot(sid),18))); } // 실제 생성된 오른쪽/아래 기준점을 읽는다.
    session.World().SetSupportOwner(x,y,1);return {x,y};
}
}
// 모든25종의 GameWorld가 동일 원본 표시기로 본체를 제출하고 일반19종은 상태/owner별 전체 픽셀이 맞는다.
TEST_CASE(CampaignWorkshopDisplay_All25ActualBodyAndColors) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto palette=WorkshopDisplayPalette(assets.game);std::array<std::uint32_t,256> logical{};
    for (std::size_t index=0;index<logical.size();++index) logical[index]=std::uint32_t(palette[index].red)|(std::uint32_t(palette[index].green)<<8)|(std::uint32_t(palette[index].blue)<<16); // 실제 파일 색으로 원본 색 치환표만 준비한다.
    const auto translated=BuildSpriteColorMaps(logical);std::size_t bodies{},plain{},comparisons{};
    for (const auto number:kWorkshopDisplayTypes) { // 독립 시작 상태에서 모든 사용자 대상 타입을 같은 실제 생성/공간/표시 경로로 검사한다.
        SessionServicesFixture external;auto services=external.Services();services.unitLost=[](Sid) {};services.formPacket=[](std::span<const std::uint8_t>) {};UseWorldTestLogin(services);
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.SetPalette(palette);const auto cursors=BindWorldTestInput(world);auto& session=world.Session();CHECK(session.LocalOwner()==1);auto core=session.Core();const auto spawn=WorkshopDisplaySupport(session);const auto actor=session.World().Create(number);core.owner.Set(actor,1);session.World().Pop(actor,spawn[0],spawn[1],9);session.RunFrame({.025,100.025,.025,1},false);
        const std::array<float,2> position{std::bit_cast<float>(Get(core.pool.Slot(actor),14)),std::bit_cast<float>(Get(core.pool.Slot(actor),18))}; // 최초 Frame의 실제 보행/회피가 바꾼 현재 위치를 읽는다.
        const auto before=world.ToScreen(position[0],position[1]);world.Scroll(before.x-512,before.y-384);const auto screen=world.ToScreen(position[0],position[1]);CHECK(screen.x==512 && screen.y==384);
        RawCampaignFactoryDisplay display(core,assets.game,[&](Sid sid,const auto& draw) { session.DrawTemple(sid,draw); });display.SetPalette(palette);CHECK(display.Handles(actor));auto raw=core.pool.AllocatedBytes(actor);const auto saved=std::vector<std::uint8_t>(raw.begin(),raw.end());const ScreenRect clip{0,28,1024,768};
        const auto drawBody=[&] { std::vector<std::uint8_t> result(1024*768,255);std::size_t count{};for (const auto& sprite:world.Sprites()) if (sprite.order.x==position[0] && sprite.order.y==static_cast<float>(static_cast<double>(position[1])+((core.types[number].flags2&0x10000U) ? 0.5 : 0.0)) && sprite.order.depth==std::bit_cast<std::int8_t>(raw[35]) && sprite.procedure && !sprite.shadow) { sprite.procedure->draw(result,1024,768,clip);++count; }if (count!=1) std::cout<<"워크샾 본체 수 TYPE"<<number<<"/수"<<count<<'\n';CHECK(count==1);return result; }; // 같은 좌표/깊이의 그림자 복제도 실제 수집기 표식으로 제외한다.
        const auto first=drawBody();CHECK(first.size()==1024*768);++bodies;
        raw[40]=0;world.Select(actor.value);CHECK(raw[40]&4U);const auto selected=drawBody();CHECK(std::any_of(selected.begin(),selected.end(),[](auto pixel) { return pixel!=255; }));world.Select(0);std::copy(saved.begin(),saved.end(),raw.begin()); // 실제 선택 서비스의 괄호/HP 및 선택 해제를 같은 원본 표시 경로로 확인한다.
        if (IsPlainAssetDraw(raw)) {
            ++plain;const auto frame=Get(raw,36);const auto x=screen.x-core.types[number].hotspotX,y=screen.y-core.types[number].hotspotY;core.bridgeState.ownerColors[1]=2;core.bridgeState.ownerColors[8]=7; // 실제 표시 전역의 명시 플레이어 색 입력이다.
            for (const std::uint32_t owner:{1U,8U}) { // 현재 원본 소유자 색 전역을 사용하는 두 플레이어다.
                raw[34]=static_cast<std::uint8_t>(owner);std::optional<ColorMap> ownerMap;
                if (number==111 || number==141 || number==137) ownerMap=BuildSpecialOwnerColorMaps(logical,number)[static_cast<std::size_t>(core.bridgeState.ownerColors[owner])];
                else if (number==133 || number==135 || number==146 || number==156) ownerMap=BuildStandardOwnerColorMaps(number,82)[static_cast<std::size_t>(core.bridgeState.ownerColors[owner])];
                for (const std::uint8_t state:{std::uint8_t{0},std::uint8_t{1},std::uint8_t{16}}) { // 일반/건설 중/추상 표시 입력만 바꾸고 물리·프레임·HP 결과는 쓰지 않는다.
                    raw[40]=state;std::vector<std::uint8_t> expected(1024*768,255);const ColorMap* colors=state==1 ? &translated.dead : state==16 ? &translated.bright : ownerMap ? &*ownerMap : nullptr;
                    if (state!=16 || owner==session.LocalOwner()) { if (colors) CHECK(DrawMappedShape(expected,1024,768,assets.game.Shapes(),number-70,frame,x,y-clip.top,{0,28,1023,767},*colors)==0);else CHECK(DrawShape(expected,1024,768,assets.game.Shapes(),number-70,frame,x,y-clip.top,{0,28,1023,767})==0); } // 양 끝 포함 VFX pane은 좌표에 pane 원점을 더하므로 절대 화면 기준점에서 먼저 뺀다.
                    const auto actual=drawBody();if (actual!=expected) { std::cout<<"실제 워크샾 표시 차이 TYPE"<<number<<"/owner"<<owner<<"/상태"<<unsigned(state)<<'\n';CHECK(false); }++comparisons;
                }
            }
        }
        std::copy(saved.begin(),saved.end(),raw.begin());CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth);std::cout<<"25종 실제 본체 표시 TYPE"<<number<<"/일반"<<IsPlainAssetDraw(raw)<<"/기준점"<<core.types[number].hotspotX<<','<<core.types[number].hotspotY<<'\n';
    }
    CHECK(bodies==25 && plain==19 && comparisons==114);std::cout<<"실제25종 본체/일반19종 상태·소유자"<<comparisons<<"장면 전체 픽셀 대조\n";
}


namespace {
// 원본 선택 프레임의 화면 기준점과 실제8비트 출력이다.
struct CampaignSelectionBarImage {
    std::vector<std::uint8_t> pixels; // 실제 원본 표시기가 쓴 전체 화면이다.
    int x{},y{},width{}; // Native 선택 헤더의 좌상단과 폭이다.
    std::uint32_t background{}; // 같은 파일 팔레트의 원본 기본0번 색이다.
};
// 같은 실제 슬롯의 선택 본체만 원본 공간 수집기에서 골라 출력한다.
CampaignSelectionBarImage DrawCampaignSelectionBars(RawCampaignSession& session,const GameAssets& assets,Sid actor) {
    auto core=session.Core();const auto raw=core.pool.Slot(actor);CHECK(raw[40]&4U);const auto type=raw[10];const auto frame=Get(raw,36);const auto& header=core.shapes[type].frames.at(frame);const auto palette=WorkshopDisplayPalette(assets);
    const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));const int cameraX=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Get(raw,22,2)))-512,cameraY=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Get(raw,24,2)))-384;
    std::array<std::uint32_t,256> logical{};for (std::size_t index=0;index<logical.size();++index) logical[index]=std::uint32_t(palette[index].red)|(std::uint32_t(palette[index].green)<<8)|(std::uint32_t(palette[index].blue)<<16); // 원래 팔레트 번호를 유지한다.
    CampaignSelectionBarImage result{std::vector<std::uint8_t>(1024*768,255),512-header.hotspotX,384-header.hotspotY,header.width,BuildPaletteColorTable(logical,assets.Edition()).basic[0]};
    RawCampaignFactoryDisplay display(core,assets,[&](Sid sid,const auto& draw) { session.DrawTemple(sid,draw); });display.SetPalette(palette);CHECK(display.Handles(actor));std::size_t bodies{};const ScreenRect clip{0,28,1024,768};
    for (const auto& sprite:display.Sprites(clip,cameraX,cameraY,session.LocalOwner())) if (!sprite.shadow && sprite.sourceSid==actor.value) { CHECK(sprite.order.x==x && sprite.order.y==static_cast<float>(static_cast<double>(y)+((core.types[type].flags2&0x10000U) ? .5 : 0.0)) && sprite.order.depth==std::bit_cast<std::int8_t>(raw[35]));CHECK(sprite.procedure);if (sprite.procedure) sprite.procedure->draw(result.pixels,1024,768,clip);++bodies; } // 보행 정렬/그림자와 물리 화면 기준점은 구별한다.
    if (bodies!=1) std::cout<<"선택 본체 TYPE"<<unsigned(type)<<"/frame"<<frame<<"/count"<<bodies<<"/state"<<unsigned(raw[11])<<"/extra"<<unsigned(raw[40])<<'\n';CHECK(bodies==1 && result.width>0);return result;
}
// 기존 독립 원문498220의 두 fill 행에서 실제 현재 값이 정한 길이와 경계색을 확인한다.
void CheckCampaignSelectionBar(const CampaignSelectionBarImage& image,int row,std::uint32_t color,int current,int maximum) {
    const auto amount=std::max(0,std::min(current,maximum));const auto length=amount*image.width/std::max(maximum,1);const auto top=image.y-row*3-4;
    for (int y=top;y<top+2;++y) for (int x=image.x-1;x<=image.x+image.width;++x) { CHECK(x>=0 && x<1024 && y>=28 && y<768);if (x>=0 && x<1024 && y>=28 && y<768) CHECK(image.pixels[static_cast<std::size_t>(y*1024+x)]==(x>=image.x && x<image.x+length ? color : image.background)); } // 채움 두 행과 좌우 배경을 모두 비교한다.
}
}

// 실제 Cast 명령의493 SharedRegular가 진행하는 동안 같은 내용물 casttime과 count로 막대를 그린다.
TEST_CASE(CampaignSelectionBars_ActualCastCommandAndClock) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);std::uint32_t requests{};services.localLogin->observePacket=[&](std::uint32_t,bool,std::uint32_t,std::span<const std::uint8_t> packet) { if (packet.size()>1 && packet[1]==0x50) ++requests; };
    services.carrierDropPopup=[](std::int32_t,std::int32_t,std::int32_t,std::uint32_t) {};services.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});const auto cursors=BindWorldTestInput(world);auto& session=world.Session();auto core=session.Core();CHECK(session.LocalConnectionPhase()==14);
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto input=core.pool.Slot(priest);const auto caster=session.World().Create(133,2);session.World().Pop(caster,std::bit_cast<float>(Get(input,14)),std::bit_cast<float>(Get(input,18)),0);core.owner.Set(caster,1);const auto prayer=session.CreateContained(caster,168,0,1,0,0);session.SetSp(1,10000);
    CHECK(session.CommandCarrierDrop(caster,0x0300011a,caster)==1);CHECK(SquidForms(core.pool).Parent(prayer)==caster);session.PumpLocalConnection();CHECK(requests==1);world.Select(caster.value);const auto maximum=static_cast<int>(assets.game.Types()[168-70].definition.Number("casttime").value_or(0))*10;CHECK(maximum>0);std::uint32_t before{};
    for (std::uint32_t phase=0;phase<2;++phase) { // 결과 필드를 쓰지 않고 실제 Kernel의493 호출 횟수를 진행한다.
        if (phase) for (std::uint32_t frame=1;frame<=20;++frame) session.RunFrame({frame*.025,100+frame*.025,.025,frame},false); // 첫0.5초의 자연 사건만 실행한다.
        const auto* cast=core.processes.FindEvent(caster,0x493);CHECK(cast);if (!cast) return;const auto count=cast->Count();if (!phase) before=count;else CHECK(count>before && count<static_cast<std::uint32_t>(maximum));const auto image=DrawCampaignSelectionBars(session,assets.game,caster);CheckCampaignSelectionBar(image,1,0xbe,maximum-static_cast<int>(count),maximum);
        std::cout<<"실제 Cast 선택 막대 count"<<count<<"/최대"<<maximum<<"/내용물"<<prayer.value<<'\n';
    }
    CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());
}

// 생산·발사로 생긴 세 비행체의 실제31 필드와 선택·현재 시각을 연결한다.
TEST_CASE(CampaignSelectionBars_ActualThreeFlyerProductionTimers) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto [homeType,flyerType,index]:{std::array<std::uint32_t,3>{139,138,0},std::array<std::uint32_t,3>{143,144,1},std::array<std::uint32_t,3>{72,73,2}}) { // 세 원본 전용 생산기로 실제 부모/Form을 만든다.
        SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);services.unitLost=[](Sid) {};services.formPacket=[](std::span<const std::uint8_t>) {};auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});const auto cursors=BindWorldTestInput(world);auto& session=world.Session();auto core=session.Core();const auto home=session.World().Create(homeType);core.owner.Set(home,1);session.World().Pop(home,206,208,1);Sid actor{};std::uint64_t frame{};
        for (frame=1;frame<=2000 && !actor.value;++frame) { // 준비·임시 표시·발사 단계를 입력하지 않고 자연 기한으로 기다린다.
            session.RunFrame({frame*.025,100+frame*.025,.025,frame},false);
            for (const auto sid:ConstructionLive(core,flyerType)) if (!(core.pool.Slot(sid)[40]&8U)) { const auto form=FindAttachedProcess(core.pool,sid,31);const auto* process=dynamic_cast<const FlyerProcess*>(core.processes.FromForm(form));if (process && Get(process->Fields().bytes,84)) { actor=sid;break; } } // 그림자나 임시 개체를 비행 프로세스로 세지 않는다.
        }
        CHECK(actor.value);if (!actor.value) return;--frame;world.Select(actor.value);double before{};
        for (std::uint32_t phase=0;phase<2;++phase) { // 실제 게임1초 후에도 현재 동일 Form의 기한에서 남은 시간을 다시 계산한다.
            if (phase) for (std::uint32_t step=0;step<40;++step) { ++frame;session.RunFrame({frame*.025,100+frame*.025,.025,frame},false); }
            const auto form=FindAttachedProcess(core.pool,actor,31);const auto* process=dynamic_cast<const FlyerProcess*>(core.processes.FromForm(form));CHECK(process);if (!process) return;const auto& bytes=process->Fields().bytes;
            constexpr std::array<std::size_t,3> offsets{72,104,40}; // 독립 원문498220이 읽는 세 outer 기한이다.
            const auto end=std::bit_cast<double>(std::uint64_t(Get(bytes,offsets[index]))|(std::uint64_t(Get(bytes,offsets[index]+4))<<32));const auto left=end-core.processState.now;if (!phase) before=left;else CHECK(left<before);const auto image=DrawCampaignSelectionBars(session,assets.game,actor);const auto current=core.pool.Slot(actor);const auto hpRow=((core.types[flyerType].flags2&0x400000U) ? (current[40]&1U) : (core.types[flyerType].flags1&0x10U)) ? 1 : 0;CheckCampaignSelectionBar(image,hpRow,index==0 ? 0xd9U : index==1 ? 9U : 0x6bU,static_cast<int>(left),index==0 ? 20 : 60);
            std::cout<<"실제 비행 선택 TYPE"<<flyerType<<"/HP막대"<<hpRow<<"/frame"<<frame<<"/기한"<<end<<"/현재"<<core.processState.now<<"/남은"<<left<<'\n';
        }
    }
}

// 명시 효과 적용 직후 상태에서 실제 Kernel57의 중복 목록·중첩 기한과 만료를 검사한다.
TEST_CASE(CampaignSelectionBars_ActualStatusKernelAndExpiry) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});const auto cursors=BindWorldTestInput(world);auto& session=world.Session();auto core=session.Core();const auto position=WorkshopDisplaySupport(session);const auto actor=session.World().Create(78);core.owner.Set(actor,1);session.World().Pop(actor,position[0],position[1],9);session.RunFrame({.025,100.025,.025,1},false);auto raw=core.pool.AllocatedBytes(actor);raw[40]=0;world.Select(actor.value);raw[40]|=0x30; // 원본117/118 효과 적용 직후의 명시 입력이며 효과 수신/자연 생산 완료를 주장하지 않는다.
    BombEffectTimerHooks hooks;hooks.unpop=[&](Sid sid,std::uint32_t flags) { session.World().Unpop(sid,flags); };hooks.repop=[&](Sid sid,std::uint32_t flags) { session.World().Repop(sid,flags); };hooks.refresh=[&](Sid sid) { core.display.Update(core.pool.Slot(sid)); };hooks.fence=[](Sid) { throw std::logic_error("이 실제 상태 장면의 예상 밖 썬 바리케이드"); };
    const std::array<std::uint32_t,2> twice{actor.value,actor.value};const std::array<std::uint32_t,1> once{actor.value};std::array<const BombEffectTimerProcess*,3> timers{};std::array<ProcessId,3> pids{};std::size_t slot{};
    for (const auto [delay,flags]:{std::pair{6,1U},std::pair{9,3U},std::pair{8,2U}}) { // flags3의 우선순위와 각 효과의 최대 기한은 실제 현재 목록에서 얻는다.
        auto process=std::make_unique<BombEffectTimerProcess>(core.pool,core.types,core.processState.now,delay,flags==1 ? std::span<const std::uint32_t>(twice) : std::span<const std::uint32_t>(once),flags,hooks);timers[slot]=process.get();CHECK(core.processes.Attach(std::move(process),57,actor,0x50)==timers[slot]);pids[slot]=core.kernel.Find(*timers[slot]);CHECK(pids[slot]);++slot;
    }
    const auto first=DrawCampaignSelectionBars(session,assets.game,actor);const auto abstractMaximum=static_cast<int>(assets.game.Types()[117-70].definition.Number("effecttime").value_or(0)),paralyzeMaximum=static_cast<int>(assets.game.Types()[118-70].definition.Number("effecttime").value_or(0));CHECK(abstractMaximum>=9 && paralyzeMaximum>=9);CheckCampaignSelectionBar(first,1,0xc3,9,abstractMaximum);CheckCampaignSelectionBar(first,2,0xfe,8,paralyzeMaximum);
    for (std::uint32_t frame=2;frame<=401;++frame) { // 같은 Kernel에서0.025초씩 진행하고 실제 자기 삭제/공간 갱신을 실행한다.
        session.RunFrame({frame*.025,100+frame*.025,.025,frame},false);
        if (frame==41) { const auto image=DrawCampaignSelectionBars(session,assets.game,actor);CheckCampaignSelectionBar(image,1,0xc3,8,abstractMaximum);CheckCampaignSelectionBar(image,2,0xfe,7,paralyzeMaximum); }
        if (frame==251) { CHECK(!(raw[40]&0x10U) && (raw[40]&0x20U));CHECK(!core.processes.IsRegistered(pids[0],timers[0]) && core.processes.IsRegistered(pids[1],timers[1]) && core.processes.IsRegistered(pids[2],timers[2])); } // 짧은 은신 효과가 먼저 끝나도 더 긴 flags3이 비트를 재적용하지 않는다.
        if (frame==331) { CHECK(!(raw[40]&0x30U));CHECK(core.processes.IsRegistered(pids[1],timers[1]) && !core.processes.IsRegistered(pids[2],timers[2])); } // 마비 효과도 긴 flags3과 독립적으로 끝난다.
    }
    for (std::size_t index=0;index<timers.size();++index) CHECK(!core.processes.IsRegistered(pids[index],timers[index])); // 재사용된 번호를 다른 객체의 생존으로 오인하지 않는다.
    CHECK(!(raw[40]&0x30U) && (raw[40]&4U) && !(raw[11]&7U));static_cast<void>(DrawCampaignSelectionBars(session,assets.game,actor));CHECK(!core.bookkeeping.depth && !core.destroy.PreDepth() && !core.destroy.PostDepth());std::cout<<"실제 상태57 세 목록/중복·최대 기한/중첩 만료 및 표시 통과\n";
}

// 저장 중 기도 상태의 입력을 복원하며 아직 미연결인605 실행 완료로 확대하지 않는다.
TEST_CASE(CampaignSelectionBars_PriestPrayerSnapshot) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});const auto cursors=BindWorldTestInput(world);auto& session=world.Session();auto core=session.Core();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;std::uint32_t kind{};int maximum{};
    for (const auto& type:assets.game.Types()) if (const auto time=type.definition.Number("praytime");time && *time>0) { kind=70+static_cast<std::uint32_t>(type.block);maximum=static_cast<int>(*time);break; } // 실제 TYPE의 첫 유효 praytime을 명시 기도 입력으로 고른다.
    CHECK(kind && maximum>0);auto raw=core.pool.AllocatedBytes(priest);const auto saved=std::vector<std::uint8_t>(raw.begin(),raw.end());Put(raw,42,kind);Put(raw,46,static_cast<std::uint32_t>(maximum/2));auto* prayer=core.processes.AddRegular(priest,0x25d,1);CHECK(prayer);if (!prayer) return;prayer->SetNextTime(core.processState.now+1000000);world.Select(priest.value);const auto image=DrawCampaignSelectionBars(session,assets.game,priest);CheckCampaignSelectionBar(image,1,0x7b,maximum-maximum/2,maximum);world.Select(0);core.processes.Kill(*prayer,0);std::copy(saved.begin(),saved.end(),raw.begin());std::cout<<"실제 사제 선택/605 기도 상태 표시 TYPE"<<kind<<"/praytime"<<maximum<<'\n';
}
