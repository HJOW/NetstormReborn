// 현재 서버 이름 조회의 LAN 번역/무효 조회 보존/유효 행의80BYTE 초기화를 구별한다.
#include "client/ClientNsInfoGlobals.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
namespace netstorm::client {
// 현재 보호 파일은 짧은 두 행이다. 미복원 긴 원본 line-reader 경계를 임의 분할하지 않는다.
void ClientNsInfoGlobals::LoadServerNames(std::span<const std::uint8_t> file) {
    std::istringstream input(std::string(reinterpret_cast<const char*>(file.data()),file.size()));std::vector<std::string> loaded;std::string line;
    while (std::getline(input,line)) { // Native 시작 목록의 비어 있지 않은 행 순서를 유지한다.
        if (line.size()>256) throw std::length_error("시작 서버 목록의 긴 원본 행 미연결");line.erase(std::remove(line.begin(),line.end(),'\r'),line.end());if (!line.empty()) loaded.push_back(line);
    }
    servers=std::move(loaded);
}
// 원본은 LAN일 때 strcpy하고 그 외에는 유효 목록 항목에서만 출력 버퍼를 지운다.
std::string_view ClientNsInfoGlobals::ResolveServerName(const std::function<std::string(std::string_view)>& translate) {
    if (serverSelection==-1) {
        if (!translate) throw std::logic_error("현재 LAN 서버 이름 번역 미연결");const auto value=translate("LAN Server");const auto size=value.find('\0')==std::string::npos ? value.size() : value.find('\0');
        if (size>=serverName.size()) throw std::length_error("LAN 서버 이름의 원본80BYTE 경계 초과");std::copy_n(value.data(),size,serverName.data());serverName[size]=0;
    } else if (serverSelection>=0 && static_cast<std::size_t>(serverSelection)<servers.size()) {
        serverName.fill(0);const auto& record=servers[static_cast<std::size_t>(serverSelection)];const auto value=std::string_view(record).substr(0,record.find('\0'));const auto first=value.find(';');
        if (first!=std::string_view::npos) { const auto end=value.find(';',first+1);const auto name=value.substr(first+1,end==std::string_view::npos ? end : end-first-1);std::copy_n(name.data(),std::min(name.size(),serverName.size()-1),serverName.data()); }
    }
    return {serverName.data(),static_cast<std::size_t>(std::find(serverName.begin(),serverName.end(),'\0')-serverName.begin())};
}
}
