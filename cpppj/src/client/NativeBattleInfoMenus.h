// BattleInfo의 다섯 메뉴 자식을 원문 선택 문자열/현재 TYPE·지식 행과 같은 controlled 스택에 작성한다.
#pragma once
#include "client/HudNsInfoCreate.h"
#include "client/KnowledgeSort.h"
#include <functional>

namespace netstorm::client { // 문자열 주소의 비교 의미와 호스트 문자열 소유를 구별한다.
struct BattleOptionMenuText {
    std::string text; // 현재 원본 글꼴 바이트의 선택 문자열이다.
    std::uint64_t identity{}; // 같은 실제 선택/번역 포인터의 논리 정체성이며 호스트 임시string 주소가 아니다.
};
struct NativeBattleInfoMenuHooks {
    std::function<std::uint32_t(std::uint32_t)> read; // 같은 현재 명령 DWORD와 제외할 TYPE 별칭을 읽는다.
    std::function<std::string(std::string_view)> translate; // 현재 원문 언어의 실제 번역이다.
    std::function<BattleOptionMenuText(std::int32_t,std::uint32_t)> choice; //0041c410의 현재 선택 포인터/조건부 번역 정체성이다.
    std::function<BattleOptionMenuText(std::span<const std::uint8_t>,std::int32_t)> value; //0041c3d0의 현재 BYTE 선택/조건부 번역 정체성이다.
    std::function<void()> beginFancy,beginVertical,attach; // 원문의 같은 controlled 생성과 Current→현재루트/원시좌표 부착이다.
    std::function<void(std::string_view)> title; // 실제 AddTitle이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,bool)> item; // include1/force0와 원문 enabled를 같은 AddText에 전달한다.
    std::function<std::uint32_t()> take; // 현재 하위 controlled 칸을 꺼내 같은 메뉴 토큰을 반환한다.
    std::function<void(std::string_view,std::uint32_t)> submenu; // 현재 부모에 실제 하위 메뉴를 추가한다.
    std::function<std::int32_t()> typeCount; //00541348의 현재 TYPE 개수를 각 반복 조건에서 읽는다.
    std::function<const o::RiftTypeRecord&(std::uint32_t)> type; // 현재 TYPE+98/+ec를 같은 저장소에서 읽는다.
    std::function<std::span<const o::RiftTypeRecord>()> types; // 기존 CRT/453f70 정렬이 같은 현재 표를 빌린다.
    std::function<bool(std::uint32_t)> globallyExcluded; //52f7e0의 현재 전역 옵션 조회이며 편집 중인 전투 옵션으로 대체하지 않는다.
    std::function<KnowledgeRecord(std::uint32_t)> knowledge; //492960(type,0,1)의 실제마나/설명·현재기술/생산 조회를 수행한다.
    std::function<std::string(std::uint32_t)> description; // 현재 TYPE의49af80 번역/설정치환을 메뉴 작성 때 다시 읽는다.
};
void BuildBattleSortMenu(HudBattleInfoNode& node,const NativeBattleInfoMenuHooks& hooks); //정렬1/2의 원문 현재mode별 enabled와 Fancy 제목/부착이다.
void BuildBattleOptionsMenu(HudBattleInfoNode& node,std::span<const std::uint8_t> options,const NativeBattleInfoMenuHooks& hooks); //활성 설정1..21의 모든 선택값을 각각 세로 하위 메뉴로 만든다.
void BuildBattleOptionMenu(std::span<const std::uint8_t> options,std::int32_t index,const NativeBattleInfoMenuHooks& hooks); //개별설정의 포함최댓값/현재포인터 비교·현재이름 제목/부착이다.
void BuildBattleExcludeMenu(const NativeBattleInfoMenuHooks& hooks); //현재전역제외/12별칭·TYPE분류/492960·여섯정렬/다섯하위메뉴 전체다.
void BuildBattleRemoveMenu(std::uint32_t type,const NativeBattleInfoMenuHooks& hooks); //현재설명/256BYTE 제거확인·Yes/No의TYPE/0을작성한다.
}
