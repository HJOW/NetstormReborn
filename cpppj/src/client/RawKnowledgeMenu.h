// 원본 ShowTechnology의 전체 기술 카드 필터·문자열·정렬·네 줄 배치를 같은 실제 Gump에 작성한다.
#pragma once
#include "client/KnowledgeSort.h"
#include "client/MenuGump.h"
#include <variant>
namespace netstorm::client {
inline constexpr std::uint32_t kShowTechnologyCommand=0x020000a0U; // 보호00542974의 실제 스크립트/사제/F6 명령이다.
// 원본 이름 표의 ShowTechnology/CheatAllTechnology를 조회한다. 미등록은 원본ffffffff다.
std::uint32_t KnowledgeScriptCode(std::string_view name);
// 화면/언어/아이콘 치수와 원본 타입/명령 전역을 현재 호출에서 제공한다.
struct KnowledgeMenuState {
    std::int32_t width{640},height{480},iconWidth{65},iconHeight{60}; // 00531860/64·00540cbc/cc0의 현재 값이다.
    std::uint32_t language{1},templeProduct{133},helpCommand{0x02000017}; // 00542384/0054126c/00542754다. 언어3은 설명 줄을 비운다.
};
using KnowledgeParameter=std::variant<std::uint32_t,std::string>; // 실제 타입 번호와 정적 themeHelp 문자열을 구별한다.
// 게임 계산/GUI 조각은 같은 기존 몸체를 사용하며 도달한 효과만 필수로 요구한다.
struct KnowledgeMenuHooks {
    std::function<bool()> blocked; // 00594fc8의 현재 메타맵 모드다. 비제로면 아무 카드도 만들지 않는다.
    std::function<void()> cancel; // 실제00446e70 커서 취소다.
    std::function<std::uint32_t()> currentTechnology; // 실제0048fc60을 한 번 호출한다.
    std::function<bool(std::uint32_t,std::uint32_t)> owns; // 같은 현재 부모에서 flags4/kind타입/index0을 조회한다.
    std::function<std::string(std::uint32_t)> mana,description; // 현재 TYPE 마나/실제 번역 설명의 원본 바이트다.
    std::function<bool(std::uint32_t)> inProduction,excluded; // 실제 Deck 포함과52f7e0 제외 목록의 현재 조회다.
    std::function<std::string(std::string_view)> translate; // 원본 네 머리/Excluded 번역이다.
    std::function<std::uint32_t()> begin; // 실제 BeginVertical(0)로 현재 메뉴를 한 칸 만든다.
    std::function<MenuGumpNode&(std::uint32_t)> node; // 실제 현재 메뉴 저장소이며 flags/목표폭을 같은 주소에 쓴다.
    std::function<void(std::string_view)> title; // 같은 controlled AddTitle이다.
    std::function<void()> separator; // 같은 controlled AddSeparator다.
    std::function<void(std::string_view,std::uint32_t,KnowledgeParameter,bool)> item; // 실제 include1/forceColor0의 AddText다.
    std::function<void(std::int32_t,std::int32_t)> attach; // 현재 메뉴를 루트에 부착하고 controlled 칸을 내린다.
    std::function<void(std::uint32_t)> push,pop; // 실제 가상+18/+1c의 분리/재부착이다.
    std::function<void(std::uint32_t,float)> width,height; // 실제475f20/475fb0이다.
    std::function<ScreenPoint(std::uint32_t)> position; // 원본464740의 현재 정수 위치다.
    std::function<void(std::uint32_t)> mark; // 4c5340(command,null)의 마지막 명령·80BYTE 빈 표식이다.
};
// 00492b60: 전체 카드 토큰을 생성 순서대로 반환한다. 실제 게임/GUI는 훅의 현재 소비자를 사용한다.
std::vector<std::uint32_t> ShowKnowledgeMenu(std::span<const o::RiftTypeRecord> types,const KnowledgeMenuState& state,const KnowledgeMenuHooks& hooks);
}
