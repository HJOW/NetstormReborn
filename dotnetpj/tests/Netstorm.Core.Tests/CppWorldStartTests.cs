using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 미션 시작 상태를 cpppj 콘솔 덤프(<c>--dump-world</c>)와 줄 단위로 대조한다 (LEFT_JOBS.dotnetpj.md 6-1·6-2·6-5).
/// 기대값 <c>Fixtures/world-start-1078.tsv</c> 는 <c>dotnetpj/tools/export_cpp_world.py</c> 가 만든다.
/// 원본 x86 실행 결과가 아니라 **cpppj 의 현재 월드 어댑터 출력**이므로, 어댑터가 임의로 정한 부분
/// (소유자를 저장하지 않는 타입의 소유자 1)은 대조에서 구분한다.
/// </summary>
public sealed class CppWorldStartTests
{
    /// <summary>대조하는 미션과 그 미션이 읽는 맵 이름 (cpppj 덤프 순서).</summary>
    private static readonly (string Mission, string Fort)[] Missions =
        [("thewarbegins", "thewarbegins"), ("savetheisland", "savetheisland"), ("tutorial1", "BridgeTheGap"), ("TEST01", "TEST01")];

    /// <summary>cpppj 덤프의 object 줄 하나.</summary>
    /// <param name="Type">타입 자산 이름</param>
    /// <param name="Owner">cpppj 가 정한 소유자</param>
    /// <param name="Territory">영역 번호 (Chaff 는 -1)</param>
    /// <param name="X">칸 x</param>
    /// <param name="Y">칸 y</param>
    /// <param name="Frame">프레임 번호</param>
    /// <param name="Quantity">수량 (saveQB, 없으면 saveQA, 없으면 0)</param>
    /// <param name="HitPoints">시작 HP</param>
    /// <param name="Contents">내용물 수</param>
    private sealed record CppObject(string Type, int Owner, int Territory, int X, int Y, int Frame, int Quantity, double HitPoints, int Contents);

    /// <summary>cpppj 덤프의 player 줄 하나.</summary>
    /// <param name="Number">플레이어 번호</param>
    /// <param name="StormPower">시작 Storm Power</param>
    /// <param name="Color">색 번호</param>
    /// <param name="Allies">동맹 비트 (비트 n = 플레이어 n 과 동맹, 자기 비트 포함)</param>
    /// <param name="Knowledge">시작 지식 머리 값 (';' 구분)</param>
    private sealed record CppPlayer(int Number, int StormPower, int Color, int Allies, string Knowledge);

    /// <summary>fixture 를 한 번만 읽어 미션별로 나눈 값.</summary>
    private static readonly Lazy<(ILookup<string, CppPlayer> Players, ILookup<string, CppObject> Objects)> Fixture = new(Load);

