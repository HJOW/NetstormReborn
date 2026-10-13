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
