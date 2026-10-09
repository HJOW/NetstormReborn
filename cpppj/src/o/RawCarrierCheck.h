// Carrier의 가상 +0xcc 지면 조회를 복원하고 실제 사제 +0xc8 분배와 합성한다.
#pragma once
#include "o/RawCarrierPostPop.h"
#include "o/RawPriestSpawn.h"

namespace netstorm::o {
// 타입/지면이 허용할 때만 walker 낙하를 요청하는 조회 객체다. 상태/HP는 자체 조건이 아니다.
class RawCarrierCheck {
public:
    // 표는 참조하며 생성 뒤에도 현재 genus/spot을 읽는다. 참조와 낙하 함수 대상은 더 오래 살아야 한다.
    RawCarrierCheck(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,
        std::function<bool(Sid)> fallWalker);
    // genus & 0x320000이면 즉시 거짓, 거의 올림 spot & 6이면 거짓, 그 외에는 +0xc8의 0 아님을 반환한다.
    // 원본: 00426fc0 / CD 004e50d0. 사제의 실제 genus 0x210000은 좌표/낙하를 읽지 않고 제외된다.
    bool Check(Sid sid) const;
    // 실제 postPop/생성 연결에서 다른 월드의 풀 혼합을 거부한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const std::uint8_t> spots_;
    std::function<bool(Sid)> fallWalker_;
};
// postPop의 +0xcc만 실제 조회로 바꾸며 지면 소유/공통 장부 경계는 유지한다.
CarrierPostPopHooks MakeCarrierCheckPostPopHooks(const SidPool& pool,const RawCarrierCheck& check,CarrierPostPopHooks hooks);
// 사제 나선 생성의 genus 래퍼 다음 +0xcc만 같은 실제 조회로 연결한다.
PriestSpawnHooks MakeCarrierCheckSpawnHooks(const SidPool& pool,const RawCarrierCheck& check,PriestSpawnHooks hooks);
}
