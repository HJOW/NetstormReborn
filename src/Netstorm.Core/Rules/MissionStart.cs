using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>
/// 타입별 "만들 수 있는 기술" 표. 머리 값 techAllowed 는 이 표의 **시작 상태**일 뿐이다: 원본은 미션 시작에 머리 값으로 표를 채우고
/// (Mission.cpp FUN_00482eb0), 튜토리얼 단계 처리가 실행 중에 표를 바꾼다 — 예: 튜토리얼 2 는 단계 B 에서 sunFactory 를 허용한다
/// (docs/exe/mission-header-flags.md). 그래서 이 표는 <see cref="SetAll"/>·<see cref="Set"/> 으로 바꿀 수 있다.
/// 머리 값 해석 순서: ';' 로 나눈 토큰을 차례로 읽으며 "deny"·"allow" 는 현재 모드를 바꾸고, "all" 은 모든 타입에,
/// 타입 이름은 그 타입에만 현재 모드를 적용한다 (Totalmade.cpp FUN_004c23c0·004c23e0, 조회는 FUN_004c2400).
/// techAllowed 가 없으면 모든 타입이 허용된다.
/// 원본이 이 표를 확인하는 곳은 메뉴·덱(사제 Construct 메뉴 항목, 생산 목록 메뉴, 덱 갱신, 템플의 골렘 항목)이다.
/// </summary>
public sealed class TechPermissions
{
    /// <summary>모든 타입의 기본값 (마지막 "all" 이 정한 값, 처음에는 허용)</summary>
    private bool _defaultAllowed = true;

    /// <summary>타입별로 따로 정한 값 (이름 → 허용 여부, 대소문자 무시)</summary>
    private readonly Dictionary<string, bool> _overrides = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>techAllowed 문자열을 해석한다</summary>
    /// <param name="value">머리 값 (없으면 모두 허용)</param>
    public static TechPermissions Parse(string? value)
    {
        var permissions = new TechPermissions();
        bool allow = true;
        // 토큰을 원본과 같은 순서로 적용한다
        foreach (string raw in (value ?? "").Split(';'))
        {
            string token = raw.Trim();
            if (token.Length == 0)
            {
                continue;
            }
            if (token.Equals("deny", StringComparison.OrdinalIgnoreCase))
            {
                allow = false;
            }
            else if (token.Equals("allow", StringComparison.OrdinalIgnoreCase))
            {
                allow = true;
            }
            else if (token.Equals("all", StringComparison.OrdinalIgnoreCase))
            {
                // "all" 은 앞서 정한 개별 값까지 모두 덮어쓴다 (원본은 표 전체를 다시 채운다)
                permissions.SetAll(allow);
            }
            else
            {
                permissions.Set(token, allow);
            }
        }
        return permissions;
    }

    /// <summary>모든 타입의 허용 여부를 한 값으로 다시 채운다 (원본 FUN_004c23c0: 개별 값은 모두 사라진다)</summary>
    /// <param name="allowed">허용 여부</param>
    public void SetAll(bool allowed)
    {
        _defaultAllowed = allowed;
        _overrides.Clear();
    }

    /// <summary>타입 하나의 허용 여부를 바꾼다 (원본 FUN_004c23e0: 미션 시작 해석과 튜토리얼 단계 처리가 부른다)</summary>
    /// <param name="typeName">.type 이름</param>
    /// <param name="allowed">허용 여부</param>
    public void Set(string typeName, bool allowed) => _overrides[typeName] = allowed;

    /// <summary>타입을 만들거나 등록할 수 있는지</summary>
    /// <param name="typeName">.type 이름</param>
    public bool IsAllowed(string typeName) => _overrides.TryGetValue(typeName, out bool allowed) ? allowed : _defaultAllowed;

