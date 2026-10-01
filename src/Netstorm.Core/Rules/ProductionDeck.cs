using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>워크샵에 등록할 수 있는 유닛의 규칙용 정보.</summary>
/// <param name="Name">타입 이름</param>
/// <param name="Element">유닛 원소</param>
/// <param name="IsGenerator">Generator 인지 (Sun Workshop 이 다른 원소 Generator 를 등록할 수 있는 예외)</param>
public sealed record ProducibleUnit(string Name, Element Element, bool IsGenerator)
{
    /// <summary>타입 정의에서 만든다. 생산 대상이 아니거나 원소가 없으면 null.</summary>
    /// <param name="type">로딩 목록의 타입</param>
    public static ProducibleUnit? FromType(TypeInfo type)
    {
        ObjectKind kind = ObjectKinds.Of(type);
        Element? element = Elements.FromTheme(type.Definition.GetString("theme"));
        return ObjectKinds.IsProducibleUnit(kind) && element != null
            ? new ProducibleUnit(type.Name, element.Value, kind == ObjectKind.Generator)
            : null;
    }
}

/// <summary>생산 창 항목의 종류.</summary>
public enum DeckEntryKind
{
    /// <summary>다리 조각 (템플이 공급)</summary>
    Bridge,

    /// <summary>골렘 (템플이 공급)</summary>
    Golem,

    /// <summary>워크샵에 등록한 유닛</summary>
    Unit,
}

/// <summary>생산 창(덱)의 항목 하나. SourceId 는 공급한 건물(템플·워크샵) 번호다.</summary>
/// <param name="Kind">종류</param>
/// <param name="TypeName">유닛 타입 이름 (다리는 "bridge", 골렘은 "sunwalker")</param>
/// <param name="SourceId">공급 건물 번호</param>
public sealed record DeckEntry(DeckEntryKind Kind, string TypeName, int SourceId);

/// <summary>워크샵 등록 시도의 결과.</summary>
public enum RegisterResult
{
    /// <summary>등록됨</summary>
    Registered,

    /// <summary>해당 워크샵이 없음 (파괴됨 등)</summary>
    NoWorkshop,

    /// <summary>워크샵 원소가 맞지 않음 (Sun Workshop 의 Generator 예외 제외)</summary>
    WrongElement,

    /// <summary>이미 어느 워크샵에든 등록된 유닛 (한 유닛은 한 워크샵에만)</summary>
    AlreadyRegistered,

    /// <summary>생산 칸이 모두 찼음 (Level I 2 · II 3 · III 4)</summary>
    NoFreeSlot,

    /// <summary>획득하지 않은 지식</summary>
    UnknownKnowledge,
}

/// <summary>
/// 한 플레이어의 생산 창(덱)과 워크샵 등록 상태 (docs/gameplay/workshop-deck.md 2절, 사용자 확인 규칙).
/// <list type="bullet">
/// <item><description>템플이 있으면 다리 조각과 골렘이 등록 절차 없이 덱에 있다. 템플이 없어지면 둘 다 사라진다.</description></item>
/// <item><description>워크샵은 자기 원소 유닛만 등록한다. Sun Workshop 은 다른 원소 Generator 도 등록할 수 있다.</description></item>
/// <item><description>한 유닛은 한 워크샵에만 등록된다. 워크샵이 파괴되면 그 등록이 모두 풀리고, 재건한 워크샵은 빈 상태로 시작한다.</description></item>
/// <item><description>생산 칸: Level I 2개, II 3개, III 4개 (GAME.HLP, 패치 exe 판정은 미확인).</description></item>
/// </list>
/// </summary>
public sealed class ProductionDeck
{
    /// <summary>워크샵 레벨별 생산 칸 수 (인덱스 = 레벨 − 1)</summary>
    private static readonly int[] SlotsPerLevel = [2, 3, 4];

    /// <summary>워크샵 최대 레벨 (업그레이드 2번)</summary>
    public const int MaxWorkshopLevel = 3;

