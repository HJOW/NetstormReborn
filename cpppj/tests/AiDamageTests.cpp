// 실제 AI 피해 반응149입력의 전체 본체·네 목록·효과 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawAiDamage.h"
#include <set>
#include <tuple>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kDamageBrain=0x13000000; // native와 같은 실제 등록부의 첫 opaque32 핸들이다.
constexpr std::array<std::size_t,4> kDamageLists{0x360,0x36c,0x378,0x384}; // 네 실제 목록 offset이다.
// 독립 입력으로 실제 memory/Pool을 준비한다. 결과 필드는 fixture에서만 읽는다.
struct DamageScene {
    std::vector<std::uint32_t> input; // native가 받은17개 독립 정수다.
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // Session과 같은 실제 풀 크기이며 예측 영역23001을 포함한다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 TYPE genus/group 입력이다.
    std::unique_ptr<RawAiBrainMemory> memory; // 실제 본체·네 backing 블록의 소유자다.
    std::unique_ptr<RawAiDamage> damage; // 같은 memory/Pool의 실제 피해 처리기다.
    std::vector<std::uint32_t> blocks; // 실제 Allocate가 반환한 네 목록 token이다.
    std::vector<std::uint16_t> slots; // native가 출력한 모든 입력 슬롯 순서다.
    std::string trace; // 실제 home/log 요청 순서다.
    std::size_t allocation{}; // 각 미초기화 입력 패턴의 독립 번호다.
    // 현재 경계가 실제 호출될 때만 효과를 기록한다.
    void Event(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 실제 raw 슬롯의 입력 바이트만 준비한다. 할당·생성 성공을 합성하지 않는다.
    void Slot(std::uint16_t sid,std::uint8_t type,std::uint8_t state,float x,float y) {
        const auto view=pool.Slot(Sid{sid});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(view.data()),view.size()};
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*3+sid); // 전체 입력 padding 패턴이다.
        raw[10]=type;raw[11]=state;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));slots.push_back(sid);
    }
    // 입력 준비 뒤 같은 실제 등록부에 모듈을 연결한다.
    explicit DamageScene(std::string_view values) {
        for (const auto& value:Split(values,',')) input.push_back(Number(value)); // 원본 입력 DWORD 비트를 유지한다.
        CHECK(input.size()==17);
        memory=std::make_unique<RawAiBrainMemory>(kDamageBrain,[this](std::span<std::uint8_t> bytes) {
            for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>(allocation ? i*7+input[15]+(allocation-1)*17 : i*13+input[15]); // 실제 할당 직후의 독립 입력이다.
            ++allocation;
        });CHECK(memory->Allocate(RawAiBrainConstructor::kBytes)==kDamageBrain);
        for (std::size_t index=0;index<kDamageLists.size();++index) {
            const auto block=memory->Allocate(32);blocks.push_back(block);const auto offset=kDamageLists[index];const auto cap=index==1 ? input[5] : index==2 ? input[8] : 4U;const auto count=index==1 ? input[6] : index==2 ? input[9] : index==0 ? input[3] : input[2]; // 실제 초기 pointer/capacity/count 입력이다.
            auto raw=memory->Bytes(kDamageBrain);Put(raw,offset,block);Put(raw,offset+4,cap);Put(raw,offset+8,count);
            for (std::uint32_t item=0;item<count;++item) Put(memory->Bytes(block),item*4,index==0 || index==3 ? 50U : item==0 && input[index==1 ? 7 : 10] ? 60U : 200U+static_cast<std::uint32_t>(index)*10U+item); // native와 같은 초기 목록 내용이다.
        }
        Put(memory->Bytes(kDamageBrain),0x248,input[4]);Put(memory->Bytes(kDamageBrain),0x404,0x11223344);
        const auto victimType=static_cast<std::uint8_t>(input[1] ? 164 : 71);types[victimType].flags2=input[0];types[140].flags2=input[11] ? 0x100000U : 0U;types[73].group=input[14];
        Slot(50,victimType,0,5.5F,7.5F);Slot(60,140,0,-4.75F,9.25F);if (input[12]>0 && input[12]<24000) Slot(static_cast<std::uint16_t>(input[12]),73,static_cast<std::uint8_t>(input[13]),12.75F,-3.5F);std::sort(slots.begin(),slots.end());
        AiDamageHooks hooks;hooks.home=[this](Sid attacker) { Event("H:"+std::to_string(attacker.value));return Sid{static_cast<std::uint16_t>(input[12])}; };
        hooks.log=[this](std::uint32_t token,const AiDamageLog& log) { CHECK(token==kDamageBrain);Event("L:"+std::to_string(log.more ? 1 : 0)+':'+std::to_string(log.attacker.value)+':'+std::to_string(log.type)+':'+std::to_string(log.x)+':'+std::to_string(log.y)); };
        damage=std::make_unique<RawAiDamage>(*memory,pool,types,std::move(hooks));
    }
    // 네 실제 backing 블록의 보존 바이트도 모두 직렬화한다.
    std::string Lists() const { std::string result;for (const auto token:blocks) result+=Hex(memory->Bytes(token));return result; }
    // 입력 슬롯이 피해 반응으로 변하지 않았는지 전체 폭을 비교한다.
    std::string Slots() const { std::string result;for (const auto sid:slots) result+=Hex(pool.Slot(Sid{sid}));return result; }
};
}
// 모든 기대값은 두 정밀도로 일치한 실제10.78 기계어 관찰이다.
TEST_CASE(AiDamage_Native149) {
    std::size_t count=0;std::set<std::string> unique;
    for (const auto& row:LoadFixture(NETSTORM_AI_DAMAGE_FIXTURE).rows) {
        if (row[0]=="input") continue;CHECK(row.size()==5 && unique.insert(row[0]).second);DamageScene scene(row[0]);scene.damage->Notice(kDamageBrain,Sid{50},Sid{static_cast<std::uint16_t>(scene.input[16])});CHECK(Hex(scene.memory->Bytes(kDamageBrain))==row[1]);CHECK(scene.Lists()==row[2]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[3]);CHECK(scene.Slots()==row[4]);CHECK(scene.memory->Allocations().size()==5 && scene.memory->Frees().empty());++count;
    }
    CHECK(count==149);
}
// 미연결 실제 효과를 빈 성공으로 바꾸지 않고 요청 시점에 거부한다.
TEST_CASE(AiDamage_RejectsMissingSelectedEffects) {
    DamageScene scene("512,0,0,0,2,4,0,0,4,0,0,1,100,0,3,19,60");RawAiDamage missing(*scene.memory,scene.pool,scene.types,{});CHECK(Throws([&] { missing.Notice(kDamageBrain,Sid{50},Sid{60}); }));
    CHECK(Get(scene.memory->Bytes(kDamageBrain),0x374)==1);CHECK(Get(scene.memory->Bytes(scene.blocks[1]),0)==60);CHECK(Get(scene.memory->Bytes(kDamageBrain),0x380)==0);
    CHECK(Throws([&] { missing.Notice(kDamageBrain+1,Sid{50},Sid{60}); }));
}
