using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>한 플레이어의 게임 상태: Storm Power, 생산 창(덱), 다리 조각 칸, 미션이 정한 기술 허용.</summary>
public sealed class PlayerState
{
    /// <summary>플레이어 번호 (1부터)</summary>
    public int Number { get; }

    /// <summary>현재 Storm Power</summary>
    public int StormPower { get; internal set; }

    /// <summary>생산 창(덱): 템플이 공급하는 다리·골렘과 워크샵 등록 유닛, 획득한 지식</summary>
    public ProductionDeck Deck { get; } = new();

    /// <summary>다리 조각 칸 (템플이 있는 동안 1초마다 채워진다)</summary>
    public BridgeTray Tray { get; }

    /// <summary>커서로 집고 있는 다리 조각 (없으면 null). 회전은 화면이 관리하므로 모양만 의미가 있다.</summary>
    public BridgePiece? HeldPiece { get; internal set; }

    /// <summary>미션 techAllowed 가 정한 만들 수 있는 기술 (제한이 없으면 모두 허용)</summary>
    public TechPermissions Tech { get; }

    /// <summary>유닛 타입 이름 → 그 유닛을 다시 놓을 수 있게 되는 틱 (재충전, 대소문자 무시)</summary>
    internal Dictionary<string, long> UnitReadyTick { get; } = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>
    /// 지금 선택한 오브젝트 번호 (없으면 0). 원본 DAT_005caea0 에 해당하며 튜토리얼 단계 처리(단계 C·F)가 읽는다.
    /// 화면 조작이지만 상태를 명령(<see cref="SelectEntityCommand"/>)으로만 바꿔 명령 스트림만으로 같은 결과가 나오게 한다.
    /// </summary>
    public int SelectedEntityId { get; internal set; }

    /// <summary>
    /// 타입 이름(소문자) → 게임 중 지은 누적 수와 그 타입의 플래그2. 원본 Totalmade.cpp 의 DAT_005c98d0 표에 해당한다:
    /// 오브젝트가 생길 때 올리고, 파괴·회수로는 줄이지 않는다 (줄이는 곳은 전체 삭제 함수 004c27c0 뿐).
    /// 건물은 완공할 때, 유닛은 놓을 때 센다. 맵에 처음부터 있던 오브젝트는 세지 않는다.
    /// </summary>
    private readonly SortedDictionary<string, MadeEntry> _made = new(StringComparer.Ordinal);

    /// <summary>템플이 있는지 (다리 조각·골렘 공급 조건)</summary>
    public bool HasTemple => Deck.TempleId != null;

    /// <summary>타입 하나를 지은 누적 수 (원본 FUN_004c24c0, 없으면 0)</summary>
    /// <param name="typeName">.type 이름 (대소문자 무시)</param>
    public int Made(string typeName) => _made.TryGetValue(typeName.ToLowerInvariant(), out MadeEntry entry) ? entry.Count : 0;

    /// <summary>플래그2 비트를 하나라도 가진 타입들의 누적 수 합 (원본 FUN_004c2500: 템플 = 0x200, 워크샵 = 0x4000)</summary>
    /// <param name="flags2">플래그2 비트 마스크</param>
    public int MadeWithFlags(uint flags2) => _made.Values.Where(e => (e.Flags2 & flags2) != 0).Sum(e => e.Count);

    /// <summary>지은 누적 수를 이름 순서로 나열한다 (세션 검사합용, Dictionary 순서에 의존하지 않는다)</summary>
    public IEnumerable<(string Name, int Count)> MadeSnapshot() => _made.Select(pair => (pair.Key, pair.Value.Count));

    /// <summary>타입 하나를 지었다고 기록한다 (누적 수 +1)</summary>
    /// <param name="typeName">.type 이름</param>
    /// <param name="flags2">그 타입의 플래그2</param>
    internal void RecordMade(string typeName, uint flags2)
    {
        string key = typeName.ToLowerInvariant();
        _made[key] = new MadeEntry(flags2, Made(key) + 1);
    }

    /// <summary>지은 누적 수 한 칸: 타입의 플래그2와 개수</summary>
    /// <param name="Flags2">타입 플래그2</param>
    /// <param name="Count">누적 개수</param>
    private readonly record struct MadeEntry(uint Flags2, int Count);

    /// <summary>플레이어 상태를 만든다</summary>
    /// <param name="number">플레이어 번호</param>
    /// <param name="stormPower">시작 Storm Power</param>
    /// <param name="tray">다리 조각 칸</param>
    /// <param name="tech">기술 허용</param>
    internal PlayerState(int number, int stormPower, BridgeTray tray, TechPermissions tech)
    {
        Number = number;
        StormPower = stormPower;
        Tray = tray;
        Tech = tech;
    }
}
