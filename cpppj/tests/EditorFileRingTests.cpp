// 실제Template의Edit파일조회가Client와같은경로고리를계속사용하는지확인한다.
#include "RawSceneSupport.h"
#include "client/EditorFortFileDevices.h"
#include "o/RawFileName.h"
#include "platform/Utf8Path.h"
#include <chrono>
#include <fstream>

namespace { // Git제외scratch만입력으로만들고보호원본은사용하지않는다.
std::filesystem::path EditorRingWork() { // 같은검사프로세스에서도고유한Unicode작업경로다.
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-editor-file-ring-738/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/L"편집😀";
}
}

TEST_CASE(EditorFileRing_TemplateReadQueriesContinueSourceOwner) { // 원문절대Exists/readonly/읽기는같은장치getter한칸씩을쓴다.
    using namespace netstorm;const auto root=EditorRingWork();o::BaseFileSystem source(root/"source");client::EditorFiles work(root/"work",{root/"source"});o::ConfigInterface config({});client::EditorFortFileDevices devices(work,source,config,{});const std::array<std::uint8_t,2> bytes{{'F',0}};const auto path=platform::PathToUtf8(work.PreparePath("d/map.fort",bytes));auto names=source.FileNameOwner();
    CHECK(devices.Exists(path)==5 && names->Index()==1);CHECK(!devices.ReadOnly(path) && names->Index()==2);CHECK(devices.Read(path)==std::vector<std::uint8_t>(bytes.begin(),bytes.end()) && names->Index()==3);
    CHECK(work.FileNameOwner()==names); // 두장치가단지같은번호가아닌실제같은소유자를빌린다.
}

TEST_CASE(EditorFileRing_MissingQueriesPreserveReservationAndWrapBorrowedView) { // 누락파일/진단뒤에도같은현재번호와기존반환뷰의수명을보존한다.
    using namespace netstorm;const auto root=EditorRingWork();auto names=std::make_shared<o::RawFileName>();client::EditorFiles work(root/"work",{root/"source"},names);std::vector<std::uint32_t> assertions;bool failed{};const auto missing=platform::PathToUtf8(work.ResolvePath("missing.fort"));
    CHECK(work.Exists(missing)==0 && names->Index()==1);try { static_cast<void>(work.ReadOnly(missing,[&](auto line) { assertions.push_back(line); })); }catch (const std::runtime_error&) { failed=true; }
    CHECK(failed && assertions==std::vector<std::uint32_t>{0x48e} && names->Index()==2);CHECK(!work.TryRead(missing) && names->Index()==4); // VFS실패는장치1+같은작업보조root조합1이며source TAFF를등록하지않는다.
    const std::array<o::FileNamePart,1> parts{{{-3,"marker"}}};const auto marker=names->Format(parts);const auto at=names->Index();
    for (unsigned call=0;call<9;++call) CHECK(work.Exists(missing)==0); // 원문열번째칸까지돌기전에는처음빌린주소가유지된다.
    CHECK(marker==".\\marker" && names->Index()==(at+9)%10);CHECK(work.Exists(missing)==0 && names->Index()==at && marker!=".\\marker");
}

TEST_CASE(EditorFileRing_MetadataNamespaceAndRejectedPathsKeepClientRing) { // cpppj전용장부I/O와보호경로거부를원문파일조회로합산하지않는다.
    using namespace netstorm;const auto root=EditorRingWork();auto names=std::make_shared<o::RawFileName>();client::EditorFiles work(root/"work",{root/"source"},names);auto metadata=work.SiblingNamespace("-state");const std::array<std::uint8_t,1> bytes{{7}};const auto path=platform::PathToUtf8(metadata.PreparePath("catalog.dat",bytes));
    CHECK(metadata.FileNameOwner()!=names && metadata.Exists(path)==5 && metadata.TryRead(path)==std::vector<std::uint8_t>{7});CHECK(names->Index()==0);
    bool rejected{};try { static_cast<void>(work.TryRead("../source/escape.fort")); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected && names->Index()==0); // 보호검사밖요청은파일/Native고리를빌리기전에거부한다.
}

TEST_CASE(EditorFileRing_ActualTemplateLoadUsesOwnerUntilTitleBoundary) { // 실제RawTemplate/DataManager서명·35섹션읽기에서readonly/read두칸을같은Client장치에연결한다.
    using namespace netstorm;const auto root=EditorRingWork();o::BaseFileSystem source(root/"source");client::EditorFiles work(root/"work",{root/"source"});o::ConfigInterface config({});client::EditorFortFileDevices devices(work,source,config,{});std::vector<std::uint8_t> image{'F',0};
    for (unsigned section=0;section<35;++section) { image.push_back(2);image.push_back(0); } // 실제최소35섹션입력이며내용물은없다.
    const auto path=platform::PathToUtf8(work.PreparePath("d/map.fort",image));auto names=source.FileNameOwner();bool title{};client::TemplateFileLoadHooks effects;effects.typeCount=[] { return 188; };effects.currentSlot=[] { return 0U; };effects.refreshTitle=[&] { title=true;CHECK(names->Index()==2); };effects.wrongName=[] { throw std::logic_error("예상밖Template이름진단"); };effects.tooManyTypes=[] { throw std::logic_error("예상밖TYPE진단"); };effects.rejectFile=[](auto) { throw std::logic_error("예상밖서명삭제"); };
    client::CampaignTemplateStorage storage;storage.LoadFile(0,{},path,devices.BindLoad(std::move(effects)));CHECK(title && names->Index()==2 && storage.OwnedImages()==1 && storage.Name(0)==path);
}
