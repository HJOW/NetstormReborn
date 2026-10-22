// 실제 x86에서 동결한 플레어139입력의 전체 슬롯·난수·호출 순서·반환float를 대조한다.
#include "ConstructionSupport.h"
#include "o/RawFlare.h"
#include <filesystem>
using namespace netstorm::test::construction;
namespace {
// 원본과 같은112개 프레임 코드/16구간의 명시 입력이다.
RiftTypeFrames FlareFrames() {
    std::vector<FrameCode> codes;
    // 각 글자에7개의 같은 변형 프레임을 넣는다.
    for (std::uint8_t i=0;i<112;++i) codes.push_back({static_cast<std::uint8_t>('A'+i/7),'A',i,0});return RiftTypeFrames(std::move(codes));
}
// 원본의 독립 하위 입력을 공급하고 새 C++의 효과 인자만 기록한다.
struct FlareScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원본과 같은50바이트 슬롯과 client 범위다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 genus 입력이다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,FlareFrames()); // 실제 first/count와 코드 입력이다.
    GameRandom random;double suppressed{};std::uint32_t loading{},type{106}; // 동일 난수와 억제/타입 전역이다.
    std::int32_t kernel{};std::array<std::int16_t,2> hotspot{};std::uint32_t allocations{},reserved{},created{}; // 명시 Kernel/헤더/할당 입력과 경계 누계다.
    FlareHooks hooks;std::string trace; // 실제 호출 순서이며 기대 계산을 하지 않는다.
    // 각 하위는 원본 실행기에 공급한 명시 효과와 같은 필드만 변경한다.
    explicit FlareScene(const std::vector<std::uint32_t>& input,bool spawn) {
        // 본체가 직접 쓰는 대상은 실제 할당한 슬롯이다.
        for (std::uint16_t i=5;i<110;++i) CHECK(pool.Allocate(2).value==i);
        // 원본 관측의 부모와10개 생성 후보에 같은 보존 패턴을 넣는다.
        for (std::uint16_t sid=50;sid<110;++sid) {
            if (sid!=50 && sid<100) continue;auto raw=InputRaw(pool,Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x5096a0U);raw[10]=106;raw[11]=0;raw[34]=8;raw[35]=99;raw[40]=32;Put(raw,36,77);
        }
        auto parent=InputRaw(pool,Sid{50});
        if (spawn) {
            const auto bits=std::uint64_t(input[2])|(std::uint64_t(input[3])<<32);suppressed=std::bit_cast<double>(bits);loading=input[4];kernel=std::bit_cast<std::int32_t>(input[5]);types[85].flags2=input[6];parent[10]=85;Put(parent,14,input[7]);Put(parent,18,input[8]);parent[35]=static_cast<std::uint8_t>(input[9]);Put(parent,36,0);hotspot={static_cast<std::int16_t>(input[10]),static_cast<std::int16_t>(input[11])};random.SetState(input[12]);allocations=input[13];
        } else { kernel=std::bit_cast<std::int32_t>(input[3]);Put(parent,14,input[4]);Put(parent,18,input[5]);Put(parent,36,input[0]*7);random.SetState(input[6]);allocations=input[7]; }
        hooks.processCount=[this] { Event("K,"+std::to_string(kernel));return kernel; };
        hooks.create=[this](auto kind,auto flags) { ++created;const auto sid=99U+created;Add("C",kind,flags,sid);return Sid{static_cast<std::uint16_t>(sid)}; };
        hooks.sourceHotspot=[this](Sid parent) { CHECK(parent.value==50);Event("H");return hotspot; };
        hooks.owner=[this](Sid sid,auto owner) { Add("O",sid.value,owner);InputRaw(pool,sid)[34]=static_cast<std::uint8_t>(owner); };
        hooks.pop=[this](Sid sid,float x,float y,auto flags) { Add("P",sid.value,Bits(x),Bits(y),flags);Put(InputRaw(pool,sid),14,Bits(x));Put(InputRaw(pool,sid),18,Bits(y)); };
        hooks.reserveRegular=[this] { const auto success=(allocations>>reserved)&1U;++reserved;Add("R",success);return success!=0; };
        hooks.addRegular=[this](Sid sid,auto event,float payload) { Add("A",sid.value,event,Bits(payload)); };
        hooks.frame=[this](Sid sid,std::int32_t frame,auto flags) { Add("F",sid.value,std::bit_cast<std::uint32_t>(frame),flags);Put(InputRaw(pool,sid),36,std::bit_cast<std::uint32_t>(frame)); };
        hooks.update=[this](Sid sid) { Add("U",sid.value); };hooks.destroy=[this](Sid sid,auto flags) { Add("D",sid.value,flags); };
        hooks.sound=[this](float x,float y,std::string_view sound,auto flags) { CHECK(sound=="powerup.wav");Add("S",Bits(x),Bits(y),flags); };
    }
    // 한 효과의 인자는DWORD 비트를 부호 없이 출력한다.
    template<class... T> void Add(std::string label,T... values) { ((label+=","+std::to_string(static_cast<std::uint32_t>(values))),...);Event(std::move(label)); }
    // 각 실제 효과 사이에만 구분자를 넣는다.
    void Event(std::string label) { if (!trace.empty()) trace+=';';trace+=label; }
};
// 음수와64비트double 억제 입력을 원본 비트로 읽는다.
std::vector<std::uint32_t> FlareInputs(const std::string& text,bool spawn) {
    std::vector<std::uint32_t> result;std::size_t index{};
    // Spawn의double은 두DWORD로 나누어 테스트 입력에만 보관한다.
    for (const auto& word:Split(text,',')) { const auto value=word.front()=='-' ? static_cast<std::uint64_t>(std::stoll(word)) : std::stoull(word);result.push_back(static_cast<std::uint32_t>(value));if (spawn && index==2) result.push_back(static_cast<std::uint32_t>(value>>32));++index; }
    return result;
}
}
// 원본 동결139행의 본체/자식550바이트·난수·모든 효과·float 비트를 대조한다.
TEST_CASE(Flare_Native139) {
    std::size_t spawnCount{},regularCount{};
    // 기대값은 원본 TSV에만 있으며 새 몸체로 생성하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_FLARE_FIXTURE).rows) {
        const bool spawn=row[0]=="Spawn";const auto input=FlareInputs(row[1],spawn);FlareScene scene(input,spawn);RawFlare flare(scene.pool,scene.types,scene.frames,scene.random,{scene.suppressed,scene.loading,scene.type},scene.hooks);std::uint32_t result{};
        if (spawn) { flare.Spawn(Sid{50},std::bit_cast<std::int32_t>(input[0]),input[1]);++spawnCount; }
        else { result=Bits(flare.Handle(Sid{50},input[1],input[2],Float(input[8])));++regularCount; }
        std::string raw;
        // 원본이 관측한 같은 순서의 부모/10후보 슬롯 전체를 읽는다.
        for (std::uint16_t sid=50;sid<110;++sid) if (sid==50 || sid>=100) raw+=Hex(scene.pool.Slot(Sid{sid}));
        const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (result!=Number(row[2]) || scene.random.State()!=Number(row[3]) || raw!=row[4] || trace!=row[5]) std::printf("플레어 %s %s 불일치\n실제:%u/%u/%s\n원본:%s/%s/%s\n",row[0].c_str(),row[1].c_str(),result,scene.random.State(),trace.c_str(),row[2].c_str(),row[3].c_str(),row[5].c_str());
        CHECK(result==Number(row[2]));CHECK(scene.random.State()==Number(row[3]));CHECK(raw==row[4]);CHECK(trace==row[5]);
    }
    CHECK(spawnCount==30 && regularCount==109);
}
// C/D 방향은 별도 보호 원문으로 전체 슬롯·난수·예약 인자까지 비교한다.
TEST_CASE(Flare_BombStatusDirectionsNative60) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-status-flare-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 원본 입력30개를 두 방향에서 각각 실행한다.
        const auto input=FlareInputs(row[1],true);FlareScene scene(input,true);RawFlare flare(scene.pool,scene.types,scene.frames,scene.random,{scene.suppressed,scene.loading,scene.type},scene.hooks);flare.SpawnDirection({50},std::bit_cast<std::int32_t>(input[0]),input[1],106,Number(row[0]));std::string raw;
        for (const auto sid:std::array<std::uint16_t,11>{50,100,101,102,103,104,105,106,107,108,109}) raw+=Hex(scene.pool.Slot({sid})); // 방문하지 않은 주변 슬롯도 모두 비교한다.
        CHECK(Number(row[2])==0 && scene.random.State()==Number(row[3]) && raw==row[4] && (scene.trace.empty() ? "-" : scene.trace)==row[5]);++count;
    }
    CHECK(count==60);
}
