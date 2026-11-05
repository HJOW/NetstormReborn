// 선택 체력 막대의 현재HP/최대HP/색 조회 순서를 원문 그대로 보관한다.
#pragma once
#include "o/RiftType.h"
#include <array>
#include <functional>

namespace netstorm::client { // 실제 픽셀 출력과 독립인 선택 막대의 정수 부모다.
struct SpriteHitPointBar { // 출력기는 이 원문 결과에 판본별 막대 기하를 적용한다.
    std::uint32_t color{}; // 현재 정상/절반/1/4 색 중 원문이 마지막으로 고른 값이다.
    std::int32_t current{},maximum{}; // PATCH는 마지막 조회 전 HP, CD는 WORD HP의 원문 clamp 결과다.
};
struct SpriteHitPointBarHooks { // 도달한 읽기에서만 실제 장치를 요구한다.
    std::function<std::int32_t()> current; // 같은 물리 슬롯의 현재 signed DWORD/WORD HP를 읽는다.
    std::function<std::int32_t()> maximum; // 처음 선택한 TYPE this의 현재 최대 HP getter를 호출한다.
};
SpriteHitPointBar ReadSpriteHitPointBar(o::OriginalEdition edition,const std::array<std::uint32_t,3>& colors,const SpriteHitPointBarHooks& hooks); // PATCH3회/CD1~2회 최대HP와 현재HP·색의 원문 선후 관계를 실행한다.
}
