// 최초 원문264입력과 같은 현재 Client 옵션/실제 Config의 연결을 확인한다.
#include "RawSceneSupport.h"
#include "client/RotationKeyInput.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ScriptCommand.h"
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

// 전체 일반 입력에서 동결한 C/c·수정키·뗌·정지와 Config 재진입/중단을 같은 분리에 재생한다.
TEST_CASE(RotationKeyInput_IndependentNative264Inputs) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/rotation-key-input-x86.tsv").string().c_str());
    std::size_t count{};
    for (const auto& row:fixture.rows) { // 기대값은 원문 출력이며 제품 계산으로 생성하지 않는다.
        CHECK(row.size()==10);if (row.size()!=10) continue;
        auto mode=Number(row[1]);std::string assignment="-";std::uint32_t atCall=0xffffffffU;bool threw=false,handled=false;
        RotationKeyInputHooks hooks;hooks.paused=[&] { return Number(row[2])!=0; };
        hooks.configure=[&](auto text) { assignment=text;atCall=mode;if (count>=256) mode=Number(row[3]);if (Number(row[4])) throw std::runtime_error("원문 Config 명시 중단 입력"); }; //마지막8행만 callback 변경 입력이며 앞256행은 기본 설정 장치다.
        try { handled=DispatchRotationKeyInput({Number(row[0]),37,-19},mode,hooks); }catch (const std::runtime_error&) { threw=true; }
        CHECK(mode==Number(row[5]) && assignment==row[6] && atCall==Number(row[7]));
        CHECK(threw==(Number(row[8])==0));if (!threw) CHECK(handled==!(Number(row[0])&InputCode::kRelease));
        CHECK(Number(row[8])==0 || Number(row[9])==0);++count; //Native 정상 반환의 스택0이며 C++ 스택 배치와 합산하지 않는다.
    }
    CHECK(count==264 && count==fixture.rows.size());
}

// 키/정지 관문은 필요한 장치만 요구하고 설정 실패 시 먼저 쓴 반전값을 유지한다.
TEST_CASE(RotationKeyInput_ExactCodesAndPartialEffects) {
    std::uint32_t mode=0;RotationKeyInputHooks hooks;
    for (const auto code:std::array{InputCode::kRelease|'C',InputCode::kOutside|'c',InputCode::kCommand|'C',0x430000U,std::uint32_t('V')}) { //가상키/명령을 같은 문자로 오인하지 않는다.
        CHECK(!DispatchRotationKeyInput({code,0,0},mode,hooks) && mode==0);
    }
    hooks.paused=[] { return true; };CHECK(DispatchRotationKeyInput({'C',0,0},mode,hooks) && mode==0);
    hooks.paused=[] { return false; };bool threw=false;try { static_cast<void>(DispatchRotationKeyInput({'c',0,0},mode,hooks)); }catch (const std::logic_error&) { threw=true; }CHECK(threw && mode==1);
}

// 창/게임 실행 없이 실제 Client의 현재 전역과 같은 Config parser·동기 옵션 해석을 연결한다.
TEST_CASE(RotationKeyInput_ActualClientConfigAndWorldResetLifetime) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";
    Client client(std::move(options));auto& config=client.Configuration();auto& globals=client.GameplayGlobals()->CursorGlobals();auto& mode=globals.RotationMode();
    CHECK(mode==0);mode=37;config.Set("rotationProbe","1");CHECK(mode==0); //원문435220은 누락 rotmode를 매번0으로 시작한다.
    config.Set("rotmode","-2147483648");CHECK(mode==0x80000000U);globals.ResetForGeneralWorld();CHECK(mode==0x80000000U); //월드/커서 정리는 사용자 옵션을 지우지 않는다.
    RotationKeyInputHooks hooks;hooks.paused=[] { return false; };hooks.configure=[&](auto assignment) { ApplyScriptConfig(assignment,config); };
    CHECK(DispatchRotationKeyInput({'C',0,0},mode,hooks) && mode==0 && config.GetInt("rotmode")==0);
    CHECK(DispatchRotationKeyInput({'c'|InputCode::kControl|InputCode::kShift|InputCode::kAlt,0,0},mode,hooks) && mode==1 && config.GetInt("rotmode")==1);
    config.Set("rotmode","-1");CHECK(mode==0xffffffffU);CHECK(DispatchRotationKeyInput({'C',0,0},mode,hooks) && mode==0 && config.GetInt("rotmode")==0);
}
