// 다이스 자격·진단 복귀·읽기 getter만 검사한다. 실제 의식/포획/희생/전투/성공 이동은 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawDaisRegular.h"
#include "o/RawDaisLifecycle.h"
#include "o/RawGameAuxState.h"
#include "o/SquidUnpop.h"
#include <memory>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 실제 Pool/Host/PathStart를 공유하되 행동 효과는 모두 명시 실패 장치로 연결한다.
struct DaisParentScene { // 합성 입력의 변경과 제품이 요청한 진단/조회만 보관한다.
    SidPool pool;SquidHash hash;std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 같은 실제 슬롯/해시/점유 지도를 사용한다.
    std::vector<RiftTypeRecord> types;std::vector<RiftTypeFrames> frames;std::vector<SquidDisplayShape> shapes; // 현재 타입/방향/핫스폿 입력이다.
    SquidPostPopState book;SquidRewardState rewardState;GameRandom random{0x12345678}; // 같은 장부/경제/난수의 원시 상태다.
    Kernel kernel{32};SquidProcessState clock;RawPathProcessState pathState{clock}; // 프레임을 진행하지 않는 실제 Kernel/Host/경로 상태다.
    CarrierPreDestroyState finderState;TempleRitualState templeState;RunePostPopState runeState; // 자격 조회가 공유하는 현재 전역이다.
    std::uint32_t localOwner{1},checking{1},dudeType{70},teleportType{134},manaType{105},priestType{158},geyserType{122},canonicalPriestType{158}; // 실제 참조형 상태와 현재 canonical 종류에 연결할 장면 소유 DWORD다.
    bool snap{},retry{},currentBoss{true};std::int32_t currentAlliance{}; // 독립 정책 값과 현재 권한/동맹 조회 입력이다.
    DaisRegularState state{templeState,runeState,clock,rewardState,localOwner}; // 다이스가 읽는 동일 상태 참조다.
    std::size_t effects{},pathCreates{},countReads{},allianceReads{};std::uint32_t countValue{1}; // 금지 행동과 허용된 실패 경로/가상 조회 횟수를 구분한다.
    std::vector<std::uint32_t> diagnostics;std::vector<std::array<std::uint32_t,2>> allianceArguments; // 실제 원문 진단/현재 동맹 인자 관측이다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> onAssertion;std::function<void()> onCount; // 정상 복귀 중 적용할 명시 외부 입력이다.
    std::unique_ptr<SquidPostPop> post;std::unique_ptr<SquidUnpop> unpop;std::unique_ptr<RawSquidDestroy> destroy;std::unique_ptr<SquidProcessHost> host; // 실제 공통 공간/프로세스 모듈이다.
    std::unique_ptr<SquidFrame> frame;std::unique_ptr<RawCarrierPreDestroy> carrier;std::unique_ptr<RawContainedLookup> lookup;std::unique_ptr<RawPathStart> path; // 읽기/실패 경로의 같은 실제 의존 객체다.
    std::unique_ptr<RawSquidNeighbors> footprint;std::unique_ptr<ScrambledSpStore> store;std::unique_ptr<SquidReward> reward; // 의식 생성자가 요구하는 동일 풀의 실제 보조 모듈이다.
    std::unique_ptr<RawPriestRitualActions> actions;std::unique_ptr<RawTempleRitual> ritual;std::unique_ptr<RawRunePostPop> rune;std::unique_ptr<RawDaisRegular> dais; // 검사에서 허용한 조회/명령 실패 부모만 호출한다.

