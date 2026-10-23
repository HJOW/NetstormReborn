// 실제 원문 앞단의 폴링/현재 기록 재조회/같은 큐 효과를 제품 모듈과 대조한다.
#include "RawSceneSupport.h"
#include "client/GumpPointerBoundary.h"
#include <bit>
#include <stdexcept>
#include <vector>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 원문 fixture의 세 DWORD를 signed 좌표 비트 손실 없이 읽는다.
InputEvent PointerEvent(std::string_view field) {
    const auto words=Split(field,',');if (words.size()!=3) throw std::runtime_error("포인터 fixture 사건 필드 오류");
    return {Number(words[0]),std::bit_cast<std::int32_t>(Number(words[1])),std::bit_cast<std::int32_t>(Number(words[2]))};
}
// signed 좌표를 해석하지 않고 실제 원문과 같은 DWORD 문자열로 관찰한다.
std::string PointerWords(InputEvent event) {
    return std::to_string(event.code)+','+std::to_string(std::bit_cast<std::uint32_t>(event.x))+','+std::to_string(std::bit_cast<std::uint32_t>(event.y));
}
// 사건 세 필드를 각각 보존하는 값 비교다. 명령 code 마스크를 적용하지 않는다.
bool SamePointer(InputEvent first,InputEvent second) { return first.code==second.code && first.x==second.x && first.y==second.y; }
}
// 제품 기대 계산 없이 독립 PE3878입력의 최종 기록·전체 queue·원문 callback 순서를 비교한다.
TEST_CASE(GumpPointerBoundary_Native3878) {
    std::size_t count{}; // 실제 비교한 fixture 입력 수다.
    for (const auto& row:LoadFixture(NETSTORM_GUMP_POINTER_BOUNDARY_FIXTURE).rows) { // 기존 원문을 실행 중 수정하지 않는다.
        CHECK(row.size()==18);if (row.size()!=18) return;
        GumpPointerBoundaryState state{Number(row[1]),Number(row[2]),PointerEvent(row[3]),PointerEvent(row[4])};
        const auto language=Number(row[0]),mutation=Number(row[9]),prefill=Number(row[10]); // 명시 입력이며 결과를 계산하지 않는다.
        const auto primary=PointerEvent(row[5]),control=PointerEvent(row[6]),left=PointerEvent(row[7]),right=PointerEvent(row[8]);
        InputQueue queue;std::uint32_t polls{},queued{};std::string trace; // 실제40칸 큐와 외부 경계 관찰이다.
        for (std::uint32_t index=0;index<prefill;++index) queue.Push(0x02000100U+index,static_cast<std::int32_t>(index),static_cast<std::int32_t>(index+100)); // 포화 입력의 기존 사건이다.
        const auto record=[&](std::string item) { if (!trace.empty()) trace+=';';trace+=item; }; // 원문 외부 효과 순서만 직렬화한다.
        const auto mutate=[&](std::string_view stage) { // 원문과 같은 callback 입력의 현재 기록 변경만 주입한다.
            if (mutation==1 && stage=="P3") state.leftOutside={0x10000,41,42};
            else if (mutation==2 && stage=="Q1") state.rightOutside={0x80020000,51,52};
            else if (mutation==3 && stage=="Q1") state.rightOutside={0x20000,61,62};
            else if (mutation==4 && stage=="P3") { state.leftOutside={0x80010000,71,72};state.wasInside=0x12345678;state.initializationFlags=0x40000003; }
            else if (mutation==5 && stage=="P1") { state.leftOutside={0x80010000,81,82};state.rightOutside={0x80020000,83,84};state.initializationFlags=0x80000002; }
            else if (mutation==6 && stage=="Q1") { state.wasInside=0x12345678;state.initializationFlags=0x40000003; }
            else return;
            record("M"+std::to_string(mutation));
        };
        GumpPointerBoundaryHooks hooks; // 폴링은 명시 입력만 공급하고 제품 분기/기록을 대신 계산하지 않는다.
        hooks.poll=[&](std::uint32_t code) {
            ++polls;InputEvent event; // 같은 requested code에 주어진 현재 사건이다.
            switch(code) { case 0x120000:event=primary;break;case 0x110000:event=control;break;case 0x10000:event=left;break;case 0x20000:event=right;break;default:throw std::logic_error("원문 밖 포인터 폴링 요청"); }
            record("P,"+std::to_string(code)+','+PointerWords(event));mutate("P"+std::to_string(polls));return event;
        };
        hooks.queue=[&](InputEvent event) { ++queued;record("Q,"+PointerWords(event));mutate("Q"+std::to_string(queued));queue.Push(event.code,event.x,event.y); };
        const auto pointer=PrepareGumpPointerBoundary(state,language,hooks);
        CHECK(PointerWords(pointer)==row[11]);CHECK(state.initializationFlags==Number(row[12]) && state.wasInside==Number(row[13]));
        CHECK(PointerWords(state.leftOutside)==row[14] && PointerWords(state.rightOutside)==row[15]);CHECK(trace==row[17]);
        std::string actual; // 전체 기존 사건/보정 사건을 같은 큐에서 꺼내 비교한다.
        while (!queue.Empty()) { if (!actual.empty()) actual+=';';actual+=PointerWords(queue.Pop(false)); } // release도 버리지 않고 원문 순서를 읽는다.
        CHECK((actual.empty() ? "-" : actual)==row[16]);++count;
    }
    CHECK(count==3878);
}
// 밖으로 나갈 때 기록하고 밖에 계속 있으면 유지하며 재진입에서만 같은 raw release를 기존 사건 뒤에 넣는다.
TEST_CASE(GumpPointerBoundary_ReentryUsesCurrentRawQueue) {
    GumpPointerBoundaryState state;InputQueue queue;std::vector<std::uint32_t> calls;std::uint32_t phase{};
    CHECK(state.wasInside==1 && !state.initializationFlags); //54354c의 실제 PE 초기값은 BSS0이 아니다.
    const auto poll=[&](std::uint32_t code)->InputEvent {
        calls.push_back(code);
        if (code==0x120000) return {phase<2 ? 0x84120000U : 0x80120000U,11,12};
        if (code==0x10000) return phase==0 ? InputEvent{0x10000,21,22} : InputEvent{0x98010000,-1,42};
        if (code==0x20000) return phase==0 ? InputEvent{0x20000,31,32} : InputEvent{0xa4020000,51,-2};
        throw std::logic_error("중간 버튼은 이 보정의 폴링 대상이 아님");
    };
    static_cast<void>(PrepareGumpPointerBoundary(state,0,poll,queue));CHECK(!state.wasInside && state.initializationFlags==3 && queue.Empty());
    const auto left=state.leftOutside,right=state.rightOutside;calls.clear();phase=1;
    static_cast<void>(PrepareGumpPointerBoundary(state,0,poll,queue));CHECK(calls==std::vector<std::uint32_t>({0x120000}));
    CHECK(SamePointer(left,state.leftOutside) && SamePointer(right,state.rightOutside) && queue.Empty());
    queue.Push(0x02000095,-3,99);calls.clear();phase=2;const auto pointer=PrepareGumpPointerBoundary(state,0,poll,queue);
    CHECK(pointer.code==0x80120000 && pointer.x==11 && pointer.y==12 && state.wasInside==1);CHECK(calls==std::vector<std::uint32_t>({0x120000,0x10000,0x20000}));
    CHECK(SamePointer(queue.Pop(false),{0x02000095,-3,99}));CHECK(SamePointer(queue.Pop(false),{0x98010000,-1,42}));CHECK(SamePointer(queue.Pop(false),{0xa4020000,51,-2}));CHECK(queue.Empty());
    CHECK(SamePointer(left,state.leftOutside) && SamePointer(right,state.rightOutside)); // 재진입은 outside 기록을 현재 release로 덮지 않는다.
}
// language3은 두 폴링이 모두 눌렸을 때 첫 code의 release만 더하며 실제 도달한 큐 경계만 요구한다.
TEST_CASE(GumpPointerBoundary_LanguageAndReachedCallbacks) {
    GumpPointerBoundaryState state;std::vector<std::uint32_t> calls;GumpPointerBoundaryHooks hooks;
    hooks.poll=[&](std::uint32_t code) { calls.push_back(code);return code==0x120000 ? InputEvent{0x58120000,-8,9} : InputEvent{0x110000,100,200}; };
    const auto pointer=PrepareGumpPointerBoundary(state,3,hooks);CHECK(pointer.code==0xd8120000 && pointer.x==-8 && pointer.y==9);CHECK(calls==std::vector<std::uint32_t>({0x120000,0x110000}));
    bool missing{};try { static_cast<void>(PrepareGumpPointerBoundary(state,0,GumpPointerBoundaryHooks{})); } catch (const std::invalid_argument&) { missing=true; }CHECK(missing);
    state={3,0,{0x10000,1,2},{0x80020000,3,4}};hooks.poll=[](std::uint32_t code) { return InputEvent{code|0x80000000U,5,6}; };missing=false;
    try { static_cast<void>(PrepareGumpPointerBoundary(state,0,hooks)); } catch (const std::logic_error&) { missing=true; }CHECK(missing && !state.wasInside);
}
