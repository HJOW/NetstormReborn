// 15차 Reset All: 사용자 파일 범위, 레거시 재이관 차단, 실패 경계와 실제 메뉴/확인 본문을 창 없이 검사한다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ClientAudio.h"
#include "client/ClientOptionsDisk.h"
#include "client/ClientStaticStrings.h"
#include "client/ProductPresentation.h"
#include "client/RawGameMenus.h"
#include "client/ResetAllSettings.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>

namespace { // 각 검사가 독립 생성한 파일만 사용하며 보호 자료를 읽거나 수정하지 않는다.
using namespace netstorm;
// 충돌 없는 한글 임시 루트다. 검사 산출물을 남겨 실패 원시 파일도 확인할 수 있게 한다.
std::filesystem::path ResetWork(std::string_view name) {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count(); // 이름 충돌 방지용 호스트 시각이며 게임 난수와 무관하다.
    return std::filesystem::path(__FILE__).parent_path().parent_path()/"build/test-data"/("reset-all-"+std::string(name)+'-'+std::to_string(stamp))/L"한글😀";
}
// 검사 자료의 상위 폴더를 만든 뒤 지정 바이트만 기록한다.
void ResetWrite(const std::filesystem::path& path,std::string_view bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path,std::ios::binary);file.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));CHECK(file.good());
}
// 파일의 실제 바이트를 읽는다. 파일 부재와 빈 파일은 호출자에서 별도로 구별한다.
std::string ResetRead(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
// 창/음향 없이 실제 Client의 사용자 설정 로더를 구성한다.
client::ClientOptions ResetClientOptions(const std::filesystem::path& install,const platform::UserDataRoot& user) {
    client::ClientOptions options;options.gameDirectory=install;options.noAudio=true;options.userData=user;return options;
}
// 실제 Options 작성기를 실행해 메뉴 항목의 표시와 Tell 인자를 현재 순서대로 수집한다.
std::vector<std::pair<std::string,std::string>> ResetMenuRows(bool enabled) {
    client::GameMenusHooks hooks;std::vector<std::pair<std::string,std::string>> rows; // 수집한 실제 제품 출력이며 기대 목록을 계산하지 않는다.
    hooks.global=[](auto address) { return address==0x542948 ? 0x02000095U:0U; };
    hooks.config=[](auto) { return 0; };hooks.readInt=[](auto,std::int32_t&) {};hooks.translate=[](auto text) { return std::string(text); };
    hooks.vertical=[] {};hooks.separator=[] {};hooks.take=[] { return 1U; };hooks.submenu=[](auto,auto,auto,auto) {};
    hooks.item=[&](auto text,auto,const client::GameMenuParameter& parameter,auto,auto) { rows.emplace_back(text,std::holds_alternative<std::string>(parameter) ? std::get<std::string>(parameter):""); };
    hooks.checked=[](auto,auto,auto,const auto&,auto) {};hooks.resetAllOption=enabled;
    CHECK(client::RawGameMenus(std::move(hooks)).Options()==1);return rows;
}
}

// 초기화는 사용자 설정과 재접속 파일만 바꾸고 제작한 맵·로그·기타 파일·설치 옵션을 보존한다.
TEST_CASE(ResetAll_OnlyExplicitUserSettingsAndProgressAreCleared) {
    const auto base=ResetWork("scope");const platform::UserDataRoot user(base/"user"); // 이번 검사의 독립 사용자 루트다.
    const auto installOptions=base/"install/d/options.cfg"; // 설치의 옛 옵션도 삭제 대상에 넣지 않는지 확인한다.
    ResetWrite(installOptions,"legacy-settings");ResetWrite(user.OptionsFile(),"old-settings-and-Done");
    ResetWrite(user.SavedStateFile(),"checkpoint");ResetWrite(user.FullScreenStateFile(),"fullscreen");ResetWrite(user.Saves()/L"!rootservers.dat","servers");
    ResetWrite(user.CustomMaps()/"mine.fort","fort");ResetWrite(user.CustomMaps()/"mine.english","scenario");ResetWrite(user.Logs()/"test.log","log");ResetWrite(user.Saves()/"unrelated.bin","unrelated");
    client::ResetAllUserSettings(user);
    CHECK(std::filesystem::is_regular_file(user.OptionsFile()) && std::filesystem::file_size(user.OptionsFile())==0);
    CHECK(!std::filesystem::exists(user.SavedStateFile()) && !std::filesystem::exists(user.FullScreenStateFile()) && !std::filesystem::exists(user.Saves()/L"!rootservers.dat"));
    CHECK(ResetRead(installOptions)=="legacy-settings" && ResetRead(user.CustomMaps()/"mine.fort")=="fort" && ResetRead(user.CustomMaps()/"mine.english")=="scenario");
    CHECK(ResetRead(user.Logs()/"test.log")=="log" && ResetRead(user.Saves()/"unrelated.bin")=="unrelated");
}

// 파일이 아직 없는 첫 실행의 초기화도 성공하고 반복 요청이 다른 경로를 지우지 않는다.
TEST_CASE(ResetAll_EmptyRootAndRepeatedResetKeepAnEmptyUserOptionsFile) {
    const platform::UserDataRoot user(ResetWork("empty")/"user"); // 존재하지 않는 새 루트다.
    client::ResetAllUserSettings(user);client::ResetAllUserSettings(user);
    CHECK(std::filesystem::is_directory(user.Saves()) && std::filesystem::file_size(user.OptionsFile())==0);
}

