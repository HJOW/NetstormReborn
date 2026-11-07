// 관리 파일 읽기는 원래 위치도 지원하고 자동 저장은 기존 사용자 홈 정책을 따른다.
#include "root/SharedFiles.h"
#include "platform/Utf8Path.h"
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
std::ifstream SharedFileStorage::OpenWorking(std::string_view name) const { return std::ifstream(Select(name,workingDirectory_)); } // 기본 텍스트 모드의 CRLF/EOF 처리를 실제 CRT에 맡긴다.
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
