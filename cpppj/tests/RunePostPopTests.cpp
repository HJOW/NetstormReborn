// Rune 실제예약/신전조회/건설시간과 같은Host·공간·Construction완료를 검사한다.
#include "RawSceneSupport.h"
#include "o/RawRunePostPop.h"
#include "o/RawConstructionProcess.h"
#include "o/RawCarrierPreDestroy.h"
#include "o/RawSquidHitPoints.h"
#include "o/SquidFactory.h"
#include "o/SquidOwner.h"
#include "o/SquidDisplay.h"
#include "o/SquidPop.h"
#include "o/SquidProcess.h"
#include "o/SquidUnpop.h"
#include "o/RawGameAuxState.h" // 현재 제단/다이스 번호의 장기 owner를 실제 연결과 같은 형식으로 빌린다.
#include "o/RawPriestParticipation.h" // 참조형 참여자 입력에 getter 임시값을 남기지 않는지 확인한다.
#include "o/RawAiBuildChoice.h" // AI가 보관하는 같은 다이스 번호 참조를 확인한다.
#include "o/RawAiDropTargets.h" // 배치 AI의 제단 번호 참조를 확인한다.

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 순서가있는명시효과를원본fixture와같은문자열로합친다.
std::string RuneEffects(const std::vector<std::string>& effects) {
    std::string result;
    // 예약/건설/공통호출순서를보존한다.
    for (const auto& effect:effects) { if (!result.empty()) result+=';';result+=effect; }
    return result.empty() ? "-" : result;
}
// 실제SquidDisplay의변경영역을수집하며억제하지않는다.
class RuneDisplaySink final:public SquidDisplaySink {
public:
    bool Suppressed() const override { return false; } // 실제표시분기와frameCheck를실행한다.
    void Invalidate(SquidDisplayRect rect,std::uint32_t flags) override { dirty.push_back(rect);lastFlags=flags; } // 현재영역과플래그를기록한다.
    std::vector<SquidDisplayRect> dirty; // Pop/HP표시/Unpop/Repop가등록한모든영역이다.
    std::uint32_t lastFlags{}; // 마지막실제표시요청플래그다.
};
}

