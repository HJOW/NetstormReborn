// 실제 PE Guide Run/Advance의 전체 두 객체 상태와 current/pending/가상 단계 경계 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawMissionGuide.h"
#include <array>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::size_t kGuideMissionBytes=0xd0; // native 입력의 전체 미션 물리 크기이며 주변 stale BYTE도 비교한다.
constexpr std::uint32_t kGuideNativeVtable=0x12002000; // native의 명시 입력 가상 표 번호이며 호스트 함수 주소가 아니다.
// 현재 비교하는 명시 입력은13DWORD다. 기대 분기나 결과를 여기서 계산하지 않는다.
struct GuideScene {
    std::array<std::uint32_t,13> input{}; // active/두flags·sequence/pending/dialog/trigger/action/value/ordinal/budget/runs다.
    MissionStartState first,second; // Start와 같은 실제 다섯 상태 필드이며 별도 Guide 단계 사본을 두지 않는다.
    std::uint32_t active{},budget{},getters{}; // 현재 포인터의 입력 번호와 외부 효과 한도/실제 조회 횟수다.
    std::string trace; // 원문 current 읽기와 실제 외부 호출 위치에서만 순서를 누적한다.
    // 한 행의 독립 입력을 읽고 두 객체의 다섯 실제 필드를 같은 명시 패턴으로 준비한다.
    explicit GuideScene(std::string_view values) {
        const auto fields=Split(values,',');if (fields.size()!=input.size()) throw std::invalid_argument("Guide 입력 DWORD 수 오류");
        // 입력 필드를 원문 DWORD로 읽으며 부호 확장이나 기대값 보정을 하지 않는다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);
        first={input[1],input[2],0x12345679,0x89abcdee,0x13579be0};
        second={input[3],input[4],0x1234567a,0x89abcded,0x13579be1};active=input[0];budget=input[11];
    }
    // 실제 경계 호출 한 번의 trace를 이어 붙인다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 명시 식별자에 해당하는 현재 미션을 빌린다. 범위 밖 포인터는 성공으로 바꾸지 않는다.
    MissionStartState* Object(std::uint32_t identifier) {
        if (identifier>2) throw std::out_of_range("Guide 현재 입력 번호 범위 오류");
        return identifier==0 ? nullptr : identifier==1 ? &first : &second;
    }
    // 외부 가상 호출이 받은 실제 대상과 현재 포인터가 다를 수 있으므로 참조에서 번호를 찾는다.
    std::uint32_t Identifier(MissionStartState& state) const {
        if (&state==&first) return 1;if (&state==&second) return 2;throw std::logic_error("Guide 실제 가상 대상 누락");
    }
    // fixture에 적힌 외부 효과만 적용한다. action1은 제품의 실제 Advance를 호출하며 기대 출력을 쓰지 않는다.
    void Mutate(std::uint32_t trigger,std::uint32_t target) {
        if (trigger!=input[7] || budget==0 || (trigger==6 && getters!=input[10])) return;
        --budget;auto* state=Object(target);const auto action=input[8]; // 원본 경계에 공급한 실제 변경 입력이다.
        if (action==1 && state) RawMissionGuide::Advance(*state);
        else if (action==2 && state) state->flags|=1U;
        else if (action==3 && state) state->flags|=8U;
        else if (action==4 && state) state->sequence=input[9];
        else if (action==5) active=0;
        else if (action==6) active=target==2 ? 1U : 2U;
        else if (action==8 && state) { state->flags|=1U;state->sequence=input[9]; }
    }
    // 원본의 명시 외부 경계만 조립한다. stage/pending/dialog 몸체 자체를 복원했다고 주장하지 않는다.
    MissionGuideHooks Hooks() {
        MissionGuideHooks hooks;
        hooks.current=[this] { ++getters;Mutate(6,active);Add("C,"+std::to_string(active));return Object(active); };
        hooks.pending=[this] { Add("P");Mutate(1,active);return input[5]; };
        hooks.dialogsClear=[this] { Add("D");Mutate(2,active);return input[6]!=0; };
        hooks.stage=[this](MissionStartState& state,std::uint32_t sequence) {
            const auto target=Identifier(state);Add("S,"+std::to_string(target)+','+std::to_string(sequence));Mutate(0,target);
        };
        hooks.postpone=[this](MissionStartState& state) { const auto target=Identifier(state);Add("H,"+std::to_string(target));Mutate(3,target); };
        hooks.release=[this] { const auto target=active;Add("L");active=0;Mutate(5,target); };
        hooks.invalidStage=[this] { Add("A");Mutate(4,active); };
        return hooks;
    }
    // 같은 호스트 상태를 원본 물리 필드 위치에 써 전체208BYTE 표현으로 비교한다.
    std::string Observe(std::uint32_t identifier) {
        std::array<std::uint8_t,kGuideMissionBytes> bytes{};const auto seed=identifier==1 ? 0x29U : 0xa7U; // 원본 준비와 같은 독립 stale 입력 패턴이다.
        // 구현의 다섯 필드 밖 모든BYTE는 native 전체 객체 관찰과 함께 보존 여부를 확인한다.
        for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(index*37+seed);
        const auto& state=*Object(identifier);Put(bytes,0,kGuideNativeVtable);Put(bytes,0x54,state.flags);Put(bytes,0x58,state.sequence);
        Put(bytes,0xa0,state.tellAtIndex);Put(bytes,0xc0,state.fieldC0);Put(bytes,0xc4,state.fieldC4);return Hex(bytes);
    }
};
}
// 정상 반환한 독립 native 전체 출력과 현재 getter/짧은평가/단계/진단/해제 순서가 모두 같아야 한다.
TEST_CASE(MissionGuide_Native1550) {
    std::size_t compared=0;
    // 예상 결과는 C++ 규칙으로 계산하지 않고 보호 PE에서 별도로 보존한 fixture를 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_GUIDE_FIXTURE).rows) {
        if (row[0]=="kind") continue;CHECK(row.size()==6);GuideScene scene(row[1]);
        RawMissionGuide guide(OriginalEdition::Patch1078,scene.Hooks());
        // 반복 Run도 별도 원본 호출이며 stage5→6 경계 뒤 다음 Run을 앞당기지 않는다.
        for (std::uint32_t run=0;run<scene.input[12];++run) {
            if (row[0]=="R") guide.Run();else if (row[0]=="A") RawMissionGuide::Advance(scene.first);else throw std::invalid_argument("Guide 원문 함수 종류 오류");
        }
        CHECK(scene.active==Number(row[2]));CHECK(scene.Observe(1)==row[3]);CHECK(scene.Observe(2)==row[4]);
        CHECK((scene.trace.empty() ? "-" : scene.trace)==row[5]);++compared;
    }
    CHECK(compared==1550);
}
// stage5의 Advance는 sequence6까지만 만들며 같은 Run에 stage6/release를 호출하지 않는다.
TEST_CASE(MissionGuide_StageFiveDefersReleaseUntilNextRun) {
    GuideScene scene("1,0,5,268435456,2,0,1,0,1,0,1,1,1");RawMissionGuide guide(OriginalEdition::Patch1078,scene.Hooks());
    guide.Run();CHECK(scene.first.sequence==6 && scene.active==1);
    CHECK(scene.trace=="C,1;P;D;C,1;C,1;C,1;S,1,5;C,1");scene.trace.clear();
    guide.Run();CHECK(scene.active==0);CHECK(scene.trace=="C,1;P;D;C,1;C,1;C,1;S,1,6;L;C,0");
}
// switch가 선택한 번호와 실제 가상 호출 대상의 현재 조회는 독립 경계다.
TEST_CASE(MissionGuide_CurrentGetterSeparatesStageSelectionAndTarget) {
    GuideScene scene("1,0,0,268435456,2,0,1,6,6,0,4,1,1");RawMissionGuide guide(OriginalEdition::Patch1078,scene.Hooks());guide.Run();
    CHECK(scene.active==2);CHECK(scene.first.sequence==0 && scene.second.sequence==2);
    CHECK(scene.trace=="C,1;P;D;C,1;C,1;C,2;S,2,0;C,2;C,2;P;D;C,2;C,2;C,2;S,2,2;C,2");
}
// 아직 없는 원본 UI/가상몸체를 빈 성공으로 바꾸지 않도록 모든 경계와 판본을 생성 시 확인한다.
TEST_CASE(MissionGuide_RequiresEveryBoundaryAndPatchEdition) {
    GuideScene scene("1,0,0,268435456,2,0,1,0,0,0,1,1,1");
    CHECK(Throws([&] { RawMissionGuide guide(OriginalEdition::Cd1072,scene.Hooks()); }));
    // 각 필수 경계 하나만 누락해도 생성이 거부되어야 한다.
    for (std::size_t index=0;index<7;++index) {
        auto hooks=scene.Hooks();
        switch (index) {
        case 0:hooks.current={};break;case 1:hooks.pending={};break;case 2:hooks.dialogsClear={};break;
        case 3:hooks.stage={};break;case 4:hooks.postpone={};break;case 5:hooks.release={};break;case 6:hooks.invalidStage={};break;
        }
        CHECK(Throws([&] { RawMissionGuide guide(OriginalEdition::Patch1078,hooks); }));
    }
}