    [[noreturn]] void Forbidden() { ++effects;throw std::logic_error("다이스 checking 검사에서 실제 행동 효과 금지"); } // 호출된 행동을 빈 성공으로 대체하지 않는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 진단 뒤 현재 슬롯 입력을 같은 물리에 구성한다.
    bool Patch() const { return pool.Edition()==OriginalEdition::Patch1078; } // 판본별 필드 폭을 선택하는 검사 도구다.
    void Owner(Sid sid,std::uint32_t owner) { Raw(sid)[Patch() ? 34 : 32]=static_cast<std::uint8_t>(owner); } // owner BYTE만 변경한다.
    void FrameInput(Sid sid,std::uint32_t value) { Put(Raw(sid),Patch() ? 36U : 34U,value,Patch() ? 4U : 1U); } // raw 입력이며 실제 Frame setter를 호출하지 않는다.
    void Object(Sid sid,std::uint8_t type,float x,float y) { auto raw=Raw(sid);raw[10]=type;Put(raw,0,0x501dd0);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y)); } // 공간 등록 없이 객체 입력만 구성한다.
    std::unique_ptr<RawPathProcess> EmptyPath() { // 성공 경로나 검색/이동 효과 없이 초기 Count0인 실제 객체만 만든다.
        RawPathProcessHooks hooks;hooks.unpop=[this](Sid,std::uint32_t) { Forbidden(); };hooks.repop=hooks.unpop;hooks.pop=[this](Sid,RawPathPoint,std::uint32_t) { Forbidden(); };
        hooks.halt=[this](Sid) -> std::uint32_t { Forbidden(); };hooks.arrival=[this](Sid,std::uint32_t,std::uint32_t) -> std::uint32_t { Forbidden(); };hooks.soundId=hooks.halt;hooks.stepSound=[this](Sid,float,float,std::uint32_t) { Forbidden(); }; // 도착/정지/음향은 실행 금지다.
        hooks.search=[this](RawPathRuntime&,Sid,RawPathPoint,RawPathPoint,std::uint32_t) { Forbidden(); };hooks.clearBlocked=[this](Sid,RawPathPoint,std::uint32_t,std::uint32_t) { Forbidden(); };hooks.cleanup=[this](RawPathRuntime&,Sid,std::uint32_t) { Forbidden(); }; // 경로 검색/정리도 실행 금지다.
        return std::make_unique<RawPathProcess>(pool,hash,types,frames,shapes,spots,pathState,std::move(hooks),true);
    }
    explicit DaisParentScene(OriginalEdition edition=OriginalEdition::Patch1078):pool(edition,24000),types(Patch() ? 188U : 171U),frames(types.size(),RiftTypeFrames({})),shapes(types.size()) { // 같은 원문 의존 생성자만 연결하며 게임 루프를 열지 않는다.
        for (const auto type:{74U,158U,164U,165U,166U}) { // 자격·StagePoint 조회가 읽는 합성 코드/핫스폿만 준비한다.
            types[type].footX=types[type].footY=type==165 ? 3 : 1;types[type].maxHitPoints=200;std::vector<FrameCode> codes(180,FrameCode{'I','P',1,0});
            if (type==164) codes[1].side='M';frames[type]=RiftTypeFrames(std::move(codes));shapes[type].loaded=true;shapes[type].frameCount=180;shapes[type].frames.resize(180); // 기본 제단 방향I와 거부 방향M을 구분한다.
        }
        types[158].flags2=0x200000;
        for (std::uint16_t sid=5;sid<=12;++sid) { CHECK(pool.Allocate(2)==Sid{sid});auto raw=Raw(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{}); } // 사용 슬롯을 실제 할당해 Host 부착 입력과 공유한다.
        Object(Sid{5},165,40.75F,41.25F);Object(Sid{6},158,43.75F,44.25F);Object(Sid{7},164,40.75F,41.25F);Put(Raw(Sid{5}),0,Patch() ? kPatchDaisVtable : kCdDaisVtable);FrameInput(Sid{5},3);Owner(Sid{5},1);Owner(Sid{6},1);Owner(Sid{7},1);hash.Bucket(1,40.75F,41.25F)=7; // start 표면 지도0은 비워 PathStart를 실패 경로로 제한한다.
        Put(Raw(Sid{5}),6,10,2);Raw(Sid{10})[10]=6;Put(Raw(Sid{10}),18,158,2);Put(Raw(Sid{10}),20,0x20,2);Raw(Sid{10})[12]=2; // 포획 행동 없이 내용물 finder의 명시 자격 입력만 둔다.
        state.debug.Bind(checking);finderState.checkingDead.Bind(checking);finderState.sharedBoss=&currentBoss;clock.boss=false;
        const auto assertion=[this](std::string_view expression,std::string_view file,std::uint32_t line) { diagnostics.push_back(line);if (onAssertion) onAssertion(expression,file,line);else throw std::logic_error("검사에서 선택하지 않은 원문 진단"); };finderState.assertion=assertion;
        post=std::make_unique<SquidPostPop>(pool,types,book);unpop=std::make_unique<SquidUnpop>(pool,hash,spots);destroy=std::make_unique<RawSquidDestroy>(pool,*unpop,types);host=std::make_unique<SquidProcessHost>(pool,types,kernel,*destroy,clock);
        SquidFrameHooks frameHooks;frameHooks.frameSize=[this](Sid,std::int32_t) -> std::array<float,2> { Forbidden(); };frameHooks.update=[this](Sid,std::uint32_t) { Forbidden(); };frameHooks.unpop=frameHooks.update;frameHooks.pop=[this](Sid,float,float,std::uint32_t) { Forbidden(); };frame=std::make_unique<SquidFrame>(pool,types,std::move(frameHooks));
        CarrierPreDestroyHooks carrierHooks;carrierHooks.validateDamageable=[this](Sid,std::uint32_t) { Forbidden(); };carrierHooks.damageablePre=carrierHooks.validateDamageable;carrierHooks.dependentFound=[this] { Forbidden(); };carrier=std::make_unique<RawCarrierPreDestroy>(pool,finderState,std::move(carrierHooks));lookup=std::make_unique<RawContainedLookup>(pool,[this](Sid,std::uint32_t) { Forbidden(); });
        RawPathStartHooks start;start.processType=[](std::uint32_t) { return 54U; };start.create=[this](bool priest) { CHECK(priest);++pathCreates;return EmptyPath(); };start.metadata=[this](Sid) -> RawPathStartType { Forbidden(); };start.clearBlocked=[this](RawPathPoint,std::uint32_t,std::uint32_t) { Forbidden(); };path=std::make_unique<RawPathStart>(pool,hash,types,*host,*lookup,*frame,RawPathStartState{clock,dudeType,teleportType,manaType,priestType,geyserType,snap,retry},std::move(start));
        footprint=std::make_unique<RawSquidNeighbors>(pool,hash,spots,types,frames);store=std::make_unique<ScrambledSpStore>(random,[this]() -> std::uint32_t { Forbidden(); });reward=std::make_unique<SquidReward>(pool,types,book,rewardState,Patch() ? store.get() : nullptr);
        PriestRitualActionHooks action;action.setFrame=[this](Sid,std::int32_t,std::uint32_t) { Forbidden(); };action.reserveShared=[this]() -> bool { Forbidden(); };action.addShared=[this](Sid,std::uint32_t,float) { Forbidden(); };actions=std::make_unique<RawPriestRitualActions>(pool,frames,std::move(action));
        TempleRitualHooks temple;temple.create=[this](std::uint32_t,std::uint32_t) -> Sid { Forbidden(); };temple.setHitPoints=[this](Sid,std::int32_t) { Forbidden(); };temple.setOwner=[this](Sid,std::uint32_t) { Forbidden(); };temple.pop=[this](Sid,float,float,std::uint32_t) { Forbidden(); };temple.playAt=[this](float,float,std::string_view,std::uint32_t) { Forbidden(); };temple.reserveShared=[this]() -> bool { Forbidden(); };temple.addShared=[this](Sid,std::uint32_t,float) { Forbidden(); };temple.count=[this](Sid) { ++countReads;if (onCount) onCount();return countValue; };ritual=std::make_unique<RawTempleRitual>(pool,hash,types,frames,shapes,spots,*footprint,*reward,*actions,templeState,std::move(temple));
        RunePostPopHooks runeHooks;runeHooks.reserveRegular=[this]() -> bool { Forbidden(); };runeHooks.addRegular=[this](Sid,std::uint32_t,float) { Forbidden(); };runeHooks.startConstruction=[this](Sid,float) { Forbidden(); };rune=std::make_unique<RawRunePostPop>(pool,hash,types,runeState,*post,std::move(runeHooks));
        DaisRegularHooks hooks;hooks.setFrame=[this](Sid,std::int32_t,std::uint32_t) { Forbidden(); };hooks.destroy=[this](Sid,std::uint32_t) { Forbidden(); };hooks.reserveShared=[this]() -> bool { Forbidden(); };hooks.addShared=[this](Sid,std::uint32_t,float) { Forbidden(); };hooks.playAt=[this](float,float,std::string_view,std::uint32_t) { Forbidden(); };
        hooks.reserveSound=[this]() -> bool { Forbidden(); };hooks.loadSound=[this](std::string_view,std::uint32_t,std::uint32_t) -> std::uint32_t { Forbidden(); };hooks.attachSound=[this](Sid,std::uint32_t,std::uint32_t,std::uint32_t) -> std::uintptr_t { Forbidden(); };hooks.wallSeconds=[this]() -> double { Forbidden(); };hooks.setSoundStart=[this](std::uintptr_t,double) { Forbidden(); };hooks.stopVoice=[this](std::uint32_t,std::uint32_t,std::uint32_t) { Forbidden(); }; // 소리/지연 예약 경계다.
        hooks.processType=[](std::uint32_t) { return 54U; };hooks.pathMetadata=[this](Sid) -> RawPathStartType { Forbidden(); };hooks.damageable=[this](Sid,std::uint32_t,std::uint32_t,float) -> float { Forbidden(); };hooks.addLocalSp=[this](float) { Forbidden(); };hooks.normalizeForeignSp=[this](std::uint32_t) { Forbidden(); };hooks.grantForeignType=[this](std::uint32_t,std::uint32_t) { Forbidden(); }; // 성공 이동/피해/경제 경계다.
        hooks.localSpecial=[this](std::uint32_t) { Forbidden(); };hooks.localType=hooks.localSpecial;hooks.refreshOwner=hooks.localSpecial;hooks.refreshLocal=[this] { Forbidden(); };hooks.priestExists=[this](std::uint32_t) -> bool { Forbidden(); };hooks.multiplayerStatus=[this]() -> std::uint32_t { Forbidden(); };hooks.chat=[this](const DaisChat&) { Forbidden(); }; // 획득/UI/멀티 안내 경계다.
        hooks.assertion=assertion;hooks.boss=[this] { return currentBoss; };hooks.alliance=[this](std::uint32_t row,std::uint32_t column) { ++allianceReads;allianceArguments.push_back({row,column});return currentAlliance; };hooks.count=[this](Sid sid) { return ritual->Count(sid); };hooks.priestType=[this] { return canonicalPriestType; };dais=std::make_unique<RawDaisRegular>(pool,hash,types,frames,*ritual,*rune,*carrier,*host,*lookup,*path,state,std::move(hooks));
    }
    ~DaisParentScene() { kernel.Clear(); } // 실제 Host/경로 의존 모듈을 반납하기 전에 객체 소멸만 끝낸다. 프레임은 실행하지 않는다.
};
}

