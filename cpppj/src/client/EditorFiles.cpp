// 원본을 수정하지 않는 준비/권한/쓰기 경계만 제공하며 게임의 저장 관문과 직렬화는 기존 몸체가 수행한다.
#include "client/EditorFiles.h"
#include "client/CampaignTemplateStorage.h"
#include "o/BaseFile.h"
#include "o/RawBaseFileIO.h"
#include "o/OriginalText.h"
#include "platform/Utf8Path.h"
#include <cstdio>
#include <stdexcept>
#include <sys/stat.h>
#include <utility>
#include <limits>
#ifdef _WIN32
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX //원문 Win32 복사/flush 장치의 매크로가 표준 수치 범위를 바꾸지 않게 한다.
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace netstorm::client {
namespace {
using platform::PathFromUtf8; // 편집 문자열은 UTF-8이며 원본 ANSI와 다른 Windows10/11 호환 경계다.
using platform::PathToUtf8; // 실제 경로를 장부/기존 문자열 인터페이스에 손실 없이 전달한다.
// 원문rb/r+b/w+b모드를같은CRT로열며Windows10/11한글경로는원본ANSI대신wide문자로보존한다.
FILE* OpenEditorFile(const std::filesystem::path& path,std::string_view mode) {
    if (mode!="rb" && mode!="r+b" && mode!="w+b") throw std::invalid_argument("편집파일의실제CRT모드범위");
#ifdef _WIN32
    const std::wstring wide(mode.begin(),mode.end());return _wfopen(path.c_str(),wide.c_str());
#else
    const std::string narrow(mode);return std::fopen(path.c_str(),narrow.c_str());
#endif
}
// 기존일반준비/복사와존재조회는원래의잘라쓰기/읽기계약을유지하며같은CRT장치만공유한다.
FILE* OpenEditorFile(const std::filesystem::path& path,bool write) { return OpenEditorFile(path,std::string_view(write ? "w+b" : "rb")); }
// 원문0041bb90의지역파일을같은기존BaseFile몸체에빌리고정상/예외모두한번반납한다.
struct EditorTemplateWriter {
    o::RawBaseFileState state{}; // 실제FILE포인터토큰을보관하며원문24BYTE ABI로직렬화하지않는다.
    o::RawBaseFileIOHooks hooks; // 원문열기/한element쓰기/닫기의실제CRT장치다.부분반환은재시도/예외로바꾸지않는다.
    // 실제쓰기경로는호출자가이미보호검사했고이동기수명동안고정된절대경로만연다.
    explicit EditorTemplateWriter(const std::filesystem::path& path) {
        o::ConstructBaseFile(state);hooks.open=[&path](auto,auto mode) { return reinterpret_cast<std::uintptr_t>(OpenEditorFile(path,mode)); };
        hooks.write=[](auto token,auto bytes,auto size,auto count) { static_cast<void>(std::fwrite(bytes.data(),size,count,reinterpret_cast<FILE*>(token))); }; //0041a910은fwrite(data,size,1)의반환을사용하지않는다.
        hooks.close=[](auto token) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(token))); }; //0041bb90의fclose반환으로새실패분기를만들지않는다.
    }
    ~EditorTemplateWriter() { o::DestructBaseFile(state,hooks); } //원문처럼FILE을닫고현재키를반납한다.이소비자는암호키를설치하지않는다.
};
// 존재하는 상위 경로의 링크도 해석한 절대 경로를 사용한다.
std::filesystem::path Actual(const std::filesystem::path& path) { return std::filesystem::weakly_canonical(std::filesystem::absolute(path)); }
// Windows의 ASCII 경로 대소문자와 경로 요소 경계를 함께 비교한다.
bool Contains(const std::filesystem::path& root,const std::filesystem::path& path) {
    auto prefix=o::AsciiLower(PathToUtf8(root,true));const auto name=o::AsciiLower(PathToUtf8(path,true));if (prefix==name) return true;if (!prefix.ends_with('/')) prefix+='/';return name.starts_with(prefix);
}
}
// 자산 경로를 실제 디렉터리로 먼저 정규화하여 끝 구분자가 자식 -editor 경로를 만들지 않게 한다.
std::filesystem::path EditorWorkingRoot(const std::filesystem::path& gameDirectory) {
    auto directory=Actual(gameDirectory); //기존 상위 링크와 ./..를 해석한 실제 자산 디렉터리다.
    while (directory.has_relative_path() && directory.filename().empty()) directory=directory.parent_path(); //끝 구분자의 빈 요소만 제거하고 드라이브/파일시스템 루트는 넘지 않는다.
    if (!directory.has_filename() || directory==directory.root_path()) throw std::invalid_argument("편집 작업본의 자산 디렉터리 이름이 없습니다");
    auto name=directory.filename();name+="-editor";return directory.parent_path()/name; //같은 실제 이름으로 형제 작업 디렉터리만 선택한다. 원본 보호 검사는 EditorFiles가 계속 수행한다.
}
// 작업 경로를 원본/CD 아래로 지정한 경우 파일 생성 전에 거부한다.
EditorFiles::EditorFiles(std::filesystem::path root,std::vector<std::filesystem::path> protectedRoots):root_(Actual(root)) {
    if (protectedRoots.empty()) throw std::invalid_argument("편집 파일 장치의 원본 보호 경로 누락");
    for (const auto& path:protectedRoots) { //모든 원본 루트를 실제 경로로 보관하고 작업 루트와의 관계를 확인한다.
        const auto source=Actual(path);if (Contains(source,root_)) throw std::invalid_argument("편집 작업 디렉터리가 보호 원본 내부입니다");protected_.push_back(source);
    }
}
// 상대 파일명은 작업 루트 기준이며 폴더 링크를 통해 원본으로 들어가는 경우도 거부한다.
std::filesystem::path EditorFiles::Checked(const std::filesystem::path& path) const {
    const auto target=Actual(path.is_absolute() ? path : root_/path);if (!Contains(root_,target)) throw std::invalid_argument("편집 파일 쓰기가 현재 작업 디렉터리를 벗어납니다");
    for (const auto& source:protected_) if (Contains(source,target)) throw std::invalid_argument("보호 원본 파일에 대한 편집 쓰기 요청"); //작업 루트가 상위 폴더인 경우도 각 원본을 다시 검사한다.
    return target;
}
// 원본 pending fort 이름의 점 없는 계약과 실제 Windows 파일명의 물리 범위를 지킨다.
std::filesystem::path EditorFiles::Prepare(std::string_view name,std::span<const std::uint8_t> original) {
    if (name.empty() || name.find('\0')!=std::string_view::npos || name.find_first_of("./\\:*?\"<>|")!=std::string_view::npos) throw std::invalid_argument("편집 Fort 이름의 원본 점/경로 문자 계약 위반");
    return PreparePath(PathToUtf8(std::filesystem::path("d")/PathFromUtf8(std::string(name)+".fort")),original);
}
// 실제 설정 경로도 기존의 같은 root/보호 루트 검사를 거친다.
std::filesystem::path EditorFiles::ResolvePath(std::string_view configuredPath) const { return Checked(PathFromUtf8(configuredPath)); }
// root 주소는 장치 수명 동안 고정이며 경로 조회를 파일 쓰기로 바꾸지 않는다.
const std::filesystem::path& EditorFiles::RootPath() const { return root_; }
// 정규화된 절대 표기에서 root 길이만 제거하므로 ASCII case 별칭과 파일명의 모든 점을 유지한다.
std::filesystem::path EditorFiles::RelativePath(std::string_view configuredPath) const {
    const auto requested=PathFromUtf8(configuredPath);const auto lexical=std::filesystem::absolute(requested.is_absolute() ? requested : root_/requested).lexically_normal(); //키는 실제 디렉터리 안의 물리 이름 전체다.
    if (PathToUtf8(lexical.filename()).find_first_of("*?")==std::string::npos) static_cast<void>(Checked(lexical));else static_cast<void>(Checked(lexical.parent_path()));
    const auto selected=Contains(root_,lexical) ? lexical : Checked(lexical);const auto prefix=PathToUtf8(root_,true),name=PathToUtf8(selected,true);return name.size()==prefix.size() ? std::filesystem::path(".") : PathFromUtf8(name.substr(prefix.size()+1));
}
// metadata가 임의 fortSpec 목록에 끼지 않도록 writable root 밖의 검증된 형제만 허용한다.
EditorFiles EditorFiles::SiblingNamespace(std::string_view suffix) const {
    if (suffix.empty() || suffix.find('\0')!=std::string_view::npos || suffix.find_first_of("./\\:*?\"<>|")!=std::string_view::npos) throw std::invalid_argument("편집 metadata 형제 이름 범위 위반");
    auto roots=protected_;roots.push_back(root_);auto name=root_.filename();name+=PathFromUtf8(suffix).native();const auto expected=root_.parent_path()/name;if (o::AsciiLower(PathToUtf8(Actual(expected),true))!=o::AsciiLower(PathToUtf8(expected.lexically_normal(),true))) throw std::invalid_argument("편집 metadata 형제의 외부 링크 경계");return EditorFiles(expected,std::move(roots));
}
// 생성 앞뒤 같은 실제 경로를 검사하고 원본/CD/외부 junction은 폴더 생성 전에 거부한다.
void EditorFiles::EnsureDirectory(std::string_view configuredPath) const { const auto path=ResolvePath(configuredPath);std::filesystem::create_directories(path);static_cast<void>(ResolvePath(configuredPath)); }
// copy source는 읽기만 열며 fail-if-exists로 기존 사용자 데이터/속성을 덮어쓰지 않는다.
void EditorFiles::CopyMissing(const std::filesystem::path& source,std::string_view configuredPath) const {
    const auto path=ResolvePath(configuredPath);if (std::filesystem::exists(path)) return;EnsureDirectory(PathToUtf8(path.parent_path()));const auto checked=ResolvePath(configuredPath);
#ifdef _WIN32
    if (!CopyFileW(source.c_str(),checked.c_str(),TRUE)) { const auto error=GetLastError();if ((error==ERROR_FILE_EXISTS || error==ERROR_ALREADY_EXISTS) && std::filesystem::exists(ResolvePath(configuredPath))) return;throw std::runtime_error("편집 원본 최초 복사 실패:"+std::to_string(error)); }
#else
    std::filesystem::copy_file(source,checked,std::filesystem::copy_options::skip_existing);
#endif
}
// 검증된 metadata 임시 파일과 최종 파일만 쓰며 실패 시 기존 장부와 앞선 효과를 보존한다.
void EditorFiles::WriteAtomic(std::string_view configuredPath,std::span<const std::uint8_t> bytes) const {
    if (bytes.size()>std::numeric_limits<std::uint32_t>::max()) throw std::length_error("편집 장부 DWORD 쓰기 범위");const auto target=ResolvePath(configuredPath);EnsureDirectory(PathToUtf8(target.parent_path()));const auto pending=ResolvePath(PathToUtf8(target)+".pending");
#ifdef _WIN32
    const auto handle=CreateFileW(pending.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if (handle==INVALID_HANDLE_VALUE) throw std::runtime_error("편집 장부 임시 파일 열기 실패");
    DWORD written{};const auto good=WriteFile(handle,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr) && written==bytes.size() && FlushFileBuffers(handle);const auto closed=CloseHandle(handle);
    if (!good || !closed) throw std::runtime_error("편집 장부 쓰기/flush/닫기 실패");
    constexpr unsigned kReplaceAttempts=51; //외부 인덱서 등의 짧은 공유 거부만 최대1초 기다리고 장부를 직접 덮어쓰지 않는다.
    constexpr DWORD kReplaceWaitMilliseconds=20; //실패한 OS 교체 사이의 실제 장치 대기이며 게임 상태나 진행값을 변경하지 않는다.
    for (unsigned attempt=0;attempt<kReplaceAttempts;++attempt) { //매 교체 직전에 같은 원본/외부 링크 보호 경로를 다시 검사한다.
        if (MoveFileExW(ResolvePath(PathToUtf8(pending)).c_str(),ResolvePath(PathToUtf8(target)).c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) return;
        const auto error=GetLastError(); //실패 직후 원시 오류를 보존하며 이후 경로/속성 조회의 오류와 혼동하지 않는다.
        const bool transient=error==ERROR_ACCESS_DENIED || error==ERROR_SHARING_VIOLATION || error==ERROR_LOCK_VIOLATION || error==ERROR_USER_MAPPED_FILE; //Windows 파일 교체의 공유/매핑 거부만 제한적으로 다시 시도한다.
        if (!transient || attempt+1==kReplaceAttempts) throw std::runtime_error("편집 장부 durable 교체 실패:"+std::to_string(error));
        Sleep(kReplaceWaitMilliseconds); //성공을 합성하거나 old 장부를 삭제하지 않고 같은 atomic 교체만 다시 요청한다.
    }
#else
    Write(PathToUtf8(pending),bytes);std::filesystem::rename(pending,target); //Windows 외의 준비용 대응이며 실제 durable 검증으로 주장하지 않는다.
#endif
}
// 와일드카드는 마지막 파일 요소에서만 허용하고 열거 폴더와 각 실제 결과를 같은 보호 경계에서 검사한다.
std::vector<std::string> EditorFiles::MatchDisk(std::string_view pattern) const {
    if (pattern.find('\0')!=std::string_view::npos) throw std::invalid_argument("편집 목록 패턴의 NUL 경계 위반");
    const auto requested=PathFromUtf8(pattern); //현재 UTF-8 설정의 상대/절대 패턴을 파일명과 부모 경로로 분리한다.
    if (PathToUtf8(requested.parent_path()).find_first_of("*?")!=std::string::npos) throw std::invalid_argument("편집 목록의 폴더 와일드카드 범위 위반");
    const auto directory=ResolvePath(PathToUtf8(requested.parent_path())),absolute=directory/requested.filename(); //실제 부모만 canonical 검사하고 파일 와일드카드를 filesystem 경로 조회에 넘기지 않는다.
    const o::BaseFileSystem disk(root_);std::vector<std::string> result; //기존 읽기 전용 OS 열거만 빌리며 전역 파일 저장소는 바꾸지 않는다.
    for (const auto& name:disk.MatchDisk(PathToUtf8(absolute))) { //원본 FindFirst/Next 순서를 유지하되 작업본 밖으로 향하는 링크는 여기서 거부한다.
        auto listed=PathFromUtf8(name);const auto leaf=PathToUtf8(listed.filename());if (leaf!="." && leaf!="..") static_cast<void>(ResolvePath(name));listed.make_preferred();result.push_back(PathToUtf8(listed)); //OS 자체 . / .. 후보는 읽기 요청이 아니므로 부모 검사 뒤 보존한다. 실제 reader/writer의 root 보호는 그대로다.
    }
    return result;
}
// 실제 파일을 읽는 매번 canonical 검사를 반복하므로 열거 이후 링크 교체도 다음 접근에서 다시 확인한다.
std::optional<std::vector<std::uint8_t>> EditorFiles::TryRead(std::string_view configuredPath) const {
    if (configuredPath.find('\0')!=std::string_view::npos) throw std::invalid_argument("편집 목록 파일명의 NUL 경계 위반");
    const auto requested=PathFromUtf8(configuredPath);const auto path=PathToUtf8(requested.filename()).find_first_of("*?")==std::string::npos ? ResolvePath(configuredPath) : ResolvePath(PathToUtf8(requested.parent_path()))/requested.filename(); //파일명 wildcard를 fopen에 전달하는 원문 실패도 canonical wildcard 조회 없이 보존한다.
    return o::BaseFileSystem(root_).TryRead(PathToUtf8(path));
}
// 원래 Prepare의 기존 파일 보존/새 파일 생성 순서를 설정 경로에서도 그대로 사용한다.
std::filesystem::path EditorFiles::PreparePath(std::string_view configuredPath,std::span<const std::uint8_t> original) {
    const auto target=ResolvePath(configuredPath);
    if (std::filesystem::exists(target)) { if (!std::filesystem::is_regular_file(target)) throw std::runtime_error("편집 작업본이 일반 파일이 아닙니다");return target; }
    std::filesystem::create_directories(target.parent_path());Write(PathToUtf8(target),original);return target;
}
// 경로를 가진 파일의 원문분기는 아카이브/보조 경로보다 해당 실제 파일만 fopen한다.
std::int32_t EditorFiles::Exists(std::string_view fileName) const {
    const auto path=ResolvePath(fileName);FILE* file=OpenEditorFile(path,false);if (!file) return 0;std::fclose(file);return 5;
}
// 실제 작업 파일은 원문 resolver의출처5다. stat의쓰기비트는NTFS readonly속성을반영하고ACL쓰기성공검사는하지않는다.
bool EditorFiles::ReadOnly(std::string_view fileName,const std::function<void(std::uint32_t)>& assertion) const {
    if (fileName.empty()) { if (!assertion) throw std::logic_error("편집 readonly의원문빈이름진단미연결");assertion(0x480);throw std::invalid_argument("편집 readonly의빈실제경로"); }
    const auto path=ResolvePath(fileName);
#ifdef _WIN32
    static_assert(_S_IWRITE==0x80); //원문stat mode하위BYTE의부호비트와같은CRT쓰기비트다.
    struct _stat64 status{};const auto result=_wstat64(path.c_str(),&status); //Windows10/11의 한글 경로는 원본 ANSI stat 대신 같은 wide CRT로 조회한다.
#else
    struct stat status{};const auto result=stat(path.string().c_str(),&status);
#endif
    if (result) { if (!assertion) throw std::logic_error("편집 readonly의원문stat실패진단미연결");assertion(0x48e);throw std::runtime_error("편집 readonly의stat실패뒤미정mode접근미복원"); }
#ifdef _WIN32
    return (status.st_mode&_S_IWRITE)==0;
#else
    return (status.st_mode&S_IWUSR)==0; //cpppj Windows외의호스트용대응이며NTFS관측으로주장하지않는다.
#endif
}
// chmod 실패는 원본처럼 쓰기를 중단시키지 않는다. 실제 쓰기 실패는 Write가 보고한다.
void EditorFiles::MakeWritable(std::string_view fileName,std::uint32_t mode) const {
    if (mode!=0x180U) throw std::invalid_argument("편집 파일 장치의 미지원 원본 권한 모드");const auto path=Checked(PathFromUtf8(fileName));
#ifdef _WIN32
    static_cast<void>(_wchmod(path.c_str(),static_cast<int>(mode))); //원본 권한 비트/실패 정책을 유지하고 Windows10/11의 경로 문자만 wide로 전달한다.
#else
    std::error_code error;std::filesystem::permissions(path,std::filesystem::perms::owner_read|std::filesystem::perms::owner_write,std::filesystem::perm_options::add,error);
#endif
}
// 일반복사/준비입력은기존내용을자르되작업경로를쓰기직전에검사한다.Template저장부모는별도Writer를사용한다.
void EditorFiles::Write(std::string_view fileName,std::span<const std::uint8_t> bytes) const {
    const auto path=Checked(PathFromUtf8(fileName));FILE* file=OpenEditorFile(path,true);if (!file) throw std::runtime_error("편집 작업본 파일 열기 실패");
    const auto written=std::fwrite(bytes.data(),1,bytes.size(),file);const auto closed=std::fclose(file);if (written!=bytes.size() || closed) throw std::runtime_error("편집 작업본 파일 쓰기/닫기 실패");
}
// 실제Template저장은기존파일의tail을자르지않고원문BaseFile의mode1/fallback/void쓰기/반납순서를사용한다.
void EditorFiles::WriteTemplate(std::string_view fileName,std::span<const std::uint8_t> bytes) const {
    const auto path=Checked(PathFromUtf8(fileName));EditorTemplateWriter writer(path);
    if (!o::OpenBaseFileDisk(writer.state,PathToUtf8(path),1,1,writer.hooks)) throw std::runtime_error("편집Template작업본파일열기실패"); //0041b4c0의장치경로쓰기실패는예외이며TAFF/보조경로로재조회하지않는다.
    o::WriteBaseFile(writer.state,bytes,-1,writer.hooks); //새r+b/w+b FILE의현재0위치부터요청한이미지한element를쓴다.부모폴더는생성하지않는다.
}
// 저장 부모가 결정한 이름과 이미지 원문을 같은 실제 장치에 인계한다.
TemplateFileWriteHooks EditorFiles::Writer() const {
    return {[this](auto name,auto mode) { MakeWritable(name,mode); },[this](auto name,auto bytes) { WriteTemplate(name,bytes); }};
}
// 실제 삭제 직전에 같은 canonical 경로를 다시 검사하며 원본/CD/작업루트 밖 요청은 수행하지 않는다.
void EditorFiles::Delete(std::string_view fileName) const {
    const auto path=Checked(PathFromUtf8(fileName)); // 디렉터리 제거와 재귀 삭제는 원문 DeleteFileA 경계에 포함되지 않는다.
#ifdef _WIN32
    static_cast<void>(_wunlink(path.c_str())); //Windows10/11의 한글 파일명은 원본 ANSI unlink 대신 같은 wide CRT로 삭제한다.
#else
    static_cast<void>(unlink(path.string().c_str()));
#endif
}
}
