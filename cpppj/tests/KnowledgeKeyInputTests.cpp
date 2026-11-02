// F6의 실제 정지 경계/원문16행과 일반 큐 수명을 같은 선택 키 부모에서 확인한다.
#include "RawSceneSupport.h"
#include "client/SelectionKeyInput.h"
#include "client/GameMenuCommands.h"
#include <bit>
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client; // 실제 Client와 같은 공개 입력/명령/큐를 사용한다.
using namespace netstorm::test::rawscene; // fixture 문자열/수치만 해독하며 기대 분기는 만들지 않는다.

TEST_CASE(KnowledgeKeyInput_IndependentNativePauseAndModifiers) { // PE가 먼저 정지를 조회하고 허용된 F6만0/0으로 예약한16행이다.
    const auto fixture=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/knowledge-key-input-x86.tsv").string().c_str());std::size_t count{};
    for (const auto& row:fixture.rows) { // 모든 원문 수정키 조합과 정지 입력을 빠짐없이 재생한다.
        CHECK(row.size()==2);if (row.size()!=2) continue;const auto fields=Split(row[0],',');CHECK(fields.size()==3);if (fields.size()!=3) continue;std::string trace;SelectionKeyInputHooks hooks;
        hooks.paused=[&] { trace="P:"+fields[1];return Number(fields[1])!=0; };hooks.command=[](auto address) { CHECK(address==0x542974);return GameMenuCommandValue(address).value(); };
        hooks.queue=[&](auto code,auto first,auto second) { trace+=";Q:"+std::to_string(code)+':'+std::to_string(first)+':'+std::to_string(second); };
        CHECK(DispatchSelectionKeyInput({Number(fields[0]),37,-19},hooks));CHECK(trace==row[1]);++count;
    }
    CHECK(count==16);
}

TEST_CASE(KnowledgeKeyInput_PausedAndModifiedKeysRequireNoQueueDevices) { // 뗌/다른 사건은 미소비이며 정지와 Control/Shift 거부는 현재 정지 뒤 소비한다.
    SelectionKeyInputHooks hooks;CHECK(!DispatchSelectionKeyInput({0x80750000U,0,0},hooks));CHECK(!DispatchSelectionKeyInput({InputCode::kCommand|0x750000U,0,0},hooks));CHECK(!DispatchSelectionKeyInput({0x780000U,0,0},hooks));
    std::uint32_t reads{};hooks.paused=[&] { ++reads;return true; };CHECK(DispatchSelectionKeyInput({0x750000U,0,0},hooks) && reads==1);
    hooks.paused=[&] { ++reads;return false; };CHECK(DispatchSelectionKeyInput({InputCode::kControl|0x750000U,0,0},hooks));CHECK(DispatchSelectionKeyInput({InputCode::kShift|0x750000U,0,0},hooks));CHECK(reads==3);
}

TEST_CASE(KnowledgeKeyInput_UsesCurrentCommandAndRealFifoWithoutCoordinates) { // 일반 명령은 앞선 사건 뒤에 들어가며 실제 마우스 좌표/owner를 재사용하지 않는다.
    InputQueue queue;queue.Push(0x1234,5,6);bool paused{};std::uint32_t queried{};SelectionKeyInputHooks hooks;hooks.paused=[&] { return paused; };
    hooks.command=[&](auto address) { ++queried;CHECK(address==0x542974);return GameMenuCommandValue(address).value(); };hooks.queue=[&](auto code,auto first,auto second) { queue.PushCommand(code,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };
    CHECK(DispatchSelectionKeyInput({InputCode::kAlt|0x750000U,37,-19},hooks));paused=true;CHECK(DispatchSelectionKeyInput({0x750000U,0,0},hooks) && queried==1);
    auto event=queue.Pop(false);CHECK(event.code==0x1234 && event.x==5 && event.y==6);event=queue.Pop(false);CHECK(event.code==0x020000a0U && !event.x && !event.y && queue.Empty());
}

TEST_CASE(KnowledgeKeyInput_QueueFullAndMissingBoundaryPreservePriorEffects) { // 큐 포화는 기존 FIFO에 맡기고 명령 조회 뒤 누락된 예약 장치를 감추지 않는다.
    InputQueue queue;
    for (std::size_t index=0;index<InputQueue::kCapacity-1;++index) queue.Push('V',0,0); // 기존39개 사건을 새 F6으로 밀어내지 않는다.
    SelectionKeyInputHooks hooks;hooks.paused=[] { return false; };hooks.command=[](auto address) { return GameMenuCommandValue(address).value(); };hooks.queue=[&](auto code,auto first,auto second) { queue.PushCommand(code,static_cast<std::int32_t>(first),static_cast<std::int32_t>(second)); };CHECK(DispatchSelectionKeyInput({0x750000U,0,0},hooks));std::size_t count{};
    while (!queue.Empty()) { CHECK(queue.Pop(false).code=='V');++count; } // 이미 포화된 큐는 기존 순서와 개수를 보존한다.
    CHECK(count==39);hooks.queue={};std::uint32_t reads{};hooks.command=[&](auto address) { ++reads;return GameMenuCommandValue(address).value(); };bool failed{};try { static_cast<void>(DispatchSelectionKeyInput({0x750000U,0,0},hooks)); }catch (const std::logic_error&) { failed=true; }CHECK(failed && reads==1);
}
