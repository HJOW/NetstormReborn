// 원본은 프레임·소유자·Graph를 변경하지 않고 현재 genus를 확인한 뒤 섬 WORD만 쓴다.
#include "o/RawSquidRegion.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 Squid의 타입 BYTE·섬 WORD와 세 표면 genus 묶음/다리 비트다.
constexpr std::size_t kType=10,kRegion=8;
constexpr std::uint32_t kSurfaceGenus=0x01000006,kBridge=4;
}
// 같은 현재 raw 슬롯과 타입의 판본 차이를 보존한다.
void SetSquidRegion(SidPool& pool,std::span<const RiftTypeRecord> types,Sid sid,std::uint32_t region,
    std::uint32_t noRegion,const std::function<void(SquidRegionAssert)>& diagnostic) {
    const auto raw=pool.Slot(sid);if (raw[kType]>=types.size()) throw std::out_of_range("섬 번호 지정 타입 범위 오류");
    const auto genus=types[raw[kType]].flags2;
    if ((genus&kSurfaceGenus)==0 || (genus&kBridge)!=0) {
        if (pool.Edition()==OriginalEdition::Patch1078 && ((genus&kSurfaceGenus)==0 || region!=noRegion)) {
            if (!diagnostic) throw std::logic_error("섬 번호 지정 원본 진단 경계 누락");
            diagnostic((genus&kSurfaceGenus)==0 ? SquidRegionAssert::UnsupportedGenus : SquidRegionAssert::BridgeRegion);
        }
        return;
    }
    auto target=pool.AllocatedBytes(sid);target[kRegion]=static_cast<std::uint8_t>(region);target[kRegion+1]=static_cast<std::uint8_t>(region>>8);
}
}
