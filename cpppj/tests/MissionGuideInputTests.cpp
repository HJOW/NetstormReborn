// 제품 작성 전 동결한 실제 PE First/Guide 사건과 F8 내부 분기의 전체 상태/순서를 대조한다.
#include "RawSceneSupport.h"
#include "client/MissionGuideInput.h"
#include <array>

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kNativeMissionTable=0x12002000U; // 격리 PE 입력의 가상표 식별값이며 호스트 포인터가 아니다.
// 독립 입력의 관련 필드만 실제 상태로 공급하며 무관한 stale 바이트를 최종 비교까지 보존한다.
struct FirstScene {
    std::array<std::uint32_t,10> input{}; // op/flags/sequence/demo/stage/shown/Tell/current/code/하위 입력 반환이다.
    std::array<std::uint8_t,0xd0> raw{}; // 원본 미션의 전체 물리 바이트 입력이다.
    MissionStartState state; // 원본과 같은 flags/sequence의 단일 현재 저장소다.
    MissionFrameGateState gate; // 현재 demo/단계의 단일 저장소다.
    std::uint32_t shown{}; // 현재+b4의 마지막 Tell 반환 DWORD다.
    std::string trace; // 실제 외부 경계 호출 순서와 바이트/인자다.
    // fixture의 독립 입력을 읽고 원본과 같은 stale 패턴/관련 필드만 준비한다.
    explicit FirstScene(std::string_view values) {
        const auto fields=Split(values,',');CHECK(fields.size()==input.size());
        // 기대 동작 없이 원문 입력 열을 DWORD로 옮긴다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);
        // 원본 관측 준비와 같은 무관 바이트 표식이다.
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*37U+41U);
        Put(raw,0,kNativeMissionTable);Put(raw,0x54,input[1]);Put(raw,0x58,input[2]);Put(raw,0x68,input[3]);Put(raw,0xb4,input[5]);
        state.flags=input[1];state.sequence=input[2];gate.demo=input[3];gate.stage=std::bit_cast<std::int32_t>(input[4]);shown=input[5];
    }
    // 경계 한 번의 전체 trace를 순서대로 추가한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // Tell의 반환값/팝업/정지는 명시 원본 외부 입력이며 단계/결과 계산을 대신하지 않는다.
    void Show() {
        TellMissionStage(gate,shown,{[this](std::string_view section) {
            Add("tell:"+Hex({reinterpret_cast<const std::uint8_t*>(section.data()),section.size()}));return input[6];
        },[this] { Add("close"); },[this] { Add("pause"); }});
    }
    // 같은 상태에 실제 제품 First와 사건 분기/입력 큐를 실행한다.
    std::string Run() {
        std::string result="void"; // First 자체에는 의미 있는 반환값이 없다.
        if (input[0]==0) MissionFirst(state,[this] { Show(); });
        else if (input[0]==1) {
            const InputEvent event{input[8],0x12345678,std::bit_cast<std::int32_t>(0x87654321U)}; // native의 세 DWORD 입력이다.
            std::function<void()> first;if (input[7]) first=[this] { MissionFirst(state,[this] { Show(); }); };
            if (DispatchMissionFirst(event,first)) result="1";
            else if (input[7]) { Add("input:"+std::to_string(input[8])+":305419896:2271560481");result=std::to_string(input[9]); }
            else result="0";
        } else {
            InputQueue queue;static_cast<void>(QueueMissionFirstKey({input[8],0,0},queue,input[7]!=0));
            if (!queue.Empty()) { const auto event=queue.Pop(false);Add("queue:"+std::to_string(event.code)+":"+std::to_string(event.x)+":"+std::to_string(event.y)); }
            result="slice";
        }
        Put(raw,0x54,state.flags);Put(raw,0x58,state.sequence);Put(raw,0xb4,shown);
        return result+'|'+Hex(raw)+'|'+(trace.empty() ? "-" : trace);
    }
};
}

// 함수 정상 반환264개와 F8 내부 분기14개를 구별한 최초278입력의 전체 바이트/인자를 대조한다.
TEST_CASE(MissionGuideInput_Native278) {
    const auto rows=LoadFixture(NETSTORM_MISSION_FIRST_FIXTURE).rows;CHECK(rows.size()==278);
    // 제품 실행 결과를 동결된 원문과 직접 비교한다. 기대값을 다시 계산하지 않는다.
    for (const auto& row:rows) { CHECK(row.size()==2);FirstScene scene(row[0]);CHECK(scene.Run()==row[1]); }
}

// 실제40칸 큐의 순서/초과 폐기와 release 관문을 함께 확인한다.
TEST_CASE(MissionGuideInput_QueueOrderCapacityAndRelease) {
    InputQueue queue;queue.Push(17,12,13);CHECK(QueueMissionFirstKey({0x770000U,1,2},queue,true));
    CHECK(queue.Pop(false).code==17);const auto first=queue.Pop(false);CHECK(first.code==kMissionFirstCommand && first.x==0 && first.y==0 && queue.Empty());
    CHECK(!QueueMissionFirstKey({InputCode::kRelease|0x770000U,0,0},queue,true));
    CHECK(!QueueMissionFirstKey({0x770000U,0,0},queue,false));CHECK(!QueueMissionFirstKey({0x780000U,0,0},queue,true));CHECK(queue.Empty());
    // 앞선39개 사건이 가득 차면 F8의 새 First는 기존 큐 규칙대로 버린다.
    for (std::uint32_t index=0;index<39;++index) queue.Push(index+100,0,0);
    CHECK(QueueMissionFirstKey({0x770000U,0,0},queue,true));
    // 기존 사건39개의 순서는 F8 예약 시도 뒤에도 그대로다.
    for (std::uint32_t index=0;index<39;++index) CHECK(queue.Pop(false).code==index+100);
    CHECK(queue.Empty());MissionStartState state;MissionFirst(state,{});CHECK(state.flags==1 && !state.sequence);
    state.sequence=1;CHECK(Throws([&] { MissionFirst(state,{}); }));
}
