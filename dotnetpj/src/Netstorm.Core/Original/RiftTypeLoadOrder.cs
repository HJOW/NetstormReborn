namespace Netstorm.Core.Original;

/// <summary>
/// 원본 자산(.type) 로딩 순서다(cpppj <c>o/TypeLoadOrder.cpp</c>의 <c>TypeLoadOrder</c>). 패치판 00540dd0의 116개, CD판 0051c6f8의 101개 이름이며
/// 타입 번호는 <see cref="RiftTypeTable.FirstAssetTypeNumber"/> + 이 순서다. CD판 목록은 패치판 목록의 앞 101개와 같다.
/// 기존 경로의 <c>Netstorm.Assets.TypeLoadOrder</c>와 이름 충돌을 피하려고 <c>RiftTypeLoadOrder</c>로 두고, 이 표는 cpppj 소스에서 따로 옮겨 적었다
/// (검사가 두 표의 일치를 확인한다).
/// </summary>
/// <remarks>2026-11-02 F1에서 추가했다. 게임 프로세스 타입 이름 목록은 이 표와 별개이며 <see cref="RiftTypeTable.ProcessTypeNames"/>에 있다.</remarks>
public static class RiftTypeLoadOrder
{
    /// <summary>원본의 자산 로딩 순서 116개다. CD판 목록은 이 배열의 앞 101개와 동일하다.</summary>
    private static readonly string[] AssetTypes =
    [
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
    ];

    /// <summary>CD판의 실제 포인터 배열에서 확인한 자산 타입 개수다.</summary>
    private const int CdAssetTypeCount = 101;

    /// <summary>판본별 로딩 순서를 돌려준다. 판본별 배열 길이를 쓰며 추가 자산을 CD판 그래픽에 끼워 넣지 않는다.</summary>
    /// <param name="edition">원본 판본.</param>
    public static IReadOnlyList<string> Get(OriginalEdition edition) =>
        edition == OriginalEdition.Cd1072 ? new ArraySegment<string>(AssetTypes, 0, CdAssetTypeCount) : AssetTypes;
}
