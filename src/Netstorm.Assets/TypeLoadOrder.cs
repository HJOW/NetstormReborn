namespace Netstorm.Assets;

/// <summary>
/// 원본 게임이 .type 파일을 읽는 고정 순서. Netstorm.exe 0x540DD0 의 포인터 배열(116개)을 옮긴 것.
/// i 번째 타입에 _shapes.shp 의 i 번째 셰이프 블록이 배정된다. (docs/formats/shp.md "블록 ↔ 타입 대응")
/// </summary>
public static class TypeLoadOrder
{
    /// <summary>로딩 순서대로 나열한 타입 이름 (대소문자는 원본 그대로)</summary>
    public static IReadOnlyList<string> Names { get; } =
    [
        "dude", "sunArcher", "sunAviary", "sunFlyer", "banner", "windBattery", "rainBattery",
        "thunderBattery", "sunBlocker", "thunderBlocker", "blankMissile", "bolt", "bridge", "sunDisc",
        "emptyGeyser", "sunFactory", "windFactory", "rainFactory", "thunderFactory", "residence",
        "fencemark", "flag", "fortGump", "icon", "island", "islandStalag", "playerBanner", "isle",
        "isleBig", "geyserBrightener", "treeTwo", "treeThree", "fringe", "mana", "range", "manabolt",
        "flare", "puzzlePiece", "playerIsland", "battleIsland", "particlePlaceHolder", "sunBalloon",
        "buried", "bombExplodeSmall", "bombExplodeMedium", "bombExplodeLarge", "bombHeal",
        "bombInvisible", "bombParalyze", "bombHardener", "bombTreason", "edgeFarm", "geyser",
        "windVortex", "rainVortex", "thunderVortex", "outpost", "mcloud", "sunCannon", "rainCannon",
        "rainCannonMissile", "thunderCannon", "thunderCannonMissile", "sunWalker", "teleportEffect",
        "bulf", "sunFence", "windWalker", "windFlyer", "windAviary", "windArcher", "windBalloon",
        "windBlocker", "rainAviary", "rainFlyer", "rainFence", "rainBalloon", "rainBlocker",
        "thunderArcher", "thunderFence", "growingRainBlocker", "platform", "player", "mog", "nugget",
        "bridgeConnector", "rainWalker", "noIsland", "priest", "lightning", "anim", "challengeIsland",
        "fakeThreeByThreeSurface", "sunFlyerBomb", "altar", "dais", "rune", "forceField",
        "bombSpecialOne", "daisExtraFrames", "fenceShield", "bombTwister", "bombIITwister",
        "bombIIITwister", "GravitationEffect", "bombGraviton", "HealEffect", "MeteorEffect",
        "BOMBmeteor", "bombIMano", "bombIIMano", "bombIIIMano", "bombLightingZap",
        "bombLightingwave", "RUIN", "Monument",
    ];

    /// <summary>대소문자 무시 이름 → 순서 번호</summary>
    private static readonly Dictionary<string, int> IndexByName = BuildIndex();

    /// <summary>타입 이름의 로딩 순서 번호를 돌려준다 (없으면 -1)</summary>
    /// <param name="typeName">타입 이름 (대소문자 무시)</param>
    public static int IndexOf(string typeName) =>
        IndexByName.TryGetValue(typeName, out int index) ? index : -1;

    /// <summary>이름 → 번호 사전을 만든다</summary>
    private static Dictionary<string, int> BuildIndex()
    {
        var map = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
        // 목록 순서대로 번호를 매긴다
        for (int i = 0; i < Names.Count; i++)
        {
            map[Names[i]] = i;
        }
        return map;
    }
}
