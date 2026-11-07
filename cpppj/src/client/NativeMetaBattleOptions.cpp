// 기록포인터와자기인덱스를캡처하고음향뒤현재필드/길이를다시읽는원문순서를보존한다.
#include "client/NativeMetaBattleOptions.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace netstorm::client { //실제송신의159상한과수신의160상한을임의통합하지않는다.
namespace { //선택된필드폭/장치만요구하며선행부분효과는되돌리지않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("메타67h 수신의 실제 장치 미연결");return hook; } //원문이도달한효과위치에서만요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타67h 수신의 실제 필드 부족");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //현재BYTE/WORD/DWORD를원문낮은BYTE부터읽는다.
}
// 현재전투의flagBYTE를먼저검사하고자기master면음향/복사/NSInfo를모두생략한다.
void ReceiveNativeMetaBattleOptions(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaBattleOptionsHooks& h) {
    const auto battle=challenge.Battle(Read(packet,4,1));
    if (!(Read(battle,0,1)&1U)) { Need(h.log)("ERROR in handleBattleOptions: battle "+std::to_string(Read(packet,4,1))+" is not valid!\n");return; }
    if (Read(packet,5,2)>160U) { Need(h.log)("ERROR in handleChalBattleOptions: option data too long\n[bufferLen == 160, optionLen == "+std::to_string(Read(packet,5,2))+"]\n");return; }
    const auto own=challenge.Participant(Need(h.owner)());
    if (Read(own,0xa4)==Read(packet,4,1) && (Read(own,0x64,1)&4U)) return;
    Need(h.guySound)();const auto count=Read(packet,5,2);std::uint32_t offset{};
    for (;offset+4U<=count;offset+=4U) { //rep movsd는같은네BYTE를먼저읽고저장한다.전체선행복사/남은필드초기화는없다.
        if (7U+offset>packet.size() || 4U>packet.size()-7U-offset) throw std::out_of_range("메타67h source DWORD 부족");const std::array<std::uint8_t,4> word{packet[7+offset],packet[8+offset],packet[9+offset],packet[10+offset]};
        if (0x59U+offset>battle.size() || 4U>battle.size()-0x59U-offset) throw std::out_of_range("메타67h destination DWORD 부족");std::copy(word.begin(),word.end(),battle.begin()+0x59+offset);
    }
    for (;offset<count;++offset) { //나머지0..3BYTE도각실제읽기후쓰기순서로처리한다.
        const auto value=Read(packet,7U+offset,1);if (0x59U+offset>=battle.size()) throw std::out_of_range("메타67h destination BYTE 부족");battle[0x59+offset]=static_cast<std::uint8_t>(value);
    }
    battle[0x58]=static_cast<std::uint8_t>(Read(packet,5,1));const auto ownBattle=Read(own,0xa4);if (ownBattle && ownBattle==Read(packet,4,1)) Need(h.refreshInfo)();
}
}
