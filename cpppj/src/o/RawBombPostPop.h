// 세 주문 효과의 최초 Regular 예약을 실제 가상 표와 같은 게임 난수에서 처리한다.
#pragma once
#include "o/SidPool.h"
#include "o/SpStore.h"
#include <functional>

namespace netstorm::o { // 10.78 효과의 접두만 다루며 공통 장부는 상위 분배가 한 번 실행한다.
struct BombPostPopHooks { // 선택된 원문 호출 위치에서만 장치를 요구한다.
    std::function<bool(Sid,std::uint32_t)> hasRegular; // 004afb40(parent,1)의 현재 예약 조회다.
    std::function<bool()> reserveRegular; // new0x28의 실제 할당 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 확보한 같은 메모리의 00496e80(event1,parent,delay)다.
};
class RawBombPostPop { // 그라비테이션·힐·미티어의 +20 접두이며 시전/공격 프레임은 실행하지 않는다.
public:
    RawBombPostPop(const SidPool& pool,GameRandom& random,BombPostPopHooks hooks); // 같은 PATCH 풀과 장기 난수를 빌린다.
    bool Handles(Sid parent) const; // 표시 TYPE와 무관하게 세 실제 가상 표만 받는다.
    void Prefix(Sid parent,std::uint32_t flags) const; // 최초·중복 없음·할당 성공 뒤에만 난수와 예약을 실행한다.
    const SidPool& Pool() const; // 공통 후처리 연결에서 다른 월드의 풀을 거부한다.
private:
    const SidPool& pool_; // free/contained 사전 관문 없이 현재 물리 가상 표를 읽는다.
    GameRandom& random_; // 00532710을 같은 Client의 다른 소비자와 공유한다.
    BombPostPopHooks hooks_; // 실행 중 자기 연결 반납에 대비해 선택 callable을 복사한다.
};
}
