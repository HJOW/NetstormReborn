// 실제 썬 디스크83 가상 Handle의 TYPE 방향별 modulo 프레임 진행을 복원한다.
#pragma once
#include "o/RawArcherAnimation.h"
namespace netstorm::o {
// 글자별 프레임 구간과 구별하는 실제 +114/+110의 방향별 반복 처리기다.
class RawDiscRegular {
public:
    // 풀/TYPE 방향 자료/프레임 지정기는 이 처리기보다 오래 살아야 한다.
    RawDiscRegular(const SidPool& pool,std::span<const ArcherTypeData> types,const SquidFrame& setter);
    // 실제 signed modulo를 사용하며 정상 프레임 범위와 TYPE 분모를 확인한다.
    static std::int32_t NextFrame(std::int32_t current,std::int32_t frameCount,std::int32_t directions);
    // 실제 가상 표50b570/CD501440을 확인한다. 타입 번호만으로 부모를 추측하지 않는다.
    bool Handles(Sid sid) const;
    // PATCH00469540/CD00437290은 event/count를 사용하지 않고 현재 프레임 진행 후 payload를 반환한다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 다른 실제 부모 처리기를 보존하며 썬 디스크의 가상 Handle만 선택한다.
    RegularHandler Handler(RegularHandler fallback={}) const;
private:
    const SidPool& pool_; // 현재 프레임/가상 표를 매번 읽는 동일 풀이다.
    std::span<const ArcherTypeData> types_; // 현재 TYPE 클러스터 수와 방향 수다.
    const SquidFrame& setter_; // 실제 표시/공간 갱신을 수행하는 동일 지정기다.
};
}
