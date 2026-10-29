// 실제 임시 파일과 원문 오류 기대값을 사용하며 게임/승리/재접속을 실행하지 않는다.
#include "TestSupport.h"
#include "client/SavedStateFiles.h"
#include "client/NativeMainMenuState.h"
#include "client/ClientLocalStateGlobals.h"
#include "o/Win32FileError.h"
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

namespace { // 이번 검사만 만든 build/test-data의 새 경로를 사용한다.
using namespace netstorm::client;
// 기존 파일을 정리하거나 재사용하지 않게 단조 시각을 이름에 붙인다.
std::filesystem::path StateWork(std::string_view label) {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::path(__FILE__).parent_path().parent_path()/"build/test-data"/("saved-state-"+std::string(label)+'-'+std::to_string(stamp))/L"한글 설치";
}
// 임시 파일만 작성하며 부모 디렉터리 준비는 각 검사가 명시한다.
void WriteStateTest(const std::filesystem::path& path,std::string_view text) {
    std::ofstream file(path,std::ios::binary);file.write(text.data(),static_cast<std::streamsize>(text.size()));CHECK(file.good());
}
// 이웃 파일과 실패한 삭제 대상의 실제 바이트를 대조한다.
std::string ReadStateTest(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
}

// 모든 표/포함 구간/기본 오류와 중복6의 첫 일치를 독립 원본263행에 대조한다.
TEST_CASE(SavedStateFiles_NativeCrt263AndFirstDuplicateErrorMapping) {
    std::ifstream file(NETSTORM_SAVED_STATE_FILE_FIXTURE);CHECK(file.good());std::string line;std::size_t count{};
    while (std::getline(file,line)) { // 주석 외 모든 원문 행의 unsignedDWORD와 TLS를 읽는다.
        if (line.empty() || line[0]=='#') continue;std::istringstream fields(line);std::uint32_t success{},error{},result{},mapped{},dos{};fields>>success>>error>>result>>mapped>>dos;CHECK(!fields.fail());++count;
        if (success || !error) { CHECK(!result && mapped==0x3456U && dos==0x11223344U); }
        else CHECK(result==0xffffffffU && dos==error && netstorm::o::Win32FileErrorCode(error)==static_cast<std::int32_t>(mapped));
    }
    CHECK(count==263 && netstorm::o::Win32FileErrorCode(6)==9);
}

// 파일명 조회는 쓰지 않으며 실제 Unicode 삭제도 지정한 한 파일만 없애야 한다.
TEST_CASE(SavedStateFiles_UnicodeExactTargetAndSuccessfulCrtErrorPreservation) {
    const auto root=StateWork("success"),path=SavedStateFilePath(root);CHECK(!std::filesystem::exists(root));
    CHECK(path==std::filesystem::absolute(root/"d"/"!state.dat") && path.filename()==L"!state.dat");
    std::filesystem::create_directories(path.parent_path());WriteStateTest(path,"state bytes");WriteStateTest(path.parent_path()/"other.dat","keep");WriteStateTest(path.parent_path()/"options.cfg","options");
    errno=E2BIG;static_cast<void>(::_set_doserrno(0x11223344UL));CHECK(DeleteSavedStateFile(root)==0);
    unsigned long dos{};CHECK(::_get_doserrno(&dos)==0 && dos==0x11223344UL && errno==E2BIG);
    CHECK(!std::filesystem::exists(path) && ReadStateTest(path.parent_path()/"other.dat")=="keep" && ReadStateTest(path.parent_path()/"options.cfg")=="options");
}

// 원문 DeleteFile의 부재/readonly/디렉터리 실패는 파일 속성·내용·디렉터리를 바꾸지 않는다.
TEST_CASE(SavedStateFiles_MissingReadonlyAndDirectoryKeepOriginalFailureSemantics) {
    const auto root=StateWork("errors"),path=SavedStateFilePath(root);std::filesystem::create_directories(path.parent_path());
    CHECK(DeleteSavedStateFile(root)==-1 && errno==ENOENT);unsigned long dos{};CHECK(::_get_doserrno(&dos)==0 && dos==ERROR_FILE_NOT_FOUND);
    WriteStateTest(path,"protected bytes");const auto attributes=::GetFileAttributesW(path.c_str());CHECK(attributes!=INVALID_FILE_ATTRIBUTES);
    CHECK(::SetFileAttributesW(path.c_str(),attributes|FILE_ATTRIBUTE_READONLY)!=0);CHECK(DeleteSavedStateFile(root)==-1 && errno==EACCES);
    CHECK(::_get_doserrno(&dos)==0 && dos==ERROR_ACCESS_DENIED && ReadStateTest(path)=="protected bytes" && (::GetFileAttributesW(path.c_str())&FILE_ATTRIBUTE_READONLY));
    CHECK(::SetFileAttributesW(path.c_str(),attributes)!=0);CHECK(DeleteSavedStateFile(root)==0);std::filesystem::create_directory(path);
    CHECK(DeleteSavedStateFile(root)==-1 && errno==EACCES && std::filesystem::is_directory(path));
}

// 명시 State3 입력은 실제 파일 장치를 호출하되 승패 판정/월드/게임을 만들지 않는다.
TEST_CASE(SavedStateFiles_NativeMainMenuIgnoresRemoveFailureAndRecomputesSecondPath) {
    for (std::uint32_t scenario=0;scenario<3;++scenario) { // 성공·처음부터 부재·같은 부모의 두 삭제를 구별한다.
        const auto root=StateWork(std::to_string(scenario)),path=SavedStateFilePath(root);std::filesystem::create_directories(path.parent_path());if (scenario!=1) WriteStateTest(path,"state");
        ClientLocalStateGlobals local;local.State().current[0]=3;local.State().phase[0]=scenario==2 ? 7U : 4U;
        std::uint32_t source=scenario==2 ? 1U : 0U,outcome=scenario==1 ? 2U : 1U,first{},editor{};bool battle{},tail{};
        std::array<char,80> background{};std::array<std::uint8_t,32> current{},pending{};std::vector<std::int32_t> removed;std::vector<std::string> music;
        NativeMainMenuStateGlobals globals{local.State(),source,outcome,first,battle,editor,background,current,pending,{},{}};
        NativeMainMenuStateHooks hooks;hooks.configInt=[](auto key) { CHECK(key=="intro" || key=="WelcomeGiven");return 1; };
        hooks.deleteSavedState=[&] { removed.push_back(DeleteSavedStateFile(root)); };hooks.requestMusic=[&](auto name) { CHECK(!std::filesystem::exists(path));music.emplace_back(name); };
        hooks.sourceBackground=[](auto type)->std::optional<std::string> { CHECK(type==1);return std::nullopt; };
        hooks.prefix=[] {};hooks.hudPrefix=[] {};hooks.title=[] {};hooks.closeGuide=[] {};hooks.tell=[](auto section) { CHECK(section=="Blank"); };hooks.startup=[] { return 0U; };hooks.tail=[&] { tail=true; };
        CHECK(InstallNativeMainMenuState(globals,hooks)==netstorm::o::LocalMissionParentResult::Commit && tail && !outcome);
        const std::vector<std::int32_t> expected=scenario==2 ? std::vector<std::int32_t>{0,-1} : std::vector<std::int32_t>{scenario==1 ? -1 : 0};CHECK(removed==expected);
        CHECK(music==std::vector<std::string>(scenario==2 ? 2U : 1U,scenario==1 ? "defeat.mus" : "fanfare.mus"));
    }
}
