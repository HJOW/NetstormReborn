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

#include "o/RawMissionAiSettings.h" // 같은 실제 생성자로 AI 수신자의 지갑/참여를 준비한다.
#include "client/SendMoneyInput.h" // 같은 실제 Session의 숫자 확인을 공격/포획 프레임 없이 선택한다.
// 현재 Client checking/진단을 실제 SP·AI 지갑과 공유하며 송신 가지는 선택하지 않는다.
TEST_CASE(MoneyConfirmChecking_ActualClientSessionAiWallet) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);BurningSessionDevices external;ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));auto services=external.Services();services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.processKernel=&client.GetKernel();services.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };std::uint32_t diagnostics{};services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };services.ai.participation.clearChat=[&](auto) { ++external.notices; };services.ai.participation.addChat=[&](auto,auto,auto) { ++external.notices; }; // 실제 생성자의 미정 메모리/외부 Chat 요청만 명시 입력으로 준다.
    services.ai.assertion=[&](std::string_view expression) { CHECK(expression=="inSingle");++diagnostics;client.GameplayGlobals()->AiDebug().Checking()=0; }; // 진단 복귀 뒤 실제 차감/지갑은 그대로 진행해야 한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));session.Load();client.GameplayGlobals()->player.playerKnown=1;client.GameplayGlobals()->player.battleOwner=1;std::array<std::uint8_t,RawMissionAiSettings::kSize> profile;profile.fill(0x71);MissionAiSettingsHooks settingsHooks;settingsHooks.get=[](std::string_view)->std::string { throw std::logic_error("송금 AI 기본값의 미선택 설정 조회"); };RawMissionAiSettings(OriginalEdition::Patch1078,session.MissionAiSettings(),std::move(settingsHooks)).Initialize(profile);const std::string name="Money AI";std::copy(name.begin(),name.end(),profile.begin());profile[name.size()]=0;const auto brain=session.CreateAi(2,profile);CHECK(brain);if (!brain) return;auto hooks=session.MoneyInputHooks();CHECK(hooks.flags(2)&0x20U);if (!(hooks.flags(2)&0x20U)) return;const auto local=hooks.local();CHECK(local>0 && local<9 && hooks.known());if (!local || local>=9 || !hooks.known()) return;const auto wallet=session.AiMoney(2);std::vector<std::pair<std::string,std::uint32_t>> locations;auto report=hooks.assertionAt;hooks.assertionAt=[&](auto expression,auto file,auto line) { locations.emplace_back(file,line);report(expression,file,line); }; // 실제 위치별 진단을 원래 현재 Client 서비스로 그대로 넘긴다.
    const auto packets=external.packets.size();client.GameplayGlobals()->world.missionPost=0;client.GameplayGlobals()->AiDebug().Checking()=0;session.SetSp(local,2000);
    CHECK(!hooks.checking() && ConfirmSendMoney(2,"700",hooks)==1);CHECK(!diagnostics && session.Sp(local)==1300 && session.AiMoney(2)==wallet+700);
    client.GameplayGlobals()->AiDebug().Checking()=std::bit_cast<std::int32_t>(0x80000000U);CHECK(hooks.checking()==0x80000000U && ConfirmSendMoney(2,"700",hooks)==1);CHECK(diagnostics==1 && !hooks.checking() && session.Sp(local)==600 && session.AiMoney(2)==wallet+1400 && external.packets.size()==packets); // 실제 Kernel/AI/통신 프레임은 진행하지 않는다.
    CHECK(DispatchSendMoneyInput({0x02000120U,0,2},hooks));CHECK(diagnostics==1 && session.Sp(local)==300 && session.AiMoney(2)==wallet+1700 && external.packets.size()==packets); // 같은 현재 checking0을 고정300 명령에서도 읽는다.
    session.SetSp(local,5000);client.GameplayGlobals()->AiDebug().Checking()=std::bit_cast<std::int32_t>(0x80000000U);CHECK(DispatchSendMoneyInput({0x02000108U,0,2},hooks));CHECK(diagnostics==2 && !hooks.checking() && session.Sp(local)==4300 && session.AiMoney(2)==wallet+2400);
    client.GameplayGlobals()->AiDebug().Checking()=-1;CHECK(DispatchSendMoneyInput({0x020000ecU,0,2},hooks));CHECK(diagnostics==3 && !hooks.checking() && session.Sp(local)==3300 && session.AiMoney(2)==wallet+3400);
    client.GameplayGlobals()->AiDebug().Checking()=1;CHECK(DispatchSendMoneyInput({0x02000120U,0,2},hooks));CHECK(diagnostics==4 && !hooks.checking() && session.Sp(local)==3000 && session.AiMoney(2)==wallet+3700 && external.packets.size()==packets);
    const std::vector<std::pair<std::string,std::uint32_t>> expected{{".\\Ui.cpp",0x61c},{".\\Userinput.cpp",0x8e9},{".\\Userinput.cpp",0x8c2},{".\\Userinput.cpp",0x913}};CHECK(locations==expected); // 숫자 UI와 세 고정 금액의 실제 원문 파일/줄을 구별한다.

}