TEST_CASE(DaisCheckingParents_CanBridgeCurrentFinderAndMetadataAfterDiagnostic) { // genus 진단 전에 보관한 first/next와 진단 뒤 현재 TYPE/owner/kind/동맹을 구별한다.
    DaisParentScene scene;scene.types[158].flags2=0;Put(scene.Raw(Sid{10}),4,11,2);Put(scene.Raw(Sid{10}),20,0,2);scene.Raw(Sid{11})[10]=6;Put(scene.Raw(Sid{11}),18,0,2);scene.Raw(Sid{12})[10]=6;
    scene.onAssertion=[&](auto expression,auto file,std::uint32_t line) { CHECK(expression=="priest.getGenus() & gPRIEST" && file==".\\Dais.cpp" && line==0x140);scene.Raw(Sid{6})[10]=74;scene.types[74].flags2=0x200000;scene.canonicalPriestType=74;scene.Owner(Sid{5},2);scene.Owner(Sid{6},2);scene.Owner(Sid{7},3);Put(scene.Raw(Sid{5}),6,12,2);Put(scene.Raw(Sid{10}),4,0,2);Put(scene.Raw(Sid{11}),18,74,2);Put(scene.Raw(Sid{11}),20,0x20,2);scene.Raw(Sid{11})[12]=0x84;scene.currentAlliance=0; }; // 새 head12와 바뀐 next0으로 검색을 재시작하면 결과가 달라진다.
    CHECK(scene.dais->CanBridge(Sid{5},Sid{6}));CHECK(scene.diagnostics==std::vector<std::uint32_t>{0x140} && scene.allianceArguments==(std::vector<std::array<std::uint32_t,2>>{{3,4}}) && scene.effects==0);
}

