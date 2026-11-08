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
void WriteRequestMenuPacket(std::span<std::uint8_t> packet,std::string_view section); //004d7136..7168의87h헤더→+22 GET/NUL→+4 섹션 strcpy를보존하며Zsend의세필드로대체하지않는다.
void SendRequestMenu(std::string_view section,const std::function<void(std::int32_t,std::span<std::uint8_t>)>& send); //현재일반입력의섹션을같은11번mutable송신에전달하고서버페이지를직접생성하지않는다.
}