    /// <summary>
    /// 같은 값을 가진 독립된 표를 만든다. 표는 실행 중에 바뀌므로 세션마다 자기 복사본을 써야 한다
    /// (같은 <see cref="MissionStart"/> 로 세션을 둘 만들었을 때 한쪽의 변경이 다른 쪽에 새지 않게 한다).
    /// </summary>
    public TechPermissions Clone()
    {
        var copy = new TechPermissions { _defaultAllowed = _defaultAllowed };
        // 개별 값을 그대로 옮긴다
        foreach ((string name, bool allowed) in _overrides)
        {
            copy._overrides[name] = allowed;
        }
        return copy;
    }

    /// <summary>
    /// 표의 내용을 이름 순서로 나열한다 (기본값 뒤에 개별 값). 세션 검사합이 Dictionary 순회 순서에 의존하지 않도록
    /// 이름을 소문자로 바꿔 순서대로 내준다.
    /// </summary>
    public IEnumerable<(string Name, bool Allowed)> Snapshot() =>
        _overrides.Select(pair => (pair.Key.ToLowerInvariant(), pair.Value)).OrderBy(pair => pair.Item1, StringComparer.Ordinal);

    /// <summary>모든 타입의 기본 허용 값 (마지막 "all" 이 정한 값)</summary>
    public bool DefaultAllowed => _defaultAllowed;
}

