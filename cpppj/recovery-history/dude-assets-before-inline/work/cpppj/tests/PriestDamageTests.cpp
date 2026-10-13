// 두PE의실제사제Hit+HPsetter+rank보정+burning 숫자몸체를독립관찰과대조한다.
#include "RawSceneSupport.h"
#include "o/RawPriestDamage.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 저장된원본관찰만읽으며복원구현으로기대값을생성하지않는다.
const std::vector<std::vector<std::string>>& DamageRows() { static const auto rows=LoadFixture(NETSTORM_PRIESTDAMAGE_FIXTURE).rows;return rows; }
// 현재풀/실제HPsetter와명시관찰경계의순서를공유한다.
struct DamageScene {
    // pool/types/frames: 원본피격자와타입/현재방향프레임의독립입력이다.
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    std::vector<RiftTypeFrames> frames;
    // random/store/book/hpMode/hp: 기존원본최대HP모듈의실제공유상태다.
    GameRandom random;
    ScrambledSpStore store;
    SquidPostPopState book;
    SquidRewardState hpMode;
    std::unique_ptr<SquidReward> hp;
    // spots/hpState: 기존사제HPsetter의현재공간자료와구현이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::unique_ptr<RawPriestState> hpState;
    // mode/damageState/burningState: 원본권한과현재플레이어rank/효과전역이다.
    PriestHitPointMode mode;
    PriestDamageState damageState;
    DamageableBurningState burningState;
    // row/events/output: 독립현재입력과외부경계순서/보정결과buffer다.
    const std::vector<std::string>* row{};
    std::vector<std::string> events;
    std::array<std::uint8_t,4> output{};
    // patch: 현재판본의raw저장폭을선택한다.
    bool patch;
    // 검사SID50만실제확보하고기존SP모듈은HP조회만수행한다.
    explicit DamageScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size(),RiftTypeFrames({FrameCode{65,0,0,0}})),store(random,[] { return 1U; }),patch(edition==OriginalEdition::Patch1078) {
        // 이후독립입력들이같은원본번호를사용한다.
        for (std::uint16_t id=5;id<=50;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 입력숫자를그대로현재타입/HP/rank/프레임에반영한다.
    void Prepare(const std::vector<std::string>& value) {
        CHECK(value.size()==23);row=&value;events.clear();types.assign(types.size(),{});const auto type=Number(value[17]);types[type].maxHitPoints=std::stoi(value[18]);types[type].flags1=0x69012;types[type].flags2=0x210000;types[type].damageEffect=std::stoi(value[13]);frames[type]=RiftTypeFrames({FrameCode{static_cast<std::uint8_t>(65+Number(value[12])),0,0,0}});
        hp=std::make_unique<SquidReward>(pool,types,book,hpMode,patch ? &store : nullptr);hpState=std::make_unique<RawPriestState>(pool,*hp,types,spots);mode.authority=Number(value[10])!=0;mode.recoveryAllowed.fill(true);damageState.invulnerable=Number(value[9])!=0;damageState.boss=mode.authority;damageState.levelCorrectionDisabled=Number(value[8])!=0;damageState.levels.fill(0);damageState.levels[Number(value[6])]=std::stoi(value[4]);damageState.levels[Number(value[7])]=std::stoi(value[5]);burningState.checking=Number(value[16])!=0;
        auto raw=pool.AllocatedBytes(kSource);
        // raw padding도native의독립입력패턴과동일하게준비한다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*17+3);
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);Put(raw,4,0,2);Put(raw,6,0,2);raw[10]=static_cast<std::uint8_t>(type);raw[11]=0;raw[patch ? 40U : 35U]=0;raw[patch ? 34U : 32U]=static_cast<std::uint8_t>(Number(value[7]));Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));Put(raw,26,static_cast<std::uint32_t>(std::stoi(value[2])),patch ? 4U : 2U);Put(raw,patch ? 36U : 34U,0,patch ? 4U : 1U);Put(output,0,static_cast<std::uint32_t>(std::stoi(value[3])));
    }
    // 원본가상Unpop/Repop 관찰시점의현재HP/좌표를기록한다.
    void Space(char marker,Sid sid,std::uint32_t flags) {
        const auto raw=pool.Slot(sid);events.push_back(std::string(1,marker)+':'+std::to_string(sid.value)+':'+std::to_string(flags)+':'+std::to_string(hpState->CurrentHitPoints(sid))+':'+std::to_string(Get(raw,14))+':'+std::to_string(Get(raw,18)));
    }
    // nonzero burning의명시조회/생성/삭제/소리경계다.실제파생생성완료로세지않는다.
    DamageableBurningHooks BurningHooks() {
        DamageableBurningHooks hooks;
        hooks.find=[this](Sid sid,std::uint32_t type,std::uint32_t index) { const Sid result{static_cast<std::uint16_t>(Number((*row)[14]) && !index ? 90U : 0U)};events.push_back("B:"+std::to_string(sid.value)+':'+std::to_string(type)+':'+std::to_string(index)+':'+std::to_string(result.value));return result; };
        hooks.create=[this](Sid sid) { events.push_back("A:"+std::to_string(sid.value));return Sid{static_cast<std::uint16_t>(Number((*row)[15]) ? 90U : 0U)}; };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("X:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.bulfSound=[this](Sid sid,float x,float y) { events.push_back("S:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))); };return hooks;
    }
    // native경계와동일한회복/프레임/소유자관찰및실제HPsetter/burning을연결한다.
    PriestDamageHooks PriestHooks(const RawDamageableBurning& burning) {
        PriestDamageHooks hooks;
        hooks.hasRegenEvent=[this](Sid) { return Number((*row)[11])!=0; };
        hooks.cancelEvent=[this](Sid sid,std::uint32_t event) { events.push_back("C:"+std::to_string(sid.value)+':'+std::to_string(event)); };
        hooks.defaultFrame=[](Sid) { return 0; };
        hooks.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) { events.push_back("F:"+std::to_string(sid.value)+':'+std::to_string(frame)+':'+std::to_string(flags));Put(pool.AllocatedBytes(sid),patch ? 36U : 34U,static_cast<std::uint32_t>(frame),patch ? 4U : 1U); };
        hooks.setHitPoints=[this](Sid sid,std::int32_t value) { hpState->SetHitPoints(sid,value,mode,{[this](Sid target,std::uint32_t flags) { Space('U',target,flags); },[this](Sid target,std::uint32_t flags) { Space('R',target,flags); }}); };
        hooks.ownerDamageNotice=[this](std::uint8_t owner,Sid sid,Sid attacker) { events.push_back("N:"+std::to_string(owner)+':'+std::to_string(sid.value)+':'+std::to_string(attacker.value)); };
        hooks.damageableRefresh=[&burning](Sid sid) { burning.Refresh(sid); };return hooks;
    }
    // 반환/raw 전체/관찰순서/보정buffer를원본과같은형식으로출력한다.
    std::vector<std::string> Run() {
        std::string result="-";RawDamageableBurning burning(pool,types,*hp,burningState,BurningHooks());RawPriestDamage priest(pool,types,frames,damageState,PriestHooks(burning));
        switch (Number((*row)[1])) {
        case 0:Put(output,0,static_cast<std::uint32_t>(RawPriestDamage::Adjust(std::stoi((*row)[3]),Number((*row)[6]),Number((*row)[7]),damageState,pool.Edition())));break;
        case 1:result=priest.Hit(kSource,std::stoi((*row)[3]),Number((*row)[6]),Sid{60}) ? "1" : "0";break;
        case 2:result=std::to_string(burning.Refresh(kSource).value);break;
        default:throw std::logic_error("PriestDamage fixture body 오류");
        }
        std::string actions;
        // 실제원본에서관찰한경계순서만직렬화한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        return {result,actions,Hex(pool.Slot(kSource)),Hex(output)};
    }
};
// 각판본의고유원본입력을한번씩재생한다.
void ReplayDamage(std::string_view edition) {
    DamageScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0;
    // 기대값은저장된native관찰만사용한다.
    for (const auto& row:DamageRows()) {
        if (row[0]!=edition) continue;scene.Prepare(row);const auto output=scene.Run();const auto expected=std::vector<std::string>(row.begin()+19,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초차이를원본관찰과복원출력으로직접보인다.
            for (std::size_t i=0;i<output.size();++i) if (output[i]!=expected[i]) std::printf("  %s PriestDamage 행%zu/body%s 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,row[1].c_str(),i,expected[i].c_str(),output[i].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==DamageRows().size()/2);CHECK(!scene.store.Initialized());
}
}
// 패치실제rank24칸변환/20기준피해와사제Hit·HPsetter·burning을대조한다.
TEST_CASE(PriestDamage_PatchNativeHitSetterRankAndBurning) { ReplayDamage("originals"); }
// CD10.72의BYTE level/8기준피해와signedWORD HP·실제Hit를대조한다.
TEST_CASE(PriestDamage_CdNativeHitSetterRankAndBurning) { ReplayDamage("originalCD"); }
// 자연zero 경로는성공하며nonzero burning의누락생성몸체는명시적으로거부한다.
TEST_CASE(PriestDamage_MissingNonzeroBurningRejected) {
    DamageScene scene(OriginalEdition::Patch1078);auto row=DamageRows().front();row[1]="2";row[2]="99";row[13]="0";scene.Prepare(row);RawDamageableBurning zero(scene.pool,scene.types,*scene.hp,scene.burningState,{});CHECK(zero.Refresh(kSource)==Sid{});
    row[13]="1";scene.Prepare(row);RawDamageableBurning missing(scene.pool,scene.types,*scene.hp,scene.burningState,{});CHECK(Throws([&] { missing.Refresh(kSource); }));
}
// 실제Regular Form/Kernel취소와HP1사제의두감시쌍통지/삭제를같은풀에서연결한다.
// 공간/frame/소유자통지는명시관찰경계이며실제사제Pop/GUI 완료로세지않는다.
TEST_CASE(PriestDamage_ActualRegularKernelAndDependChain) {
    // 두판본의실제프로세스부착과의존쌍삭제폭을모두사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(patch ? 188 : 171);types[158].constructorAddress=TypeConstructorAddress(edition,158);types[158].maxHitPoints=200;types[158].flags1=0x69012;types[158].flags2=0x210000;std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({FrameCode{65,0,0,0}}));SquidFactory factory(pool,types,false);const auto victim=factory.Create(158,2),firstWatcher=factory.Create(90,2),secondWatcher=factory.Create(91,2);
        // 아직공간Pop을주장하지않는독립live 입력으로사제피격몸체를준비한다.
        auto raw=pool.AllocatedBytes(victim);raw[11]=0;raw[patch ? 34U : 32U]=1;Put(raw,26,150,patch ? 4U : 2U);Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));Put(raw,patch ? 36U : 34U,0,patch ? 4U : 1U);
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState clock;SquidProcessHost host(pool,types,kernel,destroy,clock);
        const auto* first=host.AddRegular(victim,0x25c,0.1f);const auto* second=host.AddRegular(victim,0x25d,0.2f);CHECK(first && second && kernel.Size()==2);const auto firstForm=first->Form(),secondForm=second->Form();
        std::uint32_t notices=0;DependFormHost depends(pool,destroy,[&](Sid watcher,Sid watched,Sid) { CHECK((watcher==firstWatcher || watcher==secondWatcher) && watched==victim);++notices; });const auto firstPair=depends.Create(victim,firstWatcher),secondPair=depends.Create(victim,secondWatcher);CHECK(depends.Has(victim,firstWatcher) && depends.Has(victim,secondWatcher));
        GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidPostPopState book;SquidRewardState hpMode;SquidReward hp(pool,types,book,hpMode,patch ? &store : nullptr);RawPriestState hpState(pool,hp,types,spots);PriestHitPointMode mode;mode.recoveryAllowed.fill(true);DamageableBurningState burningState;RawDamageableBurning burning(pool,types,hp,burningState,{});PriestDamageState state;std::uint32_t framesChanged=0,ownerNotices=0,spaces=0;
        PriestDamageHooks hooks;hooks.defaultFrame=[](Sid) { return 0; };hooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) { CHECK(sid==victim && frame==0 && flags==0);++framesChanged; };hooks.ownerDamageNotice=[&](std::uint8_t owner,Sid sid,Sid attacker) { CHECK(owner==1 && sid==victim && attacker==firstWatcher);++ownerNotices; };
        hooks=MakePriestDamageWorldHooks(pool,host,hpState,mode,{[&](Sid sid,std::uint32_t flags) { CHECK(sid==victim && flags==0);++spaces; },[&](Sid sid,std::uint32_t flags) { CHECK(sid==victim && flags==0);++spaces; }},burning,std::move(hooks));RawPriestDamage damage(pool,types,frames,state,std::move(hooks),&depends);
        auto gun=MakePriestGunHitHooks(pool,damage,{});gun.damage(victim,200,2,firstWatcher);
        CHECK(hpState.CurrentHitPoints(victim)==1 && kernel.Size()==0 && framesChanged==1 && ownerNotices==1 && spaces==2 && notices==2);CHECK(!depends.Has(victim,firstWatcher) && !depends.Has(victim,secondWatcher));CHECK(Get(pool.Slot(victim),6,2)==0);
        // 실제쌍삭제와Regular삭제가각자의양판본SID수명을끝내는지확인한다.
        for (const auto sid:{firstForm,secondForm,firstPair.watchedForm,firstPair.watcherForm,secondPair.watchedForm,secondPair.watcherForm}) CHECK((pool.Slot(sid)[11]&3)==3);
        CHECK(destroy.PreDepth()==0 && destroy.PostDepth()==0 && !store.Initialized());
    }
}
