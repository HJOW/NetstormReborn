// 실제 임시 작업본/원본 디스크로 목록 위치, 현재 읽기, 새 이름 재조회와 경로 경계를 확인한다.
#include "client/EditorMapCatalogFiles.h"
#include "client/EditorFiles.h"
#include "client/SoloMapCatalog.h"
#include "client/FortCurrentNameDevices.h"
#include "client/EditorFortFileDevices.h"
#include "o/BaseFile.h"
#include "o/ConfigInterface.h"
#include "o/OriginalText.h"
#include "TestSupport.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX //Win32 min/max 매크로가 표준 조회를 바꾸지 않게 한다.
#endif
#include <windows.h>
#include <winioctl.h>
#endif

// 같은 검사 전용 이름과 보조 장치를 다른 테스트 번역 단위에 노출하지 않는다.
namespace {
using namespace netstorm::client; //실제 작업본과 목록 부모만 짧은 이름으로 사용한다.
using namespace netstorm::o; //기존 파일/Config와 ASCII 원문 규칙을 사용한다.
// 기존 사용자 작업본/원본을 변경하지 않는 이번 검사만의 새 임시 경로를 만든다.
std::filesystem::path NewRoot(std::string_view label) {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();return std::filesystem::temp_directory_path()/("netstorm-editor-catalog-"+std::string(label)+'-'+std::to_string(stamp));
}
// 목록이 읽는 Subscriber/State만 독립 길이 레코드로 구성하고 월드 파서를 사용하지 않는다.
std::vector<std::uint8_t> Image(std::uint8_t kind,std::uint8_t first=0) {
    std::vector<std::uint8_t> bytes{'F',0}; //정상 서명과 현재 타입 수188의 저장형0이다.
    for (std::size_t index=0;index<35;++index) { //원본 고정35섹션 중 목록이 실제로 읽는 두 섹션만 채운다.
        std::vector<std::uint8_t> section; //현재 한 섹션의 원시 BYTE이며 길이에는 머리2BYTE를 포함한다.
        if (!index) section={1,0,0,0,2,0,'M','e'};
        else if (index==1) { section.resize(28);section[0]=first;section[17]=kind; }
        else if (index==4 || index==7) section={0xff};
        const auto length=section.size()+2;bytes.push_back(static_cast<std::uint8_t>(length));bytes.push_back(static_cast<std::uint8_t>(length>>8));bytes.insert(bytes.end(),section.begin(),section.end());
    }
    return bytes;
}
// 생성한 검사 전용 source만 준비하며 실제 originals/originalCD를 사용하지 않는다.
void Seed(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) {
    std::filesystem::create_directories(path.parent_path());std::ofstream file(path,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!file) throw std::runtime_error("편집 목록 검사 전용 파일 쓰기 실패");
}
// 성공한 빈 결과와 경로 거부를 혼동하지 않고 실제 예외 발생을 확인한다.
template<class Function> bool Rejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
// 권한 상승이 필요 없는 Windows directory junction으로 실제 canonical 링크 경계를 검사한다.
void Junction(const std::filesystem::path& link,const std::filesystem::path& target) {
#ifdef _WIN32
    //MountPoint reparse 형식은 두 UTF-16 경로와 각 NUL을 고정 머리 뒤에 배치한다.
    struct JunctionData {
        std::uint32_t tag{IO_REPARSE_TAG_MOUNT_POINT}; //NTFS directory junction의 실제 태그다.
        std::uint16_t dataLength{}; //공통8BYTE를 제외한 실제 전송 길이다.
        std::uint16_t reserved{}; //원본 Win32 reparse 공통 예약값0이다.
        std::uint16_t substituteOffset{}; //PathBuffer 시작의 NT 경로다.
        std::uint16_t substituteLength{}; //NT 경로의 NUL 제외 BYTE 길이다.
        std::uint16_t printOffset{}; //NT 경로/NUL 다음의 일반 표시 경로 위치다.
        std::uint16_t printLength{}; //표시 경로의 NUL 제외 BYTE 길이다.
        std::array<wchar_t,4096> paths{}; //최대 검사 경로 두 개와 각 NUL의 소유 버퍼다.
    } data;
    static_assert(offsetof(JunctionData,paths)==16); //FSCTL_SET_REPARSE_POINT의 실제 MountPoint 머리 크기다.
    const auto actual=std::filesystem::absolute(target).wstring(),substitute=std::wstring(L"\\??\\")+actual; //기존 디렉터리를 지정하는 Win32/NT 두 이름이다.
    if (actual.size()+substitute.size()+2>data.paths.size()) throw std::length_error("검사 junction 경로 버퍼 범위");
    data.substituteLength=static_cast<std::uint16_t>(substitute.size()*sizeof(wchar_t));data.printOffset=static_cast<std::uint16_t>(data.substituteLength+sizeof(wchar_t));data.printLength=static_cast<std::uint16_t>(actual.size()*sizeof(wchar_t));
    std::copy(substitute.begin(),substitute.end(),data.paths.begin());std::copy(actual.begin(),actual.end(),data.paths.begin()+substitute.size()+1);
    data.dataLength=static_cast<std::uint16_t>(8+(actual.size()+substitute.size()+2)*sizeof(wchar_t));std::filesystem::create_directory(link);
    const auto handle=CreateFileW(link.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,nullptr); //일반 임시 디렉터리 자체에 junction 데이터를 설치한다.
    if (handle==INVALID_HANDLE_VALUE) throw std::runtime_error("검사 junction 디렉터리 열기 실패");
    DWORD returned{};const auto written=DeviceIoControl(handle,FSCTL_SET_REPARSE_POINT,&data,static_cast<DWORD>(8+data.dataLength),nullptr,0,&returned,nullptr);CloseHandle(handle);
    if (!written) throw std::runtime_error("검사 junction 생성 실패");
#else
    std::filesystem::create_directory_symlink(target,link); //비Windows 검사 호스트에서는 같은 directory 링크를 사용한다.
#endif
}
}