// 실제Rune postPop/반경조회/시간helper와callback후raw를양판본원본관찰과전수대조한다.
TEST_CASE(rawRunePostPop_native_regular_owned_dais_and_duration) {
    const auto fixture=LoadFixture(NETSTORM_RUNEPOSTPOP_FIXTURE);std::size_t checked=0;
    // 각입력은독립pool/hash/player표를사용해이전기대값을재사용하지않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==7);const auto input=Split(row[1],',');CHECK(input.size()==8);const auto flags=Number(input[0]),profile=Number(input[1]),reserve=Number(input[2]),word=Number(input[3]),playerFlag=Number(input[4]),suppress=Number(input[5]),mutation=Number(input[6]),extra=Number(input[7]);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078;
        SidPool pool(edition,24000,true);SquidHash hash;std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::array<std::span<std::uint8_t>,9> slots;
        // 후보세개의사용하지않는필드까지native와같은0초기값으로만든다.
        for (std::uint16_t sid=5;sid<=8;++sid) { CHECK(pool.Allocate(2)==Sid{sid});slots[sid]=pool.AllocatedBytes(Sid{sid});std::fill(slots[sid].begin(),slots[sid].end(),std::uint8_t{}); }
        const auto add=[&](Sid sid,std::uint32_t type,float x,float y,std::uint32_t currentExtra,std::uint32_t rawState,std::int32_t foot) { auto raw=slots[sid.value];Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(rawState);raw[patch ? 40 : 35]=static_cast<std::uint8_t>(currentExtra);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));types[type].footX=types[type].footY=foot;auto& head=hash.Bucket(1,x,y);Put(raw,4,head,2);head=sid.value; };
        add(Sid{8},166,40.75F,41.25F,extra==0 ? 0 : extra==1 ? 1 : 8,0,1);
        if (profile) add(Sid{6},165,profile==7 ? 80.75F : profile==6 ? 42.75F : 40.75F,profile==7 ? 81.25F : profile==6 ? 43.25F : 41.25F,profile==3 ? 8 : profile==5 ? 1 : 0,profile==4 ? 2 : 0,profile==6 ? 3 : 1);
        if (profile!=8) add(Sid{7},164,profile==9 ? 80.75F : 40.75F,profile==9 ? 81.25F : 41.25F,0,0,1);
        Put(slots[8],0,patch ? kPatchRuneVtable : kCdRuneVtable);slots[8][patch ? 34 : 32]=1;slots[6][patch ? 34 : 32]=static_cast<std::uint8_t>(profile==2 ? 2 : 1);Put(slots[7],12,word,2);RunePostPopState state;state.suppressDuration=suppress!=0;state.playerFlags[word]=playerFlag;SquidPostPopState book;book.depth=1;SquidPostPop base(pool,types,book);std::vector<std::string> effects;RunePostPopHooks hooks;
        hooks.reserveRegular=[&] { effects.push_back("N:40");if (mutation==1) state.suppressDuration=!state.suppressDuration;return reserve!=0; };
        hooks.addRegular=[&](Sid sid,std::uint32_t event,float payload) { CHECK(sid==Sid{8} && event==0x304 && payload==0.01F);effects.push_back("R:"+std::to_string(event)+",8,"+std::to_string(std::bit_cast<std::uint32_t>(payload)));if (mutation==2) slots[8][patch ? 34 : 32]=2; };
        hooks.startConstruction=[&](Sid sid,float seconds) { CHECK(sid==Sid{8});effects.push_back("C:8,"+std::to_string(std::bit_cast<std::uint32_t>(seconds))); };
        RawRunePostPop rune(pool,hash,types,state,base,std::move(hooks));CHECK(rune.ConstructionSeconds(Sid{8})==static_cast<std::int32_t>(Number(row[2])));if (patch) CHECK(rune.FindOwnedDais(Sid{8}).value==Number(row[3]));rune.PostPop(Sid{8},flags);CHECK(book.depth==0);effects.push_back("B:8,"+std::to_string(flags));CHECK(RuneEffects(effects)==row[4] && state.suppressDuration==(Number(row[6])!=0));const auto images=Split(row[5],';');CHECK(images.size()==3);
        // 가상표주입외각후보/raw Rune의모든바이트를비교한다.
        for (std::uint16_t sid=6;sid<=8;++sid) { const auto image=Hex(pool.Slot(Sid{sid}));CHECK(image.size()==images[sid-6].size() && image.substr(8)==images[sid-6].substr(8)); }
        ++checked;
    }
    CHECK(checked>1000);
}