TEST_CASE(DaisCheckingParents_DeadParentDiagnosticPrecedesCurrentHeadRead) { // finder의 dead 진단 복귀 뒤에는 현재 head를 처음 읽는다.
    DaisParentScene scene;scene.currentBoss=false;scene.Raw(Sid{5})[11]=2;Put(scene.Raw(Sid{10}),20,0,2);scene.Raw(Sid{11})[10]=6;Put(scene.Raw(Sid{11}),18,158,2);Put(scene.Raw(Sid{11}),20,0x20,2);scene.Raw(Sid{11})[12]=3;
    scene.onAssertion=[&](auto expression,auto file,std::uint32_t line) { CHECK(expression=="!p.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy" && file=="\\Ns\\O\\Squidfinder.cpp" && line==0x13c);scene.Raw(Sid{5})[11]=0;Put(scene.Raw(Sid{5}),6,11,2);scene.currentBoss=true; };
    CHECK(scene.dais->CanBridge(Sid{5},Sid{6}));CHECK(scene.diagnostics==std::vector<std::uint32_t>{0x13c} && scene.allianceArguments==(std::vector<std::array<std::uint32_t,2>>{{1,3}}) && scene.effects==0);
}

TEST_CASE(DaisCheckingParents_CanBridgeCurrentAllianceAndCdNoPatchDiagnostic) { // 동일 조회에서 현재 동맹을 다시 읽고 CD에 패치 진단을 추가하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 같은 자격 입력에서 두 판본의 의도된 동맹 차이를 유지한다.
        DaisParentScene scene(edition);scene.currentAlliance=1;const auto before=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());CHECK(scene.dais->CanBridge(Sid{5},Sid{6})==(edition==OriginalEdition::Cd1072));scene.currentAlliance=0;CHECK(scene.dais->CanBridge(Sid{5},Sid{6}));CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()) && scene.effects==0);
        if (edition==OriginalEdition::Cd1072) { scene.types[158].flags2=0;CHECK(!scene.dais->CanBridge(Sid{5},Sid{6}) && scene.diagnostics.empty() && scene.allianceReads==0); }
    }
}

