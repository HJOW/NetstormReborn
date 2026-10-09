using Netstorm.Core.Simulation;
using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>배치 판정 결과.</summary>
/// <param name="Problem">첫 번째 문제 (없으면 None)</param>
/// <param name="Footprint">배치할 발자국</param>
/// <param name="Island">기준점 위치의 섬 상태</param>
/// <param name="Requirement">필요 에너지</param>
/// <param name="Energy">에너지 판정 (필요 에너지가 없으면 빈 결과)</param>
/// <param name="Cost">Storm Power 비용</param>
public sealed record PlacementCheck(PlacementProblem Problem, Footprint Footprint, IslandState Island,
    EnergyRequirement Requirement, EnergyCheck Energy, int Cost)
{
    /// <summary>배치할 수 있는지</summary>
    public bool Allowed => Problem == PlacementProblem.None;
}

/// <summary>
/// 맵 한 판의 규칙 상태: 섬 소유권·에너지 공급원·전투 옵션. 저장된 맵(.fort)에서 초기 상태를 만든다.
/// 이동·전투 규칙은 아직 없으며, 배치 판정에 필요한 정적 규칙과 오브젝트 점유를 모았다.
/// 게임 진행(틱·명령·플레이어 상태)은 <c>Netstorm.Core.Simulation.BattleSession</c> 이 이 객체를 가지고 다룬다.
/// </summary>
public sealed class BattleMap
{
    /// <summary>에너지 공급원 (템플·Generator)</summary>
    private readonly List<EnergySource> _sources = [];

    /// <summary>칸 → 영역 번호 (섬 칸이 아니면 없음)</summary>
    private readonly Func<int, int, int?> _territoryAt;

    /// <summary>두 플레이어가 동맹인지 (같은 플레이어 포함). 기본은 같은 번호만</summary>
    private readonly Func<int, int, bool> _allied;

    /// <summary>오브젝트가 차지한 칸 → 겹쳐 차지한 오브젝트 수 (배치 시 "빈 자리" 판정, 제거 시 마지막 하나가 빠질 때만 칸이 빈다)</summary>
    private readonly Dictionary<(int X, int Y), int> _occupied = [];

    /// <summary>맵을 읽을 때 등록한 오브젝트와 번호 (noIsland 제외, 저장 순서)</summary>
    private readonly List<(int Id, FortMapObject Item)> _initialObjects = [];

    /// <summary>한 판의 모든 알려진 월드 오브젝트가 공유하는 서버 번호 풀.</summary>
    public SidPool Sids { get; }

    /// <summary>섬 소유권</summary>
    public IslandOwnership Ownership { get; } = new();

    /// <summary>전투 옵션 (공급 반지름 등)</summary>
    public BattleOptions Options { get; }

    /// <summary>에너지 공급원 목록</summary>
    public IReadOnlyList<EnergySource> Sources => _sources;

    /// <summary>전투 목표 선택도 에너지 공급과 같은 동맹 판정을 사용한다.</summary>
    public bool AreAllied(int first, int second) => first == second || _allied(first, second);

    /// <summary>맵을 읽을 때 등록한 오브젝트와 번호 (noIsland 제외). 엔티티 목록을 만들 때 같은 번호를 쓰도록 노출한다.</summary>
    public IReadOnlyList<(int Id, FortMapObject Item)> InitialObjects => _initialObjects;

