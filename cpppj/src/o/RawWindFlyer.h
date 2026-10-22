// 더스트 데빌의 전용 초기화·애니메이션·주기 피해를 원본 10.78 순서로 실행한다.
#pragma once
#include "o/RawFlyerProcess.h"
#include "o/RawGunTarget.h"

namespace netstorm::o {
// 실제 원문이 선택한 큰 효과만 현재 월드에 위임한다.
struct WindFlyerHooks {
    std::function<std::int32_t(Sid)> runCount; // 현재 글자 프레임 구간의 개수다.
    std::function<void(std::uint32_t)> assertion; // 원본 period 진단의 Flyer.cpp 행 번호다.
    std::function<float(Sid,std::uint32_t,std::uint32_t,float)> damageable; // event0 외에는 공통44b900으로 넘긴다.
    std::function<void(Sid,std::uint32_t)> destroy; // count80 이상/boss의 실제 가상10이다.
    std::function<void(float,float,std::uint32_t)> sound; // 실제 activeSound를 위치와 함께 재생한다.
    std::function<bool(Sid,Sid)> mayTarget; // 현재 Gun 문맥의 실제46af80이다.
    std::function<float(Sid,Sid)> damageRate; // 현재 source의 가상bc가 반환하는 DPS다.
    std::function<void(Sid,std::int32_t,std::uint8_t,Sid)> damage; // 실제 target 가상98에 전달한다.
    std::function<void(Sid)> weaken; // 실제421db0의 다리 수명/프레임 약화다.
};
// 현재 풀·해시·TYPE와 Kernel 시각을 공유하며 별도 공격 월드를 만들지 않는다.
class RawWindFlyer {
public:
    RawWindFlyer(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<std::uint32_t> soundFrames,std::span<const std::uint32_t> activeSounds,
        const double& now,const bool& boss,const std::uint32_t& frame,WindFlyerHooks hooks); // 같은188타입 자료를 확인한다.
    static void Specialize(FlyerProcessFields& fields); // 45e1c0의 기본 생성자 이후104BYTE 파생 쓰기다.
    void Initialize(FlyerProcessFields& fields) const; // 45e230의 최초 시각/공격/착륙/20초 대기다.
    std::int32_t Animation(FlyerProcessFields& fields) const; // 45c020의0.08초 주기와 signed 나머지다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float period) const; // 45e8b0의80회 공격/40회 소리/삭제다.
    void Attack(Sid sid,float period) const; // 45e9a0의한칸 탐색·10항목 저장·범위 피해다.
private:
    SidPool& pool_; // 현재 비행체와 후보가 공유하는 유일한 풀이다.
    const SquidHash& hash_; // 실제 일반finder의 네 단계 해시다.
    std::span<const RiftTypeRecord> types_; // 후보의 현재 genus를 조회한다.
    std::span<std::uint32_t> soundFrames_; // TYPE+1f0의 현재 프레임별 소리 억제 장부다.
    std::span<const std::uint32_t> activeSounds_; // 같은 TYPE 번호의 activeSound 토큰이다.
    const double& now_; // 같은 Kernel의 현재 게임 시각이다.
    const bool& boss_; // count80 이상에서만 실제 세션의 현재 삭제 권한을 읽는다.
    const std::uint32_t& frame_; // 같은 화면 프레임의 현재 번호다.
    WindFlyerHooks hooks_; // 원본이 선택한 효과의 실제 소비자다.
};
}