TEST_CASE(DaisCheckingParents_BridgeUsesCurrentBossAndNullPathDiagnosticReturns) { // 명령은 현재 권한을 읽고 실제 빈 PathStart 뒤 p 진단에서 정상 반환한다.
    DaisParentScene scene;scene.currentBoss=false;scene.templeState.boss=true;scene.dais->BridgeCommand(Sid{5},Sid{6},1);CHECK(scene.pathCreates==0 && scene.effects==0);
    scene.currentBoss=true;scene.templeState.boss=false;scene.onAssertion=[&](auto expression,auto file,std::uint32_t line) { CHECK(expression=="p" && file==".\\Dais.cpp" && line==0x18e);scene.currentBoss=false;scene.checking=0; }; // NULL 뒤 진단 복귀는 재시도나 성공 경로 효과를 실행하지 않는다.
    scene.dais->BridgeCommand(Sid{5},Sid{6},1);CHECK(scene.pathCreates==1 && scene.diagnostics==std::vector<std::uint32_t>{0x18e} && scene.kernel.Size()==0 && scene.effects==0 && !scene.retry);
}

TEST_CASE(DaisCheckingParents_DepthDiagnosticsReturnCurrentRuntimeValue) { // 두 진단의 비교는 저장 depth이며 최종 반환은 현재 depth다.
    DaisParentScene scene;auto* process=RawPathProcess::Attach(*scene.host,scene.EmptyPath(),Sid{6},0x10);CHECK(process);if (!process) return;process->Runtime().SetMoveAwayDepth(-1);
    scene.onAssertion=[&](auto expression,auto file,std::uint32_t line) { CHECK(expression=="0 <= p->moveAwayDepth" && file=="\\Ns\\O\\Carrier.cpp" && line==0x87b);process->Runtime().SetMoveAwayDepth(5); };CHECK(scene.dais->MoveAwayDepth(Sid{6})==5 && scene.diagnostics==std::vector<std::uint32_t>{0x87b}); // 새 depth5로 두 번째 비교를 다시 만들지 않는다.
    scene.diagnostics.clear();process->Runtime().SetMoveAwayDepth(4);scene.onAssertion=[&](auto expression,auto file,std::uint32_t line) { CHECK(expression=="p->moveAwayDepth <= MAX_MOVE_AWAY_DEPTH" && file=="\\Ns\\O\\Carrier.cpp" && line==0x87c);scene.checking=0;process->Runtime().SetMoveAwayDepth(2); };CHECK(scene.dais->MoveAwayDepth(Sid{6})==2 && scene.diagnostics==std::vector<std::uint32_t>{0x87c} && scene.effects==0);
}

