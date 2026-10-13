// teleportEffect 건설 운반 객체의 도착 판정과 서버 대상 건설 시작/예측 대기를 복원한다.
#pragma once
#include "o/RawConstructionProcess.h"
#include "o/RawSquidFinder.h"

namespace netstorm::o {
// 도착 판정 뒤에도 현재 값을 읽는 원본 전역이다. PathProcess의 시각/단계 상태와 구별한다.
struct ConstructionArrivalState {
    bool checking{}; // 005e4794 / CD 00531454: 예측 대기에서 streak contained 정리를 생략하는 debug 전역이다.
    std::uint32_t streakProcessType{12}; // 005410a0 / CD 0051c990: 예측 대기 때 첫 contained에서 지우는 실제 타입이다.
    float duration{0.6F}; // 005325c0 / CD 0053fc38: 확정 대상에 전달하는 건설 지속 시간이다.
};
// 도착 몸체가 요청하는 하위 효과다. 현재 raw/전역/해시를 바꿀 수 있으며 다음 후보는 현재 상태를 읽는다.
struct ConstructionArrivalHooks {
    std::function<void(Sid,float)> start; // 00443fe0 / CD 004d06a0: 같은 칸의 서버 abstract 대상에서 실제 건설을 시작한다.
    std::function<void(Sid,std::uint32_t)> removeContained; // 004b1a90 / CD 004eb310(sid,type,0,1): 예측만 보이면 첫 streak를 삭제한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 도착 운반 객체 가상 +0x10(flags0). 이후 this를 읽지 않는다.
};
// PathProcess가 parent 가상 +0x3c를 호출할 때 사용하는 몸체다. 경로 탐색/이동/재시도 분배 자체는 외부에 있다.
class RawConstructionArrival {
public:
    // 동일 raw 풀/해시/타입/전역은 이 객체보다 오래 살아야 한다. 필수 하위 효과는 생성 때 확인한다.
    RawConstructionArrival(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        const ConstructionArrivalState& state,ConstructionArrivalHooks hooks);
    // 00444080 / CD 004d2c80: 현재 좌표의 정수 칸→actual finder→abstract/needsBuilder→서버 대상 Start 순서다.
    // 예측 대상만 있으면 1002(대기), 서버 대상에 시작했거나 대상이 없으면 자기 삭제 후 1001을 반환한다.
    std::uint32_t Arrive(Sid builder) const;
    // 실제 건설/삭제 연결에서 같은 풀인지 확인한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_; // 도착 운반 객체와 현재 후보의 raw 필드다.
    const SquidHash& hash_; // 일반 네 단계 finder가 읽는 현재 공간 등록이다.
    std::span<const RiftTypeRecord> types_; // 후보의 현재 flags1을 조회한다.
    const ConstructionArrivalState& state_; // callback 뒤에도 현재 duration/debug/type을 사용한다.
    ConstructionArrivalHooks hooks_; // 건설 시작/contained/자기 삭제의 실제 하위 효과다.
};
// 시작 경계를 같은 풀의 실제 ConstructionProcess로 연결한다. contained/가상 삭제 경계는 유지한다.
ConstructionArrivalHooks MakeConstructionArrivalHooks(const SidPool& pool,const RawConstructionProcess& process,
    ConstructionArrivalHooks hooks);
}