namespace { //수정 구현의 기대는 합성 source 순서가 아닌 실제 work OS 원문에서 만든다.
// 검사에서 빌린 파일 장치는 workspace보다 오래 살아야 하며 archive-only 장치를 명시한다.
std::shared_ptr<EditorCatalogWorkspace> Workspace(EditorFiles& work,const std::filesystem::path& root,const BaseFileSystem& source) {
    return std::make_shared<EditorCatalogWorkspace>(std::shared_ptr<EditorFiles>(&work,[](auto*) {}),EditorCatalogSourceDevices{root,[&source](auto path) { return source.TryReadArchiveOnly(path); }});
}
// 두 실제 장치만 workspace에 연결하고 원본 분류/메타/Raw50 몸체는 그대로 사용한다.
SoloMapCatalogEffects Effects(std::shared_ptr<EditorCatalogWorkspace> workspace) {
    SoloMapCatalogEffects effects;effects.typeCount=[] { return 188; };effects.allocationBytes=[](auto) { throw std::logic_error("정상 목록의 예상 밖 빈 State"); };effects.userName=[] { return std::string("Me"); };effects.translate=[](auto text) { return std::string(text); };
    effects.enumeratePaths=[workspace](auto path) { return workspace->Enumerate(path); };effects.readFile=[workspace=std::move(workspace)](auto path) { return workspace->Read(path); };return effects;
}
// 구분자 표시만 같게 하고 . / .. 후보나 물리 파일명은 정규화/제거하지 않는다.
std::vector<std::string> Slash(const std::vector<std::string>& paths) { std::vector<std::string> result;for (const auto& path:paths) result.push_back(std::filesystem::path(path).generic_string());return result; } //같은 OS 후보의 /와 역슬래시 차이만 제거한다.
// 첫점 제거는 원문 FileSpec 이름 변환이며 중복 제거/순서 계산은 하지 않는다.
std::string Name(const std::string& path) { auto name=std::filesystem::path(path).filename().string();const auto dot=name.find('.');if (dot!=std::string::npos) name.resize(dot);return name; }
// 검사 전용 장부 상태를 원시 binary로 명시해 복사 실패/Pending과 손상 경계를 독립 입력한다.
std::vector<std::uint8_t> Ledger(const EditorFiles& work,const std::filesystem::path& source,std::string_view key,std::uint32_t state) {
    std::vector<std::uint8_t> bytes{'N','S','E','F','O','R','T',1};const auto word=[&](std::uint32_t value) { for (unsigned shift=0;shift<32;shift+=8) bytes.push_back(static_cast<std::uint8_t>(value>>shift)); }; //명시 DWORD의 원시 little endian 입력이다.
    const auto text=[&](std::string_view value) { word(static_cast<std::uint32_t>(value.size()));bytes.insert(bytes.end(),value.begin(),value.end()); }; //제품 parser를 호출하지 않고 길이/원문을 기록한다.
    const auto identity=[](const auto& path) { const auto value=std::filesystem::weakly_canonical(std::filesystem::absolute(path)).generic_u8string();return AsciiLower({reinterpret_cast<const char*>(value.data()),value.size()}); }; //명시 검사 경로의 UTF-8 identity만 만든다.
    text(identity(source));text(identity(work.RootPath()));word(1);text(key);word(state);return bytes;
}
// 작은 단일 항목 archive는 등록순서/disk 분리의 독립 입력이며 기존 fixture를 수정하지 않는다.
std::vector<std::uint8_t> Archive(char value) {
    std::vector<std::uint8_t> bytes(0x45);const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),bytes.begin());
    const auto word=[&](std::size_t at,std::uint32_t number) { for (unsigned index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(number>>(8*index)); }; //고정 머리/인덱스/항목 DWORD만 채운다.
    word(0x14,1);word(0x1c,0x2c);word(0x20,0x30);word(0x24,0x14);word(0x28,0x44);word(0x30,0);word(0x34,1);const std::string name="\\d\\archive";std::copy(name.begin(),name.end(),bytes.begin()+0x38);bytes[0x44]=static_cast<std::uint8_t>(value);return bytes;
}
}