    /// <summary>템플이 공급하는 골렘의 타입 이름</summary>
    public const string GolemType = "sunwalker";

    /// <summary>다리 조각의 타입 이름</summary>
    public const string BridgeType = "bridge";

    /// <summary>워크샵 번호 → 상태 (등록 순서를 보존하기 위해 목록 사용)</summary>
    private readonly Dictionary<int, WorkshopState> _workshops = [];

    /// <summary>획득한 지식(유닛 타입 이름, 대소문자 무시)</summary>
    private readonly HashSet<string> _knowledge = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>현재 템플 번호 (없으면 null). 플레이어당 템플은 동시에 1기다.</summary>
    public int? TempleId { get; private set; }

    /// <summary>획득한 지식 목록</summary>
    public IReadOnlyCollection<string> Knowledge => _knowledge;

    /// <summary>지식(생산 가능한 유닛)을 얻는다. 희생·미션 시작 설정에서 호출한다.</summary>
    /// <param name="typeName">유닛 타입 이름</param>
    public void LearnKnowledge(string typeName) => _knowledge.Add(typeName);

    /// <summary>템플이 세워졌다 (다리·골렘 공급 시작)</summary>
    /// <param name="templeId">템플 번호</param>
    public void SetTemple(int templeId) => TempleId = templeId;

    /// <summary>템플이 파괴되었다 (다리·골렘이 덱에서 사라짐)</summary>
    public void RemoveTemple() => TempleId = null;

    /// <summary>워크샵이 완성되었다. 재건한 워크샵도 등록이 빈 상태로 시작한다.</summary>
    /// <param name="workshopId">워크샵 번호</param>
    /// <param name="element">워크샵 원소</param>
    /// <param name="level">워크샵 레벨 (1~3)</param>
    public void AddWorkshop(int workshopId, Element element, int level = 1) =>
        _workshops[workshopId] = new WorkshopState(element, Math.Clamp(level, 1, MaxWorkshopLevel));

    /// <summary>워크샵이 파괴되었다. 그 워크샵으로 등록한 유닛이 덱에서 사라지고 다른 워크샵에 다시 등록할 수 있게 된다.</summary>
    /// <param name="workshopId">워크샵 번호</param>
    public void RemoveWorkshop(int workshopId) => _workshops.Remove(workshopId);

    /// <summary>워크샵 레벨을 1 올린다. 이미 최대 레벨이거나 없으면 false.</summary>
    /// <param name="workshopId">워크샵 번호</param>
    public bool UpgradeWorkshop(int workshopId)
    {
        if (!_workshops.TryGetValue(workshopId, out WorkshopState? state) || state.Level >= MaxWorkshopLevel)
        {
            return false;
        }
        state.Level++;
        return true;
    }

    /// <summary>워크샵 레벨. 없으면 0.</summary>
    /// <param name="workshopId">워크샵 번호</param>
    public int WorkshopLevel(int workshopId) => _workshops.TryGetValue(workshopId, out WorkshopState? s) ? s.Level : 0;

    /// <summary>검사합용 워크샵 상태. 번호순이며 등록 순서를 보존한 복사본이다.</summary>
    public IReadOnlyList<(int Id, Element Element, int Level, IReadOnlyList<string> Registered)> WorkshopSnapshot() =>
        _workshops.OrderBy(pair => pair.Key).Select(pair => (pair.Key, pair.Value.Element, pair.Value.Level,
            (IReadOnlyList<string>)pair.Value.Registered.ToArray())).ToArray();

    /// <summary>워크샵의 남은 생산 칸 수 ("Production Slots Available"). 없으면 0.</summary>
    /// <param name="workshopId">워크샵 번호</param>
    public int FreeSlots(int workshopId) =>
        _workshops.TryGetValue(workshopId, out WorkshopState? s) ? SlotsPerLevel[s.Level - 1] - s.Registered.Count : 0;

