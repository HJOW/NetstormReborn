// 5차(사용자 홈 저장) 검사: 설정/체크포인트/전체화면 표식이 설치 폴더가 아닌 Saved Games\Netstorm 아래에만 만들어지는지 확인한다.
// 실제 게임/GUI는 실행하지 않으며 임시 폴더에서 Client 설정 계층과 파일 장치만 사용한다.
#include "TestSupport.h"
#include "client/BitmapFont.h"
#include "client/ClientMain.h"
#include "client/HelpListDisk.h"
#include "client/TellMapMenus.h"
#include "o/RawSoloMapList.h"
#include "client/ClientOptionsDisk.h"
#include "client/ClientStaticStrings.h"
#include "client/EditorFiles.h"
#include "client/EditorMapCatalogFiles.h"
#include "client/SavedStateFiles.h"
#include "o/RawFileName.h"
#include "platform/UserDataRoot.h"
#include "platform/Utf8Path.h"
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

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

// 사용자 홈 정책의 Editor 루트는 CustomMaps/Editor이고, 정책이 꺼지면 기존 형제 -editor 배치이며 보호 루트는 같다.
TEST_CASE(EditorRoots_UserPolicySelectsCustomMapsAndEditorMetadata) {
    const auto base = UserWork("editor-roots");
    const auto install = base / "game" / "d2";
    const platform::UserDataRoot userRoot(base / "Saved Games" / "Netstorm");
    const auto legacy = client::ResolveEditorRoots(install, std::nullopt);
    CHECK(legacy.work == client::EditorWorkingRoot(install) && legacy.metadata.empty());
    const auto user = client::ResolveEditorRoots(install, userRoot);
    CHECK(user.work == userRoot.CustomMaps() && user.metadata == userRoot.Editor());
    CHECK(user.protectedRoots == legacy.protectedRoots && user.protectedRoots.size() == 3); // 설치 폴더와 originals/originalCD 보호는 정책과 무관하다.
    CHECK(!std::filesystem::exists(base)); // 계산만으로는 아무 것도 만들지 않는다.
}

// 사용자 홈 정책에서 Editor 이관 복사본/장부/잠금/새 맵은 사용자 루트에만 생기고 설치 폴더와 그 형제에는 아무 것도 생기지 않는다.
TEST_CASE(EditorUserPolicy_CatalogCopiesAndLedgersLiveOnlyUnderUserRoot) {
    const auto base = UserWork("editor-catalog");
    const auto install = base / "install";
    const platform::UserDataRoot userRoot(base / "Saved Games" / "Netstorm");
    std::filesystem::create_directories(install / "d");
    WriteAll(install / "d" / "Alpha.fort", "alpha bytes");
    const auto roots = client::ResolveEditorRoots(install, userRoot);
    {
        auto work = std::make_shared<client::EditorFiles>(roots.work, roots.protectedRoots);
        client::EditorCatalogSourceDevices source{install, [](auto) -> std::optional<std::vector<std::uint8_t>> { return std::nullopt; }, roots.metadata};
        client::EditorCatalogWorkspace catalog(work, std::move(source));
        catalog.ImportDisk("d/*.fort"); // 설치의 원본 목록을 사용자 작업 루트로 한 번 복사한다.
        CHECK(catalog.Enumerate("d/*.fort").size() == 1);
        const std::vector<std::uint8_t> fresh{'n', 'e', 'w'};
        const auto saved = work->Prepare("NewMap", fresh); // 새 맵 이름의 작업본은 작업 루트의 d 아래에 만든다.
        CHECK(std::filesystem::is_regular_file(saved) && client::EditorWorkingRoot(install) != roots.work);
        CHECK(catalog.Enumerate("d/*.fort").size() == 2);
    }
    CHECK(ReadAll(userRoot.CustomMaps() / "d" / "Alpha.fort") == "alpha bytes"); // 이관 복사본이다.
    CHECK(std::filesystem::is_regular_file(userRoot.CustomMaps() / "d" / "NewMap.fort"));
    CHECK(std::filesystem::is_regular_file(userRoot.Editor() / "catalog-v1.dat") && std::filesystem::is_regular_file(userRoot.Editor() / "catalog.lock"));
    CHECK(ReadAll(install / "d" / "Alpha.fort") == "alpha bytes" && !std::filesystem::exists(install / "d" / "NewMap.fort")); // 설치 파일은 불변이고 새 맵은 설치에 없다.
    CHECK(!std::filesystem::exists(base / "install-editor") && !std::filesystem::exists(base / "install-editor-state")); // 기존 형제 배치를 만들지 않는다.
    std::size_t installEntries{};
    for (const auto& entry : std::filesystem::recursive_directory_iterator(install)) { static_cast<void>(entry); ++installEntries; } // 설치 아래 모든 항목(d 폴더와 Alpha.fort)을 센다.
    CHECK(installEntries == 2);
}

