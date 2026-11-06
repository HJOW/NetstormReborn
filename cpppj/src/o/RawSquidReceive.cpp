// 원문 수신의 부분 효과와 콜백 뒤 재조회 순서를 유지한다.
#include "o/RawSquidReceive.h"
#include "o/RawSquidHitPoints.h"
#include <bit>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace netstorm::o { // 패킷 비정렬 필드를 같은 물리 풀에만 쓴다.
namespace { // 선택된 장치와 원문 폭을 처리하는 지역 도우미다.
template<class F> F NeedReceive(const F& value) { if (!value) throw std::logic_error("객체 수신의 선택된 실제 장치 미연결");return value; } // 콜백 중 장치 교체에도 현재 호출을 보존한다.
std::uint32_t ReceiveRead(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) { std::uint32_t value{};for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[offset+i])<<(i*8);return value; } // 반복은 낮은BYTE부터 해당 필드만 읽는다.
void ReceivePut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) { for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(i*8)); } // 반복은 주변 payload를 보존한다.
std::span<std::uint8_t> ReceiveRaw(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 예약/free 여부를 선행 필터로 추가하지 않는다.
std::uint16_t ReceiveCoordinate(double value) { if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("객체 수신 좌표의 실제 정수 범위");return static_cast<std::uint16_t>(static_cast<std::int32_t>(value)); } // 유효 표시 좌표의 ftol 하위WORD를 쓴다.
}
void ReceiveRawSquid(SidPool& pool,std::span<const std::uint8_t> packet,SquidReceiveState state,const SquidReceiveHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || packet.size()<35) throw std::invalid_argument("객체 수신의 PATCH35BYTE 물리 계약");
    const Sid incoming{static_cast<std::uint16_t>(ReceiveRead(packet,4,2))};
    const auto assertion=[&](std::string_view expression,std::uint32_t line) { NeedReceive(hooks.assertion)(expression,".\\Clientboard.cpp",line); }; // 현재 진단 정상 복귀 뒤에만 다음 명령을 실행한다.
    if (!state.verifySid) {
        const auto selectedType=packet[16];const auto oldState=pool.Slot(incoming)[11];state.typeWas=selectedType;
        const auto object=NeedReceive(hooks.take)(selectedType,incoming);auto raw=ReceiveRaw(pool,object);
        ReceivePut(raw,14,ReceiveRead(packet,6));ReceivePut(raw,18,ReceiveRead(packet,10));
        ReceivePut(raw,22,ReceiveCoordinate(double(std::bit_cast<float>(ReceiveRead(raw,14)))*16.0+0.5),2);ReceivePut(raw,24,ReceiveCoordinate(double(std::bit_cast<float>(ReceiveRead(raw,18)))*11.0+0.5),2); // PE503358/50334c의16/11과503350의0.5를 적용한다.
        NeedReceive(hooks.word)(object,static_cast<std::uint16_t>(ReceiveRead(packet,14,2)));
        ReceivePut(raw,42,ReceiveRead(packet,19));ReceivePut(raw,46,ReceiveRead(packet,23));raw[11]=packet[17];
        NeedReceive(hooks.owner)(object,packet[18]);raw[35]=packet[29];ReceivePut(raw,36,ReceiveRead(packet,30));
        const auto extra=static_cast<std::uint8_t>(packet[34]^((raw[40]^packet[34])&4U));raw[40]=static_cast<std::uint8_t>(((raw[40]^extra)&2U)^extra); //004acec0은 기존2/4비트만 보존한다.
        if (NeedReceive(hooks.flags1)(selectedType)&0x10U) SetRawSquidHitPoints(pool,object,std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReceiveRead(packet,27,2))));
        const auto receivedState=raw[11];raw[11]|=4;std::uint32_t flags=8;
        if ((receivedState&0x10U) || (oldState&1U)) {
            flags=NeedReceive(hooks.firstPop)(object)|8U;raw[11]&=0xefU;
            const auto ownerMask=1U<<(raw[34]&31U);
            if (!NeedReceive(hooks.hasTechnology)(raw[34],raw[10])) {
                if (raw[10]>=state.noMakeWarnings.size()) throw std::out_of_range("객체 수신의 실제 경고 표 범위");
                if (!(state.noMakeWarnings[raw[10]]&ownerMask)) { NeedReceive(hooks.noMake)(object);if (raw[10]>=state.noMakeWarnings.size()) throw std::out_of_range("Tell 뒤 현재 경고 TYPE 범위");state.noMakeWarnings[raw[10]]|=ownerMask; }
                NeedReceive(hooks.markNoMake)(object);
            }
        }
        if (raw[11]&8U) assertion("!(s->isContained())",0xc5);
        if (!(raw[11]&8U)) NeedReceive(hooks.pop)(object,std::bit_cast<float>(ReceiveRead(packet,6)),std::bit_cast<float>(ReceiveRead(packet,10)),flags);
        else NeedReceive(hooks.containedPop)(object,Sid{static_cast<std::uint16_t>(ReceiveRead(packet,6)&0xffffU)},flags);
        state.typeWas=0;return;
    }
    if (incoming.value!=state.verifySid) assertion("zus->sid == sidToVerify",0x8d);
    const auto raw=pool.Slot(incoming); // 원문은 진단 뒤 packet SID 슬롯을 보관하고 로그 TYPE는 현재 verify SID를 쓴다.
    const auto mismatch=[&](std::string_view field,std::uint32_t serverValue,std::uint32_t clientValue) { // 각 항목의 현재 verify SID/TYPE를 로그 장치 전에 저장한다.
        if (serverValue==clientValue) return;const auto current=state.verifySid;if (current>65535U || current>=pool.Capacity()) throw std::out_of_range("검증 로그의 현재 SID 범위");const auto name=NeedReceive(hooks.typeName)(Sid{static_cast<std::uint16_t>(current)});
        std::array<char,512> message{};const auto count=std::snprintf(message.data(),message.size(),"VERIFY %u (client type=%s): %s %u != %u\n",current,name.c_str(),std::string(field).c_str(),serverValue,clientValue);if (count<0 || static_cast<std::size_t>(count)>=message.size()) throw std::length_error("객체 검증 로그 범위");NeedReceive(hooks.log)(0x1d,{message.data(),static_cast<std::size_t>(count)});
    };
    mismatch("type",packet[16],raw[10]);mismatch("playerId",packet[18],raw[34]);mismatch("freeFlag",packet[17]&1U,raw[11]&1U);
    mismatch("deadFlag",packet[17]&2U,(raw[11]>>1)&1U);mismatch("containedFlag",packet[17]&8U,(raw[11]>>3)&1U); // 원문은 packet 마스크를 bool로 줄이지 않는다.
    state.verifySid=0;
}
}
