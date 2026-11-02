// 원문6후보탐색의전체고리와 실제Unicode설치/기본·CD아카이브/설정reader를 확인한다.
#include "RawSceneSupport.h"
#include "client/StartupFiles.h"
#include "o/RawFileName.h"
#include "o/RawBaseFileReadPolicy.h"
#include "o/ConfigInterface.h"
#include "platform/Utf8Path.h"
#include <chrono>
#include <fstream>

namespace { // 기대 결과는 원문fixture에서읽고 자체파일은extracted의고유폴더만사용한다.
std::string LocationBytes(std::string_view hex) { // 첫NUL뒤BYTE도줄이지않는다.
    std::string result;if (hex=="-") return result;
    for (std::size_t at=0;at<hex.size();at+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(at,2)),nullptr,16)); // 두자리마다한BYTE다.
    return result;
}
std::filesystem::path LocationWork() { // 새검사마다다른Unicode경로를생성한다.
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-startup-location-735/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/L"설치😀";
}
void LocationWrite(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) { // 추적원본/fixture대신새입력만쓴다.
    std::filesystem::create_directories(path.parent_path());std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));CHECK(file.good());
}
void LocationConfig(const std::filesystem::path& path,std::string_view text) { const auto bytes=netstorm::o::EncodeConfigFile(text);LocationWrite(path,bytes); } // 원문서명/XOR가있는실제설정파일이다.
void LocationArchive(const std::filesystem::path& path,char first,char second) { // 기존Native등록fixture와동일한최소TAFF입력형식에명시내용두BYTE를넣는다.
    using netstorm::test::rawscene::Put;std::vector<std::uint8_t> data(0x56);const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),data.begin());
    const std::array<std::pair<std::size_t,std::uint32_t>,11> fields{{{0x14,2},{0x1c,0x2c},{0x20,0x34},{0x24,0x20},{0x28,0x54},{0x2c,0x10},{0x30,0},{0x34,0},{0x38,1},{0x44,1},{0x48,1}}}; // 헤더·이름인덱스·자료영역의명시입력이다.
    for (const auto [offset,value]:fields) Put(data,offset,value); // 원문폭DWORD로작성한다.
    const std::string a="\\d\\a",b="\\d\\b";std::copy(a.begin(),a.end(),data.begin()+0x3c);std::copy(b.begin(),b.end(),data.begin()+0x4c);data[0x54]=static_cast<std::uint8_t>(first);data[0x55]=static_cast<std::uint8_t>(second);LocationWrite(path,data);
}
}

TEST_CASE(StartupLocation_Native84CandidatesAndWholeRing) { // 정상72행과assert12행의전체버퍼를독립기계어출력과대조한다.
    using namespace netstorm;using namespace test::rawscene;std::size_t count{},failures{};
    for (const auto& row:LoadFixture(NETSTORM_STARTUP_LOCATION_FIXTURE).rows) { // 상대/장치/UNC와초기번호0/9를같은Probe정책으로재생한다.
        CHECK(row.size()==8);o::RawFileName names;std::array<const std::uint8_t*,10> slots{};
        for (unsigned call=0;call<10;++call) { static_cast<void>(names.Format({}));slots[names.Index()]=names.CurrentBytes().data(); } // 실제각칸주소를빌리고BSS0의번호0으로돌아간다.
        for (std::uint32_t call=0;call<Number(row[2]);++call) static_cast<void>(names.Format({})); // Native와같은초기현재번호다.
        std::vector<std::string> probes;bool failed{};std::string prefix;
        try { prefix=client::LocateStartupData(LocationBytes(row[0]),names,[&](auto input) { const auto ordinal=probes.size();probes.emplace_back(input);return o::ProbeBaseFileByPolicy(input,[&](auto site,auto name) {
            if (site==o::BaseFileReadSite::PrimaryDisk || site==o::BaseFileReadSite::SecondaryDisk) static_cast<void>(o::JoinBaseFileRoot("",name,names)); // 명시Native최초BSS의기본/보조빈root와같은실제조합이다.
            return (site==o::BaseFileReadSite::DirectDisk || site==o::BaseFileReadSite::PrimaryDisk) && ordinal==Number(row[1]);
        },&names); }); }catch (const std::logic_error&) { failed=true; }
        CHECK(failed==(row[3]=="assert") && names.Index()==Number(row[6]));CHECK(prefix==LocationBytes(row[5]));const auto expected=Split(row[4],',');CHECK(probes.size()==expected.size());
        for (std::size_t at=0;at<probes.size() && at<expected.size();++at) CHECK(probes[at]==LocationBytes(expected[at])); // 원문이실제로방문한후보BYTE와순서를확인한다.
        for (std::size_t slot=0;slot<slots.size();++slot) CHECK(Hex(std::span<const std::uint8_t>(slots[slot],256))==std::string_view(row[7]).substr(slot*512,512)); // 전체고리는조회실패후의부분내용도포함한다.
        failures+=failed;++count;
    }
    CHECK(count==84 && failures==12);
}

