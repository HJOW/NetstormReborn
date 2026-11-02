// 실제 편집 저장/삭제의 경로 고리와 원문 직접 chmod의 비소모를 구별한다.
#include "TestSupport.h"
#include "client/EditorFiles.h"
#include "client/CampaignTemplateStorage.h"
#include "o/BaseFile.h"
#include "o/RawFileName.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <stdexcept>
#include <utility>
#ifndef NOMINMAX
#define NOMINMAX // Win32 매크로가 표준 함수 이름을 가리지 않게 한다.
#endif
#include <windows.h>

namespace { // 검사 파일은 Git 제외 scratch의 고유 디렉터리 안에만 만든다.
using namespace netstorm; // 실제 파일 장치와 원문 경로 소유자를 함께 사용한다.
// 이전 검사 자료를 지우지 않는 실제 Unicode 작업 루트다.
std::filesystem::path MutationRoot() {
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-editor-mutation-ring-740/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/L"저장 삭제😀";
}
// 실패의 종류와 앞선 경로 예약을 관측하며 성공으로 바꾸지 않는다.
template<class Function> bool MutationRejects(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; }
}

TEST_CASE(EditorMutationRing_TemplateWriterUsesOneSlotAfterDirectChmod) { // 44d8d0의 chmod는0칸, 41bb90→41b4c0의 절대 장치 판정은1칸이다.
    const auto root=MutationRoot();auto names=std::make_shared<o::RawFileName>();client::EditorFiles files(root/"work",{root/"source"},names);const std::vector<std::uint8_t> old(96,0x5a);const auto path=files.PreparePath("d/map.fort",old);const auto name=platform::PathToUtf8(path);const std::array<std::uint8_t,3> prefix{{'F',0,7}};const auto writer=files.Writer();
    CHECK(names->Index()==0);writer.permissions(name,0x180);CHECK(names->Index()==0);writer.write(name,prefix);
    auto expected=old;std::copy(prefix.begin(),prefix.end(),expected.begin());CHECK(names->Index()==1 && o::ReadFileBytes(path)==expected);
    writer.permissions(name,0x180);CHECK(names->Index()==1);writer.write(name,{});CHECK(names->Index()==2 && o::ReadFileBytes(path)==expected);
}

TEST_CASE(EditorMutationRing_CreationFallbackHasOneReservationAndHostPreparationHasNone) { // 두 fopen 모드의 fallback도 상위 FileSpec을 다시 만들지 않는다.
    const auto root=MutationRoot();auto names=std::make_shared<o::RawFileName>();client::EditorFiles files(root/"work",{root/"source"},names);files.EnsureDirectory("d");const auto path=files.ResolvePath("d/new.fort");const auto name=platform::PathToUtf8(path);const auto writer=files.Writer();const std::array<std::uint8_t,3> bytes{{1,2,3}};
    writer.write(name,bytes);CHECK(names->Index()==1 && o::ReadFileBytes(path)==std::vector<std::uint8_t>(bytes.begin(),bytes.end()));
    const auto empty=files.ResolvePath("d/empty.fort");writer.write(platform::PathToUtf8(empty),{});CHECK(names->Index()==2 && std::filesystem::exists(empty) && o::ReadFileBytes(empty).empty());
    files.Write(name,std::span<const std::uint8_t>(bytes).first(1));CHECK(names->Index()==2 && o::ReadFileBytes(path)==std::vector<std::uint8_t>{1});
    CHECK(files.PreparePath("d/new.fort",bytes)==path && names->Index()==2 && o::ReadFileBytes(path)==std::vector<std::uint8_t>{1}); // 호스트의 기존 파일 보존/잘라쓰기에 원문 getter를 합성하지 않는다.
}

TEST_CASE(EditorMutationRing_FailedOpenAndSharedDeleteKeepOneReservationEach) { // 실패 경로도 예약을 되돌리지 않으며 삭제 실패 반환은 원문처럼 무시한다.
    const auto root=MutationRoot();auto names=std::make_shared<o::RawFileName>();client::EditorFiles files(root/"work",{root/"source"},names);const std::array<std::uint8_t,3> bytes{{4,5,6}};const auto path=files.PreparePath("d/locked.fort",bytes);const auto name=platform::PathToUtf8(path);const auto writer=files.Writer();
    struct Locked { // 이 검사에서 연 공유 거부 핸들 한 개만 반납한다.
        HANDLE value{INVALID_HANDLE_VALUE}; // 다른 쓰기/삭제를 허용하지 않는 실제 읽기 핸들이다.
        ~Locked() { if (value!=INVALID_HANDLE_VALUE) CloseHandle(value); } // 실패한 검사도 사용자 핸들을 건드리지 않는다.
    } lock{CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)};
    CHECK(lock.value!=INVALID_HANDLE_VALUE);if (lock.value==INVALID_HANDLE_VALUE) return;
    CHECK(MutationRejects([&] { writer.write(name,{}); }) && names->Index()==1 && o::ReadFileBytes(path)==std::vector<std::uint8_t>(bytes.begin(),bytes.end()));
    files.Delete(name);CHECK(names->Index()==2 && std::filesystem::exists(path));
    const auto absent=files.ResolvePath("absent/child.fort");CHECK(MutationRejects([&] { writer.write(platform::PathToUtf8(absent),bytes); }) && names->Index()==3 && !std::filesystem::exists(absent.parent_path()));
}

