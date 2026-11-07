// Native의 현재 WORD 재조회와 DWORD/나머지BYTE 복사·후속 효과를 보존한다.
#include "o/RawStateBattleOptions.h"
#include <stdexcept>

namespace netstorm::o { // 현재87BYTE 밖 쓰기는 앞선 효과를 유지한 물리 경계 오류로 거부한다.
namespace { // Native 필드 폭과 실제 선택 callback을 공유한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("State67h 실제 하위 장치 미연결");return function; } // 조기 반환/비선택 효과의 장치를 선검사하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("State67h 실제 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } // 낮은 BYTE부터 현재WORD/DWORD를 읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("State67h 실제 쓰기 범위");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } // 선택된 필드 폭만 쓰고 미복사 꼬리를 지우지 않는다.
}
// 길이 진단이 현재 packet/CS/관문을 바꿔도 각 원문 효과 위치에서 다시 읽는다.
void ReceiveStateBattleOptions(std::uint32_t sender,std::span<const std::uint8_t> packet,StateBattleOptionGlobals g,const StateBattleOptionHooks& h) {
    if (Read(packet,5,2)>87U) Need(h.assertion)("z->dataLen <= sizeof(globalBattleOptions)");const auto length=Read(packet,5,2);std::size_t at{};
    for (std::uint32_t word=length>>2;word;--word,at+=4) Put(g.options,at,Read(packet,7+at,4),4); // 원문 rep movsd의 현재 한 DWORD씩 읽기/쓰기다.
    for (std::uint32_t byte=length&3U;byte;--byte,++at) Put(g.options,at,Read(packet,7+at,1),1); // 마지막0..3BYTE만 추가로 복사한다.
    if (g.challenge) Need(h.challengeOptions)(sender,packet);Need(h.range)();const auto cost=Need(h.cost)();Need(h.storeCost)(static_cast<float>(cost));g.value=Need(h.currentValue)();if (g.battle && !g.mission && g.saved) Need(h.reconnect)();
}
}
