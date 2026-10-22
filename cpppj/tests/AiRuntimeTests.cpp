// 원본 전체AI행동/개별Tick/Player순회와명시하위의효과순서를독립대조한다.
#include "o/RawAiRuntime.h"
#include "RawSceneSupport.h"
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

namespace {
// 원문16진수입력과출력을BYTE로복원한다.
std::vector<std::uint8_t> Bytes(std::string_view text) { std::vector<std::uint8_t> raw;for (std::size_t index=0;index<text.size();index+=2) raw.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));return raw; } //한쌍당한BYTE다.
// 본체/Player/명시효과입력만설치하고실제프레임의결과는제품이계산한다.
struct RuntimeScene {
    RawAiBrainMemory memory{0x10000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //실제본문의유일소유자다.
    std::array<std::uint32_t,2> tokens{memory.Allocate(RawAiBrainConstructor::kBytes),memory.Allocate(RawAiBrainConstructor::kBytes)}; //동일opaque값으로설치한실제본문두개다.
    std::vector<std::uint8_t> players; //실제9×180BYTE입력이다.
    std::vector<std::uint64_t> cfg; //원본실행기에공급한명시하위입력10개다.
    std::vector<std::string> trace; //도달한하위/인자/순서관측이다.
    std::uint32_t timing{},dryLog{99},active{tokens[0]},freed{}; //현재타이밍전역/로그종류/본문/수명관측이다.
    std::size_t walls{}; //실제벽시각호출의현재인덱스다.
    AiRuntimeHooks hooks; //선택된명시하위효과다.
    // 독립fixture의입력만복사하고기대본문/순서를설치하지않는다.
    explicit RuntimeScene(const std::vector<std::string>& row) {
        for (const auto& value:Split(row[2],',')) cfg.push_back(std::stoull(value)); //QWORD도손실없이읽는다.
        const auto bodies=Bytes(row[3]);players=Bytes(row[4]);for (std::size_t index=0;index<tokens.size();++index) std::copy_n(bodies.begin()+static_cast<std::ptrdiff_t>(index*RawAiBrainConstructor::kBytes),RawAiBrainConstructor::kBytes,memory.Bytes(tokens[index]).begin()); //같은실제본문순서다.
        timing=static_cast<std::uint32_t>(cfg[1]);Bind();
    }
    // 현재하위진입을기록하고현재token을후속시계/SP경계에유지한다.
    void Event(const char* name,std::uint32_t token) { active=token;trace.push_back(std::string(name)+","+std::to_string(token)); }
    // 원본도구와같은명시효과만공급한다.전체분기/비교/순회는제품이수행한다.
    void Bind() {
        hooks.initialize=[&](auto token) { Event("initialize",token);Put(memory.Bytes(token),0x258,static_cast<std::uint32_t>(cfg[2])); };
        hooks.prepare=[&](auto token) { Event("prepare",token);if (cfg[7]&1) Put(memory.Bytes(token),0x258,0); };
        hooks.selectGoal=[&](auto token) { Event("select",token); };
        hooks.collectors=[&](auto token) { Event("collectors",token);if (cfg[7]&2) Put(memory.Bytes(token),0x29c,37); };
        hooks.factories=[&](auto token) { Event("factories",token); };
        hooks.dryScan=[&](auto token) { Event("dry",token);if (cfg[7]&4) Put(memory.Bytes(token),0x280,static_cast<std::uint32_t>(cfg[3]));return static_cast<std::uint32_t>(cfg[3]); };
        hooks.dryNotice=[&](auto token,auto kind,bool fake) { trace.push_back("dryNotice,"+std::to_string(token)+","+std::to_string(kind)+","+(fake ? "1" : "0")); };
        hooks.priest=[&](auto token,auto priest) { Event("priest",token);trace.back()+=","+std::to_string(priest);if (cfg[7]&8) Put(memory.Bytes(token),0x260,3); };
        hooks.drops=[&](auto token) { Event("drops",token);if (cfg[7]&64) Put(memory.Bytes(token),0x254,1); };
        hooks.bridges=[&](auto token) { Event("bridges",token);if (cfg[7]&128) timing=1; };
        hooks.carriers=[&](auto token) { Event("carriers",token); };
        hooks.economy=[&](auto token) { Event("economy",token);if (cfg[7]&16) Put(memory.Bytes(token),0x2d8,static_cast<std::uint32_t>(cfg[8])); };
        hooks.money=[&](auto owner) { trace.push_back("money,"+std::to_string(owner));if (cfg[7]&32) Put(memory.Bytes(active),0x2d8,0);return std::bit_cast<float>(static_cast<std::uint32_t>(cfg[4])); };
        hooks.wall=[&] { trace.push_back("wall,"+std::to_string(walls));const auto value=cfg[walls%2==0 ? 5 : 6];++walls;return std::bit_cast<double>(value); };
        hooks.took=[&](auto token,double elapsed) { trace.push_back("took,"+std::to_string(token)+","+std::to_string(std::bit_cast<std::uint64_t>(elapsed))); };
        hooks.assertion=[&](auto,auto line) { trace.push_back("assert,"+std::to_string(line)); };
        hooks.destruct=[&](auto token) { trace.push_back("destruct,"+std::to_string(token)); };
        hooks.free=[&](auto token) { trace.push_back("free,"+std::to_string(token));for (std::size_t index=0;index<tokens.size();++index) if (tokens[index]==token) freed|=1U<<index;memory.Free(token); }; //실제소유블록을반납하고다시읽지않는다.
    }
    // 같은현재Player/본문과동적타이밍전역을전달한다.
    RawAiRuntime Make() { return {memory,players,{[&] { return cfg[0]!=0; },timing,dryLog},hooks}; }
    // 반납본문은부재0으로정규화하고해제메모리의바이트일치를주장하지않는다.
    std::string BodiesHex() { std::string text;for (const auto token:tokens) text+=memory.Contains(token) ? Hex(memory.Bytes(token)) : std::string(RawAiBrainConstructor::kBytes*2,'0');return text; } //원본관측과같은정규화다.
    // 도달순서를같은구분자로직렬화한다.
    std::string Trace() { std::string text;for (const auto& value:trace) { if (!text.empty()) text+=';';text+=value; }return text; } //첫항목앞구분자는없다.
};
// 기존원본기대를수정하지않는새관측만읽는다.
FixtureData Fixture() { const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/ai-runtime-x86.tsv";return LoadFixture(path.string().c_str()); }
}
// 세진입의본문/Player/float지갑/동적기한로그/현재필드/수명/순서전체를대조한다.
TEST_CASE(AiRuntime_Native145ActionsTickFrameAndLifetime) {
    std::size_t count{};
    for (const auto& row:Fixture().rows) { //원본caller입력만제품에공급하고같은현재저장소로재개한다.
        CHECK(row.size()==10);if (row.size()!=10) return;RuntimeScene scene(row);auto runtime=scene.Make();bool alive{};
        for (std::uint64_t repeat=0;repeat<scene.cfg[9];++repeat) { if (Number(row[1])==0) runtime.Actions(scene.tokens[0]);else if (Number(row[1])==1) alive=runtime.Tick(scene.tokens[0]);else runtime.Frame(); } //원본과같은진입/반복호출이다.
        const auto same=scene.BodiesHex()==row[5] && Hex(scene.players)==row[6] && scene.freed==Number(row[7]) && (Number(row[1])!=1 || alive==(Number(row[8])!=0)) && scene.Trace()==row[9];if (!same) std::printf("AI 전체 프레임 원본 차이 행%zu 수명%u/%s 순서%s/%s\n",count,scene.freed,row[7].c_str(),scene.Trace().c_str(),row[9].c_str());CHECK(same);++count;
    }
    CHECK(count==145);
}
// 254종료는초기화/시계/행동이없고현재Player본문소멸뒤반납/0쓰기만수행한다.
TEST_CASE(AiRuntime_TerminationKeepsPlayerFlagsAndSkipsAllActions) {
    const auto rows=Fixture().rows;RuntimeScene scene(rows[0]);scene.hooks={};Put(scene.memory.Bytes(scene.tokens[0]),0x254,1);Put(scene.memory.Bytes(scene.tokens[1]),0x254,1);const auto flag=scene.players[180+0x50];scene.hooks.destruct=[&](auto token) { scene.trace.push_back("D"+std::to_string(token)); };scene.hooks.free=[&](auto token) { scene.trace.push_back("F"+std::to_string(token));scene.memory.Free(token); };scene.Make().Frame();CHECK(scene.trace.size()==4 && !Get(scene.players,180+0xb0) && !Get(scene.players,8*180+0xb0) && scene.players[180+0x50]==flag && Get(scene.players,0xb0)==scene.tokens[0]);
}
