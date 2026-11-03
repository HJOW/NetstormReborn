// Graph의 원문 진단 복귀와 실제 영역 통지의 현재 행/다음 SID 소비를 확인한다.
#include "RawSceneSupport.h"
#include "o/RawGraph.h"
#include "o/RawPlayerPlacementAnchor.h"
#include <algorithm>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 게임/공격/포획 없이 원문 행과 일반 표면만 구성한다.
// 원본6BYTE 기록을 padding 없이 직렬화하여 fixture 전체 바이트와 비교한다.
std::string RecordHex(const GraphRecord& record) {
    std::array<std::uint8_t,6> bytes{};Put(bytes,0,static_cast<std::uint16_t>(record.surfaces),2);
    Put(bytes,2,static_cast<std::uint16_t>(record.inUse),2);Put(bytes,4,static_cast<std::uint16_t>(record.reserved),2);return Hex(bytes);
}
// 실제 일반 finder가 조회하는 비표면 발자국2칸과 각 한 칸 표면을 준비한다.
std::vector<RiftTypeRecord> ReferenceTypes() {
    std::vector<RiftTypeRecord> types(188);types[73].footX=2;types[73].footY=1;
    types[74].flags1=TypeFlag1::kSurface;types[74].footX=types[74].footY=1;return types;
}
// 동일 RawGraph/Pool/Hash와 장기 기록을 제공하며 생성/대조가 별도 게임을 실행하지 않는다.
struct RegionScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true}; // 원문 필드를 기록할 일반 할당 슬롯이다.
    SquidHash hash; // 단계0의 서로 다른 현재 두 머리를 제공한다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 두 표면의 내부 bit8을 입력한다.
    std::vector<RiftTypeRecord> types{ReferenceTypes()}; // 원본 일반 영역/표면 필터의 고정 입력이다.
    std::vector<std::vector<FrameCode>> frames{188}; // 영역 단독 통지는 프레임을 읽지 않는다.
    RawGraphStorage storage; // 진단 callback이 바꾸는 실제 행과 미사용 stack이다.
    RawGraph graph{pool,hash,spots,types,frames,storage}; // 사전 계산과 실제 통지를 같은 물리에 연결한다.
    Sid first{},second{},source{}; // 현재 두 후보와 발자국2칸의 통지 원천이다.
    std::uint32_t checking{0x80000000U}; // bool로 정규화하지 않는 실제 장기 진단 입력이다.
    // 연결된 표면 외의 hash/spot/타입은 사용하지 않으며 raw 데이터는 입력으로만 쓴다.
    RegionScene() {
        ResetRawGraphTable(storage);first=pool.Allocate(2);second=pool.Allocate(2);source=pool.Allocate(2);
        // 실제 슬롯의 다른 payload를 일정하게 유지하여 graph BYTE 밖의 변경도 검출한다.
        for (const auto sid:{first,second,source}) {
            auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=sid==source ? 73 : 74;
            Put(raw,14,std::bit_cast<std::uint32_t>(sid==first ? 20.0F : 21.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));
        }
        pool.AllocatedBytes(first)[30]=3;pool.AllocatedBytes(second)[30]=4;storage.records[3]={0,0,971};storage.records[4]={0,0,972};storage.records[5]={3,1,973};
        hash.Cell(0,20,20)=first.value;hash.Cell(0,21,20)=second.value;spots[20*256+20]=spots[20*256+21]=8;
    }
};
}
// 원문760 입력의 signed WORD·전체 checking·식/파일/행 순서·SID0 반환을 대조한다.
TEST_CASE(GraphReferenceChecking_ReplaysNativeBothEditions) {
    const auto rows=LoadFixture(NETSTORM_GRAPH_REFERENCE_CHECKING_FIXTURE).rows;std::size_t count=0;
    // 기대값은 독립 PE 결과이며 C++로 재계산하지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==10);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        GraphRecord record{static_cast<std::int16_t>(std::stoi(row[2])),static_cast<std::int16_t>(std::stoi(row[3])),19433};
        std::uint32_t checking=Number(row[4]);const auto action=Number(row[5]);std::string events;
        const auto assertion=[&](std::string_view expression,std::string_view file,std::uint32_t line) {
            if (!events.empty()) events+='|';events+=std::to_string(line)+':'+std::string(expression)+':'+std::string(file);
            if (action==1) checking=0;else if (action==2) record={3,1,-310};else if (action==3) record.surfaces=4;else if (action==4) record.inUse=1;
        };
        if (row[1]=="Remove") RemoveGraphSurface(record,edition,{RawCheckingFlag{checking},assertion});
        else {
            CHECK(row[1]=="Zero");SidPool pool(edition,32768,true);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
            std::vector<std::uint16_t> surfaces(65536);SquidPostPopState books;SquidPostPopList additional;ContainedFinderState contained;contained.checkingDead.Bind(checking);
            PlayerPlacementAnchorState state;state.graphAssertion=assertion;RawPlayerPlacementAnchor anchor(pool,types,surfaces,books,additional,contained,state);
            CHECK(anchor.GraphFor(0)==Number(row[9]));
        }
        CHECK(RecordHex(record)==row[6] && checking==Number(row[7]) && (events.empty() ? "-" : events)==row[8]);++count;
    }
    CHECK(count==760);
}
// 실제 영역 통지는 진단 중 고친 같은 행과 이후 후보의 현재 번호를 읽고 현재 checking 변경도 보존한다.
TEST_CASE(GraphReferenceChecking_ActualRegionUsesCurrentRecordsAndNextSlots) {
    RegionScene scene;std::size_t assertions=0;
    scene.graph.BindDiagnostics({RawCheckingFlag{scene.checking},[&](std::string_view expression,std::string_view file,std::uint32_t line) {
        CHECK(expression=="inUse" && file=="\\Ns\\O\\Graph.cpp" && line==0x59);++assertions;
        scene.storage.records[3]={2,1,991};scene.pool.AllocatedBytes(scene.second)[30]=5;scene.checking=0;
    }});
    const auto before=Hex(scene.pool.Bytes());scene.graph.ValidatePostPop(scene.source,true,false);
    CHECK(assertions==0 && Hex(scene.pool.Bytes())==before && scene.storage.records[3].surfaces==0);
    scene.graph.PostPop(scene.source,true,false);CHECK(assertions==1 && scene.checking==0);
    CHECK(scene.storage.records[3].surfaces==1 && scene.storage.records[3].inUse==1 && scene.storage.records[3].reserved==991);
    CHECK(scene.storage.records[4].surfaces==0 && scene.storage.records[4].inUse==0 && scene.storage.records[4].reserved==972);
    CHECK(scene.storage.records[5].surfaces==2 && scene.storage.records[5].inUse==1 && scene.storage.records[5].reserved==973);
    CHECK(scene.pool.Slot(scene.first)[30]==254 && scene.pool.Slot(scene.second)[30]==254);
    CHECK(scene.storage.records[254].surfaces==0x7dfd && std::all_of(scene.storage.stack.begin(),scene.storage.stack.end(),[](auto word) { return word==0; }));
}
// 선택된 장치 누락은 현재 첫 행/graph BYTE 쓰기 전에 멈추며 Validate는 진단 장치를 요구하지 않는다.
TEST_CASE(GraphReferenceChecking_MissingSelectedDiagnosticKeepsFirstWriteBoundary) {
    RegionScene scene;scene.graph.BindDiagnostics({RawCheckingFlag{scene.checking},{}});const auto before=Hex(scene.pool.Bytes());
    scene.graph.ValidatePostPop(scene.source,true,false);bool rejected=false;
    try { scene.graph.InvalidateRegion(scene.source); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && Hex(scene.pool.Bytes())==before && scene.storage.records[3].surfaces==0 && scene.storage.records[3].inUse==0);
}
