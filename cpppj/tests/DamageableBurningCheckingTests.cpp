// 두 판본 CPU의 연소 갱신/현재 전역/진단 복귀를 합성 입력으로 대조한다. 실제 공격은 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawDamageableBurning.h"
#include <bit>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 기대값은 새 CPU fixture에서만 읽으며 제품 출력으로 만들지 않는다.
// 같은 입력 슬롯/장치와 현재 owner를 한 갱신 동안 유지한다.
struct BurningCheckingScene {
    SidPool pool;std::vector<RiftTypeRecord> types; // 판본별 실제50/36BYTE와 TYPE 표다.
    SquidPostPopState book;GameRandom random;ScrambledSpStore store;SquidRewardState hpState; // 같은 최대HP 모듈의 장기 의존성이다.
    DamageableBurningState state;DamageableBurningHooks hooks; // 현재 checking/타입과 실제 선택 경계다.
    std::uint32_t checking{},first{},second{},created{},action{};std::int32_t maximum{}; // CPU에 공급한 현재 전역/검색 입력이다.
    std::string trace;bool patch; // 실제 호출 순서와 판본 선택이다.
    std::unique_ptr<SquidReward> hp;std::unique_ptr<RawDamageableBurning> body; // 참조 대상보다 먼저 소멸하는 제품 몸체다.
    explicit BurningCheckingScene(const std::vector<std::string>& row):pool(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,true),types(row[0]=="originals" ? 188 : 171),store(random,[] { return 1U; }),patch(row[0]=="originals") { // 결과를 계산하지 않고 명시 입력만 복제한다.
        for (std::uint16_t id=5;id<=50;++id) CHECK(pool.Allocate(2)==Sid{id}); // 동일 피해 SID50을 실제 할당한다.
        checking=Number(row[7]);first=Number(row[4]);second=Number(row[5]);created=Number(row[6]);action=Number(row[11]);maximum=std::stoi(row[3]);state.checking.Bind(checking);auto raw=pool.AllocatedBytes(Sid{50});
        for (std::size_t byte=0;byte<raw.size();++byte) raw[byte]=static_cast<std::uint8_t>(byte*7U+19U); // 예약 바이트도 CPU와 같다.
        Put(raw,0,0x61f000);raw[10]=static_cast<std::uint8_t>(Number(row[8]));raw[11]=static_cast<std::uint8_t>(Number(row[9]));Put(raw,14,0x3fa00000);Put(raw,18,0xc0200000);Put(raw,26,Number(row[2]),patch ? 4U : 2U);
        types[raw[10]].flags1=0x10;types[raw[10]].maxHitPoints=maximum;types[raw[10]].damageEffect=Number(row[10]);types[135].damageEffect=Number(row[10]);
        hp=std::make_unique<SquidReward>(pool,types,book,hpState,patch ? &store : nullptr);
        hooks.find=[this](Sid victim,std::uint32_t type,std::uint32_t index) { CHECK(victim==Sid{50} && index<=1);const auto result=index ? second : first;trace+="F,50,"+std::to_string(type)+','+std::to_string(index)+','+std::to_string(result)+';';if (!index && (action&1U)) { checking=0x80000000;state.burningType=58;Put(Raw(),26,0x7fffffff);maximum=0; }return Sid{static_cast<std::uint16_t>(result)}; };
        hooks.assertion=[this](auto expression,auto file,auto line) { trace+="D,"+std::to_string(line)+':'+std::string(expression)+':'+std::string(file)+';';if (action&2U) { checking=0;Raw()[10]=135;Put(Raw(),14,0x40700000);Put(Raw(),18,0x40900000);Put(Raw(),26,0x7fffffff);maximum=std::bit_cast<std::int32_t>(0x80000000U);state.bulfType=135;state.burningType=61; }if (action&4U) { Raw()[11]|=1;checking=0; }if (action&8U) first=first ? 0U : 90U; };
        hooks.create=[this](Sid victim) { CHECK(victim==Sid{50});trace+="C,50,"+std::to_string(created)+';';return Sid{static_cast<std::uint16_t>(created)}; };
        hooks.destroy=[this](Sid form,std::uint32_t flags) { trace+="X,"+std::to_string(form.value)+','+std::to_string(flags)+';'; };
        hooks.bulfSound=[this](Sid victim,float x,float y) { CHECK(victim==Sid{50});trace+="S,50,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+",bulfHurt.wav,0;"; };
        Rebind();
    }
    std::span<std::uint8_t> Raw() { const auto raw=pool.Slot(Sid{50});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free 입력에도 관측용 raw 바이트를 제공한다.
    void Rebind() { body=std::make_unique<RawDamageableBurning>(pool,types,*hp,state,hooks); } // 선택 장치를 바꾼 후 같은 전역/HP 몸체를 다시 연결한다.
    std::vector<std::string> Run() { const auto result=body->Refresh(Sid{50});return {std::to_string(result.value),Hex(Raw()),std::to_string(checking),std::to_string(state.burningType),std::to_string(state.bulfType),std::to_string(maximum),std::to_string(first),trace.empty() ? "-" : trace}; } // 반환/전체 raw/현재 owner/순서만 직렬화한다.
};
const auto& BurningCheckingRows() { static const auto rows=LoadFixture(NETSTORM_DAMAGEABLE_BURNING_CHECKING_FIXTURE).rows;return rows; } // 최초 CPU550행씩을 그대로 사용한다.
void ReplayBurningChecking(std::string_view edition) { // 판본별 진단 유무/HP 폭을 각각 비교한다.
    std::size_t count{};
    for (const auto& row:BurningCheckingRows()) { // 첫 불일치를 원문과 제품 필드로 나란히 남긴다.
        if (row[0]!=edition) continue;CHECK(row.size()==20);BurningCheckingScene scene(row);const auto actual=scene.Run();const auto expected=std::vector<std::string>(row.begin()+12,row.end());CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t i=0;i<actual.size();++i) if (actual[i]!=expected[i]) std::printf("연소 %s 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),row[1].c_str(),i,expected[i].c_str(),actual[i].c_str());return; }++count; // 기대 fixture는 바꾸지 않는다.
    }CHECK(count==550);
}
}
// 현재 DWORD/중복 진단·저장값/현재값·오버플로 절반 계산을 실제 패치 CPU와 비교한다.
TEST_CASE(DamageableBurningChecking_PatchNativeResumeAndCurrentOwner) { ReplayBurningChecking("originals"); }
// CD는 진단 없이 signed HP WORD와 같은 생성/삭제/위치음 순서를 따른다.
TEST_CASE(DamageableBurningChecking_CdNativeWordAndNoDiagnostic) { ReplayBurningChecking("originalCD"); }
// 누락된 선택 장치와 진단 중단은 이미 실행한 검색/위치음을 되돌리지 않는다.
TEST_CASE(DamageableBurningChecking_SelectedDevicesAndPartialEffects) {
    auto row=BurningCheckingRows().front();row[2]="49";row[3]="100";row[4]="0";row[5]="91";row[6]="92";row[7]="2147483648";row[8]="135";row[11]="0";
    BurningCheckingScene missing(row);missing.hooks.assertion={};missing.Rebind();CHECK(Throws([&] { missing.Run(); }));CHECK(missing.trace=="F,50,14,0,0;F,50,14,1,91;");
    BurningCheckingScene stopped(row);stopped.hooks.assertion=[&](auto,auto,auto) { stopped.checking=0;throw std::runtime_error("선택 진단 중단"); };stopped.Rebind();CHECK(Throws([&] { stopped.Run(); }));CHECK(!stopped.checking && stopped.trace==missing.trace);
    row[7]="0";BurningCheckingScene sound(row);sound.hooks.create={};sound.Rebind();CHECK(Throws([&] { sound.Run(); }));CHECK(sound.trace=="F,50,14,0,0;S,50,1067450368,3223322624,bulfHurt.wav,0;");
    row[9]="1";BurningCheckingScene free(row);free.hooks={};free.Rebind();CHECK(!free.body->Refresh(Sid{50}).value && free.trace.empty());
    row[9]="0";row[10]="0";BurningCheckingScene noEffect(row);noEffect.hooks={};noEffect.Rebind();CHECK(!noEffect.body->Refresh(Sid{50}).value && noEffect.trace.empty());
}
