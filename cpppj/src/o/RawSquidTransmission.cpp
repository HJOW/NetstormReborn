// PE004ad170의35BYTE 쓰기와00432fe0의압축을 복원하며 단독 캠페인에서도 writer를 생략하지 않는다.
#include "o/RawSquidTransmission.h"
#include "o/SquidFactory.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace netstorm::o { // 원문 주소는 분배 메타데이터이며 C++ 함수 포인터로 실행하지 않는다.
namespace { // 정확한 원문 폭/현재 장치/헤더 경계다.
template<class F> F WireNeed(const F& function) { if (!function) throw std::logic_error("Squid 전송의 선택된 실제 장치 미연결");return function; } // 실행 중 자기 연결 교체에도 현재 callable을 보존한다.
std::uint32_t WireRead(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) { std::uint32_t value{};for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);return value; } // 선택한 낮은 바이트부터 조립한다.
void WirePut(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width) { for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); } // 원문 WORD/DWORD 밖은 보존한다.
template<class Header> void WireHeader(std::span<std::uint8_t> packet,Header header) { packet[0]=header.protocol&0x7f;packet[1]=header.message;WirePut(packet,2,std::max<std::uint32_t>(4,header.length),2); } // erase0인004df590의 실제 세 필드만 쓴다.
bool WireByteCoordinate(float value) { return std::isfinite(value) && value>=0 && value<=255 && value==std::trunc(value); } //4328c0의정수변환/8비트/float동일 비교다. 범위 밖 ftol 미정 비트는 통과하지 않는다.
}
RawSquidTransmission::RawSquidTransmission(const SidPool& pool,RawSquidTransmissionState& state,RawSquidTransmissionHooks hooks):pool_(pool),state_(state),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("현재 Squid 전송은10.78 전용"); } // CD writer를 패치 레이아웃으로 해석하지 않는다.
std::int32_t RawSquidTransmission::Write(Sid sid,std::uint32_t writerArgument) const { // 현재+54의 공통/빈 writer·두 Form·실제 ProcessForm을 분배한다.
    const auto raw=pool_.Slot(sid);const auto table=WireRead(raw,0,4);
    if (table==0x512098U) return WireNeed(hooks_.processPacket)(sid,state_.buffer,writerArgument); //004ae360은 부모 진단 뒤 같은 PID 객체의 실제 writer를 부른다.
    if (table==0x5092d0U) return 0; // 보호83VT중이표의+54만00455ae0의실제0반환이다.
    if (!SquidFactory::SupportsConstructedVtable(pool_.Edition(),table)) {
        RawFormPacket forms(pool_,state_.forms,state_.deletion);return forms.WriteData(sid,state_.buffer); // 다른현재VT는기존writer의명시실패를유지한다.
    }
    auto& packet=state_.buffer;WireHeader(packet,state_.update);WirePut(packet,4,sid.value,2);
    std::copy_n(raw.begin()+14,8,packet.begin()+6);std::copy_n(raw.begin()+12,2,packet.begin()+14);
    packet[16]=raw[10];packet[17]=raw[11];packet[18]=raw[34];
    std::copy_n(raw.begin()+42,8,packet.begin()+19);std::copy_n(raw.begin()+26,2,packet.begin()+27);packet[29]=raw[35];std::copy_n(raw.begin()+36,4,packet.begin()+30);packet[34]=raw[40];
    if (!state_.compress) return 35;
    std::array<std::uint8_t,35> saved{};std::copy_n(packet.begin(),saved.size(),saved.begin()); // 같은버퍼의압축출력전에원문35BYTE를보관한다.
    return Compress(saved);
}
std::int32_t RawSquidTransmission::Compress(std::span<const std::uint8_t> input) const { // 원문 encoder의0..15 순서를 지키며14/15는 무효과다.
    const auto& type=WireNeed(hooks_.type)(input[16]);std::array<std::uint8_t,64> data{};std::size_t count{};std::uint32_t mask{};
    const auto append=[&](std::size_t offset,std::size_t width,std::uint32_t field) { if (count+width>data.size()) throw std::out_of_range("압축Squid의원문64BYTE범위");std::copy_n(input.begin()+offset,width,data.begin()+count);count+=width;mask|=1U<<field; }; // 각선택필드의낮은BYTE/WORD만기록한다.
    const auto x=std::bit_cast<float>(WireRead(input,6,4)),y=std::bit_cast<float>(WireRead(input,10,4));
    if (WireByteCoordinate(x) && WireByteCoordinate(y)) { data[count++]=static_cast<std::uint8_t>(x);data[count++]=static_cast<std::uint8_t>(y);mask|=1; }
    else append(6,8,1);
    const auto quantity=WireRead(input,14,2);if (quantity) append(14,quantity<=255 ? 1U : 2U,quantity<=255 ? 2U : 3U);
    if (input[16]!=static_cast<std::uint8_t>(WireNeed(hooks_.defaultType)())) append(16,1,4);
    if (input[17]&0xefU) append(17,1,5);
    if (input[17]&0x10U) mask|=0x40;
    if (state_.sameOwnerEnabled && state_.lastOwner!=0xffffff9dU && input[18]==state_.lastOwner) mask|=0x80;
    else { append(18,1,8);state_.lastOwner=input[18]; }
    if (type.flags1&0x10U) { const auto hp=WireRead(input,27,2);if (std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(hp))!=WireNeed(hooks_.maxHp)(input[16])) append(27,hp<=255 ? 1U : 2U,hp<=255 ? 9U : 10U); }
    if (static_cast<std::int32_t>(std::bit_cast<std::int8_t>(input[29]))!=type.zOrder) append(29,1,11);
    if (WireRead(input,30,4)) append(30,1,12);
    append(34,1,13);
    auto& packet=state_.buffer;WireHeader(packet,state_.compressed);WirePut(packet,4,WireRead(input,4,2),2);WirePut(packet,6,mask,2);std::copy_n(data.begin(),count,packet.begin()+9);packet[8]=static_cast<std::uint8_t>(count);
    const auto length=static_cast<std::uint32_t>(count)+9; ++state_.compressedCount;state_.compressedBytes+=length;
    if ((state_.compressedCount&0xffU)==0xffU) {
        const auto numerator=std::bit_cast<std::int32_t>(state_.compressedBytes*100U),denominator=std::bit_cast<std::int32_t>(state_.compressedCount*35U);if (!denominator || (numerator==INT32_MIN && denominator==-1)) throw std::out_of_range("압축Squid누적비율의원문idiv범위");
        std::array<char,80> text{};const auto size=std::snprintf(text.data(),text.size(),"compressedUpdateSquid %d%%\n",numerator/denominator);if (size<0 || static_cast<std::size_t>(size)>=text.size()) throw std::length_error("압축Squid로그범위");WireNeed(hooks_.trace)(4,{text.data(),static_cast<std::size_t>(size)});
    }
    return static_cast<std::int32_t>(length);
}
void RawSquidTransmission::Transmit(Sid sid,std::int32_t target,std::uint32_t writerArgument) const { //004ac260의 selector!=-2 경로이며 네 번째 flags는 이 분기에서 사용하지 않는다.
    if (!state_.clientSidAllowed && sid.value>=5 && sid.value<15000) WireNeed(hooks_.assertion)("!isClientSid()","\\Ns\\O\\Squid.cpp",0x2d8);
    const auto size=Write(sid,writerArgument);
    if (size>=2048) WireNeed(hooks_.assertion)("transmitSize < TRANSMIT_BUFFER_SIZE","\\Ns\\O\\Squid.cpp",0x2e3);
    if (size>0) {
        if (static_cast<std::size_t>(size)>state_.buffer.size()) throw std::out_of_range("Squid송신공통버퍼범위");const auto packet=std::span<const std::uint8_t>(state_.buffer).first(static_cast<std::size_t>(size));
        if (target==-1) WireNeed(hooks_.broadcast)(packet);else if (WireNeed(hooks_.connected)(target)) WireNeed(hooks_.send)(target,packet);
    }
}
// 기존 Pop의 일반 방송/인자0 계약은 같은 공통 몸체를 사용한다.
void RawSquidTransmission::Pop(Sid sid,std::uint32_t) const { Transmit(sid,-1,0); }
// 동기화의 대상 번호/인자1을 보존하며 호출 이후 현재 연결을 다시 조회한다.
void RawSquidTransmission::SendTo(Sid sid,std::int32_t target,std::uint32_t writerArgument) const { Transmit(sid,target,writerArgument); }
// 압축 모드가 같아도 원문처럼 owner 캐시를 항상 초기화한다.
void RawSquidTransmission::SameOwnerMode(std::uint32_t mode) const { state_.sameOwnerEnabled=mode;state_.lastOwner=0xffffff9dU; }
}
