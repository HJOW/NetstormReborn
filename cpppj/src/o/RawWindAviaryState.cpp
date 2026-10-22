// 원본은 전송 중 double을 계산하지 않고 비트를 복사한다. NaN payload와 음수0도 그대로 유지한다.
#include "o/RawWindAviaryState.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::size_t kDirty=8,kTime=0x10,kStage=0x18,kForm=0x3c,kParent=0x40; // 실제80BYTE 본체의 독립 필드 위치다.
constexpr std::array<std::size_t,5> kOffsets{0x10,0x18,0x20,0x28,0x30}; // dirty2/4/8/10/20에 대응하는 원본 필드 순서다.
constexpr std::array<std::size_t,5> kSizes{8,4,8,8,4}; // 각 선택 필드의 실제 복사 폭이다.
// 정렬되지 않은 원문에서 DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    // 정확4바이트를 읽고 주변 padding을 해석하지 않는다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(bytes[at+index])<<(8*index);
    return value;
}
// 원본 WORD/DWORD/DOUBLE 저장의 낮은 바이트부터 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint64_t value,std::size_t size=4) {
    // 요청한 폭만 쓰고 caller의 인접 원문을 유지한다.
    for (std::size_t index=0;index<size;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 실제 선택 비트만 길이에 포함한다. 미사용 비트는 헤더에서 그대로 보존한다.
std::size_t PayloadSize(std::uint8_t dirty) {
    std::size_t size{};
    // 다섯 필드의 원본 비트와 폭을 독립 순서로 더한다.
    for (std::size_t index=0;index<kSizes.size();++index) if (dirty&(2U<<index)) size+=kSizes[index];
    return size;
}
}
// 생성자는0x1c/0x30/0x34/inner 꼬리를 초기화하지 않는다. 전체0 초기화로 대신하지 않는다.
void RawWindAviaryState::Initialize(WindAviaryProcessFields& fields,bool server,double now) {
    auto& raw=fields.bytes;Put(raw,kTime,0,8);Put(raw,0x20,0,8);Put(raw,0x28,0,8);raw[kDirty]=0xff;
    Put(raw,0x38,0x5019ec);Put(raw,kParent,0);Put(raw,kForm,0);Put(raw,0,0x509dfc);Put(raw,0x38,0x509ddc);
    if (server) { Put(raw,kTime,std::bit_cast<std::uint64_t>(now),8);raw[kDirty]|=4;Put(raw,kStage,1); }
}
// 강제 전송의 부모 선택은 inner thunk가 먼저 수행하고 본문은 실제 비제로 부모만 삽입한다.
std::uint16_t RawWindAviaryState::WriteProcess(WindAviaryProcessFields& fields,std::span<std::uint8_t> packet,
    bool includeParent,bool force,std::uint8_t protocol,std::uint8_t message) {
    auto& raw=fields.bytes;const auto parent=includeParent || force ? Read(raw,kParent) : 0U;
    const auto dirty=force ? std::uint8_t{0xff} : raw[kDirty];const auto size=9U+(parent ? 4U : 0U)+PayloadSize(dirty);
    if (packet.size()<size) throw std::out_of_range("데빌메이커 상태 전송 버퍼 부족");
    if (force) raw[kDirty]=0xff;packet[0]=protocol;packet[1]=message;Put(packet,4,Read(raw,kForm));std::size_t at=9;
    if (parent) { raw[kDirty]|=1;Put(packet,at,parent);at+=4; }
    // native writer가 읽는 각 선택 필드를 계산 없이 현재 원문에서 복사한다.
    for (std::size_t index=0;index<kSizes.size();++index) if (raw[kDirty]&(2U<<index)) {
        std::copy_n(raw.begin()+static_cast<std::ptrdiff_t>(kOffsets[index]),kSizes[index],packet.begin()+static_cast<std::ptrdiff_t>(at));at+=kSizes[index];
    }
    Put(packet,2,at,2);packet[8]=raw[kDirty];raw[kDirty]=0;return static_cast<std::uint16_t>(at);
}
// 잘린 host 버퍼는 첫 쓰기 전에 거부한다. 정상 원문의 부분 갱신과 부모 출력0은 그대로 따른다.
std::uint32_t RawWindAviaryState::ReadProcess(WindAviaryProcessFields& fields,std::span<const std::uint8_t> packet) {
    if (packet.size()<9) throw std::out_of_range("데빌메이커 상태 수신 머리 부족");
    const auto dirty=packet[8];const auto size=9U+((dirty&1U) ? 4U : 0U)+PayloadSize(dirty);
    if (packet.size()<size) throw std::out_of_range("데빌메이커 상태 수신 필드 부족");
    std::size_t at=9;std::uint32_t parent{};if (dirty&1U) { parent=Read(packet,at);at+=4; }
    // 선택되지 않은 상태·예약 시각·목표·padding은 caller 원문 그대로 보존한다.
    for (std::size_t index=0;index<kSizes.size();++index) if (dirty&(2U<<index)) {
        std::copy_n(packet.begin()+static_cast<std::ptrdiff_t>(at),kSizes[index],fields.bytes.begin()+static_cast<std::ptrdiff_t>(kOffsets[index]));at+=kSizes[index];
    }
    return parent;
}
}
