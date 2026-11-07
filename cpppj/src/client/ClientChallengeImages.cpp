// 같은 CS 포인터 토큰으로 관리하는 Image의 깊은 복사와 현재 두 Animator TYPE를 제공한다.
#include "client/ClientChallengeGlobals.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 그림 버퍼는 기존 CS 가변 할당 소유자에만 등록한다.
namespace { // 포인터를 호스트 주소로 자르지 않고 실제 토큰과 바이트를 사용한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("CS Image의 실제 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } // 선택한 비정렬 필드만 낮은 BYTE부터 읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("CS Image의 실제 쓰기 범위");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } // 원문 폭만 기록한다.
void Copy(std::span<std::uint8_t> target,std::span<const std::uint8_t> source,std::uint32_t length) { std::size_t at{};for (std::uint32_t word=length>>2;word;--word,at+=4) Put(target,at,Read(source,at));for (std::uint32_t byte=length&3U;byte;--byte,++at) Put(target,at,Read(source,at,1),1); } // DWORD 복사 뒤 남은 BYTE를 순서대로 복사하며 전체 선검사를 추가하지 않는다.
}
// 먼저 지운 뒤 현재 source를 읽으므로 자기 복사는 원문처럼 빈 이미지가 된다.
void ClientChallengeGlobals::CopyImage(std::span<std::uint8_t> target,std::span<const std::uint8_t> source,const std::function<void(std::string_view,std::uint32_t)>& assertion) {
    ClearArray(target);Put(target,8,Read(source,8));Put(target,0xc,Read(source,0xc));
    if (Read(source,0)) {
        const auto width=Read(source,8),height=Read(source,0xc);if (Read(target,0)) { if (!assertion) throw std::logic_error("Image.cpp 실제 진단 미연결");assertion("imageBits == NULL",0x37); }
        const auto length=width*height,token=AllocateArray(length);Put(target,0xc,height);Put(target,0,token);Put(target,8,width);const auto input=ArrayBytes(Read(source,0));const auto output=ArrayBytes(token);Copy(output,input,length);
    }
    if (Read(source,4)) { const auto length=Read(source,0x10);Put(target,0x10,length);if (std::bit_cast<std::int32_t>(length)>0) { const auto token=AllocateArray(length);const auto currentLength=Read(target,0x10);Put(target,4,token);const auto input=ArrayBytes(Read(source,4));const auto output=ArrayBytes(token);Copy(output,input,currentLength); } }
}
std::uint32_t& ClientChallengeGlobals::IslandAnimatorType() { return islandAnimatorType_; } // 현재 TYPE 주소를 그대로 빌린다.
const std::uint32_t& ClientChallengeGlobals::IslandAnimatorType() const { return islandAnimatorType_; } // 조회가 값을 초기화하지 않는다.
std::uint32_t& ClientChallengeGlobals::CameraAnimatorType() { return cameraAnimatorType_; } // 별도 카메라 TYPE 주소다.
const std::uint32_t& ClientChallengeGlobals::CameraAnimatorType() const { return cameraAnimatorType_; } // Session 수명 밖에서도 동일 주소다.
}
