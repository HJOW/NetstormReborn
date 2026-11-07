// BattleInfo가 사용하는 실제 BattleOptions BYTE 변경과 제외 목록을 원문 순서로 처리한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o { // 같은 현재87BYTE를 빌리며 별도 설정/제외 목록 사본을 만들지 않는다.
std::uint32_t BattleOptionGameType(std::span<const std::uint8_t> options); //0041c3c0: 현재 첫 BYTE를 그대로 DWORD로 확장한다.
std::int32_t MaximumBattleOptionChoice(std::int32_t index); //0041c4b0: 같은 정적22레코드의 포함 최댓값이며 선택 개수가 아니다.
void CycleBattleOption(std::span<std::uint8_t> options,std::int32_t index,const std::function<void()>& guySound); //0041c460: BYTE증가/최댓값순환을 계산하고 guy.wav 뒤에 최종BYTE를 저장한다.
void SetBattleOptionChoice(std::span<std::uint8_t> options,std::int32_t index,std::int32_t value); //0041c4d0: 음수는 표 조회 없이 반환하고 포함 최댓값 이내만 실제BYTE에 쓴다.
std::uint32_t AddBattleExcludedType(std::span<std::uint8_t> options,std::uint32_t type,const std::function<void()>& guySound); //0041c6d0: 전체DWORD 비교 중복/초기count40관문·BYTE추가/count증가→음향이다.
std::uint32_t RemoveBattleExcludedType(std::span<std::uint8_t> options,std::uint32_t type,const std::function<void()>& guySound); //0041c730: 최초 일치 뒤 초기count 길이memmove→음향→현재count BYTE감소다.끝칸을지우지않는다.
std::uint32_t BattleTypeExcluded(std::span<const std::uint8_t> options,std::uint32_t type); //0041c7a0: 최초count만큼 실제BYTE와 전체DWORD를 비교하고 최초 일치1/없음0이다.
}
