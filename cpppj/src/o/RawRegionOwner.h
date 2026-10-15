// 현재 표면의 실제 소유자 getter를 미션 사제 구출 판정에 제공한다.
#pragma once
#include "o/SidPool.h"
#include <span>

namespace netstorm::o {
// PATCH0046f710/0046f770과 CD004bddb0/004bde10의 원본 표면/좌표 조회다.
class RawRegionOwner {
public:
    // 같은 Pool/hash0과 현재 isle 타입 DWORD를 빌린다. 자료는 처리기보다 오래 살아야 한다.
    RawRegionOwner(const SidPool& pool,std::span<const std::uint16_t> surfaces,const std::uint32_t& terrainType);
    // 정수 칸이256×256 밖이거나 표면이 없거나 현재 isle 타입이 아니면0이다.
    std::uint32_t At(std::int32_t x,std::int32_t y) const;
    // 원본 +0.99999f/ftol→float 저장→ftol 순서로 칸을 고른 뒤 At을 실행한다.
    std::uint32_t At(float x,float y) const;
    // 상위 실제 월드와 동일 풀인지 연결 시 확인한다.
    const SidPool& Pool() const;
private:
    // 실제 CRT의64비트 절삭 결과에서 signed 하위 DWORD를 얻는다. 무효 정수 결과의 하위 DWORD는0이다.
    static std::int32_t Truncate(double value);
    // 원본 snap helper가 float 필드에 저장한 뒤 호출자가 다시 절삭하는 순서다.
    static std::int32_t Snap(float value);
    const SidPool& pool_; // 소유자 BYTE를 현재 raw 슬롯에서 직접 읽는 동일 객체 풀이다.
    std::span<const std::uint16_t> surfaces_; // 같은 원본 hash0 WORD 배열이며 복사본을 만들지 않는다.
    const std::uint32_t& terrainType_; // 현재 isle 타입 전역이다. 연결 시의 값으로 고정하지 않는다.
};
}
