// 가변 mod 바이트 뒤 contact의 첫 네 DWORD만 원문 위치/현재 길이대로 읽는다.
#include "client/NativeMetaGeometry.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //별도 CS 사본이나 sender/known/phase 관문을 만들지 않는다.
namespace { //원문의 비정렬 BYTE/WORD/DWORD와 선택된 진단 장치만 처리한다.
constexpr std::uint32_t kHeightBits=0x411ba2e9U; //PE00513780의 실제9.727272987365723 float이며 정수10/근삿값으로 대체하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { //현재 지정 폭만 낮은 BYTE부터 읽는다.
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타26h의 실제 필드 물리 부족");std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte); //선택하지 않은 mod/contact 바이트는 읽지 않는다.
    return value;
}
}
// 각 진단에서 복귀한 뒤 다음 필드와 최종 길이를 현재 패킷에서 다시 읽는다.
void ObserveNativeMetaGeometry(std::span<std::uint8_t> packet,bool sending,const std::function<void(std::string_view,std::uint32_t)>& assertion) {
    if (!sending) return;
    if (Read(packet,4,2)==0xffffU) { if (!assertion) throw std::logic_error("26h modLen 실제 진단 미연결");assertion("modLen != (unsigned short)-1",0x5eb); }
    if (Read(packet,6,1)==0xffU) { if (!assertion) throw std::logic_error("26h contactLen 실제 진단 미연결");assertion("contactLen != (unsigned char)-1",0x5ec); }
    const auto length=Read(packet,6,1)+Read(packet,4,2)+9U;if (packet.size()<4) throw std::out_of_range("26h 길이 WORD의 실제 물리 부족");packet[2]=static_cast<std::uint8_t>(length);packet[3]=static_cast<std::uint8_t>(length>>8);
}
// 원문 x87의 BYTE 높이+PE float는 DWORD 스택에 저장된 뒤497f80에 전달된다.
void ReceiveNativeMetaGeometry(std::span<const std::uint8_t> packet,o::ChallengeGeometry& geometry,std::uint32_t& mode,const std::function<void(float,float)>& limits) {
    const auto width=static_cast<float>(Read(packet,7,1));const auto height=static_cast<float>(static_cast<double>(Read(packet,8,1))+static_cast<double>(std::bit_cast<float>(kHeightBits)));
    if (!limits) throw std::logic_error("26h의 실제 Renderer 한계 장치 미연결");limits(width,height);mode=0;const auto count=Read(packet,6,1)>>2;if (count<=2) return;
    auto at=9U+Read(packet,4,2);geometry.originX=std::bit_cast<float>(Read(packet,at));geometry.originY=std::bit_cast<float>(Read(packet,at+4));at+=8;
    for (std::uint32_t index=0;index<2 && index<count-2U;++index,at+=4) { //첫 세 DWORD일 때 둘째 반경의 stale 값을 보존하고 넷 이상이어도 추가 contact는 읽지 않는다.
        const auto value=std::bit_cast<float>(Read(packet,at));if (index==0) geometry.firstRadius=value;else geometry.secondRadius=value;
    }
    mode=1;
}
}