/// <summary>
/// 미션 머리 값에서 읽은 플레이어 시작 조건 (docs/formats/mission-script.md 머리 값 표).
/// myStartMoney·myTech 의 의미는 문서와 원본 실행 관찰로 확인했다:
/// 튜토리얼 1(myStartMoney = 0)은 0 SP, 튜토리얼 2(10000, myTech = "sunArcher")는 10,000 SP·Sun Disc Thrower 지식으로 시작한다
/// (docs/screens/README.md 1.6·1.7절).
/// </summary>
/// <param name="Title">미션 제목 (title)</param>
/// <param name="LoadFort">불러올 맵 이름 (loadFort, 없으면 미션 파일 이름)</param>
/// <param name="StartStormPower">시작 Storm Power (myStartMoney, 없으면 null — 전투 옵션의 시작 금액을 쓴다)</param>
/// <param name="Knowledge">시작 지식 (myTech 의 ';' 구분 타입 이름)</param>
/// <param name="Tech">만들 수 있는 기술 (techAllowed)</param>
/// <param name="DenySalvage">회수 금지의 시작 값 (denySalvage). 실행 중 바뀔 수 있다 — 튜토리얼 2 는 1 로 시작해 단계 H 에서 0 이 된다</param>
/// <param name="DenyAscend">승천 금지의 시작 값 (denyAscend)</param>
/// <param name="AiOff">AI 끔 (aiOff)</param>
/// <param name="TutorialNumber">튜토리얼 번호 (tutorialNumber, 없으면 null)</param>
public sealed record MissionStart(string? Title, string? LoadFort, int? StartStormPower, IReadOnlyList<string> Knowledge,
    TechPermissions Tech, bool DenySalvage, bool DenyAscend, bool AiOff, int? TutorialNumber)
{
    /// <summary>AI 플레이어 번호의 최대값 (원본 머리 값 ai1~ai8, exe 승패 판정 FUN_004c36c0 도 1~8 을 검사한다)</summary>
    public const int MaximumPlayer = 8;

    /// <summary>
    /// 동맹 목록 (aiNAllyList = "1;4" → N 과 1·4 가 동맹). 한쪽만 적어도 동맹으로 본다.
    /// 3-4 Enemy Territory 의 구출 대상(ai2AllyList = "1")처럼 사람 플레이어와의 동맹도 여기서 온다.
    /// </summary>
    public IReadOnlyDictionary<int, IReadOnlyList<int>> AllyLists { get; init; } = new Dictionary<int, IReadOnlyList<int>>();

    /// <summary>
    /// 사람 플레이어의 동맹 목록 (머리 값 myAllyList = "4;6;7;8"). 사람 플레이어 번호는 세션이 정하므로(<see cref="AreAllied"/> 의 human)
    /// aiNAllyList 와 따로 둔다. 한쪽만 적어도 동맹으로 본다.
    /// </summary>
    public IReadOnlyList<int> MyAllyList { get; init; } = [];

    /// <summary>
    /// 기절하지 않은 사제·동맹 사제도 수송 유닛이 집을 수 있는지 (allowAnyCapture). 구출 미션(3-4 Enemy Territory)이 쓴다.
    /// </summary>
    public bool AllowAnyCapture { get; init; }

    /// <summary>두 플레이어가 미션 동맹인지 (같은 번호 포함)</summary>
    /// <param name="first">플레이어</param>
    /// <param name="second">플레이어</param>
    /// <param name="human">사람 플레이어 번호 (myAllyList 의 주인)</param>
    public bool AreAllied(int first, int second, int human = 1) =>
        first == second
        || ListedBy(first, second, human) || ListedBy(second, first, human);

    /// <summary>owner 의 동맹 목록(사람이면 myAllyList, AI 이면 aiNAllyList)에 other 가 있는지</summary>
    private bool ListedBy(int owner, int other, int human) =>
        (owner == human && MyAllyList.Contains(other))
        || AllyLists.TryGetValue(owner, out IReadOnlyList<int>? list) && list.Contains(other);

    /// <summary>미션 스크립트의 머리 값으로 만든다</summary>
    /// <param name="script">미션 스크립트</param>
    public static MissionStart FromScript(MissionScript script) => FromHeader(script.GetHeader);

    /// <summary>머리 값 조회 함수로 만든다 (값은 원본처럼 따옴표를 벗겨 쓴다)</summary>
    /// <param name="header">키 → 원문 값 (없으면 null)</param>
    public static MissionStart FromHeader(Func<string, string?> header)
    {
        string? Text(string key) => header(key) is { } raw ? raw.Trim().Trim('"').Trim() : null;
        int? Number(string key) => int.TryParse(Text(key), out int value) ? value : null;
        bool Flag(string key) => Number(key) is { } value && value != 0;
        string[] knowledge = (Text("myTech") ?? "").Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        var allies = new Dictionary<int, IReadOnlyList<int>>();
        // 번호 목록 문자열("2;3,4")을 읽는다 (숫자가 아닌 조각은 버린다)
        static int[] Numbers(string? text) => [.. (text ?? "").Split(';', ',', ' ')
            .Select(part => int.TryParse(part, out int number) ? number : 0).Where(number => number > 0)];
        // ai1~ai8 의 동맹 목록을 읽는다
        for (int player = 1; player <= MaximumPlayer; player++)
        {
            int[] list = Numbers(Text($"ai{player}AllyList"));
            if (list.Length > 0)
            {
                allies[player] = list;
            }
        }
        return new MissionStart(Text("title"), Text("loadFort"), Number("myStartMoney"), knowledge,
            TechPermissions.Parse(Text("techAllowed")), Flag("denySalvage"), Flag("denyAscend"), Flag("aiOff"), Number("tutorialNumber"))
        {
            AllyLists = allies,
            MyAllyList = Numbers(Text("myAllyList")),
            AllowAnyCapture = Flag("allowAnyCapture"),
        };
    }

    /// <summary>
    /// 생산 창 상태에 시작 지식을 넣는다. techAllowed 로 막힌 타입은 넣지 않는다.
    /// </summary>
    /// <param name="deck">플레이어 생산 창</param>
    public void ApplyKnowledge(ProductionDeck deck)
    {
        // 시작 지식 중 허용된 타입만 배운다
        foreach (string name in Knowledge.Where(Tech.IsAllowed))
        {
            deck.LearnKnowledge(name);
        }
    }

    /// <summary>시작 Storm Power: 미션 값이 있으면 그것, 없으면 전투 옵션의 시작 금액</summary>
    /// <param name="options">전투 옵션</param>
    public int StormPower(BattleOptions options) => StartStormPower ?? options.StartingStormPower;
}