// 글꼴 캐시는 사용자 Cache에만 새로 만들고 두 번째 FontStore는 같은 파일을 다시 읽는다. 설치의 d 폴더에는 아무 것도 생기지 않는다.
TEST_CASE(FontCache_UserPolicyWritesOnlyUnderUserCacheAndReloads) {
    const auto base = UserWork("font-cache");
    const auto install = base / "install";
    const platform::UserDataRoot userRoot(base / "Saved Games" / "Netstorm");
    std::filesystem::create_directories(install / "d");
    o::BaseFileSystem files(install);
    std::vector<std::uint8_t> bytes;
    {
        client::FontStore first(files, "Arial", [](const auto&) {});
        first.SetUserCacheDirectory(userRoot.Cache());
        const auto& font = first.Get();
        const auto cache = font.CacheBytes(); // 생성된 글꼴의 직렬 캐시 바이트다.
        bytes.assign(cache.begin(), cache.end());
        CHECK(font.Measure("Hello") > 0 && !bytes.empty());
    }
    std::vector<std::filesystem::path> cached;
    for (const auto& entry : std::filesystem::directory_iterator(userRoot.Cache())) cached.push_back(entry.path()); // 사용자 Cache의 모든 항목을 모은다.
    CHECK(cached.size() == 1 && cached[0].extension() == ".chfnt" && cached[0].filename().string().starts_with("!Arial"));
    CHECK(ReadAll(cached[0]) == std::string(bytes.begin(), bytes.end()));
    CHECK(std::filesystem::is_empty(install / "d")); // 설치 폴더에는 캐시를 만들지 않는다.
    const auto written = std::filesystem::last_write_time(cached[0]);
    {
        client::FontStore next(files, "Arial", [](const auto&) {});
        next.SetUserCacheDirectory(userRoot.Cache());
        const auto cache = next.Get().CacheBytes();
        CHECK(std::vector<std::uint8_t>(cache.begin(), cache.end()) == bytes); // 사용자 캐시를 읽어 같은 글꼴을 만든다.
    }
    CHECK(std::filesystem::last_write_time(cached[0]) == written); // 재사용 때는 파일을 다시 쓰지 않는다.
}

// 도움말 목록 HTML은 사용자 Cache에 만들어지고 설치 폴더에는 생기지 않는다(원문 열기 인자 1,1은 그대로).
TEST_CASE(HelpList_UserPolicyCreatesPieceListUnderUserCache) {
    const auto base = UserWork("help-list");
    const auto install = base / "install";
    const platform::UserDataRoot userRoot(base / "Saved Games" / "Netstorm");
    std::filesystem::create_directories(install);
    platform::UserDataRoot::EnsureDirectory(userRoot.Cache());
    o::BaseFileSystem installFiles(install);
    o::BaseFileSystem cacheFiles(userRoot.Cache(), std::filesystem::path{}, installFiles.FileNameOwner()); // 같은 이름 고리를 공유한다.
    auto hooks = client::MakeHelpListDiskHooks(userRoot.Cache(), &cacheFiles);
    std::array<std::uint8_t, 24> storage{};
    const client::QuickHelpListFile file(storage);
    hooks.construct(file);
    hooks.open(file, "NetStormPieceList.html", 1, 1);
    hooks.write(file, "<html></html>", -1);
    hooks.close(file);
    hooks.destruct(file);
    CHECK(ReadAll(userRoot.Cache() / "NetStormPieceList.html") == "<html></html>");
    CHECK(!std::filesystem::exists(install / "NetStormPieceList.html") && std::filesystem::is_empty(install));
}