TEST_CASE(EditorMutationRing_ReadonlyMissingAndSuccessfulDeleteShareCurrentOwner) { // 원문 삭제의 권한 거부/성공/부재 모두 같은 절대 장치 한 칸이다.
    const auto root=MutationRoot();auto names=std::make_shared<o::RawFileName>();client::EditorFiles files(root/"work",{root/"source"},names);const std::array<std::uint8_t,1> bytes{{8}};const auto path=files.PreparePath("d/readonly.fort",bytes);const auto name=platform::PathToUtf8(path);
    const auto changed=SetFileAttributesW(path.c_str(),FILE_ATTRIBUTE_READONLY);CHECK(changed!=0);if (!changed) return;
    files.Delete(name);CHECK(names->Index()==1 && std::filesystem::exists(path));files.MakeWritable(name,0x180);CHECK(names->Index()==1);
    files.Delete(name);CHECK(names->Index()==2 && !std::filesystem::exists(path));files.Delete(name);CHECK(names->Index()==3 && !std::filesystem::exists(path));
}

TEST_CASE(EditorMutationRing_ProtectedRequestsAndMetadataDoNotConsumeClientOwner) { // 원본 보호 거부는 예약 전이며 cpppj 전용 장부는 독립 고리를 유지한다.
    const auto root=MutationRoot();auto names=std::make_shared<o::RawFileName>();client::EditorFiles files(root/"work",{root/"source"},names);files.EnsureDirectory("d");const auto writer=files.Writer();const auto protectedName=platform::PathToUtf8(root/"source/kept.fort");
    CHECK(MutationRejects([&] { writer.permissions(protectedName,0x180); }) && MutationRejects([&] { writer.write(protectedName,{}); }) && MutationRejects([&] { files.Delete(protectedName); }) && names->Index()==0);
    CHECK(MutationRejects([&] { files.MakeWritable("d/map.fort",0); }) && names->Index()==0);
    auto metadata=files.SiblingNamespace("-state");metadata.EnsureDirectory(".");const auto name=platform::PathToUtf8(metadata.ResolvePath("catalog.dat"));metadata.Writer().write(name,{});metadata.Delete(name);
    CHECK(metadata.FileNameOwner()!=names && metadata.FileNameOwner()->Index()==2 && names->Index()==0);
}

TEST_CASE(EditorMutationRing_ActualTemplateSaveUsesClientOwnerOnce) { // 실제35섹션 직렬화/현재 이름/chmod/파일 저장까지의 소비량을 확인한다.
    const auto root=MutationRoot();o::BaseFileSystem source(root/"source");auto names=source.FileNameOwner();client::EditorFiles files(root/"work",{root/"source"},names);const std::vector<std::uint8_t> sentinel(128,0xa7);const auto path=files.PreparePath("d/map.fort",sentinel);const auto name=platform::PathToUtf8(path);
    o::FortTemplate fort;fort.sections.resize(35);client::CampaignTemplateStorage storage;storage.Install(1,std::move(fort),name);std::uint32_t readOnly{},owner=1;std::array<std::uint8_t,9> hidden{};o::FortImageHooks image;image.typeCount=[] { return 188U; };image.assertion=[](auto) { throw std::logic_error("실제 저장 고리의 예상 밖 이미지 진단"); };
    storage.SaveFile(1,{readOnly,owner,hidden},image,files.Writer());const auto saved=o::ReadFileBytes(path);
    CHECK(names->Index()==1 && storage.Name(1)==name && saved.size()==sentinel.size() && saved[0]=='F');CHECK(std::all_of(saved.begin()+72,saved.end(),[](auto byte) { return byte==0xa7; }));
}

TEST_CASE(EditorMutationRing_WritesWrapTheBorrowedViewAfterTenCalls) { // 실제 저장도 Client의 기존 열 칸 주소를 공유하여 열 번째에 이전 뷰를 덮어쓴다.
    const auto root=MutationRoot();auto names=std::make_shared<o::RawFileName>();client::EditorFiles files(root/"work",{root/"source"},names);const std::array<std::uint8_t,1> bytes{{9}};const auto name=platform::PathToUtf8(files.PreparePath("d/map.fort",bytes));const auto writer=files.Writer();const std::array<o::FileNamePart,1> parts{{{-3,"marker"}}};const auto marker=names->Format(parts);const auto at=names->Index();
    for (unsigned call=0;call<9;++call) { writer.permissions(name,0x180);writer.write(name,bytes); } // chmod는 예약하지 않고 실제 쓰기마다 한 칸만 진행한다.
    CHECK(marker==".\\marker" && names->Index()==(at+9)%10);writer.write(name,bytes);CHECK(marker!=".\\marker" && names->Index()==at);
}
