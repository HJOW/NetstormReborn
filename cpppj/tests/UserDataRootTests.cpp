// 5차(사용자 홈 저장) 검사: 설정/체크포인트/전체화면 표식이 설치 폴더가 아닌 Saved Games\Netstorm 아래에만 만들어지는지 확인한다.
// 실제 게임/GUI는 실행하지 않으며 임시 폴더에서 Client 설정 계층과 파일 장치만 사용한다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ClientOptionsDisk.h"
#include "client/ClientStaticStrings.h"
#include "client/SavedStateFiles.h"
#include "o/RawFileName.h"
#include "platform/UserDataRoot.h"
#include "platform/Utf8Path.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <string>

namespace { // 이번 검사만 쓰는 새 임시 폴더와 환경 변수 보조다.
using namespace netstorm;
// 매 호출마다 새 한글/보충 문자 경로를 만들어 기존 폴더를 재사용하거나 정리하지 않는다.
std::filesystem::path UserWork(std::string_view label) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count(); // 단조 시각으로 이름 충돌을 피한다.
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "build/test-data" / ("user-data-" + std::string(label) + '-' + std::to_string(stamp)) / L"한글😀";
}
// 파일 전체 바이트를 읽는다(없으면 빈 문자열). 파일이 있는지는 별도로 검사한다.
std::string ReadAll(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
// 지정한 경로에 바이트를 쓴다. 부모 폴더는 호출자가 준비한다.
void WriteAll(const std::filesystem::path& path, std::string_view text) {
    std::ofstream file(path, std::ios::binary);
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    CHECK(file.good());
}
// 임시 설치 폴더를 만들고 Client를 사용자 루트 정책으로 구성한다. userRoot가 비어 있으면 옛 설치 폴더 동작이다.
client::ClientOptions MakeOptions(const std::filesystem::path& install, std::optional<platform::UserDataRoot> userRoot) {
    client::ClientOptions options;
    options.gameDirectory = install;
    options.noAudio = true;
    options.userData = std::move(userRoot);
    return options;
}
}

// 하위 폴더 이름과 파일 경로가 정확하고 폴더 생성은 명시 호출에서만 일어난다.
TEST_CASE(UserDataRoot_LayoutNamesAndLazyDirectoryCreation) {
    const auto base = UserWork("layout");
    platform::UserDataRoot root(base / "Saved Games" / "Netstorm");
    CHECK(root.Root() == std::filesystem::absolute(base / "Saved Games" / "Netstorm").lexically_normal());
    CHECK(root.Saves() == root.Root() / "Saves" && root.CustomMaps() == root.Root() / "CustomMaps");
    CHECK(root.Editor() == root.Root() / "Editor" && root.Captures() == root.Root() / "Captures" && root.Diagnostics() == root.Root() / "Diagnostics");
    CHECK(root.OptionsFile() == root.Saves() / "options.cfg" && root.SavedStateFile() == root.Saves() / "!state.dat");
    CHECK(root.FullScreenStateFile() == root.Saves() / "fullscreenStateFile.dat");
    CHECK(!std::filesystem::exists(base)); // 경로 계산만으로는 아무 폴더도 만들지 않는다.
    platform::UserDataRoot::EnsureDirectory(root.Saves());
    CHECK(std::filesystem::is_directory(root.Saves()));
    platform::UserDataRoot::EnsureDirectory(root.Saves()); // 이미 있어도 성공한다.
    WriteAll(root.CustomMaps().parent_path() / "blocker", "file");
    bool rejected{};
    try { platform::UserDataRoot::EnsureDirectory(root.Root() / "blocker" / "child"); } catch (const std::system_error&) { rejected = true; } // 같은 이름의 파일 아래에는 만들 수 없다.
    CHECK(rejected);
}

// 환경 변수 재정의가 있으면 그 폴더를 그대로 쓰고, 없으면 Saved Games 아래 Netstorm이며 어느 쪽도 폴더를 만들지 않는다.
TEST_CASE(UserDataRoot_ResolveHonorsOverrideAndDefaultsToSavedGamesNetstorm) {
    const auto base = UserWork("resolve");
    CHECK(::_wputenv_s(L"NETSTORM_USER_DATA", base.c_str()) == 0);
    const auto overridden = platform::UserDataRoot::Resolve();
    CHECK(overridden.Root() == std::filesystem::absolute(base).lexically_normal() && !std::filesystem::exists(base));
    CHECK(::_wputenv_s(L"NETSTORM_USER_DATA", L"") == 0); // 빈 값은 재정의로 취급하지 않는다.
    const auto normal = platform::UserDataRoot::Resolve();
    CHECK(normal.Root().filename() == L"Netstorm" && normal.Root().is_absolute() && normal.Root() != overridden.Root());
    CHECK(normal.OptionsFile().filename() == L"options.cfg" && normal.OptionsFile().parent_path().filename() == L"Saves");
}

