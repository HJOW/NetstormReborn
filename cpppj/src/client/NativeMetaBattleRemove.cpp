// 원문 삭제의 부분 효과와 진입 CS/현재 패킷 ID 재조회를 보존한다.
#include "client/NativeMetaBattleRemove.h"
#include <stdexcept>
#include <string>

namespace netstorm::client { // 삭제 뒤 별도 NSInfo 갱신이나 참가자 이동을 합성하지 않는다.
namespace { // 지정한 물리 필드와 선택한 하위 장치만 읽고 쓴다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("메타 전투 삭제 실제 장치 미연결");return function; } // 조기 반환 분기에서 쓰지 않는 장치는 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 전투 삭제 실제 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } // 지정된 BYTE/DWORD를 낮은 바이트부터 읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("메타 전투 삭제 실제 쓰기 범위");for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } // 원문 DWORD 하나만 기록한다.
}
// 원문 this를 진입 때 캡처하여 Rig/진단이 전역 CS 포인터를 바꿔도 기존 기록을 계속 처리한다.
void ReceiveNativeMetaBattleRemove(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaBattleRemoveHooks& h) {
    const auto id=Read(packet,4,1);const auto bytes=challenge.Bytes();if (id>=o::kChallengeBattleCount) throw std::out_of_range("메타 삭제 실제 전투10기록 범위");const auto record=bytes.subspan(o::kChallengeBattleOffset+id*o::kChallengeBattleBytes,o::kChallengeBattleBytes);
    if (!(Read(record,0,1)&1U)) { Need(h.log)("ERROR in handleChalBattleDel: battle "+std::to_string(Read(packet,4,1))+" doesn't exist!\n");return; }
    Need(h.leave)(Read(record,0x20));challenge.ClearArray(record.subspan(0x24,16));
    for (std::size_t word=0;word<0x49;++word) Put(record,word*4,0); // 원문73DWORD가292BYTE 기록 전체를 같은 순서로 지운다.
    Put(record,0x18,0xffffffffU);Put(record,0x1c,0x100U);
    for (std::uint32_t peer=0;peer<o::kChallengeParticipantCount;++peer) if (Read(bytes,o::kChallengeParticipantOffset+peer*o::kChallengeParticipantBytes+0xa4)==Read(packet,4,1)) Need(h.assertion)("chalPlayer[i].battle != z->battle","\\Ns\\O\\Ch.cpp",0x696); // 원문40기록은 active 필터 없이 현재 패킷 ID를 매번 재조회한다.
}
}