TEST_CASE(StartupLocation_ActualDiskSixCandidatesStopAtFirstSuccess) { // 각후보의실제setup파일과선택이후후보미방문을확인한다.
    using namespace netstorm;const auto work=LocationWork();constexpr std::array<std::string_view,6> paths{{".","d","../d","../../d","..","../.."}}; // Native에대응하는실제OS입력폴더다.
    for (std::size_t hit=0;hit<paths.size();++hit) { // 자체고유설치마다첫성공을달리배치한다.
        const auto root=work/std::to_string(hit)/"a/b";std::filesystem::create_directories(root);const auto data=(root/platform::PathFromUtf8(paths[hit])).lexically_normal();LocationConfig(data/"setup.cfg","mQdsTCase=1\r\n");o::BaseFileSystem files(root);auto owner=files.FileNameOwner();std::size_t probes{};
        const auto selected=client::LocateStartupData(platform::PathToUtf8(root),*owner,[&](auto name) { ++probes;return files.Probe(name); });
        CHECK(std::filesystem::equivalent(platform::PathFromUtf8(selected),data) && probes==hit+1);
    }
}

TEST_CASE(StartupLocation_UnicodeVirtualCwdReplacesPreloadAndKeepsReaderAndCd) { // 자식폴더EXE에서찾은설정·부모자료와별도CD를같은VFS/기존reader에연결한다.
    using namespace netstorm;const auto work=LocationWork(),install=work/"package",entry=install/"bin",cd=work/L"CD자료",cwd=std::filesystem::current_path();
    LocationConfig(install/"d/setup.cfg","mQdsT\r\nSCREENW=1234\r\nMarker=\"selected\"\r\n");std::filesystem::create_directories(entry);LocationArchive(entry/"stale.tarc",'X','x');LocationArchive(install/"live.tarc",'P','p');LocationArchive(cd/"disc.tarc",'S','s'); // 원문서명첫줄은키로읽히지않으므로검사값은다음줄이다.
    o::BaseFileSystem files(entry);files.RegisterArchives();const auto owner=files.FileNameOwner();CHECK(files.Read("d/a")==std::vector<std::uint8_t>{'X'});const auto before=owner->Index();o::ConfigInterface config([&files](auto name) { return files.TryRead(name); },owner);std::uint32_t cdIndex{};
    const auto locations=client::PrepareStartupFiles(files,entry,[&] { cdIndex=owner->Index();return platform::PathToUtf8(cd); });
    CHECK(locations.installDirectory==install && std::filesystem::current_path()==cwd);CHECK(cdIndex==(before+9)%10 && owner->Index()==(before+4)%10);CHECK(files.FileNameOwner()==owner && locations.cdDirectory==platform::PathToUtf8(cd)); //736의기본/CD두pattern+각Full한칸씩이이전고리에서추가된다.
    CHECK(files.TryReadArchiveOnly("d/a")==std::vector<std::uint8_t>{'P'} && files.TryReadArchiveOnly("d/a",true)==std::vector<std::uint8_t>{'S'});CHECK(files.Read("d/a")==std::vector<std::uint8_t>{'P'});
    o::ConfigInterface::StartupOptions options;options.dataPrefix=locations.dataPrefix;options.installDir=platform::PathToUtf8(locations.installDirectory);options.cdDir=locations.cdDirectory;config.Startup(options);
    CHECK(config.GetInt("SCREENW")==1234 && config.Configuration().GetRaw("Marker")=="selected");CHECK(config.Configuration().GetRaw("InstallDir")==options.installDir && config.Configuration().GetRaw("CDDir")==options.cdDir); // 기존install과경로동일성은위에서검사하고설정에는실제선택한정규화문자열이그대로보관돼야한다.
}

TEST_CASE(StartupLocation_MissingSetupStopsBeforeCdAndKeepsExistingFiles) { // 원문전체실패를임의d경로로통과시키지않고파일장치를보존한다.
    using namespace netstorm;const auto root=LocationWork()/"empty/a/b";std::filesystem::create_directories(root);LocationArchive(root/"kept.tarc",'K','k');o::BaseFileSystem files(root);files.RegisterArchives();const auto owner=files.FileNameOwner();bool failed{},cdCalled{};
    try { static_cast<void>(client::PrepareStartupFiles(files,root,[&] { cdCalled=true;return std::string{}; })); }catch (const std::logic_error& error) { failed=std::string_view(error.what()).find(":195:")!=std::string_view::npos; }
    CHECK(failed && !cdCalled && files.FileNameOwner()==owner && owner->Index()==2);CHECK(files.TryReadArchiveOnly("d/a")==std::vector<std::uint8_t>{'K'});
}
