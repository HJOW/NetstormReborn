// 원본10.78 운반물의 내려놓기 위치426a70과 발자국 탐색49bfe0을 복원한다.
#pragma once
#include "o/RawCanonPlacementPipeline.h"
#include "o/RawGunTarget.h"

namespace netstorm::client {
struct CarrierMenuHooks; // 기존 실제 운반물 메뉴의 위치 조회 경계다.
// 초기 PE의 지도 한계와 실패점은 현재 값으로 읽는다. 실패점은 임의의 -1로 치환하지 않는다.
struct CarrierDropPointState {
    std::int32_t mapLimit{256}; // 00531928: signed 정수를 float로 저장한 뒤 엄격한 상한 비교에 쓴다.
    std::array<float,2> invalidPoint{}; // 00545370/374: 발자국/주변 탐색 실패 때 그대로 복사하는 현재 전역이다.
};
// 전체 배치 검사와 실제 원본 중심만 외부 몸체이며 좌표 선택/반복/유효성은 이 클래스가 계산한다.
struct CarrierDropPointHooks {
    std::function<bool(const o::CanonPlacementQuery&)> mayPlace; // 실제0049b510의 타입·argument·flags·owner·mode를 유지한다.
    std::function<std::array<float,2>(o::Sid)> center; // 실제004adea0의 현재 발자국 중심이다.
};
// 같은 실제 Pool/TYPE와 현재 전역을 사용하며 객체가 free/dead라는 새 필터를 추가하지 않는다.
class RawCarrierDropPoint {
public:
    // 모든 참조와 콜백의 실제 저장소는 이 객체보다 오래 살아야 한다. PATCH 전용이다.
    RawCarrierDropPoint(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
        const CarrierDropPointState& state,CarrierDropPointHooks hooks);
    // 현재 점 즉시 허용→북/동/남/서→대각선 순서다.100 타입은 실제 중심을 거의 올림해 바로 반환한다.
    std::array<float,2> Find(o::Sid carrier,std::uint32_t type) const;
    // 49bfe0: x 바깥/y 안쪽 순서로 대상 타입의 현재 footX/Y를 순회하고 첫 허용점을 반환한다.
    std::array<float,2> FindFootprint(std::uint32_t type,std::uint32_t argument,float x,float y,
        std::uint32_t flags,std::uint32_t owner) const;
    // 41d7a0: 각 float에 원본 bias를 기본53비트 x87 정밀도로 더하고 CRT 하위 signed DWORD를 float로 저장한다.
    static std::array<float,2> Snap(std::array<float,2> point);
    // 같은 실제 세션의 위치/메뉴/배치 검사 연결 여부를 검증할 때 사용한다.
    const o::SidPool& Pool() const;
private:
    const o::RiftTypeRecord& Type(std::uint32_t type) const; // 현재 타입 표의 같은 항목을 조회한다.
    std::uint32_t Owner(o::Sid carrier) const; // 매 주변 탐색 진입에서 현재 소유자 BYTE를 다시 읽는다.
    bool Valid(std::array<float,2> point) const; // 현재 signed 지도 한계와 두 축의 엄격한 ordered 비교다.
    const o::SidPool& pool_; // 실제50바이트 슬롯의 SID 풀이다.
    std::span<const o::RiftTypeRecord> types_; // 현재 flags2/footX/footY를 제공하는 같은 타입 표다.
    const CarrierDropPointState& state_; // 같은 지도 한계와 실패점 전역이다.
    CarrierDropPointHooks hooks_; // 실제 배치/중심 몸체의 외부 연결이다.
};
// 전체 일반 배치 파이프라인과 기존 실제 중심 조회기를 같은 풀로 연결한다.
CarrierDropPointHooks MakeCarrierDropPointHooks(const o::SidPool& pool,o::RawCanonPlacementPipeline& placement,
    const o::RawGunTarget& centers);
// 실제 메뉴의 위치 조회 경계만 연결하며 다른 세션 풀은 즉시 거부한다.
void BindCarrierMenuDropPoint(CarrierMenuHooks& hooks,const o::SidPool& pool,const RawCarrierDropPoint& point);
}
