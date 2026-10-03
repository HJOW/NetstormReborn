namespace Netstorm.Core.Rules;

/// <summary>한 플레이어 입장에서 본 섬(영역)의 상태 (docs/gameplay/island-ownership.md).</summary>
public enum IslandState
{
    /// <summary>섬이 아님 (하늘·다리 위 등 영역 밖)</summary>
    NoIsland,

    /// <summary>내 섬: 내 템플이 있는 섬</summary>
    Mine,

    /// <summary>빈 섬: 아무도 소유하지 않은 섬 (템플 없음)</summary>
    Empty,

    /// <summary>남의 섬: 다른 플레이어의 템플이 있는 섬</summary>
    Others,
}

/// <summary>배치할 수 없는 이유.</summary>
public enum PlacementProblem
{
    /// <summary>문제 없음</summary>
    None,

    /// <summary>남의 섬에는 건물·유닛을 일체 지을 수 없다</summary>
    OthersIsland,

    /// <summary>빈 섬의 건물형 유닛은 내 섬과 다리로 연결되어야 한다</summary>
    NotConnected,

    /// <summary>템플은 빈 섬에만 지을 수 있다</summary>
    TempleNeedsEmptyIsland,

    /// <summary>템플은 플레이어당 동시에 1기</summary>
    TempleAlreadyExists,

    /// <summary>알타는 플레이어당 동시에 1기 (웹 팬게임 규칙. 의식으로 소멸하면 다시 지을 수 있다)</summary>
    AltarAlreadyExists,

    /// <summary>섬 위가 아니고 내 다리 끝도 아니다</summary>
    NotOnIslandOrBridgeEnd,

    /// <summary>건물(템플·워크샵·알타·아웃포스트)은 섬 위에만 지을 수 있다 (다리 끝 불가)</summary>
    BuildingNeedsIsland,

    /// <summary>자리가 비어 있지 않다 (다른 오브젝트·다리·거주지와 겹침)</summary>
    Occupied,

    /// <summary>Storm Power 가 부족하다</summary>
    NotEnoughStormPower,

    /// <summary>필요 에너지를 채우지 못했다</summary>
    NotEnoughEnergy,
}

/// <summary>
/// 영역(섬) 소유권: 템플이 있는 영역은 그 템플 소유자의 섬이다.
/// </summary>
public sealed class IslandOwnership
{
    /// <summary>영역 번호 → 템플 소유자</summary>
    private readonly Dictionary<int, int> _owners = [];

    /// <summary>템플이 있는 영역과 소유자를 등록한다 (템플이 세워질 때·맵을 읽을 때).</summary>
    /// <param name="territory">영역 번호</param>
    /// <param name="owner">템플 소유자</param>
    public void SetTemple(int territory, int owner) => _owners[territory] = owner;

    /// <summary>영역의 템플이 사라졌다 (파괴·희생). 영역은 빈 섬이 된다.</summary>
    /// <param name="territory">영역 번호</param>
    public void RemoveTemple(int territory) => _owners.Remove(territory);

    /// <summary>영역 소유자 (빈 섬이면 null)</summary>
    /// <param name="territory">영역 번호</param>
    public int? OwnerOf(int territory) => _owners.TryGetValue(territory, out int owner) ? owner : null;

    /// <summary>플레이어가 템플을 가진 영역 (없으면 null)</summary>
    /// <param name="player">플레이어 번호</param>
    public int? TempleTerritoryOf(int player) =>
        _owners.Where(pair => pair.Value == player).Select(pair => (int?)pair.Key).FirstOrDefault();

    /// <summary>플레이어 입장에서 본 영역 상태</summary>
    /// <param name="territory">영역 번호 (영역 밖이면 null)</param>
    /// <param name="player">플레이어 번호</param>
    public IslandState StateFor(int? territory, int player)
    {
        if (territory is not int t)
        {
            return IslandState.NoIsland;
        }
        int? owner = OwnerOf(t);
        return owner == null ? IslandState.Empty : owner == player ? IslandState.Mine : IslandState.Others;
    }
}

