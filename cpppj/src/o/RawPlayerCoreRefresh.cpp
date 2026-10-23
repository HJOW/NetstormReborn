// Coregetter별현재물리와iterator캐시/가상호출뒤현재Pool재조회를구별한다.
#include "o/RawPlayerCoreRefresh.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kStride=50,kType=10,kFlags=11,kKind=18,kListFlags=20,kOwner=34; // PATCH10.78 raw객체필드의실제위치다.
// 선택해서도달한자식만그위치에서필수로요구한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("현재Core갱신의실제하위장치미연결");
    return function;
}
// 원문DWORD비트를signed연산/비교에사용하고host signedoverflow를피한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 실제단일load폭만검사하며표끝/NULL을임의값으로대체하지않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::uint32_t offset,std::uint32_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("현재Core갱신의실제load물리범위");
    std::uint32_t value{};
    for (std::uint32_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8); // 지정폭전체를원문레지스터에캡처한다.
    return value;
}
// 같은store전체폭의host물리를확인한뒤그바이트만쓴다.
void Write(std::span<std::uint8_t> bytes,std::uint32_t offset,std::uint32_t width,std::uint32_t value) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("현재Core갱신의실제store물리범위");
    for (std::uint32_t byte=0;byte<width;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 이웃Core필드는그대로남긴다.
}
// 부모가항상value1로호출하는실제Ch0042b280의범위진단/마스크shift/같은this쓰기다.
void SetTechnology(std::span<std::uint8_t> core,std::int32_t bit,const PlayerCoreRefreshHooks& hooks) {
    if (bit<0 || bit>=32) Need(hooks.assertion)(0x15a);
    const auto old=Read(core,6,4);Write(core,6,4,old|(1U<<(static_cast<std::uint32_t>(bit)&31U)));
}
// 실제객체주소와접근가능한그슬롯의같은물리뷰를가상자식에게넘긴다.
PlayerCoreRefreshForm Form(const PlayerCoreRefreshPool& pool,std::uint32_t offset) {
    if (offset>pool.bytes.size()) throw std::out_of_range("현재Core갱신가상객체물리범위");
    return {pool.bytes.subspan(offset,std::min<std::size_t>(kStride,pool.bytes.size()-offset)),pool.address+offset};
}
// 최초0040eb40과후속인라인루프는1을증가한뒤cached상한안에서free bit0만건너뛴다.
void Advance(std::int32_t& index,std::int32_t cachedEnd,const PlayerCoreRefreshPool& pool) {
    index=Signed(static_cast<std::uint32_t>(index)+1U);
    while (index<cachedEnd) { // 현재capacity로최초cached상한을대체하지않는다.
        if (!(Read(pool.bytes,static_cast<std::uint32_t>(index)*kStride+kFlags,1)&1U)) return;
        index=Signed(static_cast<std::uint32_t>(index)+1U);
    }
}
}
// 기술순회를먼저완료하고같은현재Player의실제객체로Core+5하한을갱신한다.
void RefreshPlayerCore(const PlayerCoreRefreshHooks& hooks) {
    Write(Need(hooks.core)(),6,4,0);
    const auto parent=Need(hooks.technologyParent)();auto cursor=Need(hooks.begin)(parent);
    for (;;) { // 독립Finder의현재후보와이미보관한next를사용한다.
        const auto candidate=Need(cursor.current)();if (!candidate) break;
        const auto pool=Need(hooks.pool)();const auto offset=candidate*kStride;
        const auto flags=Read(pool.bytes,offset+kListFlags,1);const auto kind=Read(pool.bytes,offset+kKind,2); // 원문은flag분기전에도종류WORD를읽는다.
        if (flags&4U) {
            const auto bit=Need(hooks.technologyBit)(kind);
            if (bit!=-1) SetTechnology(Need(hooks.core)(),bit,hooks);
        }
        Need(cursor.next)();
    }
    const auto initial=Need(hooks.pool)();const auto cachedEnd=Signed(initial.capacity);std::int32_t index=1,maximum=1; // 실제iterator초기1과누적max초기1이다.
    Advance(index,cachedEnd,Need(hooks.pool)());auto pool=Need(hooks.pool)();
    for (;;) { // 유효관문은현재capacity를읽고free-skip만최초cached상한을유지한다.
        const auto offset=static_cast<std::uint32_t>(index)*kStride;const auto address=pool.address+offset;
        if (address<pool.address+kStride || address>=pool.address+Need(hooks.pool)().capacity*kStride) break;
        if (Read(pool.bytes,offset+kType,1)==Need(hooks.playerType)() && Read(pool.bytes,offset+kOwner,1)==Need(hooks.owner)()) {
            const auto first=Need(hooks.level)(Form(pool,offset));pool=Need(hooks.pool)();
            if (first>maximum && !(Read(pool.bytes,offset+kFlags,1)&2U)) {
                maximum=Need(hooks.level)(Form(pool,offset));pool=Need(hooks.pool)(); // 둘째반환을다시max와비교하거나첫째로대체하지않는다.
            }
        }
        Advance(index,cachedEnd,pool);
    }
    const auto lower=Signed(static_cast<std::uint32_t>(maximum)-1U);const auto first=Read(Need(hooks.core)(),5,1);
    if (lower>static_cast<std::int32_t>(first)) Write(Need(hooks.core)(),5,1,static_cast<std::uint32_t>(lower));
    else {
        const auto selected=Read(Need(hooks.core)(),5,1);Write(Need(hooks.core)(),5,1,selected); // 원문else는다른현재Core일수있는두번째load와세번째store다.
    }
}
}
