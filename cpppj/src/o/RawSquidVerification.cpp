// 원문 log의 저장 SID와 현재 TYPE 선택, 두 DWORD의 부분 효과를 구별한다.
#include "o/RawSquidVerification.h"
#include <array>
#include <bit>
#include <cstdio>
#include <stdexcept>

namespace netstorm::o { // 이 모듈은 객체·공간·자유 목록을 수정하지 않는다.
namespace { // 선택된 장치와 원문 로그 서식을 공유한다.
template<class F> F NeedVerification(const F& function) { if (!function) throw std::logic_error("객체 검증의 선택된 실제 장치 미연결");return function; } // 콜백이 자기 연결을 바꿔도 현재 호출을 유지한다.
Sid VerificationSid(const SidPool& pool,std::uint32_t value) { if (value>65535U || value>=pool.Capacity()) throw std::out_of_range("객체 검증의 실제 SID 물리 범위");return Sid{static_cast<std::uint16_t>(value)}; } // DWORD 순회 위치를 물리 검사 전에 WORD로 자르지 않는다.
std::uint32_t VerificationPacketSid(std::span<const std::uint8_t> packet) { return std::uint32_t(packet[4])|(std::uint32_t(packet[5])<<8); } // 원문 WORD는 zero-extend한다.
void VerificationLog(const SidPool& pool,const SquidVerificationHooks& hooks,std::uint32_t value,std::string_view field,std::uint32_t serverValue,std::uint32_t clientValue) { // 로그 인자는 TYPE 이름 조회 전에 저장된 값이다.
    const auto name=NeedVerification(hooks.typeName)(VerificationSid(pool,value));std::array<char,512> message{};
    const auto count=std::snprintf(message.data(),message.size(),"VERIFY %u (client type=%s): %s %u != %u\n",value,name.c_str(),std::string(field).c_str(),serverValue,clientValue);if (count<0 || static_cast<std::size_t>(count)>=message.size()) throw std::length_error("객체 검증의 실제 로그 범위");NeedVerification(hooks.log)(0x1d,{message.data(),static_cast<std::size_t>(count)});
}
}
void AdvanceSquidVerification(const SidPool& pool,SquidVerificationState state,const SquidVerificationHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("검증 간격의 PATCH SID 순회 계약");
    if (!state.previous) { state.previous=state.sid;return; }
    while (state.previous!=state.sid) { // 원문은 다음 SID에서 시작하고 현재 검증 SID 자체는 조사하지 않는다.
        ++state.previous;if (std::bit_cast<std::int32_t>(state.previous)>=65535) state.previous=15000; // PE의 signed jl과 DWORD 증가/감김을 보존한다.
        if (state.previous==state.sid) return;
        const auto raw=pool.Slot(VerificationSid(pool,state.previous));const auto freeOrForm=(raw[11]&1U) || raw[10]<70;
        if (!freeOrForm) VerificationLog(pool,hooks,state.previous,"on server is free||form, but not on client",1,0);
        if (raw[10]) VerificationLog(pool,hooks,state.previous,"server type != client type",0,raw[10]); // 앞선 로그 뒤에도 저장한 슬롯의 현재 TYPE를 비교한다.
    }
}
void BeginSquidVerification(const SidPool& pool,std::span<const std::uint8_t> packet,SquidVerificationState state,const SquidVerificationHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("검증 시작의 PATCH 계약");
    if (packet.size()<6) throw std::out_of_range("검증 시작의6BYTE 부족");if (state.sid) NeedVerification(hooks.assertion)("!sidToVerify",".\\Clientboard.cpp",0x4c);
    state.sid=VerificationPacketSid(packet);AdvanceSquidVerification(pool,state,hooks);
}
void ReceiveFreeSquidVerification(const SidPool& pool,std::span<const std::uint8_t> packet,SquidVerificationState state,const SquidVerificationHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("자유 객체 검증의 PATCH 계약");
    if (packet.size()<7) throw std::out_of_range("자유 객체 검증의7BYTE 부족");const auto incoming=VerificationPacketSid(packet);const auto raw=pool.Slot(VerificationSid(pool,incoming));state.sid=incoming;
    if (!(raw[11]&1U)) VerificationLog(pool,hooks,incoming,"isFree",1,raw[11]&1U);
    if (packet[6]!=raw[10]) VerificationLog(pool,hooks,state.sid,"freeType",packet[6],raw[10]);
    AdvanceSquidVerification(pool,state,hooks);state.sid=0;
}
}
