// 실제 한글·보충 평면 경로의 편집 읽기/쓰기/이관/장부를 게임 창 없이 확인한다.
#include "client/EditorFiles.h"
#include "client/EditorMapCatalogFiles.h"
#include "client/EditorFortFileDevices.h"
#include "platform/Utf8Path.h"
#include "TestSupport.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // Windows 매크로가 표준 함수 이름을 바꾸지 않게 한다.
#endif
#include <windows.h>
#endif

namespace { // 실제 보호 원본과 사용자 작업본을 건드리지 않는 검사 장치다.
using namespace netstorm::client; // 실제 편집 파일/목록/현재 이름 장치를 사용한다.
using namespace netstorm::platform; // 경로 입력은 명시 UTF-8이며 시스템 ANSI에 의존하지 않는다.
// UTF-8 리터럴을 기존 게임의 문자열 인터페이스에 전달한다.
std::string Utf8(std::u8string_view text) { return {reinterpret_cast<const char*>(text.data()),text.size()}; }
// 기존 실패 자료를 지우지 않고 새 고유 검사 디렉터리를 만든다.
std::filesystem::path NewRoot() {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count(); // 이번 실행의 고유 이름이다.
    return std::filesystem::temp_directory_path()/PathFromUtf8(Utf8(u8"Netstorm 편집 😀-" )+std::to_string(stamp));
}
// 검사 소유의 source 복사 입력만 기록하며 보호 원본을 열지 않는다.
void Seed(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) {
    std::filesystem::create_directories(path.parent_path());std::ofstream output(path,std::ios::binary|std::ios::trunc); // 실제 wide 경로의 원시 파일이다.
    output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!output) throw std::runtime_error("Unicode 편집 검사 입력 쓰기 실패");
}
// 실제 거부와 파일 부재를 구별한다.
template<class F> bool Rejects(F function) { try { function(); }catch (const std::exception&) { return true; }return false; }
}

// ANSI로 나타낼 수 없는 경로에서도 전체 파일·권한·원자 장부·삭제와 root 보호가 함께 동작해야 한다.
TEST_CASE(EditorUnicode_FilesPermissionsAtomicAndBoundary) {
    const auto root=NewRoot(),source=root/PathFromUtf8(Utf8(u8"보호 원본 😀")),work=root/PathFromUtf8(Utf8(u8"작업본 😀")); // 원본 역할도 검사 소유의 별도 경로다.
    const std::vector<std::uint8_t> initial{3,1,4,1,5},edited{9,2,6}; // 실제 파일의 입력/수정 바이트다.
    EditorFiles files(work,{source});const auto name=Utf8(u8"시험맵 😀"),relative=Utf8(u8"d/시험맵 😀.fort"); // 파일 이름과 설치 루트 모두 비ANSI 문자다.
    const auto path=files.Prepare(name,initial);const auto absolute=PathToUtf8(path); // 기존 파일을 보존하는 실제 준비 장치를 사용한다.
    CHECK(files.Exists(absolute)==5 && files.TryRead(absolute)==initial && PathToUtf8(files.RelativePath(absolute),true)==relative);
#ifdef _WIN32
    CHECK(SetFileAttributesW(path.c_str(),FILE_ATTRIBUTE_READONLY)!=0);CHECK(files.ReadOnly(absolute));
#endif
    files.MakeWritable(absolute,0x180);CHECK(!files.ReadOnly(absolute));files.Write(absolute,edited);CHECK(files.TryRead(relative)==edited);
    files.WriteAtomic(Utf8(u8"장부 😀/기록.dat"),initial);CHECK(files.TryRead(Utf8(u8"장부 😀/기록.dat"))==initial);
    const auto names=files.MatchDisk(Utf8(u8"d/*.fort"));CHECK(std::any_of(names.begin(),names.end(),[&](const auto& entry) { return std::filesystem::equivalent(PathFromUtf8(entry),path); })); // UTF-8 결과를 다시 실제 파일에 사용할 수 있어야 한다.
    CHECK(Rejects([&] { files.Write(PathToUtf8(source/"blocked.fort"),edited); }));CHECK(Rejects([&] { files.Write(PathToUtf8(work.parent_path()/PathFromUtf8(Utf8(u8"작업본 😀-다른곳"))/"blocked.fort"),edited); }));
    files.Delete(absolute);CHECK(!files.TryRead(absolute) && files.Exists(absolute)==0);CHECK(!std::filesystem::exists(source/"blocked.fort"));
}

