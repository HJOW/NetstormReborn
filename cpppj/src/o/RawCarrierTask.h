// Carrier426db0의 현재 task 조회/생성/낙하 유지/시간 재설정을 복원한다.
#pragma once
#include "o/SquidProcess.h"
namespace netstorm::o {
// 실제 Host 객체와 독립 할당/생성 및 복귀 가능한 진단을 공급한다.
struct CarrierTaskHooks {
    std::function<RegularProcess*(Sid)> find; // 4afbe0(parent,fff0,5100)의 현재 프로세스다.
    std::function<bool()> allocate; // new28 성공 여부이며 실패는 원본 NULL 쓰기 경로다.
    std::function<RegularProcess*(Sid,std::uint32_t,bool)> construct; // 확보 메모리의496e80/496f00이며 shared 여부를 구별한다.
    std::function<void()> assertion; // 기존 낙하 task 교체의 Carrier.cpp407 진단이다.
};
// 모든 시각과 프로세스는 같은 Session의 현재 상태다.
class RawCarrierTask {
public:
    RawCarrierTask(const SidPool& pool,const double& now,CarrierTaskHooks hooks); // PATCH 풀과 현재 double 시각을 참조한다.
    void Set(Sid parent,std::uint32_t task,float delay) const; // 현재 낙하를 보존하고 payload를 바꾸지 않으며 count0/새 기한을 쓴다.
    const SidPool& Pool() const; // 다른 Host/Session의 상태 혼합을 거부할 때 사용한다.
private:
    const SidPool& pool_; // 원본 부모 번호를 사용하는 동일 물리 풀이다.
    const double& now_; // 생성/진단 콜백 뒤 원본 위치에서 읽는 현재 시각이다.
    CarrierTaskHooks hooks_; // 기존 프로세스와 같은 확보 메모리의 실제 생성이다.
};
}
