// 네 원본 전략의 실제 전체 본체/목록/난수/하위 호출 순서를 독립 대조한다.
#include "o/RawAiBridgeStrategies.h"
#include "o/RawAiBridgePlanning.h"
#include "o/RawSquidNeighbors.h"
#include "RawSceneSupport.h"
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

namespace {
// 원문16진수의caller입력/독립기대를BYTE로복원한다.
std::vector<std::uint8_t> Bytes(std::string_view text) { std::vector<std::uint8_t> result;for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));return result; } //한쌍마다한BYTE를추가한다.
// 현재두float를호출순서비교용원문DWORD로나타낸다.
std::string XY(RawPathPoint point) { return std::to_string(std::bit_cast<std::uint32_t>(point.x))+","+std::to_string(std::bit_cast<std::uint32_t>(point.y)); }
// 명시하위입력과원본본문/목록/슬롯만설치한다.전략결과를미리만들지않는다.
struct StrategyScene {
    RawAiBrainMemory memory{0x10000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd}); }}; //원본시험과같은opaque값의실제소유블록이다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; //실제0xb08본체다.
    std::array<std::uint32_t,4> lists{}; //actor/연결/미연결/포기목록의실제소유핸들이다.
    SidPool pool{OriginalEdition::Patch1078,32768,false}; //실제50BYTE풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //원본fixture의실제genus/발자국입력이다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); //중심계산과계획모듈이공유할현재코드표다.
    SquidHash hash; //중심계산은같은실제현재해시를빌린다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); //같은실제지도입력이다.
    GameRandom random; //모든실제난수계산이공유하는상태다.
    double now{}; //현재QWORD게임시각이다.
    std::int32_t mapLimit{256}; //현재유효좌표상한이다.
    std::uint32_t logKind{29}; //현재간헐천로그종류다.
    std::vector<std::uint64_t> cfg; //관측도구가주입한명시하위반환/전역입력14개다.
    std::vector<std::string> trace; //도달한실제하위의순서/인자만관측한다.
    AiBridgeStrategyHooks hooks; //현재명시효과경계이며계산의기대값을반환하지않는다.
    // 독립fixture행의입력바이트와현재하위입력을구성한다.
    explicit StrategyScene(const std::vector<std::string>& row) {
        for (auto& list:lists) list=memory.Allocate(64); //네actual소유목록을원본입력과같은핸들순서로만든다.
        for (int index=5;index<=55;++index) CHECK(pool.Allocate(2).value==index); //명시슬롯50..55를실제풀에확보한다.
        for (const auto& value:Split(row[1],',')) cfg.push_back(std::stoull(value)); //현재QWORD도손실없이보존한다.
        const auto body=Bytes(row[2]),listBytes=Bytes(row[3]),slots=Bytes(row[4]);std::copy(body.begin(),body.end(),memory.Bytes(token).begin());
        for (std::size_t index=0;index<lists.size();++index) std::copy_n(listBytes.begin()+static_cast<std::ptrdiff_t>(index*64),64,memory.Bytes(lists[index]).begin()); //각목록의여유꼬리도그대로설치한다.
        for (std::uint16_t index=0;index<6;++index) std::copy_n(slots.begin()+index*50,50,pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(50+index)}).begin()); //타입/생사/좌표등전체슬롯입력이다.
        types[97].flags2=0x10000000;types[98].flags2=0x200;types[97].footX=types[97].footY=types[98].footX=types[98].footY=3;types[99].footX=types[99].footY=1;now=std::bit_cast<double>(cfg[0]);random.SetState(static_cast<std::uint32_t>(cfg[1]));Bind();
    }
    // 각명시하위의입력/반환을관측도구와같이공급한다.목록/전략계산은제품이수행한다.
    void Bind() {
        hooks.technology=[&](auto owner) { trace.push_back("technology,"+std::to_string(owner));return Sid{50}; };
        hooks.refresh=[&](auto,auto parent) { trace.push_back("refresh,"+std::to_string(parent.value));if (cfg[13]) Put(memory.Bytes(token),0x2d4,static_cast<std::uint32_t>(cfg[13])); };
        hooks.graph=[&](Sid sid) { trace.push_back("graph,"+std::to_string(sid.value));return static_cast<std::uint32_t>(cfg[4]); };
        hooks.center=[&](Sid sid) { trace.push_back("center,"+std::to_string(sid.value));RawSquidNeighbors neighbors(pool,hash,spots,types,frames);const auto center=neighbors.Center(sid);return RawPathPoint{center[0],center[1]}; };
        hooks.connected=[&](auto,Sid sid) { trace.push_back("connected,"+std::to_string(sid.value));return cfg[5]!=0; };
        hooks.advance=[&](auto,RawPathPoint point) { trace.push_back("advance,"+XY(point));return cfg[6]!=0; };
        hooks.home=[&](auto) { trace.push_back("home");return RawPathPoint{101.25f,102.75f}; };hooks.reclassify=[&](auto) { trace.push_back("reclassify"); };
        hooks.select=[&](auto,RawPathPoint point,auto excluded,auto required) { trace.push_back("select,"+XY(point)+","+std::to_string(excluded)+","+std::to_string(required));return Sid{static_cast<std::uint16_t>(cfg[12])}; };
        hooks.randomStart=[&](auto,const AiBuildSearchBounds& bounds) { trace.push_back("random,"+XY({bounds.left,bounds.top})+","+XY({bounds.right,bounds.bottom}));return RawPathPoint{std::bit_cast<float>(static_cast<std::uint32_t>(cfg[8])),std::bit_cast<float>(static_cast<std::uint32_t>(cfg[9]))}; };
        hooks.extend=[&](auto,RawPathPoint source,RawPathPoint target) { trace.push_back("extend,"+XY(source)+","+XY(target));return RawPathPoint{std::bit_cast<float>(static_cast<std::uint32_t>(cfg[10])),std::bit_cast<float>(static_cast<std::uint32_t>(cfg[11]))}; };
        hooks.rectangle=[&](auto left,auto top,auto right,auto bottom) { trace.push_back("rectangle,"+std::to_string(static_cast<std::uint32_t>(left))+","+std::to_string(static_cast<std::uint32_t>(top))+","+std::to_string(static_cast<std::uint32_t>(right))+","+std::to_string(static_cast<std::uint32_t>(bottom)));return cfg[7]!=0; };
        hooks.log=[&](auto,auto kind,AiBridgeStrategyLog event,auto sid) { trace.push_back("log,"+std::to_string(kind)+","+std::to_string(static_cast<int>(event))+","+std::to_string(sid)); };
        hooks.assertion=[&](auto,auto line) { trace.push_back("assert,"+std::to_string(line)); };
        hooks.delay=[&](auto current) { trace.push_back("delay");std::uint32_t bridge{82},fence{90},archer{140};RawAiBridgeCandidates candidates(memory,pool,types,frames,hash.Entries(0),spots,{bridge,fence,archer});RawAiBridgePlanning planning(candidates,random,now,{});planning.DelayRetry(current); };
    }
    // 같은실제메모리와현재관문을제품의네전략에전달한다.
    RawAiBridgeStrategies Make() { return {memory,pool,types,random,{now,mapLimit,[] { return 254U; },[&] { return cfg[2]!=0; },[&] { return cfg[3]!=0; },logKind},hooks}; }
    // 실제목록네개의미사용꼬리까지순서대로비교한다.
    std::string ListsHex() { std::string text;for (const auto list:lists) text+=Hex(memory.Bytes(list));return text; } //입력과같은소유핸들순서다.
    // 도달한현재하위호출들을원문과같은구분자로나타낸다.
    std::string Trace() { std::string text;for (const auto& event:trace) { if (!text.empty()) text+=';';text+=event; }return text; } //첫항목앞에는구분자를붙이지않는다.
};
// 별도모델로전략기대를만들지않고독립원본행을읽는다.
FixtureData Fixture() { const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/ai-bridge-strategies-x86.tsv";return LoadFixture(path.string().c_str()); }
}
// 네case전체/공통관문/NaN/재고뒤목표재조회/중복목록/기한/좌표의독립원문이다.
TEST_CASE(AiBridgeStrategies_Native198WholeBodiesListsRandomAndOrder) {
    std::size_t count{};
    for (const auto& row:Fixture().rows) { //원본입력만설치하고전체제품몸체를한번호출한다.
        CHECK(row.size()==9);if (row.size()!=9) return;StrategyScene scene(row);scene.Make().Update(scene.token);const auto same=Hex(scene.memory.Bytes(scene.token))==row[5] && scene.ListsHex()==row[6] && scene.random.State()==Number(row[7]) && scene.Trace()==row[8];
        if (!same) std::printf("AI 네다리 전략 원본 차이 행%zu 난수%u/%s 순서%s/%s\n",count,scene.random.State(),row[7].c_str(),scene.Trace().c_str(),row[8].c_str());CHECK(same);++count;
    }
    CHECK(count==198);
}
// checking이거짓이면disabled장치를읽지않고재고효과뒤현재목표/좌표를다시쓴다.
TEST_CASE(AiBridgeStrategies_CurrentGoalAndCoordinateAfterEffects) {
    const auto rows=Fixture().rows;StrategyScene scene(rows[5]);std::size_t disabled{};scene.hooks.refresh=[&](auto,auto) { Put(scene.memory.Bytes(scene.token),0x2d4,4); };scene.hooks.rectangle=[&](auto,auto,auto,auto) { Put(scene.memory.Bytes(scene.token),0xaf0,std::bit_cast<std::uint32_t>(111.0f));Put(scene.memory.Bytes(scene.token),0xaf4,std::bit_cast<std::uint32_t>(112.0f));return true; };scene.hooks.advance=[&](auto,RawPathPoint point) { CHECK(point.x==111 && point.y==112);return false; };
    RawAiBridgeStrategies strategy(scene.memory,scene.pool,scene.types,scene.random,{scene.now,scene.mapLimit,[] { return 254U; },[] { return false; },[&] { ++disabled;return true; },scene.logKind},scene.hooks);strategy.Update(scene.token);CHECK(!disabled && Get(scene.memory.Bytes(scene.token),0xaf0)==0xc61c4000U && Get(scene.memory.Bytes(scene.token),0xaf4)==0xc61c4000U);
}
// graph하위가대상을바꾸면중심도변경후현재SID에서다시읽는다.
TEST_CASE(AiBridgeStrategies_TempleReadsCurrentTargetAfterGraph) {
    const auto rows=Fixture().rows;StrategyScene scene(rows[5]);scene.hooks.graph=[&](Sid sid) { CHECK(sid.value==51);Put(scene.memory.Bytes(scene.token),0x3cc,52);return 8U; };bool advanced{};scene.hooks.advance=[&](auto,RawPathPoint point) { CHECK(point.x==125 && point.y==124);advanced=true;return true; };scene.Make().Update(scene.token);CHECK(advanced && scene.Trace()=="technology,2;refresh,50;center,52");
}
// 실제Point동등비교와같이연장결과가시작점과같으면확장잔여수를소비하지않는다.
TEST_CASE(AiBridgeStrategies_UnchangedExtensionKeepsBudget) {
    const auto rows=Fixture().rows;StrategyScene scene(rows[13]);Put(scene.memory.Bytes(scene.token),0x2f0,0);scene.cfg[10]=scene.cfg[8];scene.cfg[11]=scene.cfg[9];const auto before=Get(scene.memory.Bytes(scene.token),0x3c8);scene.Make().Update(scene.token);CHECK(Get(scene.memory.Bytes(scene.token),0x3c8)==before && Get(scene.memory.Bytes(scene.token),0x3d0)==scene.cfg[8] && Get(scene.memory.Bytes(scene.token),0x3d4)==scene.cfg[9]);CHECK(scene.Trace().find("extend,")!=std::string::npos);
}
// 기한이NaN이면네전략모두현재재고갱신까지만하고선택/배치는실행하지않는다.
TEST_CASE(AiBridgeStrategies_UnorderedDeadlineSkipsEveryCase) {
    const auto rows=Fixture().rows;
    for (std::size_t goal=1;goal<=4;++goal) { //각전략의현재기한필드만quiet NaN으로공급한다.
        StrategyScene scene(rows[goal*4+1]);const auto at=goal<=2 ? 0x5a8U : 0x5b0U;Put(scene.memory.Bytes(scene.token),at,0);Put(scene.memory.Bytes(scene.token),at+4,0x7ff80000U);scene.Make().Update(scene.token);CHECK(scene.Trace()=="technology,2;refresh,50");
    }
}