// 최초 복사와 UTF-8 장부 재열기 뒤 사용자 삭제를 보존하고 source 절대 경로도 같은 work에 매핑한다.
TEST_CASE(EditorUnicode_CatalogImportReopenAndDeletion) {
    const auto root=NewRoot(),source=root/PathFromUtf8(Utf8(u8"원본 😀")),work=root/PathFromUtf8(Utf8(u8"편집 😀")); // 실제 source/work는 검사 소유다.
    const auto key=Utf8(u8"d/한글맵 😀.fort");const std::vector<std::uint8_t> input{7,8,9}; // 최초 복사 전 원문 바이트다.
    const auto original=source/PathFromUtf8(key);Seed(original,input);auto files=std::make_shared<EditorFiles>(work,std::vector<std::filesystem::path>{source}); // 원본과 작업본을 동시에 보호한다.
    {
        EditorCatalogWorkspace catalog(files,{source,{}});const auto names=catalog.Enumerate("d/*.fort"); // 실제 Win32 열거/복사/원자 장부를 통과한다.
        const auto mapped=catalog.Resolve(PathToUtf8(original));CHECK(mapped==work/PathFromUtf8(key));CHECK(catalog.Read(key)==input && std::any_of(names.begin(),names.end(),[&](const auto& entry) { return std::filesystem::equivalent(PathFromUtf8(entry),mapped); }));
        files->Delete(PathToUtf8(mapped));CHECK(!catalog.Read(key));
    }
    {
        EditorCatalogWorkspace reopened(files,{source,{}});CHECK(reopened.Enumerate("d/*.fort").empty());CHECK(!reopened.Read(PathToUtf8(original))); // UTF-8 완료 장부의 삭제 상태를 새 owner도 유지한다.
    }
    CHECK(netstorm::o::ReadFileBytes(original)==input);
}

// 현재 Config/FileSpec/이름으로 만든 절대 경로가 기존 준비·읽기·readonly 장치 사이에서 변형되지 않는다.
TEST_CASE(EditorUnicode_CurrentFortDeviceRoundTrip) {
    const auto root=NewRoot(),sourceRoot=root/PathFromUtf8(Utf8(u8"자산 😀")),workRoot=root/PathFromUtf8(Utf8(u8"편집 😀")); // ANSI와 다른 실제 작업 루트를 사용한다.
    std::string name=Utf8(u8"지도 😀");const auto key="d/"+name+".fort";const std::vector<std::uint8_t> input{1,0,2,0,3};Seed(sourceRoot/PathFromUtf8(key),input); // 현재 이름에서 파일을 선택한다.
    EditorFiles work(workRoot,{sourceRoot});netstorm::o::BaseFileSystem source(sourceRoot);netstorm::o::ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");
    EditorFortPathEffects effects;effects.currentName=[&]()->std::string_view { return name; };effects.selectFileSpec=[] {};effects.fileSpecKey=[]()->std::string_view { return "fortSpec"; };effects.assertion=[](auto) { throw std::logic_error("Unicode 현재 파일의 예상 밖 진단"); }; // 실제 현재 이름/키만 검사 입력으로 공급한다.
    EditorFortFileDevices devices(work,source,config,std::move(effects));const std::string path(devices.PrepareCurrent()); // 실제 파일 준비가 원문을 UTF-8 경로에 복사한다.
    CHECK(PathFromUtf8(path)==workRoot/PathFromUtf8(key) && devices.Exists(path)==5 && !devices.ReadOnly(path));CHECK(devices.Read(path)==input);
    CHECK(netstorm::o::ReadFileBytes(sourceRoot/PathFromUtf8(key))==input);
}

// 작업본 부재를 source VFS의 장치 경로 재조회와 혼동해 원본 내용을 되살리면 안 된다.
TEST_CASE(EditorUnicode_MissingWorkDoesNotReadSourceArchive) {
    const auto root=NewRoot(),repository=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path(); // 원본 아카이브는 읽기 입력으로만 사용한다.
    const auto archive=repository/"originals/netstorm.tarc";const auto parsed=netstorm::o::TaffArchive::Open(archive); // 보호 자료의 실제 기존 엔트리를 고른다.
    std::string key; // 기대 파일 내용을 제품 출력으로 만들지 않고 격리할 실제 이름만 선택한다.
    for (const auto& entry:parsed.Entries()) { // d/ 엔트리는 source/work 공통 접두 제거의 충돌을 실제로 일으킬 수 있다.
        const auto candidate=netstorm::o::NormalizeGamePath(entry.name);if (candidate.starts_with("d/") && entry.size) { key=candidate;break; }
    }
    CHECK(!key.empty());if (key.empty()) return;
    EditorFiles work(root/"d",{root/"source",repository/"originals",repository/"originalCD"});netstorm::o::BaseFileSystem source(root/"source");CHECK(source.RegisterArchive(archive));
    const auto path=PathToUtf8(work.ResolvePath(key.substr(2)));const auto fallback=source.TryRead(path);CHECK(fallback && !fallback->empty()); // 일반 VFS에는 실제로 source 아카이브 재조회가 가능한 입력이다.
    netstorm::o::ConfigInterface config({});EditorFortFileDevices devices(work,source,config,{});CHECK(devices.Read(path).empty());CHECK(!std::filesystem::exists(PathFromUtf8(path))); // 읽기는 작업본 부재를 보존하고 원본을 복사하지 않는다.
}
