// 원본을 수정하지 않는 준비/권한/쓰기 경계만 제공하며 게임의 저장 관문과 직렬화는 기존 몸체가 수행한다.
#include "client/EditorFiles.h"
#include "client/CampaignTemplateStorage.h"
#include "o/OriginalText.h"
#include <cstdio>
#include <stdexcept>
#include <sys/stat.h>
#include <utility>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace netstorm::client {
namespace {
// 존재하는 상위 경로의 링크도 해석한 절대 경로를 사용한다.
std::filesystem::path Actual(const std::filesystem::path& path) { return std::filesystem::weakly_canonical(std::filesystem::absolute(path)); }
// Windows의 ASCII 경로 대소문자와 경로 요소 경계를 함께 비교한다.
bool Contains(const std::filesystem::path& root,const std::filesystem::path& path) {
    auto prefix=o::AsciiLower(root.generic_string());const auto name=o::AsciiLower(path.generic_string());if (prefix==name) return true;if (!prefix.ends_with('/')) prefix+='/';return name.starts_with(prefix);
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
    return PreparePath((std::filesystem::path("d")/(std::string(name)+".fort")).string(),original);
}
// 실제 설정 경로도 기존의 같은 root/보호 루트 검사를 거친다.
std::filesystem::path EditorFiles::ResolvePath(std::string_view configuredPath) const { return Checked(std::filesystem::path(configuredPath)); }
// 원래 Prepare의 기존 파일 보존/새 파일 생성 순서를 설정 경로에서도 그대로 사용한다.
std::filesystem::path EditorFiles::PreparePath(std::string_view configuredPath,std::span<const std::uint8_t> original) {
    const auto target=ResolvePath(configuredPath);
    if (std::filesystem::exists(target)) { if (!std::filesystem::is_regular_file(target)) throw std::runtime_error("편집 작업본이 일반 파일이 아닙니다");return target; }
    std::filesystem::create_directories(target.parent_path());Write(target.string(),original);return target;
}
// 경로를 가진 파일의 원문분기는 아카이브/보조 경로보다 해당 실제 파일만 fopen한다.
std::int32_t EditorFiles::Exists(std::string_view fileName) const {
    const auto path=ResolvePath(fileName);FILE* file=std::fopen(path.string().c_str(),"rb");if (!file) return 0;std::fclose(file);return 5;
}
// 실제 작업 파일은 원문 resolver의출처5다. stat의쓰기비트는NTFS readonly속성을반영하고ACL쓰기성공검사는하지않는다.
bool EditorFiles::ReadOnly(std::string_view fileName,const std::function<void(std::uint32_t)>& assertion) const {
    if (fileName.empty()) { if (!assertion) throw std::logic_error("편집 readonly의원문빈이름진단미연결");assertion(0x480);throw std::invalid_argument("편집 readonly의빈실제경로"); }
    const auto path=ResolvePath(fileName);
#ifdef _WIN32
    static_assert(_S_IWRITE==0x80); //원문stat mode하위BYTE의부호비트와같은CRT쓰기비트다.
    struct _stat64 status{};const auto result=_stat64(path.string().c_str(),&status);
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
    if (mode!=0x180U) throw std::invalid_argument("편집 파일 장치의 미지원 원본 권한 모드");const auto path=Checked(std::filesystem::path(fileName));
#ifdef _WIN32
    static_cast<void>(_chmod(path.string().c_str(),static_cast<int>(mode)));
#else
    std::error_code error;std::filesystem::permissions(path,std::filesystem::perms::owner_read|std::filesystem::perms::owner_write,std::filesystem::perm_options::add,error);
#endif
}
// 원본처럼 기존 내용을 자르되 작업 경로 검사는 권한 변경과 쓰기 직전에 각각 수행한다.
void EditorFiles::Write(std::string_view fileName,std::span<const std::uint8_t> bytes) const {
    const auto path=Checked(std::filesystem::path(fileName));FILE* file=std::fopen(path.string().c_str(),"w+b");if (!file) throw std::runtime_error("편집 작업본 파일 열기 실패");
    const auto written=std::fwrite(bytes.data(),1,bytes.size(),file);const auto closed=std::fclose(file);if (written!=bytes.size() || closed) throw std::runtime_error("편집 작업본 파일 쓰기/닫기 실패");
}
// 저장 부모가 결정한 이름과 이미지 원문을 같은 실제 장치에 인계한다.
TemplateFileWriteHooks EditorFiles::Writer() const {
    return {[this](auto name,auto mode) { MakeWritable(name,mode); },[this](auto name,auto bytes) { Write(name,bytes); }};
}
// 실제 삭제 직전에 같은 canonical 경로를 다시 검사하며 원본/CD/작업루트 밖 요청은 수행하지 않는다.
void EditorFiles::Delete(std::string_view fileName) const {
    const auto path=Checked(std::filesystem::path(fileName)); // 디렉터리 제거와 재귀 삭제는 원문 DeleteFileA 경계에 포함되지 않는다.
#ifdef _WIN32
    static_cast<void>(_unlink(path.string().c_str()));
#else
    static_cast<void>(unlink(path.string().c_str()));
#endif
}
}
