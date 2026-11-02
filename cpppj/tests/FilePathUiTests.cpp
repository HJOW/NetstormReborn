// 옵션/캡처/CD/기술 지원의 실제 경로 고리와 디스크 경계를 창 없이 확인한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "client/ClientOptionsFile.h"
#include "client/ClientOptionsDisk.h"
#include "client/ClientCaptureSequence.h"
#include "client/ClientCdGlobals.h"
#include "client/CdDirectory.h"
#include "client/IndexedBitmapFile.h"
#include "client/TechnicalHelp.h"
#include "o/RawFileName.h"
#include "platform/Utf8Path.h"
#include <chrono>
#include <fstream>
#include <iterator>

namespace { // 보호 경로 밖의 새 전용 파일과 원문 fixture 바이트만 준비한다.
std::string UiPathBytes(std::string_view hex) { // 상위BYTE/빈 문자열을 변환 없이 복원한다.
    std::string result;if (hex=="-") return result;
    for (std::size_t at=0;at<hex.size();at+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(at,2)),nullptr,16)); // 두16진수마다 원래 한BYTE다.
    return result;
}
std::filesystem::path UiPathWork(std::string_view label) { // 반복 실행에도 이전 파일을 덮지 않는 새 Unicode 게임 디렉터리다.
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-file-path-ui-729/tests"/(std::string(label)+'-'+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))/L"게임😀";
}
void WriteUiPath(const std::filesystem::path& path,std::string_view value) { // 명시 새 부모/파일만 검사 입력으로 만든다.
    std::filesystem::create_directories(path.parent_path());std::ofstream file(path,std::ios::binary);file.write(value.data(),static_cast<std::streamsize>(value.size()));CHECK(file.good());
}
std::vector<std::uint8_t> ReadUiPath(const std::filesystem::path& path) { // 실제 OS 저장 바이트를 다시 연다.
    std::ifstream file(path,std::ios::binary);CHECK(file.good());return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
}

//441d10의8개 경로/전체256BYTE와440f30의 이름 한 칸 뒤에도 writer가 지역 이름을 보존하는지 대조한다.
TEST_CASE(FilePathUi_OptionsNative8LocalCopyAndFailureOrdering) {
    using namespace netstorm;std::size_t count{};
    for (const auto& row:test::rawscene::LoadFixture(NETSTORM_OPTIONS_PATH_PARENT_FIXTURE).rows) { // 기대 이름은 제품이 아닌 실제 부모 기계어 출력이다.
        CHECK(row.size()==4);o::RawFileName names;o::ConfigInterface config({});config.Startup({});config.Set("InstallDir","H:\\게임😀");const auto root=UiPathBytes(row[1]);const auto directory=row[0]=="null" ? std::nullopt:std::make_optional(std::string_view(root));unsigned writes{};
        client::SaveClientOptions(config,[&](auto name,auto bytes) { ++writes;CHECK(name==UiPathBytes(row[2]) && names.Index()==2 && test::rawscene::Hex(names.CurrentBytes())==row[3]);CHECK(o::ConfigText::FromBytes(bytes).GetRaw("InstallDir")=="H:\\게임😀");
            const std::array<o::FileNamePart,1> overwrite{{{-3,"overwritten"}}}; // 실제 writer 재진입이 같은 열 칸을 한 바퀴 쓰는 입력이다.
            for (unsigned call=0;call<10;++call) static_cast<void>(names.Format(overwrite)); // 최초 formatter 반환 칸까지 덮어쓴다.
            CHECK(name==UiPathBytes(row[2]) && names.Index()==2);CHECK(config.Configuration().changed);
        },names,directory);CHECK(writes==1 && !config.Configuration().changed);++count;
    }
    CHECK(count==8);o::RawFileName names;o::ConfigInterface config({});config.Startup({});config.Set("InstallDir",std::string("bad\xc0\xaf",5));unsigned writes{};bool invalid{};
    try { client::SaveClientOptions(config,[&](auto,auto) { ++writes; },names); }catch (const std::invalid_argument&) { invalid=true; }
    CHECK(invalid && writes==0 && names.Index()==2 && config.Configuration().changed); // 직렬화 실패보다 먼저 부모 경로/지역 복사와 이름 getter 두 칸이 완료된다.
    config.Set("InstallDir","H:\\게임😀");bool failed{};
    try { client::SaveClientOptions(config,[&](auto name,auto) { CHECK(name==".\\d\\options.cfg");++writes;throw std::runtime_error("쓰기 실패 관측"); },names); }catch (const std::runtime_error&) { failed=true; }
    CHECK(failed && writes==1 && names.Index()==4 && config.Configuration().changed);
}

// 실제 Client::SaveOptions가 쓰는 같은 디스크 어댑터의 Unicode 저장/재적재와 부모 부재 실패를 확인한다.
TEST_CASE(FilePathUi_OptionsActualGameDirectoryDiskAndMissingParent) {
    using namespace netstorm;const auto root=UiPathWork("options");std::filesystem::create_directories(root/"d");o::ConfigInterface config({});config.Startup({});config.Set("InstallDir",platform::PathToUtf8(root));o::RawFileName names;
    client::SaveClientOptionsInGameDirectory(config,root,names);CHECK(names.Index()==4 && !config.Configuration().changed);const auto bytes=ReadUiPath(root/"d/options.cfg");CHECK(o::ConfigText::FromBytes(bytes).GetRaw("InstallDir")==platform::PathToUtf8(root));CHECK(!std::filesystem::exists(root/"options.cfg"));
    const auto absent=UiPathWork("absent");config.Set("InstallDir","changed");bool failed{};
    try { client::SaveClientOptionsInGameDirectory(config,absent,names); }catch (const std::runtime_error&) { failed=true; }
    CHECK(failed && names.Index()==8 && config.Configuration().changed && !std::filesystem::exists(absent));
}