namespace { // 맵 목록 검사용 가짜 파일 장치다. 게임 계산은 하지 않고 이름/종류/구독자 입력만 명시한다.
// 120개처럼 긴 목록을 항상 종류2·정상 Template로 돌려주는 장치를 만든다.
netstorm::o::SoloMapListHooks MapListHooks(std::vector<std::string>& names) {
    netstorm::o::SoloMapListHooks hooks;
    hooks.mayEditMissions = [] { return 1; };
    hooks.enumerate = [&names] { return names; };
    hooks.kind = [](auto) { return std::uint8_t{2}; };
    hooks.construct = [] {};
    hooks.load = [](auto) {};
    hooks.present = [] { return true; };
    hooks.subscriber = [] { return std::string("me"); };
    hooks.stateByte = [] { return std::uint8_t{0}; };
    hooks.userName = [] { return std::string("me"); };
    hooks.reserved = [] { return 0U; };
    hooks.translate = [](auto key) { return std::string(key); };
    return hooks;
}
// 0부터 count-1까지의 이름을 만든다.
std::vector<std::string> MapNames(int count) {
    std::vector<std::string> names;
    for (int index = 0; index < count; ++index) names.push_back("Map" + std::to_string(1000 + index)); // 반복마다 정렬 가능한 같은 폭의 이름 하나를 만든다.
    return names;
}
// AppendTellMapMenus의 호출을 기록하는 가짜 메뉴 장치다.
struct MenuProbe {
    std::int32_t total{};                              // 현재 목록 개수다.
    std::int32_t first{};                              // 표시 창 첫 번호다.
    bool keep{};                                       // 다음 호출이 캐시 재사용인지다.
    std::vector<std::int32_t> rows;                    // 추가된 맵 행 번호다.
    std::vector<std::pair<std::string, std::int32_t>> scrolls; // 추가된 이동 행(문구, 이동 행 수)이다.
    int columns{};                                     // 만들어진 세로 열 수다.
    int invalidated{};                                 // invalidate 호출 횟수다.
    int reused{};                                      // 캐시 재사용(reuseCache) 호출 횟수다.
    netstorm::client::TellMapMenuHooks Hooks() {
        netstorm::client::TellMapMenuHooks hooks;
        hooks.beginVertical = [this](auto) { ++columns; };
        hooks.invalidate = [this] { ++invalidated; };
        hooks.reuseCache = [this] { ++reused; };
        hooks.selectFileSpec = [](bool) {};
        hooks.refresh = [](auto) {};
        hooks.count = [this] { return total; };
        hooks.appendRow = [this](auto index) { if (index >= total) return false; rows.push_back(index); return true; };
        hooks.windowFirst = [this] { return first; };
        hooks.setWindowFirst = [this](auto value) { first = value; };
        hooks.takeKeepCache = [this] { const bool value = keep; keep = false; return value; };
        hooks.appendScroll = [this](auto label, auto delta) { scrolls.emplace_back(std::string(label), delta); };
        return hooks;
    }
};
}

