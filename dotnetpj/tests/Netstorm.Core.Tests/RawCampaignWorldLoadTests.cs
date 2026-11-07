// 실제 캠페인 설치 부모의 생성 검증·이름 복사·현재 Player 선택·사제 후처리·Load 순서를 검증한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// RawCampaignWorldLoad의 PATCH 본체를 검증하는 테스트 클래스다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
public sealed class RawCampaignWorldLoadTests
{
    /// <summary>실제 Player 한 기록 크기다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int PlayerStride = 0xb4;

    /// <summary>실제 Template header 크기다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int TemplatesBytes = 180;

    /// <summary>Load 순서를 관찰하는 장면이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private sealed class LoadScene
    {
        /// <summary>같은 Pool을 공유하는 원시 풀이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public RawSidPool Pool = new(32768, false, null, OriginalEdition.Patch1078);

        /// <summary>런타임·빌더가 읽는 현재 해시다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public RawSquidHash Hash = new();

        /// <summary>빌더 attach의 원본 지도다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public ChunkMap World = new();

        /// <summary>3204바이트 섬 저장소다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public byte[] IslandRecords = new byte[RawIslandStorage.Bytes];

        /// <summary>런타임 모드 전역이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public FortFinishOwnerState Modes = new();

        /// <summary>런타임 거주 목록이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public RawPostPopList Residences = new();

        /// <summary>런타임 다시 그리기 전역이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public uint Changed;

        /// <summary>런타임 지도 리비전 전역이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public uint Revision;

        /// <summary>같은 TYPE 프레임 표 188개다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public RiftTypeFrames[] Frames = EmptyFrames();

        /// <summary>현재 9개 Player 바이트다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public byte[] Players = new byte[9 * PlayerStride];

        /// <summary>현재 Player 전역이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public PlayerWorldState State = new() { PlayerKnown = 1u, BattleOwner = 1u, TemplateOwner = 2u };

        /// <summary>현재 Template header 180바이트다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public byte[] Templates = new byte[TemplatesBytes];

        /// <summary>현재 handle 저장소다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public byte[] Handle = new byte[64];

        /// <summary>빌린 my 외부 상자다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public UIntBox MyBox = new();

        /// <summary>빌린 login 외부 상자다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public UIntBox LoginBox = new() { Value = 7u };

        /// <summary>호출 순서 기록이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public List<string> Order = [];

        /// <summary>config에 전달된 이름이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public string ConfigName = string.Empty;

        /// <summary>config에 전달된 handle 바이트다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public byte[] ConfigHandle = [];

        /// <summary>생성된 Player 슬롯 번호다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        public int CreatedSid;

        /// <summary>빈 프레임 표 188개를 만든다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        /// <returns>프레임 표다.</returns>
        private static RiftTypeFrames[] EmptyFrames()
        {
            var frames = new RiftTypeFrames[188];
            var empty = new RiftTypeFrames([]);
            // 모든 슬롯을 빈 표로 초기화하는 반복문이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
            for (int index = 0; index < frames.Length; ++index)
            {
                frames[index] = empty;
            }

            return frames;
        }

        /// <summary>같은 풀의 섬 런타임을 만든다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        /// <returns>섬 런타임이다.</returns>
        public RawIslandRuntime Islands() => new(
            Pool,
            Hash,
            IslandRecords,
            Modes,
            Residences,
            () => Changed,
            value => Changed = value,
            () => Revision,
            value => Revision = value,
            new IslandRuntimeHooks
            {
                SetResidenceOwner = (_, _) => throw new InvalidOperationException("Load 순서는 Residence owner를 요청하지 않는다"),
                Assertion = _ => throw new InvalidOperationException("Load 순서의 Runtime 진단은 원본 분류와 불일치한다"),
            });

        /// <summary>같은 풀의 Territory 부모를 만든다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        /// <returns>Territory 부모다.</returns>
        public RawFortWorldLoad Territory()
        {
            var load = this;
            return new RawFortWorldLoad(
                Pool,
                World,
                Islands(),
                Frames,
                () => 0u,
                () => 2u,
                () => 0,
                () => 0u,
                _ => { },
                new FortWorldLoadHooks
                {
                    TemplatePresent = _ => true,
                    Territory = slot =>
                    {
                        load.Order.Add($"territory:{slot}");
                        var stream = new FortTerritoryStream();
                        stream.Set(new byte[120], 0u, 120u);
                        return stream;
                    },
                    CurrentTemplate = _ => new FortTemplate(),
                    Assertion = _ => throw new InvalidOperationException("Load 순서의 Territory 진단은 원본 분류와 불일치한다"),
                    LoadingDepthChanged = _ => throw new InvalidOperationException("PATCH Load 순서는 깊이 효과를 요청하지 않는다"),
                    BuilderAssertion = _ => throw new InvalidOperationException("Load 순서의 빌더 진단은 원본 분류와 불일치한다"),
                    Generate = (_, _) => { },
                    Objects = (_, _) => { },
                    Money = (_, _) => { },
                    Badges = (_, _) => { },
                    Technology = (_, _) => { },
                    PopulateIsland = (_, _) => { },
                    RebuildGraph = _ => { },
                });
        }

        /// <summary>같은 풀·9기록의 Player 부모를 만든다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        /// <returns>Player 부모다.</returns>
        public RawPlayerWorld Player()
        {
            var load = this;
            var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 쓰지 않는 번호도 슬롯 검사의 범위 안에 들어야 한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
            for (int index = 0; index < types.Length; ++index)
            {
                types[index] = new OriginalTypeRecord();
            }

            int sid = Pool.Allocate(SidAllocation.Client);
            load.CreatedSid = sid;
            var inner = new RawPostPop(Pool, types, new RawPostPopState { Suppressed = true });
            return new RawPlayerWorld(
                Pool,
                inner,
                Players,
                State,
                new PlayerWorldHooks
                {
                    Find = (_, _, _) =>
                    {
                        load.Order.Add("find");
                        return 0;
                    },
                    Create = (type, flags) =>
                    {
                        load.Order.Add($"create:{type}:{flags}");
                        return sid;
                    },
                    SetOwner = (slot, owner) => load.Order.Add($"owner:{slot}:{owner}"),
                    Pop = (slot, _, _, _) => load.Order.Add($"pop:{slot}"),
                });
        }

        /// <summary>순서를 기록하는 훅 묶음을 만든다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        /// <returns>훅 묶음이다.</returns>
        public CampaignWorldLoadHooks Hooks()
        {
            var load = this;
            return new CampaignWorldLoadHooks
            {
                TranslateYou = () =>
                {
                    load.Order.Add("translate");
                    return [(byte)'Y', (byte)'o', (byte)'u', 0];
                },
                SetConfig = (name, handle) =>
                {
                    load.Order.Add($"setConfig:{name}:{Convert.ToHexString(handle)}");
                    load.ConfigName = name;
                    load.ConfigHandle = (byte[])handle.Clone();
                },
                RefreshWindowTitle = () => load.Order.Add("title"),
                BuilderAssertion = assertion => load.Order.Add($"builderDiag:{assertion}"),
                Generate = (_, region) => load.Order.Add($"generate:{region}"),
                CurrentTemplate = slot =>
                {
                    load.Order.Add($"template:{slot}");
                    return new FortTemplate();
                },
                Objects = (_, slot) => load.Order.Add($"objects:{slot}"),
                RebuildGraph = graph => load.Order.Add($"graph:{graph}"),
                ResetCamera = () => load.Order.Add("camera"),
            };
        }

        /// <summary>전역·부모·훅을 연결한 적재 부모를 만든다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
        /// <returns>적재 부모다.</returns>
        public RawCampaignWorldLoad Build() => new(
            World,
            Islands(),
            Frames,
            () => 0u,
            () => 0,
            Territory(),
            Player(),
            CampaignWorldGlobals.BorrowSubscribers(MyBox, LoginBox),
            Templates,
            Handle,
            Hooks());
    }

