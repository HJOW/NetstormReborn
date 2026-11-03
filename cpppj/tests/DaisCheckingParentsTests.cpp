// 다이스 자격·진단 복귀·읽기 getter만 검사한다. 실제 의식/포획/희생/전투/성공 이동은 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawDaisRegular.h"
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
