// 원본설정탐색앞의Client생성자가파일을읽지않는경계와독립파일조회계약을확인한다.
#include "RawSceneSupport.h"
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "client/StartupFiles.h"
#include "platform/Utf8Path.h"
#include <chrono>
#include <fstream>

namespace { // 보호원본대신Git제외의고유입력만만든다.
std::filesystem::path StartupIoWork() { // 실제Unicode설치경로를새로선택한다.
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-startup-io-737/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/L"순서😀";
}
void StartupIoWrite(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) { // 자체입력파일만쓰고기존fixture는고치지않는다.
    std::filesystem::create_directories(path.parent_path());std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));CHECK(file.good());
}
void StartupIoBadArchive(const std::filesystem::path& path) { // 등록입구의44BYTE/매직검사는통과하지만기존hostparser의layout검사에서실패하는입력이다.
    std::vector<std::uint8_t> bytes(0x2c);const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),bytes.begin());StartupIoWrite(path,bytes);
}
void StartupIoArchive(const std::filesystem::path& path,char first) { // 두개정상파일을포함하는명시최소TAFF입력이다.
    using netstorm::test::rawscene::Put;std::vector<std::uint8_t> bytes(0x56);const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),bytes.begin());const std::array<std::pair<std::size_t,std::uint32_t>,11> fields{{{0x14,2},{0x1c,0x2c},{0x20,0x34},{0x24,0x20},{0x28,0x54},{0x2c,0x10},{0x30,0},{0x34,0},{0x38,1},{0x44,1},{0x48,1}}}; // 기존Native등록입력과같은헤더/이름인덱스/디렉터리다.
    for (const auto [offset,value]:fields) Put(bytes,offset,value); // 값마다원문DWORD폭으로쓴다.
    const std::string a="\\d\\a",b="\\d\\b";std::copy(a.begin(),a.end(),bytes.begin()+0x3c);std::copy(b.begin(),b.end(),bytes.begin()+0x4c);bytes[0x54]=static_cast<std::uint8_t>(first);bytes[0x55]='B';StartupIoWrite(path,bytes);
}
void StartupIoSetup(const std::filesystem::path& install) { const auto bytes=netstorm::o::EncodeConfigFile("mQdsT\r\nSCREENW=1234\r\n");StartupIoWrite(install/"d/setup.cfg",bytes); } // 실제선택할설정파일이며서명첫줄과키줄을분리한다.
}

TEST_CASE(StartupIo_ClientConstructorDoesNotReadArchiveBeforeSetupSearch) { // 선택대상이아닌하위폴더의손상TAFF를생성시점에열면원문보다먼저실패한다.
    const auto root=StartupIoWork();StartupIoBadArchive(root/"too-early.tarc");netstorm::client::ClientOptions options;options.gameDirectory=root;options.noAudio=true;bool constructed{};
    try { netstorm::client::Client instance(std::move(options));constructed=true;CHECK(instance.StaticStrings()->FileNames().Index()==0);instance.Configuration().Set("BeforeStartup","yes");CHECK(instance.Configuration().Configuration().Get("BeforeStartup")=="yes"); }catch (const std::runtime_error&) {}
    CHECK(constructed);
}

TEST_CASE(StartupIo_ActualClientSelectsParentAndNeverLoadsPoisonedEntryArchive) { // 실제Run과같은초기화가setup탐색뒤선택한자료만연다.
    using namespace netstorm;const auto install=StartupIoWork()/"package",entry=install/"launcher";StartupIoSetup(install);StartupIoArchive(install/"live.tarc",'P');StartupIoBadArchive(entry/"poison.tarc");client::ClientOptions options;options.gameDirectory=entry;options.noAudio=true;client::Client instance(std::move(options));const auto owner=instance.Files().FileNameOwner();bool cdCalled{};
    auto locations=instance.InitializeNativeStartupFiles([&] { cdCalled=true;CHECK(owner->Index()==9);return std::string{}; });CHECK(cdCalled && instance.GameDirectory()==install && locations.installDirectory==install && instance.Files().FileNameOwner()==owner);CHECK(owner->Index()==2 && locations.archiveSearch && !locations.archiveSearch->Valid());
    CHECK(instance.Files().Read("d/a")==std::vector<std::uint8_t>{'P'});o::ConfigInterface::StartupOptions startup;startup.dataPrefix=locations.dataPrefix;startup.installDir=platform::PathToUtf8(locations.installDirectory);startup.cdDir=locations.cdDirectory;instance.Configuration().Startup(startup);locations.archiveSearch.reset();CHECK(instance.Configuration().GetInt("SCREENW")==1234);
}

