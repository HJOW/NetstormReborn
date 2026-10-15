// 현재 부모 raw VT의 실제 내용물 가상 +24를 분배한다.
#pragma once
#include "o/ContentFormHost.h"

namespace netstorm::o {
// 공통 빈 몸체와 파생 효과를 구분한다. 파생 callback은 실제 같은 월드의 구현을 공급해야 한다.
class RawContentParentNotice {
public:
    // 풀과 파생 callback 대상은 이 분배기보다 오래 살아야 한다.
    RawContentParentNotice(const SidPool& pool,ContentFormNotice derived={});
    // 현재 VT의 실제 진입점이다. 알려지지 않은 VT는 호출 전에 거부한다.
    std::uint32_t NativeEntry(Sid parent) const;
    // 기본 몸체는 그대로 반환하고, 파생 몸체는 실제 소비자 또는 명시 미복원 오류로 분배한다.
    void Notice(Sid parent,Sid form,std::uint32_t reason) const;
private:
    const SidPool& pool_; // 부모의 현재 raw VT를 읽는 같은 SID 저장소다.
    ContentFormNotice derived_; // 워커/다이스/워크샾/Player의 실제 파생 통지 경계다.
};
}
