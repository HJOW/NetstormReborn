// State정리의0048fcd0이같은9Player기술링크/목록개수/동적Deck을반납한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// Player할당과동적Deck소유자를부모보다오래살려두고현재저장소를직접빌린다.
struct PlayerLinksCleanupHooks {
    std::function<std::span<std::uint8_t>()> players; //0059534c의현재물리를각원문load/store직전에순수조회한다.
    std::function<void(std::uint32_t)> release; //비0 Player+68토큰을같은실제Deck소유자에반납한다.Player필드는여기서지우지않는다.
};
//0048fcd0:Player0..8각+9c0→+80→+68비0일때free→현재base+680이다.배열/Player자체는반납하지않는다.
void CleanupPlayerLinks(const PlayerLinksCleanupHooks& hooks);
}