namespace { // 실제 finder/시간 helper/공통 부모를 작은 독립 현재 월드에 연결한다.
struct RuneCurrentScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true};SquidHash hash; // 현재 세 물리 슬롯과 실제 일반 finder 해시다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188);RunePostPopState state; // 현재 타입 표와 독립 시간 전역이다.
    SquidPostPopState book;SquidPostPop base{pool,types,book}; // 같은 공통 부모와 원본 깊이다.
    std::array<std::span<std::uint8_t>,8> slots{};std::vector<std::string> events; // 세 슬롯 뷰와 실제 도달 효과의 관찰이다.
    RuneCurrentScene() { // finder 입력만 준비하고 Rune 제품의 선택 결과는 계산하지 않는다.
        for (std::uint16_t value=5;value<=7;++value) { // Rune5·Dais6·Altar7을 같은 위치에 준비한다.
            CHECK(pool.Allocate(2)==Sid{value});auto raw=pool.AllocatedBytes(Sid{value});slots[value]=raw;std::fill(raw.begin(),raw.end(),std::uint8_t{});const auto type=value==5 ? 166U : value==6 ? 165U : 164U;raw[10]=static_cast<std::uint8_t>(type);raw[34]=1;Put(raw,14,std::bit_cast<std::uint32_t>(40.75F));Put(raw,18,std::bit_cast<std::uint32_t>(41.25F));types[type].footX=types[type].footY=1;Put(raw,0,value==5 ? kPatchRuneVtable : kVtable);
            if (value!=5) { auto& head=hash.Bucket(1,40.75F,41.25F);Put(raw,4,head,2);head=value; }
        }
        Put(slots[7],12,1,2);book.depth=1;book.suppressed=true;base.BindCurrentTypes(types);
    }
    RunePostPopHooks Hooks() { // 호출의 인자/순서만 관찰하고 finder·시간 계산은 실제 제품에 맡긴다.
        RunePostPopHooks hooks;hooks.reserveRegular=[this] { events.push_back("N");return true; };hooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { CHECK(sid==Sid{5} && event==0x304 && payload==.01F);events.push_back("R"); };hooks.startConstruction=[this](Sid sid,float seconds) { CHECK(sid==Sid{5});events.push_back("C"+std::to_string(static_cast<int>(seconds))); };return hooks;
    }
};
}
TEST_CASE(BuriedRuneCurrent_RuneActualVtableWithDifferentTypeAndMissingUnusedDevices) { // 실제 PATCH VT 진입은 TYPE/free 사전 검사를 추가하지 않고 공통 부모로 간다.
    RuneCurrentScene scene;auto raw=scene.slots[5];raw[10]=74;raw[11]=1;raw[40]=8;RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,{});scene.base.SetRunePostPop(&rune);CHECK(rune.Handles(Sid{5}) && Throws([&] { rune.Validate(Sid{5},0); }));scene.base.PostPopVirtual(Sid{5},0);CHECK(scene.book.depth==0 && scene.events.empty());
}
TEST_CASE(BuriedRuneCurrent_RuneMissingReservePrecedesFinderAndBase) { // 최초 확보가 없으면 뒤의 좌표 오류나 공통 부모보다 먼저 실패한다.
    RuneCurrentScene scene;Put(scene.slots[5],14,0x7fc00000);auto hooks=scene.Hooks();hooks.reserveRegular={};RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));CHECK(Throws([&] { rune.PostPop(Sid{5},3); }) && scene.events.empty() && scene.book.depth==1);
}
TEST_CASE(BuriedRuneCurrent_RuneFailedAllocationSkipsMissingConstructor) { // 최초 확보 실패는 미연결 생성자를 요구하지 않고 flags2의 현재 Dais/시간을 계속한다.
    RuneCurrentScene scene;auto hooks=scene.Hooks();hooks.reserveRegular=[&] { scene.events.push_back("N");return false; };hooks.addRegular={};RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));rune.PostPop(Sid{5},3);CHECK(RuneEffects(scene.events)=="N;C7" && scene.book.depth==0);
}
TEST_CASE(BuriedRuneCurrent_RuneMissingConstructorPreservesReservation) { // 확보 성공 뒤 생성자 실패는 뒤 finder/시간/공통 부모를 실행하지 않는다.
    RuneCurrentScene scene;Put(scene.slots[5],14,0x7fc00000);auto hooks=scene.Hooks();hooks.addRegular={};RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));CHECK(Throws([&] { rune.PostPop(Sid{5},3); }) && RuneEffects(scene.events)=="N" && scene.book.depth==1);
}
TEST_CASE(BuriedRuneCurrent_RuneRegularMutationUsesCurrentDaisOwnerAndPhysicalSlot) { // 304 생성 효과 뒤 현재 Dais TYPE/소유자를 읽고 free/VT 변경을 재검사하지 않는다.
    RuneCurrentScene scene;auto hooks=scene.Hooks();hooks.addRegular=[&](auto,auto,auto) { scene.events.push_back("R");scene.slots[5][11]=1;scene.slots[5][10]=255;scene.slots[5][34]=2;Put(scene.slots[5],0,0);scene.slots[6][34]=2;scene.slots[6][10]=74;scene.state.daisType=74;scene.types[74].footX=scene.types[74].footY=1; };RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));rune.PostPop(Sid{5},3);CHECK(RuneEffects(scene.events)=="N;R;C7" && scene.book.depth==0 && scene.slots[5][11]==1 && scene.slots[5][10]==255);
}
TEST_CASE(BuriedRuneCurrent_RuneNoDaisSkipsMissingConstruction) { // 후보가 없으면 사용하지 않는 현재 Dais DWORD와 건설 장치를 미리 검사하지 않는다.
    RuneCurrentScene scene;scene.state.daisType=65535;auto hooks=scene.Hooks();hooks.startConstruction={};RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));rune.PostPop(Sid{5},2);CHECK(scene.events.empty() && scene.book.depth==0);
}
TEST_CASE(BuriedRuneCurrent_RuneDurationReadPrecedesMissingConstruction) { // 원문 helper의 현재 타입 표 역참조 오류가 도달할 건설 장치 검사보다 앞선다.
    RuneCurrentScene scene;scene.state.altarType=188;auto hooks=scene.Hooks();hooks.startConstruction={};RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));bool physical=false;try { rune.PostPop(Sid{5},2); }catch (const std::out_of_range&) { physical=true; }CHECK(physical && scene.book.depth==1 && scene.events.empty());
}
TEST_CASE(BuriedRuneCurrent_RuneDurationReadsBoundGlobalsAfterReservation) { // 독립 사본과 다른 실제 저장소를 예약 효과 뒤에 읽어25초를 선택한다.
    RuneCurrentScene scene;bool suppressed=true;std::uint32_t flags=0;scene.state.suppressDuration=true;auto hooks=scene.Hooks();hooks.reserveRegular=[&] { scene.events.push_back("N");suppressed=false;flags=1;return false; };hooks.suppressDuration=[&] { scene.events.push_back("S");return suppressed; };hooks.playerFlags=[&](auto owner) { CHECK(owner==1);scene.events.push_back("P");return flags; };RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));rune.PostPop(Sid{5},3);CHECK(RuneEffects(scene.events)=="N;S;P;C25" && scene.book.depth==0);
}
TEST_CASE(BuriedRuneCurrent_RuneDurationSuppressionSkipsInvalidOwnerRecord) { // 현재 억제 전역은 원래 owner WORD의 Player 접근보다 먼저 선택된다.
    RuneCurrentScene scene;Put(scene.slots[7],12,65535,2);auto hooks=scene.Hooks();hooks.suppressDuration=[&] { scene.events.push_back("S");return true; };hooks.playerFlags=[](auto)->std::uint32_t { throw std::out_of_range("접근하지 않을 Player"); };RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));rune.PostPop(Sid{5},2);CHECK(RuneEffects(scene.events)=="S;C10" && scene.book.depth==0);
}
TEST_CASE(BuriedRuneCurrent_RuneCdKeepsConstructorAndTypePreflight) { // CD의 기존 전수 연결/TYPE 계약을 PATCH 수정과 구별한다.
    SidPool pool(OriginalEdition::Cd1072,24000,true);SquidHash hash;std::vector<RiftTypeRecord> types(171);RunePostPopState state;SquidPostPopState book;SquidPostPop base(pool,types,book);CHECK(Throws([&] { RawRunePostPop missing(pool,hash,types,state,base,{}); }));CHECK(pool.Allocate(2)==Sid{5});auto raw=pool.AllocatedBytes(Sid{5});Put(raw,0,kCdRuneVtable);raw[10]=74;RunePostPopHooks hooks;hooks.reserveRegular=[] { return false; };hooks.addRegular=[](auto,auto,auto) {};hooks.startConstruction=[](auto,auto) {};RawRunePostPop rune(pool,hash,types,state,base,std::move(hooks));CHECK(!rune.Handles(Sid{5}) && Throws([&] { rune.PostPop(Sid{5},0); }));
}

