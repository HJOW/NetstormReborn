// 새 원문684입력 중 C/Tab/숫자 후보를 같은 공개 helper에 재생한다. 게임/OS/MP는 실행하지 않는다.
#include "client/ConstructionHistoryKey.h"
#include "client/SavedCameraKey.h"

namespace { // 원문 앞단/커서 분류와 helper의 국소 효과를 구별하는 집중 검사 도구다.
// 명시 인자 없는 관측을 원문과 같은 세미콜론 순서로 축적한다.
void Outside761Trace(std::string& trace,std::string_view event) { if (!trace.empty()) trace+=';';trace+=event; }
// Guide/Gump/커서 분류는 실제 helper 밖이므로 새 fixture의 국소 효과만 읽는다.
std::string Outside761Effects(std::string_view trace) {
    std::string result;std::size_t first{};
    while (first<trace.size()) { // 순서를 바꾸지 않고 명시 앞단/후단 관측만 제거한다.
        const auto end=trace.find(';',first);const auto piece=trace.substr(first,end==trace.npos ? trace.size()-first : end-first);
        if (piece!="Guide" && piece!="Gump" && !piece.starts_with("Cursor:")) Outside761Trace(result,piece);
        if (end==trace.npos) break;first=end+1;
    }
    return result;
}
// 이 좁은 helper 검사는 선행 Guide/Gump/모달 소비가 없는 원문 행만 사용한다.
bool Outside761Row(const std::vector<std::string>& row) { CHECK(row.size()==13);return row.size()==13 && row[1]=="0" && row[2]=="0" && row[3]=="0"; }
}

// outside C/c도 정지→반전값 저장→Config 순서를 원문처럼 실행하고 뗌은 거부한다.
TEST_CASE(Outside761_RotationNative32Rows) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/outside-input-x86.tsv").string().c_str());std::size_t count{};
    for (const auto& row:fixture.rows) { // Native의 같은 C/c 입력과 현재 결과를 전부 대조한다.
        if (!Outside761Row(row)) continue;const auto event=Number(row[0]),key=event&0x03ffffffU;if (key!='C' && key!='c') continue;
        std::uint32_t mode{};std::string trace;RotationKeyInputHooks hooks;hooks.paused=[&] { Outside761Trace(trace,"Pause:"+row[4]);return Number(row[4])!=0; };
        hooks.configure=[&](auto assignment) { CHECK(assignment=="rotmode=1");Outside761Trace(trace,"FormatRotation:"+std::to_string(mode));Outside761Trace(trace,"Config"); };
        const auto handled=DispatchRotationKeyInput({event,37,-19},mode,hooks);CHECK(handled==!(event&InputCode::kRelease));CHECK(mode==Number(row[8]));CHECK(trace==Outside761Effects(row[6]));++count;
    }
    CHECK(count==32);
}

// outside Tab도 현재 Point 판정과 중심 이후 순번 증가를 실행한다.
TEST_CASE(Outside761_HistoryNative16Rows) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/outside-input-x86.tsv").string().c_str());std::size_t count{};
    for (const auto& row:fixture.rows) { // 저장 숫자10칸과 별개인 다섯 최근 Point 장부만 사용한다.
        if (!Outside761Row(row)) continue;const auto event=Number(row[0]);if ((event&0x03ffffffU)!=9) continue;
        netstorm::o::ConstructionHistoryState history;history.index=0;history.points[0]={41.25F,41.25F};std::int32_t cells=256;std::string trace;ConstructionHistoryKeyHooks hooks;
        hooks.paused=[&] { Outside761Trace(trace,"Pause:"+row[4]);return Number(row[4])!=0; };hooks.center=[&](auto x,auto y) { CHECK(x==41.25F && y==41.25F);Outside761Trace(trace,"Center"); };
        const auto handled=DispatchConstructionHistoryKey({event,37,-19},history,cells,hooks);CHECK(handled==!(event&InputCode::kRelease));CHECK(history.index==Number(row[10]));CHECK(trace==Outside761Effects(row[6]));++count;
    }
    CHECK(count==16);
}

// outside 숫자는 정지/언어 조회 뒤 종료하며 raw 슬롯 계산을 보존해 음향·캡처·중심·쓰기0을 유지한다.
TEST_CASE(Outside761_SavedCameraNative32RowsAndNoOutsideWrites) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/outside-input-x86.tsv").string().c_str());std::size_t count{},outsideCount{};
    for (const auto& row:fixture.rows) { // inside '1'/Shift+'!'의 원래 효과도 함께 대조한다.
        if (!Outside761Row(row)) continue;const auto event=Number(row[0]),key=event&0x03ffffffU;if (key!='1' && key!='!') continue;
        SavedCameraPositions saved;for (std::size_t slot=0;slot<saved.points.size();++slot) saved.points[slot]={41.25F+static_cast<float>(slot),71.5F+static_cast<float>(slot)}; // Native와 같은 전체80BYTE 명시 입력이다.
        const auto initial=saved;std::string trace;SavedCameraKeyHooks hooks;hooks.paused=[&] { Outside761Trace(trace,"Pause:"+row[4]);return Number(row[4])!=0; };hooks.language=[&] { Outside761Trace(trace,"Language:4294967295");return 0; }; //원문 조회 인자는 -1이며 반환 언어0과 구별한다.
        hooks.sound=[&] { Outside761Trace(trace,"Sound"); };hooks.capture=[&] { Outside761Trace(trace,"Capture");return netstorm::o::ConstructionNoticePoint{61.25F,71.5F}; };hooks.center=[&](auto x,auto y) { CHECK(x==41.25F && y==71.5F);Outside761Trace(trace,"Center"); };
        const auto handled=DispatchSavedCameraKey({event,37,-19},saved,hooks);CHECK(handled==!(event&InputCode::kRelease));CHECK(trace==Outside761Effects(row[6]));
        for (std::size_t slot=0;slot<saved.points.size();++slot) { // Native 쓰기2인 안쪽 Shift 저장만 첫 Point의 두DWORD를 바꾼다.
            const auto changed=Number(row[11])==2 && slot==0;CHECK(saved.points[slot].x==(changed ? 61.25F : initial.points[slot].x));CHECK(saved.points[slot].y==initial.points[slot].y);
        }
        if (event&InputCode::kOutside) { CHECK(Number(row[11])==0);++outsideCount; }++count;
    }
    CHECK(count==32 && outsideCount==16);
}