TEST_CASE(DaisCheckingParents_StageUsesOneVirtualCountAndSavedInitialFrame) { // Stage는 효과 없이 count callback 한 번과 저장 프레임만으로0..5를 반환한다.
    constexpr std::array<std::int32_t,11> offsets{0,10,11,21,22,32,33,43,44,54,55};constexpr std::array<std::uint32_t,11> stages{0,0,1,1,2,2,3,3,4,4,5};constexpr std::array<std::int32_t,3> first{3,64,120}; // 제품 분기를 복제하지 않는 수동 경계 기대값이다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 현재 frame의 DWORD/BYTE 차이도 같은 수동 표로 대조한다.
        DaisParentScene scene(edition);for (std::size_t count=0;count<first.size();++count) for (std::size_t index=0;index<offsets.size();++index) { // count1..3과 각 단계 양 끝을 독립 입력으로 조회한다.
            scene.countValue=static_cast<std::uint32_t>(count+1);scene.FrameInput(Sid{5},static_cast<std::uint32_t>(first[count]+offsets[index]));const auto before=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());const auto reads=scene.countReads;CHECK(scene.ritual->Stage(Sid{5})==stages[index] && scene.countReads==reads+1);CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
        }
        scene.countValue=0;const auto zeroFirst=edition==OriginalEdition::Patch1078 ? 1 : 10611;CHECK(scene.ritual->Stage(Sid{5},zeroFirst)==0 && scene.ritual->Stage(Sid{5},zeroFirst+55)==5); // 실제 PE의 count0 첫 DWORD는 양판본1/10611로 서로 다르다.
        scene.countValue=4;bool failed=false;try { scene.ritual->Stage(Sid{5},3); }catch (const std::out_of_range&) { failed=true; }CHECK(failed && scene.effects==0); // 가상 count가 표를 벗어나면 명시 실패한다.
        scene.countValue=1;scene.FrameInput(Sid{5},25);scene.onCount=[&] { scene.FrameInput(Sid{5},3); };const auto reads=scene.countReads;CHECK(scene.ritual->Stage(Sid{5})==2 && scene.countReads==reads+1 && scene.effects==0); // callback이 바꾼 프레임3 대신 최초25에서 count1 첫3을 뺀다.
    }
}

