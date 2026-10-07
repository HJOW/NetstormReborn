// 원본 Bridge.cpp의 삭제 전후 훅과 낙하 이벤트 좌표 인코딩. 세 PE의 제한 x86 기대값으로 검사한다.
#include "o/RawBridgeLifecycle.h"
#include "o/RawSquidFinder.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 각 후보의 현재 raw 필드를 읽는 일반 탐색기를 기존 순서 있는 삭제 훅 인터페이스에 공급한다.
BridgeLifecycleHooks MakeBridgeLifecycleHooks(RawSquidFinder& finder,
    std::function<void(const BridgeLifecycleEvent&)> emit) {
    return {
        // 다리 훅의 정수 탐색 범위를 원본 일반 사각형 Begin에 그대로 넘긴다.
        [&finder](BridgeLifecycleSearch area) { return finder.Begin({area.left,area.top,area.right,area.bottom}); },
        // 이전 후보의 삭제 뒤 다음 raw 체인/버킷을 읽는다.
        [&finder] { return finder.Next(); },std::move(emit)
    };
}
namespace {
// 공통 슬롯 오프셋과 extra/state/genus 마스크. 링크의 첫 참조는 수명 필드와 같은 +12 WORD다.
constexpr std::size_t kType = 10,kState = 11,kFirstReference = 12,kSecondReference = 8,kX = 14,kY = 18;
constexpr std::uint8_t kDead = 2,kAbstractOrBuried = 9;
constexpr std::uint32_t kWalker = 0x10000;
// 00502310 ↔ CD 005017a0의 실제 float 비트. preDestroy 범위의 거의 올림이며 표면 조회의 0.9999와도 다르다.
constexpr float kAreaCoordinateBias = std::bit_cast<float>(0x3f7fff58u);
// little endian raw DWORD/WORD를 정렬된 호스트 포인터 없이 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t size) {
    std::uint32_t value = 0;
    // 원본 바이트 순서대로 합친다.
    for (std::size_t i=0;i<size;++i) value |= static_cast<std::uint32_t>(raw[offset+i]) << (i*8);
    return value;
}
// raw 좌표의 float 비트를 그대로 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) { return std::bit_cast<float>(Read(raw,offset,4)); }
// 정상 월드 좌표의 0 방향 절삭이다. ±1 확장까지 int 범위 안이어야 한다(새 코드의 보호).
int Cell(float value,float bias=0.0f) {
    // x87 덧셈 뒤 _ftol과 같이 중간값을 float로 다시 좁히지 않는다.
    const double wide = static_cast<double>(value)+static_cast<double>(bias);
    if (!std::isfinite(wide) || wide <= std::numeric_limits<int>::min() || wide >= std::numeric_limits<int>::max())
        throw std::out_of_range("Bridge lifecycle coordinate");
    return static_cast<int>(wide);
}
// 외부 효과가 실제로 소비되고 순서 있는 탐색을 끝낼 수 있는지 먼저 확인한다.
void ValidateHooks(const BridgeLifecycleHooks& hooks,bool ordinary) {
    if (!hooks.emit || (ordinary && (!hooks.begin || !hooks.next))) throw std::invalid_argument("Bridge lifecycle hooks");
}
// 원본 CRT _ftol은 64비트 절삭값의 하위 DWORD를 사용한다. 비유한/64비트 밖은 하위 0이다.
std::uint32_t FtolLow(float value) {
    const double wide = value;
    if (!(wide > -9223372036854775808.0 && wide < 9223372036854775808.0)) return 0;
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(wide)));
}
}
// 타입 표의 번호 인덱스는 원본 getGenus와 같다. 포인터 수명은 호출자가 보장한다.
RawBridgeLifecycle::RawBridgeLifecycle(const SidPool& pool,std::span<const RiftTypeRecord> types):pool_(pool),types_(types) {}
// 자기 번호 0은 탐색 종료 표식이며 객체로 호출할 수 없다.
std::span<const std::uint8_t> RawBridgeLifecycle::Object(Sid sid) const {
    if (sid.value==0 || sid.value>=pool_.Capacity()) throw std::out_of_range("Bridge lifecycle SID");
    return pool_.Slot(sid);
}
// buried/abstract는 다리 고유 효과만 건너뛰고 공통 삭제 단계에는 계속 도달한다.
bool RawBridgeLifecycle::Ordinary(Sid sid) const {
    return (Object(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&kAbstractOrBuried)==0;
}
// free/void 비트를 추가로 걸러내지 않는다. 원본은 유효 번호와 첫 참조 extra, 두 dead 비트만 본다.
bool RawBridgeLifecycle::LinkNeedsDestroy(Sid link) const {
    const auto raw = Object(link);
    const Sid first{static_cast<std::uint16_t>(Read(raw,kFirstReference,2))};
    const Sid second{static_cast<std::uint16_t>(Read(raw,kSecondReference,2))};
    if (first.value==0 || first.value>=pool_.Capacity() || second.value==0 || second.value>=pool_.Capacity()) return false;
    return Ordinary(first) && ((Object(first)[kState]|Object(second)[kState])&kDead)!=0;
}
// 탐색 시작 후 매 항목에서 현재 타입·참조를 다시 읽는다. destroy 훅의 변경을 다음 판단에 반영한다.
void RawBridgeLifecycle::PreDestroy(Sid bridge,std::uint32_t flags,std::uint32_t linkType,const BridgeLifecycleHooks& hooks) const {
    const bool ordinary = Ordinary(bridge);
    ValidateHooks(hooks,ordinary);
    if (ordinary) {
        const auto raw = Object(bridge);
        const int x = Cell(Coordinate(raw,kX),kAreaCoordinateBias),y = Cell(Coordinate(raw,kY),kAreaCoordinateBias);
        // 원본 finder 순서와 0 종료 표식을 그대로 사용한다.
        for (Sid current=hooks.begin({x-1,y-1,x+1,y+1});current.value!=0;current=hooks.next()) {
            if (Object(current)[kType]==linkType && LinkNeedsDestroy(current))
                hooks.emit({BridgeLifecycleEffect::DestroyLink,current,0});
        }
    }
    hooks.emit({BridgeLifecycleEffect::BasePreDestroy,bridge,flags});
}
// 제거 통지/소리가 풀 좌표를 바꿨을 수도 있으므로 그 뒤 좌표를 다시 읽어 다음 효과에 넘긴다.
void RawBridgeLifecycle::PostDestroy(Sid bridge,std::uint32_t flags,const BridgeLifecycleHooks& hooks) const {
    const bool ordinary = Ordinary(bridge);
    ValidateHooks(hooks,ordinary);
    if (ordinary) {
        hooks.emit({BridgeLifecycleEffect::NotifyRemoval,bridge,0});
        auto raw = Object(bridge);
        hooks.emit({BridgeLifecycleEffect::FallSound,bridge,0,Coordinate(raw,kX),Coordinate(raw,kY)});
        raw = Object(bridge);
        const int x = Cell(Coordinate(raw,kX)),y = Cell(Coordinate(raw,kY));
        // walker 여부도 낙하 훅이 바꾼 뒤의 현재 타입 표/슬롯을 기준으로 다시 읽는다.
        for (Sid current=hooks.begin({x,y,x,y});current.value!=0;current=hooks.next()) {
            const auto type = Object(current)[kType];
            if (type>=types_.size()) throw std::out_of_range("Bridge lifecycle type");
            if ((types_[type].flags2&kWalker)!=0) hooks.emit({BridgeLifecycleEffect::FallWalker,current,0});
        }
    }
    hooks.emit({BridgeLifecycleEffect::BasePostDestroy,bridge,flags});
}
// 포장된 signed DWORD의 비트가 아니라 그 수치를 float로 바꾸는 원식을 보존한다.
float RawBridgeLifecycle::DelayedFallPayload(float x,float y) {
    const std::uint32_t packed = (FtolLow(x)&0xffu)|(FtolLow(y)<<8);
    return static_cast<float>(std::bit_cast<std::int32_t>(packed));
}
}
