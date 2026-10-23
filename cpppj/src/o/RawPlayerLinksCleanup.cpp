// free가현재Playerbase를바꿔도반납인자와뒤의0쓰기대상을원문대로구별한다.
#include "o/RawPlayerLinksCleanup.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kPlayerCount=9,kStride=180,kTechnology=0x9c,kCount=8,kDeck=0x68; // 실제PATCH9Player의선택된세DWORD위치다.
// 실제도달한현재물리/반납장치만그위치에서요구한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Player링크정리의선택된실제장치미연결");
    return function;
}
// 실제선택DWORD의전체load폭을확인하고같은값을레지스터에보관한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::uint32_t at) {
    if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("Player링크정리의실제load물리범위");std::uint32_t value{};
    for (std::uint32_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8); // 낮은BYTE부터정확히4BYTE만읽는다.
    return value;
}
// 원문단일DWORD store전에현재물리를확인하고앞선store는복구하지않는다.
void Zero(std::span<std::uint8_t> bytes,std::uint32_t at) {
    if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("Player링크정리의실제store물리범위");
    for (std::uint32_t byte=0;byte<4;++byte) bytes[at+byte]=0; // 같은Player의이웃필드는보존한다.
}
}
// 현재owner관문이나기술객체삭제를추가하지않고고정9기록을순서대로처리한다.
void CleanupPlayerLinks(const PlayerLinksCleanupHooks& hooks) {
    for (std::uint32_t owner=0;owner<kPlayerCount;++owner) { // 재진입은별도현재루프위치를가지며같은현재물리를본다.
        const auto offset=owner*kStride;
        Zero(Need(hooks.players)(),offset+kTechnology);Zero(Need(hooks.players)(),offset+kCount);
        const auto deck=Read(Need(hooks.players)(),offset+kDeck);
        if (deck) { Need(hooks.release)(deck);Zero(Need(hooks.players)(),offset+kDeck); } // free복귀뒤현재base를다시읽고0일때는후속0쓰기자체도생략한다.
    }
}
}