namespace { // 현재 가상 분배/일반 후처리만 관찰하며 희생/해방/소리/공격은 모든 장치에서 거부한다.
struct DaisLifecycleCurrentScene : DaisParentScene { // 기존 읽기 장면의 같은 Pool/Host/Temple을 수명 객체에 연결한다.
    CarrierPostPopState loading;DamageablePreDestroyState damageState;AltarSacrificeState altarState; // 가상 분배의 독립 입력 상태다.
    double loginEnd{1};DaisLifecycleState lifeState{loginEnd,loading};std::size_t common{},validations{}; // 최초 프레임 초기화가 없는 시각과 공통 진입 횟수다.
    std::unique_ptr<RawDamageablePreDestroy> damage;std::unique_ptr<RawCarrierPostPop> carrierPost;std::unique_ptr<RawFrameAdvance> advance;std::unique_ptr<RawAltarSacrifice> altar; // 실제 같은 풀의 의존 몸체를 검사보다 오래 유지한다.
    template<class R,class... A> std::function<R(A...)> Ban() { return [this](A...)->R { Forbidden(); }; } // 실행 금지 효과는 빈 성공 대신 관측 가능한 실패를 낸다.
    explicit DaisLifecycleCurrentScene(OriginalEdition edition=OriginalEdition::Patch1078):DaisParentScene(edition) { // 실제 게임/틱 없이 가상 진입에 필요한 동일 풀의 몸체만 설치한다.
        DamageablePreDestroyHooks d;d.collapse=Ban<void,Sid>();d.explosion=d.collapse;d.sound=Ban<void,const DamageableSoundEvent&>();d.releaseContained=d.collapse;d.validateBase=Ban<void,Sid,std::uint32_t>();d.base=d.validateBase;damage=std::make_unique<RawDamageablePreDestroy>(pool,spots,finderState,damageState,std::move(d));
        CarrierPostPopHooks c;c.carrierCheck=Ban<void,Sid>();c.claimGround=Ban<void,float,float,std::uint8_t>();c.validateBase=[this](Sid,std::uint32_t) { ++validations; };c.base=[this](Sid,std::uint32_t) { ++common; };carrierPost=std::make_unique<RawCarrierPostPop>(pool,types,loading,std::move(c));advance=std::make_unique<RawFrameAdvance>(pool,frames,*frame);
        AltarSacrificeHooks a;a.setFrame=Ban<void,Sid,std::int32_t,std::uint32_t>();a.destroy=Ban<void,Sid,std::uint32_t>();a.reserveShared=Ban<bool>();a.addShared=Ban<void,Sid,std::uint32_t,float>();a.randomInt=Ban<std::int32_t,std::int32_t,std::int32_t>();a.randomFloat=Ban<float,float,float>();a.playAt=Ban<void,float,float,std::string_view,std::uint32_t>();a.playGlobal=Ban<void,std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t>();a.removeAnimation=Ban<void,Sid>();a.unpop=a.destroy;a.repop=a.destroy;a.create=Ban<Sid,std::uint32_t,std::uint32_t>();a.setOwner=a.destroy;a.pop=Ban<void,Sid,float,float,std::uint32_t>();a.escapeEffect=Ban<void,Sid,float,std::uint32_t>();a.refreshOwner=Ban<void,std::uint32_t>();a.damageable=Ban<float,Sid,std::uint32_t,std::uint32_t,float>();altar=std::make_unique<RawAltarSacrifice>(pool,hash,types,frames,*footprint,*rune,*ritual,*carrier,*dais,*advance,*host,*damage,state,altarState,std::move(a));
    }
    DaisLifecycleHooks FullHooks() { // CD의 기존 필수 장치 계약을 만족시키되 선택되지 않아야 할 효과는 모두 거부한다.
        DaisLifecycleHooks h;h.setFrame=Ban<void,Sid,std::int32_t,std::uint32_t>();h.unpop=Ban<void,Sid,std::uint32_t>();h.repop=h.unpop;h.create=Ban<Sid,std::uint32_t,std::uint32_t>();h.setOwner=h.unpop;h.pop=Ban<void,Sid,float,float,std::uint32_t>();h.reserveRegular=Ban<bool>();h.addRegular=Ban<void,Sid,std::uint32_t,float>();h.surface=Ban<Sid,float,float>();h.hasIsland=Ban<bool,Sid>();h.islandNumber=Ban<std::uint32_t,Sid>();h.option=Ban<bool,std::string_view>();h.chat=Ban<void,std::string_view>();h.destroy=h.unpop;h.damageablePostDestroy=h.unpop;return h;
    }
    std::unique_ptr<RawDaisLifecycle> Make(DaisLifecycleHooks h) { return std::make_unique<RawDaisLifecycle>(pool,hash,types,*footprint,*ritual,*altar,*carrierPost,*damage,state,altarState,lifeState,std::move(h)); } // 실제 동일 풀 검증을 우회하지 않는다.
};
}
TEST_CASE(DaisClassCurrent_RegularKeepsVtableFunctionIdentity) { // PATCH의 Rune/Dais 분기는 표시 TYPE가 아니라 실제 가상 함수로 정한다. CD 기존 계약도 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 같은 두 입력에서 판본별 허용 계약을 분리한다.
        DaisParentScene scene(edition);auto raw=scene.Raw(Sid{5});Put(raw,0,scene.Patch() ? kPatchRuneVtable : kCdRuneVtable);raw[10]=165;CHECK(scene.dais->Handles(Sid{5})==scene.Patch());
        if (scene.Patch()) CHECK(scene.dais->Handle(Sid{5},99,0,.25F)==.25F && scene.effects==0);
        raw[10]=166;CHECK(scene.dais->Handles(Sid{5}) && scene.dais->Handle(Sid{5},99,0,.5F)==.5F && scene.effects==0);
        Put(raw,0,scene.Patch() ? kPatchDaisVtable : kCdDaisVtable);raw[10]=166;CHECK(scene.dais->Handles(Sid{5})==scene.Patch());
        if (scene.Patch()) { bool selected=false;try { static_cast<void>(scene.dais->Handle(Sid{5},99,0,.25F)); }catch (const std::logic_error&) { selected=true; }CHECK(selected && scene.effects==1); } // 실제 의식 대신 선택된 일반 Damageable 경계에서 중단한다.
    }
}
TEST_CASE(DaisClassCurrent_CountKeepsEditionSpecificPlayerDestination) { // 가상 Count가 owner/Player 저장소를 바꿔도 원문 판본별 주소 저장 순서를 유지한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // PATCH는 주소를 먼저, CD는 owner만 먼저 고정한다.
        DaisLifecycleCurrentScene scene(edition);const auto stride=scene.Patch() ? 0xb4U : 0xacU,offset=scene.Patch() ? 0x71U : 0x69U;std::vector<std::uint8_t> before(9*stride,0xa5),after(9*stride,0xb6);scene.state.BindPlayerRecords(edition,before);auto h=scene.FullHooks();std::size_t counts{};h.suppressed=[] { return false; };h.count=[&](Sid sid) { CHECK(sid==Sid{5});++counts;scene.Owner(sid,2);scene.state.BindPlayerRecords(edition,after);return 3U; };auto life=scene.Make(std::move(h));life->PostPop(Sid{5},0);
        CHECK(counts==1 && scene.common==1 && scene.effects==0);CHECK(before[stride+offset]==(scene.Patch() ? 2 : 0xa5));CHECK(after[stride+offset]==(scene.Patch() ? 0xb6 : 2));CHECK(after[2*stride+offset]==0xb6); // 새 owner2로 쓰거나 PATCH에서 새 저장소로 쓰면 실패한다.
    }
}
TEST_CASE(DaisClassCurrent_LifecycleLazyDevicesAndPartialPlayerCleanup) { // 미배치/표시 TYPE70에서도 실제 VT 후처리는 선택 장치와 도달 전 부분 효과를 보존한다.
    DaisLifecycleCurrentScene scene;scene.Raw(Sid{5})[10]=70;scene.Raw(Sid{5})[11]=1;scene.Owner(Sid{5},0);DaisLifecycleHooks h;h.suppressed=[] { return true; };auto life=scene.Make(std::move(h));CHECK(life->Handles(Sid{5}));life->PostPop(Sid{5},0);CHECK(scene.common==1 && scene.effects==0);
    bool explicitRejected=false;try { life->Validate(Sid{5},0); }catch (const std::invalid_argument&) { explicitRejected=true; }CHECK(explicitRejected); // 명시 preflight의 기존 TYPE 계약은 실제 가상 진입과 구별한다.
    Put(scene.Raw(Sid{7}),0,kPatchAltarVtable);scene.Raw(Sid{7})[10]=165;CHECK(scene.altar->Handles(Sid{7}) && life->Handles(Sid{7}));life->PostPop(Sid{7},0);CHECK(scene.common==2 && scene.effects==0); // Altar VT는 표시 TYPE165여도 Count/모드 장치에 진입하지 않는다.
    scene.Owner(Sid{5},1);scene.lifeState.players[1]={Sid{5},Sid{123},Sid{7},Sid{124},0};bool missing=false;try { life->PostDestroy(Sid{5},0x123); }catch (const std::logic_error&) { missing=true; }const auto& p=scene.lifeState.players[1];CHECK(missing && p.first.value==0 && p.firstMirror.value==0 && p.second==Sid{7} && p.secondMirror==Sid{124} && scene.effects==0); // 선택된 공통 장치 부재 앞의 SID쌍 쓰기를 되돌리지 않는다.
}

