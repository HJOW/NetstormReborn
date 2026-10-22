// 원본41e020과 특별 Wind 생산12단계의 현재 TYPE/SHP 원주 좌표를 복원한다.
#pragma once
#include "o/SidPool.h"
#include "o/RawWindAviaryState.h"
#include <functional>

namespace netstorm::o {
// SHP 추가 헤더에서 사용하는 실제 필드이며 VFX 사각형의 높이와 구별한다.
struct WindCircleHeader {
    float walkStepY{}; // +14의 float 높이 보정이다.
    std::int16_t height{},hotspotY{}; // +1a/+1e의 signed WORD다.
};
// 현재 생성 비행체의 가상 TYPE/추상 인수/프레임13에 대응하는 헤더를 읽는다.
using WindCircleHeaderRead=std::function<WindCircleHeader(Sid,std::uint32_t,std::int32_t)>;
// 360/count 정수 나눗셈·index DWORD 감김·원본 각도 상수·두 float 저장을 보존한다.
std::array<float,2> WindCirclePoint(float x,float y,std::int32_t count,std::int32_t index);
// 헤더 뒤 부모의 현재 중심/원문 count·index를 읽고 실제13번 높이·현재 발자국을 보정한다.
std::array<float,2> WindCirclePosition(const SidPool& pool,std::span<const RiftTypeRecord> types,Sid parent,Sid flyer,const WindAviaryProcessFields& fields,const WindCircleHeaderRead& header);
}
