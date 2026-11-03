// 실제 Graph 부모의 진단/로그 재진입을 독립 PE의 전체 표/stack/슬롯과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGraph.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>
#include <memory>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 원문 입력을 실제 할당 SID에 옮기며 기대 Graph 효과를 계산하지 않는다.
// 원문144행의 현재 물리와 명시 진단/로그 변경 입력을 조립한다.
struct LiveGraphScene {
    SidPool pool; // 기존 고정 분할을 만족하는 독립 raw 풀이다.
    SquidHash hash; // 원문 표면 지도이며 로그에서 뒤 셀을 변경할 수 있다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 실제 raw 탐색기의 내부 필터 입력이다.
    std::vector<RiftTypeRecord> types; // 원문70+번호의 합성 타입 표다.
    std::vector<std::vector<FrameCode>> frames; // 각 표면의 A/P 코드 한 개다.
    RawGraphStorage storage; // 콜백이 직접 바꾸는 현재 기록/stack이다.
    std::array<Sid,6> ids{}; // 원문도 예약1~4 밖의5..10을 사용하여 전역 iterator 순서까지 같은 할당 번호로 비교한다.
    std::unique_ptr<RawGraph> graph; // 위 현재 물리를 실제 실행과 Validate가 함께 빌린다.
    std::uint32_t checking{},action{},traces{}; // Native의 전체 DWORD와 콜백 종류/로그 수다.
    std::string events; // 실제 진단과 로그의 호출 순서를 보관한다.
    // 원문 위치/기록/stack 입력만 조립하며 최종 기대값은 fixture에서 읽는다.
    explicit LiveGraphScene(const std::vector<std::string>& row):pool(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768,true),
        types(pool.Edition()==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size()),checking(Number(row[2])),action(Number(row[3])) {
        const std::array<std::array<float,2>,6> positions{{{20,20},{19,20},{21,20},{22,20},{20,21},{20,19}}}; // Native 장면의 입력 좌표다.
        const std::array<std::uint8_t,6> numbers{{row[1]=="Flood" ? std::uint8_t{3} : std::uint8_t{254},3,4,4,5,6}}; // Native가 준 시작 graph BYTE다.
        // 원문 객체를 실제 할당 슬롯에만 기록하고 주변 발자국 지도도 같은 입력으로 만든다.
        for (std::size_t index=0;index<ids.size();++index) {
            ids[index]=pool.Allocate(2);CHECK(ids[index].value==index+5);auto raw=pool.AllocatedBytes(ids[index]);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            const auto type=static_cast<std::uint8_t>(75+index);types[type].footX=types[type].footY=1;types[type].flags1=TypeFlag1::kSurface;types[type].flags2=4;frames[type]={{'A','P',1,0}};
            raw[10]=type;raw[pool.Edition()==OriginalEdition::Patch1078 ? 30 : 28]=numbers[index];
            const auto x=index==0 && row[1]=="AddZero" ? 0.0F : positions[index][0],y=positions[index][1];
            Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));
            hash.Cell(0,static_cast<int>(x),static_cast<int>(y))=ids[index].value;
        }
        if (row[1]=="AddVoid") pool.AllocatedBytes(ids[0])[11]=4;
        // 비활성 행의 reserved와 전체 오래된 stack도 원문 입력대로 넣는다.
        for (std::size_t index=0;index<storage.records.size();++index) storage.records[index]={0,0,static_cast<std::int16_t>(100+index)};
        // 원문 표의 장부 차이는 보정하지 않는다.
        for (const auto& input:std::array<std::array<int,3>,6>{{{3,2,2},{4,2,1},{5,1,1},{6,1,1},{7,0,1},{254,0x7dfd,0x7dfd}}}) {
            auto& record=storage.records[static_cast<std::size_t>(input[0])];record.surfaces=static_cast<std::int16_t>(input[1]);record.inUse=static_cast<std::int16_t>(input[2]);
        }
        // 현재 stack의 사용하지 않는 모든 DWORD까지 고유한 입력으로 채운다.
        for (std::size_t index=0;index<storage.stack.size();++index) storage.stack[index]=0xa0000000U+static_cast<std::uint32_t>(index);
        graph=std::make_unique<RawGraph>(pool,hash,spots,types,frames,storage,GraphRecovery::FullPool);
        graph->BindDiagnostics({RawCheckingFlag{checking},[this](auto expression,auto file,auto line) { Assert(expression,file,line); }});
        graph->BindTrace([this](auto level,auto text) { Trace(level,text); });
    }
    // 원문의 진단 입력을 기록하고 지정한 실제 행/다음 슬롯 변경을 수행한 뒤 돌아온다.
    void Assert(std::string_view expression,std::string_view file,std::uint32_t line) {
        Append("a:"+std::to_string(line)+':'+std::string(expression)+':'+std::string(file));
        if (action==1) {
            checking=0;storage.records[3]={9,0,-310};auto raw=pool.AllocatedBytes(ids[0]);raw[11]=0;
            Put(raw,14,std::bit_cast<std::uint32_t>(20.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));
        } else if (action==2) {
            pool.AllocatedBytes(ids[0])[11]=0;
            if (line==0x100 || line==0xc2) { pool.AllocatedBytes(ids[2])[pool.Edition()==OriginalEdition::Patch1078 ? 30 : 28]=5;storage.records[5].surfaces=3; }
            else if (line==0x113) { storage.records[5].surfaces=1;storage.records[4].surfaces=77; }
        } else if (action==4 && line==0x68) checking=0;
    }
    // 로그 인자의 수를 먼저 보관한 뒤 뒤 후보/현재 행/stack/지도만 Native 입력대로 바꾼다.
    void Trace(std::int32_t level,std::string_view text) {
        Append("l:"+std::to_string(level)+':'+std::string(text.substr(0,text.find(' '))));++traces;
        if (traces==1 && action==3) {
            const auto offset=pool.Edition()==OriginalEdition::Patch1078 ? 30 : 28;const auto number=pool.Slot(ids[0])[offset];
            // 지역 연결 목록은 건드리지 않고 뒤 SID의 실제 번호를 바꾼다.
            for (const auto index:{2U,4U}) pool.AllocatedBytes(ids[index])[offset]=number;
            if (number<251) storage.records[number].surfaces=91;storage.stack[4095]=0xdeadbeef;
        } else if (traces==1 && action==4) storage.records[254].surfaces=storage.records[254].inUse=0;
        else if (traces==1 && action==5) hash.Cell(0,22,20)=0;
        else if (traces==1 && action==6) pool.AllocatedBytes(ids[2])[11]=1;
    }
    // Native의 호출 순서 문자열과 같은 구분자로 연결한다.
    void Append(const std::string& event) { if (!events.empty()) events+='|';events+=event; }
    // 원문6BYTE 행의 padding 없는 전체 바이트를 직렬화한다.
    std::string Records() const {
        std::vector<std::uint8_t> bytes(storage.records.size()*6);
        // reserved WORD까지 모든 행을 포함한다.
        for (std::size_t index=0;index<storage.records.size();++index) {
            const auto& record=storage.records[index];Put(bytes,index*6,static_cast<std::uint16_t>(record.surfaces),2);
            Put(bytes,index*6+2,static_cast<std::uint16_t>(record.inUse),2);Put(bytes,index*6+4,static_cast<std::uint16_t>(record.reserved),2);
        }
        return Hex(bytes);
    }
    // 양쪽 모두 실제 할당5..10을 사용하므로 SID 정규화 없이 전체 원문 stack을 직접 비교한다.
    std::string Stack() const {
        std::vector<std::uint8_t> bytes(storage.stack.size()*4);
        // 사용/미사용 영역 전체를 비교하여 콜백의 뒤쪽 변경이 Commit에 덮이지 않는지도 확인한다.
        for (std::size_t index=0;index<storage.stack.size();++index) {
            Put(bytes,index*4,storage.stack[index]);
        }
        return Hex(bytes);
    }
    // 타입/상태/좌표/payload를 포함한 실제 슬롯 전체를 원문 순서로 비교한다.
    std::string Slots() const {
        std::vector<std::uint8_t> bytes;
        // SID를 저장한 next 같은 입력 필드는 이 장면에서 모두0이므로 raw 바이트를 직접 이어 붙인다.
        for (const auto sid:ids) { const auto raw=pool.Slot(sid);bytes.insert(bytes.end(),raw.begin(),raw.end()); }
        return Hex(bytes);
    }
};
}
// 지정한 PE fixture의 전체 현재 표/stack/슬롯과 진단·로그 순서를 비교한다.
static void ReplayGraphLiveRows(const char* fixture,std::size_t expectedRows) {
    const auto rows=LoadFixture(fixture).rows;std::size_t count=0;
    // 기대값은 원문 출력이며 C++의 별도 모델로 다시 계산하지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==10);LiveGraphScene scene(row);std::string result="-";
        if (row[1].starts_with("Add")) scene.graph->Add(scene.ids[0]);
        else if (row[1]=="Flood") result=std::to_string(scene.graph->Flood(scene.ids[0],7));
        else scene.graph->Rebuild(row[1]=="Rebuild1");
        const auto records=scene.Records(),stack=scene.Stack(),slots=scene.Slots(),events=scene.events.empty() ? "-" : scene.events;
        if (result!=row[4] || records!=row[5] || stack!=row[6] || slots!=row[7] || scene.checking!=Number(row[8]) || events!=row[9])
            std::printf("Graph 현재 실행 불일치 %s %s checking=%s action=%s result=%d records=%d stack=%d slots=%d checkingMatch=%d\nexpectedEvents=%s\nactualEvents=%s\n",
                row[0].c_str(),row[1].c_str(),row[2].c_str(),row[3].c_str(),result==row[4],records==row[5],stack==row[6],slots==row[7],scene.checking==Number(row[8]),row[9].c_str(),events.c_str());
        if (stack!=row[6]) {
            // 처음 달라진 DWORD와 앞8DWORD를 출력하며 기대 stack은 그대로 유지한다.
            for (std::size_t offset=0;offset<stack.size();offset+=8) if (stack.substr(offset,8)!=row[6].substr(offset,8)) {
                std::printf("stack 첫 차이 index=%zu expected=%s actual=%s prefixExpected=%s prefixActual=%s\n",offset/8,row[6].substr(offset,8).c_str(),stack.substr(offset,8).c_str(),row[6].substr(0,64).c_str(),stack.substr(0,64).c_str());break;
            }
        }
        CHECK(result==row[4] && records==row[5] && stack==row[6] && slots==row[7] && scene.checking==Number(row[8]) && events==row[9]);++count;
    }
    CHECK(count==expectedRows);
}
// 두 PE의 실제144행과 진단/로그 변경6종을 원문 배열 전체에 대조한다.
TEST_CASE(GraphLiveChecking_ReplaysNativeParentsAndCallbackMutations) { ReplayGraphLiveRows(NETSTORM_GRAPH_LIVE_CHECKING_FIXTURE,144); }
// 첫 로그에서 뒤 후보가 free가 되어도 원문 내부 Flood가 계속 쓰는 별도24행을 대조한다.
TEST_CASE(GraphLiveChecking_ReplaysNativeFreeCandidateAfterTrace) { ReplayGraphLiveRows(NETSTORM_GRAPH_LIVE_FREE_FIXTURE,24); }
// 실제 World의 전체 풀 소진 복구와 장기 Storage→Client 로그 장치 연결을 창 없이 검증한다.
TEST_CASE(GraphLiveChecking_ActualWorldUsesFullPoolAndStorageTrace) {
    using namespace netstorm::client;
    const auto edition=OriginalEdition::Patch1078;std::vector<RiftTypeRecord> types(188);
    std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);
    types[74].flags1=TypeFlag1::kSurface;types[74].flags2=4;types[74].footX=types[74].footY=1;frames[74]=RiftTypeFrames({{'A','P',1,0}});
    auto storage=std::make_shared<RawSurfaceStorage>(edition,types,frames,shapes);
    std::vector<std::pair<std::int32_t,std::string>> traces;storage->BindGraphTrace([&](auto level,auto text) { traces.emplace_back(level,text); });
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    {
        RawSurfaceWorld world(edition,types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();const auto sid=core.pool.Allocate(2);
        auto raw=core.pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=74;raw[30]=254;
        Put(raw,14,std::bit_cast<std::uint32_t>(20.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));core.hash.Cell(0,20,20)=sid.value;
        // 실제 World가 부분 snapshot의 Reject 정책을 쓰면 이 원문 소진 복구는 실행되지 않는다.
        for (std::size_t index=0;index<Graph::kCount;++index) storage->graph.records[index]={0,1,static_cast<std::int16_t>(100+index)};
        core.graph.Add(sid);CHECK(core.pool.Slot(sid)[30]==1);CHECK(storage->graph.records[1].surfaces==1);
        CHECK(traces.size()==3);
        // 현재 World에 설치한 실제 owner 콜백이 원문 수준/문구를 그대로 받는지 확인한다.
        for (const auto& [level,text]:traces) CHECK(level==0x20 && text.ends_with(" graphs in use\n"));
    }
    const auto before=traces.size();storage->TraceGraph(32,"owner remains\n");CHECK(traces.size()==before+1 && traces.back().second=="owner remains\n");
}