TEST_CASE(RuneSharedCurrent_TempleLookupReadsTypeAfterVirtualCount) { // 두 원문 모두 가상 Count로 단계를 얻은 뒤 현재 Rune 번호를 읽는다. 의식/생성/삭제는 실행하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 표시 프레임 폭이 달라도 같은 순서로 현재 번호와 일반 finder를 사용한다.
        DaisParentScene scene(edition);RawGameAuxState owner;scene.templeState.sharedRuneType=&owner.RuneType();owner.RuneType()=70;scene.types[71]=scene.types[166];scene.frames[71]=scene.frames[166];scene.shapes[71]=scene.shapes[166];scene.Object(Sid{8},71,40.75F,41.25F);scene.Raw(Sid{8})[scene.Patch() ? 40 : 35]=1;auto& head=scene.hash.Bucket(1,40.75F,41.25F);Put(scene.Raw(Sid{8}),4,head,2);head=8;scene.onCount=[&] { owner.RuneType()=71; };
        CHECK(scene.ritual->RuneAt(Sid{5},{40.75F,41.25F})==Sid{8} && owner.RuneType()==71 && scene.countReads==1);CHECK(scene.ritual->HasAbstractRune(Sid{5}));owner.RuneType()=70;CHECK(!scene.ritual->HasAbstractRune(Sid{5}) && scene.effects==0); // 사본166 또는 Count 전 번호70으로 검색하면 결과가 달라진다.
    }
}
