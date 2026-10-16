// 실제10.78 TYPE131 일반 피해와 기본 가상 방어를 새 독립 원본 행과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDamageableHit.h"
#include "o/RawSquidDefense.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 도구 KEYS의 순서이며 마지막 Group은 검사 분류다.
enum Column : std::size_t {
    Kind,Current,Damage,AttackerRank,VictimRank,AttackerOwner,VictimOwner,Disabled,NoDamage,
    Server,State,Extra,Reject,HasHP,Dais,AttackerGenus,Effect,Existing,Created,Checking,
    LocalOwner,IslandGenus,IslandId,Primary,Clock,Warning,Group,Output
};
// 실제 원본 관찰을 읽으며 제품 계산으로 기대값을 만들지 않는다.
const std::vector<std::vector<std::string>>& Rows() {
    static const auto rows=LoadFixture(NETSTORM_CAMPAIGN_DAMAGE_FIXTURE).rows;return rows;
}
// 실제 raw/HP/소유자/경고와 선언한 하위 효과를 같은 저장소에 준비한다.
struct CampaignDamageScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 동일 SID50/60/70 저장소다.
    std::vector<RiftTypeRecord> types{188}; // 원본 TYPE131/140/90 자료 입력이다.
    SquidPostPopState book; // 같은 최대HP 조회의 실제 목록 상태다.
    GameRandom random; // 분산 SP 저장소가 공유하는 실제 난수 객체다.
    ScrambledSpStore store{random,[] { return 1U; }}; // 현재 HP 분류가 받는 기존 공용 저장소다.
    SquidRewardState hpMode; // 원본 일반 최대HP와 사제 분류 모드다.
    PriestDamageState levels; // 공유 NoDamage/Challenge rank/보정 차단 상태다.
    DamageableHitState state{levels}; // 서버/소유자/주 섬/경고와 사망 문맥이다.
    DamageableBurningState burningState; // 현재 process14/진단 전역이다.
    std::array<SquidDefenseEntry,1> defense{{{0x50bad0,0x43d9b0}}}; // 보호 PE의 실제 TYPE131 기본 방어다.
    const std::vector<std::string>* input{}; // 현재 독립 원본 입력 행이다.
    std::vector<std::string> events; // 명시 하위 효과의 순서와 인자다.
    bool currentInputs{}; // 별도 검사는 낡은 backing 값을 두고 현재 getter로 같은 원본 입력을 읽는다.
    // 동일한 원본 번호까지 실제 슬롯을 한 번 확보한다.
    CampaignDamageScene() {
        // 모든 입력에서 source/attacker/ground 번호를 유지한다.
        for (std::uint16_t id=5;id<=70;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // free 입력도 다음 독립 행에서 재설정하는 검사 전용 raw 접근이다.
    std::span<std::uint8_t> Slot(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // wall clock의 독립 double 원시 비트를 그대로 읽는다.
    double D(Column column) const { return std::bit_cast<double>(std::stoull(input->at(column))); }
    // native의 실제 TYPE131/140/지면 상태와 raw padding을 동일하게 구성한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==Output+5);input=&row;events.clear();types.assign(types.size(),{});
        types[131].maxHitPoints=300;types[131].flags1=Number(row[HasHP]) ? 0x10U : 0U;types[131].flags2=Number(row[Dais]) ? 0x400000U : 0U;types[131].damageEffect=static_cast<std::int32_t>(Number(row[Effect]));types[140].flags2=Number(row[AttackerGenus]);types[90].flags2=Number(row[IslandGenus]);
        levels.invulnerable=Number(row[NoDamage])!=0;levels.levelCorrectionDisabled=Number(row[Disabled])!=0;levels.levels.fill(0);levels.levels[Number(row[AttackerOwner])]=static_cast<std::int32_t>(Number(row[AttackerRank]));levels.levels[Number(row[VictimOwner])]=static_cast<std::int32_t>(Number(row[VictimRank]));
        state.server=Number(row[Server])!=0;state.checking=Number(row[Checking])!=0;state.localOwner=static_cast<std::uint8_t>(Number(row[LocalOwner]));state.primaryIslands.fill(0);state.primaryIslands[Number(row[VictimOwner])]=static_cast<std::int32_t>(Number(row[Primary]));state.warningTime=D(Warning);state.killedVictim={};state.killerOwner=0;burningState.checking=state.checking;
        if (currentInputs) {
            state.serverRead=[this] { return Number(input->at(Server))!=0; };
            state.localOwnerRead=[this] { return static_cast<std::uint8_t>(Number(input->at(LocalOwner))); };
            state.primaryIslandRead=[this](std::uint8_t owner) { CHECK(owner==Number(input->at(VictimOwner)));return std::bit_cast<std::int32_t>(Number(input->at(Primary))); };
            levels.levelCorrectionDisabledRead=[this] { return Number(input->at(Disabled))!=0; };
            state.server=!state.server;state.localOwner=255;state.primaryIslands.fill(-123);levels.levelCorrectionDisabled=!levels.levelCorrectionDisabled;
        }
        auto raw=Slot(Sid{50});
        // 원본이 보존하는 padding까지 전체50BYTE로 대조한다.
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*17+3);
        Put(raw,0,0x50bad0);Put(raw,4,0,2);Put(raw,6,0,2);raw[10]=131;raw[11]=static_cast<std::uint8_t>(Number(row[State]));raw[40]=static_cast<std::uint8_t>(Number(row[Extra]));raw[34]=static_cast<std::uint8_t>(Number(row[VictimOwner]));Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));Put(raw,26,Number(row[Current]));Put(raw,36,0);
        auto attacker=Slot(Sid{60});std::fill(attacker.begin(),attacker.end(),std::uint8_t{});attacker[10]=140;auto ground=Slot(Sid{70});std::fill(ground.begin(),ground.end(),std::uint8_t{});ground[10]=90;Put(ground,8,Number(row[IslandId]),2);
    }
    // 실제 burning 숫자 몸체를 실행하며 외부 find/create/destroy만 관찰한다.
    DamageableBurningHooks Burning() {
        DamageableBurningHooks hooks;
        hooks.find=[this](Sid sid,std::uint32_t type,std::uint32_t index) {
            const Sid found{static_cast<std::uint16_t>(Number(input->at(Existing)) && !index ? 90U : 0U)};events.push_back("B:"+std::to_string(sid.value)+':'+std::to_string(type)+':'+std::to_string(index)+':'+std::to_string(found.value));return found;
        };
        hooks.create=[this](Sid sid) { events.push_back("A:"+std::to_string(sid.value));return Sid{static_cast<std::uint16_t>(Number(input->at(Created)) ? 90U : 0U)}; };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("X:"+std::to_string(sid.value)+':'+std::to_string(flags)); };return hooks;
    }
    // 실제 기본 방어와 일반 피해에 필요한 명시 하위 경계를 구성한다.
    DamageableHitHooks Hooks(const RawDamageableBurning& burning) {
        DamageableHitHooks hooks;
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("D:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.ownerDamageNotice=[this](std::uint8_t owner,Sid victim,Sid attacker) { events.push_back("N:"+std::to_string(owner)+':'+std::to_string(victim.value)+':'+std::to_string(attacker.value)); };
        hooks.unrealize=[this](Sid sid) { events.push_back("U:"+std::to_string(sid.value)); };
        hooks.islandAt=[this](float x,float y) { events.push_back("I:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y)));return Sid{70}; };
        hooks.realTime=[this] { events.push_back("W:"+input->at(Clock));return D(Clock); };
        hooks.warningEvent=[this] { events.push_back("V:12"); };
        hooks.warningSound=[this] { events.push_back("S:islandUnderAttack.wav"); };
        hooks.warningText=[this] { events.push_back("F:islandUnderAttack");events.push_back("L:islandUnderAttack"); };
        return MakeRawSquidDefenseHooks(pool,defense,MakeDamageableHitBurningHooks(pool,burning,std::move(hooks)));
    }
    // 같은 원본 피해/방어/HP/burning 계산을 실행하고 전체 관찰 결과를 직렬화한다.
    std::vector<std::string> Run() {
        SquidReward hp(pool,types,book,hpMode,&store);RawDamageableBurning burning(pool,types,hp,burningState,Burning());RawDamageableHit hit(pool,types,state,Hooks(burning));const bool result=Number(input->at(Kind)) ? hit.Hit(Sid{50},std::stoi(input->at(Damage)),Number(input->at(AttackerOwner)),Sid{60}) : hit.BaseHit(Sid{50},std::stoi(input->at(Damage)),Number(input->at(AttackerOwner)),Sid{60});std::string actions,slots;
        // 원본 외부 효과의 호출 순서를 그대로 합친다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        // victim/attacker/ground의 전체150BYTE를 같은 순서로 비교한다.
        for (const auto sid:{Sid{50},Sid{60},Sid{70}}) slots+=Hex(pool.Slot(sid));
        std::array<std::uint8_t,8> death{},warning{};Put(death,0,state.killedVictim.value);Put(death,4,state.killerOwner);const auto deadline=std::bit_cast<std::uint64_t>(state.warningTime);Put(warning,0,static_cast<std::uint32_t>(deadline));Put(warning,4,static_cast<std::uint32_t>(deadline>>32));return {result ? "1" : "0",actions,slots,Hex(death),Hex(warning)};
    }
};
// 분류별 실제 원본 행을 전체 바이트/반환/효과/전역으로 전수 대조한다.
void Replay(std::uint32_t group,std::size_t expectedCount,bool currentInputs=false) {
    CampaignDamageScene scene;scene.currentInputs=currentInputs;std::size_t count=0;int failures=0;
    // 해당 분류의 모든 고유 입력을 한 번씩만 실행한다.
    for (const auto& row:Rows()) {
        if (Number(row[Group])!=group) continue;scene.Prepare(row);const auto actual=scene.Run();const std::vector<std::string> expected(row.begin()+Output,row.end());CHECK(actual==expected);
        if (actual!=expected) {
            // 최초 다섯 차이를 원문 기대값과 함께 출력한다.
            for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("  CampaignDamage 분류%u 행%zu 결과%zu 기대%s / 실제%s\n",group,count,index,expected[index].c_str(),actual[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(count==expectedCount && failures==0);
}
}
// 일반 생존/NoDamage/서버 사망/clientHP1을 실제 기본 방어와 대조한다.
TEST_CASE(CampaignDamage_NativeHealthAndAuthority) { Replay(0,48); }
// 현재 Challenge rank 변환/보정 차단·중립·범위 밖 소유자의 원본 자연 분기를 대조한다.
TEST_CASE(CampaignDamage_NativeRankAndOwner) { Replay(1,32); }
// 주 섬/3x3/다리와 경고 마감 동률을 실제 wall clock 저장 순서로 대조한다.
TEST_CASE(CampaignDamage_NativeIslandWarning) { Replay(2,18); }
// held/free/unrealize·HP보유/burning 및 서버 삭제 flags를 대조한다.
TEST_CASE(CampaignDamage_NativeSpaceBurningAndDeathFlags) { Replay(3,19); }
// 낡은 초기값과 다른 현재 서버/로컬 번호/주 섬/미션 전역을 사용해 같은 원문117행에 일치해야 한다.
TEST_CASE(CampaignDamage_CurrentInputGetters) { Replay(0,48,true);Replay(1,32,true);Replay(2,18,true);Replay(3,19,true); }
