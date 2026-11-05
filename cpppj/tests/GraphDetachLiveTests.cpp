// 실제 Detach의 현재 정책/진단/로그/타이머 복귀와 전체 raw 물리를 PE432행에 대조한다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "RawSceneSupport.h"
#include "o/RawGraph.h"
#include "o/RawSquidNeighbors.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>
#include <memory>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 기대값 계산 없이 원문 입력만 구성하는 독립 장면이다.
// 실제 현재 물리와 경계 콜백을 한 장면 수명에 묶는 검사 입력이다.
struct DetachLiveScene {
    SidPool pool; // 원문과 같은 실제 할당5..10을 사용하는 raw 풀이다.
    SquidHash hash; // 실제 일반 네 단계와 표면 지도가 같은 배열이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 이웃/영역의 현재 spot 입력이다.
    std::vector<RiftTypeRecord> types; // raw 타입75..80과 삭제 정보만 치환할81을 둔다.
    std::vector<std::vector<FrameCode>> frames; // 원문 입력의 A/P 코드 한 개다.
    RawGraphStorage storage; // 진단/로그가 직접 바꾸는 현재 표와 전체 stack이다.
    std::array<Sid,6> ids{}; // 원문과 호스트 모두 예약1~4 밖의 같은 SID다.
    std::unique_ptr<RawGraph> graph; // 위 물리보다 먼저 소멸하는 실행기다.
    std::uint32_t checking{},action{},enabled{1},rebuild{},traces{},clocks{}; // 현재 DWORD 뷰와 외부 효과 호출 수다.
    std::string scene,events; // 입력 장면 이름과 실제 진단/로그/시각 호출 순서다.
    // 원문 행의 초기 입력만 조립한다. 최종 번호/표/stack은 fixture에서 읽는다.
    explicit DetachLiveScene(const std::vector<std::string>& row):pool(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768,true),
        types(::netstorm::test::MakeFormTypeInputs(pool.Edition()==OriginalEdition::Patch1078 ? 188 : 171)),frames(types.size()),checking(Number(row[2])),action(Number(row[3])) {
        const auto separator=row[1].find(':');scene=row[1].substr(0,separator);rebuild=Number(row[1].substr(separator+1));enabled=scene=="Disabled" ? 0 : 1;
        const std::array<std::array<float,2>,6> positions{{{20,20},{19,20},{21,20},{22,20},{20,21},{20,19}}}; // 원문 입력 순서다.
        // 동일 raw 슬롯/타입/위치와 네 일반 지도 중0단계만 채운다.
        for (std::size_t index=0;index<ids.size();++index) {
            ids[index]=pool.Allocate(2);CHECK(ids[index].value==index+5);auto raw=pool.AllocatedBytes(ids[index]);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            const auto type=static_cast<std::uint8_t>(75+index);types[type].footX=types[type].footY=1;types[type].flags1=TypeFlag1::kSurface;types[type].flags2=4;frames[type]={{'A','P',1,0}};
            raw[10]=type;raw[11]=index==0 ? 2 : 0;raw[Offset()]=scene=="Lookup" && index ? 4 : 3;
            const auto x=positions[index][0],y=scene=="Lookup" && index==5 ? 20.0F : positions[index][1];
            Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));hash.Cell(0,static_cast<int>(x),static_cast<int>(y))=ids[index].value;
        }
        if (scene=="Lookup") Put(pool.AllocatedBytes(ids[5]),4,ids[0].value,2);
        if (scene=="Missing") hash.Cell(0,20,20)=0;
        types[81].footX=types[81].footY=3;types[81].flags1=TypeFlag1::kSurface;types[81].flags2=8;frames[81]={{'A','P',1,0}};
        // 미사용 행의 reserved와 전체 미활성 stack 흔적까지 원문 입력과 같다.
        for (std::size_t index=0;index<storage.records.size();++index) storage.records[index]={0,0,static_cast<std::int16_t>(100+index)};
        storage.records[3].surfaces=6;storage.records[3].inUse=scene=="Inactive" ? 0 : 2;storage.records[4].surfaces=6;storage.records[4].inUse=1;
        storage.records[254].surfaces=storage.records[254].inUse=action==5 ? 0 : 0x7dfd;
        // 원문은 stack 전체를 초기화하지 않으므로 입력 흔적을 같은 값으로 둔다.
        for (std::size_t index=0;index<storage.stack.size();++index) storage.stack[index]=0xa0000000U+static_cast<std::uint32_t>(index);
        graph=std::make_unique<RawGraph>(pool,hash,spots,types,frames,storage,GraphRecovery::FullPool);
        graph->BindDetachState(RawCheckingFlag{enabled},RawCheckingFlag{rebuild});
        graph->BindDiagnostics({RawCheckingFlag{checking},[this](auto expression,auto file,auto line) { Assert(expression,file,line); }});
        graph->BindTrace([this](auto level,auto text) { Trace(level,text); });graph->BindClock([this] { return Clock(); });
    }
    // 현재 판본의 실제 graph BYTE 위치다.
    std::size_t Offset() const { return pool.Edition()==OriginalEdition::Patch1078 ? 30U : 28U; }
    // 원문 순서 문자열에 한 이벤트를 추가한다.
    void Append(const std::string& value) { if (!events.empty()) events+='|';events+=value; }
    // 선택된 원문 진단만 기록하며 첫 sentinel 진단의 현재 checking/활성 변경을 보존한다.
    void Assert(std::string_view expression,std::string_view file,std::uint32_t line) {
        Append("a:"+std::to_string(line)+':'+std::string(expression)+':'+std::string(file));
        if (line==0x59 && (action==9 || action==10)) {
            auto raw=pool.AllocatedBytes(ids[0]);raw[11]=1;if (action==10) raw[10]=82;
        }
        if ((action==2 || action==5) && line==0x68) { checking=0;if (action==5) enabled=0; }
    }
    // 첫 Flood 로그에서 뒤 후보/기록/정책/지도만 지정 입력대로 바꾼다.
    void Trace(std::int32_t level,std::string_view text) {
        Append("l:"+std::to_string(level)+':'+std::string(text.substr(0,text.find(' '))));++traces;if (traces!=1) return;
        if (action==1) {
            // 지역 연결 배열은 그대로 두고 다음 후보의 실제 graph BYTE를 바꾼다.
            for (const auto index:{2U,4U}) pool.AllocatedBytes(ids[index])[Offset()]=3;
            storage.records[3]={91,2,-321};storage.stack[4095]=0xdeadbeef;rebuild=1;
        } else if (action==2) storage.records[254].surfaces=storage.records[254].inUse=0;
        else if (action==4) pool.AllocatedBytes(ids[2])[11]=1;
        else if (action==6) hash.Cell(0,22,20)=0;
        else if (action==8) rebuild=0;
    }
    // 두 원문 지역 시각 읽기의 경계에서 현재 정책을 바꾸며 값 원천 좌표는 보존돼야 한다.
    double Clock() {
        Append("t");++clocks;
        if (clocks==1 && action==3) {
            rebuild=1;enabled=0;auto raw=pool.AllocatedBytes(ids[0]);Put(raw,14,std::bit_cast<std::uint32_t>(200.0F));Put(raw,18,std::bit_cast<std::uint32_t>(200.0F));storage.records[3]={91,2,-321};
        } else if (clocks==1 && action==7) rebuild=0;
        return 17.25;
    }
    // padding 없는 원문255행의 세 WORD 전체를 직렬화한다.
    std::string Records() const {
        std::vector<std::uint8_t> bytes(storage.records.size()*6);
        // reserved도 비교해 지역 사본이 진단 쓰기를 덮지 않는지 확인한다.
        for (std::size_t index=0;index<storage.records.size();++index) {
            const auto& record=storage.records[index];Put(bytes,index*6,static_cast<std::uint16_t>(record.surfaces),2);Put(bytes,index*6+2,static_cast<std::uint16_t>(record.inUse),2);Put(bytes,index*6+4,static_cast<std::uint16_t>(record.reserved),2);
        }
        return Hex(bytes);
    }
    // 사용/미사용4096DWORD 전체를 SID 정규화 없이 비교한다.
    std::string Stack() const {
        std::vector<std::uint8_t> bytes(storage.stack.size()*4);
        // 원문 흔적과 로그가 바꾼 마지막DWORD까지 포함한다.
        for (std::size_t index=0;index<storage.stack.size();++index) Put(bytes,index*4,storage.stack[index]);
        return Hex(bytes);
    }
    // 원문 실제 SID5..10의 상태/좌표/next/타입/graph와 나머지 payload를 모두 비교한다.
    std::string Slots() const {
        std::vector<std::uint8_t> bytes;
        // 같은 실제 할당 번호이므로 슬롯을 그대로 이어 붙인다.
        for (const auto sid:ids) { const auto raw=pool.Slot(sid);bytes.insert(bytes.end(),raw.begin(),raw.end()); }
        return Hex(bytes);
    }
};
}
// 두 원본 PE가 낸 현재 표/stack/슬롯/전체 DWORD와 이벤트를 한 행씩 그대로 비교한다.
TEST_CASE(GraphDetachLive_ReplaysNativeCurrentStateAndValueSource) {
    const auto rows=LoadFixture(NETSTORM_GRAPH_DETACH_LIVE_FIXTURE).rows;std::size_t count=0;
    // 원문432행의 기대값은 C++ 모델로 재계산하거나 기존 fixture를 바꾸지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==12);DetachLiveScene input(row);const bool replacement=input.scene=="Override";
        input.graph->Detach(input.ids[0],input.rebuild!=0,replacement ? 9 : 1,replacement ? std::optional<std::uint8_t>{std::uint8_t{81}} : std::nullopt);
        const auto records=input.Records(),stack=input.Stack(),slots=input.Slots(),events=input.events.empty() ? "-" : input.events;
        const bool equal=records==row[5] && stack==row[6] && slots==row[7] && input.checking==Number(row[8]) && events==row[9] && input.enabled==Number(row[10]) && input.rebuild==Number(row[11]);
        if (!equal) std::printf("Detach 불일치 %s %s checking=%s action=%s records=%d stack=%d slots=%d checking=%d enabled=%d rebuild=%d\nexpectedEvents=%s\nactualEvents=%s\n",row[0].c_str(),row[1].c_str(),row[2].c_str(),row[3].c_str(),records==row[5],stack==row[6],slots==row[7],input.checking==Number(row[8]),input.enabled==Number(row[10]),input.rebuild==Number(row[11]),row[9].c_str(),events.c_str());
        CHECK(equal);++count;
    }
    CHECK(count==432);
}

