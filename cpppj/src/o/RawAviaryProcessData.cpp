// 계산 없는8/4BYTE 복사와 원문 선택 순서를 유지한다.
#include "o/RawAviaryProcessData.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o { // 두48BYTE 클래스의 공통 접두만 처리한다.
std::uint32_t ReadAviaryProcessData(std::span<std::uint8_t> fields,std::span<const std::uint8_t> packet) {
    if (packet.size()<9) throw std::out_of_range("아비어리 수신의 실제 접두 물리 범위");
    const auto dirty=packet[8];std::uint32_t parent{};std::size_t offset=9;
    const auto require=[&](std::size_t width) { if (offset>packet.size() || width>packet.size()-offset) throw std::out_of_range("아비어리 수신의 선택 필드 부족"); }; // 앞선 부분 쓰기는 되돌리지 않는다.
    const auto copy=[&](std::size_t destination,std::size_t width) { require(width);if (destination>fields.size() || width>fields.size()-destination) throw std::out_of_range("아비어리 수신의 실제 객체 필드 미연결");std::copy_n(packet.begin()+offset,width,fields.begin()+destination);offset+=width; }; // NULL this는 실제 선택된 저장에서만 호스트 오류다.
    if (dirty&1U) { require(4);for (std::size_t i=0;i<4;++i) parent|=std::uint32_t(packet[offset+i])<<(8*i);offset+=4; } // 반복은 별도 부모 출력DWORD만 조립한다.
    if (dirty&2U) copy(16,8);
    if (dirty&4U) copy(24,4);
    return parent;
}
}
