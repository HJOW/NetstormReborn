// 원문 전체459990/435cd0의 최초 출력과 실제 Unicode 배포/Client 소유권 인계를 확인한다.
#include "RawSceneSupport.h"
#include "o/RawFileName.h"
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "platform/PortableStartup.h"
#include "platform/Utf8Path.h"
#include <chrono>
#include <fstream>

namespace { // 기대 경로를 재계산하지 않고 Native 전체 바이트를 읽는 검사 장치다.
std::string DirectoryBytes(std::string_view hex) { // 빈 입력과 상위 바이트를 그대로 복원한다.
    std::string result;if (hex=="-") return result;
    for (std::size_t at=0;at<hex.size();at+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(at,2)),nullptr,16)); // 한BYTE씩 해독한다.
    return result;
}
class DirectoryPackage { // 새로 만든 배포 자료는 Git 제외 extracted 아래의 고유 폴더만 소유한다.
public:
    DirectoryPackage():root(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-file-directory-734/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/L"휴대😀") {}
    void Archive(const std::filesystem::path& directory) { std::filesystem::create_directories(directory);std::ofstream(directory/"netstorm.tarc",std::ios::binary).put('x'); } // 등록에 실패하는 더미이며 실제 원본 자산은 사용하지 않는다.
    std::filesystem::path root; // 추적 원본이나 기존 fixture를 건드리지 않는 검사 전용 경로다.
};
}

TEST_CASE(FileDirectory_Native192WholeSlotsAndReadFaultBoundary) { // 정상162행과 원문 NULL 읽기30행의 부분 버퍼를 구별한다.
    using namespace netstorm;using namespace test::rawscene;std::size_t count{},faults{};
    for (const auto& row:LoadFixture(NETSTORM_FILE_DIRECTORY_FIXTURE).rows) { // separator/level/255BYTE 경계의 현재 슬롯 전체를 각각 비교한다.
        CHECK(row.size()==5);const auto input=DirectoryBytes(row[2]);o::RawFileName names;bool fault{};std::string_view value;
        try { value=names.Directory(input,std::stoi(row[1]),static_cast<char>(Number(row[0]))); }catch (const std::invalid_argument&) { fault=true; }
        CHECK(fault==(row[3]=="read-fault"));CHECK(names.Index()==1);CHECK(Hex(names.CurrentBytes())==row[4]);
        if (!fault) { const auto expected=DirectoryBytes(row[4]);CHECK(value==std::string_view(expected.data(),expected.find('\0')));CHECK(value.data()==reinterpret_cast<const char*>(names.CurrentBytes().data())); }
        faults+=fault;++count;
    }
    CHECK(count==192 && faults==30);
}

TEST_CASE(FileDirectory_NativeModuleParentFivePathsAndThreeSlots) { // OS 경계 뒤의 실제 부모 경로와 전체10칸을 비교한다.
    using namespace netstorm;using namespace test::rawscene;std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_FILE_MODULE_DIRECTORY_FIXTURE).rows) { // 명시 경로의 원문 복사만 분기는 별도 Native 근거로 두고 무인자5행을 제품과 비교한다.
        CHECK(row.size()==5);if (row[0]!="-") { CHECK(Number(row[3])==0);continue; }o::RawFileName names;std::array<const std::uint8_t*,10> slots{};
        for (unsigned call=0;call<10;++call) { static_cast<void>(names.Format({}));slots[names.Index()]=names.CurrentBytes().data(); } // 각 주소를 빌리고 현재 번호를 다시0으로 맞춘다.
        const auto path=platform::ModuleDirectory(DirectoryBytes(row[1]),names);CHECK(path==DirectoryBytes(row[2]) && names.Index()==Number(row[3]));
        for (std::size_t slot=0;slot<slots.size();++slot) CHECK(Hex(std::span<const std::uint8_t>(slots[slot],256))==std::string_view(row[4]).substr(slot*512,512)); // getter 두 칸과 최종 작성기 한 칸의 물리 내용을 대조한다.
        ++count;
    }
    CHECK(count==5);
}

TEST_CASE(FileDirectory_AliasClearAndPhysicalOverflowAreExplicit) { // 원문의 비정상 접근과 호스트 메모리 범위는 정상 빈 경로로 합성하지 않는다.
    using namespace netstorm::o;RawFileName names;const auto first=names.Directory(".\\d\\file");CHECK(first==".\\d\\");
    for (unsigned call=0;call<9;++call) static_cast<void>(names.Format({})); // 열 번째 목적지를 첫 출력의 별칭과 같게 만든다.
    bool fault{};try { static_cast<void>(names.Directory(first)); }catch (const std::invalid_argument&) { fault=true; }
    CHECK(fault && names.Index()==1);CHECK(std::all_of(names.CurrentBytes().begin(),names.CurrentBytes().end(),[](auto byte) { return byte==0; }));
    bool overflow{};try { static_cast<void>(names.Directory(std::string(256,'x'))); }catch (const std::length_error&) { overflow=true; }
    CHECK(overflow && names.Index()==2);CHECK(std::all_of(names.CurrentBytes().begin(),names.CurrentBytes().end(),[](auto byte) { return byte==0; }));
}

TEST_CASE(FileDirectory_PortableUnicodeOwnerContinuesIntoActualClient) { // Windows 파일 경로와 원문 버퍼의 소유권을 같은 실제 Client까지 넘긴다.
    using namespace netstorm;DirectoryPackage fixture;fixture.Archive(fixture.root/"game");auto names=std::make_shared<o::RawFileName>();
    const auto directory=platform::PortableGameDirectory(fixture.root/"Netstorm.exe",names.get());CHECK(directory==fixture.root/"game" && names->Index()==3);
    client::ClientOptions options;options.gameDirectory=directory;options.noAudio=true;options.startupFileNames=names;client::Client instance(std::move(options));
    CHECK(instance.Files().FileNameOwner()==names && &instance.StaticStrings()->FileNames()==names.get() && names->Index()==3);
    names.reset();auto owner=instance.Files().FileNameOwner();CHECK(owner.use_count()>=3);static_cast<void>(instance.StaticStrings()->FileNames().Format({}));CHECK(owner->Index()==4);
    const std::array<std::string,1> arguments{{"@d/options.cfg"}};instance.Configuration().Load(arguments);CHECK(owner->Index()==8); // 실제 설정 이름 한 칸과 누락 파일의 장치/기본/보조 세 칸이 같은 고리를 계속 쓴다.
    CHECK(!instance.Files().TryRead("d/missing.cfg") && owner->Index()==1); // 뒤의 직접 VFS 조회도 새 고리를 만들지 않는다.
}

TEST_CASE(FileDirectory_PortableLongUnicodeKeepsHostPathWithoutOverflow) { // 원본256BYTE 밖 경로는 기존 Windows10/11 호환 정책에 따라 OS 경로로 선택한다.
    using namespace netstorm;DirectoryPackage fixture;const auto root=fixture.root/std::wstring(75,L'한');fixture.Archive(root/"game");o::RawFileName names;
    CHECK(platform::PathToUtf8(root/"Netstorm.exe").size()>=256);CHECK(platform::PortableGameDirectory(root/"Netstorm.exe",&names)==root/"game" && names.Index()==0);
}
