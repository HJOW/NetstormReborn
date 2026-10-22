// 보호 원본을 바꾸지 않고 실제 Notify/저장 부모/파일 장치/다시 읽기 전체 경로를 확인한다.
#include "client/EditorFiles.h"
#include "client/CampaignTemplateStorage.h"
#include "DudeSpawnSupport.h"
#include <chrono>
using namespace netstorm::client;

namespace {
// 기존 사용자 작업본을 정리하거나 덮어쓰지 않고 이번 검사만의 새 작업 디렉터리를 사용한다.
std::filesystem::path WorkRoot(std::string_view label) {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();return std::filesystem::path(__FILE__).parent_path().parent_path()/"build/test-data"/("editor-files-"+std::string(label)+'-'+std::to_string(stamp));
}
// 원본/CD의 실제 읽기 전용 디렉터리를 모두 명시한다.
std::vector<std::filesystem::path> Protected() {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();return {root/"originals",root/"originalCD"};
}
// 실패한 쓰기를 정상 저장으로 해석하지 않는다.
template<class Function> bool Rejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
// 실제 현재 타입 개수와 예상 밖 직렬화 진단을 연결한다.
FortImageHooks ImageHooks() { return {[] { return 188U; },[](auto) { throw std::logic_error("편집 파일 검사에서 예상하지 않은 이미지 진단"); }}; }
}

// TEST01의 실제 원문을 복사한 뒤 같은 현재 State를 Notify로 저장하고 다시 읽는다.
TEST_CASE(EditorFiles_NotifySaveAndReloadProtectedTest01) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/TEST01.fort");EditorFiles files(WorkRoot("roundtrip"),Protected());const auto path=files.Prepare("TEST01",original);CHECK(ReadFileBytes(path)==original);
    CampaignTemplateStorage storage;storage.InstallFile(1,FortTemplate::Parse(original,assets.game.TypeTable()),original,path.string());storage.Borrow(8,1);const auto sourceFlag=storage.FileHeader(1)[1];storage.Template(1).flag=sourceFlag ? 0 : 0xfe; //파서의 과거 flag 사본을 일부러 현재 manager 서명과 다르게 만든다.
    std::uint32_t depth{},readOnly{},owner=1;bool mission{};std::array<std::uint8_t,9> hidden{};std::size_t saves{};
    RawPlayerTemplateState state(OriginalEdition::Patch1078,storage.Headers(),[&](auto slot)->PlayerTemplateStateStream& { return storage.SelectState(slot); },[&](auto slot)->const std::string& { return storage.Name(slot); },depth,mission,[](auto) { throw std::logic_error("편집 Notify 예상 밖 진단"); },[](auto) { throw std::logic_error("PATCH 편집 Notify의 CD 깊이 변경 금지"); });
    state.BindMinimumSize([&](auto slot,auto size)->PlayerTemplateStateStream& { return storage.EnsureStateSize(slot,size); });
    state.BindFileSave([&](auto slot) { ++saves;storage.SaveFile(slot,{readOnly,owner,hidden},ImageHooks(),files.Writer()); });
    auto bytes=state.Get(1);CHECK(bytes.size()>=28 && bytes[17]==2);bytes[12]^=0x5a;const auto position=storage.State(1).position;state.Changed(8);CHECK(saves==1 && storage.CurrentSection(1)=="Terr19" && storage.State(1).position==position); //빌린 헤더도 같은 현재 manager를 저장한다.
    const auto saved=ReadFileBytes(path);CHECK(saved!=original && saved[1]==(sourceFlag ? 0xfe : 0));const auto decoded=FortTemplate::Parse(saved,assets.game.TypeTable());CHECK(decoded.sections==storage.Template(1).sections);
    CHECK(files.Prepare("TEST01",original)==path && ReadFileBytes(path)==saved); //다음 진입 준비가 기존 사용자 작업본을 원본으로 되돌리지 않는다.
    bytes=state.Get(1);bytes[12]^=0x19;readOnly=1;state.Changed(1);CHECK(saves==2 && ReadFileBytes(path)==saved);readOnly=0;hidden[owner]=1;state.Changed(1);CHECK(saves==3 && ReadFileBytes(path)==saved);
    storage.Headers()[20+8]=1;state.Changed(1);CHECK(saves==3);storage.Headers()[20+8]=0;storage.Headers()[20+16]=1;state.Changed(1);CHECK(saves==3);
    CHECK(assets.files.Read("d/TEST01.fort")==original); //원본은 복사 입력/재검증만 수행한다.
}