// 실제 Client/Session/World의 공통 Unpop은 같은 GumpForm 종속을 찾고 기존 입력 FIFO에 한 번 예약한다.
TEST_CASE(SquidContainedUnpop_ActualClientSessionWorldAndInputQueue) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    BurningSessionDevices external;ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));
    auto services=external.Services();services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.processKernel=&client.GetKernel();
    services.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); }; // Deck의 미초기화 바이트에만 기존 명시 검사 입력을 공급한다.
    services.assets.queueCommand=[&client](auto command,auto first,auto second) { client.Input().PushCommand(command,first,second); }; // 제품과 같은 실제 큐의 정규화/FIFO를 실행한다.
    std::uint32_t diagnostics{};services.ai.assertion=[&](std::string_view expression) { CHECK(expression=="!isContained()");++diagnostics; }; // 공통 부모의 진단은 정상 복귀하고 상태를 바꾸지 않는다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));session.Load();
    while (!client.Input().Empty()) static_cast<void>(client.Input().Pop(false)); // 초기 설치 사건과 이번 원문 예약을 분리한다.
    auto core=session.Core();const auto parent=core.factory.Create(74,2),source=core.factory.Create(74,2);auto raw=core.pool.AllocatedBytes(source);
    CHECK(Get(raw,0)==0x501dd0U && core.types[74].name=="banner" && core.types[74].flags1==0x28000003U && core.types[74].flags2==0x02000000U); // 보호 TYPE74 banner의 실제 플래그와 공통 자산 VT를 사용하며 운반/포획 경로를 선택하지 않는다.
    raw[11]=8;Put(raw,14,parent.value,2);Put(raw,16,0,2);Put(raw,4,0,2);Put(core.pool.AllocatedBytes(parent),6,source.value,2); // 비공간 contained 입력을 실제 같은 풀에 배치한다.
    const auto form=session.GumpForms().Create(2);session.GumpForms().Attach(form,source,0x50); // GumpForm의 실제 부착/Pop이 source 자신의 종속 머리를 만든다.
    core.unpop.Unpop(source,core.types[74]);
    CHECK(diagnostics==1 && raw[11]==4 && Get(core.pool.Slot(parent),6,2)==0 && Get(raw,6,2)==form.value);
    const auto event=client.Input().Pop(false);CHECK(event.code==0x02000039U && event.x==source.value && !event.y && client.Input().Empty()); // Player의31h 알림이나 즉시 UI 삭제로 바꾸지 않는다.
    session.GumpForms().Destroy(form,0x50);CHECK(Get(raw,6,2)==0); // 같은 실제 Form 삭제/혼합 종속 체인으로 검사 자원을 반납한다.
}

// 실제 Take 호출자는 void/부모 삭제 중에도 원문 Unpop 접두와 postTake의 현재 TYPE/진단을 이어 실행한다.
TEST_CASE(FactoryTakeChecking_ActualClientSessionCurrentTypeAndContainedCaller) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);BurningSessionDevices external;ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));
    auto services=external.Services();services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.processKernel=&client.GetKernel();
    services.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); }; // 기존 Session 장치 입력과 같은 미초기화 Deck 바이트다.
    services.assets.queueCommand=[&client](auto command,auto first,auto second) { client.Input().PushCommand(command,first,second); }; // 실제 UpdateContents FIFO를 실행한다.
    RawCampaignSession* current{};Sid source{};std::uint32_t diagnostics{};std::uint8_t order{};
    services.ai.assertion=[&](std::string_view expression) { // 첫 접두 진단에서 현재 TYPE를 바꾸고 후단 진단의 zOrder 선행 쓰기를 확인한다.
        CHECK(expression=="!isContained()");++diagnostics;auto core=current->Core();if (diagnostics==1) current->World().Storage()->types[74].zOrder=order;else CHECK(core.pool.Slot(source)[35]==order);
    };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));current=&session;session.Load();auto core=session.Core();const auto originalOrder=core.types[74].zOrder;
    for (const bool initialVoid:{false,true}) for (const bool dyingParent:{false,true}) { // 비공간 banner만 사용하며 포획/희생/공격/전투 프레임을 실행하지 않는다.
        while (!client.Input().Empty()) static_cast<void>(client.Input().Pop(false)); // 앞선 설치/반복 사건을 비운다.
        const auto parent=core.factory.Create(74,2);source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);CHECK(source.value>=core.pool.Layout().serverFirst && Get(raw,0)==0x501dd0U);
        raw[11]=static_cast<std::uint8_t>(8U|(initialVoid ? 4U : 0U));Put(raw,14,parent.value,2);Put(raw,16,0,2);Put(raw,4,0,2);Put(core.pool.AllocatedBytes(parent),6,source.value,2);if (dyingParent) core.pool.AllocatedBytes(parent)[11]|=2;
        const auto form=session.GumpForms().Create(2);session.GumpForms().Attach(form,source,0x50);diagnostics=0;order=static_cast<std::uint8_t>(70+initialVoid*2+dyingParent);const auto free=core.pool.FreeCount();
        CHECK(core.factory.Take(74,source)==source);const bool staysContained=initialVoid || dyingParent;
        CHECK(diagnostics==(staysContained ? 2U : 1U) && raw[11]==(staysContained ? 12U : 4U) && raw[35]==order && raw[10]==74 && core.pool.FreeCount()==free);
        CHECK(Get(core.pool.Slot(parent),6,2)==(staysContained ? source.value : 0U));
        if (initialVoid) CHECK(client.Input().Empty());else { const auto event=client.Input().Pop(false);CHECK(event.code==0x02000039U && event.x==source.value && !event.y && client.Input().Empty()); }
        session.GumpForms().Destroy(form,0x50);CHECK(!Get(raw,6,2)); // 실제 Form을 반납하고 다음 명시 사례를 준비한다.
    }
    session.World().Storage()->types[74].zOrder=originalOrder; // 검사에서만 바꾼 TYPE 입력을 복원한다.
}
