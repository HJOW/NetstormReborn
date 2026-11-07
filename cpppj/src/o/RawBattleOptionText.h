// 10.78의 실제22개 BattleOptions 문자열 표와 조건부 번역 조회를 제공한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::o { // 원문0052f5d0의 정적 표를 복원하며 현재 옵션 BYTE는 caller가 빌린다.
struct BattleOptionTextDefinition { //각24BYTE 레코드에서 표시 getter가 읽는 세 필드다.
    std::string_view name; //+0의 원문 이름이며 getter는 항상 현재 번역기를 호출한다.
    std::span<const std::string_view> values; //+4 선택 포인터 표이며 +8은 개수가 아닌 포함 최댓값이다.
    bool translateValue; //+c 비영일 때만 선택 문자열을 현재 번역기로 보낸다.
};
const BattleOptionTextDefinition& BattleOptionText(std::uint32_t index); //실제0..21 레코드를 읽고 물리 표 밖 인덱스는 거부한다.
std::string BattleOptionChoiceText(std::uint32_t index,std::uint32_t selected,const std::function<std::string(std::string_view)>& translate); //0041c410: 지정된 선택 포인터와+c 플래그에 따른 현재 번역이다.현재옵션BYTE는읽지않는다.
std::uint32_t BattleOptionChoiceAddress(std::uint32_t index,std::uint32_t selected); //보호PE의실제선택문자열포인터값이다.호스트역참조가아닌메뉴의원문포인터정체성에만사용한다.
std::string BattleOptionValueText(std::span<const std::uint8_t> options,std::uint32_t index,const std::function<std::string(std::string_view)>& translate); //0041c3d0: 현재 BYTE→선택 포인터→조건부 번역이다.
std::string BattleOptionNameText(std::uint32_t index,const std::function<std::string(std::string_view)>& translate); //0041c440: 현재 레코드 이름의 필수 번역 조회다.
}