    /// <summary>
    /// 맵 오브젝트에서 템플 소유 영역과 공급원을 모은다.
    /// </summary>
    /// <param name="objects">월드 좌표가 정해진 오브젝트 (FortMap.Objects)</param>
    /// <param name="territoryAt">칸 → 영역 번호 (지면 미리보기의 섬 칸 등). 섬 칸이 아니면 null</param>
    /// <param name="options">전투 옵션 (null 이면 원본 기본값)</param>
    /// <param name="allied">동맹 판정 (null 이면 같은 플레이어만)</param>
    /// <param name="sids">공유 번호 풀 (없으면 원본 크기의 로컬 서버 풀)</param>
    public BattleMap(IEnumerable<FortMapObject> objects, Func<int, int, int?> territoryAt, BattleOptions? options = null,
        Func<int, int, bool>? allied = null, SidPool? sids = null)
    {
        Sids = sids ?? new SidPool(SidPool.NativeCapacity1078);
        _territoryAt = territoryAt;
        Options = options ?? new BattleOptions();
        _allied = allied ?? ((a, b) => a == b);
        // 저장된 오브젝트마다 소유권·공급원·점유 칸을 등록한다.
        foreach (FortMapObject item in objects)
        {
            int id = NextId(item.Object.Type);
            // noIsland도 원본의 논리 오브젝트 번호를 받지만 엔티티·점유는 만들지 않는다.
            if (item.Object.Type.Name.Equals(LogicalGroundType, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }
            _initialObjects.Add((id, item));
            ObjectKind kind = ObjectKinds.Of(item.Object.Type);
            int owner = item.Object.LoadOwner;
            if (kind == ObjectKind.Temple && item.Territory is int territory && owner != 0)
            {
                Ownership.SetTemple(territory, owner);
            }
            Footprint foot = Footprint.ForType(item.Object.Type.Definition, item.X, item.Y);
            if (kind != ObjectKind.Flyer && !item.Object.Type.Definition.HasFlag("balloon")) AddOccupant(foot);
            Element? element = Elements.FromTheme(item.Object.Type.Definition.GetString("theme"));
            if (ObjectKinds.IsEnergySource(kind) && element != null && owner != 0)
            {
                _sources.Add(new EnergySource(id, element.Value, foot.CenterX, foot.CenterY, owner));
            }
        }
    }

    /// <summary>자리를 차지하지 않는 논리 지면 타입 (작은 받침 칸)</summary>
    private const string LogicalGroundType = "noIsland";

    /// <summary>새 서버 오브젝트 번호를 받는다. 저장 오브젝트·다리·게임 중 생성이 같은 FIFO 풀을 쓴다.</summary>
    public int NextId(TypeInfo? type = null) => Sids.AllocateWorld((byte)(type?.RuntimeIndex ?? 0));

    /// <summary>발자국 칸을 점유로 표시한다 (새 오브젝트를 놓을 때)</summary>
    /// <param name="footprint">차지할 발자국</param>
    public void AddOccupant(Footprint footprint)
    {
        // 발자국의 모든 칸의 점유 수를 하나씩 올린다.
        foreach ((int x, int y) in footprint.Cells())
        {
            _occupied[(x, y)] = _occupied.GetValueOrDefault((x, y)) + 1;
        }
    }

    /// <summary>발자국 칸의 점유를 해제한다 (오브젝트를 회수·파괴할 때). 다른 오브젝트가 겹쳐 있던 칸은 계속 차 있다.</summary>
    /// <param name="footprint">비울 발자국</param>
    public void RemoveOccupant(Footprint footprint)
    {
        // 발자국의 모든 칸의 점유 수를 하나씩 내리고 0 이 되면 칸을 비운다.
        foreach ((int x, int y) in footprint.Cells())
        {
            if (!_occupied.TryGetValue((x, y), out int count))
            {
                continue;
            }
            if (count <= 1)
            {
                _occupied.Remove((x, y));
            }
            else
            {
                _occupied[(x, y)] = count - 1;
            }
        }
    }

    /// <summary>발자국이 기존 오브젝트와 한 칸이라도 겹치는지</summary>
    /// <param name="footprint">검사할 발자국</param>
    /// <param name="ignored">설치가 허용되는 점유자(사제)의 발자국. 겹친 다른 점유자의 수는 그대로 남긴다.</param>
    public bool IsOccupied(Footprint footprint, IReadOnlyList<Footprint>? ignored = null) =>
        footprint.Cells().Any(cell => _occupied.GetValueOrDefault(cell) > (ignored?.Count(foot => foot.Contains(cell.X, cell.Y)) ?? 0));

    /// <summary>
    /// 유닛 자리를 점유하고 활성 상태이면 공급원도 등록한다. 생산 예약은 도착 후 별도로 공급원을 등록한다.
    /// </summary>
    /// <param name="type">유닛 타입</param>
    /// <param name="check">CheckUnit 결과 (Allowed 여야 한다)</param>
    /// <param name="player">소유 플레이어</param>
    /// <returns>새 오브젝트 번호</returns>
    /// <param name="activate">즉시 공급 효과를 적용할지. 생산 자원 운송 중이면 false.</param>
    public int PlaceUnit(TypeInfo type, PlacementCheck check, int player, bool activate = true)
    {
        if (!check.Allowed)
        {
            throw new InvalidOperationException($"배치할 수 없는 위치입니다: {PlacementRules.Describe(check.Problem)}");
        }
        int id = NextId(type);
        if (!type.Definition.HasFlag("balloon") && ObjectKinds.Of(type) != ObjectKind.Flyer) AddOccupant(check.Footprint);
        Element? element = Elements.FromTheme(type.Definition.GetString("theme"));
        if (activate && ObjectKinds.IsEnergySource(ObjectKinds.Of(type)) && element != null)
        {
            _sources.Add(new EnergySource(id, element.Value, check.Footprint.CenterX, check.Footprint.CenterY, player));
        }
        return id;
    }

    /// <summary>공급원을 추가한다 (Generator·템플 건설 시)</summary>
    /// <param name="source">공급원</param>
    public void AddSource(EnergySource source) => _sources.Add(source);

    /// <summary>공급원을 제거한다 (파괴 시). 이미 지은 유닛에는 영향이 없다 (건설 순간에만 확인).</summary>
    /// <param name="id">공급원 번호</param>
    public void RemoveSource(int id) => _sources.RemoveAll(s => s.Id == id);

    /// <summary>칸의 영역 번호 (섬 칸이 아니면 null)</summary>
    /// <param name="x">칸 x</param>
    /// <param name="y">칸 y</param>
    public int? TerritoryAt(int x, int y) => _territoryAt(x, y);

    /// <summary>플레이어가 공급원을 쓸 수 있는지 (자기 또는 동맹 소유)</summary>
    /// <param name="player">플레이어</param>
    /// <param name="owner">공급원 소유자</param>
    public bool IsFriendly(int player, int owner) => _allied(player, owner);

    /// <summary>
    /// 워크샵에서 생산하는 유닛을 기준점에 놓을 수 있는지 판정한다: 위치 → 빈 자리 → Storm Power → 에너지 순서.
    /// </summary>
    /// <param name="type">유닛 타입</param>
    /// <param name="anchorX">기준점(오른쪽 아래 칸) x</param>
    /// <param name="anchorY">기준점 y</param>
    /// <param name="player">플레이어</param>
    /// <param name="stormPower">플레이어의 현재 Storm Power</param>
    /// <param name="connectedToHome">빈 섬일 때 내 섬과 다리로 연결되었는지 (호출자가 다리 연결을 보고 판단)</param>
    /// <param name="atOwnBridgeEnd">섬 밖일 때 내 다리 끝인지 (호출자가 판단)</param>
    public PlacementCheck CheckUnit(TypeInfo type, int anchorX, int anchorY, int player, int stormPower,
        bool connectedToHome, bool atOwnBridgeEnd)
    {
        Footprint foot = Footprint.ForType(type.Definition, anchorX, anchorY);
        IslandState island = Ownership.StateFor(TerritoryAt(anchorX, anchorY), player);
        PlacementProblem problem = PlacementRules.CheckUnitSite(island, connectedToHome, atOwnBridgeEnd);
        return Finish(type, foot, island, problem, player, stormPower);
    }

    /// <summary>
    /// 사제가 짓는 건물(템플·워크샵·알타·아웃포스트)을 기준점에 지을 수 있는지 판정한다.
    /// 위치는 섬 소유 규칙(템플은 빈 섬에 플레이어당 1기, 알타도 1기, 남의 섬 불가, 섬 밖 불가), 이어서 빈 자리·Storm Power·에너지 순서다.
    /// 사제가 그 자리까지 걸어갈 수 있는지는 맵이 아니라 세션(<c>BattleSession.CheckBuilding</c>)이 판정한다.
    /// </summary>
    /// <param name="type">건물 타입</param>
    /// <param name="anchorX">기준점(오른쪽 아래 칸) x</param>
    /// <param name="anchorY">기준점 y</param>
    /// <param name="player">플레이어</param>
    /// <param name="stormPower">플레이어의 현재 Storm Power</param>
    /// <param name="playerHasTemple">플레이어에게 이미 템플이 있는지 (건설 중인 템플 포함)</param>
    /// <param name="playerHasAltar">플레이어에게 이미 알타가 있는지 (건설 중인 알타 포함)</param>
    /// <param name="ignored">건물 설치를 막지 않는 사제 발자국.</param>
    public PlacementCheck CheckBuilding(TypeInfo type, int anchorX, int anchorY, int player, int stormPower, bool playerHasTemple,
        bool playerHasAltar = false, IReadOnlyList<Footprint>? ignored = null)
    {
        Footprint foot = Footprint.ForType(type.Definition, anchorX, anchorY);
        IslandState island = Ownership.StateFor(TerritoryAt(anchorX, anchorY), player);
        PlacementProblem problem = PlacementRules.CheckBuildingSite(ObjectKinds.Of(type), island, playerHasTemple, playerHasAltar);
        // 건물은 기준점뿐 아니라 발자국 전체가 같은 섬의 지면 위에 있어야 한다 (TEST02 가장자리 설치 시험).
        if (problem == PlacementProblem.None && foot.Cells().Any(cell => TerritoryAt(cell.X, cell.Y) != TerritoryAt(anchorX, anchorY)))
            problem = PlacementProblem.BuildingNeedsIsland;
        return Finish(type, foot, island, problem, player, stormPower, ignored);
    }

    /// <summary>위치 판정 뒤의 공통 검사(빈 자리 → Storm Power → 에너지)를 하고 결과를 만든다.</summary>
    private PlacementCheck Finish(TypeInfo type, Footprint foot, IslandState island, PlacementProblem problem, int player, int stormPower,
        IReadOnlyList<Footprint>? ignored = null)
    {
        EnergyRequirement requirement = EnergyRequirement.ForType(type.Definition);
        int cost = StormPower.TypeCost(type.Definition);
        EnergyCheck energy = EnergySupply.Check(requirement, _sources, foot, Options.GeneratorRadiusSquared,
            owner => IsFriendly(player, owner));
        if (problem == PlacementProblem.None && IsOccupied(foot, ignored))
        {
            problem = PlacementProblem.Occupied;
        }
        if (problem == PlacementProblem.None && cost > stormPower)
        {
            problem = PlacementProblem.NotEnoughStormPower;
        }
        if (problem == PlacementProblem.None && !energy.Satisfied)
        {
            problem = PlacementProblem.NotEnoughEnergy;
        }
        return new PlacementCheck(problem, foot, island, requirement, energy, cost);
    }
}
