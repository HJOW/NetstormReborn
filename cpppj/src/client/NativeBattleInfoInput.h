// BattleInfo의 입력 분기/현재 옵션 기본값 적용과 메뉴·재부착·송신의 실제 부모를 복원한다.
#pragma once
#include "client/HudNsInfoCreate.h"
#include <functional>
#include <span>

namespace netstorm::client { // 실제 입력 연결은 모든 메뉴 자식 장치를 마련한 뒤 분배기에 등록한다.
struct NativeBattleInfoInputHooks {
    std::function<std::uint32_t()> metamap; // 같은 현재594fc8을 최초 관문에서 읽는다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> battle; // 같은 현재 CS의292BYTE 전투 기록이다.
    std::function<std::uint32_t(std::uint32_t)> command,serial; // 현재 명령 DWORD/제목 자식serial을 읽는다.
    std::function<void()> guySound; // 현재 guy.wav(0,0,0,0,0) 음향 장치다.
    std::function<void(HudBattleInfoNode&)> sortMenu; // 원문의 Sort Game Options By 전체 Fancy 메뉴 자식이다.
    std::function<void(HudBattleInfoNode&,std::span<std::uint8_t>)> optionsMenu,excludeMenu; // 전체 활성 옵션 하위 메뉴와 현재 TYPE 분류/정렬의 제외 추가 메뉴다.
    std::function<void(HudBattleInfoNode&,std::span<std::uint8_t>,std::int32_t)> optionMenu; // 선택 옵션 하나의 모든 포함 선택값 메뉴다.
    std::function<void(std::uint32_t)> removeMenu; // 현재 TYPE 설명을 쓰는 제외 제거 확인 메뉴다.
    std::function<void(std::uint32_t)> push; // 같은 파생 Gump의 현재 가상 Push다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // Push 뒤 현재 x/y를 읽어 부모0/mode0으로 가상 부착한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>,std::uint32_t)> data; // 같은 현재 BattleOptions87BYTE를 실제67h로 송신한다.
};
void ApplyMetaBattleDefaults(HudBattleInfoNode& node,std::span<std::uint8_t> options); //0047b180 전체: 현재설정1..21의값을각실제선택writer로적용한다.
std::uint32_t HandleNativeBattleInfo(std::uint32_t token,HudBattleInfoNode& node,std::uint32_t event,std::uint32_t first,std::uint32_t second,const NativeBattleInfoInputHooks& hooks); //0047b8f0 입력 부모 전체의 관문/선택·변경·재부착·송신이며 메뉴 자식 본문은 실제 장치로 결합한다.
}
