// 원문 이름 getter의 반환 주소/전체 바이트와 실제 옵션 저장의 같은 Client 수명을 확인한다.
#include "RawSceneSupport.h"
#include "o/RawFileName.h"
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "client/ClientOptionsDisk.h"
#include "client/CdDirectory.h"
#include "platform/Utf8Path.h"
#include <chrono>
#include <fstream>

namespace { // 최초 기계어 fixture를 BYTE로만 복원한다.
std::string LeafFixtureBytes(std::string_view hex) { // 빈 문자열/중간NUL/상위BYTE를 그대로 보존한다.
    std::string result;if (hex=="-") return result;
    for (std::size_t at=0;at<hex.size();at+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(at,2)),nullptr,16)); // 두16진수마다 한BYTE다.
    return result;
}
}

// extension0/1·두separator·bare/빈/colon/UNC/선두dot/255BYTE를 원문64개와 비교한다.
TEST_CASE(FileNameLeaf_Native64OffsetsAndWholeBuffers) {
    using namespace netstorm::test::rawscene;std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_FILE_NAME_LEAF_FIXTURE).rows) { // 반환 offset과 NUL 뒤 원래 전체 문자열은 독립 CPU 기대값이다.
        CHECK(row.size()==6);netstorm::o::RawFileName names;const auto input=LeafFixtureBytes(row[2]),before=input;const auto result=names.Name(input,Number(row[1])!=0,static_cast<char>(Number(row[0])));
        CHECK(names.Index()==1 && input==before && result==LeafFixtureBytes(row[3]));CHECK(result.data()==reinterpret_cast<const char*>(names.CurrentBytes().data())+Number(row[4]));CHECK(Hex(names.CurrentBytes())==row[5]);++count;
    }
    CHECK(count==64);
}

// 내부 뷰도 열 번째 후속 호출에 덮이고 별칭 입력은 선행 clear 뒤 현재 바이트를 읽는다.
TEST_CASE(FileNameLeaf_InternalAliasAndPhysicalBoundary) {
    using namespace netstorm::o;RawFileName names;const auto alias=names.Name(".\\first.ext");CHECK(alias=="first.ext");const auto original=alias.data();
    for (unsigned call=0;call<9;++call) static_cast<void>(names.Format({})); // 원래 물리 칸만 마지막까지 남긴다.
    CHECK(alias=="first.ext" && names.Index()==0);const auto cleared=names.Name(alias);CHECK(cleared.empty() && names.Index()==1 && original[0]==0 && std::all_of(names.CurrentBytes().begin(),names.CurrentBytes().end(),[](auto byte) { return byte==0; }));
    bool rejected{};try { static_cast<void>(names.Name(std::string(256,'x'))); }catch (const std::length_error&) { rejected=true; }CHECK(rejected && names.Index()==2); // 원문의 배열 밖 미정 쓰기는 성공/절단으로 합성하지 않는다.
}

// 실제 Client 옵션 저장의4칸 뒤 VFS읽기2칸·CD경로1칸도 같은 물리 번호를 이어 쓴다.
TEST_CASE(FileNameLeaf_ActualOptionsReadAndCdKeepOneClientOwner) {
    using namespace netstorm;const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-file-name-leaf-730/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/L"한글😀";std::filesystem::create_directories(root/"d");client::ClientOptions options;options.gameDirectory=root;options.noAudio=true;client::Client instance(std::move(options));auto& names=instance.StaticStrings()->FileNames();CHECK(instance.Files().FileNameOwner().get()==&names);
    instance.Configuration().Startup({});instance.Configuration().Set("InstallDir",platform::PathToUtf8(root));const auto before=names.Index();client::SaveClientOptionsInGameDirectory(instance.Configuration(),root,names);CHECK(names.Index()==(before+4)%10 && !instance.Configuration().Configuration().changed); // Startup 자체의 실제 파일 읽기 소모를0으로 가정하지 않는다.
    const auto bytes=instance.Files().Read("d/options.cfg");CHECK(names.Index()==(before+6)%10 && o::ConfigText::FromBytes(bytes).GetRaw("InstallDir")==platform::PathToUtf8(root));auto cd=client::MakeWindowsCdPresenceHooks(&names);CHECK(cd.path("Q:\\",client::kCdPresenceMarker)=="Q:\\"+std::string(client::kCdPresenceMarker) && names.Index()==(before+7)%10);
    instance.Configuration().Set("InstallDir","relative-root");client::SaveClientOptionsInGameDirectory(instance.Configuration(),std::filesystem::relative(root,std::filesystem::current_path()),names);CHECK(names.Index()==(before+11)%10);const auto relative=instance.Files().Read("d/options.cfg");CHECK(names.Index()==(before+13)%10 && o::ConfigText::FromBytes(relative).GetRaw("InstallDir")=="relative-root"); // 상대 게임 root를 두 번 붙이지 않고 같은 실제 파일을 갱신한다.
}
