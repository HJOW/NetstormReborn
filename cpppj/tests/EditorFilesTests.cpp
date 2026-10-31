// 보호 원본을 바꾸지 않고 실제 Notify/저장 부모/파일 장치/다시 읽기 전체 경로를 확인한다.
#include "client/EditorFiles.h"
#include "client/FortSelection.h"
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

TEST_CASE(EditorFiles_SaveParentPreservesTailWhenCurrentTemplateGetsShorter) { // 원문 저장 부모의72BYTE 빈 이미지가 기존 긴 파일의 끝을 자르지 않고 현재 서명/섹션만 갱신해야 한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/TEST01.fort");CHECK(original.size()>72);if (original.size()<=72) return;EditorFiles files(WorkRoot("save-tail"),Protected());const auto path=files.Prepare("ShorterImage",original);
    FortTemplate fort;fort.sections.resize(35);CampaignTemplateStorage storage;storage.Install(1,std::move(fort),path.string());std::uint32_t readOnly{},owner=1;std::array<std::uint8_t,9> hidden{};
    storage.SaveFile(1,{readOnly,owner,hidden},ImageHooks(),files.Writer());auto expected=original;std::fill_n(expected.begin(),72,std::uint8_t{});expected[0]='F';
    for (std::size_t section=0;section<35;++section) expected[2+section*2]=2; // 원문35개 빈 섹션의 길이WORD는 각2이며 기존72BYTE 뒤 자료는 그대로 남긴다.
    const auto saved=ReadFileBytes(path);CHECK(saved==expected && saved.size()==original.size());CHECK(storage.CurrentSection(1)=="Terr19" && (storage.FileHeader(1)==std::array<std::uint32_t,2>{0x46,0}));
    const auto decoded=FortTemplate::Parse(saved,assets.game.TypeTable());CHECK(decoded.sections==storage.Template(1).sections && assets.files.Read("d/TEST01.fort")==original);
    readOnly=1;storage.SaveFile(1,{readOnly,owner,hidden},ImageHooks(),files.Writer());CHECK(ReadFileBytes(path)==saved); // 쓰기 억제 관문은 새 파일 장치 연결 뒤에도 기존 파일을 보존한다.
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

// ReadFort가 pending/next만 예약하면서 기존 전투 Fort의 실제 State/Notify/저장도 같은 장치에 적용한다.
TEST_CASE(EditorFiles_FortSelectionUsesCurrentStateAndWriter) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/TEST01.fort");
    EditorFiles files(WorkRoot("selection"),Protected());const auto path=files.Prepare("CurrentMap",original);CampaignTemplateStorage storage;
    storage.InstallFile(1,FortTemplate::Parse(original,assets.game.TypeTable()),original,path.string());
    std::uint32_t depth{},readOnly{},templateOwner=1,mode=2,next=7,battle=1,currentState=14,owner=7;bool mission{};
    std::array<std::uint8_t,9> hidden{};std::size_t saves{},loads{};std::string pending="old";
    RawPlayerTemplateState state(OriginalEdition::Patch1078,storage.Headers(),[&](auto slot)->PlayerTemplateStateStream& { return storage.SelectState(slot); },[&](auto slot)->const std::string& { return storage.Name(slot); },depth,mission,
        [](auto) { throw std::logic_error("Fort 선택의 예상 밖 State 진단"); },[](auto) { throw std::logic_error("PATCH Fort 선택의 CD 깊이 변경 금지"); });
    state.BindMinimumSize([&](auto slot,auto size)->PlayerTemplateStateStream& { return storage.EnsureStateSize(slot,size); });
    state.BindFileSave([&](auto slot) { ++saves;storage.SaveFile(slot,{readOnly,owner,hidden},ImageHooks(),files.Writer()); });
    hidden[templateOwner]=1; // 저장 관문은 Template 슬롯1이 아닌 실제 owner7의 hidden을 읽어야 한다.
    const auto initial=state.Get(1);const std::vector<std::uint8_t> before(initial.begin(),initial.end());
    FortSelectionHooks hooks;hooks.resolveString=[](auto token)->std::string_view { CHECK(token==202);return "NextMap"; };
    hooks.pending=[&](auto name) { SetPendingFortName(pending,name); };hooks.currentFortPath=[&] { return storage.Name(templateOwner); };
    hooks.loadFort=[&](auto slot,auto fileName) { // 실파일 읽기는 기존 현재 이름을 사용하며 선택한 pending 맵을 설치하지 않는다.
        CHECK(slot==1 && fileName==path.string());const auto bytes=ReadFileBytes(std::filesystem::path(fileName));storage.InstallFile(slot,FortTemplate::Parse(bytes,assets.game.TypeTable()),bytes,std::string(fileName));++loads;
    };
    hooks.core=[&](auto slot) { return state.Get(slot); };hooks.gameSeconds=[] { return 45.75; };
    hooks.ownedIslands=[](auto selected) { CHECK(selected==7);return 0; };hooks.notify=[&](auto slot) { state.Changed(slot); };
    CHECK(DispatchFortSelectionCommand({kReadFortCommand,101,202},{mode,next,battle,currentState,templateOwner,owner},hooks));
    CHECK(pending=="NextMap" && next==1 && mode==2 && loads==1 && saves==1 && storage.Name(1)==path.string());
    auto expected=before;expected[0]&=0xf9U;expected[12]=45;expected[13]=0;expected[14]=0;expected[15]=0;
    const auto after=state.Get(1);CHECK(std::vector<std::uint8_t>(after.begin(),after.end())==expected);
    const auto saved=ReadFileBytes(path);CHECK(saved!=original);const auto decoded=FortTemplate::Parse(saved,assets.game.TypeTable());CHECK(decoded.sections==storage.Template(1).sections);
    CampaignTemplateStorage reload;reload.InstallFile(1,decoded,saved,path.string());const auto reloaded=reload.State(1).bytes;CHECK(std::vector<std::uint8_t>(reloaded.begin(),reloaded.end())==expected);
    CHECK(assets.files.Read("d/TEST01.fort")==original); // 원본은 읽기 전용 입력으로만 사용했고 저장은 새 작업본에 수행했다.
}