/// <summary>
/// 배치 위치 조건 (사용자 확인 규칙, docs/gameplay/island-ownership.md 규칙 2~6, docs/sources/game-manual.md "건물").
/// 다리 연결·다리 끝 판정은 다리 규칙(Bridge.cpp) 분석 전이므로 호출하는 쪽이 결과를 넘긴다.
/// </summary>
public static class PlacementRules
{
    /// <summary>
    /// 워크샵에서 생산하는 유닛(건물형·이동형)의 위치 조건.
    /// 내 섬, 내 섬과 다리로 연결된 빈 섬, 내 다리 끝에 지을 수 있고 남의 섬에는 지을 수 없다.
    /// </summary>
    /// <param name="island">배치 위치의 섬 상태</param>
    /// <param name="connectedToHome">빈 섬이 내 섬과 다리로 연결되었는지</param>
    /// <param name="atOwnBridgeEnd">섬 밖이면, 그 위치가 내 다리 끝인지</param>
    public static PlacementProblem CheckUnitSite(IslandState island, bool connectedToHome, bool atOwnBridgeEnd) => island switch
    {
        IslandState.Mine => PlacementProblem.None,
        IslandState.Others => PlacementProblem.OthersIsland,
        IslandState.Empty => connectedToHome ? PlacementProblem.None : PlacementProblem.NotConnected,
        _ => atOwnBridgeEnd ? PlacementProblem.None : PlacementProblem.NotOnIslandOrBridgeEnd,
    };

    /// <summary>
    /// 사제가 짓는 건물의 위치 조건. 템플은 빈 섬에만, 플레이어당 1기. 워크샵·알타는 내 섬 또는 빈 섬(사제가 도달하면 다리 불필요).
    /// 아웃포스트는 멀티플레이 중립 섬 규칙이 추가 분석 대상이라 워크샵과 같은 조건으로 둔다.
    /// </summary>
    /// <param name="kind">건물 종류</param>
    /// <param name="island">배치 위치의 섬 상태</param>
    /// <param name="playerHasTemple">플레이어에게 이미 템플이 있는지</param>
    /// <param name="playerHasAltar">플레이어에게 이미 알타가 있는지 (건설 중 포함)</param>
    public static PlacementProblem CheckBuildingSite(ObjectKind kind, IslandState island, bool playerHasTemple, bool playerHasAltar = false)
    {
        if (island == IslandState.NoIsland) return PlacementProblem.BuildingNeedsIsland;
        if (island == IslandState.Others) return PlacementProblem.OthersIsland;
        if (kind == ObjectKind.Temple)
        {
            if (playerHasTemple) return PlacementProblem.TempleAlreadyExists;
            if (island != IslandState.Empty) return PlacementProblem.TempleNeedsEmptyIsland;
        }
        if (kind == ObjectKind.Altar && playerHasAltar) return PlacementProblem.AltarAlreadyExists;
        return PlacementProblem.None;
    }

    /// <summary>문제 설명 (개발용 한국어 문구)</summary>
    /// <param name="problem">문제</param>
    public static string Describe(PlacementProblem problem) => problem switch
    {
        PlacementProblem.None => "배치 가능",
        PlacementProblem.OthersIsland => "남의 섬에는 지을 수 없음",
        PlacementProblem.NotConnected => "빈 섬이 내 섬과 다리로 연결되지 않음",
        PlacementProblem.TempleNeedsEmptyIsland => "템플은 빈 섬에만 지을 수 있음",
        PlacementProblem.TempleAlreadyExists => "템플은 플레이어당 1기",
        PlacementProblem.AltarAlreadyExists => "알타는 플레이어당 1기",
        PlacementProblem.NotOnIslandOrBridgeEnd => "섬 위나 내 다리 끝이 아님",
        PlacementProblem.BuildingNeedsIsland => "건물은 섬 위에만 지을 수 있음",
        PlacementProblem.Occupied => "자리가 비어 있지 않음",
        PlacementProblem.NotEnoughStormPower => "Storm Power 부족",
        PlacementProblem.NotEnoughEnergy => "에너지 부족",
        _ => problem.ToString(),
    };
}
