// 관리 파일 읽기는 원래 위치도 지원하고 자동 저장은 기존 사용자 홈 정책을 따른다.
#include "root/SharedFiles.h"
#include "platform/Utf8Path.h"
#include "platform/PortableStartup.h" // 개발 실행의 게임 자료 폴더와 분리된 모듈 옆 서버 페이지 원천을 찾는다.
#include <bit>
#include <stdexcept>

namespace netstorm::root { // 실제 wide 파일 경로를 사용하는 R 전용 파일 장치다.
SharedFileStorage::SharedFileStorage(std::filesystem::path workingDirectory,std::filesystem::path dataDirectory,platform::UserDataRoot user)
    :workingDirectory_(std::move(workingDirectory)),dataDirectory_(std::move(dataDirectory)),user_(std::move(user)) {}
void SharedFileStorage::DataDirectory(std::filesystem::path directory) { dataDirectory_=std::move(directory); }
std::filesystem::path SharedFileStorage::Select(std::string_view name,const std::filesystem::path& fallback) const {
    const auto leaf=platform::PathFromUtf8(name);if (leaf.empty() || leaf.has_parent_path() || leaf!=leaf.filename()) throw std::invalid_argument("R 관리 파일의 단일 파일 이름 필요");
    const auto candidate=user_.Saves()/leaf;std::error_code error;const auto present=std::filesystem::exists(candidate,error);if (error) throw std::filesystem::filesystem_error("R 사용자 관리 파일 조회 실패",candidate,error);return present ? candidate:fallback/leaf;
}
// 사용자 Saves와 원문 작업 폴더가 우선이며 누락된 세 서버 페이지만 모듈 옆 불변 자산을 읽는다.
std::ifstream SharedFileStorage::OpenWorking(std::string_view name) const {
    auto selected=Select(name,workingDirectory_); // 사용자 파일의 존재/조회 오류를 기존 선택 장치에 그대로 맡긴다.
    const bool serverPage=name=="Admin.server" || name=="Update.server" || name=="Guild.server"; // 다른 관리 파일의 원문 검색 순서와 자동 저장 위치는 바꾸지 않는다.
    if (serverPage) {
        std::error_code error;const bool present=std::filesystem::exists(selected,error); // 존재하지만 읽을 수 없는 사용자 파일을 번들 내용으로 덮어 숨기지 않는다.
        if (!present && !error) selected=platform::CurrentExecutableFile().parent_path()/L"server"/platform::PathFromUtf8(name);
    }
    return std::ifstream(selected); // 기본 텍스트 모드의 CRLF/EOF와 실제 실패를 그대로 반환하며 파일을 생성하지 않는다.
}
std::filesystem::path SharedFileStorage::DataFile(std::string_view name) const { return Select(name,dataDirectory_); }
std::filesystem::path SharedFileStorage::WorkingFile(const std::filesystem::path& relative) const { return Select(platform::PathToUtf8(relative.filename()),workingDirectory_/relative.parent_path()); }
std::filesystem::path SharedFileStorage::WritableFile(std::string_view name) const {
    const auto leaf=platform::PathFromUtf8(name);if (leaf.empty() || leaf.has_parent_path() || leaf!=leaf.filename() || leaf==L"." || leaf==L"..") throw std::invalid_argument("R 자동 생성 파일의 단일 파일 이름 필요");platform::UserDataRoot::EnsureDirectory(user_.Saves());return user_.Saves()/leaf;
}
void SharedFileStorage::SaveBlacklist(std::span<const std::uint32_t> subscribers) const {
    SaveSubscribers("blacklist.txt",subscribers);
}
void SharedFileStorage::SaveSubscribers(std::string_view name,std::span<const std::uint32_t> subscribers) const {
    std::ofstream output(WritableFile(name),std::ios::out|std::ios::trunc);
    for (const auto subscriber:subscribers) output<<std::bit_cast<std::int32_t>(subscriber)<<std::endl; // 현재 표 순서를 유지하며 각 signed DWORD 뒤 LF/flush를 수행한다. 원문처럼 stream 실패를 별도 성공으로 보고하지 않는다.
}
void SharedFileStorage::SaveAddresses(std::span<const std::array<char,16>> addresses) const {
    std::ofstream output(WritableFile("ipbanlist.txt"),std::ios::out|std::ios::trunc);
    for (const auto& address:addresses) { const auto text=std::string_view(address.data(),address.size());const auto end=text.find('\0');if (end==text.npos) throw std::out_of_range("R IP 저장의 실제16BYTE 문자열 범위");output<<text.substr(0,end)<<std::endl; } // 현재 정규화된 각 문자열/endl을 쓰며 이 writer에서 다시 정규화하지 않는다.
}
}
