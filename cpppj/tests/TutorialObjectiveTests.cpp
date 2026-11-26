// 신규 안내의 범위·예약·표시 부호화와 기존 지도 입력 보존을 확인한다.
#include "TestSupport.h"
#include "client/TutorialObjective.h"
#include "client/GumpEvents.h"
#include "o/OriginalText.h"
#include <map>
#include <set>
using namespace netstorm::client;

// 공식40단계가 빠짐없이 존재하고 완료 이후에는 이전 안내가 남지 않아야 한다.
TEST_CASE(TutorialObjective_OfficialStagesAndCompletion) {
    const std::array<int,6> expected{6,8,7,8,6,5}; // 실제 판정기의 A부터 마지막 행동 단계까지의 수다.
    std::set<std::pair<std::uint32_t,char>> seen;
    for (const auto& entry:TutorialObjectives()) { // 모든 표시 단계는 하나의 실제 미션/단계에만 대응한다.
        CHECK(seen.emplace(entry.tutorial,entry.stage).second);CHECK(!entry.english.empty());
    }
    CHECK(seen.size()==40);
    for (std::uint32_t number=1;number<=6;++number) { // 여섯 미션의 마지막 완료 섹션도 각각 확인한다.
        netstorm::o::MissionFrameGateState gate;gate.started=1;gate.lockFrames=0;gate.tutorialNumber=number;
        for (int index=0;index<expected[number-1];++index) { // 실제 존재하는 모든 단계의 문구를 선택한다.
            gate.stage='A'+index;CHECK(!TutorialObjectiveKey("Tutorial"+std::to_string(number),gate,0).empty());
        }
        gate.stage='A'+expected[number-1];CHECK(TutorialObjectiveKey("tutorial"+std::to_string(number),gate,0).empty());
    }
}
// 일반 캠페인의 Tutorial 유형, 다른 맵·데모·편집·시작 전 상태는 안내 대상이 아니다.
TEST_CASE(TutorialObjective_ScopeAndPendingTimerReadOnly) {
    netstorm::o::MissionFrameGateState gate;gate.started=1;gate.lockFrames=0;gate.tutorialNumber=2;gate.stage='F';
    const auto instruction=TutorialObjectiveKey("tutorial2",gate,0);CHECK(instruction.find("4 seconds")!=std::string_view::npos);
    CHECK(TutorialObjectiveKey("thewarbegins",gate,0).empty());CHECK(TutorialObjectiveKey("tutorial1",gate,0).empty());CHECK(TutorialObjectiveKey("tutorial2",gate,1).empty());
    gate.demo=1;CHECK(TutorialObjectiveKey("tutorial2",gate,0).empty());gate.demo=0;
    gate.started=0;CHECK(TutorialObjectiveKey("tutorial2",gate,0).empty());gate.started=1;
    gate.timerSeconds=42.5;CHECK(TutorialObjectiveKey("tutorial2",gate,0)=="Wait for the next tutorial message.");CHECK(gate.timerSeconds==42.5 && gate.stage=='F' && gate.lockFrames==0);
    gate.stage='C';CHECK(TutorialObjectiveKey("tutorial2",gate,0).find("production")!=std::string_view::npos); // 잘못 신전을 고른 교정 타이머는 생산 목표를 대체하지 않는다.
    gate.timerSeconds=0;gate.lockFrames=10;CHECK(TutorialObjectiveKey("tutorial2",gate,0)=="Wait for the next tutorial message.");CHECK(gate.lockFrames==10);
}
// 영문 단어·긴 단어·명시 줄바꿈과4바이트 확장 문자가 폭 제한에서 깨지지 않는다.
TEST_CASE(TutorialObjective_WrapPreservesExtendedCharacters) {
    const auto ascii=WrapTutorialObjective("abc def ghi",7,[](auto line) { return static_cast<int>(line.size()); });CHECK(ascii.size()==2 && ascii[0]=="abc def" && ascii[1]=="ghi");
    const auto longWord=WrapTutorialObjective("abcdefgh",3,[](auto line) { return static_cast<int>(line.size()); });CHECK(longWord.size()==3 && longWord[0]=="abc" && longWord[2]=="gh");
    const auto encoded=netstorm::o::EncodeDisplayText("다음단계");const std::string bytes(encoded.begin(),encoded.end());
    const auto wrapped=WrapTutorialObjective(bytes,24,[](auto line) { return static_cast<int>(line.size()/4)*12; });CHECK(wrapped.size()==2);
    CHECK(wrapped[0]+wrapped[1]==bytes);CHECK(wrapped[0].size()==8 && wrapped[1].size()==8);
    CHECK(WrapTutorialObjective("first\nsecond",20,[](auto line) { return static_cast<int>(line.size()); }).size()==2);
}
// 안내가 월드보다 앞에 보여도 같은 위치의 클릭·우클릭 대상은 원래 지도 노드여야 한다.
TEST_CASE(TutorialObjective_TransparentGumpDoesNotPickOrBlockChild) {
    std::map<std::uint32_t,GumpCreateNode> nodes;GumpCreateState create;GumpLifetimeState lifetime;GumpEventState state;
    create.entries[0]=1;create.collection.root=1;nodes[1].child=2;
    auto& hint=nodes[2];hint.parent=1;hint.next=3;hint.flags=0x80000001U;hint.width=hint.height=100;hint.depth=static_cast<std::uint32_t>(-50);hint.serial=12;
    auto& map=nodes[3];map.parent=1;map.flags=1;map.width=map.height=100;map.depth=static_cast<std::uint32_t>(-40);
    const auto lookup=[&](auto token)->GumpCreateNode* { const auto it=nodes.find(token);return it==nodes.end() ? nullptr:&it->second; }; // 모든 처리기가 같은 실제 노드를 읽는다.
    GumpCollection collection(create.collection,{lookup,{}});GumpEvents events(create,lifetime,state,collection,{lookup,{},{}});
    CHECK(events.Pick({10,10},0)==2); // 원본 flags의 상위 비트도 선택 의미를 바꾸지 않는다.
    events.SetDisplayOnly(2,12,true);
    CHECK(events.Pick({10,10},0)==3);CHECK(events.PickTopLevel({10,10})==3);
    hint.child=4;auto& child=nodes[4];child.parent=2;child.flags=1;child.width=child.height=100;child.depth=static_cast<std::uint32_t>(-60);
    CHECK(events.Pick({10,10},0)==4); // 표시 전용 부모 아래 일반 상호작용 자식이 있을 경우 재귀는 보존한다.
    hint.child=0;hint.serial=13;CHECK(events.Pick({10,10},0)==2); // 같은 토큰을 재사용한 새 객체에는 이전 표시 전용 등록이 적용되지 않는다.
    events.SetDisplayOnly(2,12,false);CHECK(events.Pick({10,10},0)==2);
}