// 실제Rune생성자→abstract Pop→Construction부착/진행/완료→실제Repop→304Regular를같은공간에서실행한다.
TEST_CASE(rawRunePostPop_real_pop_construction_complete_and_304) {
    // 두판본의서버Rune과clientProcessForm을같은풀에등록한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,24000,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        // 실제타입별생성자와합성HP/한칸프레임자료를분리해입력한다.
        for (const auto type:{164U,165U,166U}) { types[type].constructorAddress=TypeConstructorAddress(edition,type);types[type].footX=types[type].footY=1;types[type].flags1=16;types[type].maxHitPoints=200; }
        std::vector<SquidDisplayShape> shapes(types.size());shapes[166].loaded=true;shapes[166].frameCount=6;shapes[166].frames.assign(6,SquidDisplayFrame{16,11,0,0,1,1});RuneDisplaySink sink;SquidDisplay display(edition,types,shapes,sink,SquidDisplayView{0,0,65536,{0,0,4096,4096}});SquidPostPopState book;book.localOwner=1;SquidPostPop post(pool,types,book);SquidPop pop(pool,hash,spots,&display,&post);SquidUnpop unpop(pool,hash,spots,&display);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel(32);SquidProcessState clock;clock.now=2;SquidProcessHost host(pool,types,kernel,destroy,clock);SquidFactory factory(pool,types);SquidOwnerMode ownerMode;SquidOwner owner(pool,types,book,ownerMode);
        const auto dais=factory.Create(165,2),altar=factory.Create(164,2),rune=factory.Create(166,0);owner.Set(dais,1);owner.Set(rune,1);auto daisRaw=pool.AllocatedBytes(dais),altarRaw=pool.AllocatedBytes(altar),runeRaw=pool.AllocatedBytes(rune);Put(altarRaw,12,1,2);SetRawSquidHitPoints(pool,rune,0);runeRaw[patch ? 40 : 35]|=1;
        // 초기신전/altar는기존일반finder의명시공간입력이며Rune은실제Pop을실행한다.
        for (const auto sid:{dais,altar}) { auto raw=pool.AllocatedBytes(sid);raw[11]=0;Put(raw,14,std::bit_cast<std::uint32_t>(40.75F));Put(raw,18,std::bit_cast<std::uint32_t>(41.25F));auto& head=hash.Bucket(1,40.75F,41.25F);Put(raw,4,head,2);head=sid.value; }
        GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidRewardState rewardState;SquidReward reward(pool,types,book,rewardState,patch ? &store : nullptr);ContainedFinderState containedState;CarrierPreDestroyHooks containedHooks;
        containedHooks.validateDamageable=[](Sid,std::uint32_t) { throw std::logic_error("Rune소리조회는Carrier삭제검사를실행하지않는다"); };containedHooks.damageablePre=[](Sid,std::uint32_t) { throw std::logic_error("Rune소리조회는Carrier삭제를실행하지않는다"); };containedHooks.dependentFound=[] { throw std::logic_error("Rune소리조회는Carrier후처리를실행하지않는다"); };RawCarrierPreDestroy contained(pool,containedState,std::move(containedHooks));
        ConstructionProcessState constructionState;constructionState.now=2;constructionState.quickBuild=true;std::uint32_t begun=0,refreshes=0,completedSounds=0,spatial=0;ConstructionProcessHooks constructionHooks;
        constructionHooks.reserve=[] { return true; };constructionHooks.effect=[&](Sid sid,bool instant) { CHECK(sid==rune && !instant);++begun; };
        constructionHooks.loadSound=[](std::string_view name) { CHECK(name=="jimBuild.wav");return 77U; };constructionHooks.reserveSound=[] { return false; };constructionHooks.attachSound=[](Sid,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("진행음확보실패입력은소리프로세스를부착하지않는다"); };
        constructionHooks.refresh=[&] { ++refreshes; };constructionHooks.updateDisplay=[&](Sid sid) { CHECK(sid==rune);display.Update(pool.Slot(sid),0); };
        constructionHooks.removeContained=[&](Sid sid,std::uint32_t kind) { CHECK(sid==rune && kind==47 && contained.FindContained(sid,0,kind,0)==Sid{}); };
        constructionHooks.playAt=[&](float x,float y,std::string_view name,std::uint32_t flags) { CHECK(x==40.75F && y==41.25F && name=="buildDone.wav" && flags==0);++completedSounds; };constructionHooks.readySound=[](std::uint32_t type) { CHECK(type==166);return 0U; };constructionHooks.play=[](std::uint32_t) { throw std::logic_error("readySound0입력은추가소리를재생하지않는다"); };
        constructionHooks.unpop=[&](Sid sid,std::uint32_t flags) { CHECK(sid==rune && flags==0);unpop.Unpop(sid,types[166],flags);++spatial; };
        constructionHooks.repop=[&](Sid sid,std::uint32_t flags) { CHECK(sid==rune && flags==9);CHECK(pop.Pop(sid,types[166],1,1,40.75F,41.25F,flags)==RawPopResult::Registered);++spatial; };
        constructionHooks.push=[](float,float,std::uint32_t,std::uint32_t) { throw std::logic_error("이Rune입력genus는건설완료밀어내기대상이아니다"); };
        RawConstructionProcess construction(pool,types,constructionState,MakeConstructionProcessHooks(pool,reward,nullptr,std::move(constructionHooks)),&host);RunePostPopState runeState;RunePostPopHooks runeHooks;runeHooks.reserveRegular=[] { return true; };RawRunePostPop runePost(pool,hash,types,runeState,post,MakeRunePostPopHooks(pool,host,construction,std::move(runeHooks)));post.SetRunePostPop(&runePost);
        CHECK(post.HandlesRune(rune) && !SquidPop::Supports(edition,Get(runeRaw,0),0));CHECK(pop.Pop(rune,types[166],1,1,40.75F,41.25F,0)==RawPopResult::Registered);CHECK(begun==1 && kernel.Size()==1 && !host.FindEvent(rune,0x304));const Sid form{static_cast<std::uint16_t>(Get(runeRaw,6,2))};auto* process=dynamic_cast<ConstructionProcess*>(host.FromForm(form));CHECK(process && process->Fields().start==2 && process->Fields().end==9);
        process->RunFrame();CHECK(Get(pool.Slot(rune),26,patch ? 4 : 2)==200 && !(pool.Slot(rune)[patch ? 40 : 35]&1));CHECK(spatial==2 && refreshes==2 && completedSounds==1 && book.depth==0 && book.globalCounts[166]==1 && !sink.dirty.empty());const auto* pending=host.FindEvent(rune,0x304);CHECK(pending && pending->Payload()==0.01F && pending->Time()==2 && pending->Count()==0 && kernel.Size()==1);kernel.Clear();
    }
}