// 목록 한도가 원문 50일 때는 50개에서 멈추고, 한도를 올리면 필요한 만큼 늘어나며 다시 비우면 처음 크기로 돌아온다.
TEST_CASE(MapList_LimitKeepsOriginalFiftyAndUnlimitedGrowsAndShrinks) {
    auto names = MapNames(120);
    netstorm::o::SoloMapListState original;
    CHECK(original.limit == 50 && original.names.size() == 50);
    CHECK(netstorm::o::RawSoloMapList(original, MapListHooks(names)).Refresh(2) == 50 && original.names.size() == 50); // 기본값은 원문과 같다.
    netstorm::o::SoloMapListState unlimited;
    unlimited.limit = std::numeric_limits<std::int32_t>::max();
    const netstorm::o::RawSoloMapList list(unlimited, MapListHooks(names));
    CHECK(list.Refresh(2) == 120 && unlimited.names.size() == 120 && unlimited.flags.size() == 120);
    const auto last = list.Row(119, "", "");
    CHECK(last && last->name == "Map1119" && last->enabled); // 50을 넘는 칸도 같은 행 규칙을 쓴다.
    unlimited.dirty = 1;
    names = MapNames(70);
    CHECK(list.Refresh(2) == 70 && unlimited.names.size() == 70); // 다시 열거하면 새 목록 길이를 따른다.
    unlimited.dirty = 1;
    names = MapNames(3);
    CHECK(list.Refresh(2) == 3 && unlimited.names.size() == 50 && !unlimited.names[3]); // 비운 뒤 늘어난 칸은 처음 50칸으로 줄어든다.
}

// 50행 이하는 원문처럼 두 열로 한 번에, 더 길면 50행 창과 이전/다음 이동 행이 나오고 마지막 창은 꽉 채운다.
TEST_CASE(MapList_TellMenuScrollsFiftyRowWindowsAndKeepsCacheOnScroll) {
    using netstorm::client::AppendTellMapMenus;
    std::uint32_t menus = 0;
    MenuProbe shortList;
    shortList.total = 30;
    CHECK(AppendTellMapMenus("*FORTLIST*", menus, shortList.Hooks()) && shortList.rows.size() == 30 && shortList.scrolls.empty() && shortList.columns == 2); // 15개를 넘으므로 원문처럼 반으로 나눈다.
    MenuProbe list;
    list.total = 130;
    list.first = 77; // 새로 열 때는 이전 창 위치를 버리고 0에서 시작해야 한다.
    menus = 0;
    CHECK(AppendTellMapMenus("*FORTLIST*", menus, list.Hooks()));
    CHECK(list.invalidated == 1 && list.first == 0 && list.rows.size() == 50 && list.rows.front() == 0 && list.rows.back() == 49);
    CHECK(list.columns == 2 && list.scrolls.size() == 1 && list.scrolls[0].second == 50 && list.scrolls[0].first.find("Next") != std::string::npos); // 첫 창에는 다음 행만 있다.
    MenuProbe second;
    second.total = 130;
    second.first = 50;
    second.keep = true; // 스크롤 이동 뒤 재표시다.
    menus = 0;
    CHECK(AppendTellMapMenus("*FORTLIST*", menus, second.Hooks()));
    CHECK(second.invalidated == 0 && second.reused == 1 && list.reused == 0 && second.rows.size() == 50 && second.rows.front() == 50 && second.rows.back() == 99); // 목록을 다시 열거하지 않는다.
    CHECK(second.scrolls.size() == 2 && second.scrolls[0].second == -50 && second.scrolls[1].second == 50);
    MenuProbe tail;
    tail.total = 130;
    tail.first = 400; // 범위를 넘는 요청이다.
    tail.keep = true;
    menus = 0;
    CHECK(AppendTellMapMenus("*FORTLIST*", menus, tail.Hooks()));
    CHECK(tail.first == 80 && tail.rows.front() == 80 && tail.rows.back() == 129 && tail.rows.size() == 50); // 마지막 창은 항상 50행이다.
    CHECK(tail.scrolls.size() == 1 && tail.scrolls[0].second == -50 && tail.scrolls[0].first.find("Previous") != std::string::npos);
}