// 캡처의 실제BMP 출력과 CD 캐시 확인이 Client/VFS의 같은 고리 번호를 이어서 소비한다.
TEST_CASE(FilePathUi_CaptureBitmapAndCdPresenceShareActualClientRing) {
    using namespace netstorm;const auto root=UiPathWork("capture");std::filesystem::create_directories(root/"d");client::ClientOptions options;options.gameDirectory=root;options.noAudio=true;client::Client instance(std::move(options));auto& names=instance.StaticStrings()->FileNames();CHECK(instance.Files().FileNameOwner().get()==&names);
    const auto path=client::ResolveCaptureBitmapPath(root,"d/frame000.bmp",&names);CHECK(path==root/"d/frame000.bmp" && names.Index()==2);
    const std::array<std::uint8_t,4> pixels{1,2,3,4};const std::array<std::uint8_t,1024> palette{};client::IndexedBitmapScratch scratch;client::WriteIndexedBitmapFile(path,pixels,{0,0,4,1,4},palette,scratch);const auto bitmap=ReadUiPath(path);CHECK(bitmap.size()==1082 && bitmap[0]=='B' && bitmap[1]=='M' && bitmap[1078]==1 && bitmap[1081]==4); // 논리 팔레트는 원문 RGB0 256개이며 packed RGB24가 아니다.
    CHECK(client::ResolveCaptureBitmapPath(root,platform::PathToUtf8(path),&names)==path && names.Index()==3);const auto drive=platform::PathToUtf8(root.root_name());CHECK(client::ResolveCaptureBitmapPath(root,drive+"frame001.bmp",&names)==root/"frame001.bmp" && names.Index()==4);
    auto hooks=client::MakeWindowsCdPresenceHooks(&names);CHECK(hooks.path("Q:\\",client::kCdPresenceMarker)=="Q:\\"+std::string(client::kCdPresenceMarker) && names.Index()==5);
    auto cache=instance.CdGlobals()->CachedRoot();cache[0]='Q';cache[1]=':';cache[2]='\\';cache[3]=0;hooks.exists=[](auto path) { CHECK(path=="Q:\\"+std::string(client::kCdPresenceMarker));return true; };hooks.roots=[](auto)->std::uint32_t { throw std::logic_error("캐시 성공은 실제 CD 열거 불필요"); }; // 실제 OS 장치 삽입/열거를 새로 시험하지 않는다.
    CHECK(instance.CdGlobals()->Check(hooks)==1 && names.Index()==6);CHECK(client::ResolveCaptureBitmapPath(root,"d/frame002.bmp",&names)==root/"d/frame002.bmp" && names.Index()==8);
}

// 외부 프로세스를 실행하지 않고 실제 기본/보조/직접 Help 조회의 장치 한 칸과 경로 한 칸을 센다.
TEST_CASE(FilePathUi_TechnicalHelpActualClientPrimarySecondaryAndDirect) {
    using namespace netstorm;const auto root=UiPathWork("help"),disc=UiPathWork("disc");WriteUiPath(root/"d/program.exe","not executed");WriteUiPath(root/"d/help.txt","help");WriteUiPath(disc/"d/program.exe","not executed");WriteUiPath(disc/"d/help.txt","help");client::ClientOptions options;options.gameDirectory=root;options.noAudio=true;client::Client instance(std::move(options));auto& config=instance.Configuration();auto& names=instance.StaticStrings()->FileNames();config.Set("techHelpExe","d/program.exe");config.Set("techHelpFile","d/help.txt");
    CHECK(client::ClientTechnicalHelpAvailable(instance) && names.Index()==4);std::filesystem::remove(root/"d/program.exe");std::filesystem::remove(root/"d/help.txt");config.Set("CDDir",platform::PathToUtf8(disc));CHECK(client::ClientTechnicalHelpAvailable(instance) && names.Index()==0);
    config.Set("techHelpExe","C:\\not-existing-program.exe");config.Set("techHelpFile","C:\\not-existing-help.txt");CHECK(client::ClientTechnicalHelpAvailable(instance) && names.Index()==2); // 원문 직접 장치는 존재를 확인하지 않는다. 실행은 호출하지 않는다.
    config.Set("techHelpExe","d/missing.exe");CHECK(!client::ClientTechnicalHelpAvailable(instance) && names.Index()==5); // 첫 조회 실패는 두 번째 이름을 해석하지 않는다.
    config.Set("techHelpExe","C:\\netstorm-absent-729\\*.exe");CHECK(!client::ClientTechnicalHelpAvailable(instance) && names.Index()==8); // 실패 FindFirstFile은 FileSpec을 비우므로 직접 장치 성공으로 취급하면 안 된다.
}
