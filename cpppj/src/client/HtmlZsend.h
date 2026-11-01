// HTML Zsend의 세 원시 문자열을 원문87h 패킷과 현재11번 송신기로 전달한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { //구 root 서비스 연결을 새로 만들지 않는 디컴파일 기반 사전 구현이다.
inline constexpr std::size_t kHtmlZsendPacketSize=0x112; //0040a150의 실제 선언길이274BYTE다.
void WriteHtmlZsendPacket(std::span<std::uint8_t> packet,std::string_view first,std::string_view second,std::string_view third); //헤더4BYTE와+22/+72/+c2의각NUL문자열만쓰며나머지caller바이트는보존한다.
void SendHtmlZsend(std::string_view first,std::string_view second,std::string_view third,
    const std::function<void(std::int32_t,std::span<std::uint8_t>)>& send); //작성뒤004e2dc0(11,packet)의같은mutable송신을실행한다.새MP검사는추가하지않는다.
}