    /// <summary>같은 풀·저장소·필수 훅이 아니면 거절한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ConstructorRejectsMismatchedPoolAndStorageAndHooks()
    {
        var scene = new LoadScene();
        var valid = scene.Build();
        Assert.NotNull(valid);
        // 다른 풀의 섬 런타임은 거절한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        var other = new LoadScene();
        Assert.Throws<ArgumentException>(() => new RawCampaignWorldLoad(
            scene.World, other.Islands(), scene.Frames, () => 0u, () => 0, scene.Territory(), scene.Player(),
            CampaignWorldGlobals.BorrowSubscribers(scene.MyBox, scene.LoginBox), scene.Templates, scene.Handle, scene.Hooks()));
        // 188개가 아닌 프레임 표는 거절한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        var shortFrames = new List<RiftTypeFrames>(scene.Frames);
        shortFrames.RemoveAt(shortFrames.Count - 1);
        Assert.Throws<ArgumentException>(() => new RawCampaignWorldLoad(
            scene.World, scene.Islands(), shortFrames, () => 0u, () => 0, scene.Territory(), scene.Player(),
            CampaignWorldGlobals.BorrowSubscribers(scene.MyBox, scene.LoginBox), scene.Templates, scene.Handle, scene.Hooks()));
        // 180바이트가 아닌 header는 거절한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        Assert.Throws<ArgumentException>(() => new RawCampaignWorldLoad(
            scene.World, scene.Islands(), scene.Frames, () => 0u, () => 0, scene.Territory(), scene.Player(),
            CampaignWorldGlobals.BorrowSubscribers(scene.MyBox, scene.LoginBox), new byte[179], scene.Handle, scene.Hooks()));
        // 빈 handle 저장소는 거절한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        Assert.Throws<ArgumentException>(() => new RawCampaignWorldLoad(
            scene.World, scene.Islands(), scene.Frames, () => 0u, () => 0, scene.Territory(), scene.Player(),
            CampaignWorldGlobals.BorrowSubscribers(scene.MyBox, scene.LoginBox), scene.Templates, [], scene.Hooks()));
        // 필수 번역 훅이 없으면 거절한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        var hooks = scene.Hooks();
        hooks.TranslateYou = null;
        Assert.Throws<ArgumentException>(() => new RawCampaignWorldLoad(
            scene.World, scene.Islands(), scene.Frames, () => 0u, () => 0, scene.Territory(), scene.Player(),
            CampaignWorldGlobals.BorrowSubscribers(scene.MyBox, scene.LoginBox), scene.Templates, scene.Handle, hooks));
    }

    /// <summary>NUL까지 복사하고 나머지 stale을 보존한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void CopyNameCopiesThroughNulAndPreservesStaleTail()
    {
        byte[] target = [0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA];
        RawCampaignWorldLoad.CopyName(target, [(byte)'Y', (byte)'o', (byte)'u', 0, (byte)'X']);
        Assert.Equal([(byte)'Y', (byte)'o', (byte)'u', 0, 0xAA, 0xAA], target);
    }

    /// <summary>NUL이 없거나 대상이 작으면 거절한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void CopyNameRejectsMissingNulAndSmallTarget()
    {
        byte[] target = new byte[4];
        Assert.Throws<ArgumentOutOfRangeException>(() => RawCampaignWorldLoad.CopyName(target, [(byte)'A', (byte)'B']));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawCampaignWorldLoad.CopyName(target.AsSpan(0, 1), [(byte)'A', 0]));
    }

    /// <summary>현재 battleOwner 기록을 선택하고 범위를 벗어나면 거절한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentPlayerSelectsBattleOwnerRecord()
    {
        var scene = new LoadScene();
        var load = scene.Build();
        scene.Players[1 * PlayerStride + 0x9c] = 0x11;
        scene.Players[2 * PlayerStride + 0x9c] = 0x22;
        Assert.Equal(0x11, load.CurrentPlayer()[0x9c]);
        scene.State.BattleOwner = 9u;
        Assert.Throws<ArgumentOutOfRangeException>(() => load.CurrentPlayer());
    }

    /// <summary>missionPost가 참이면 준비 부모를 호출하지 않는다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void FinishMissionPriestsSkipsWhenPostTrue()
    {
        var scene = new LoadScene();
        var load = scene.Build();
        load.FinishMissionPriests();
        Assert.Empty(scene.Order);
    }

    /// <summary>missionPost가 거짓이면 준비 부모를 요구하고 호출한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void FinishMissionPriestsRequiresHookAndCallsIt()
    {
        var scene = new LoadScene();
        var load = scene.Build();
        load.FinishMissionPriests();
        var globals = new CampaignWorldGlobals(0u, 0u, 0u, false);
        var unhooked = new RawCampaignWorldLoad(
            scene.World, scene.Islands(), scene.Frames, () => 0u, () => 0, scene.Territory(), scene.Player(),
            globals, scene.Templates, scene.Handle, scene.Hooks());
        Assert.Throws<InvalidOperationException>(() => unhooked.FinishMissionPriests());
        int calls = 0;
        var hooks = scene.Hooks();
        hooks.PreparePriests = () => ++calls;
        var hooked = new RawCampaignWorldLoad(
            scene.World, scene.Islands(), scene.Frames, () => 0u, () => 0, scene.Territory(), scene.Player(),
            globals, scene.Templates, scene.Handle, hooks);
        hooked.FinishMissionPriests();
        Assert.Equal(1, calls);
    }

    /// <summary>Load 11단계를 원본 순서로 실행한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadRunsElevenStepsInOrder()
    {
        var scene = new LoadScene();
        var load = scene.Build();
        load.Load();
        // 구독자 복사를 확인한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        Assert.Equal(7u, scene.MyBox.Value);
        // Template owner 2의 +8 DWORD가 1이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        Assert.Equal(1, scene.Templates[2 * 20 + 8]);
        Assert.Equal(0, scene.Templates[2 * 20 + 9]);
        Assert.Equal(0, scene.Templates[2 * 20 + 10]);
        Assert.Equal(0, scene.Templates[2 * 20 + 11]);
        // handle 저장소와 config 전달을 확인한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        Assert.Equal([(byte)'Y', (byte)'o', (byte)'u', 0], scene.Handle.AsSpan(0, 4).ToArray());
        Assert.Equal("fort.mySubHandle", scene.ConfigName);
        Assert.Equal([(byte)'Y', (byte)'o', (byte)'u', 0], scene.ConfigHandle);
        // 현재 Player 기록 0xc에 handle이 복사됐다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        Assert.Equal([(byte)'Y', (byte)'o', (byte)'u', 0], scene.Players.AsSpan(1 * PlayerStride + 0xc, 4).ToArray());
        // 호출 순서를 확인한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        string[] order = [.. scene.Order];
        int create = Array.IndexOf(order, $"create:152:2");
        Assert.Equal(
        [
            "translate",
            "setConfig:fort.mySubHandle:596F7500",
            "title",
            "find",
            $"create:152:2",
            $"owner:{scene.CreatedSid}:1",
            $"pop:{scene.CreatedSid}",
            "territory:2",
            "generate:0",
            "template:2",
            "objects:2",
            "graph:False",
            "camera",
        ], order);
        Assert.True(create > 0);
    }

    /// <summary>editor가 0이 아니면 현재 Player의 residence DWORD를 0으로 쓴다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadZeroesEditorResidenceWhenEditorSet()
    {
        var scene = new LoadScene();
        scene.Players.AsSpan(1 * PlayerStride + 0x78, 4).Fill(0xFF);
        var globals = new CampaignWorldGlobals(0u, 0u, 5u, true);
        var load = new RawCampaignWorldLoad(
            scene.World, scene.Islands(), scene.Frames, () => 0u, () => 0, scene.Territory(), scene.Player(),
            globals, scene.Templates, scene.Handle, scene.Hooks());
        load.Load();
        Assert.Equal([0, 0, 0, 0], scene.Players.AsSpan(1 * PlayerStride + 0x78, 4).ToArray());
    }

    /// <summary>missionPost가 거짓이면 Load가 준비 부모를 Graph와 Camera 사이에서 호출한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadCallsPreparePriestsBetweenGraphAndCameraWhenPostCleared()
    {
        var scene = new LoadScene();
        var globals = new CampaignWorldGlobals(0u, 0u, 0u, false);
        var hooks = scene.Hooks();
        hooks.PreparePriests = () => scene.Order.Add("priests");
        var load = new RawCampaignWorldLoad(
            scene.World, scene.Islands(), scene.Frames, () => 0u, () => 0, scene.Territory(), scene.Player(),
            globals, scene.Templates, scene.Handle, hooks);
        load.Load();
        string[] order = [.. scene.Order];
        string[] tail = order[^3..];
        Assert.Equal(["graph:False", "priests", "camera"], tail);
    }
}
