// 원문 right-tag/직접명령의 반환값과 옵션 변경 뒤에만 수행하는 송신/재부착을 유지한다.
#include "client/NativeBattleInfoInput.h"
#include "client/HudNsInfoMeta.h"
#include "o/RawBattleOptionMutation.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 같은 파생 노드/옵션을 빌리며 authority나 mode를 복제하지 않는다.
namespace { // 메뉴 자식은 빈 성공으로 대체하지 않고 실제 선택 위치에서 요구한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("BattleInfo 입력의 실제 메뉴/장치 미연결");return hook; } // 선택되지 않은 자식은 요구하지 않는다.
// 가상 Push 효과 뒤 바뀔 수 있는 현재 위치를 읽어 실제 같은 파생 Attach에 전달한다.
void Reattach(std::uint32_t token,HudBattleInfoNode& node,const NativeBattleInfoInputHooks& h) { Need(h.push)(token);const auto x=node.x,y=node.y;Need(h.attach)(token,x,y,0,0); }
}
// 게임 종류0은 그대로 두고 설정 쌍의 현재 값1..21을 원문 순서로 각각 적용한다.
void ApplyMetaBattleDefaults(HudBattleInfoNode& node,std::span<std::uint8_t> options) {
    for (std::int32_t index=1;index<22;++index) o::SetBattleOptionChoice(options,index,std::bit_cast<std::int32_t>(node.settings[static_cast<std::size_t>(index)][0])); //각writer호출직전에현재쌍을다시읽는다.
}
// 이 부모는 playerKnown 관문을 추가하지 않는다. 실제 전투 flag BYTE1을 통과한 뒤에만 event를 마스킹한다.
std::uint32_t HandleNativeBattleInfo(std::uint32_t token,HudBattleInfoNode& node,std::uint32_t event,std::uint32_t first,std::uint32_t second,const NativeBattleInfoInputHooks& h) {
    if (!Need(h.metamap)() || !node.battle) return 0;const auto battle=Need(h.battle)(node.battle);if (battle.empty()) throw std::out_of_range("BattleInfo 입력의 실제 전투 flag BYTE 부족");if (!(battle[0]&1U)) return 0;
    const auto options=battle.subspan(0x59);const auto code=event&0x03ffffffU;std::uint32_t result{},changed{};
    if (code==Need(h.command)(0x54278c)) {
        if (first==Need(h.serial)(node.title)) {
            if (std::bit_cast<std::int32_t>(second)<23) {
                if (!node.changed) return 0;
                if (node.mode==2) {
                    const auto index=std::bit_cast<std::int32_t>(second-1U);o::CycleBattleOption(options,index,h.guySound);
                    if (!index) { PrepareMetaBattleSettings(node,o::BattleOptionGameType(options));ApplyMetaBattleDefaults(node,options); }changed=1;
                } else if (second==1) { o::CycleBattleOption(options,0,h.guySound);PrepareMetaBattleSettings(node,o::BattleOptionGameType(options));ApplyMetaBattleDefaults(node,options);changed=1; }
                else { const auto index=second-1U;o::SetBattleOptionChoice(options,std::bit_cast<std::int32_t>(index),std::bit_cast<std::int32_t>(node.settings.at(index)[0]));changed=1; }
            } else if (std::bit_cast<std::int32_t>(second)<27 || std::bit_cast<std::int32_t>(second)>48) {
                if (second==24) Need(h.sortMenu)(node);
                else if (second==23) { if (!node.changed) return 0;Need(h.optionsMenu)(node,options); }
                else if (second==1000) { if (!node.changed) return 0;Need(h.excludeMenu)(node,options); }
                else if (std::bit_cast<std::int32_t>(second)>121) { if (!node.changed) return 0;Need(h.removeMenu)(second-122U); }
            } else { if (!node.changed) return 0;Need(h.optionMenu)(node,options,std::bit_cast<std::int32_t>(second-27U)); }
        }
        result=1;
    }
    if (code==Need(h.command)(0x542b80)) { node.mode=second;Reattach(token,node,h); }
    if (code==Need(h.command)(0x542b7c)) {
        if (!node.changed) return result;const auto index=std::bit_cast<std::int32_t>(second)>>16;const auto value=std::bit_cast<std::int32_t>((static_cast<std::uint32_t>(index)<<16)^second);o::SetBattleOptionChoice(options,index,value);++changed;
    }
    if (code==Need(h.command)(0x5429e0)) { if (!node.changed) return result;if (second) { o::AddBattleExcludedType(options,second,h.guySound);++changed;result=1; } }
    if (code==Need(h.command)(0x5429e4)) { if (!node.changed) return result;if (second) { o::RemoveBattleExcludedType(options,second,h.guySound);++changed;result=1; } }
    if (changed && node.changed) { Need(h.data)(node.battle,options,0x57);Reattach(token,node,h); }
    return result;
}
}
