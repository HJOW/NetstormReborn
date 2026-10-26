// 편집 작업본만 쓰는 실제 파일 장치다. 원본 파일의 복사 입력은 호출자가 읽기 전용으로 공급한다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client {
struct TemplateFileWriteHooks; //실제 Template 저장 부모가 사용하는 권한/쓰기 경계다.
std::filesystem::path EditorWorkingRoot(const std::filesystem::path& gameDirectory); //실제 자산 디렉터리의 형제 <이름>-editor를 계산한다. 끝 구분자와 ./.. 표기는 같은 작업본을 선택하며 파일을 만들지 않는다.
// 사용자 작업본과 보호된 원본 경로를 다른 실제 디렉터리로 유지한다.
class EditorFiles {
public:
    EditorFiles(std::filesystem::path root,std::vector<std::filesystem::path> protectedRoots); //원본/CD 보호 루트들을 반드시 명시하며 생성만으로 파일을 쓰지 않는다.
    const std::filesystem::path& RootPath() const; //검사된 실제 작업 root를 I/O 장치에만 빌린다. 쓰기 보호 범위를 넓히지 않는다.
    std::filesystem::path RelativePath(std::string_view configuredPath) const; //같은 보호 검사를 통과한 전체 물리 상대 경로를 얻는다. 이름 첫점/파일명으로 축약하지 않는다.
    EditorFiles SiblingNamespace(std::string_view suffix) const; //현재 보호 roots와 work root를 모두 보호하는 형제 metadata 장치를 만들며 파일을 쓰지 않는다.
    void EnsureDirectory(std::string_view configuredPath) const; //검사된 namespace 안에 필요한 디렉터리만 만들고 생성 뒤 다시 검사한다.
    void CopyMissing(const std::filesystem::path& source,std::string_view configuredPath) const; //검사된 대상에 CopyFileW(TRUE)로 최초 복사한다. source 검사는 명시 source owner가 수행하며 기존 대상과 readonly 속성은 보존한다.
    void WriteAtomic(std::string_view configuredPath,std::span<const std::uint8_t> bytes) const; //metadata용 임시 파일을 flush한 뒤 같은 검사된 namespace에서 write-through 교체한다.
    std::filesystem::path Prepare(std::string_view name,std::span<const std::uint8_t> original); //없는 작업본만 원문으로 만들고 기존 사용자 작업본은 유지한다.
    std::filesystem::path ResolvePath(std::string_view configuredPath) const; //현재Config.PathSpec의상대경로를같은작업namespace의검사된절대경로로해석한다.파일생성/읽기는하지않는다.
    std::vector<std::string> MatchDisk(std::string_view pattern) const; //검사된 작업 디렉터리의 원본 Win32 순서를 읽는다. 디렉터리/명시 부재1경로도 제거하지 않고 각 결과의 경로만 검사한다.
    std::optional<std::vector<std::uint8_t>> TryRead(std::string_view configuredPath) const; //검사된 작업본만 기존 절대 파일 reader로 읽는다. 부재는 빈 optional이며 경로 거부를 부재로 바꾸지 않는다.
    std::filesystem::path PreparePath(std::string_view configuredPath,std::span<const std::uint8_t> original); //같은Configured경로의기존작업본은보존하고없는파일만기존Write장치로복사한다.
    std::int32_t Exists(std::string_view fileName) const; //준비된절대작업경로의원문0041ab10직접파일분기다.fopen rb성공출처5/실패0을돌려준다.
    bool ReadOnly(std::string_view fileName,const std::function<void(std::uint32_t)>& assertion={}) const; //현재CRT_stat의_S_IWRITE비트만읽는다.Config readOnlyForts와별개이며stat실패는진단뒤명시중단한다.
    void MakeWritable(std::string_view fileName,std::uint32_t mode) const; //실제 쓰기 경로를 확인한 뒤 원본 chmod(0x180) 요청을 수행한다.
    void Write(std::string_view fileName,std::span<const std::uint8_t> bytes) const; //검사한 작업본에만 원본 w+b 형식으로 쓴다. 부모 폴더를 임의 생성하지 않는다.
    void Delete(std::string_view fileName) const; // 원문0041aef0의 직접 파일 삭제를 작업 namespace에서만 수행한다. 같은 DeleteFileA를 쓰는 CRT unlink의 실패 반환은 원문처럼 사용하지 않는다.
    TemplateFileWriteHooks Writer() const; //이 객체보다 짧게 살아야 하는 실제 저장 부모의 장치를 반환한다.
private:
    std::filesystem::path Checked(const std::filesystem::path& path) const; //현재 실제 경로가 작업 루트 안/모든 원본 루트 밖인지 확인한다.
    std::filesystem::path root_; //실제 정규화한 사용자 편집 작업 디렉터리다.
    std::vector<std::filesystem::path> protected_; //원본/CD와 다른 읽기 전용 자산 디렉터리들의 실제 경로다.
};
}