// 기존 사용자 내용/readonly를 보존하고 실제 work OS 결과를 그대로 원문 목록에 전달한다.
TEST_CASE(EditorCatalogFiles_WorkOsOrderPreservesExistingCopyAndReadonly) {
    const auto root=NewRoot("single-directory"),sourceRoot=root/"source";const auto original=Image(2),edited=Image(1,1);Seed(sourceRoot/"d/First.fort",original);Seed(sourceRoot/"d/Second.fort",original);BaseFileSystem source(sourceRoot);EditorFiles work(root/"work",{sourceRoot});
    const auto first=work.Prepare("First",edited);work.Prepare("NewSaved",original);
#ifdef _WIN32
    CHECK(SetFileAttributesW((sourceRoot/"d/Second.fort").c_str(),FILE_ATTRIBUTE_READONLY)!=0);CHECK(SetFileAttributesW(first.c_str(),FILE_ATTRIBUTE_READONLY)!=0); //새 source 속성과 기존 사용자 속성을 독립 입력한다.
#endif
    auto workspace=Workspace(work,sourceRoot,source);const auto listed=workspace->Enumerate("d/*.fort"),actual=work.MatchDisk("d/*.fort");CHECK(Slash(listed)==Slash(actual) && actual.size()==3);
    CHECK(workspace->Read("d/First.fort")==edited && ReadFileBytes(sourceRoot/"d/First.fort")==original && work.ReadOnly(first.string()));CHECK(work.ReadOnly(work.ResolvePath("d/Second.fort").string()));
    ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");config.SetInt("mayEditMissions",1);SoloMapCatalog catalog(source,config,Effects(workspace));CHECK(catalog.Refresh(2)==3);
    for (std::size_t index=0;index<actual.size();++index) { CHECK(catalog.Cached().names[index]==Name(actual[index]));CHECK(catalog.Cached().flags[index]==(AsciiLower(Name(actual[index]))=="first" ? 2U : 0U)); } //원시 work 이름의 종류/메타를 같은 실제 위치에 확인한다.
    auto defaults=Effects(workspace);defaults.enumeratePaths={};defaults.readFile={};SoloMapCatalog unchanged(source,config,std::move(defaults));CHECK(unchanged.Kind("First")==2 && unchanged.Refresh(2)==2);
}

