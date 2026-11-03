// 화면 없는 실제 Session의 같은 회복/검색/진단/생성·삭제 소비자를 확인한다.
#include "DudeSpawnSupport.h"
#include "CameraTestDevices.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawCampaignParticles.h"
#include "o/RawContainedLookup.h"
#include <unordered_set>
using namespace netstorm::client;
namespace { // 아래 장치는 기존 공통 Session 검사와 같은 외부 요청만 관측한다.
// 동일 초기 장치 입력으로 실제 Session을 로드하며 게임 프레임/공격/통신은 실행하지 않는다.
struct BurningSessionDevices {
    SoundList names{OriginalEdition::Patch1078}; // 실제 이름표의 opaque sound handle이다.
    std::unordered_set<SoundHandle> playing; // 장치 외부 입력의 현재 재생 상태다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 원본 부모의 명시 영어 번역 입력이다.
    std::vector<std::vector<std::uint8_t>> packets; // 실제 직렬화 송신 요청 전체바이트다.
    std::vector<std::array<std::uint32_t,3>> allianceRequests; // 로컬 로그인 연결을 선택하지 않은 검사의 실제 서버/owner/동맹mask 송신 요청이다.
    std::size_t notices{}; // 외부 UI/음향 요청의 관찰 개수다.
    std::size_t constructionConfigReads{},constructionRefreshes{}; // 명시 건설 요청만 선택해야 하는 실제 설정/표시 외부 경계다.
    // 같은 fixture가 세션보다 오래 살아야 하며 모든 실제 선택 경계를 명시한다.
    RawCampaignSessionServices Services() {
        names.Initialize();RawCampaignSessionServices s;s.unixSeconds=[] { return 946684800; };s.soundNames=&names;s.cameraCommit=netstorm::test::camera::CommitScreenless;
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
}
// 실제 Client의 현재 checking을 세션이 빌리고 진단 복귀 후 같은 Host의 첫 결과를 유지한다.
TEST_CASE(DamageableBurningChecking_ActualClientSessionRecoveryAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);BurningSessionDevices external;ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));auto services=external.Services();services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.processKernel=&client.GetKernel();services.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };RawCampaignSession* current{};Sid second{};std::uint32_t diagnostics{};std::uint32_t maximum{};const Sid temple{15002}; // 같은 Client 소유자를 모두 연결하고 Deck의 미초기화 바이트만 명시 입력으로 준다.
    services.ai.assertion=[&](std::string_view expression) { // 진단 복귀가 현재 소유자/HP/중복 Form을 바꾼다는 명시 외부 입력이다.
        CHECK(expression=="findProcess (mySid, burningSystemProcessType, 1 ) == 0");++diagnostics;client.GameplayGlobals()->AiDebug().Checking()=0;auto core=current->Core();Put(core.pool.AllocatedBytes(temple),26,maximum);current->World().Destroy(second,0);
    };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));current=&session;session.Load();auto core=session.Core();auto raw=core.pool.AllocatedBytes(temple);CHECK(raw[10]==124 && core.types[124].damageEffect!=0);maximum=Get(raw,26);Put(raw,26,maximum/4);
    auto& particles=const_cast<RawCampaignParticles&>(session.Particles());auto* a=particles.AddBurning(temple,1);auto* b=particles.AddBurning(temple,1);CHECK(a && b);if (!a || !b) return;const auto tokenA=a->Fields().registration.token,tokenB=b->Fields().registration.token;RawContainedLookup lookup(core.pool,[&](Sid sid,std::uint32_t flags) { session.World().Destroy(sid,flags); });const auto first=lookup.Find(temple,14,0);second=lookup.Find(temple,14,1);CHECK(first.value && second.value && first!=second);
    client.GameplayGlobals()->AiDebug().Checking()=std::bit_cast<std::int32_t>(0x80000000U);session.Priests().DamageableRefresh(temple);CHECK(diagnostics==1 && !client.GameplayGlobals()->AiDebug().Checking());CHECK(Get(raw,26)==maximum && lookup.Find(temple,14)==first && !lookup.Find(temple,14,1).value);CHECK(core.processes.FromForm(first) && !core.processes.FromForm(second)); // 같은 호출 안에서 새 HP를 읽었다면 첫 효과도 잘못 제거한다.
    session.Priests().DamageableRefresh(temple);CHECK(!lookup.Find(temple,14).value && !core.processes.FromForm(first));CHECK(!particles.Rendering().Lookup(tokenA) && !particles.Rendering().Lookup(tokenB));CHECK(!core.destroy.PreDepth() && !core.destroy.PostDepth() && !core.bookkeeping.depth); // 다음 갱신은 현재 회복 HP로 단 한 번 정리한다.
    Put(raw,26,maximum/4);session.Priests().DamageableRefresh(temple);const auto created=lookup.Find(temple,14);auto* effect=dynamic_cast<BurningVisualProcess*>(core.processes.FromForm(created));CHECK(effect && effect->Fields().mode==1 && effect->Fields().stage==0);Put(raw,26,maximum);session.Priests().DamageableRefresh(temple);CHECK(!core.processes.FromForm(created) && !lookup.Find(temple,14).value && diagnostics==1); // 실제 기본 생성자도 사용하되 캡처/Kernel/전투 프레임은 진행하지 않는다.
}

// 같은 Client/Session의 기본 마나 진단 연결이 실제 공통 postPop과 현재 공급원 배열에 도달한다.
TEST_CASE(ManaRegistration_ActualClientSessionDefaultDiagnosticAndProviderOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);BurningSessionDevices external;ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));auto services=external.Services();services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.processKernel=&client.GetKernel();services.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };RawCampaignSession* current{};std::uint32_t diagnostics{};
    services.ai.assertion=[&](std::string_view expression) { CHECK(expression=="!alreadyThere");++diagnostics;client.GameplayGlobals()->AiDebug().Checking()=0;auto core=current->Core();core.bookkeeping.Providers().SetCount(0);core.bookkeeping.productionDirty=0xffffffff; }; // 진단이 목록/현재 카운터를 바꿔도 저장한 중복 판정은 유지해야 한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));current=&session;session.Load();auto core=session.Core();Sid provider{};
    for (const auto value:core.bookkeeping.Providers().Items()) { const auto sid=Sid{static_cast<std::uint16_t>(value)};const auto raw=core.pool.Slot(sid);if (!(raw[40]&9U) && !(core.types[raw[10]].flags2&0x4200U)) { provider=sid;break; } } // 공통 마나 부모만 선택하고 별도 Factory 등록은 제외하는 명시 입력이다.
    CHECK(provider.value);if (!provider.value) return;const auto* address=core.bookkeeping.Providers().Storage().data();const auto type=core.pool.Slot(provider)[10];const auto global=core.bookkeeping.GlobalCounts()[type];core.bookkeeping.graphsEnabled=false;core.bookkeeping.depth=1;client.GameplayGlobals()->AiDebug().Checking()=std::bit_cast<std::int32_t>(0x80000000U);core.postPop.PostPopBase(provider,1);
    CHECK(diagnostics==1 && !client.GameplayGlobals()->AiDebug().Checking());CHECK(core.bookkeeping.Providers().Count()==0 && core.bookkeeping.Providers().Storage().data()==address && !core.bookkeeping.productionDirty && !core.bookkeeping.depth && core.bookkeeping.GlobalCounts()[type]==global+1); // 실제 일반 후처리의 통계까지 이어가며 게임/AI/공격 프레임은 실행하지 않는다.
}
