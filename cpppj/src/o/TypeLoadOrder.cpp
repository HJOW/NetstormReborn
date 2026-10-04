// PE 읽기 전용 추출: 패치판 00540dd0, CD판 0051c6f8. 게임 프로세스 타입 목록은 별도다.
#include "o/RiftType.h"
#include <array>

namespace netstorm::o {
namespace {
// 원본의 자산 로딩 순서 116개. CD판 목록은 이 배열의 앞 101개와 동일하다.
constexpr std::array<std::string_view, 116> kAssetTypes{
    "dude", "sunArcher", "sunAviary", "sunFlyer", "banner",
    "windBattery", "rainBattery", "thunderBattery", "sunBlocker", "thunderBlocker",
    "blankMissile", "bolt", "bridge", "sunDisc", "emptyGeyser",
    "sunFactory", "windFactory", "rainFactory", "thunderFactory", "residence",
    "fencemark", "flag", "fortGump", "icon", "island",
    "islandStalag", "playerBanner", "isle", "isleBig", "geyserBrightener",
    "treeTwo", "treeThree", "fringe", "mana", "range",
    "manabolt", "flare", "puzzlePiece", "playerIsland", "battleIsland",
    "particlePlaceHolder", "sunBalloon", "buried", "bombExplodeSmall", "bombExplodeMedium",
    "bombExplodeLarge", "bombHeal", "bombInvisible", "bombParalyze", "bombHardener",
    "bombTreason", "edgeFarm", "geyser", "windVortex", "rainVortex",
    "thunderVortex", "outpost", "mcloud", "sunCannon", "rainCannon",
    "rainCannonMissile", "thunderCannon", "thunderCannonMissile", "sunWalker", "teleportEffect",
    "bulf", "sunFence", "windWalker", "windFlyer", "windAviary",
    "windArcher", "windBalloon", "windBlocker", "rainAviary", "rainFlyer",
    "rainFence", "rainBalloon", "rainBlocker", "thunderArcher", "thunderFence",
    "growingRainBlocker", "platform", "player", "mog", "nugget",
    "bridgeConnector", "rainWalker", "noIsland", "priest", "lightning",
    "anim", "challengeIsland", "fakeThreeByThreeSurface", "sunFlyerBomb", "altar",
    "dais", "rune", "forceField", "bombSpecialOne", "daisExtraFrames",
    "fenceShield", "bombTwister", "bombIITwister", "bombIIITwister", "GravitationEffect",
    "bombGraviton", "HealEffect", "MeteorEffect", "BOMBmeteor", "bombIMano",
    "bombIIMano", "bombIIIMano", "bombLightingZap", "bombLightingwave", "RUIN",
    "Monument",
};
// CD판의 실제 포인터 배열에서 확인한 자산 타입 개수.
constexpr std::size_t kCdAssetTypeCount = 101;
}
// 판본별 배열 길이를 사용하며 추가 자산을 CD판 그래픽에 끼워 넣지 않는다.
std::span<const std::string_view> TypeLoadOrder(OriginalEdition edition) {
    const std::span<const std::string_view> order(kAssetTypes);
    return edition == OriginalEdition::Cd1072 ? order.first(kCdAssetTypeCount) : order;
}
}
