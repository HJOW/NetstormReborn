// 보호 R PE의 실제 필드 폭·진단 순서와 송신 길이 WORD 쓰기를 공유한다.
#include "root/PacketRegistration.h"
#include <bit>
#include <stdexcept>

namespace netstorm::root { // 원문 관찰기를 실행하는 전송/독립 등록이 같은 몸체를 사용한다.
namespace { // 비정렬 원문 필드를 호스트 실제 span 범위 안에서만 조작한다.
// 선택된 BYTE/WORD/DWORD만 읽으며 논리 길이와 물리 버퍼를 같게 강제하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> packet,std::size_t offset,std::size_t width) {
    if (offset>packet.size() || width>packet.size()-offset) throw std::out_of_range("R 관찰기 필드의 실제 물리 부족");std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(packet[offset+byte])<<(byte*8);return value; // 낮은 BYTE부터 실제 x86 폭으로 조립한다.
}
// 최종 하위 WORD만 헤더에 쓰고 나머지 패킷 물리를 유지한다.
void Length(std::span<std::uint8_t> packet,std::uint32_t value) { if (packet.size()<4) throw std::out_of_range("R 관찰기 헤더의 실제 물리 부족");packet[2]=static_cast<std::uint8_t>(value);packet[3]=static_cast<std::uint8_t>(value>>8); }
// 0F/16/7Bh에서 원문 signed BYTE의 음수 곱셈/덧셈도 DWORD 모듈러 합에 보존한다.
std::uint32_t SignedByte(std::span<const std::uint8_t> packet,std::size_t offset) { return static_cast<std::uint32_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(Read(packet,offset,1)))); }
}
// 원문 진단은 각 필드를 검사한 시점에 호출하고 길이 계산은 진단 복귀 뒤 필드를 새로 읽는다.
void ObservePacketFields(std::uint8_t type,std::span<std::uint8_t> packet,std::uint32_t sending,const PacketAssertion& assertion) {
    if (!sending) return;
    const auto check=[&](std::size_t offset,std::size_t width,std::uint32_t invalid,std::string_view expression,std::uint32_t line) { // 진단이 선택된 때만 실제 장치를 요구하며 현재 packet을 바꾼 복귀도 허용한다.
        if (Read(packet,offset,width)!=invalid) return;if (!assertion) throw std::logic_error("R 관찰기의 선택된 진단 장치 누락");assertion(expression,"c:\\ns\\o\\_p_packets.cpp",line);
    };
    switch (type) {
    case 0x0b:case 0x0c:check(8,4,0xffffffffU,"extraDataLen != (int)-1",type==0x0b ? 885:917);Length(packet,Read(packet,8,4)+12U);break;
    case 0x0f:
        check(12,4,0xffffffffU,"rootServerLen != (int)-1",949);check(10,1,255,"numChals != (char)-1",950);check(11,1,255,"numFriendsInChal != (char)-1",951);
        Length(packet,Read(packet,12,4)+20U+SignedByte(packet,10)*3U+SignedByte(packet,11)*5U);break;
    case 0x16:check(5,1,255,"numFriends != (char)-1",1109);Length(packet,SignedByte(packet,5)*4U+14U);break;
    case 0x1a:check(14,1,255,"numClients != (unsigned char)-1",1164);Length(packet,Read(packet,14,1)*71U+15U);break;
    case 0x1c:check(45,1,255,"nameLen != (unsigned char)-1",1223);Length(packet,Read(packet,45,1)+46U);break;
    case 0x20:check(18,1,255,"nameLen != (unsigned char)-1",1329);Length(packet,Read(packet,18,1)+53U);break;
    case 0x21:check(4,1,255,"numPlayers != (unsigned char)-1",1411);Length(packet,Read(packet,4,1)+5U);break;
    case 0x26:
        check(4,2,65535,"modLen != (unsigned short)-1",1512);check(6,1,255,"contactLen != (unsigned char)-1",1513);Length(packet,Read(packet,4,2)+9U+Read(packet,6,1));break;
    case 0x27:
        check(12,1,255,"len != (unsigned char)-1",1550);check(13,4,0xffffffffU,"fortFileSize != (int)-1",1551);Length(packet,Read(packet,12,1)+17U+Read(packet,13,4));break;
    case 0x2a:check(12,4,0xffffffffU,"fortFileSize != (int)-1",1647);Length(packet,Read(packet,12,4)+17U);break;
    case 0x2b:check(51,1,255,"nameLen != (unsigned char)-1",1682);Length(packet,Read(packet,51,1)+59U);break;
    case 0x30:check(13,2,65535,"nodeCount != (short)-1",1811);Length(packet,Read(packet,13,2)+24U);break;
    case 0x31:check(4,4,0xffffffffU,"len != (int)-1",1864);Length(packet,Read(packet,4,4)+8U);break;
    case 0x3f:check(26,1,255,"sidCount != (unsigned char)-1",1990);Length(packet,Read(packet,26,1)*2U+27U);break;
    case 0x41:
        check(10,2,65535,"nameLen != (short)-1",2074);check(12,2,0xffffffffU,"bufferLen != (short)-1",2075);check(14,1,255,"addrLen != (unsigned char)-1",2076); // 원문 movzx WORD와ffffffff 비교는 항상 거짓이지만 해당 필드 읽기 순서는 유지한다.
        Length(packet,Read(packet,10,2)+15U+Read(packet,12,2)+Read(packet,14,1)*4U);break; // 두 WORD의 부호 확장 차이는 최종 하위WORD에 영향을 주지 않는다.
    case 0x55:check(4,4,0xffffffffU,"count != (int)-1",2265);Length(packet,Read(packet,4,4)+8U);break;
    case 0x56:check(8,4,0xffffffffU,"dataLength != (int)-1",2293);Length(packet,Read(packet,8,4)+12U);break;
    case 0x57:check(8,4,0xffffffffU,"specLength != (int)-1",2323);Length(packet,Read(packet,8,4)+12U);break;
    case 0x5f:check(6,4,0xffffffffU,"count != (int)-1",2408);Length(packet,Read(packet,6,4)*2U+10U);break;
    case 0x62:check(4,4,0xffffffffU,"count != (int)-1",2473);Length(packet,Read(packet,4,4)*2U+8U);break;
    case 0x67:case 0x85:check(5,2,65535,"dataLen != (unsigned short)-1",type==0x67 ? 2564:2601);Length(packet,Read(packet,5,2)+7U);break;
    case 0x69:check(4,1,255,"len != (unsigned char)-1",2652);Length(packet,Read(packet,4,1)+5U);break;
    case 0x70:check(4,4,0xffffffffU,"tellLength != (int)-1",2756);Length(packet,Read(packet,4,4)+8U);break;
    case 0x71:check(9,1,255,"descLen != (unsigned char)-1",2784);Length(packet,Read(packet,9,1)+10U);break;
    case 0x72:check(6,4,0xffffffffU,"len != (int)-1",2816);Length(packet,Read(packet,6,4)+10U);break;
    case 0x75:check(16,4,0xffffffffU,"count != (int)-1",2848);Length(packet,Read(packet,16,4)*2U+20U);break;
    case 0x7a:check(8,1,255,"dataLength != (unsigned char)-1",2944);Length(packet,Read(packet,8,1)+9U);break;
    case 0x7b:check(9,1,255,"addrLen != (char)-1",2981);Length(packet,SignedByte(packet,9)+10U);break;
    case 0x7d:check(16,2,65535,"numSids != (unsigned short)-1",3031);Length(packet,Read(packet,16,2)*2U+22U);break;
    case 0x81:check(4,4,0xffffffffU,"numPlayers != (int)-1",3130);Length(packet,Read(packet,4,4)*4U+8U);break;
    default:break; // 실제 빈 관찰기/폐기7f·80은 필드·진단·길이 쓰기가 없다. NULL 등록 여부는 초기화 표가 구별한다.
    }
}
}
