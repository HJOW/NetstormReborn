// 편집 작업본만 쓰는 실제 파일 장치다. 원본 파일의 복사 입력은 호출자가 읽기 전용으로 공급한다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace netstorm::client {
struct TemplateFileWriteHooks; //실제 Template 저장 부모가 사용하는 권한/쓰기 경계다.
// 사용자 작업본과 보호된 원본 경로를 다른 실제 디렉터리로 유지한다.
class EditorFiles {
public:
    EditorFiles(std::filesystem::path root,std::vector<std::filesystem::path> protectedRoots); //원본/CD 보호 루트들을 반드시 명시하며 생성만으로 파일을 쓰지 않는다.
    std::filesystem::path Prepare(std::string_view name,std::span<const std::uint8_t> original); //없는 작업본만 원문으로 만들고 기존 사용자 작업본은 유지한다.
    std::filesystem::path ResolvePath(std::string_view configuredPath) const; //현재Config.PathSpec의상대경로를같은작업namespace의검사된절대경로로해석한다.파일생성/읽기는하지않는다.
    std::filesystem::path PreparePath(std::string_view configuredPath,std::span<const std::uint8_t> original); //같은Configured경로의기존작업본은보존하고없는파일만기존Write장치로복사한다.
    std::int32_t Exists(std::string_view fileName) const; //준비된절대작업경로의원문0041ab10직접파일분기다.fopen rb성공출처5/실패0을돌려준다.
    bool ReadOnly(std::string_view fileName,const std::function<void(std::uint32_t)>& assertion={}) const; //현재CRT_stat의_S_IWRITE비트만읽는다.Config readOnlyForts와별개이며stat실패는진단뒤명시중단한다.
    void MakeWritable(std::string_view fileName,std::uint32_t mode) const; //실제 쓰기 경로를 확인한 뒤 원본 chmod(0x180) 요청을 수행한다.
    void Write(std::string_view fileName,std::span<const std::uint8_t> bytes) const; //검사한 작업본에만 원본 w+b 형식으로 쓴다. 부모 폴더를 임의 생성하지 않는다.
    TemplateFileWriteHooks Writer() const; //이 객체보다 짧게 살아야 하는 실제 저장 부모의 장치를 반환한다.
private:
    std::filesystem::path Checked(const std::filesystem::path& path) const; //현재 실제 경로가 작업 루트 안/모든 원본 루트 밖인지 확인한다.
    std::filesystem::path root_; //실제 정규화한 사용자 편집 작업 디렉터리다.
    std::vector<std::filesystem::path> protected_; //원본/CD와 다른 읽기 전용 자산 디렉터리들의 실제 경로다.
};
}
