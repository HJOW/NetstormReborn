// 원본10.78의 가상+b0=498000을 같은 현재 풀/TYPE/물리SHP 헤더로 실행한다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidDisplay.h"
#include <functional>

namespace netstorm::client { // 선택용 출력은 화면의 배율/소유자색 캡처와 구별한다.
// 같은200×200 원색 버퍼를 기본 그림과 실제+a4가 순서대로 공유한다.
struct ObjectHitHooks {
    std::function<std::int32_t(o::Sid)> frame; // +78이며 화면+a0와 혼용하지 않는다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::int32_t,std::span<std::uint8_t>)> plain; // 4daa70의 현재TYPE/물리프레임/기준점을 그린다.
    std::function<void(o::Sid,std::int32_t,std::int32_t,std::span<std::uint8_t>)> additional; // 같은 버퍼에 실제+a4를 호출하며 미도달일 때 요구하지 않는다.
};
// 498000의 헤더 선택·좌표·opaqueCollide·200픽셀 캡처 순서를 복원한다.
class RawObjectHit {
public:
    RawObjectHit(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::span<const o::SquidDisplayShape> shapes,ObjectHitHooks hooks); // 현재 참조만 빌리며 객체를 생성하지 않는다.
    bool Hit(o::Sid sid,std::int32_t x,std::int32_t y,const std::array<std::int32_t,2>& camera) const; // 카메라는 같은 현재 snapshot의 signed 픽셀 원점이다.
private:
    const o::SidPool& pool_; // 실제 현재50BYTE 슬롯이다.
    std::span<const o::RiftTypeRecord> types_; // TYPE의 원본 flags/기준점이다.
    std::span<const o::SquidDisplayShape> shapes_; // 논리클러스터 수와 물리 헤더를 구별한다.
    ObjectHitHooks hooks_; // +78/plain/+a4의 같은 수명 소비자다.
};
}
