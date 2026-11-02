// 원문 Startup의 post-location 이름 작성/지역 복사/Load와 실제 Client 파일 수명을 확인한다.
#include "RawSceneSupport.h"
#include "o/ConfigInterface.h"
#include "o/RawFileName.h"
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "client/ClientOptionsDisk.h"
#include "platform/Utf8Path.h"
#include <chrono>
#include <fstream>

namespace { // 기대 경로/버퍼는 독립 CPU fixture에서만 읽는다.
std::string StartupBytes(std::string_view hex) { // 빈 문자열과 첫NUL 뒤 BYTE도 보존한다.
    std::string result;if (hex=="-" || hex=="NULL") return result;
    for (std::size_t at=0;at<hex.size();at+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(at,2)),nullptr,16)); // 두16진수마다 한BYTE다.
    return result;
}
std::string StartupIdentity(const std::vector<std::string>& row) { // 이미 선택된 StartupOptions.identity와 원문 getenv fallback 입력을 대응한다.
    auto value=StartupBytes(row[1]);value.resize(value.find('\0')==value.npos ? value.size():value.find('\0'));if (!value.empty()) return value;
    value=StartupBytes(row[2]);value.resize(value.find('\0')==value.npos ? value.size():value.find('\0'));return value;
}
std::vector<std::string> StartupReads(const std::vector<std::string>& row) { // Native가 지역 복사로 Load에 전달한 비어 있지 않은 파일 인자만 읽는다.
    const auto fields=netstorm::test::rawscene::Split(row[5],',');std::vector<std::string> result;CHECK(fields.size()==9);
    for (std::size_t index=3;index<fields.size();++index) { const auto value=StartupBytes(fields[index]);if (!value.empty()) result.push_back(value); } // command/version/@는 파일 인자가 아니다.
    return result;
}
netstorm::o::ConfigInterface::StartupOptions StartupInput(const std::vector<std::string>& row) { // 원문 구간에 제공한 확정 prefix/선택 identity/버전과 같은 입력이다.
    netstorm::o::ConfigInterface::StartupOptions options;options.commandLine="cheat=1;SCREENW=800";options.majorVersion=10;options.minorVersion=78;options.dataPrefix=StartupBytes(row[0]);options.identity=StartupIdentity(row);return options;
}
void WriteStartupFile(const std::filesystem::path& path) { // 모든 읽기 후보가 실제 기본 디스크에 있는 새 입력을 만든다.
    std::filesystem::create_directories(path.parent_path());const auto bytes=netstorm::o::EncodeConfigFile("mQdsTignored=0\r\nIdentityMarker=\"loaded\"\r\nSCREENW=1024\r\n");std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));CHECK(file.good());
}
}

//24개 root/identity/초기번호에서 전체10칸을 비교해 작성 순서와 미사용demo 예약도 검출한다.
TEST_CASE(StartupFileNames_Native24OwnedArgumentsAndWholeRing) {
    using namespace netstorm;using namespace test::rawscene;std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_STARTUP_FILE_NAMES_FIXTURE).rows) { // 첫 파일을 읽기 전 모든 경로가 지역 소유 복사로 준비돼야 한다.
        CHECK(row.size()==8);auto names=std::make_shared<o::RawFileName>();std::array<const std::uint8_t*,10> slots{};
        for (unsigned reserve=0;reserve<10;++reserve) { static_cast<void>(names->Format({}));slots[names->Index()]=names->CurrentBytes().data(); } // 공개256BYTE 뷰의 각 실제 주소만 빌리며 내용을 바꾸지 않는다.
        for (std::uint32_t reserve=0;reserve<Number(row[3]);++reserve) static_cast<void>(names->Format({})); // Native의 명시 초기 번호/모든0버퍼와 동일하다.
        std::vector<std::string> reads;o::ConfigInterface config([&](auto name)->std::optional<std::vector<std::uint8_t>> { reads.emplace_back(name);return {}; },names);const auto input=StartupInput(row);config.Startup(input);
        CHECK(reads==StartupReads(row) && names->Index()==(Number(row[3])+Number(row[6]))%10);
        for (std::size_t slot=0;slot<slots.size();++slot) CHECK(Hex(std::span<const std::uint8_t>(slots[slot],256))==std::string_view(row[7]).substr(slot*512,512)); // 실제Native 전체 고리가 작성/읽기 순서를 독립 검증한다.
        CHECK(input.dataPrefix==StartupBytes(row[0]) && input.identity==StartupIdentity(row));++count;
    }
    CHECK(count==24);
}

// Reader가 같은 고리를 열 번 덮어도 준비한 모든 파일 인자는 원문 local 배열처럼 유지된다.
TEST_CASE(StartupFileNames_ReentrantReadKeepsLocalCopies) {
    using namespace netstorm;const auto rows=test::rawscene::LoadFixture(NETSTORM_STARTUP_FILE_NAMES_FIXTURE).rows;const auto& row=rows.at(2);auto names=std::make_shared<o::RawFileName>();std::vector<std::string> reads;
    o::ConfigInterface config([&](auto name)->std::optional<std::vector<std::uint8_t>> { const std::string before(name);const std::array<o::FileNamePart,1> overwrite{{{-3,"overwrite"}}};
        for (unsigned call=0;call<10;++call) static_cast<void>(names->Format(overwrite)); // 최초 작성기 칸까지 덮어쓰는 실제 재진입이다.
        CHECK(name==before);reads.emplace_back(name);return {};
    },names);config.Startup(StartupInput(row));CHECK(reads==StartupReads(row));
}

// identity 포함7작성+기본파일6개(이름1/읽기2)=25칸 뒤 옵션저장/재적재도 같은 실제Client다.
TEST_CASE(StartupFileNames_ActualClientUnicodeStartupSaveAndReload) {
    using namespace netstorm;const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-startup-names-732/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/L"게임😀";
    for (const auto name:{"u.cfg","user.cfg","options.cfg","dev.cfg","guild.cfg","setup.cfg"}) WriteStartupFile(root/"d"/name); // demo는 만들거나 읽지 않는다.
    client::ClientOptions options;options.gameDirectory=root;options.noAudio=true;client::Client instance(std::move(options));auto& names=instance.StaticStrings()->FileNames();o::ConfigInterface::StartupOptions startup;startup.commandLine="SCREENW=800";startup.identity="u";startup.installDir=platform::PathToUtf8(root);instance.Configuration().Startup(startup);
    CHECK(names.Index()==5 && instance.Files().FileNameOwner().get()==&names);CHECK(instance.Configuration().GetInt("SCREENW")==800 && instance.Configuration().Configuration().Get("IdentityMarker")=="loaded");CHECK(!std::filesystem::exists(root/"d/demo.cfg"));
    client::SaveClientOptionsInGameDirectory(instance.Configuration(),root,names);CHECK(names.Index()==9);const auto bytes=instance.Files().Read("d/options.cfg");CHECK(names.Index()==1 && o::ConfigText::FromBytes(bytes).GetRaw("InstallDir")==platform::PathToUtf8(root));
}
