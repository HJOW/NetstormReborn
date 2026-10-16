// PATCH 썬 펜스의 방향별 최근접 검색과 실제 연결 개수를 같은 공간 해시에 연결한다.
#pragma once
#include "o/RawSquidFinder.h"
#include <array>

namespace netstorm::o {
// 이웃 검색의 TYPE 사거리와 방향별9×9 동맹은 현재 원본 저장소를 빌린다.
class RawFenceNeighbors {
public:
    RawFenceNeighbors(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const float> ranges,std::span<const std::uint32_t> alliances); // PATCH의 실제 타입/행렬 크기를 확인한다.
    std::array<float,4> SearchBounds(Sid sid,std::uint32_t direction) const; // 원본 Rect min/max의 현재 float 경계다.
    Sid Select(Sid sid,std::span<const Sid> candidates,bool allied) const; // finder가 반환한 순서대로 엄격한 최근접 한 개를 선택한다.
    Sid Neighbor(Sid sid,std::uint32_t direction,bool allied=false) const; // 00456220 또는00455fb0을 같은 실제 finder로 실행한다.
    bool WithinRange(Sid sid,std::array<float,4> endpoints) const; // 00455f00의 축 거리 절삭과 사거리 포함 판정이다.
    std::uint32_t Connections(Sid sid) const; // 004573d0의0/2/4/6 동맹 이웃 연결 개수다.
private:
    std::span<const std::uint8_t> Slot(Sid sid) const; // 원본 유효 포인터 범위와 현재 타입을 확인한다.
    std::array<float,2> Position(Sid sid) const; // 실제 Pop 좌표를 읽는다.
    std::array<float,2> Center(Sid sid) const; // TYPE 발자국 반쪽을 뺀 실제 float 중심이다.
    std::int32_t Range(Sid sid) const; // TYPE+44의 현재 float를 원본 CRT 절삭한다.
    const SidPool& pool_; // 동적 후보의 같은50BYTE 풀이다.
    const SquidHash& hash_; // 후보 순서를 정하는 같은4단계 해시다.
    std::span<const RiftTypeRecord> types_; // 현재 발자국/타입 번호 표다.
    std::span<const float> ranges_; // 실제 TYPE+44를 타입 번호로 조회한다.
    std::span<const std::uint32_t> alliances_; // 후보 소유자 행→원본 소유자 열의 현재81DWORD다.
};
}