    /// <summary>워크샵에 등록된 유닛 ("View Current Production")</summary>
    /// <param name="workshopId">워크샵 번호</param>
    public IReadOnlyList<string> RegisteredAt(int workshopId) =>
        _workshops.TryGetValue(workshopId, out WorkshopState? s) ? s.Registered : [];

    /// <summary>
    /// 워크샵의 "Knowledge Available" 목록: 획득한 지식 중 원소 조건을 만족하고 아직 어느 워크샵에도 등록되지 않은 유닛.
    /// </summary>
    /// <param name="workshopId">워크샵 번호</param>
    /// <param name="units">생산 가능한 전체 유닛 정보</param>
    public IReadOnlyList<ProducibleUnit> AvailableKnowledge(int workshopId, IEnumerable<ProducibleUnit> units)
    {
        if (!_workshops.TryGetValue(workshopId, out WorkshopState? state))
        {
            return [];
        }
        return units.Where(u => _knowledge.Contains(u.Name) && Accepts(state.Element, u) && !IsRegistered(u.Name)).ToArray();
    }

    /// <summary>유닛을 워크샵에 등록한다 ("Put Knowledge into Production").</summary>
    /// <param name="workshopId">워크샵 번호</param>
    /// <param name="unit">등록할 유닛</param>
    public RegisterResult Register(int workshopId, ProducibleUnit unit)
    {
        if (!_workshops.TryGetValue(workshopId, out WorkshopState? state)) return RegisterResult.NoWorkshop;
        if (!_knowledge.Contains(unit.Name)) return RegisterResult.UnknownKnowledge;
        if (!Accepts(state.Element, unit)) return RegisterResult.WrongElement;
        if (IsRegistered(unit.Name)) return RegisterResult.AlreadyRegistered;
        if (state.Registered.Count >= SlotsPerLevel[state.Level - 1]) return RegisterResult.NoFreeSlot;
        state.Registered.Add(unit.Name);
        return RegisterResult.Registered;
    }

    /// <summary>
    /// 현재 생산 창 항목: 템플이 있으면 다리·골렘, 그 뒤에 워크샵 번호 순으로 등록한 유닛.
    /// </summary>
    public IReadOnlyList<DeckEntry> Entries()
    {
        var entries = new List<DeckEntry>();
        if (TempleId is int temple)
        {
            entries.Add(new DeckEntry(DeckEntryKind.Bridge, BridgeType, temple));
            entries.Add(new DeckEntry(DeckEntryKind.Golem, GolemType, temple));
        }
        // 워크샵 번호 순서로 등록 순서를 유지해 나열한다 (Dictionary 순회 순서에 의존하지 않는다).
        foreach ((int id, WorkshopState state) in _workshops.OrderBy(pair => pair.Key))
        {
            // 한 워크샵에 등록한 순서대로 항목을 만든다.
            foreach (string name in state.Registered)
            {
                entries.Add(new DeckEntry(DeckEntryKind.Unit, name, id));
            }
        }
        return entries;
    }

    /// <summary>워크샵 원소가 유닛을 받을 수 있는지: 같은 원소, 또는 Sun Workshop 의 Generator 예외</summary>
    private static bool Accepts(Element workshop, ProducibleUnit unit) =>
        unit.Element == workshop || (workshop == Element.Sun && unit.IsGenerator);

    /// <summary>어느 워크샵에든 이미 등록되었는지</summary>
    private bool IsRegistered(string name) =>
        _workshops.Values.Any(w => w.Registered.Contains(name, StringComparer.OrdinalIgnoreCase));

    /// <summary>워크샵 하나의 원소·레벨·등록 목록</summary>
    private sealed class WorkshopState(Element element, int level)
    {
        /// <summary>워크샵 원소</summary>
        public Element Element { get; } = element;

        /// <summary>현재 레벨 (1~3)</summary>
        public int Level { get; set; } = level;

        /// <summary>등록한 유닛 이름 (등록 순서)</summary>
        public List<string> Registered { get; } = [];
    }
}
