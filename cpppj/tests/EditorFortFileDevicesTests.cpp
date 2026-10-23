// 실제작업파일과현재Config/Storage를검사하며원본/CD와공유build에는쓰지않는다.
#include "client/EditorFortFileDevices.h"
#include "TestSupport.h"
#include <array>
#include <chrono>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX //Win32매크로와표준함수의이름충돌을막는다.
#endif
#include <windows.h>
#endif
using namespace netstorm::client;
using namespace netstorm::o;
namespace {
// 실행마다새격리경로를만들며기존사용자작업본이나실패자료를지우지않는다.
std::filesystem::path FreshRoot(std::string_view label) { return std::filesystem::path(NETSTORM_EDITOR_FILE_DEVICE_TEST_ROOT)/(std::string(label)+'-'+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())); }
// 이번검사가직접만드는원문입력복사와실제보호루트모두쓰기장치에서차단한다.
std::vector<std::filesystem::path> Protected(const std::filesystem::path& source) { const std::filesystem::path root(NETSTORM_REPOSITORY_ROOT);return {root/"originals",root/"originalCD",source}; }
// 이검사소유경로의합성원문만생성한다.파일작성실패는성공으로숨기지않는다.
void SourceFile(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) {
    std::filesystem::create_directories(path.parent_path());std::ofstream file(path,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!file) throw std::runtime_error("장치검사합성원문쓰기실패");
}
// 실제reader가소비할35개길이레코드와28BYTE State만만들며미정성공기대를만들지않는다.
std::vector<std::uint8_t> FortImage(std::uint8_t marker=7) {
    std::vector<std::uint8_t> result{'F',0};
    for (std::size_t index=0;index<35;++index) { // 실제고정이름표의State만명시입력payload를갖는다.
        const std::uint16_t length=index==1 ? 30 : 2;result.push_back(static_cast<std::uint8_t>(length));result.push_back(static_cast<std::uint8_t>(length>>8));if (index==1) { std::array<std::uint8_t,28> state{};state[12]=marker;state[17]=2;result.insert(result.end(),state.begin(),state.end()); }
    }return result;
}
// 의도한경로/진단/물리거부를CHECK실패와구분한다.
template<class Function> bool Rejects(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; }
// 현재NUL제외이름과같은키를명시효과로공급한다.제품의curFort owner를만들었다고주장하지않는다.
EditorFortPathEffects PathEffects(std::string& name,std::string& key,std::size_t& selects,std::vector<std::uint32_t>& assertions) {
    return {[&name]()->std::string_view { return name; },[&selects] { ++selects; },[&key]()->std::string_view { return key; },[&assertions](auto line) { assertions.push_back(line); }};
}
// 파일장치외부의현재owner/TYPE/제목/원문진단만검사입력으로명시한다.
TemplateFileLoadHooks LoadEffects() {
    TemplateFileLoadHooks hooks;hooks.typeCount=[] { return 188; };hooks.currentSlot=[] { return 1U; };hooks.refreshTitle=[] {};
    hooks.wrongName=[] { throw std::logic_error("예상밖기존이름진단"); };hooks.tooManyTypes=[] { throw std::logic_error("예상밖TYPE진단"); };hooks.rejectFile=[](auto) { throw std::logic_error("예상밖서명제거요청"); };return hooks;
}
}

// CurrentPath는설정/현재이름을매번조회하지만파일/manager/dirty를만들지않는다.
TEST_CASE(EditorFortFiles_CurrentPathReusesConfigAndCurrentFileSpec) {
    const auto root=FreshRoot("path");EditorFiles work(root/"work",Protected(root/"source"));BaseFileSystem source(root/"source");ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");config.Set("SOLOSpec","maps/{local.1}.fort");
    std::string name="One",key="fortSpec";std::size_t selects{};std::vector<std::uint32_t> assertions;EditorFortFileDevices devices(work,source,config,PathEffects(name,key,selects,assertions));
    const auto first=std::string(devices.CurrentPath());CHECK(first==work.ResolvePath(NormalizeGamePath("d/One.fort")).string() && !devices.Exists(first) && !std::filesystem::exists(root/"work"));
    name="Two";key="SOLOSpec";const auto second=std::string(devices.CurrentPath());CHECK(second==work.ResolvePath(NormalizeGamePath("maps/Two.fort")).string() && first!=second && selects==2 && assertions.empty());
    CHECK(devices.Read(second).empty()); //원본/현재프로세스의같은이름으로fallback하지않는다.
}