// 참조형 소비자가 원래 owner를 계속 읽으며 fallback 필드와 섞이지 않는지 검사한다(단위870).
TEST_CASE(RuneTypeNumber_ReferencesForParticipationAndAiKeepOriginalOwner) {
    RawGameAuxState owner;RunePostPopState state;const std::uint32_t priest=158,local=1,known=0,collector=133,directional=140;const std::array<std::uint32_t,1> workshops{75};
    CHECK(&state.AltarType()==&state.altarType && &state.DaisType()==&state.daisType);state.sharedAltarType=&owner.AltarType();state.sharedDaisType=&owner.DaisType();
    PriestParticipationState participation{priest,state.AltarType(),state.DaisType()};AiDropTargetGlobals drop{state.AltarType(),directional};AiBuildChoiceGlobals build{local,known,collector,state.DaisType(),workshops,[](auto) { return 128U; }};
    owner.AltarType()=70;owner.DaisType()=71;state.altarType=72;state.daisType=73;CHECK(participation.altarType==70 && participation.daisType==71 && drop.altarType==70 && build.daisType==71);
    CHECK(&participation.altarType==&owner.AltarType() && &build.daisType==&owner.DaisType());state.sharedAltarType=state.sharedDaisType=nullptr;CHECK(state.AltarType()==72 && state.DaisType()==73 && participation.altarType==70 && build.daisType==71); // 이미 보관한 참조는 원래 owner의 수명 계약을 유지한다.
}
TEST_CASE(RuneTypeNumber_CurrentFinderAndDurationUseBoundNumbers) { // 실제 일반 finder/typed altar 조회와 시간 선택만 실행한다.
    RuneCurrentScene scene;RawGameAuxState owner;scene.state.sharedAltarType=&owner.AltarType();scene.state.sharedDaisType=&owner.DaisType();RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,{});
    CHECK(rune.FindOwnedDais(Sid{5})==Sid{6} && rune.ConstructionSeconds(Sid{5})==7);owner.DaisType()=70;CHECK(rune.FindOwnedDais(Sid{5})==Sid{});
    scene.slots[6][10]=70;scene.types[70].footX=scene.types[70].footY=1;scene.state.daisType=75;CHECK(rune.FindOwnedDais(Sid{5})==Sid{6});owner.AltarType()=71;CHECK(rune.AltarAt(Sid{5})==Sid{});
    scene.slots[7][10]=71;scene.types[71].footX=scene.types[71].footY=1;scene.state.playerFlags[1]=1;CHECK(rune.AltarAt(Sid{5})==Sid{7} && rune.ConstructionSeconds(Sid{5})==25);owner.AltarType()=188;CHECK(Throws([&] { rune.ConstructionSeconds(Sid{5}); }));
}
TEST_CASE(RuneTypeNumber_ReservationChangesCurrentNumbersBeforeFindAndDuration) { // 예약 실패가 돌아온 뒤의 현재 번호를 finder/시간 helper에 전달한다.
    RuneCurrentScene scene;RawGameAuxState owner;scene.state.sharedAltarType=&owner.AltarType();scene.state.sharedDaisType=&owner.DaisType();auto hooks=scene.Hooks();
    hooks.reserveRegular=[&] { scene.events.push_back("N");owner.DaisType()=70;owner.AltarType()=71;scene.slots[6][10]=70;scene.slots[7][10]=71;scene.types[70].footX=scene.types[70].footY=scene.types[71].footX=scene.types[71].footY=1;scene.state.playerFlags[1]=1;return false; };
    RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));rune.PostPop(Sid{5},3);CHECK(RuneEffects(scene.events)=="N;C25" && scene.book.depth==0); // 건설 hook의 인자 관찰이며 실제 건설/의식/프레임 틱은 실행하지 않는다.
}

