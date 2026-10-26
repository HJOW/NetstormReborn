// 새 맵 부모가 실제 manager/공유 이름표/작업 파일 보호 장치를 함께 사용하는지 검사한다.
#include "RawSceneSupport.h"
#include "client/NewFortManagerDevices.h"
#include "client/CampaignTemplateStorage.h"
#include "client/EditorFiles.h"
#include "o/RawTemplateLoad.h"
#include <chrono>
#include <filesystem>

using namespace netstorm::client; // 같은 제품 저장소와 파일 장치만 사용한다.
using namespace netstorm::o; // 원문 manager 이름 getter와 Fort 섹션 순서를 대조한다.
using namespace netstorm::test::rawscene; // 기존 독립 BYTE 접근 검사 도우미를 재사용한다.

namespace { // 테스트마다 이전 사용자 작업본과 다른 새 경로를 고른다.
// 이번 검사만의 경로를 반환한다. 기존 디렉터리를 지우거나 덮어쓰지 않는다.
std::filesystem::path NewManagerWork(std::string_view name) {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::path(__FILE__).parent_path().parent_path()/"build/test-data"/("new-manager-"+std::string(name)+'-'+std::to_string(stamp));
}
// 읽기 전용 원본 두 루트를 실제 파일 장치에 제공한다.
std::vector<std::filesystem::path> NewManagerProtected() {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();return {root/"originals",root/"originalCD"};
}
}

// 파일 부재는 디스크에 빈 Fort를 조립하지 않고 실제 manager의35개 섹션을 생성한다.
TEST_CASE(NewFortManagerDevices_MissingFileInitializesRealImageAndKeepsStaleHeaderWords) {
    CampaignTemplateStorage storage;EditorFiles files(NewManagerWork("missing"),NewManagerProtected());const auto name=files.ResolvePath("d/New.fort").string();
    Put(storage.Headers(),28,0xa55a1234);Put(storage.Headers(),32,0x1234a55a);storage.SerializationVersion()=0xdeadbeef;
    CreateNewEditorFortManager(storage,files,1,name,0x89abcdef);
    const auto token=Get(storage.Headers(),20);CHECK(token!=0 && storage.OwnedImages()==1);CHECK(Get(storage.Headers(),24)==0);
    CHECK(Get(storage.Headers(),28)==0xa55a1234 && Get(storage.Headers(),32)==0x1234a55a && Get(storage.Headers(),36)==0x89abcdef);
    CHECK(storage.SerializationVersion()==0 && storage.Name(1)==name && TemplateDataManagerFileName(storage.ManagerBytes(token))==name);
    CHECK(storage.SectionOrder(1)==std::vector<std::string>(FortTemplate::SectionNames().begin(),FortTemplate::SectionNames().end()));
    CHECK(storage.CurrentSection(1)=="Terr19" && storage.SectionNamesOwner()->CurrentToken()==1 && !files.TryRead(name));
}

// 존재 재조회가 참이면 작업 파일만 삭제하고 임시 manager를 소멸/free한 뒤 최종 manager를 공개한다.
TEST_CASE(NewFortManagerDevices_ExistingWorkUsesTemporaryLifetimeBeforeFinalImage) {
    CampaignTemplateStorage storage;EditorFiles files(NewManagerWork("replace"),NewManagerProtected());const std::array<std::uint8_t,4> payload{'O','L','D',0};
    const auto path=files.PreparePath("d/Existing.fort",payload);CHECK(files.TryRead(path.string())==std::optional<std::vector<std::uint8_t>>(std::vector<std::uint8_t>(payload.begin(),payload.end())));
    CreateNewEditorFortManager(storage,files,1,path.string(),0);
    CHECK(Get(storage.Headers(),20)==2 && storage.OwnedImages()==1 && !files.TryRead(path.string()));
    CHECK(storage.Name(1)==path.string() && storage.SectionOrder(1).size()==35 && storage.CurrentSection(1)=="Terr19");
    storage.ClearAll();CHECK(storage.OwnedImages()==0 && storage.SectionNamesOwner()->CurrentToken()==1);
}

// 상대 파일명은 같은 작업 namespace에서 존재/삭제를 수행하되 manager의 원문 이름은 그대로 보존한다.
TEST_CASE(NewFortManagerDevices_RelativeNameChecksAndDeletesSameWorkspaceFile) {
    CampaignTemplateStorage storage;EditorFiles files(NewManagerWork("relative"),NewManagerProtected());const std::array<std::uint8_t,3> payload{'O','L','D'};
    const auto path=files.PreparePath("d/Relative.fort",payload);CHECK(files.Exists("d/Relative.fort")==5);
    CreateNewEditorFortManager(storage,files,1,"d/Relative.fort",0);
    CHECK(!files.TryRead(path.string()) && !files.Exists("d/Relative.fort"));
    CHECK(storage.Name(1)=="d/Relative.fort" && Get(storage.Headers(),20)==2 && storage.OwnedImages()==1);
}

// 보호 경로 거부는 실제 파일 효과 전에 중단하지만 원문이 먼저 쓴 두 헤더 필드를 복원하지 않는다.
TEST_CASE(NewFortManagerDevices_ProtectedPathRejectionKeepsNativePrefixWithoutFileMutation) {
    CampaignTemplateStorage storage;EditorFiles files(NewManagerWork("protected"),NewManagerProtected());const auto source=NewManagerProtected().front()/"Netstorm.exe";
    const auto size=std::filesystem::file_size(source);const auto modified=std::filesystem::last_write_time(source);Put(storage.Headers(),24,0x12345678);Put(storage.Headers(),28,0xa5a5a5a5);
    CHECK(Throws([&] { CreateNewEditorFortManager(storage,files,1,source.string(),7); }));
    CHECK(!Get(storage.Headers(),20) && !Get(storage.Headers(),24) && Get(storage.Headers(),28)==0xa5a5a5a5 && Get(storage.Headers(),36)==7);
    CHECK(!storage.OwnedImages() && !storage.SectionNamesOwner()->CurrentToken());CHECK(std::filesystem::file_size(source)==size && std::filesystem::last_write_time(source)==modified);
}

// 기존 manager 진단을 무시하고 먼저 Clear하는 편의 처리를 추가하지 않는다.
TEST_CASE(NewFortManagerDevices_ExistingManagerAssertionPrecedesAllMutations) {
    CampaignTemplateStorage storage;EditorFiles files(NewManagerWork("assertion"),NewManagerProtected());const auto first=files.ResolvePath("d/First.fort").string();CreateNewEditorFortManager(storage,files,1,first,0);
    const std::vector<std::uint8_t> before(storage.Headers().begin(),storage.Headers().end());std::uint32_t line{};NewFortManagerDiagnostics diagnostics;
    diagnostics.templateAssertion=[&](auto value) { line=value;throw std::logic_error("명시 진단 중단"); };
    CHECK(Throws([&] { CreateNewEditorFortManager(storage,files,1,files.ResolvePath("d/Other.fort").string(),9,diagnostics); }));
    CHECK(line==0x172 && storage.OwnedImages()==1 && storage.Name(1)==first && std::equal(before.begin(),before.end(),storage.Headers().begin()));
}
