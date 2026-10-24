// NSInfo가 읽는 실제 현재 전역과 서버 이름 버퍼를 Client 수명에 보관한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace netstorm::client {
struct ClientNsInfoGlobals {
    std::uint32_t current{},zone{}; //593150/5467b8의 현재 NSInfo 포인터 토큰과 zone DWORD다.
    std::int32_t serverSelection{}; //54db4c의 BSS0이며 Interpret Options의 curServer가 갱신한다.
    std::array<char,80> serverName{}; //594970의 실제 공유80BYTE이며 무효 서버 조회는 기존 바이트를 보존한다.
    std::vector<std::string> servers; //532714/71c의 실제 읽기 전용 시작 서버 행이다. 네트워크 편집/저장 부모는 별도다.
    std::uint8_t nsKind{6},playerKind{37},battleKind{60}; //540421/440/457의 보호10.78 초기 종류 BYTE다.
    std::uint32_t zoneFormat{0x50c894}; //540bac의 실제 초기 형식 문자열 포인터 토큰이다.
    void LoadServerNames(std::span<const std::uint8_t> file); // 시작 서버 파일의 현재 행을 읽는다. 보호 파일에 쓰지 않는다.
    std::string_view ResolveServerName(const std::function<std::string(std::string_view)>& translate); //47c430→유효 행58f90/58e70의 현재 이름 출력 경계다.
};
}
