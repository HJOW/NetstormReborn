// 사제 회복 HP 부모의 판본별 읽기 순서와 signed ADD 감김을 보관한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <optional>

namespace netstorm::o { // Setter/표시/공간 효과는 실제 회복 처리기가 이어서 실행한다.
struct PriestRegenHitPointHooks { // 호출 위치에서 선택된 장치만 요구한다.
    std::function<std::int32_t()> current; // 같은 사제 슬롯의 현재 signed DWORD/WORD HP다.
    std::function<std::int32_t()> maximum; // 같은 객체의 현재 MaxHitPoints를 조회한다.
    std::function<std::uint32_t()> priestType; // 각 증분에서 현재005412d0/CD0051cbbc를 읽는다.
    std::function<std::int32_t(std::uint32_t)> typeMaximum; // 저장한 번호의 TYPE HP getter를 실행한다.
};
std::optional<std::int32_t> ReadPriestRegenHitPoints(OriginalEdition edition,const PriestRegenHitPointHooks& hooks); // 빈 결과는 초기HP>=현재최대HP다. 그 외에는 원문 Setter에 넘길 signed DWORD를 반환한다.
}
