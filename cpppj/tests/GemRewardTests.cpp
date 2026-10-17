// 독립원문의 보상·변환·Deck 현재 필드와 실제 효과 경계를 재생한다.
#include "RawSceneSupport.h"
#include "o/RawGemReward.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원본 입력DWORD의 전체 비트를 읽는다.
std::vector<std::uint32_t> GemNumbers(std::string_view text) {
    std::vector<std::uint32_t> values; // 고정14DWORD 입력이다.
    // 입력 순서대로 변환하며 기대 결과는 계산하지 않는다.
    for (const auto& part:Split(text,',')) values.push_back(Number(part));return values;
}
// 큰 장치·진단·생성 효과 입력만 공급한다.
struct GemScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 실제50바이트 물리 슬롯이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재TYPE 입력이다.
    GameRandom random; // 원본전역난수00532710이다.
    SquidPostPopList excluded; // 원문의 현재BYTE 제외 항목이다.
    GemRewardState state; // 현재TYPE개수와buriedType이다.
    std::array<std::uint8_t,kProductionDeckBytes> deck; // 직접Draw의 실제352바이트다.
    std::span<std::uint8_t> currentDeck; // 벽시각/로그 효과가 실행되는 동안 살아 있는 현재Deck만 가리킨다.
    std::vector<std::uint32_t> row; // 원문 입력14DWORD다.
    std::string trace; // 실제 호출 순서·인자다.
    // 테스트 초기 물리바이트는 할당/free 여부와 독립이다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto bytes=pool.Slot(sid);return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; }
    // 실제 효과 인자만 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 실제 DWORD 폭을 보존한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 독립 원문과 동일한 초기 입력만 준비한다.
    void Prepare(std::vector<std::uint32_t> input) {
        row=std::move(input);trace.clear();types.assign(188,{});state.typeCount=row[1]==5 ? 73 : 188;state.buriedType=112;random.SetState(row[3]);excluded.entries.clear();excluded.count=0;deck.fill(0xa5);currentDeck=deck;
        // 현재6타입의 수준과genus를 명시 입력으로 둔다.
        const std::array<std::int32_t,6> levels{0,1,2,3,98,99};
        for (std::size_t i=0;i<levels.size();++i) { types[70+i].flags2=row[1]==1 ? 0U : 0x100U;types[70+i].level=levels[i];if (row[2]&(1U<<i)) excluded.entries.push_back(static_cast<std::uint32_t>(70+i)); }
        if (row[2]&64U) excluded.entries.push_back(0);excluded.count=static_cast<std::uint32_t>(excluded.entries.size());
        if (row[1]==2) types[70].level=-1;if (row[1]==3) { types[0].flags2=0x100;types[0].level=3; }if (row[1]==4) types[70].flags2=0x2100;
        // 원문과 같은 두 전체 물리슬롯을 입력한다.
        for (const auto sid:{Sid{50},Sid{61}}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(sid==Sid{50} ? row[7] : 70);raw[11]=static_cast<std::uint8_t>(sid==Sid{50} ? row[8] : 0);Put(raw,12,row[6],2);Put(raw,14,std::bit_cast<std::uint32_t>(98.0F));Put(raw,18,std::bit_cast<std::uint32_t>(122.0F)); }
        if (row[0]!=2) return;
        std::vector<std::array<std::uint8_t,5>> entries{{70,20,1,255,254},{71,30,1,255,0},{74,50,1,255,255}}; // 원문의 직접Deck 초기 입력이다.
        if (row[10]==1) entries[0]={70,20,0,255,0};if (row[10]==2) entries[0]={70,20,255,255,0};if (row[10]==3) entries.clear();if (row[10]==4) entries={{70,0,1,255,0},{71,0,1,255,0}};if (row[10]==5) entries={{70,255,127,255,255}};if (row[10]==6) entries={{70,20,128,255,0},{71,30,1,255,0}};
        std::uint32_t sum{}; // 원문에 제공한 초기합계다. 실제 재계산 결과는 주입하지 않는다.
        // 각 초기 항목의5바이트만 쓴다.
        for (std::size_t i=0;i<entries.size();++i) { std::copy(entries[i].begin(),entries[i].end(),deck.begin()+static_cast<std::ptrdiff_t>(i*5));sum+=entries[i][1]; }
        Put(deck,0x14c,row[10]==7 ? 0xffffffffU : static_cast<std::uint32_t>(entries.size()));Put(deck,0x150,row[10]==1 || row[10]==2 ? 999U : sum);Put(deck,0x154,0xffffffff);Put(deck,0x158,row[11]);Put(deck,0x15c,row[12]);
    }
    // 선택한 진단이 돌려준 뒤 원본과 동일한 명시 변경만 수행한다.
    void Assertion(std::uint32_t line) {
        Event("I",{line});if (row[9]==1 && line==0xf6) { state.typeCount=72;types[71].level=3; }if (row[9]==6 && line==0x10a) { Raw(Sid{50})[10]=112;Put(Raw(Sid{50}),12,70,2); }
    }
    // 원문의 실제 시각/진단/로그·optional power 분류 경계다.
    DeckDrawHooks DrawHooks() {
        DeckDrawHooks hooks;hooks.wallSeconds=[this] { Event("W",{row[4],row[5]});if (row[9]==3) currentDeck[2]=0;return std::bit_cast<double>((std::uint64_t(row[5])<<32)|row[4]); };
        hooks.assertion=[this](std::uint32_t line) { Assertion(line); };
        hooks.mismatch=[this](std::uint32_t stored,std::uint32_t sum) { Event("M",{stored,sum});if (row[9]==2) { Put(currentDeck,0x14c,0);Put(currentDeck,0x150,999); } };
        hooks.isProduct=[](std::uint32_t type) { return type>=70 && type<=73; };return hooks;
    }
    // 실제 계산은 제품이 수행하고 큰 게임 호출의 원문 입력만 제공한다.
    GemRewardHooks Hooks() {
        GemRewardHooks hooks;hooks.fill=[this](std::span<std::uint8_t> raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});currentDeck=raw; };hooks.deck=DrawHooks();hooks.assertion=[this](std::uint32_t line) { Assertion(line); };
        hooks.deckAssertion=[](ProductionDeckAssertion) { throw std::logic_error("보상 원문에서 선택하지 않은 항목진단"); };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { Event("C",{type,flags});Raw(Sid{61})[10]=static_cast<std::uint8_t>(type);if (row[9]==4) { Put(Raw(Sid{50}),14,std::bit_cast<std::uint32_t>(101.5F));Put(Raw(Sid{50}),18,std::bit_cast<std::uint32_t>(-4.25F)); }return Sid{61}; };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("P",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags});if (row[9]==5) Put(Raw(Sid{50}),0,0x12200100); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { Event(Get(Raw(sid),0)==0x12200000 ? "D" : "E",{sid.value,flags});Raw(sid)[11]=1; };return hooks;
    }
};
}
// 독립 원문의1250입력과 전체 반환·효과·난수·객체·직접Deck을 대조한다.
TEST_CASE(GemReward_NativeBodies) {
    GemScene scene;std::size_t count{},faults{},diagnostics{}; // 대조한 입력과 실제범위실패 수다.
    // 기대값은 fixture에서만 읽으며 제품 분기와 독립이다.
    for (const auto& fields:LoadFixture(NETSTORM_GEM_REWARD_FIXTURE).rows) {
        CHECK(fields.size()==15);if (fields.size()!=15) continue;scene.Prepare(GemNumbers(fields[0]));RawGemReward gem(scene.pool,scene.types,scene.random,scene.excluded,scene.state,scene.Hooks());std::string result;std::int32_t power=0x55555555;
        try { result=std::to_string(scene.row[0]==0 ? gem.Choose() : scene.row[0]==1 ? gem.Reveal(Sid{50}).value : RawDeckDraw(scene.deck,scene.random,scene.DrawHooks()).Draw(scene.row[13] ? &power : nullptr)); }
        catch (const std::out_of_range&) { CHECK(scene.row[0]==1 && scene.row[6]==65535);result="fault";++faults; }
        const auto trace=scene.trace.empty() ? "-" : scene.trace,raws=Hex(scene.Raw(Sid{50}))+","+Hex(scene.Raw(Sid{61}));const auto random=std::to_string(scene.random.State()),powerText=std::to_string(std::bit_cast<std::uint32_t>(power)),typeCount=std::to_string(static_cast<std::uint32_t>(scene.state.typeCount));
        if ((result!=fields[1] || trace!=fields[2] || random!=fields[3] || raws!=fields[5] || powerText!=fields[6] || typeCount!=fields[7] || (scene.row[0]==2 && Hex(scene.deck)!=fields[4])) && diagnostics++<5) std::cout<<"보상입력 "<<fields[0]<<"\n실제 "<<result<<' '<<trace<<' '<<random<<' '<<raws<<' '<<powerText<<' '<<typeCount<<"\n원본 "<<fields[1]<<' '<<fields[2]<<' '<<fields[3]<<' '<<fields[5]<<' '<<fields[6]<<' '<<fields[7]<<'\n';
        CHECK(result==fields[1]);CHECK(trace==fields[2]);CHECK(random==fields[3]);CHECK(raws==fields[5]);CHECK(powerText==fields[6]);CHECK(typeCount==fields[7]);if (scene.row[0]==2) CHECK(Hex(scene.deck)==fields[4]);
        // 두정밀도 원문은 모든 관측 열에서 같아야 한다.
        for (std::size_t i=1;i<=7;++i) CHECK(fields[i]==fields[i+7]);++count;
    }
    CHECK(count==1250 && faults==32);
}
