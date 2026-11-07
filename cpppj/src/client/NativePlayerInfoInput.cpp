// 원문 전체 분기와 헤더·옵션/CS 부분효과·두 송신의 순서를 유지한다.
#include "client/NativePlayerInfoInput.h"
#include "o/RawBattleOptionSlots.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <stdexcept>

namespace netstorm::client { // 실제 노드와 CS span은 동기 입력 동안 같은 owner의 주소다.
namespace { // 원문 폭/선택된 장치와 옵션 변경의 헤더 전후를 구별한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("PlayerInfo 입력의 실제 장치 미연결");return hook; } // 선택한 효과 위치에서만 현재 장치를 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { if (offset>bytes.size() || 4>bytes.size()-offset) throw std::out_of_range("PlayerInfo 입력의 실제 CS DWORD 부족");return std::uint32_t(bytes[offset])|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24); } // 비정렬 실제 DWORD를 낮은 BYTE부터 읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { if (offset>bytes.size() || 4>bytes.size()-offset) throw std::out_of_range("PlayerInfo 입력의 실제 DWORD 쓰기 범위");for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } // 원문의 저장 폭 네 BYTE만 쓴다.
enum class SlotChange { RankDown,ColorDown,Special,ColorUp,RankUp }; // 좌/우 행의 실제 다섯 writer 선택이며 Special만 헤더 뒤에 변경한다.
// 색/랭크는 헤더 전에 바꾸며 Special은 헤더/초기 payload 뒤에 바꾼다.4000h는 CS에 다시 저장하지 않는다.
void ChangeSlot(HudPlayerInfoNode& node,SlotChange change,const NativePlayerInfoInputHooks& h) {
    const auto record=Need(h.participant)(node.player);const auto options=Need(h.battle)(Read(record,0xa4)).subspan(0x59);
    if (change!=SlotChange::Special) {
        const auto slot=std::bit_cast<std::int32_t>(Read(record,0xa8));
        switch (change) {
        case SlotChange::RankDown:o::DecreaseBattleSlotRank(options,slot);break;case SlotChange::ColorDown:o::DecreaseBattleSlotColor(options,slot);break;
        case SlotChange::ColorUp:o::IncreaseBattleSlotColor(options,slot);break;case SlotChange::RankUp:o::IncreaseBattleSlotRank(options,slot);break;case SlotChange::Special:break;
        }
    }
    Need(h.status)([&node,change,record,options,&h](auto packet) {
        const auto subscriber=Read(Need(h.participant)(node.player),4);
        if (change==SlotChange::Special) {
            Put(packet,4,subscriber);Put(packet,8,0);o::ToggleBattleSlotSpecial(options,std::bit_cast<std::int32_t>(Read(record,0xa8)));
            if (node.player==Need(h.owner)()) { const auto at=std::uint32_t(Read(record,0xa8)+0x3fU);if (at>=options.size()) throw std::out_of_range("PlayerInfo 특수 슬롯의 실제 BYTE 범위");Need(h.auxiliary)(options[at]!=0 ? 1U:0U); }
        } else { Put(packet,8,0);Put(packet,4,subscriber); }
        Put(packet,8,Read(record,0x64)|0x4000U);
    });
    Need(h.data)(Read(record,0xa4),options,0x57);
}
// 현재 참가자 이름은 BeginFancy 이후 실제 서식 위치에서 읽고 임의 길이 절단을 하지 않는다.
void ConfirmKickoff(HudPlayerInfoNode& node,const NativePlayerInfoInputHooks& h) {
    const auto record=Need(h.participant)(node.player);Need(h.beginMenu)();const auto pattern=Need(h.translate)("Kickoff \"%s\"?\n");const auto bytes=record.subspan(0x26);const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("PlayerInfo 추방 이름의 실제 NUL 없음");const std::string name(reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin()));
    std::array<char,256> text;const auto size=std::snprintf(text.data(),text.size(),pattern.c_str(),name.c_str());if (size<0 || static_cast<std::size_t>(size)>=text.size()) throw std::out_of_range("PlayerInfo 추방 제목의 실제256BYTE 범위");Need(h.title)({text.data(),static_cast<std::size_t>(size)});
    const auto yes=Need(h.translate)("Yes");const auto participant=node.player,command=Need(h.command)(0x5429dc);Need(h.item)(yes,command,participant);const auto no=Need(h.translate)("No");Need(h.item)(no,Need(h.command)(0x5429dc),0);Need(h.attachMenu)();
}
}
// 클릭의 두 자기 버튼과 좌/우 텍스트 serial을 구별하며 태그 사건은 해당 분기에서 항상1로 소비한다.
std::uint32_t HandleNativePlayerInfo(HudPlayerInfoNode& node,std::uint32_t event,std::uint32_t first,std::uint32_t second,const NativePlayerInfoInputHooks& h) {
    const auto code=event&0x03ffffffU;
    if (code==Need(h.command)(0x5427a8) && node.player) {
        const auto player=node.player,readySerial=Need(h.serial)(node.button),owner=Need(h.owner)();
        if (first==readySerial && player==owner) {
            const auto record=Need(h.participant)(player);Need(h.status)([&node,record,&h](auto packet) { Put(packet,4,Need(h.subscriber)());const auto value=Need(h.buttonValue)(node.button);auto flags=Read(record,0x64);flags=value ? flags|2U:flags&0xfffffffdU;Put(record,0x64,flags);Put(packet,8,flags); });return 1;
        }
        if (first==Need(h.serial)(node.start) && player==owner) { if (!node.reserved) { Need(h.status)([&h](auto packet) { Put(packet,4,Need(h.subscriber)());Put(packet,8,0x12); });node.reserved=1; }return 1; }
    } else {
        if (code==Need(h.command)(0x542790)) {
            if (first==Need(h.serial)(node.text)) {
                const auto player=node.player;
                if (second==player+300U) { ChangeSlot(node,SlotChange::RankDown,h);return 1; }
                if (second==player+100U) { ChangeSlot(node,SlotChange::Special,h);return 1; }
                if (second==player+200U) { ChangeSlot(node,SlotChange::ColorDown,h);return 1; }
                if (second==player+400U) ConfirmKickoff(node,h);
            }
            return 1;
        }
        if (code==Need(h.command)(0x54278c)) {
            if (first==Need(h.serial)(node.text)) {
                const auto player=node.player;
                if (second==player+100U) { ChangeSlot(node,SlotChange::Special,h);return 1; }
                if (second==player+200U) { ChangeSlot(node,SlotChange::ColorUp,h);return 1; }
                if (second==player+300U) ChangeSlot(node,SlotChange::RankUp,h);
            }
            return 1;
        }
    }
    return 0;
}
}
