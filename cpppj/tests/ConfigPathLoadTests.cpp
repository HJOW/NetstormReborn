// 실제 설정 Load 부모의 파일명/검색 수명과 같은 Client/VFS 경로 고리를 확인한다.
#include "RawSceneSupport.h"
#include "o/ConfigInterface.h"
#include "o/RawFileName.h"
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "platform/Utf8Path.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <chrono>
#include <fstream>
#include <limits>

namespace { // Native 행은 입력·독립 기대 순서로만 해석한다.
std::string LoadPathBytes(std::string_view hex) { // NUL/상위BYTE도 그대로 복원한다.
    std::string result;if (hex=="-") return result;
    for (std::size_t at=0;at<hex.size();at+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(at,2)),nullptr,16)); // 두16진수마다 한BYTE다.
    return result;
}
std::string LoadPathHex(std::string_view value) { return netstorm::test::rawscene::Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } // 원래 문자열의 BYTE 순서를 보존한다.
std::vector<std::string> LoadSections(std::string_view text) { // 실제 설정 결과의 section 줄만 추출한다.
    std::vector<std::string> result;
    for (const auto& line:netstorm::test::rawscene::Split(text,'\n')) { auto value=std::string_view(line);if (value.ends_with('\r')) value.remove_suffix(1);if (value.starts_with('[') && value.ends_with(']')) result.emplace_back(value.substr(1,value.size()-2)); } // inline 키/본문을 section으로 세지 않는다.
    return result;
}
std::filesystem::path ConfigPathWork() { // 보호 디렉터리 밖의 새 Unicode 게임 폴더다.
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-config-path-load-731/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/L"설정😀";
}
void WriteLoadPath(const std::filesystem::path& path,std::string_view body) { // 원문 XOR 형식의 새 입력 파일만 작성한다.
    std::filesystem::create_directories(path.parent_path());const auto bytes=netstorm::o::EncodeConfigFile(body);std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));CHECK(file.good());
}
}

//440e00의 전환/NUL/검색 성공·실패12입력에서 section과 Find→Read→Close 순서를 대조한다.
TEST_CASE(ConfigPathLoad_Native12SectionsFindLifetimeAndRing) {
    using namespace netstorm;using namespace test::rawscene;std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_CONFIG_PATH_LOAD_FIXTURE).rows) { // 기대 section/읽기/검색/반납 순서는 독립 CPU 결과다.
        CHECK(row.size()==4);std::vector<std::string> arguments,expectedSections,expectedEvents,events;if (row[0]!="-") for (const auto& hex:Split(row[0],',')) arguments.push_back(LoadPathBytes(hex)); // 첫NUL 뒤의 물리도 공급한다.
        for (const auto& event:Split(row[3],';')) { if (event.starts_with("section:")) expectedSections.push_back(LoadPathBytes(std::string_view(event).substr(8)));else if (event.starts_with("read:") || event.starts_with("find:") || event.starts_with("close:")) expectedEvents.push_back(event); } // 파서/로그 자체는 원문 오라클의 명시 경계다.
        const auto found=LoadPathBytes(row[1]);auto names=std::make_shared<o::RawFileName>();bool active{};o::FileSpecQueryHooks hooks;
        hooks.find=[&](auto name) { CHECK(!active);active=true;events.push_back("find:"+LoadPathHex(name));return o::FileSpecFindResult{found.empty() ? std::numeric_limits<std::uintptr_t>::max():0x1234U,found}; };
        hooks.close=[&](auto handle) { CHECK(active);active=false;events.push_back("close:"+std::to_string(static_cast<std::uint32_t>(handle))); }; // x86의 실패 핸들을 host 모든비트1로 대응한다.
        o::ConfigInterface config([&](auto name)->std::optional<std::vector<std::uint8_t>> { if (name.find_first_of("*?")!=name.npos) CHECK(active);events.push_back("read:"+LoadPathHex(name));return {}; },names,hooks);config.Load(arguments);
        CHECK(events==expectedEvents && LoadSections(config.Configuration().Text())==expectedSections && names->Index()==Number(row[2])%10 && !active);++count;
    }
    CHECK(count==12);
}

// Reader 예외 뒤에도 동일 검색 핸들을 닫고 이미 작성한 section/고리 한 칸을 보존한다.
TEST_CASE(ConfigPathLoad_ReaderFailureClosesSearchAndKeepsPartialState) {
    using namespace netstorm::o;auto names=std::make_shared<RawFileName>();bool active{},closed{},failed{};FileSpecQueryHooks hooks;hooks.find=[&](auto) { active=true;return FileSpecFindResult{0x1234,"chosen.cfg"}; };hooks.close=[&](auto handle) { CHECK(handle==0x1234 && active);active=false;closed=true; };
    ConfigInterface config([&](auto)->std::optional<std::vector<std::uint8_t>> { CHECK(active);throw std::runtime_error("읽기 실패 관측"); },names,hooks);const std::array<std::string,1> arguments{"@d\\*.cfg"};
    try { config.Load(arguments); }catch (const std::runtime_error&) { failed=true; }
    CHECK(failed && closed && !active && names->Index()==1 && LoadSections(config.Configuration().Text())==std::vector<std::string>({"ARGS","chosen.cfg"}));
}

// 실제 Client의 파일명 한 칸과 VFS 조회2/3/1칸을 같은 소유자가 순서대로 소비한다.
TEST_CASE(ConfigPathLoad_ActualClientUnicodeFilesAndSharedOwner) {
    using namespace netstorm;const auto root=ConfigPathWork();WriteLoadPath(root/"d/one.cfg","mQdsTfirst=0\r\nloaded=\"yes\"\r\n");client::ClientOptions options;options.gameDirectory=root;options.noAudio=true;client::Client instance(std::move(options));auto names=instance.Files().FileNameOwner();CHECK(names.get()==&instance.StaticStrings()->FileNames());const auto direct=platform::PathToUtf8(root/"d/one.cfg");const std::array<std::string,3> arguments{"@d/one.cfg","missing.cfg",direct};instance.Configuration().Load(arguments);
    CHECK(names->Index()==9 && instance.Configuration().Configuration().Get("loaded")=="yes");CHECK(LoadSections(instance.Configuration().Configuration().Text())==std::vector<std::string>({"ARGS","one.cfg","missing.cfg","one.cfg","END"}));
    const auto pattern=platform::PathToUtf8((root/"d/*.cfg").make_preferred());{ o::FileSpecQuery actual(pattern);CHECK(actual.Text()==platform::PathToUtf8((root/"d/one.cfg").make_preferred())); } // 실제 W 검색 장치는 Unicode 폴더에서도 원래 backslash prefix를 보존한다.
}
