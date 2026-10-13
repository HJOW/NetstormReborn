// Damageable의 현재HP/최대HP 절반 비교와 실제burning 프로세스 생성/제거 분기를 복원한다.
#pragma once
#include "o/SquidReward.h"

namespace netstorm::o {
// 실제원본 burning process14 조회/생성/삭제와 bulf 피격음의 외부 경계다.
struct DamageableBurningHooks {
    // find: 현재 부모의burning process14에서 index번째 Form SID를찾는다.0은자연스러운없음이다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t)> find;
    // create: 실제burning 생성자00424090/CD004a6720의생성된Form SID를반환한다.
    std::function<Sid(Sid)> create;
    // destroy: 찾아낸burning Form을같은월드의실제삭제기로제거한다.
    std::function<void(Sid,std::uint32_t)> destroy;
    // bulfSound: 실제위치의bulfHurt.wav를재생한다.다른타입에는요구하지않는다.
    std::function<void(Sid,float,float)> bulfSound;
};
// 원본디버그검사와현재타입전역은호출할때다시읽는다.
struct DamageableBurningState {
    // checking: 패치만두번째burning이없는지확인하는원본전역이다.
    bool checking{};
    // burningType/bulfType: 원본process14/bulf135 전역의현재값이다.
    std::uint32_t burningType{14},bulfType{135};
};
// 0044c060/CD00462480 전체판정을수행하며nonzero 생성자몸체를묵시성공으로대체하지않는다.
class RawDamageableBurning {
public:
    // 실제공유HP모듈과동일판본의현재damageEffect를받는다.
    RawDamageableBurning(SidPool& pool,std::span<const RiftTypeRecord> types,const SquidReward& hp,
        DamageableBurningState& state,DamageableBurningHooks hooks);
    // free/damageEffect0 자연반환,저HP 생성·기존유지,회복후기존삭제를원본순서로수행한다.
    Sid Refresh(Sid victim) const;
    // 동일월드의다른피격몸체연결에사용한다.
    const SidPool& Pool() const;
private:
    // pool_: 실제피격자와burning Form을공유하는저장소다.
    SidPool& pool_;
    // types_: 현재타입의damageEffect/maxHP 자료다.
    std::span<const RiftTypeRecord> types_;
    // hp_: 원본분류/사제quarter 모드까지복원한최대HP모듈이다.
    const SquidReward& hp_;
    // state_: 실제검사/프로세스타입전역이다.
    DamageableBurningState& state_;
    // hooks_: 실제동일월드생성/삭제/소리경계다.
    DamageableBurningHooks hooks_;
};
}
