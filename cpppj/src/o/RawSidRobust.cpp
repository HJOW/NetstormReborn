// 같은현재Pool과진단DWORD를사용하며범위진단과SID유효반환을구별한다.
#include "o/RawSidRobust.h"
#include <limits>
#include <stdexcept>

namespace netstorm::o { // 원문DWORD주소와Win64의실제물리범위를구별하는공통진단이다.
namespace { // 선택된원문접근에서만하위장치와물리를요구한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("SID 원문 진단의 선택 장치 미연결");return function; } //checking0/live0/번호실패는메모리장치없이끝날수있다.
std::uintptr_t Address(std::uintptr_t base,std::uint32_t delta,SidRobustAddressMode mode) { // 원문ADD32와실제호스트포인터산술의폭을명시분리한다.
    if (mode==SidRobustAddressMode::Original32) return static_cast<std::uint32_t>(static_cast<std::uint32_t>(base)+delta);
    if (delta>std::numeric_limits<std::uintptr_t>::max()-base) throw std::out_of_range("SID 실제 호스트 주소 범위");return base+delta;
}
std::uint32_t Read(std::uintptr_t token,std::size_t width,const SidRobustHooks& hooks) { // 같은필드의선택시점현재BYTE를읽으며타입접근을범위검사뒤로옮기지않는다.
    const auto bytes=Need(hooks.memory)(token,width);if (bytes.size()<width) throw std::out_of_range("SID 원문 진단의 선택 물리 범위");std::uint32_t value{};
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(bytes[index])<<(index*8); // 실제선택폭은1또는4이며little endian원문DWORD를읽는다.
    return value;
}
}
// 최근진단0/현재type선행저장후같은base를하한/상한에사용하고필드실패bit를순서대로쓴다.
bool RawSquidIsRobust(std::uintptr_t object,std::int32_t live,RawSidRobustDiagnostics& diagnostics,const SidRobustHooks& hooks) {
    if (hooks.addressMode==SidRobustAddressMode::Original32) object=static_cast<std::uint32_t>(object);
    std::uint32_t failures{};diagnostics.lastRobust=0;diagnostics.type=Read(Address(object,10,hooks.addressMode),1,hooks);
    const auto base=Need(hooks.poolBase)();if (object<Address(base,50,hooks.addressMode)) { diagnostics.lastRobust=1;return false; }
    const auto limit=Need(hooks.capacity)()*std::uint32_t{50};if (object>=Address(base,limit,hooks.addressMode)) { diagnostics.lastRobust=1;return false; }
    if (live) {
        const auto table=Read(object,4,hooks);if (!table || table==0xddddddddU || table==0xcdcdcdcdU) { diagnostics.lastRobust=2;failures=1; }
        if (Read(Address(object,11,hooks.addressMode),1,hooks)&1U) { diagnostics.lastRobust|=4; ++failures; }
        if (Read(Address(object,11,hooks.addressMode),1,hooks)&2U) { diagnostics.lastRobust|=8; ++failures; }
        if (!Read(Address(object,10,hooks.addressMode),1,hooks)) { diagnostics.lastRobust|=0x10; ++failures; }
    } else {
        if (!Read(object,4,hooks)) { diagnostics.lastRobust=2;failures=1; }
        if (!(Read(Address(object,11,hooks.addressMode),1,hooks)&1U)) { diagnostics.lastRobust|=4; ++failures; }
    }
    return failures==0;
}
// 원문번호범위실패는0이고live0/checking0은Pool에접근하지않는다.진단복귀후에는1이다.
bool RawIsSidRobust(std::int32_t sid,std::int32_t live,RawSidRobustDiagnostics& diagnostics,const SidRobustHooks& hooks) {
    if (sid<1 || sid>65534) return false;if (!live) return true;
    if (Need(hooks.checking)()) {
        const auto object=Address(Need(hooks.poolBase)(),static_cast<std::uint32_t>(sid)*std::uint32_t{50},hooks.addressMode);
        if (!RawSquidIsRobust(object,1,diagnostics,hooks)) Need(hooks.assertion)("resolve( sid ).isRobust()",0x218);
    }
    return true;
}
}
