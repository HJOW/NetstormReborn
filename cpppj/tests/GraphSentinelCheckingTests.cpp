// 원문 sentinel 진단 복귀와 실제 Free의 현재 기록 쓰기를 헤드리스로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGraph.h"
#include <algorithm>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 게임 상태 없이 같은 원문6BYTE 행을 직렬화하는 검사 보조다.
// signed WORD의 원본 비트와 reserved를 모두 비교하는 hex를 만든다.
std::string SentinelRecordHex(const GraphRecord& record) {
    std::array<std::uint8_t,6> bytes{};
    Put(bytes,0,static_cast<std::uint16_t>(record.surfaces),2);
    Put(bytes,2,static_cast<std::uint16_t>(record.inUse),2);
    Put(bytes,4,static_cast<std::uint16_t>(record.reserved),2);return Hex(bytes);
}
}
// 독립 PE 관측과 실제 공유 RawGraph의 진단 순서·동일 행 쓰기·무효254 자연 반환을 비교한다.
TEST_CASE(GraphSentinelChecking_ReplaysNativeFreeAndCurrentSentinel) {
    const auto rows=LoadFixture(NETSTORM_GRAPH_SENTINEL_CHECKING_FIXTURE).rows;
    std::size_t count=0;
    // 각 행마다 판본/장기 물리/진단 owner를 새로 구성하여 앞 입력의 효과를 섞지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==10);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        SidPool pool(edition,32768,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        std::vector<std::vector<FrameCode>> frames(types.size());RawGraphStorage storage;
        storage.records[3]={7,1,19433};storage.records[Graph::kInvalid]={static_cast<std::int16_t>(std::stoi(row[2])),static_cast<std::int16_t>(std::stoi(row[3])),27499};
        storage.stack[4095]=0x12345678;RawGraph graph(pool,hash,spots,types,frames,storage);
        std::uint32_t checking=Number(row[4]);const auto action=Number(row[5]);std::string events;
        // 진단은 현재 sentinel/checking 또는 같은 Free 대상의 reserved도 변경한 뒤 원문처럼 돌아온다.
        GraphDiagnostics diagnostics{RawCheckingFlag{checking},[&](auto expression,auto file,auto line) {
            if (!events.empty()) events+='|';events+=std::to_string(line)+':'+std::string(expression)+':'+std::string(file);
            if (action==1) checking=0;
            else if (action==2) storage.records[Graph::kInvalid].inUse=0x7dfd;
            else if (action==3) storage.records[Graph::kInvalid].inUse=-1;
            else if (action==4) storage.records[3]={9,-1,-310};
        }};
        graph.BindDiagnostics(diagnostics);
        if (row[1]=="Check") CheckGraphSentinel(storage.records[Graph::kInvalid],edition,diagnostics);
        else graph.Free(row[1]=="Free" ? 3 : Graph::kInvalid);
        CHECK(SentinelRecordHex(storage.records[Graph::kInvalid])==row[6]);
        CHECK(SentinelRecordHex(storage.records[3])==row[7]);
        CHECK(checking==Number(row[8]) && (events.empty() ? "-" : events)==row[9]);
        CHECK(storage.stack[4095]==0x12345678);++count;
    }
    CHECK(count==1080);
}
// 필요한 진단 장치가 없으면 명시 오류로 중단하고 Free 대상과 sentinel을 임의로 정상화하지 않는다.
TEST_CASE(GraphSentinelChecking_MissingServicePreservesRecord) {
    GraphRecord record{0,0,313};std::uint32_t checking=1;bool failed=false;
    try { CheckGraphSentinel(record,OriginalEdition::Patch1078,{RawCheckingFlag{checking},{}}); }
    catch (const std::logic_error&) { failed=true; }
    CHECK(failed && record.surfaces==0 && record.inUse==0 && record.reserved==313);
    CheckGraphSentinel(record,OriginalEdition::Cd1072,{RawCheckingFlag{checking},{}});
}