// 사용자 파일을 주면 원문 이름 고리 소비는 같고 실제 위치만 바뀌며 설치 폴더의 옛 !state.dat는 건드리지 않는다.
TEST_CASE(SavedStateFiles_UserFileKeepsNameRingAndNeverTouchesInstall) {
    const auto install = UserWork("state") / "install", user = UserWork("state-user") / "Saves" / "!state.dat";
    netstorm::o::RawFileName legacyNames, userNames;
    const auto legacy = client::SavedStateFilePath(install, legacyNames);
    const auto redirected = client::SavedStateFilePath(install, userNames, user);
    CHECK(legacy == std::filesystem::absolute(install / "d" / "!state.dat") && redirected == std::filesystem::absolute(user));
    CHECK(legacyNames.Index() == userNames.Index()); // 같은 한 칸을 소비한다.
    std::filesystem::create_directories(legacy.parent_path());
    WriteAll(legacy, "install state");
    std::filesystem::create_directories(user.parent_path());
    WriteAll(user, "user state");
    CHECK(client::DeleteSavedStateFile(install, userNames, user) == 0);
    CHECK(!std::filesystem::exists(user) && ReadAll(legacy) == "install state"); // 사용자 파일만 지운다.
    CHECK(client::DeleteSavedStateFile(install, userNames, user) == -1); // 이미 없으면 원문처럼 실패 코드이며 설치 파일은 계속 보존한다.
    CHECK(ReadAll(legacy) == "install state");
}

// 옵션 저장은 사용자 Saves에만 만들고, 새 Client는 사용자 파일을 설치 파일보다 먼저 읽는다. 설치 폴더에는 아무 파일도 생기지 않는다.
TEST_CASE(ClientOptions_UserFileIsWrittenAndReadBackWithoutTouchingInstall) {
    const auto base = UserWork("options");
    const auto install = base / "install";
    const platform::UserDataRoot userRoot(base / "Saved Games" / "Netstorm");
    std::filesystem::create_directories(install / "d");
    {
        client::Client first(MakeOptions(install, userRoot));
        auto& names = first.StaticStrings()->FileNames();
        first.Configuration().Startup({});
        first.Configuration().Set("PhaseFiveProbe", "77");
        client::SaveClientOptionsInGameDirectory(first.Configuration(), install, names, userRoot.OptionsFile());
    }
    CHECK(std::filesystem::is_regular_file(userRoot.OptionsFile())); // 사용자 홈에 만들어진다.
    CHECK(!std::filesystem::exists(install / "d" / "options.cfg")); // 설치 폴더에는 만들지 않는다.
    std::size_t installEntries{};
    for (const auto& entry : std::filesystem::recursive_directory_iterator(install)) { static_cast<void>(entry); ++installEntries; } // 설치 아래 모든 항목을 세어 새 파일이 없음을 확인한다.
    CHECK(installEntries == 1); // 처음 만든 빈 d 폴더 하나뿐이다.
    {
        client::Client second(MakeOptions(install, userRoot));
        second.Configuration().Startup({});
        CHECK(second.Configuration().GetInt("PhaseFiveProbe") == 77); // 사용자 파일에서 읽었다.
    }
}

// 사용자 파일이 없으면 설치의 옛 d\options.cfg를 읽고(이관), 둘 다 있으면 사용자 파일이 이긴다.
TEST_CASE(ClientOptions_LegacyInstallSettingsMigrateAndUserFileTakesPrecedence) {
    const auto base = UserWork("migrate");
    const auto install = base / "install";
    std::filesystem::create_directories(install / "d");
    {
        client::Client legacy(MakeOptions(install, std::nullopt)); // 옛 동작: 설치 폴더의 d\options.cfg에 쓴다.
        legacy.Configuration().Startup({});
        legacy.Configuration().Set("PhaseFiveProbe", "11");
        client::SaveClientOptionsInGameDirectory(legacy.Configuration(), install, legacy.StaticStrings()->FileNames());
    }
    CHECK(std::filesystem::is_regular_file(install / "d" / "options.cfg"));
    const platform::UserDataRoot userRoot(base / "Saved Games" / "Netstorm");
    {
        client::Client migrated(MakeOptions(install, userRoot)); // 사용자 파일이 아직 없다.
        migrated.Configuration().Startup({});
        CHECK(migrated.Configuration().GetInt("PhaseFiveProbe") == 11); // 설치의 옛 설정을 읽는다.
        migrated.Configuration().Set("PhaseFiveProbe", "22");
        client::SaveClientOptionsInGameDirectory(migrated.Configuration(), install, migrated.StaticStrings()->FileNames(), userRoot.OptionsFile());
    }
    const auto installBytes = ReadAll(install / "d" / "options.cfg");
    CHECK(!installBytes.empty()); // 옛 설정 파일은 수정하지 않고 그대로 남는다.
    {
        client::Client current(MakeOptions(install, userRoot));
        current.Configuration().Startup({});
        CHECK(current.Configuration().GetInt("PhaseFiveProbe") == 22); // 사용자 파일이 설치 파일보다 우선한다.
    }
    CHECK(ReadAll(install / "d" / "options.cfg") == installBytes); // 설치 파일은 끝까지 변하지 않았다.
}
