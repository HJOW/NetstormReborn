// 실제 VFS/Client/저장 상태의 같은 경로 고리와 OS 파일 조회를 창 없이 확인한다.
#include "TestSupport.h"
#include "RawSceneSupport.h" // 장치 getter의 원문 전체 버퍼 기대값만 읽는다.
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "client/SavedStateFiles.h"
#include "o/BaseFile.h"
#include "o/RawFileName.h"
#include <array>
#include <chrono>
#include <fstream>

namespace { // 원본/CD 밖의 새 검사 폴더와 바이트만 만드는 장치다.
std::string DeviceFixtureBytes(std::string_view hex) {
    std::string result;if (hex=="-") return result;
    for (std::size_t offset=0;offset<hex.size();offset+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(offset,2)),nullptr,16)); // 상위BYTE/중간NUL을 변환하거나 생략하지 않는다.
    return result;
}
std::filesystem::path FilePathWork(std::string_view name) {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-file-paths-728/tests"/(std::string(name)+'-'+std::to_string(stamp))/L"한글😀 설치";
}
void WritePathFile(const std::filesystem::path& path,std::string_view text) {
    std::filesystem::create_directories(path.parent_path());std::ofstream file(path,std::ios::binary);file.write(text.data(),static_cast<std::streamsize>(text.size()));CHECK(file.good());
} // 지정한 새 작업 파일만 쓰며 경로 조회 함수의 성공을 합성하지 않는다.
}
// 출력 문자열이 빈 경우에도 NUL 뒤에 남는 원래 FileSpec과 같은 고리 번호를 원문 그대로 대조한다.
TEST_CASE(FileNameDevice_ReplaysNative32WholeSlotsAndReadonlyInput) {
    using namespace netstorm::test::rawscene;std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_FILE_NAME_DEVICE_FIXTURE).rows) { // 두 separator와 UNC/drive/빈/255BYTE의 독립 CPU 결과만 기대값으로 쓴다.
        CHECK(row.size()==3);netstorm::o::RawFileName formatter;const auto input=DeviceFixtureBytes(row[1]),before=input,expected=DeviceFixtureBytes(row[2]);const auto result=formatter.Device(input,static_cast<char>(Number(row[0])));
        CHECK(formatter.Index()==1 && Hex(formatter.CurrentBytes())==row[2]);CHECK(result==std::string_view(expected.data(),expected.find('\0')) && input==before);CHECK(result.data()==reinterpret_cast<const char*>(formatter.CurrentBytes().data()));++count;
    }
    CHECK(count==32);
}
// 경로 값뿐 아니라 서로 다른 실제 소비자가 같은10칸을 소모하는지와 직접 장치의 getter 한 칸을 확인한다.
TEST_CASE(FilePathClient_ActualReadProbeReadonlyAndSavedFileShareTenSlots) {
    using namespace netstorm;const auto root=FilePathWork("client");WritePathFile(root/"d/one.dat","one");client::ClientOptions options;options.gameDirectory=root;client::Client instance(std::move(options));
    const auto owner=instance.Files().FileNameOwner();CHECK(owner.get()==&instance.StaticStrings()->FileNames());auto& names=*owner;const std::array<o::FileNamePart,1> parts{{{-3,"marker"}}};const auto marker=names.Format(parts);const auto first=names.Index();CHECK(marker==".\\marker");
    const auto advance=[&](std::uint32_t count) { CHECK(names.Index()==(first+count)%10); }; // 원문 caller의 장치 getter/formatter가 실제 소모한 같은 고리 칸을 센다.
    const std::vector<std::uint8_t> expected{'o','n','e'};CHECK(instance.Files().Read("d/one.dat")==expected);advance(2);
    CHECK(!instance.Files().ReadOnly("d/one.dat"));advance(4);CHECK(instance.Files().Probe("d/one.dat")==1);advance(6);
    const auto absolute=(root/"d/one.dat").u8string();CHECK(instance.Files().Read({reinterpret_cast<const char*>(absolute.data()),absolute.size()})==expected);advance(7); // 원문 직접 장치 성공도459900의 현재 장치 문자열 한 칸은 사용한다.
    CHECK(!instance.Files().TryRead("missing.dat"));advance(10); // 장치 getter 한 칸 뒤 기본/보조 disk가 같은 경로라도 각각 조합한다.
    const auto saved=client::SavedStateFilePath(root,names);CHECK(saved==std::filesystem::absolute(root/"d/!state.dat") && !std::filesystem::exists(saved));advance(11);
    WritePathFile(saved,"state");CHECK(client::DeleteSavedStateFile(root,names)==0 && !std::filesystem::exists(saved));advance(12);
    const auto aliasMarker=names.Format(parts);const auto aliasIndex=names.Index();
    for (std::uint32_t call=1;call<=4;++call) { CHECK(instance.Files().Probe("d/one.dat")==1);CHECK(names.Index()==(aliasIndex+2*call)%10 && aliasMarker==".\\marker"); } // 각 Probe는 장치 getter/디스크 이름 두 칸을 사용한다.
    static_cast<void>(client::SavedStateFilePath(root,names));CHECK(names.Index()==(aliasIndex+9)%10 && aliasMarker==".\\marker");
    static_cast<void>(client::SavedStateFilePath(root,names));CHECK(names.Index()==aliasIndex && aliasMarker.data()==reinterpret_cast<const char*>(names.CurrentBytes().data()) && aliasMarker!=".\\marker");
}
// 보조 root 변경·VFS 반납이 같은 살아 있는 장기 경로 소유자를 초기화하거나 먼저 해제하지 않는다.
TEST_CASE(FilePathClient_SecondaryLookupAndBorrowerKeepOneOwnerAlive) {
    using namespace netstorm;const auto root=FilePathWork("secondary"),disc=FilePathWork("disc");std::filesystem::create_directories(root);WritePathFile(disc/"d/two.dat","two");
    std::shared_ptr<o::RawFileName> retained;
    {
        o::BaseFileSystem files(root,disc),independent(root);retained=files.FileNameOwner();client::ClientStaticStrings borrowed(retained);CHECK(&borrowed.FileNames()==retained.get() && independent.FileNameOwner()!=retained && retained->Index()==0);
        const std::vector<std::uint8_t> expected{'t','w','o'};CHECK(files.Read("d/two.dat")==expected && retained->Index()==3);
        CHECK(files.Probe("d/two.dat")==3 && retained->Index()==6);CHECK(!files.ReadOnly("d/two.dat") && retained->Index()==9);
        files.SetSecondaryDirectory(root);CHECK(retained->Index()==9 && files.FileNameOwner()==retained);borrowed.Initialize(10240);CHECK(retained->Index()==9);
    }
    CHECK(retained && retained->Index()==9);const std::array<o::FileNamePart,1> parts{{{-3,"alive"}}};CHECK(retained->Format(parts)==".\\alive" && retained->Index()==0);
}