TEST_CASE(RuneSharedCurrent_OwnerAndMissionModeRemainLiveAfterGeneralReset) { // Rune 번호의 장기 주소와 미션 억제의 독립 fallback을 실제 시간 조회로 구별한다.
    RuneCurrentScene scene;RawGameAuxState owner;bool mission=true;scene.state.sharedRuneType=&owner.RuneType();scene.state.sharedSuppressedDuration=&mission;scene.state.suppressDuration=false;const auto& current=scene.state.RuneType();CHECK(current==166 && &current==&owner.RuneType());owner.RuneType()=70;owner.ResetForGeneralWorld();CHECK(current==70);scene.state.runeType=71;CHECK(scene.state.RuneType()==70);scene.state.playerFlags[1]=1;RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,{});
    CHECK(rune.ConstructionSeconds(Sid{5})==10);mission=false;CHECK(rune.ConstructionSeconds(Sid{5})==25);scene.state.sharedSuppressedDuration=nullptr;scene.state.suppressDuration=true;CHECK(rune.ConstructionSeconds(Sid{5})==10);scene.state.sharedRuneType=nullptr;CHECK(scene.state.RuneType()==71 && current==70); // 이미 보관한 참조는 기존 owner를 계속 읽는다.
}
TEST_CASE(RuneSharedCurrent_ReservationChangesBoundMissionBeforeDurationRead) { // 예약 콜백 뒤 미션 상태를 다시 읽으며 의식/프레임/실제 건설은 실행하지 않는다.
    RuneCurrentScene scene;bool mission=true;scene.state.suppressDuration=true;scene.state.sharedSuppressedDuration=&mission;scene.state.playerFlags[1]=1;auto hooks=scene.Hooks();hooks.reserveRegular=[&] { scene.events.push_back("N");mission=false;return false; };RawRunePostPop rune(scene.pool,scene.hash,scene.types,scene.state,scene.base,std::move(hooks));rune.PostPop(Sid{5},3);CHECK(RuneEffects(scene.events)=="N;C25" && scene.book.depth==0);
}
