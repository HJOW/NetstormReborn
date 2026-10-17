// 너겟484f90의 최초 두 Regular 예약과 Damageable 위임을 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
namespace netstorm::o {
// 할당 뒤에만 난수를 소비하며 각 실패가 뒤의 예약/기본 처리를 막지 않는다.
struct NuggetPostPopHooks {
    std::function<bool(Sid,std::uint32_t)> hasRegular; // 4afb40(parent,1)의 현재 예약 조회다.
    std::function<bool()> allocate; // 각각의 new28을 독립 수행한다.
    std::function<float(float,float)> random; // 455930(0,0.3/0.5)의 float 저장 결과다.
    std::function<void(Sid,std::uint32_t,float)> construct; // 확보한 같은 메모리의496e80 생성/부착이다.
    std::function<void(Sid,std::uint32_t)> damageable; // 마지막44bf80의 실제 전체 몸체다.
};
// 너겟의 최초 실체/그림 미리보기 모두 같은 실제 부모 순서를 사용한다.
class RawNuggetPostPop {
public:
    RawNuggetPostPop(SidPool& pool,NuggetPostPopHooks hooks); //10.78 전용이며 풀/효과 수명은 이 객체보다 길어야 한다.
    void PostPop(Sid sid,std::uint32_t flags) const; // flags bit0일 때만 최초 예약을 하고 항상 기본 몸체를 호출한다.
    const SidPool& Pool() const; // 다른 월드로 단일 예약 메모리를 넘기지 않게 검사한다.
private:
    SidPool& pool_; // 너겟과 실제 Regular Form의 공유 풀이다.
    NuggetPostPopHooks hooks_; // 조회/할당/게임 난수/생성/기본 처리의 실제 경계다.
};
}