TEST_CASE(StartupIo_SelectedBadArchiveStillFailsAfterSetupAndCdBoundary) { // 원래host오류를숨기지않고실제등록단계로옮긴다.
    using namespace netstorm;const auto install=StartupIoWork();StartupIoSetup(install);StartupIoBadArchive(install/"selected-bad.tarc");client::ClientOptions options;options.gameDirectory=install;options.noAudio=true;client::Client instance(std::move(options));bool cdCalled{},failed{};
    try { static_cast<void>(instance.InitializeNativeStartupFiles([&] { cdCalled=true;return std::string{}; })); }catch (const std::runtime_error& error) { failed=std::string_view(error.what())=="Invalid TAFF header layout"; }
    CHECK(cdCalled && failed);CHECK(instance.StaticStrings()->FileNames().Full("owner-alive")=="owner-alive");
}

TEST_CASE(StartupIo_IndependentClientArchiveLookupsLoadOneSnapshot) { // 창없는API의아카이브읽기/메타데이터/목록은이전자동등록계약을유지한다.
    using namespace netstorm;
    for (unsigned kind=0;kind<7;++kind) { // 각조회종류가서로다른Client의첫아카이브접근이되게한다.
        const auto root=StartupIoWork();StartupIoArchive(root/"one.tarc",'A');client::ClientOptions options;options.gameDirectory=root;options.noAudio=true;client::Client instance(std::move(options));const auto& files=instance.Files();CHECK(files.FileNameOwner()->Index()==0);
        switch (kind) {
        case 0:CHECK(files.ContainsArchiveOnly("d/a"));break;
        case 1:CHECK(files.TryReadArchiveOnly("d/a")==std::vector<std::uint8_t>{'A'});break;
        case 2:CHECK(files.Read("d/a")==std::vector<std::uint8_t>{'A'});break;
        case 3:CHECK(files.Probe("d/a")==2);break;
        case 4:CHECK(files.ReadOnly("d/a"));break;
        case 5:CHECK(files.Match("d/*")==std::vector<std::string>({"d/a","d/b"}));break;
        case 6:CHECK(files.MatchFinder("d/*")==std::vector<std::string>({"\\d\\b","\\d\\a"}));break;
        }
        StartupIoArchive(root/"one.tarc",'X');CHECK(files.Read("d/a")==std::vector<std::uint8_t>{'A'}); // 같은Client는뒤조회마다자료를다시읽지않는다.
    }
}

TEST_CASE(StartupIo_DeferredCatalogIsValidThroughConstCopy) { // 읽기전용객체의논리캐시도const_cast없는정의된C++수명으로준비한다.
    using namespace netstorm;const auto root=StartupIoWork();StartupIoArchive(root/"one.tarc",'A');o::BaseFileSystem pending(root);pending.DeferPrimaryArchiveRegistration();const o::BaseFileSystem copied=pending;
    CHECK(copied.TryReadArchiveOnly("d/a")==std::vector<std::uint8_t>{'A'});StartupIoArchive(root/"one.tarc",'X');CHECK(pending.TryReadArchiveOnly("d/a")==std::vector<std::uint8_t>{'X'} && copied.TryReadArchiveOnly("d/a")==std::vector<std::uint8_t>{'A'});
}

TEST_CASE(StartupIo_DirectDiskAndStartupStyleProbeDoNotConsumeDeferredCatalog) { // 실제setup의절대Probe와성공한디스크읽기는불필요한TAFF를열지않는다.
    using namespace netstorm;const auto root=StartupIoWork();StartupIoBadArchive(root/"unneeded.tarc");const std::array<std::uint8_t,1> disk{{'D'}};StartupIoWrite(root/"data.bin",disk);client::ClientOptions options;options.gameDirectory=root;options.noAudio=true;client::Client instance(std::move(options));const auto& files=instance.Files();
    CHECK(files.Read("data.bin")==std::vector<std::uint8_t>{'D'});CHECK(files.Probe(platform::PathToUtf8(root/"data.bin"))==5);CHECK(files.MatchDisk("*.bin")==std::vector<std::string>{"data.bin"});
}
