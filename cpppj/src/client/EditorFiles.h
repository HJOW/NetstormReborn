// 편집 작업본만 쓰는 실제 파일 장치다. 문자열 경로는 Windows10/11 호환을 위해 원본 ANSI 대신 UTF-8이며 원본 복사 입력은 읽기 전용이다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "platform/UserDataRoot.h" // 5차: 사용자 홈 저장 정책의 작업/장부 루트를 계산한다.

namespace netstorm::o { class RawFileName; } // 실제Client와작업파일장치가같이빌리는원문10칸소유자다.
namespace netstorm::client {
struct TemplateFileWriteHooks; //실제 Template 저장 부모가 사용하는 권한/쓰기 경계다.
// 5차: Editor가 쓰는 작업 루트·장부(metadata) 루트·원본 보호 루트를 한 곳에서 계산한 값이다.
struct EditorRoots {
    std::filesystem::path work; // 편집 작업본(복사본/새 맵)을 두는 쓰기 가능 루트다. 사용자 홈 정책에서는 Saved Games 아래 Netstorm의 CustomMaps다.
    std::filesystem::path metadata; // 이관 장부/잠금을 두는 루트다. 비어 있으면 기존처럼 작업 루트의 형제 -state를 쓴다. 사용자 홈 정책에서는 Editor 폴더다.
    std::vector<std::filesystem::path> protectedRoots; // 쓰기를 거부할 원본 보호 루트(설치 폴더와 개발 저장소의 originals/originalCD)다.
};
// 설치 폴더와 사용자 데이터 정책에서 Editor 루트를 계산한다. userData가 비어 있으면 기존 형제 <이름>-editor 배치이며 파일을 만들지 않는다.
EditorRoots ResolveEditorRoots(const std::filesystem::path& gameDirectory,const std::optional<platform::UserDataRoot>& userData);
std::filesystem::path EditorWorkingRoot(const std::filesystem::path& gameDirectory); //실제 자산 디렉터리의 형제 <이름>-editor를 계산한다. 끝 구분자와 ./.. 표기는 같은 작업본을 선택하며 파일을 만들지 않는다.
// 사용자 작업본과 보호된 원본 경로를 다른 실제 디렉터리로 유지한다.
class EditorFiles {
public:
    EditorFiles(std::filesystem::path root,std::vector<std::filesystem::path> protectedRoots,std::shared_ptr<o::RawFileName> fileNames={}); //원본/CD보호루트를명시하며실제Client는같은경로고리도넘긴다. 독립장치는별도소유자를만든다.
    void BindFileNameOwner(std::shared_ptr<o::RawFileName> fileNames); // Template장치가같은source의고리소유권을파일조회전에연결한다. I/O/고리예약은하지않는다.
    std::shared_ptr<o::RawFileName> FileNameOwner() const; // 현재공유소유자만빌리며파일이나경로버퍼를조회하지않는다.
    const std::filesystem::path& RootPath() const; //검사된 실제 작업 root를 I/O 장치에만 빌린다. 쓰기 보호 범위를 넓히지 않는다.
    std::filesystem::path RelativePath(std::string_view configuredPath) const; //같은 보호 검사를 통과한 전체 물리 상대 경로를 얻는다. 이름 첫점/파일명으로 축약하지 않는다.
    EditorFiles ExplicitNamespace(const std::filesystem::path& directory) const; // 5차: 사용자 홈의 명시 metadata 폴더를 현재 작업/보호 root를 모두 보호하는 장치로 만든다. 작업 root와 겹치면 거부하며 파일을 쓰지 않는다.
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
    void Write(std::string_view fileName,std::span<const std::uint8_t> bytes) const; //일반복사/준비입력을검사된작업본에w+b로잘라쓴다.부모폴더를임의생성하지않으며Template저장은Writer를쓴다.
    void Delete(std::string_view fileName) const; // 원문0041aef0의 직접 파일 삭제를 작업 namespace에서만 수행한다. 같은 DeleteFileA를 쓰는 CRT unlink의 실패 반환은 원문처럼 사용하지 않는다.
    TemplateFileWriteHooks Writer() const; //이객체보다짧게사는실제Template저장장치다.기존파일은r+b로열고실패할때만w+b로생성하여이전tail을보존한다.
private:
    void WriteTemplate(std::string_view fileName,std::span<const std::uint8_t> bytes) const; //0041bb90의mode1열기/쓰기/반납을보호작업본에연결한다.일반Write의잘라쓰기와별개이며근거는docs/exe/cpp-editor-file-writer-20261101.md다.
    std::filesystem::path Checked(const std::filesystem::path& path) const; //현재 실제 경로가 작업 루트 안/모든 원본 루트 밖인지 확인한다.
    std::filesystem::path root_; //실제 정규화한 사용자 편집 작업 디렉터리다.
    std::vector<std::filesystem::path> protected_; //원본/CD와 다른 읽기 전용 자산 디렉터리들의 실제 경로다.
    std::shared_ptr<o::RawFileName> fileNames_; // 실제Template의존재/권한/읽기가같은Client고리의현재번호와물리칸을사용한다.
};
}
