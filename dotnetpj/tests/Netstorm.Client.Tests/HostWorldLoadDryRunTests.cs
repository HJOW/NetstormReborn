using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

/// <summary>호스트 세션의 공유 저장소로 월드 적재 부모를 만들어 원본 순서대로 드라이런하는지 검사한다.
/// 근거: RawCampaignWorldLoad.Load 11단계와 검사 하네스 LoadScene. 덤프·시작 흐름에는 연결하지 않는다. (2026-11-12 0.437에서 추가했다)</summary>
public sealed class HostWorldLoadDryRunTests
{
    /// <summary>세션 저장소로 만든 적재 부모가 11단계를 순서대로 실행하고 세션 기록에 쓰는지 확인한다.</summary>
    [Fact]
    public void BuildWorldLoadRunsElevenStepsInOrderOnSessionStores()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        session.MissionPlayer.PlayerKnown = 1u;
        session.MissionPlayer.BattleOwner = 1u;
        session.MissionPlayer.TemplateOwner = 2u;
        var order = new List<string>();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 쓰지 않는 번호도 슬롯 검사의 범위 안에 들어야 한다.
        for (int index = 0; index < types.Length; ++index)
        {
            types[index] = new OriginalTypeRecord();
        }

        var load = session.BuildWorldLoad(
            new IslandRuntimeHooks
            {
                SetResidenceOwner = (_, _) => throw new InvalidOperationException("드라이런은 Residence owner를 요청하지 않는다"),
                Assertion = _ => throw new InvalidOperationException("드라이런의 Runtime 진단은 원본 분류와 불일치한다"),
            },
            new FortWorldLoadHooks
            {
                TemplatePresent = _ => true,
                Territory = slot =>
                {
                    order.Add($"territory:{slot}");
                    var stream = new FortTerritoryStream();
                    stream.Set(new byte[120], 0u, 120u);
                    return stream;
                },
                CurrentTemplate = _ => new FortTemplate(),
                Assertion = _ => throw new InvalidOperationException("드라이런의 Territory 진단은 원본 분류와 불일치한다"),
                LoadingDepthChanged = _ => throw new InvalidOperationException("드라이런은 깊이 효과를 요청하지 않는다"),
                BuilderAssertion = _ => throw new InvalidOperationException("드라이런의 빌더 진단은 원본 분류와 불일치한다"),
                Generate = (_, _) => { },
                Objects = (_, _) => { },
                Money = (_, _) => { },
                Badges = (_, _) => { },
                Technology = (_, _) => { },
                PopulateIsland = (_, _) => { },
                RebuildGraph = _ => { },
            },
            types,
            new PlayerWorldHooks
            {
                Find = (_, _, _) =>
                {
                    order.Add("find");
                    return 0;
                },
                Create = (type, flags) =>
                {
                    order.Add($"create:{type}:{flags}");
                    return sid;
                },
                SetOwner = (slot, owner) => order.Add($"owner:{slot}:{owner}"),
                Pop = (slot, _, _, _) => order.Add($"pop:{slot}"),
            },
            new CampaignWorldLoadHooks
            {
                TranslateYou = () =>
                {
                    order.Add("translate");
                    return [(byte)'Y', (byte)'o', (byte)'u', 0];
                },
                SetConfig = (name, handle) => order.Add($"setConfig:{name}:{Convert.ToHexString(handle)}"),
                RefreshWindowTitle = () => order.Add("title"),
                BuilderAssertion = assertion => order.Add($"builderDiag:{assertion}"),
                Generate = (_, region) => order.Add($"generate:{region}"),
                CurrentTemplate = slot =>
                {
                    order.Add($"template:{slot}");
                    return new FortTemplate();
                },
                Objects = (_, slot) => order.Add($"objects:{slot}"),
                RebuildGraph = graph => order.Add($"graph:{graph}"),
                ResetCamera = () => order.Add("camera"),
            });
        load.Load();
        Assert.Equal(
        [
            "translate",
            "setConfig:fort.mySubHandle:596F7500",
            "title",
            "find",
            "create:152:2",
            $"owner:{sid}:1",
            $"pop:{sid}",
            "territory:2",
            "generate:0",
            "template:2",
            "objects:2",
            "graph:False",
            "camera",
        ], order);
        // Template owner 2의 +8 DWORD가 1이다.
        Assert.Equal(1, session.WorldTemplates[2 * 20 + 8]);
        Assert.Equal(0, session.WorldTemplates[2 * 20 + 9]);
        // 전투 1번 기록 0xc에 번역 이름이 복사됐다.
        Assert.Equal([(byte)'Y', (byte)'o', (byte)'u', 0], session.PlayerRecords.AsSpan(1 * 180 + 0xc, 4).ToArray());
    }

    /// <summary>효과 묶음이 없으면 적재 부모 만들기를 거절하는지 확인한다.</summary>
    [Fact]
    public void BuildWorldLoadRejectsMissingHooks()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 쓰지 않는 번호도 슬롯 검사의 범위 안에 들어야 한다.
        for (int index = 0; index < types.Length; ++index)
        {
            types[index] = new OriginalTypeRecord();
        }

        var island = new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        };
        var territory = new FortWorldLoadHooks
        {
            TemplatePresent = _ => true,
            Territory = _ => new FortTerritoryStream(),
            CurrentTemplate = _ => new FortTemplate(),
            Assertion = _ => { },
            LoadingDepthChanged = _ => { },
            BuilderAssertion = _ => { },
            Generate = (_, _) => { },
            Objects = (_, _) => { },
            Money = (_, _) => { },
            Badges = (_, _) => { },
            Technology = (_, _) => { },
            PopulateIsland = (_, _) => { },
            RebuildGraph = _ => { },
        };
        var player = new PlayerWorldHooks
        {
            Find = (_, _, _) => 0,
            Create = (_, _) => 0,
            SetOwner = (_, _) => { },
            Pop = (_, _, _, _) => { },
        };
        var hooks = new CampaignWorldLoadHooks
        {
            TranslateYou = () => [(byte)'Y', (byte)'o', (byte)'u', 0],
            SetConfig = (_, _) => { },
            RefreshWindowTitle = () => { },
            BuilderAssertion = _ => { },
            Generate = (_, _) => { },
            CurrentTemplate = _ => new FortTemplate(),
            Objects = (_, _) => { },
            RebuildGraph = _ => { },
            ResetCamera = () => { },
        };
        Assert.Throws<ArgumentNullException>(() => session.BuildWorldLoad(
            null!, territory, types, player, hooks));
        Assert.Throws<ArgumentNullException>(() => session.BuildWorldLoad(
            island, null!, types, player, hooks));
        Assert.Throws<ArgumentNullException>(() => session.BuildWorldLoad(
            island, territory, null!, player, hooks));
        Assert.Throws<ArgumentNullException>(() => session.BuildWorldLoad(
            island, territory, types, null!, hooks));
        Assert.Throws<ArgumentNullException>(() => session.BuildWorldLoad(
            island, territory, types, player, null!));
    }
}
