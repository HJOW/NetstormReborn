// 실제 두 PE 일반 피해의 full raw/현재 전역/순서와 공통HP/burning 소비자를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDamageableHit.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 복원 계산으로 기대값을 만들지 않고 저장한 원본 관찰만 읽는다.
const std::vector<std::vector<std::string>>& HitRows() { static const auto rows=LoadFixture(NETSTORM_DAMAGEABLEHIT_FIXTURE).rows;return rows; }
// 같은 raw/레벨/장부/HP/경고와 명시 실제 하위 경계를 갖는 독립 장면이다.
struct HitScene {
    // pool/types: 판본의 현재 SID/TYPE 저장소다.
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    // book/random/store/hpMode: 실제 최대HP 조회가 사용하는 기존 공용 객체다.
    SquidPostPopState book;
    GameRandom random;
    ScrambledSpStore store;
    SquidRewardState hpMode;
    // levels/state/burningState: 현재 실제 피해·권한·시계·burning 전역이다.
    PriestDamageState levels;
    DamageableHitState state{levels};
    DamageableBurningState burningState;
    // row/events/patch: 독립 입력과 현재 판본의 관찰 순서다.
    const std::vector<std::string>* row{};
    std::vector<std::string> events;
    bool patch;
    // 동일 원본 SID50/60/70까지 실제 풀에 확보한다.
    explicit HitScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),store(random,[] { return 1U; }),patch(edition==OriginalEdition::Patch1078) {
        // 각 독립 입력은 같은 원본 번호를 사용한다.
        for (std::uint16_t id=5;id<=70;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // free 입력의 다음 행 재준비에 한정한 검사 전용 raw 쓰기다.
    std::span<std::uint8_t> InputSlot(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 현재 double 비트를 판단 없이 입력으로 읽는다.
    double D(std::size_t index) const { return std::bit_cast<double>(std::stoull((*row)[index])); }
    // 현재 독립 raw/타입/상태를 native와 같은 저장 폭으로 초기화한다.
    void Prepare(const std::vector<std::string>& value) {
        CHECK(value.size()==32);row=&value;events.clear();types.assign(types.size(),{});types[71].maxHitPoints=300;types[71].flags1=Number(value[14]) ? 0x10U : 0U;types[71].flags2=Number(value[15]) ? 0x400000U : 0U;types[71].damageEffect=static_cast<std::int32_t>(Number(value[17]));types[128].flags2=Number(value[16]);types[90].flags2=Number(value[22]);levels.invulnerable=Number(value[9])!=0;levels.levelCorrectionDisabled=Number(value[8])!=0;levels.levels.fill(0);levels.levels[Number(value[6])]=static_cast<std::int32_t>(Number(value[4]));levels.levels[Number(value[7])]=static_cast<std::int32_t>(Number(value[5]));state.server=Number(value[10])!=0;state.checking=Number(value[20])!=0;state.localOwner=static_cast<std::uint8_t>(Number(value[21]));state.primaryIslands[Number(value[7])]=static_cast<std::int32_t>(Number(value[24]));state.warningTime=D(26);state.killedVictim={};state.killerOwner=0;burningState.checking=state.checking;
        auto raw=InputSlot(kSource);
        // 주변 padding 및 HP 옆 필드도 같은 native 독립 패턴을 유지한다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*17+3);
        Put(raw,0,0x11319800);Put(raw,4,0,2);Put(raw,6,0,2);raw[10]=71;raw[11]=static_cast<std::uint8_t>(Number(value[11]));raw[patch ? 40U : 35U]=static_cast<std::uint8_t>(Number(value[12]));raw[patch ? 34U : 32U]=static_cast<std::uint8_t>(Number(value[7]));Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));Put(raw,26,Number(value[2]),patch ? 4U : 2U);Put(raw,patch ? 36U : 34U,0,patch ? 4U : 1U);auto attacker=InputSlot(Sid{60});std::fill(attacker.begin(),attacker.end(),std::uint8_t{});attacker[10]=128;auto island=InputSlot(Sid{70});std::fill(island.begin(),island.end(),std::uint8_t{});island[10]=90;Put(island,8,Number(value[23]),2);
    }
    // 기존 burning 숫자 몸체에 실제 원본과 같은 관찰 find/create/destroy를 제공한다.
    DamageableBurningHooks Burning() {
        DamageableBurningHooks hooks;hooks.find=[this](Sid sid,std::uint32_t type,std::uint32_t index) { const Sid found{static_cast<std::uint16_t>(Number((*row)[18]) && !index ? 90U : 0U)};events.push_back("B:"+std::to_string(sid.value)+':'+std::to_string(type)+':'+std::to_string(index)+':'+std::to_string(found.value));return found; };hooks.create=[this](Sid sid) { events.push_back("A:"+std::to_string(sid.value));return Sid{static_cast<std::uint16_t>(Number((*row)[19]) ? 90U : 0U)}; };hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("X:"+std::to_string(sid.value)+':'+std::to_string(flags)); };return hooks;
    }
    // 명시 하위 경계 인자/순서를 관찰하며 상위 일반 피해 몸체는 실제 구현을 실행한다.
    DamageableHitHooks Hooks(const RawDamageableBurning& burning) {
        DamageableHitHooks hooks;hooks.rejectDamage=[this](Sid sid,Sid attacker) { events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(attacker.value));return Number((*row)[13])!=0; };hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("D:"+std::to_string(sid.value)+':'+std::to_string(flags)); };hooks.ownerDamageNotice=[this](std::uint8_t owner,Sid victim,Sid attacker) { events.push_back("N:"+std::to_string(owner)+':'+std::to_string(victim.value)+':'+std::to_string(attacker.value)); };hooks.unrealize=[this](Sid sid) { events.push_back("U:"+std::to_string(sid.value)); };hooks.islandAt=[this](float x,float y) { if (patch) events.push_back("I:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y)));return Sid{70}; };hooks.realTime=[this] { events.push_back("W:"+(*row)[25]);return D(25); };hooks.warningEvent=[this] { events.push_back("V:12"); };hooks.warningSound=[this] { events.push_back("S:islandUnderAttack.wav"); };hooks.warningText=[this] { events.push_back("F:islandUnderAttack");events.push_back("L:islandUnderAttack"); };return MakeDamageableHitBurningHooks(pool,burning,std::move(hooks));
    }
    // 실제 일반 피해와 공통HP/burning을 실행한 뒤 원본과 같은 최종 필드를 직렬화한다.
    std::vector<std::string> Run() {
        SquidReward hp(pool,types,book,hpMode,patch ? &store : nullptr);RawDamageableBurning burning(pool,types,hp,burningState,Burning());RawDamageableHit hit(pool,types,state,Hooks(burning));const bool result=Number((*row)[1]) ? hit.Hit(kSource,std::stoi((*row)[3]),Number((*row)[6]),Sid{60}) : hit.BaseHit(kSource,std::stoi((*row)[3]),Number((*row)[6]),Sid{60});std::string actions;
        // 원본이 관찰한 순서만 합치며 기대값을 계산하지 않는다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        std::array<std::uint8_t,8> death{},warning{};Put(death,0,state.killedVictim.value);Put(death,4,state.killerOwner);const auto deadline=std::bit_cast<std::uint64_t>(state.warningTime);Put(warning,0,static_cast<std::uint32_t>(deadline));Put(warning,4,static_cast<std::uint32_t>(deadline>>32));return {result ? "1" : "0",actions,Hex(pool.Slot(kSource)),Hex(death),Hex(warning)};
    }
};
// 현재 판본의 독립 PE 행을 실제 같은 공통 구현에서 전수 대조한다.
void ReplayGeneralHit(std::string_view edition) {
    HitScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0;
    // 각 저장된 고유 입력을 한 번씩만 대조한다.
    for (const auto& row:HitRows()) {
        if (row[0]!=edition) continue;scene.Prepare(row);const auto actual=scene.Run();const auto expected=std::vector<std::string>(row.begin()+27,row.end());const bool same=actual==expected;CHECK(same);
        if (!same) {
            // 최초 다섯 차이를 native/C++ 바이트와 인자로 함께 보고한다.
            for (std::size_t i=0;i<actual.size();++i) if (actual[i]!=expected[i]) std::printf("  %s 일반피해 행%zu/body%s 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,row[1].c_str(),i,expected[i].c_str(),actual[i].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==HitRows().size()/2);
}
}
// 패치의 실제 일반HP/피해/서버 사망/float 경고시각을 대조한다.
TEST_CASE(DamageableHit_PatchNativeBaseAndDerived) { ReplayGeneralHit("originals"); }
// CD10.72의 signedWORD HP와 별도 사망context/경고 순서를 대조한다.
TEST_CASE(DamageableHit_CdNativeBaseAndDerived) { ReplayGeneralHit("originalCD"); }
// 실제 삭제와 가상 방어가 누락되면 정상 완료로 바꾸지 않는다.
TEST_CASE(DamageableHit_MissingDefenseAndDeleteRejected) {
    HitScene scene(OriginalEdition::Patch1078);auto row=HitRows().front();row[1]="0";row[2]="1";row[3]="200";row[10]="1";row[11]="0";row[12]="0";row[13]="0";scene.Prepare(row);DamageableHitHooks hooks;RawDamageableHit noDefense(scene.pool,scene.types,scene.state,hooks);CHECK(Throws([&] { noDefense.BaseHit(kSource,200,2,Sid{60}); }));hooks.rejectDamage=[](Sid,Sid) { return false; };RawDamageableHit noDelete(scene.pool,scene.types,scene.state,std::move(hooks));CHECK(Throws([&] { noDelete.BaseHit(kSource,200,2,Sid{60}); }));CHECK(scene.state.killedVictim==Sid{});
}
// 실제 현재 raw vtable를 분배 자료와 대조하며 다른 파생 피해는 기존 callback을 보존한다.
TEST_CASE(DamageableHit_CurrentVtableDispatchAndFallback) {
    // 두 판본의 실제 BaseHit 진입주소를 독립 분배 자료로 공급한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        HitScene scene(edition);auto row=HitRows().front();row[1]="0";row[2]="150";row[3]="24";row[4]="0";row[5]="0";row[10]="1";row[11]="0";row[12]="0";row[13]="0";scene.Prepare(row);SquidReward hp(scene.pool,scene.types,scene.book,scene.hpMode,scene.patch ? &scene.store : nullptr);RawDamageableBurning burning(scene.pool,scene.types,hp,scene.burningState,scene.Burning());RawDamageableHit damage(scene.pool,scene.types,scene.state,scene.Hooks(burning));const std::array<DamageableHitEntry,1> entries{{{0x11319800,scene.patch ? 0x4add10U : 0x4aeda0U}}};std::uint32_t fallbackCalls=0;GunHitHooks fallback;
        fallback.damage=[&fallbackCalls](Sid victim,std::int32_t amount,std::uint8_t owner,Sid attacker) { CHECK(victim==kSource && amount==24 && owner==2 && attacker==Sid{60});++fallbackCalls; };auto hooks=MakeDamageableGunHitHooks(damage,entries,std::move(fallback));hooks.damage(kSource,24,2,Sid{60});CHECK(Get(scene.pool.Slot(kSource),26,scene.patch ? 4U : 2U)==126 && !fallbackCalls);Put(scene.pool.AllocatedBytes(kSource),0,0xdead1234);hooks.damage(kSource,24,2,Sid{60});CHECK(fallbackCalls==1 && Get(scene.pool.Slot(kSource),26,scene.patch ? 4U : 2U)==126);
    }
}
