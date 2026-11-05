// 진단 뒤 바뀐 전역/표 기준과 저장된 TYPE this의 HP 읽기를 원문 순서대로 보존한다.
#include "o/RawRiftHitPoints.h"
#include <limits>
#include <stdexcept>

namespace netstorm::o { // 호스트 포인터를 원본 PE 주소로 역참조하지 않는다.
namespace { // 선택한 callable은 자기 연결이 교체돼도 호출 끝까지 보존한다.
template<class F> F HpNeed(const F& function) { if (!function) throw std::logic_error("TYPE HP의 선택된 실제 장치 미연결");return function; } // 미선택 getter/진단은 요구하지 않는다.
constexpr std::string_view kHpExpression="(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType"; // 원문1e3의 식이다.
constexpr std::string_view kHpSource="\\Ns\\O\\Rifttype.cpp"; // 대소문자를 포함한 원문 진단 소스다.
}

std::int32_t ReadRiftHitPoints(OriginalEdition edition,const RiftTypeRecord& type,const RiftHitPointGlobals& globals,const RiftHitPointHooks& hooks) { // CD는 checking/예외 진단을 추가하지 않는다.
    if (edition==OriginalEdition::Patch1078 && globals.checking && !(type.flags1&TypeFlag1::kHasHitPoints)) {
        const auto number=HpNeed(hooks.number)(); // 현재 base의 번호를 계산한 뒤에 예외 TYPE DWORD를 읽는다.
        if (number!=globals.dais) HpNeed(hooks.assertion)(kHpExpression,kHpSource,0x1e3);
    }
    bool quarter=false;
    if (globals.quarter) {
        const auto number=HpNeed(hooks.number)(); // 진단 복귀 뒤 현재 base를 다시 읽고 그 뒤 현재 특별 TYPE를 비교한다.
        quarter=number==globals.priest;
    }
    return quarter ? type.maxHitPoints/4 : type.maxHitPoints; // signed 나눗셈은 음수도 원문처럼0방향으로 절삭한다.
}

std::uint32_t CurrentRiftTypeNumber(const RiftTypeRecord& type,std::span<const RiftTypeRecord> selectedBase,std::span<const RiftTypeRecord> currentBase) { // 겹치는 유효 뷰만 같은 원문 주소 영역으로 해석한다.
    const auto original=reinterpret_cast<std::uintptr_t>(selectedBase.data()),current=reinterpret_cast<std::uintptr_t>(currentBase.data()),record=reinterpret_cast<std::uintptr_t>(&type);
    constexpr auto stride=sizeof(RiftTypeRecord); // 호스트 Record 한 칸을 원문의500/468바이트 한 칸에 대응시킨다.
    const auto maximum=std::numeric_limits<std::uintptr_t>::max();
    if (selectedBase.empty() || currentBase.empty() || selectedBase.size()>maximum/stride || currentBase.size()>maximum/stride || original>maximum-selectedBase.size()*stride || current>maximum-currentBase.size()*stride)
        throw std::out_of_range("TYPE HP 표의 물리 주소 범위");
    const auto originalEnd=original+selectedBase.size()*stride,currentEnd=current+currentBase.size()*stride;
    const auto distance=current>=original ? current-original : original-current;
    if (record<original || record>=originalEnd || (record-original)%stride || distance%stride || original>=currentEnd || current>=originalEnd)
        throw std::logic_error("TYPE HP의 서로 다른 물리 표 주소 연결");
    const auto number=static_cast<std::int64_t>((record-original)/stride);
    const auto shift=static_cast<std::int64_t>(distance/stride)*(current>=original ? 1 : -1);
    return static_cast<std::uint32_t>(number-shift); // 원문 getNum의 음수 DWORD도 현재 특별 TYPE와 비트 그대로 비교한다.
}
}
