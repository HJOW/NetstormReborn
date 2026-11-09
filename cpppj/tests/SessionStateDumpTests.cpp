// 검사 전용 Session 상태 덤프(client/SessionStateDump)의 순수 서식·분산 SP 복호·풀 해시를 확인한다.
// 실제 Client 수집 경로(DumpSessionState)는 --run --ui-script 의 session-dump 단계로 확인한다.
// 2026-11-09: webpj/dotnetpj L2·L3 대조용 cpppj Session 덤프 추가와 함께 작성했다.
#include "TestSupport.h"
#include "client/SessionStateDump.h"
#include "o/SidPool.h"
#include "o/SpStore.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm;
using namespace netstorm::client;
namespace {
// 실제 월드와 같은 SID 풀 크기다. (2026-11-09 추가)
constexpr std::uint32_t kDumpPoolCapacity=32768;
// 리틀 엔디언으로 width 바이트를 쓴다. (2026-11-09 추가)
void PutLittle(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 낮은 바이트부터 차례로 기록한다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 호출이 표준 예외로 거부되는지 확인한다(다른 시험 파일과 같은 지역 도우미). (2026-11-09 추가)
template<class F> bool Throws(F action) { try { action();return false; } catch (const std::exception&) { return true; } }
// 문자열에서 접두가 같은 행 수를 센다. (2026-11-09 추가)
std::size_t CountRows(const std::string& text,const std::string& prefix) {
    std::size_t count{},begin{};
    // 줄마다 접두를 비교한다.
    while (begin<text.size()) {
        const auto end=text.find('\n',begin);const auto line=text.substr(begin,end==std::string::npos ? std::string::npos : end-begin);
        if (line.rfind(prefix,0)==0) ++count;
        if (end==std::string::npos) break;begin=end+1;
    }
    return count;
}
// 판본별 자산 슬롯 한 개와 나머지 월드 칸을 채운 덤프 입력과 그 저장소다. (2026-11-09 추가)
struct DumpScene {
    o::SidPool pool; // 실제 SID 풀이다.
    o::Sid asset; // 자산 TYPE을 넣은 슬롯이다.
    std::vector<std::uint8_t> players=std::vector<std::uint8_t>(1620); // Player 9기록 원문이다.
    std::array<std::uint8_t,9> hidden{}; // 숨김 BYTE다.
    std::array<std::uint32_t,81> alliances{}; // 동맹 표다.
    std::vector<o::GraphRecord> graph=std::vector<o::GraphRecord>(251); // Graph 표다.
    std::array<std::uint32_t,256> local{},built{},global{}; // TotalMade 세 표다.
    SessionStateDumpInput input; // 서식 함수에 넘기는 입력이다.
    explicit DumpScene(o::OriginalEdition edition):pool(edition,kDumpPoolCapacity) {
        const bool patch=edition==o::OriginalEdition::Patch1078;asset=pool.Allocate(2);auto raw=pool.AllocatedBytes(asset);
        raw[10]=90;raw[11]=0;raw[patch ? 34 : 32]=2;PutLittle(raw,patch ? 36 : 34,5,patch ? 4 : 1);
        PutLittle(raw,14,std::bit_cast<std::uint32_t>(40.5F),4);PutLittle(raw,18,std::bit_cast<std::uint32_t>(-3.25F),4);PutLittle(raw,26,300,patch ? 4 : 2);
        players[180]=0xab;hidden[8]=1;alliances[10]=1;graph[1]={3,1,0};local[90]=1;built[90]=2;
        input.edition=edition;input.frame=42;input.gameTime=1.5;input.gameRandom=0x1234;input.crtRandom=99;input.world=true;
        input.poolBytes=pool.Bytes();input.poolCapacity=pool.Capacity();input.stride=pool.Layout().stride;input.freeCount=pool.FreeCount();
        input.firstFreeServer=pool.FirstFree(false).value;input.firstFreeClient=pool.FirstFree(true).value;input.predictableCursor=pool.PredictableCursor();
        input.kernelSize=1;input.kernelCapacity=39999;input.processes.push_back({7,15001,asset.value,true,0x5104,3,std::bit_cast<std::uint32_t>(1.0F),std::bit_cast<std::uint64_t>(27.0)});
        input.players=players;input.hiddenPlayers=hidden;input.alliances=alliances;input.graph=graph;input.totalCost=-5;input.postPopDepth=0;input.productionDirty=1;
        input.totalLocal=local;input.totalBuilt=built;input.totalGlobal=global;
    }
};
}

// 분산 SP 순수 복호가 실제 저장소 Get과 같은 비트를 읽고 회전 뒤에도 값을 유지한다.
TEST_CASE(SessionStateDump_DecodeMatchesScrambledSpStore) {
    o::GameRandom random(12345);o::ScrambledSpStore store(random,[] { return 777U; });store.Initialize();
    const std::array<std::uint32_t,9> values{0,0x3f800000U,0x41200000U,0xbf800000U,0x7fc00001U,1,0xffffffffU,0x80000000U,0x12345678U}; // NaN·부호·전체 비트를 포함한 입력이다.
    // 소유자 0~8에 서로 다른 비트를 쓴다.
    for (std::uint32_t owner=0;owner<9;++owner) store.Set(owner,values[owner]);
    // 회전 전과 두 번의 회전 뒤를 모두 비교한다.
    for (int round=0;round<3;++round) {
        // 같은 저장소의 Get과 덤프 복호가 같아야 한다.
        for (std::uint32_t owner=0;owner<9;++owner) CHECK(DecodeScrambledSp(store.Pools(),store.Permutation(),store.Offset(),owner)==values[owner] && store.Get(owner)==values[owner]);
        store.Rotate();
    }
    CHECK(Throws([&] { static_cast<void>(DecodeScrambledSp(store.Pools(),store.Permutation(),store.Offset(),9)); }));
}

// 풀 해시는 FNV-1a 64의 공개 시험값과 같다.
TEST_CASE(SessionStateDump_HashIsFnv1a64) {
    const std::string a="a",foobar="foobar"; // 공개 시험 입력이다.
    CHECK(SessionDumpHash({})==0xcbf29ce484222325ULL);
    CHECK(SessionDumpHash({reinterpret_cast<const std::uint8_t*>(a.data()),a.size()})==0xaf63dc4c8601ec8cULL);
    CHECK(SessionDumpHash({reinterpret_cast<const std::uint8_t*>(foobar.data()),foobar.size()})==0x85944171f73967e8ULL);
}

// 패치/CD 월드 덤프는 판본별 소유자·프레임·HP 폭으로 자산을 해석하고 free 슬롯 원문은 쓰지 않는다.
TEST_CASE(SessionStateDump_FormatsWorldRowsForBothEditions) {
    // 두 판본의 슬롯 필드 폭을 모두 확인한다.
    for (const auto edition:{o::OriginalEdition::Patch1078,o::OriginalEdition::Cd1072}) {
        DumpScene scene(edition);const auto text=FormatSessionStateDump(scene.input);const auto sid=std::to_string(scene.asset.value);
        const bool patch=edition==o::OriginalEdition::Patch1078;
        CHECK(text.rfind("netstorm_session_dump\t1\nedition\t"+std::string(patch ? "originals" : "originalCD")+"\nframe\t42\t"+std::to_string(std::bit_cast<std::uint64_t>(1.5))+"\nrandom\t4660\t99\nworld\t1\nkernel\t1\t39999\n",0)==0);
        CHECK(text.find("process\t7\t15001\t"+sid+"\nregular\t7\t20740\t3\t1065353216\t"+std::to_string(std::bit_cast<std::uint64_t>(27.0))+"\n")!=std::string::npos);
        CHECK(text.find("\nobject\t"+sid+"\t90\t0\t2\t5\t"+std::to_string(std::bit_cast<std::uint32_t>(40.5F))+'\t'+std::to_string(std::bit_cast<std::uint32_t>(-3.25F))+"\t300\n")!=std::string::npos);
        CHECK(text.find("\nplayer\t1\tab")!=std::string::npos && CountRows(text,"player\t")==9);
        CHECK(text.find("\nhidden_players\t000000000000000001\n")!=std::string::npos);
        CHECK(text.find("\nsp\t0\t0\t-\t-\t-\t-\t-\t-\t-\t-\t-\n")!=std::string::npos);
        CHECK(text.find("\ngraph\t1\t3\t1\t0\n")!=std::string::npos && CountRows(text,"graph\t")==251);
        CHECK(text.find("\ntotals\t-5\t0\t1\ntotalmade\t90\t1\t2\t0\nend\n")!=std::string::npos);
        std::size_t used{}; // 예약 0~4번과 free가 아닌 슬롯 수다.
        // 같은 풀 원문에서 free 표시를 직접 읽어 slot 행 수와 비교한다.
        for (std::uint32_t id=0;id<scene.pool.Capacity();++id) if (id<5 || !(scene.pool.Slot(o::Sid{static_cast<std::uint16_t>(id)})[11]&1U)) ++used;
        CHECK(CountRows(text,"slot\t")==used && CountRows(text,"object\t")==1);
        CHECK(text.find("\npool\t32768\t"+std::to_string(scene.pool.Layout().stride)+'\t')!=std::string::npos);
    }
}

// Session이 없으면 월드 칸 없이 Kernel 관측까지만 쓰고 끝난다. 분산 SP 초기화 값은 비트 그대로 쓴다.
TEST_CASE(SessionStateDump_WithoutWorldAndInitializedSp) {
    DumpScene scene(o::OriginalEdition::Patch1078);auto input=scene.input;input.world=false;
    CHECK(FormatSessionStateDump(input)=="netstorm_session_dump\t1\nedition\toriginals\nframe\t42\t"+std::to_string(std::bit_cast<std::uint64_t>(1.5))+"\nrandom\t4660\t99\nworld\t0\nkernel\t1\t39999\nprocess\t7\t15001\t"+std::to_string(scene.asset.value)+"\nregular\t7\t20740\t3\t1065353216\t"+std::to_string(std::bit_cast<std::uint64_t>(27.0))+"\nend\n");
    o::GameRandom random(5);o::ScrambledSpStore store(random,[] { return 9U; });store.Initialize();store.Set(1,0x3f800000U);
    auto world=scene.input;world.spInitialized=true;world.spOffset=store.Offset();world.spPools=store.Pools();world.spPermutation=store.Permutation();
    CHECK(FormatSessionStateDump(world).find("\nsp\t1\t"+std::to_string(store.Offset())+"\t0\t1065353216\t0\t")!=std::string::npos);
    world.poolBytes=world.poolBytes.first(10);CHECK(Throws([&] { static_cast<void>(FormatSessionStateDump(world)); }));
}