// 경로가 파일 대신 디렉터리이면 기존 옵션을 비우기 전에 실패하고 디렉터리 내용도 보존한다.
TEST_CASE(ResetAll_UnexpectedDirectoryFailsBeforeOptionsAreChanged) {
    const auto base=ResetWork("directory");const platform::UserDataRoot user(base/"user"); // 오염된 체크포인트 경로를 별도 디렉터리로 만든다.
    ResetWrite(user.OptionsFile(),"keep-settings");ResetWrite(user.SavedStateFile()/"keep.bin","keep");
    client::Client client(ResetClientOptions(base/"install",user));bool failed{}; // 실제 Client 종료 관문도 실패에 켜지지 않아야 한다.
    try { client.ResetAllSettingsAndClose(); }catch (const std::filesystem::filesystem_error&) { failed=true; }
    CHECK(failed && !client.SettingsResetPending());CHECK(ResetRead(user.OptionsFile())=="keep-settings" && ResetRead(user.SavedStateFile()/"keep.bin")=="keep");
}

// 빈 사용자 옵션은 설치의 옛 진행·언어·음량을 다시 가져오지 않고 실제 Client 시작 기본값을 복원한다.
TEST_CASE(ResetAll_ClientRestartDoesNotMigrateLegacyOptionsAgain) {
    const auto base=ResetWork("restart");const auto install=base/"install";const platform::UserDataRoot user(base/"user"); // 설치·사용자 루트의 독립 경로다.
    std::filesystem::create_directories(install/"d");
    {
        auto options=ResetClientOptions(install,user);options.userData.reset();client::Client legacy(std::move(options)); // 이관 대상 파일은 검사 안에서 새로 만든다.
        legacy.Configuration().Startup({});legacy.Configuration().SetInt("welcomeGiven",1);legacy.Configuration().SetInt("DoneTheWarBegins",1);legacy.Configuration().SetInt("languageNumber",7);legacy.Configuration().SetInt("soundVolume",2);
        client::SaveClientOptionsInGameDirectory(legacy.Configuration(),install,legacy.StaticStrings()->FileNames());
    }
    const auto original=ResetRead(install/"d/options.cfg"); // 초기화 전 설치 바이트를 끝까지 비교한다.
    {
        client::Client current(ResetClientOptions(install,user));current.Configuration().Startup({});CHECK(current.Configuration().GetInt("DoneTheWarBegins")==1);
        current.ResetAllSettingsAndClose();CHECK(current.SettingsResetPending());CHECK(std::filesystem::file_size(user.OptionsFile())==0);
    }
    {
        client::Client restarted(ResetClientOptions(install,user));restarted.Configuration().Startup({});
        CHECK(restarted.Configuration().GetInt("welcomeGiven")==0 && restarted.Configuration().GetInt("DoneTheWarBegins")==0 && restarted.Configuration().GetInt("languageNumber")==0);
        const auto audio=client::ReadClientAudioOptions(restarted.Configuration()); // 실제 시작 음량 기본값도 레거시 2가 아닌 5다.
        CHECK(audio.soundVolumeStep==5 && audio.musicVolumeStep==5 && !restarted.SettingsResetPending());
    }
    CHECK(ResetRead(install/"d/options.cfg")==original);
}

// 원문 기본 메뉴에는 추가 행이 없고 명시한 메인 Options에서는 최하단 행이 확인 Tell만 보낸다.
TEST_CASE(ResetAll_MainOptionsLastRowOpensConfirmation) {
    const auto original=ResetMenuRows(false),extended=ResetMenuRows(true); // 같은 제품 작성기를 두 정책으로 실행한 결과다.
    CHECK(extended.size()==original.size()+1 && extended.back().first=="Reset All" && extended.back().second=="Patch1101.ResetAll");
    CHECK(std::equal(original.begin(),original.end(),extended.begin()));
}

// 세 UI 언어의 확인 창 모두 별도 확정·취소 링크와 종료 안내를 갖는다.
TEST_CASE(ResetAll_ConfirmationUsesExistingTellWindowInThreeLanguages) {
    o::ConfigRegistry registry;const auto product=client::MakeProductCreditsConfig(registry); // 실제 런타임이 등록하는 제품 객체의 새 섹션이다.
    CHECK(product->Section("ResetAll").has_value());
    const auto english=client::ProductTellBody("Patch1101.ResetAll",*product->Section("ResetAll"),0); // 원래 영문 섹션을 그대로 표시한다.
    const auto korean=client::ProductTellBody("Patch1101.ResetAll",english,7),japanese=client::ProductTellBody("Patch1101.ResetAll",english,5); // 같은 표시 경계의 두 번역이다.
    for (const auto& body:{english,korean,japanese}) { // 세 본문이 모두 기존 Tell의 별도 확정·취소를 사용한다.
        CHECK(body.find("Tell,Patch1101.ResetAllConfirmed")!=body.npos && body.find("Tell,Blank")!=body.npos);
    }
    CHECK(english.find("game will close")!=english.npos && korean.find("게임을 종료")!=korean.npos && japanese.find("ゲームを終了")!=japanese.npos);
}