    /// <summary>테스트 출력 폴더의 fixture 를 읽는다. '#' 으로 시작하는 줄은 설명이다.</summary>
    private static (ILookup<string, CppPlayer>, ILookup<string, CppObject>) Load()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "Fixtures", "world-start-1078.tsv");
        var players = new List<(string Mission, CppPlayer Player)>();
        var objects = new List<(string Mission, CppObject Object)>();
        // 줄 종류(player·object)에 따라 열을 읽는다.
        foreach (string line in File.ReadLines(path))
        {
            if (line.Length == 0 || line[0] == '#') continue;
            string[] f = line.Split('\t');
            if (f[0] == "player")
            {
                players.Add((f[1], new CppPlayer(int.Parse(f[2]), int.Parse(f[3]), int.Parse(f[4]), int.Parse(f[5]), f[7])));
            }
            else if (f[0] == "object")
            {
                objects.Add((f[1], new CppObject(f[3], int.Parse(f[4]), int.Parse(f[5]), int.Parse(f[6]), int.Parse(f[7]),
                    int.Parse(f[8]), int.Parse(f[9]), double.Parse(f[10], System.Globalization.CultureInfo.InvariantCulture), int.Parse(f[11]))));
            }
        }
        return (players.ToLookup(p => p.Mission, p => p.Player), objects.ToLookup(o => o.Mission, o => o.Object));
    }

    /// <summary>맵의 저장 오브젝트를 cpppj 와 같은 순서(Chaff → 영역 0~19)로 읽는다.</summary>
    /// <param name="fort">맵 이름</param>
    private static IReadOnlyList<FortMapObject> StoredObjects(string fort)
    {
        GameResources resources = OriginalData.RequireResources();
        return new FortMap(resources.LoadFort(fort, OriginalData.RequireTypes())).Objects;
    }

    /// <summary>
    /// 저장 오브젝트의 수·순서·타입·영역·칸 좌표·프레임·수량·내용물 수가 cpppj 월드와 같다 (네 미션 2,036개).
    /// </summary>
    /// <param name="mission">미션 이름</param>
    /// <param name="fort">맵 이름</param>
    /// <param name="count">저장 오브젝트 수 (docs/exe/cpp-world-reconstruction.md 의 "비교한 객체")</param>
    [Theory]
    [InlineData("thewarbegins", "thewarbegins", 172)]
    [InlineData("savetheisland", "savetheisland", 385)]
    [InlineData("tutorial1", "BridgeTheGap", 3)]
    [InlineData("TEST01", "TEST01", 1476)]
    public void StoredObjects_MatchCppWorldLineByLine(string mission, string fort, int count)
    {
        CppObject[] expected = [.. Fixture.Value.Objects[mission]];
        IReadOnlyList<FortMapObject> actual = StoredObjects(fort);
        Assert.Equal(count, expected.Length);
        Assert.Equal(count, actual.Count);
        // 같은 순번의 줄끼리 필드를 비교한다.
        for (int i = 0; i < count; i++)
        {
            FortObject o = actual[i].Object;
            Assert.True(expected[i].Type.Equals(o.Type.Name, StringComparison.OrdinalIgnoreCase), $"{mission} #{i + 1} 타입 {expected[i].Type} != {o.Type.Name}");
            Assert.Equal((expected[i].Territory, expected[i].X, expected[i].Y), (actual[i].Territory ?? -1, actual[i].X, actual[i].Y));
            int frame = o.BridgeShape ?? o.Frame ?? Math.Max(o.Type.Definition.Frames.DefaultFrame, 0);
            Assert.True(expected[i].Frame == frame, $"{mission} #{i + 1} {o.Type.Name} 프레임 {expected[i].Frame} != {frame}");
            Assert.Equal(expected[i].Quantity, o.QB ?? o.QA ?? 0);
            Assert.Equal(expected[i].Contents, o.Contents.Count);
        }
    }

    /// <summary>
    /// 저장 오브젝트의 소유자 해석이 cpppj 와 같다: 정규화한 저장 값(0·9 이상 → 1), geyser·buried·island 는 0.
    /// 소유자를 저장하지 않는 타입을 1 로 두는 것은 cpppj 어댑터의 선택이라, 네 미션에 그런 오브젝트가 없음을 함께 확인한다
    /// (있다면 <see cref="FortObject.LoadOwner"/> 의 0 과 cpppj 의 1 이 갈린다).
    /// </summary>
    [Fact]
    public void StoredOwners_MatchCppWorld()
    {
        int compared = 0;
        int normalized = 0;
        // 미션마다 같은 순번의 오브젝트끼리 소유자를 비교한다.
        foreach ((string mission, string fort) in Missions)
        {
            CppObject[] expected = [.. Fixture.Value.Objects[mission]];
            IReadOnlyList<FortMapObject> actual = StoredObjects(fort);
            // 저장 순서대로 본다.
            for (int i = 0; i < expected.Length; i++)
            {
                FortObject o = actual[i].Object;
                Assert.True(o.OwnerForLoad != null, $"{mission} #{i + 1} {o.Type.Name}: 소유자를 저장하지 않는 타입이 있다 (cpppj 어댑터 값 {expected[i].Owner})");
                Assert.True(expected[i].Owner == o.LoadOwner, $"{mission} #{i + 1} {o.Type.Name}: cpppj {expected[i].Owner} != {o.LoadOwner} (저장 값 {o.Owner})");
                if (o.Owner is int raw && raw != o.LoadOwner && (o.Type.Flags2 & 0x10002002) == 0) normalized++;
                compared++;
            }
        }
        Assert.Equal(2036, compared);
        // 저장 값 0 이 1 로 바뀌는 것은 Save the Island! 의 residence 두 채뿐이다.
        Assert.Equal(2, normalized);
    }

    /// <summary>
    /// 게임이 세션을 만드는 경로(맵 + 미션 시작 조건)로 만든 세션의 엔티티 소유자·시작 HP 가 cpppj 월드와 같다.
    /// noIsland(논리 지면)와 다리는 세션 엔티티가 아니므로 건너뛴다. 시작 HP 는 타입의 maxHitPoints 다 (6-5).
    /// </summary>
    /// <param name="mission">미션 이름</param>
    [Theory]
    [InlineData("thewarbegins")]
    [InlineData("savetheisland")]
    [InlineData("tutorial1")]
    [InlineData("TEST01")]
    public void SessionEntities_StartWithCppOwnersAndHitPoints(string mission)
    {
        (BattleSession session, MissionStart _, FortMap map) = StartSession(mission);
        CppObject[] expected = [.. Fixture.Value.Objects[mission]];
        int entities = 0;
        // 저장 오브젝트마다 대응하는 세션 엔티티를 찾아 비교한다.
        for (int i = 0; i < expected.Length; i++)
        {
            GameEntity? entity = session.EntityForInitial(map.Objects[i]);
            if (entity == null)
            {
                string type = map.Objects[i].Object.Type.Name;
                Assert.True(type == "noIsland" || ObjectKinds.Of(map.Objects[i].Object.Type) == ObjectKind.Bridge, $"{mission} #{i + 1} {type}: 세션 엔티티가 없다");
                continue;
            }
            Assert.True(expected[i].Owner == entity.Owner, $"{mission} #{i + 1} {entity.Type.Name}: 소유자 cpppj {expected[i].Owner} != {entity.Owner}");
            Assert.True(expected[i].HitPoints == entity.MaxHitPoints && entity.HitPoints == entity.MaxHitPoints,
                $"{mission} #{i + 1} {entity.Type.Name}: HP cpppj {expected[i].HitPoints} != {entity.HitPoints}/{entity.MaxHitPoints}");
            entities++;
        }
        Assert.True(entities > 0);
    }

    /// <summary>
    /// 플레이어 구성과 시작 값(Storm Power·시작 지식·동맹·색)이 cpppj 와 같다.
    /// 사람은 myStartMoney·myTech, AI 는 aiNStartMoney·aiNTech(플레이어 2 는 번호 없는 구식 키도 가능, 없으면 0)를 쓴다.
    /// 맵에 저장된 Money(1-1 은 100000)는 미션 시작 값에 쓰이지 않는다.
    /// </summary>
    /// <param name="mission">미션 이름</param>
    /// <param name="humanStormPower">사람 플레이어의 시작 Storm Power (LEFT_JOBS.dotnetpj.md 6-2 의 값)</param>
    [Theory]
    [InlineData("thewarbegins", 3000)]
    [InlineData("savetheisland", 2000)]
    [InlineData("tutorial1", 0)]
    [InlineData("TEST01", 50000)]
    public void Players_StartWithCppValues(string mission, int humanStormPower)
    {
        (BattleSession session, MissionStart start, FortMap _) = StartSession(mission);
        CppPlayer[] expected = [.. Fixture.Value.Players[mission]];
        Assert.Equal(expected.Select(p => p.Number), session.Players.Select(p => p.Number).Order());
        Assert.Equal(humanStormPower, expected.Single(p => p.Number == 1).StormPower);
        IReadOnlyDictionary<int, int> colors = PlayerColors.Table(start.AiColors);
        // cpppj 가 활성으로 본 플레이어마다 시작 값을 비교한다.
        foreach (CppPlayer cpp in expected)
        {
            PlayerState player = session.Player(cpp.Number);
            Assert.True(cpp.StormPower == player.StormPower, $"{mission} 플레이어 {cpp.Number}: SP cpppj {cpp.StormPower} != {player.StormPower}");
            IReadOnlyList<string> names = cpp.Number == session.HumanPlayer ? start.Knowledge : start.AiKnowledgeFor(cpp.Number);
            Assert.Equal(cpp.Knowledge, string.Join(';', names));
            // 덱의 지식은 머리 값을 all 확장하고 (사람은) 금지 기술을 뺀 집합이다.
            IEnumerable<string> learned = MissionStart.ExpandKnowledge(names, OriginalData.RequireTypes());
            if (cpp.Number == session.HumanPlayer) learned = learned.Where(start.Tech.IsAllowed);
            Assert.Equal(learned.Select(n => n.ToLowerInvariant()).Distinct().Order(), player.Deck.Knowledge.Select(n => n.ToLowerInvariant()).Order());
            int allies = 0;
            // 동맹 비트: 자기 자신과 미션 동맹인 번호의 비트를 켠다.
            for (int other = 1; other <= MissionStart.MaximumPlayer; other++)
            {
                if (start.AreAllied(cpp.Number, other, session.HumanPlayer)) allies |= 1 << other;
            }
            Assert.True(cpp.Allies == allies, $"{mission} 플레이어 {cpp.Number}: 동맹 비트 cpppj {cpp.Allies} != {allies}");
            Assert.True(cpp.Color == colors[cpp.Number], $"{mission} 플레이어 {cpp.Number}: 색 cpppj {cpp.Color} != {colors[cpp.Number]}");
        }
    }

    /// <summary>
    /// 게임(NetstormGame.LoadMission → FortMapViewer.InitializeSession)과 같은 순서로 미션 세션을 만든다:
    /// 미션 머리 값 → loadFort(없으면 미션 이름) 맵 → 저장된 Money(없으면 5000)를 미션 값이 없을 때의 시작 금액으로 넘긴다.
    /// </summary>
    /// <param name="mission">미션 이름</param>
    private static (BattleSession Session, MissionStart Start, FortMap Map) StartSession(string mission)
    {
        GameResources resources = OriginalData.RequireResources();
        TypeCatalog types = OriginalData.RequireTypes();
        MissionStart start = MissionStart.FromScript(resources.TryLoadMission(mission)!.Script);
        FortFile fort = resources.LoadFort(start.LoadFort ?? mission, types);
        var map = new FortMap(fort);
        TypeDefinition isle = types.Find("isle")!.Definition;
        var terrain = new FortTerrainPreview(map, isle);
        var edgeFarms = FortEdgeFarmPreview.Create(map, terrain, isle, types.Find("edgeFarm")!.Definition);
        int startStormPower = fort.Money is float money && money > 0 ? (int)money : 5000;
        BattleSession session = BattleSessionFactory.Create(map, terrain.IslandCells, edgeFarms.Select(t => (t.X, t.Y)), types, start, 1, startStormPower);
        return (session, start, map);
    }
}