// 준비는없는작업본만원문에서복사하고그뒤에는수정본/시간/권한을그대로사용한다.
TEST_CASE(EditorFortFiles_PreparedEditedBytesWinOverOriginal) {
    const auto root=FreshRoot("prepared");const auto original=FortImage(7),edited=FortImage(29);SourceFile(root/"source/d/Map.fort",original);EditorFiles work(root/"work",Protected(root/"source"));BaseFileSystem source(root/"source");ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");
    std::string name="Map",key="fortSpec";std::size_t selects{};std::vector<std::uint32_t> assertions;EditorFortFileDevices devices(work,source,config,PathEffects(name,key,selects,assertions));
    const auto path=std::string(devices.PrepareCurrent());CHECK(devices.Exists(path)==5 && devices.Read(path)==original);work.Write(path,edited);const auto time=std::filesystem::last_write_time(path);
    CHECK(devices.PrepareCurrent()==path && devices.Read(path)==edited && std::filesystem::last_write_time(path)==time && source.Read("d/Map.fort")==original);
    CampaignTemplateStorage storage;auto load=devices.BindLoad(LoadEffects());storage.LoadFile(1,FortTemplate{},path,load);CHECK(ExistingEditorTemplateValid(storage,1)==1 && storage.Name(1)==path && storage.State(1).bytes[12]==29);
    TemplateFileLoadHooks existing;existing.sameName=load.sameName;storage.LoadFile(1,FortTemplate{},path,existing);CHECK(storage.OwnedImages()==1 && storage.State(1).bytes[12]==29);
}

// 실제NTFS readonly속성에대한CRT stat모드판정이며Config값은이판정에영향을주지않는다.
TEST_CASE(EditorFortFiles_ActualReadOnlyAttributeIsSeparateFromConfigGate) {
    const auto root=FreshRoot("readonly");EditorFiles work(root/"work",Protected(root/"source"));const auto path=work.Prepare("Map",FortImage());BaseFileSystem source(root/"source");ConfigInterface config({});EditorFortFileDevices devices(work,source,config,{});
#ifdef _WIN32
    const auto old=GetFileAttributesW(path.c_str());CHECK(old!=INVALID_FILE_ATTRIBUTES && SetFileAttributesW(path.c_str(),old|FILE_ATTRIBUTE_READONLY));
    config.SetInt("readOnlyForts",0);CHECK(devices.ReadOnly(path.string()));config.SetInt("readOnlyForts",1);CHECK(devices.ReadOnly(path.string()) && devices.Exists(path.string())==5 && !devices.Read(path.string()).empty());
    CampaignTemplateStorage locked;locked.LoadFile(1,FortTemplate{},path.string(),devices.BindLoad(LoadEffects()));CHECK(locked.Headers()[20+8]==1);
    CHECK(SetFileAttributesW(path.c_str(),old&~FILE_ATTRIBUTE_READONLY));CHECK(!devices.ReadOnly(path.string())); //명시검사소유파일의원래속성만복구한다.
    CampaignTemplateStorage writable;writable.LoadFile(1,FortTemplate{},path.string(),devices.BindLoad(LoadEffects()));CHECK(writable.Headers()[20+8]==0 && config.GetInt("readOnlyForts")==1);
#else
    throw std::logic_error("이검사는실제Windows속성/CRT stat이필요합니다");
#endif
}

// Config저장억제DWORD는이미지생성뒤SaveFile이읽고파일readonly판정과혼합하지않는다.
TEST_CASE(EditorFortFiles_SaveGateKeepsWritableFileUntilCurrentDwordAllowsWrite) {
    const auto root=FreshRoot("gate");EditorFiles work(root/"work",Protected(root/"source"));const auto original=FortImage(7);const auto path=work.Prepare("Map",original);BaseFileSystem source(root/"source");ConfigInterface config({});EditorFortFileDevices devices(work,source,config,{});
    CampaignTemplateStorage storage;storage.LoadFile(1,FortTemplate{},path.string(),devices.BindLoad(LoadEffects()));storage.State(1).bytes[12]=51;
    std::uint32_t readOnlyForts=1,owner=1;std::array<std::uint8_t,9> hidden{};const FortImageHooks image{[] { return 188U; },[](auto) { throw std::logic_error("예상밖저장이미지진단"); }};
    storage.SaveFile(1,{readOnlyForts,owner,hidden},image,work.Writer());CHECK(!devices.ReadOnly(path.string()) && devices.Read(path.string())==original && storage.CurrentSection(1)=="Terr19");
    readOnlyForts=0;storage.SaveFile(1,{readOnlyForts,owner,hidden},image,work.Writer());CHECK(devices.Read(path.string())==FortImage(51)); //실제Client Config callback수명은기존별도검사범위이며여기는그DWORD참조를명시공급한다.
}

// valid는서명/토큰map조회가아닌원문첫DWORD비영검사만수행한다.
TEST_CASE(EditorFortFiles_ValidReadsOnlyCurrentHeaderPointer) {
    CampaignTemplateStorage storage;CHECK(!ExistingEditorTemplateValid(storage,1));storage.Headers()[20+2]=0x80;CHECK(ExistingEditorTemplateValid(storage,1)==1);storage.Headers()[20+2]=0;
    CHECK(!ExistingEditorTemplateValid(storage,1) && Rejects([&] { ExistingEditorTemplateValid(storage,9); }));
}

