// 현재 서버 이름 조회의 LAN 번역/무효 조회 보존/유효 행의80BYTE 초기화를 구별한다.
#include "client/ClientNsInfoGlobals.h"
#include "client/RootServerEndpoint.h"
#include <algorithm>
#include <bit>
#include <cctype>
#include <cstring>
#include <sstream>
#include <stdexcept>
namespace netstorm::client {
// 현재 보호 파일은 짧은 두 행이다. 미복원 긴 원본 line-reader 경계를 임의 분할하지 않는다.
void ClientNsInfoGlobals::LoadServerNames(std::span<const std::uint8_t> file) {
    std::istringstream input(std::string(reinterpret_cast<const char*>(file.data()),file.size()));std::vector<std::string> loaded;std::string line;
    while (std::getline(input,line)) { // Native 시작 목록의 비어 있지 않은 행 순서를 유지한다.
        if (line.size()>256) throw std::length_error("시작 서버 목록의 긴 원본 행 미연결");line.erase(std::remove(line.begin(),line.end(),'\r'),line.end());if (!line.empty()) loaded.push_back(line);
    }
    servers=std::move(loaded);RecalculateServerChecksum();
}
// 원본은 LAN일 때 strcpy하고 그 외에는 유효 목록 항목에서만 출력 버퍼를 지운다.
std::string_view ClientNsInfoGlobals::ResolveServerName(const std::function<std::string(std::string_view)>& translate) {
    if (serverSelection==-1) {
        if (!translate) throw std::logic_error("현재 LAN 서버 이름 번역 미연결");const auto value=translate("LAN Server");const auto size=value.find('\0')==std::string::npos ? value.size() : value.find('\0');
        if (size>=serverName.size()) throw std::length_error("LAN 서버 이름의 원본80BYTE 경계 초과");std::copy_n(value.data(),size,serverName.data());serverName[size]=0;
    } else if (serverSelection>=0 && static_cast<std::size_t>(serverSelection)<servers.size()) {
        ReadServerFields(serverSelection,{},serverName,nullptr);
    }
    return {serverName.data(),static_cast<std::size_t>(std::find(serverName.begin(),serverName.end(),'\0')-serverName.begin())};
}
// 원문 wrapper는 유효하지 않은 index에서 하위 parser/출력0쓰기를 실행하지 않는다.
void ClientNsInfoGlobals::ReadServerFields(std::int32_t index,std::span<char> endpoint,std::span<char> name,std::int32_t* subscriber) const {
    if (index>=0 && static_cast<std::size_t>(index)<servers.size()) ReadRootServerRecord(servers[static_cast<std::size_t>(index)],endpoint,name,subscriber);
}
// 원문 flags7의 바이트 소문자/부분검색과 기존 교체의 stale 체크섬을 유지한다.
void ClientNsInfoGlobals::UpsertServer(std::string_view row,bool preserveValue) {
    std::array<char,80> endpoint{},description{};std::int32_t value{};ReadRootServerRecord(row,std::span(endpoint).first(79),std::span(description).first(79),&value);
    std::string name(endpoint.data());for (auto& byte:name) byte=static_cast<char>(std::tolower(static_cast<unsigned char>(byte))); // CRT locale의 바이트 단위 변환이며 표시 언어를 바꾸지 않는다.
    for (std::size_t index=0;index<servers.size();++index) { // 원문은 endpoint 전체 일치 대신 첫 대소문자 무시 부분 일치를 택한다.
        auto candidate=servers[index].substr(0,servers[index].find('\0'));for (auto& byte:candidate) byte=static_cast<char>(std::tolower(static_cast<unsigned char>(byte))); // 현재 저장 문자열만 검색용으로 소문자로 만든다.
        if (candidate.find(name)==std::string::npos) continue;
        std::int32_t previous{};ReadServerFields(static_cast<std::int32_t>(index),{},{},&previous);servers[index]=name+';'+description.data()+';'+std::to_string(preserveValue ? previous:value);return;
    }
    const auto formatted=name+';'+description.data()+';'+std::to_string(value);
    for (const auto& candidate:servers) if (::_stricmp(candidate.c_str(),formatted.c_str())==0) return; // flags4의 새 행 전체 중복을 제거한다.
    servers.push_back(formatted);RecalculateServerChecksum();
}
// 유효 인덱스 관문을 먼저 통과한 실제 현재 행만 다시 upsert한다.
void ClientNsInfoGlobals::SetServerValue(std::int32_t index,std::uint32_t value) {
    if (index<0 || static_cast<std::size_t>(index)>=servers.size()) return;std::array<char,80> endpoint{},description{};
    ReadServerFields(index,std::span(endpoint).first(79),std::span(description).first(79),nullptr);UpsertServer(std::string(endpoint.data())+';'+description.data()+';'+std::to_string(std::bit_cast<std::int32_t>(value)),false);
}
void ClientNsInfoGlobals::RecalculateServerChecksum() { // 원문은 구분자나 NUL을 더하지 않고 현재 모든 행을 다시 읽는다.
    serverListChecksum=0;for (const auto& row:servers) for (const auto byte:std::string_view(row).substr(0,row.find('\0'))) serverListChecksum+=static_cast<unsigned char>(byte)+1U; // DWORD 모듈러 합계를 보존한다.
}
}