// Remove 진단으로 free/비표면이 되어도 원문의 실제 setter가 같은 graph BYTE를 쓰는8행이다.
TEST_CASE(GraphDetachLive_RegionSetterContinuesAfterFreeAndRetype) {
    const std::string parent=NETSTORM_GRAPH_DETACH_LIVE_FIXTURE;const auto separator=parent.find_last_of("/\\");
    const auto path=parent.substr(0,separator+1)+"graph-region-free-x86.tsv";const auto rows=LoadFixture(path.c_str()).rows;std::size_t count=0;
    // 새 독립 PE 기대값의 전체 기록/stack/슬롯/진단을 비교한다.
    for (const auto& row:rows) {
        CHECK(row.size()==12);DetachLiveScene input(row);input.storage.records[3].surfaces=input.storage.records[3].inUse=0;input.spots[20*256+20]=8;
        input.graph->InvalidateRegion(input.ids[0]);const auto events=input.events.empty() ? "-" : input.events;
        const bool equal=input.Records()==row[5] && input.Stack()==row[6] && input.Slots()==row[7] && input.checking==Number(row[8]) && events==row[9];
        if (!equal) std::printf("영역 free 불일치 %s checking=%s action=%s records=%d stack=%d slots=%d\nexpectedEvents=%s\nactualEvents=%s\n",row[0].c_str(),row[2].c_str(),row[3].c_str(),input.Records()==row[5],input.Stack()==row[6],input.Slots()==row[7],row[9].c_str(),events.c_str());
        CHECK(equal);++count;
    }
    CHECK(count==8);
}
// 실제 World의 현재 활성/분할 상태와 장기 Storage의 같은 Client 시계 장치가 유지되는지 확인한다.
TEST_CASE(GraphDetachLive_ActualWorldBindsCurrentModesAndStorageClock) {
    using namespace netstorm::client;
    const auto edition=OriginalEdition::Patch1078;std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);
    std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);
    types[74].flags1=TypeFlag1::kSurface;types[74].flags2=4;types[74].footX=types[74].footY=1;frames[74]=RiftTypeFrames({{'A','P',1,0}});
    std::size_t clocks=0;auto storage=std::make_shared<RawSurfaceStorage>(edition,types,frames,shapes);storage->BindGraphClock([&] { ++clocks;return 17.25; });
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    {
        RawSurfaceWorld world(edition,types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();const auto sid=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(sid);
        std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=74;raw[11]=2;raw[30]=3;Put(raw,14,std::bit_cast<std::uint32_t>(20.0F));Put(raw,18,std::bit_cast<std::uint32_t>(20.0F));core.hash.Cell(0,20,20)=sid.value;
        storage->graph.records[3]={1,1,991};core.bookkeeping.graphsEnabled=false;core.graph.Detach(sid);CHECK(clocks==0 && storage->graph.records[3].inUse==1);
        core.bookkeeping.graphsEnabled=true;world.DeletionState().rebuildGraph=true;core.graph.Detach(sid);
        CHECK(clocks==2 && storage->graph.records[3].surfaces==0 && storage->graph.records[3].inUse==0 && storage->graph.records[3].reserved==991);
    }
    CHECK(storage->ReadGraphClock()==17.25 && clocks==3);
}