// 끝 구분자와 ./..가 다른 작업본을 만들지 않으며 모든 표기에서 수정한 같은 파일을 다시 선택한다.
TEST_CASE(EditorFiles_WorkingRootNormalizesDirectorySpellingsAndPreservesWork) {
    const auto base=WorkRoot("root-normalization"),source=base/"assets";std::filesystem::create_directories(source/"d");
    const auto expected=std::filesystem::weakly_canonical(base)/"assets-editor";const std::vector<std::uint8_t> original{1,2,3},edited{4,5};
    std::vector<std::filesystem::path> spellings{source,source.string()+"/",source/".",source/"d"/"..",source/"d"/".."/""}; //같은 실제 디렉터리를 지정하는 일반/끝 구분자/상대 요소 표기다.
#ifdef _WIN32
    spellings.emplace_back(source.string()+"\\"); //Windows가 경로 구분자로 해석하는 역슬래시도 같은 디렉터리다.
#endif
    EditorFiles first(EditorWorkingRoot(source),{source});const auto saved=first.Prepare("Working",original);first.Write(saved.string(),edited);
    for (const auto& spelling:spellings) { //모든 재진입 표기가 같은 작업 namespace와 기존 수정본을 사용해야 한다.
        CHECK(EditorWorkingRoot(spelling)==expected);EditorFiles again(EditorWorkingRoot(spelling),{spelling});
        CHECK(again.Prepare("Working",original)==saved && ReadFileBytes(saved)==edited);
        CHECK(Rejects([&] { again.Write((source/"d/Working.fort").string(),original); }));
    }
    CHECK(!std::filesystem::exists(source/"-editor") && !std::filesystem::exists(source/"d/Working.fort"));
}

// 정규화로 원본 보호 검사를 우회하지 않으며 이름 없는 파일시스템 루트는 형제 작업본으로 바꾸지 않는다.
TEST_CASE(EditorFiles_WorkingRootStillRejectsProtectedAndNamelessRoots) {
    const auto base=WorkRoot("root-guards"),source=base/"assets";std::filesystem::create_directories(source);
    const auto working=EditorWorkingRoot(source/".");CHECK(Rejects([&] { EditorFiles forbidden(working,{base}); }));
    CHECK(Rejects([&] { EditorFiles forbidden(source/".."/"assets"/"-editor",{source}); }));
    CHECK(Rejects([&] { static_cast<void>(EditorWorkingRoot(std::filesystem::absolute(source).root_path())); }));
    CHECK(!std::filesystem::exists(working) && !std::filesystem::exists(source/"-editor"));
}
