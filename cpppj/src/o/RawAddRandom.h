//0040dfd0의 고정 범위·시각 지역 난수·현재 로컬 섬을 기존 배치 몸체에 조립한다.
#pragma once
#include "o/RawFortFinishOwner.h"

namespace netstorm::o { // 기존 타입/배치/섬 기록과 같은 월드만 사용한다.
// 실제 부모가 호출할 때마다 읽는 장치와 최종 배치 경계다.
struct AddRandomHooks {
    std::function<double()> wallSeconds; //004bcab0→00460d70의 현재 벽시각이다.
    std::function<std::uint32_t()> local; // 시각 seed 뒤 읽는00540c70의 현재 로컬 소유자다.
    std::function<std::int32_t(std::uint32_t,std::int32_t)> island; //0046f380의 현재 소유자/지역0 검색이다.
    std::function<std::uint32_t(GameRandom&,const FortRandomRectangle&,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)> place; //0040ded0의 지역 PRNG/사각형/지역/수량/TYPE/force다.
};
// 고정 청크1..15를 칸16..240으로 바꿔 전달하며 마지막 배치 반환은 사용하지 않는다.
void AddRandomObjects(std::uint32_t type,std::uint32_t quantity,const AddRandomHooks& hooks);
}
