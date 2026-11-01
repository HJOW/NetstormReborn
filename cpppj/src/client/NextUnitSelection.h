// 원본 N 키의 다음 이동 유닛 검색과 선택/카메라/정지 순서를 같은 실제 풀에 적용한다.
#pragma once
#include "o/SidPool.h"
#include "o/RiftType.h"
#include <functional>

namespace netstorm::client { //실제 사용자 입력의 순환 검색 부모와 장치 경계를 정의한다.
struct NextUnitSelectionHooks { //선택 자체는 기존 RawUserSelection, 가운데 이동은 같은 World가 수행한다.
    std::function<std::uint32_t()> localOwner,paused; //검색 전 소유자를 캡처하고 가운데 이동 뒤 정지를 새로 읽는다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> select; //004d5c90의 세 DWORD를 생략하지 않는다.
    std::function<void(std::uint32_t)> center; //00433ba0을 첫 선택 성공 여부와 무관하게 호출한다.
    std::function<void(std::uint32_t,std::uint32_t)> multiple; //Shift 비영값일 때 실제004da6b0(candidate,1)을 호출한다.
};
void SelectNextMovingUnit(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::uint32_t& selected,std::uint32_t add,const NextUnitSelectionHooks& hooks); //004da880의 현재 기본 선택 뒤 검색→한 번 순환→선택/가운데/정지/후속 선택이다.
}
