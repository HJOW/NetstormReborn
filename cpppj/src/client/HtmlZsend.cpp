// 원문 메타의 생성자/strcpy/채널11 순서를 유지하고 원시 인자를 명령으로 재해석하지 않는다.
#include "client/HtmlZsend.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace netstorm::client { //실제연결선택과observer는현재전송장치가담당한다.
namespace { //원문고정80BYTE필드의정상NUL범위만사용한다.
void WriteHeader(std::span<std::uint8_t> packet) { //0040a150의현재87h/버전1/길이274헤더만쓰며payload와미작성꼬리를보존한다.
    if (packet.size()<kHtmlZsendPacketSize) throw std::out_of_range("HTML Zsend의274BYTE 실제 저장소 부족");packet[0]=0;packet[1]=0;packet[2]=0;packet[3]=0;packet[1]=0x87;packet[2]=0x12;packet[3]=1;packet[0]=1;
}
void CopyField(std::span<std::uint8_t> packet,std::size_t offset,std::string_view value) { //원문strcpy처럼첫NUL까지만복사한다.
    value=value.substr(0,value.find('\0'));if (value.size()>=80) throw std::length_error("HTML Zsend의 원문80BYTE 필드 범위");std::copy(value.begin(),value.end(),packet.begin()+static_cast<std::ptrdiff_t>(offset));packet[offset+value.size()]=0;
}
}
void WriteHtmlZsendPacket(std::span<std::uint8_t> packet,std::string_view first,std::string_view second,std::string_view third) { //PE0052ed58/59/5c의실제87/1/112초기값이다.
    WriteHeader(packet);
    CopyField(packet,0x22,first);CopyField(packet,0x72,second);CopyField(packet,0xc2,third);
}
void SendHtmlZsend(std::string_view first,std::string_view second,std::string_view third,const std::function<void(std::int32_t,std::span<std::uint8_t>)>& send) { //원문미정스택의prefix30BYTE/문자꼬리는호스트에서0으로시작한다.정의된필드writer는그영역을덮지않으며원문미정물리의동일성은보장하지않는다.
    std::array<std::uint8_t,kHtmlZsendPacketSize> packet{};WriteHtmlZsendPacket(packet,first,second,third);if (!send) throw std::logic_error("HTML Zsend의 실제 현재 송신 장치 미연결");send(11,packet);
}
void WriteRequestMenuPacket(std::span<std::uint8_t> packet,std::string_view section) { //원문은GET를먼저써서긴섹션복사가그필드에닿는부분효과도같은순서로남긴다.
    WriteHeader(packet);packet[0x22]='G';packet[0x23]='E';packet[0x24]='T';packet[0x25]=0;
    for (std::size_t index=0;;++index) { //strcpy의첫NUL까지각BYTE를읽고바로저장하며다른두Zsend필드는쓰지않는다.
        const auto value=index<section.size() ? static_cast<std::uint8_t>(section[index]):std::uint8_t{}; //명시문자열끝은원문NUL이며중간NUL뒤의바이트는소비하지않는다.
        if (4+index>=packet.size()) throw std::out_of_range("RequestMenu의실제패킷물리밖문자열복사");packet[4+index]=value;if (!value) break;
    }
}
void SendRequestMenu(std::string_view section,const std::function<void(std::int32_t,std::span<std::uint8_t>)>& send) { //원문미정스택의미작성바이트는기존Zsend와같은호스트0입력으로두며writer가정의한필드만변경한다.
    std::array<std::uint8_t,kHtmlZsendPacketSize> packet{};WriteRequestMenuPacket(packet,section);if (!send) throw std::logic_error("RequestMenu의실제현재송신장치미연결");send(11,packet);
}
}