// 신규 manager 서명의-1 기본값 정규화와 signed 타입 수가 저장 억제 중에도 실행된다.
TEST_CASE(EditorFiles_RealManagerSignatureAndCurrentSearch) {
    CampaignTemplateStorage storage;FortTemplate fort;fort.flag=0xfe;fort.sections.resize(35);storage.Install(0,std::move(fort),"blank.fort");CHECK((storage.FileHeader(0)==std::array<std::uint32_t,2>{0,0xffffffffU}));
    std::uint32_t readOnly=1,owner=999,assertions{};std::array<std::uint8_t,9> hidden{};auto image=ImageHooks();image.typeCount=[] { return 0xffffffffU; };image.assertion=[&](auto) { ++assertions; };
    storage.SaveFile(0,{readOnly,owner,hidden},image,{});CHECK(!assertions && storage.CurrentSection(0)=="Terr19" && (storage.FileHeader(0)==std::array<std::uint32_t,2>{0x46,0}));
    readOnly=0;owner=0;std::vector<std::uint8_t> written;std::size_t permissions{};TemplateFileWriteHooks file;
    file.permissions=[&](auto name,auto mode) { CHECK(name=="blank.fort" && mode==0x180);++permissions; };file.write=[&](auto name,auto bytes) { CHECK(name=="blank.fort");written.assign(bytes.begin(),bytes.end()); };
    storage.SaveFile(0,{readOnly,owner,hidden},image,file);CHECK(permissions==1 && written.size()==72 && written[0]=='F' && !written[1]);
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CHECK(FortTemplate::Parse(written,assets.game.TypeTable()).sections==storage.Template(0).sections);
}

// 파일 경로와 권한 변경 모두 원본/CD 또는 작업 디렉터리 밖을 대상으로 할 수 없어야 한다.
TEST_CASE(EditorFiles_RejectsProtectedPathsAndKeepsExistingWork) {
    const auto roots=Protected();const auto original=ReadFileBytes(roots[0]/"d/TEST01.fort");const auto root=WorkRoot("guards");EditorFiles files(root,roots);
    CHECK(Rejects([&] { EditorFiles wrong(roots[0]/"forbidden",roots); }));CHECK(Rejects([&] { EditorFiles wrong(root,{}); }));
    for (const auto name:{"","../outside","with.dot","x/y","x\\y","x:y"}) CHECK(Rejects([&] { files.Prepare(name,original); })); //원본 pending 이름과 물리 파일 경로 계약을 확인한다.
    CHECK(Rejects([&] { files.Write((roots[0]/"d/TEST01.fort").string(),{}); }));CHECK(Rejects([&] { files.MakeWritable((roots[0]/"d/TEST01.fort").string(),0x180); }));
    CHECK(Rejects([&] { files.Write((roots[1]/"NETSTORM.EXE").string(),{}); }));CHECK(Rejects([&] { files.Write("../outside.fort",{}); }));
    const auto path=files.Prepare("valid",original);std::vector<std::uint8_t> edited{1,2,3};files.Write(path.string(),edited);CHECK(files.Prepare("valid",original)==path && ReadFileBytes(path)==edited);CHECK(Rejects([&] { files.MakeWritable(path.string(),0); }));
    CHECK(ReadFileBytes(roots[0]/"d/TEST01.fort")==original);
}

// 잘못된 원본 이름을 저장 부모에 넘겨도 실제 파일 권한/내용을 바꾸기 전에 거부한다.
TEST_CASE(EditorFiles_ProtectedNameRejectedThroughSaveParent) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto path=assets.root/"d/TEST01.fort";const auto original=ReadFileBytes(path);CampaignTemplateStorage storage;storage.InstallFile(1,FortTemplate::Parse(original,assets.game.TypeTable()),original,path.string());
    std::uint32_t readOnly{},owner=1;std::array<std::uint8_t,9> hidden{};EditorFiles files(WorkRoot("parent"),Protected());CHECK(Rejects([&] { storage.SaveFile(1,{readOnly,owner,hidden},ImageHooks(),files.Writer()); }));CHECK(ReadFileBytes(path)==original);
}
