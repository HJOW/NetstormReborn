// 원본 Squid의 hasLevel/getLevel/setLevel을 실제 슬롯 상태BYTE에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/RiftType.h"
#include <functional>

namespace netstorm::o {
// 원본 setter는 범위 진단 후에도 BYTE 저장을 계속한다.
enum class SquidLevelAssertion { OutOfRange }; // 실제 signed n<0 또는 n>=8의 진단이다.
// PATCH004ad0a0/004ad0f0/004ad110 및 CD004acdc0/004ace00/004ace20의 base 몸체다.
class RawSquidLevel {
public:
    // Pool/TYPE/진단 경계는 객체보다 오래 살아야 한다. 가상 분배기는 실제 base+80/+84인지를 먼저 선택한다.
    RawSquidLevel(SidPool& pool,std::span<const RiftTypeRecord> types,std::function<void(SquidLevelAssertion)> assertion);
    // 원본 hasLevel의 OR 조건 때문에 정상 TYPE에서는 항상true다. intended AND로 수정하지 않는다.
    bool Has(Sid sid) const;
    // 현재 상태BYTE의 상위3비트를 zero-extend한다.
    std::uint32_t Get(Sid sid) const;
    // signed 범위 진단 뒤 현재BYTE의 하위5비트를 보존하고 n의 하위3비트를 저장한다.
    void Set(Sid sid,std::uint32_t value) const;
    // 다른 월드의 상태 setter 연결을 거부하는 실제 참조다.
    const SidPool& Pool() const;
private:
    SidPool& pool_; // 실제 할당된 자산 슬롯 저장소다.
    std::span<const RiftTypeRecord> types_; // 현재 raw type의 원본 TYPE 조회 범위를 확인한다.
    std::function<void(SquidLevelAssertion)> assertion_; // 원본 진단 후 현재 슬롯 변경도 보존한다.
};
}
