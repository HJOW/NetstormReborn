// 필드 생략 기본값·마스크 충돌 진단 후 진행과 owner 캐시의 원문 갱신 시점을 지킨다.
#include "o/RawSquidCompressedReceive.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 가변 데이터는 먼저 지역 복사하여 콜백 이후에도 같은 입력을 소비한다.
namespace { // 원문 폭과 선택된 장치를 공유한다.
template<class F> F NeedCompressed(const F& function) { if (!function) throw std::logic_error("압축 객체 수신의 선택된 실제 장치 미연결");return function; } // 현재 콜백 수명을 보존한다.
std::uint32_t CompressedRead(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) { std::uint32_t value{};for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[offset+i])<<(i*8);return value; } // 반복은 같은 비정렬 필드만 읽는다.
void CompressedPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) { for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(i*8)); } // 반복은 인접 기본값을 보존한다.
}
std::array<std::uint8_t,35> DecodeCompressedSquid(std::span<const std::uint8_t> packet,RawSquidTransmissionState& state,const SquidCompressedReceiveHooks& hooks) {
    if (packet.size()<9) throw std::out_of_range("압축 객체의9BYTE 머리 부족");const auto count=packet[8];if (count>64 || packet.size()<std::size_t(count)+9) throw std::out_of_range("압축 객체의 실제64BYTE 지역 복사 범위");
    std::array<std::uint8_t,64> data{};std::copy_n(packet.begin()+9,count,data.begin());const auto mask=CompressedRead(packet,6,2);std::size_t index{};
    std::array<std::uint8_t,35> image{};image[0]=state.update.protocol&0x7fU;image[1]=state.update.message;CompressedPut(image,2,state.update.length,2);CompressedPut(image,4,CompressedRead(packet,4,2),2);
    CompressedPut(image,6,0xc61c4000U,4);CompressedPut(image,10,0xc61c4000U,4);image[16]=static_cast<std::uint8_t>(NeedCompressed(hooks.defaultType)());image[18]=static_cast<std::uint8_t>(state.lastOwner); //−10000 좌표·수량/추가/HP/프레임/extra0과 현재 캐시의 생성자 기본값이다.
    const auto diagnose=[&](std::string_view expression,std::uint32_t line) { NeedCompressed(hooks.assertion)(expression,".\\Clientboard.cpp",line); }; // 진단 정상 복귀 뒤 다음 필드도 처리한다.
    const auto copy=[&](std::size_t destination,std::size_t width,bool byteRead=false) { //32900/32960의 실제 소비 순서다. 복사되지 않은 Native 스택 바이트는 합성하지 않는다.
        if (index+width>count) { diagnose("index <= dataLength",byteRead ? 0x128U : 0x137U);throw std::out_of_range("압축 객체의 미정의 지역 스택 읽기"); }
        std::copy_n(data.begin()+index,width,image.begin()+destination);index+=width;
    };
    for (std::uint32_t field=0;field<16;++field) { //434080은 마스크 유무와 무관하게0..15를 순서대로 호출한다.
        if (mask&(1U<<field)) switch (field) {
        case 0: {
            if (mask&2U) diagnose("!testMask( i+1 )",0x14f);copy(6,1,true);const auto x=image[6];CompressedPut(image,6,std::bit_cast<std::uint32_t>(static_cast<float>(x)),4);copy(10,1,true);const auto y=image[10];CompressedPut(image,10,std::bit_cast<std::uint32_t>(static_cast<float>(y)),4);break;
        }
        case 1: if (mask&1U) diagnose("!testMask( i-1 )",0x15c);copy(6,8);break;
        case 2: if (mask&8U) diagnose("!testMask( i+1 )",0x169);copy(14,1);break;
        case 3: if (mask&4U) diagnose("!testMask( i-1 )",0x174);copy(14,2);break;
        case 4: copy(16,1);break;
        case 5: copy(17,1);break;
        case 6: image[17]|=0x10U;break;
        case 7: if (mask&0x100U) diagnose("!testMask( i+1 )",0x1af);if (state.lastOwner==0xffffff9dU) diagnose("hackySameAsLastPlayerId != CRAZY_PLAYER_ID",0x1b0);image[18]=static_cast<std::uint8_t>(state.lastOwner);break;
        case 8: if (mask&0x80U) diagnose("!testMask( i-1 )",0x1bd);copy(18,1);state.lastOwner=image[18];break;
        case 9: if (mask&0x400U) diagnose("!testMask( i+1 )",0x1cb);copy(27,1);break;
        case 10: if (mask&0x200U) diagnose("!testMask( i-1 )",0x1d6);copy(27,2);break;
        case 11: copy(29,1);break;
        case 12: copy(30,1);break;
        case 13: copy(34,1);break;
        default: break; // 원문14/15는 데이터를 소비하지 않는다.
        }
        if (field==4) { // 비트4가 생략되어도 같은 default TYPE의 HP/zOrder를 채운다.
            const auto& selected=NeedCompressed(hooks.type)(image[16]);if (selected.flags1&0x10U) CompressedPut(image,27,static_cast<std::uint32_t>(NeedCompressed(hooks.maximumHp)(selected)),2);image[29]=static_cast<std::uint8_t>(selected.zOrder);
        }
    }
    return image;
}
void ObserveCompressedSquid(std::span<std::uint8_t> packet,std::uint32_t sending,const std::function<void(std::string_view,std::string_view,std::uint32_t)>& assertion) { // 수신 관찰에서는 어떤 필드도 읽거나 바꾸지 않는다.
    if (!sending) return;if (packet.size()<9) throw std::out_of_range("압축 객체 송신 관찰기의 머리 부족");if (packet[8]==0xffU) NeedCompressed(assertion)("dataLength != (unsigned char)-1","\\Ns\\O\\_p_packets.cpp",0xb83);
    CompressedPut(packet,2,std::uint32_t(packet[8])+9,2); // 진단 뒤 현재 길이를 다시 읽는다.
}
}
