// Player 포인터는 진입 시 읽고, 데모 조회 뒤에도 저장해 둔 같은 brain의 현재 지갑을 다시 읽는다.
#include "o/RawAiWallet.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kPlayerBytes=180,kPlayerSlots=9,kBrainOffset=0xb0,kWalletOffset=0x2d8; // 실제 Player 표와 brain의 지정 DWORD 위치다. 전체 플레이어 수 상수와 슬롯 수를 구별한다.
constexpr std::int32_t kDemoFloor=1190; // 원본0x4a6의 signed 하한이며 일반 미션에서는 적용하지 않는다.
// 비정렬 원문 DWORD를 같은 바이트에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    // 네 BYTE만 낮은 비트부터 합친다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(8*index);return value;
}
// 실제 지갑 DWORD만 바꾸고 다른 AI 본체 바이트를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 원본 little endian 네 BYTE만 쓴다.
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index));
}
}

// 값형 기본 모드 대신 같은 현재 조회기 둘을 모두 연결한다.
RawAiWallet::RawAiWallet(RawAiBrainMemory& memory,std::span<const std::uint8_t> players,AiWalletReads reads)
    :memory_(memory),players_(players),reads_(std::move(reads)) {
    if (players.size()!=kPlayerBytes*kPlayerSlots || !reads_.disabled || !reads_.demo) throw std::invalid_argument("AI 지갑의 실제 Player9/현재 모드 조회 누락");
}
// 해제되거나 실제 크기 없는 brain을 지갑 사본으로 대신하지 않는다.
std::span<std::uint8_t> RawAiWallet::Body(std::uint32_t token) const {
    auto bytes=memory_.Bytes(token);if (bytes.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 지갑의 실제0xb08 본체 누락");return bytes;
}
// 데모 getter가 Player 포인터나 지갑을 바꾸어도 원본 ESI의 저장 포인터와 후속 재조회를 유지한다.
void RawAiWallet::Credit(std::uint32_t owner,std::int32_t amount) const {
    if (reads_.disabled() || std::bit_cast<std::int32_t>(owner)<1) return;
    if (owner>=kPlayerSlots) throw std::out_of_range("AI 지갑의 실제 Player9 범위");
    const auto token=Read(players_,static_cast<std::size_t>(owner)*kPlayerBytes+kBrainOffset);if (!token) return;
    auto body=Body(token);Put(body,kWalletOffset,Read(body,kWalletOffset)+std::bit_cast<std::uint32_t>(amount));
    if (reads_.demo()) { body=Body(token);const auto current=std::bit_cast<std::int32_t>(Read(body,kWalletOffset));Put(body,kWalletOffset,static_cast<std::uint32_t>(current>kDemoFloor ? current : kDemoFloor)); }
}
// 현재Player포인터의같은본체를읽는다.비활성/비양수owner/NULLbrain은원문처럼0을반환한다.
std::int32_t RawAiWallet::Balance(std::uint32_t owner) const { if (reads_.disabled() || std::bit_cast<std::int32_t>(owner)<1) return 0;if (owner>=kPlayerSlots) throw std::out_of_range("AI 지갑 조회 실제Player9 범위");const auto token=Read(players_,static_cast<std::size_t>(owner)*kPlayerBytes+kBrainOffset);return token ? std::bit_cast<std::int32_t>(Read(Body(token),kWalletOffset)) : 0; }
}