// 작업파일이없으면원본이있어도Read는fallback하지않고준비가명시요청된때만복사한다.
TEST_CASE(EditorFortFiles_MissingReadAndStatFailureRemainExplicit) {
    const auto root=FreshRoot("missing");SourceFile(root/"source/d/Map.fort",FortImage());EditorFiles work(root/"work",Protected(root/"source"));BaseFileSystem source(root/"source");ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");
    std::string name="Map",key="fortSpec";std::size_t selects{};std::vector<std::uint32_t> assertions;EditorFortFileDevices devices(work,source,config,PathEffects(name,key,selects,assertions));const auto path=std::string(devices.CurrentPath());
    CHECK(!devices.Exists(path) && devices.Read(path).empty() && Rejects([&] { devices.ReadOnly(path); }) && assertions==std::vector<std::uint32_t>{0x48e});
    CHECK(devices.PrepareCurrent()==path && devices.Exists(path)==5 && devices.Read(path)==FortImage());name="Absent";const auto absent=std::string(devices.PrepareCurrent());CHECK(!devices.Exists(absent) && devices.Read(absent).empty());
}

// 첫NUL진단이현재이름을고친경우와고정키선택효과뒤변경을후속PathSpec에반영한다.
TEST_CASE(EditorFortFiles_CurrentNameDiagnosticAndSelectorAreReachedLazily) {
    const auto root=FreshRoot("effects");EditorFiles work(root/"work",Protected(root/"source"));BaseFileSystem source(root/"source");ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");config.Set("SOLOSpec","solo/{local.1}.fort");std::string name,key="fortSpec";std::vector<std::uint32_t> assertions;
    EditorFortPathEffects effects;effects.currentName=[&]()->std::string_view { return name; };effects.assertion=[&](auto line) { assertions.push_back(line);name="Repaired"; };effects.selectFileSpec=[&] { key="SOLOSpec"; };effects.fileSpecKey=[&]()->std::string_view { return key; };
    EditorFortFileDevices devices(work,source,config,std::move(effects));CHECK(devices.CurrentPath()==work.ResolvePath(NormalizeGamePath("solo/Repaired.fort")).string() && assertions==std::vector<std::uint32_t>{39});
    EditorFortFileDevices missing(work,source,config,{});CHECK(missing.Read(work.ResolvePath("unprepared.fort").string()).empty() && Rejects([&] { missing.CurrentPath(); })); //미선택현재이름효과를read바인딩전에요구하지않는다.
}

// Config경로와실제장치접근모두동일작업root/보호경로검사를통과해야한다.
TEST_CASE(EditorFortFiles_RejectsProtectedAndEscapedPathsBeforeAnyWrite) {
    const auto root=FreshRoot("guard");const auto bytes=FortImage();SourceFile(root/"source/d/Map.fort",bytes);EditorFiles work(root/"work",Protected(root/"source"));BaseFileSystem source(root/"source");ConfigInterface config({});std::string name="Map",key="fortSpec";std::size_t selects{};std::vector<std::uint32_t> assertions;EditorFortFileDevices devices(work,source,config,PathEffects(name,key,selects,assertions));
    config.Set("fortSpec","../escape/{local.1}.fort");CHECK(Rejects([&] { devices.PrepareCurrent(); }));config.Set("fortSpec",(root/"source/d/Map.fort").string());CHECK(Rejects([&] { devices.PrepareCurrent(); }));
    CHECK(Rejects([&] { work.PreparePath("../escape.fort",bytes); }) && Rejects([&] { devices.Read((root/"source/d/Map.fort").string()); }));CHECK(source.Read("d/Map.fort")==bytes && !std::filesystem::exists(root/"work"));
}

// 기존VFS가게임root기준으로읽는선행역슬래시/대소문자는같은NormalizeGamePath로해석한다.
TEST_CASE(EditorFortFiles_RootedGamePathReusesBaseFileNormalization) {
    const auto root=FreshRoot("rooted");const auto bytes=FortImage(19);SourceFile(root/"source/D/Mixed.fort",bytes);EditorFiles work(root/"work",Protected(root/"source"));BaseFileSystem source(root/"source");ConfigInterface config({});config.Set("fortSpec","\\D\\{local.1}.fort");
    std::string name="Mixed",key="fortSpec";std::size_t selects{};std::vector<std::uint32_t> assertions;EditorFortFileDevices devices(work,source,config,PathEffects(name,key,selects,assertions));
    const auto path=std::string(devices.PrepareCurrent());CHECK(path==work.ResolvePath("d/mixed.fort").string() && devices.Exists(path)==5 && devices.Read(path)==bytes && source.Read("\\D\\Mixed.fort")==bytes);
}
