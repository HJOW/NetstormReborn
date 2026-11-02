// 원문 전체 정상 부모와 명시 장치 실패/부분 효과를 같은 전환 함수로 확인한다.
#include "RawSceneSupport.h"
#include "client/ScreenModeTransition.h"
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client; // 제품의 실제 화면 전환 부모다.
using namespace netstorm::test::rawscene; // fixture의 원시 수치/문자열만 읽는다.

TEST_CASE(ScreenModeTransition_Native192WholeParent) { // 장치/설정 외부 경계만 대체한384번 PE 실행 결과다.
    const auto rows=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/screen-mode-transition-x86.tsv").string().c_str());std::size_t count{};
    for (const auto& row:rows.rows) { // 원문 입력 전부를 같은 순서로 실행하며 기대값을 재계산하지 않는다.
        CHECK(row.size()==8);if (row.size()!=8) continue;auto flags=Number(row[1]),refresh=Number(row[3]);std::string trace;ScreenModeTransitionHooks hooks;
        hooks.current=[&] { return flags; };hooks.change=[&](auto requested) { trace="M:"+std::to_string(requested);flags=Number(row[2]); };
        hooks.remember=[&](auto name,auto value) { CHECK(name=="startInFullScreen");if (!trace.empty()) trace+=';';trace+="S:"+std::to_string(value);if (Number(row[4])) refresh=Number(row[4]); };
        TransitionScreenMode(Number(row[0]),refresh,hooks);CHECK(trace==row[5]);CHECK(flags==Number(row[6]) && refresh==Number(row[7]));++count;
    }
    CHECK(count==192);
}

TEST_CASE(ScreenModeTransition_RestoreAfterFailureAndReadCurrentAfterCallbacks) { // 실패한 장치의 부분 변경 뒤 진단→현재 창→최초 모드 복구→현재 설정→현재 갱신 순서다.
    std::uint32_t flags=235,refresh=99,calls{};std::string trace;ScreenModeTransitionHooks hooks;
    hooks.current=[&] { trace+='R';return flags; };
    hooks.change=[&](auto value) { trace+='M';++calls;if (calls==1) { CHECK(value==171);flags=138;throw std::runtime_error("명시 첫 장치 실패"); }CHECK(value==235);flags=171; };
    hooks.diagnostic=[&](auto level,auto message) { trace+='D';CHECK(level==29 && message=="screen mode change failed.\n");flags=10; };
    hooks.initialized=[&] { trace+='W';return true; };hooks.remember=[&](auto key,auto value) { trace+='S';CHECK(key=="startInFullScreen" && value==1);refresh=0xffffffffU; };
    TransitionScreenMode(171,refresh,hooks);CHECK(trace=="RRMDWMRS" && calls==2 && flags==171 && refresh==0);
}

TEST_CASE(ScreenModeTransition_NotInitializedKeepsActualFailedDeviceResult) { // 모드 초기화 전에는 복구 CALL이 없고 요청이 아닌 실제 남은 모드를 저장한다.
    std::uint32_t flags=235,refresh=9,changes{},remembered=99;ScreenModeTransitionHooks hooks;hooks.current=[&] { return flags; };
    hooks.change=[&](auto) { ++changes;flags=138;throw std::runtime_error("명시 장치 실패"); };hooks.diagnostic=[](auto,auto) {};hooks.initialized=[] { return false; };
    hooks.remember=[&](auto,auto value) { remembered=value; };TransitionScreenMode(171,refresh,hooks);CHECK(changes==1 && remembered==0 && flags==138 && refresh==10);
}

TEST_CASE(ScreenModeTransition_RecoveryAndRememberFailuresPropagateWithoutIncrement) { // 복구 실패를 새 성공으로 숨기거나 설정 실패 뒤 갱신하지 않는다.
    std::uint32_t flags=235,refresh=19,changes{},diagnostics{},writes{};ScreenModeTransitionHooks hooks;hooks.current=[&] { return flags; };
    hooks.change=[&](auto) { ++changes;throw std::runtime_error("명시 장치 실패"); };hooks.diagnostic=[&](auto,auto) { ++diagnostics; };hooks.initialized=[] { return true; };hooks.remember=[&](auto,auto) { ++writes; };
    bool failed{};try { TransitionScreenMode(171,refresh,hooks); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && changes==2 && diagnostics==1 && !writes && refresh==19);
    hooks.change=[&](auto value) { flags=value; };hooks.remember=[&](auto,auto) { ++writes;throw std::runtime_error("명시 설정 실패"); };failed=false;
    try { TransitionScreenMode(171,refresh,hooks); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && flags==171 && writes==1 && diagnostics==1 && refresh==19);
}

TEST_CASE(ScreenModeTransition_SameModeNeedsNoDeviceAndMissingDeviceIsExplicit) { // 같은 모드도 설정/갱신은 수행하며 실제 장치 누락은 복구 대상으로 삼키지 않는다.
    std::uint32_t refresh{};ScreenModeTransitionHooks hooks;hooks.current=[] { return 235U; };hooks.remember=[](auto,auto value) { CHECK(value==1); };
    TransitionScreenMode(235,refresh,hooks);CHECK(refresh==1);bool failed{};
    try { TransitionScreenMode(171,refresh,hooks); }catch (const std::logic_error&) { failed=true; }CHECK(failed && refresh==1);
}