// 새 저장은 dirty 뒤 실제 work 위치에서 조회되고 새 owner도 기존 사용자 내용을 읽는다.
TEST_CASE(EditorCatalogFiles_NewNameRefreshAndOwnerRestartUseActualWork) {
    const auto root=NewRoot("restart"),sourceRoot=root/"source";const auto original=Image(2),changed=Image(1,1);Seed(sourceRoot/"d/Original.fort",original);BaseFileSystem source(sourceRoot);EditorFiles work(root/"work",{sourceRoot});
    { auto workspace=Workspace(work,sourceRoot,source);ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");SoloMapCatalog catalog(source,config,Effects(workspace));CHECK(catalog.Refresh(2)==1);const auto saved=work.Prepare("SavedAgain",original);work.Write(saved.string(),original);CHECK(catalog.Refresh(2)==1);catalog.Invalidate();CHECK(catalog.Refresh(2)==2);const auto actual=work.MatchDisk("d/*.fort");for (std::size_t index=0;index<actual.size();++index) CHECK(catalog.Cached().names[index]==Name(actual[index])); } //같은 실제 owner/잠금이 모두 반납된 뒤 재시작한다.
    auto reopened=Workspace(work,sourceRoot,source);CHECK(reopened->Read("d/SavedAgain.fort")==original);work.Write(work.ResolvePath("d/SavedAgain.fort").string(),changed);CHECK(reopened->Read("d/SavedAgain.fort")==changed && !std::filesystem::exists(sourceRoot/"d/SavedAgain.fort"));
}

// 원문은 첫점 이름이 같아도 두 physical 파일을 두 행으로 수락한다.
TEST_CASE(EditorCatalogFiles_DuplicateLogicalNamesRemainTwoNativeRows) {
    const auto root=NewRoot("duplicates"),sourceRoot=root/"source";const auto bytes=Image(2);Seed(sourceRoot/"d/Same.fort",bytes);Seed(sourceRoot/"d/Same.extra.fort",bytes);Seed(sourceRoot/"d/Other.fort",bytes);BaseFileSystem source(sourceRoot);EditorFiles work(root/"work",{sourceRoot});auto workspace=Workspace(work,sourceRoot,source);
    const auto listed=workspace->Enumerate("d/*.fort"),actual=work.MatchDisk("d/*.fort");CHECK(listed.size()==3 && Slash(listed)==Slash(actual));ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");SoloMapCatalog catalog(source,config,Effects(workspace));CHECK(catalog.Refresh(2)==3);
    std::size_t duplicates{};for (std::size_t index=0;index<actual.size();++index) { CHECK(catalog.Cached().names[index]==Name(actual[index]));duplicates+=AsciiLower(Name(actual[index]))=="same"; } //OS 두 항목을 어느 위치에서도 합치지 않는다.
    CHECK(duplicates==2 && work.Exists(work.ResolvePath("d/Same.extra.fort").string())!=0);
}

// 새 이름의 포함 여부는 합성 append가 아니라 실제 work 수락첫50에서 독립 판정한다.
TEST_CASE(EditorCatalogFiles_NativeFiftyMatchesActualWorkOsSnapshot) {
    const auto root=NewRoot("native-fifty"),sourceRoot=root/"source";const auto bytes=Image(2);for (std::size_t index=0;index<53;++index) Seed(sourceRoot/"d"/("Original"+std::to_string(index)+".fort"),bytes); //수락 종류가 모두2인 명시53개다.
    BaseFileSystem source(sourceRoot);EditorFiles work(root/"work",{sourceRoot});auto workspace=Workspace(work,sourceRoot,source);workspace->ImportDisk("d/*.fort");work.Prepare("NewOnly",bytes);const auto actual=work.MatchDisk("d/*.fort");CHECK(actual.size()==54 && Slash(workspace->Enumerate("d/*.fort"))==Slash(actual));
    ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");SoloMapCatalog catalog(source,config,Effects(workspace));CHECK(catalog.Refresh(2)==50);bool expectedNew{},listedNew{};
    for (std::size_t index=0;index<50;++index) { CHECK(catalog.Cached().names[index]==Name(actual[index]));expectedNew|=AsciiLower(Name(actual[index]))=="newonly";listedNew|=catalog.Cached().names[index]=="NewOnly"; } //원본과 같은50개 수락이며 new 항상보임/항상안보임을 가정하지 않는다.
    CHECK(expectedNew==listedNew);
}

// 삭제 완료키는 모든 표기/직접 prepare/메타에서 disk/TAFF로 부활하지 않는다.
TEST_CASE(EditorCatalogFiles_CompletedDeletionAliasesAndDirectPrepareNeverRevive) {
    const auto root=NewRoot("tombstone"),sourceRoot=root/"source";const auto bytes=Image(2);Seed(sourceRoot/"d/Removed.fort",bytes);BaseFileSystem source(sourceRoot);EditorFiles work(root/"work",{sourceRoot});std::size_t archives{};
    const auto create=[&] { return std::make_shared<EditorCatalogWorkspace>(std::shared_ptr<EditorFiles>(&work,[](auto*) {}),EditorCatalogSourceDevices{sourceRoot,[&](auto)->std::optional<std::vector<std::uint8_t>> { ++archives;return bytes; }}); }; //archive에 같은 이름이 있어도 완료키에서 호출하지 않아야 한다.
    std::filesystem::path target;{ auto workspace=create();target=workspace->Prepare((sourceRoot/"d/Removed.fort").string());CHECK(AsciiLower(target.generic_string())==AsciiLower(workspace->Resolve("d/./REMOVED.fort").generic_string()) && AsciiLower(target.generic_string())==AsciiLower(workspace->Resolve(target.string()).generic_string()) && workspace->Read("d/Removed.fort")==bytes);work.Delete(target.string());CHECK(!workspace->Read((sourceRoot/"d/Removed.fort").string()) && !archives); }
    auto reopened=create();CHECK(!reopened->Read(target.string()) && !reopened->Read("d/../d/removed.fort") && reopened->Prepare("d/Removed.fort")==target && !std::filesystem::exists(target) && !archives);CHECK(reopened->Enumerate("d/*.fort").empty());
    ConfigInterface config({});config.Set("fortSpec",(sourceRoot/"d/{local.1}.fort").string());EditorFortPathEffects path;path.currentName=[] { return std::string_view("Removed"); };path.selectFileSpec=[] {};path.fileSpecKey=[] { return std::string_view("fortSpec"); };path.resolve=[&](auto value) { return reopened->Resolve(value); };path.prepare=[&](auto value) { static_cast<void>(reopened->Prepare(value)); };EditorFortFileDevices direct(work,source,config,std::move(path));CHECK(direct.PrepareCurrent()==target.string() && !direct.Exists(target.string()) && !archives);
    work.Write(target.string(),bytes);CHECK(reopened->Read("d/Removed.fort")==bytes && ReadFileBytes(sourceRoot/"d/Removed.fort")==bytes); //사용자가 명시 저장한 새 내용은 자동 원본 재복사와 구별한다.
}

// 디렉터리/명시 부재/. / .. 를 OS 그대로 유지하고 형제 metadata는 어떤 work 패턴에도 없다.
TEST_CASE(EditorCatalogFiles_DirectoriesMissingPatternsDotsAndSiblingMetadata) {
    const auto root=NewRoot("physical-entries"),sourceRoot=root/"source";std::filesystem::create_directories(sourceRoot/"d/Folder.fort/nested");Seed(sourceRoot/"d/Folder.fort/nested/unrelated",std::array<std::uint8_t,1>{1});BaseFileSystem source(sourceRoot);EditorFiles work(root/"work",{sourceRoot});auto workspace=Workspace(work,sourceRoot,source);
    const auto listed=workspace->Enumerate("d/*.fort");CHECK(listed.size()==1 && std::filesystem::is_directory(work.ResolvePath("d/Folder.fort")) && std::filesystem::is_empty(work.ResolvePath("d/Folder.fort")) && !workspace->Read("d/Folder.fort"));
    const auto missing=workspace->Enumerate("d/Missing.fort");CHECK(missing.size()==1 && Name(missing.front())=="Missing" && !workspace->Read("d/Missing.fort"));Seed(sourceRoot/"d/Missing.fort",Image(2));CHECK(workspace->Read("d/Missing.fort")==Image(2)); //첫 명시부재를 완료로 잘못 기록하면 나중 실제 입력이 이관되지 못한다.
    const auto all=workspace->Enumerate("*");const BaseFileSystem actual(work.RootPath());CHECK(Slash(all)==Slash(actual.MatchDisk((work.RootPath()/"*").string())));bool dot{},parent{};for (const auto& value:all) { const auto leaf=std::filesystem::path(value).filename().string();dot|=leaf==".";parent|=leaf=="..";CHECK(leaf!="catalog-v1.dat" && leaf!="catalog.lock" && leaf!="work-state"); } //실제 두 OS 점 후보를 합성/제거하지 않는다.
    CHECK(dot && parent && std::filesystem::is_regular_file(work.SiblingNamespace("-state").ResolvePath("catalog-v1.dat")));
    ConfigInterface config({});config.Set("fortSpec","{local.1}");SoloMapCatalog catalog(source,config,Effects(workspace));CHECK(Rejects([&] { catalog.Refresh(0); })); //원문00460c40은 파일 읽기 실패 뒤에도004bd550을 호출한다. manager0 진단/물리 실패를 빈 State나 성공한 빈 이름행으로 바꾸지 않는다.
    std::filesystem::create_directories(root/"outside");Junction(work.RootPath()/"escape",root/"outside");CHECK(Rejects([&] { workspace->Enumerate("escape/*.fort"); }));CHECK(Rejects([&] { workspace->Read("escape/any.fort"); }));CHECK(Rejects([&] { workspace->Resolve((root/"work-extra/any.fort").string()); }));
}

// durable Pending/손상/동시 owner/장부 임시파일 open 실패를 새 장부 성공이나 자동 재복사로 바꾸지 않는다.
TEST_CASE(EditorCatalogFiles_PendingCorruptLockedAndFailedJournalAreExplicit) {
    const auto root=NewRoot("journal"),sourceRoot=root/"source";const auto bytes=Image(2);Seed(sourceRoot/"d/Pending.fort",bytes);BaseFileSystem source(sourceRoot);
    EditorFiles missing(root/"missing-work",{sourceRoot});auto state=missing.SiblingNamespace("-state");state.WriteAtomic("catalog-v1.dat",Ledger(missing,sourceRoot,"d/pending.fort",1));{ auto workspace=Workspace(missing,sourceRoot,source);CHECK(Rejects([&] { workspace->Prepare("d/Pending.fort"); }) && !std::filesystem::exists(missing.ResolvePath("d/Pending.fort"))); }
    EditorFiles existing(root/"existing-work",{sourceRoot});const auto target=existing.Prepare("Pending",bytes);auto existingState=existing.SiblingNamespace("-state");existingState.WriteAtomic("catalog-v1.dat",Ledger(existing,sourceRoot,"d/pending.fort",1));{ auto workspace=Workspace(existing,sourceRoot,source);CHECK(workspace->Read("d/Pending.fort")==bytes);auto other=Workspace(existing,sourceRoot,source);CHECK(Rejects([&] { other->Read("d/Pending.fort"); })); }existing.Delete(target.string());{ auto workspace=Workspace(existing,sourceRoot,source);CHECK(!workspace->Read("d/Pending.fort")); }
    EditorFiles corrupt(root/"corrupt-work",{sourceRoot});auto corruptState=corrupt.SiblingNamespace("-state");const std::vector<std::uint8_t> broken{1,2,3};corruptState.WriteAtomic("catalog-v1.dat",broken);{ auto workspace=Workspace(corrupt,sourceRoot,source);CHECK(Rejects([&] { workspace->Read("d/Pending.fort"); }) && corruptState.TryRead("catalog-v1.dat")==broken); }
    EditorFiles failed(root/"failed-work",{sourceRoot});auto failedState=failed.SiblingNamespace("-state");failedState.EnsureDirectory("catalog-v1.dat.pending");auto workspace=Workspace(failed,sourceRoot,source);CHECK(Rejects([&] { workspace->Prepare("d/Pending.fort"); }) && Rejects([&] { workspace->Read("d/Pending.fort"); }) && !std::filesystem::exists(failed.ResolvePath("d/Pending.fort")));
}

// 실제 Native 이름 부모의 readonly 관문/두 snapshot/old 삭제도 같은 절대 source→work 경로에서 실행한다.
TEST_CASE(EditorCatalogFiles_NativeRenameReadonlyAndDeletionShareNamespace) {
    for (const auto readonly:{0U,1U}) { //서로 독립된 writable/readonly 원문 분기를 실제 파일 장치와 결합한다.
        const auto root=NewRoot("native-rename-"+std::to_string(readonly)),sourceRoot=root/"source";const auto bytes=Image(2);Seed(sourceRoot/"d/Old.fort",bytes);BaseFileSystem source(sourceRoot);EditorFiles work(root/"work",{sourceRoot});auto workspace=Workspace(work,sourceRoot,source);const auto old=workspace->Prepare("d/Old.fort");ClientFortGlobals names;std::copy_n("Old",4,names.CurrentBytes().begin());CampaignTemplateStorage storage;FortTemplate fort;fort.sections.resize(35);storage.Install(1,std::move(fort),old.string());storage.Headers()[20+8]=static_cast<std::uint8_t>(readonly);
        ConfigInterface config({});config.Set("fortSpec",(sourceRoot/"d/{local.1}.fort").string());std::vector<std::string> snapshots;FortCurrentNameHooks hooks;hooks.snapshot=[&] { snapshots.emplace_back(names.Current());work.Write(storage.Name(1),bytes); };hooks.notify=[](auto) {};hooks.refreshTitle=[] {};hooks.assertion=[](auto) { throw std::logic_error("Native rename 예상 밖 진단"); };
        FortCurrentNameGlobals globals{[] { return 1U; },[] { return 1U; },[] { return 1U; },[] { return 0U; },names.Catalog().dirty};FortCurrentNameDevices rename(names,storage,config,std::move(globals),std::move(hooks),&work,[workspace](auto path) { return workspace->Resolve(path); });const std::string selected="New";rename.Set({reinterpret_cast<const std::uint8_t*>(selected.c_str()),selected.size()+1});
        CHECK(names.Current()=="New" && storage.Name(1)==workspace->Resolve("d/New.fort").string() && workspace->Read("d/New.fort")==bytes);CHECK(std::filesystem::exists(old)==(readonly!=0));CHECK(snapshots==(readonly ? std::vector<std::string>{"New"} : std::vector<std::string>{"Old","New"}));if (!readonly) CHECK(!workspace->Read((sourceRoot/"d/Old.fort").string()));CHECK(ReadFileBytes(sourceRoot/"d/Old.fort")==bytes);
    }
}

// 새 const API는 기존 등록순서만 사용하며 disk/secondary 우선순위를 바꾸지 않는다.
TEST_CASE(EditorCatalogFiles_ArchiveOnlyReadsRegisteredOrderWithoutDiskFallback) {
    const auto root=NewRoot("archive-only");Seed(root/"source/d/archive",std::array<std::uint8_t,1>{'D'});Seed(root/"secondary/d/only-secondary",std::array<std::uint8_t,1>{'S'});Seed(root/"first.tarc",Archive('A'));Seed(root/"second.tarc",Archive('B'));BaseFileSystem source(root/"source",root/"secondary");source.RegisterArchive(root/"first.tarc");source.RegisterArchive(root/"second.tarc");
    CHECK(source.Read("d/archive")==std::vector<std::uint8_t>{'D'} && source.TryReadArchiveOnly("d/archive")==std::vector<std::uint8_t>{'A'});CHECK(!source.TryReadArchiveOnly("d/only-secondary") && source.Read("d/only-secondary")==std::vector<std::uint8_t>{'S'});
}

// 기존 장부의 공유거부 open 실패는 실제 부재가 아니므로 빈 장부/원본 재복사로 바꾸지 않는다.
TEST_CASE(EditorCatalogFiles_ExistingUnreadableLedgerNeverRevivesDeletedSource) {
    const auto root=NewRoot("ledger-sharing"),sourceRoot=root/"source";const auto bytes=Image(2);Seed(sourceRoot/"d/Deleted.fort",bytes);BaseFileSystem source(sourceRoot);EditorFiles work(root/"work",{sourceRoot});std::filesystem::path deleted;
    { auto workspace=Workspace(work,sourceRoot,source);deleted=workspace->Prepare("d/Deleted.fort");work.Delete(deleted.string()); } //완료 키와 실제 삭제를 먼저 durable 상태로 만든다.
    auto metadata=work.SiblingNamespace("-state");const auto path=metadata.ResolvePath("catalog-v1.dat");const auto before=ReadFileBytes(path);
#ifdef _WIN32
    const auto blocked=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);CHECK(blocked!=INVALID_HANDLE_VALUE);if (blocked==INVALID_HANDLE_VALUE) return;
    { auto workspace=Workspace(work,sourceRoot,source);CHECK(Rejects([&] { workspace->Read("d/Deleted.fort"); }) && !std::filesystem::exists(deleted)); } //catalog.lock와 다른 ledger HANDLE을 외부에서 잠그는 실제 반례다.
    CHECK(CloseHandle(blocked)!=0);
#endif
    CHECK(ReadFileBytes(path)==before && ReadFileBytes(sourceRoot/"d/Deleted.fort")==bytes && !std::filesystem::exists(deleted));auto reopened=Workspace(work,sourceRoot,source);CHECK(!reopened->Read("d/Deleted.fort"));
}

// 분류 파일을 읽은 뒤 교체한 현재 work를 별도 메타 Template이 다시 읽어야 한다.
TEST_CASE(EditorCatalogFiles_ClassificationAndMetadataStillRereadCurrentWork) {
    const auto root=NewRoot("current-reread"),sourceRoot=root/"source";const auto before=Image(2),after=Image(1,1);Seed(sourceRoot/"d/Current.fort",before);BaseFileSystem source(sourceRoot);EditorFiles work(root/"work",{sourceRoot});auto workspace=Workspace(work,sourceRoot,source);auto effects=Effects(workspace);std::size_t loads{};
    effects.typeCount=[&] { if (++loads==1) work.Write(workspace->Resolve("d/Current.fort").string(),after);return 188; };ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");SoloMapCatalog catalog(source,config,std::move(effects));CHECK(catalog.Refresh(2)==1 && loads==2 && catalog.Cached().flags[0]==2 && catalog.Cached().names[0]=="Current");CHECK(ReadFileBytes(sourceRoot/"d/Current.fort")==before);
}
