using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 공통 후처리의 분기·저장 genus·콜백 뒤 전역 조회를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj PostPopCurrentTests의 공통 부모와 실제 마나 진단 뒤 장부를 옮긴다.</summary>
public sealed class PostPopCurrentTests
{
    /// <summary>원문 외부 장치의 명시 변경/실패와 같은 실제 물리만 제공하는 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>네트워크를 열지 않는 실제 client 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>부모가 읽는 실제 슬롯이다.</summary>
        public int Sid;
        /// <summary>현재 필드를 같은 배열에서 바꾸는 TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>현재 억제 관문이다.</summary>
        public bool Suppressed;
        /// <summary>현재 Graph 활성이다.</summary>
        public bool Graphs;
        /// <summary>호출자가 이미 증가해둔 같은 깊이다.</summary>
        public uint Depth = 2;
        /// <summary>현재 전체 로컬 owner다.</summary>
        public uint Owner = 1;
        /// <summary>현재 Graph 활성 조회 횟수다.</summary>
        public uint GraphReads;
        /// <summary>현재 TYPE 조회 횟수다.</summary>
        public uint TypeReads;
        /// <summary>실제로 선택한 효과의 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>해당 장치에서 수행할 명시 외부 효과다.</summary>
        public Action<string>? Change;
        /// <summary>실제 부모에 전달하는 장치다.</summary>
        public readonly SquidPostPopParentHooks Hooks;
        /// <summary>free 여부와 별개로 같은 현재 물리를 조회한다.</summary>
        public Span<byte> Raw => Pool.RawSlot(Sid);

        /// <summary>분기 결과를 계산하지 않고 도달 장치의 인자만 기록한다.</summary>
        public Scene()
        {
            Sid = Pool.Allocate(SidAllocation.Client); Raw[10] = 74; Raw[34] = 1;
            Hooks = new()
            {
                Suppressed = () => Suppressed, GraphsEnabled = () => { GraphReads++; return Graphs; },
                Type = type => { TypeReads++; return Types[type]; },
                Region = sid => { Assert.Equal(Sid, sid); Event("region"); }, Add = sid => { Assert.Equal(Sid, sid); Event("add"); }, ResetGraph = sid => { Assert.Equal(Sid, sid); Event("reset"); },
                Assertion = (expression, file, line) => { Assert.Equal("!(popFlags&pfDESTROY_GRAPH)", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x80bu, line); Event("assert"); },
                Cost = _ => Event("cost"), Provider = _ => Event("provider"), Factory = _ => Event("factory"), OwnerFactory = (owner, _) => Event($"owner:{owner}"), AiNotice = (owner, _) => Event($"ai:{owner}"),
                LocalOwner = () => Owner, LocalCount = type => Event($"local:{type}"), GlobalCount = type => Event($"global:{type}"), RecheckSelection = () => Event("selection"),
            };
        }
        /// <summary>선택한 효과를 보존한 뒤 현재 외부 동작을 호출한다.</summary>
        private void Event(string value) { Events.Add(value); Action<string>? change = Change; change?.Invoke(value); }
        /// <summary>같은 실제 깊이를 원문 부모가 한 번 감소시킨다.</summary>
        public void Run(uint flags) => RawSquidPostPopParent.Run(Pool, Sid, flags, ref Depth, Hooks);
    }

    /// <summary>억제는 범위 밖 SID/TYPE와 미선택 장치를 읽기 전에 깊이만 감김 감소한다.</summary>
    [Fact]
    public void SuppressedReturnsBeforeSlotOrTypeAndWrapsDepth()
    {
        var s = new Scene { Suppressed = true, Sid = 65535, Depth = 0 }; s.Hooks.Type = null; s.Hooks.GraphsEnabled = null; s.Run(1);
        Assert.Equal(uint.MaxValue, s.Depth); Assert.Equal(0u, s.TypeReads); Assert.Empty(s.Events);
    }
    /// <summary>처음 genus가 영역이며 비활성이면 이후 Graph 재조회도 건너뛴다.</summary>
    [Fact]
    public void RegionDisabledBranchDoesNotRecheckGraphs()
    {
        var s = new Scene(); s.Types[74].Flags2 = 8; s.Hooks.GraphsEnabled = () => ++s.GraphReads > 1; s.Run(0x200);
        Assert.Equal(1u, s.GraphReads); Assert.Empty(s.Events); Assert.Equal(1u, s.Depth);
    }
    /// <summary>영역 하위가 바꾼 현재 활성을 다시 읽어 Add만 생략하고 장부로 진행한다.</summary>
    [Fact]
    public void RegionCallbackDisablesAddBeforeLedger()
    {
        var s = new Scene { Graphs = true }; s.Types[74].Flags2 = 8; s.Types[74].Flags1 = 0x800; s.Change = effect => { if (effect == "region") s.Graphs = false; }; s.Run(3);
        Assert.Equal(2u, s.GraphReads); Assert.Equal(new[] { "region", "cost", "ai:1", "local:74", "global:74", "selection" }, s.Events); Assert.Equal(1u, s.Depth);
    }
    /// <summary>checking과 무관한80b 복귀 뒤 현재 TYPE의 표면 분기를 선택한다.</summary>
    [Fact]
    public void DestroyDiagnosticResumesWithCurrentType()
    {
        var s = new Scene { Graphs = true }; s.Change = effect => { if (effect == "assert") { s.Raw[10] = 75; s.Types[75].Flags1 = 0x800; } }; s.Run(0xa00);
        Assert.Equal(new[] { "assert", "add" }, s.Events); Assert.Equal(1u, s.Depth);
    }
    /// <summary>선택된 진단 중단은 앞 영역 효과와 증가된 깊이를 그대로 남긴다.</summary>
    [Fact]
    public void AbortingDiagnosticKeepsDepthAndEarlierRegion()
    {
        var s = new Scene { Graphs = true }; s.Types[74].Flags2 = 8; s.Change = effect => { if (effect == "assert") throw new InvalidOperationException("명시80b 중단"); };
        Assert.Throws<InvalidOperationException>(() => s.Run(0x801)); Assert.Equal(new[] { "region", "assert" }, s.Events); Assert.Equal(2u, s.Depth);
    }
    /// <summary>NoGraph의 현재 표면 리셋은 장부 앞이며 Add/영역을 실행하지 않는다.</summary>
    [Fact]
    public void NoGraphSurfaceUsesResetBeforeCost()
    {
        var s = new Scene { Graphs = true }; s.Types[74].Flags1 = 0x800; s.Run(0x2000001);
        Assert.Equal(new[] { "reset", "cost", "ai:1", "local:74", "global:74", "selection" }, s.Events); Assert.Equal(1u, s.Depth);
    }
    /// <summary>각 효과가 바꾼 현재 TYPE/owner와 Graph 뒤 저장한 genus의 역할을 구분한다.</summary>
    [Fact]
    public void CurrentTypeAndSavedGenusHaveIndependentRoles()
    {
        var s = new Scene { Graphs = true }; s.Types[74].Flags2 = 8;
        s.Change = effect =>
        {
            if (effect == "region") { s.Raw[10] = 75; s.Types[75].Flags1 = 0x800; }
            else if (effect == "add") { s.Raw[10] = 76; s.Types[76].Flags2 = 0x4200; }
            else if (effect == "cost") { s.Raw[10] = 77; s.Types[77].Flags1 = 0x10000000; }
            else if (effect == "provider") { s.Raw[10] = 78; s.Raw[34] = 3; }
            else if (effect == "owner:3") s.Raw[34] = 4; else if (effect == "factory") s.Raw[34] = 5;
            else if (effect == "ai:5") { s.Raw[34] = 6; s.Owner = 6; s.Raw[10] = 255; } else if (effect == "local:255") s.Raw[10] = 254;
        };
        s.Run(3); Assert.Equal(new[] { "region", "add", "cost", "provider", "owner:3", "factory", "ai:5", "local:255", "global:254", "selection" }, s.Events); Assert.Equal(1u, s.Depth);
    }
    /// <summary>현재 extra1/8/9는 Graph 실행 뒤 장부만 생략한다.</summary>
    [Theory]
    [InlineData(1)]
    [InlineData(8)]
    [InlineData(9)]
    public void AbstractExtraSkipsLedgerButFinishesGraph(byte extra)
    {
        var s = new Scene { Graphs = true }; s.Types[74].Flags1 = 0x800; s.Raw[40] = extra; s.Run(1);
        Assert.Equal(new[] { "add" }, s.Events); Assert.Equal(1u, s.Depth);
    }
    /// <summary>마지막 선택 하위 실패도 앞 장부 효과와 현재 깊이를 되돌리지 않는다.</summary>
    [Fact]
    public void SelectionFailureKeepsEarlierEffectsAndDepth()
    {
        var s = new Scene(); s.Change = effect => { if (effect == "selection") throw new InvalidOperationException("명시 선택 실패"); };
        Assert.Throws<InvalidOperationException>(() => s.Run(1)); Assert.Equal(new[] { "cost", "ai:1", "local:74", "global:74", "selection" }, s.Events); Assert.Equal(2u, s.Depth);
    }
    /// <summary>로컬 getter보다 먼저 읽어둔 owner를 비교한다.</summary>
    [Fact]
    public void OwnerReadPrecedesCurrentLocalOwnerGetter()
    {
        var s = new Scene(); s.Hooks.LocalOwner = () => { s.Raw[34] = 2; return 1; }; s.Run(1);
        Assert.Equal(2, s.Raw[34]); Assert.Equal(new[] { "cost", "ai:1", "local:74", "global:74", "selection" }, s.Events);
    }
    /// <summary>실제 마나 중복 진단 뒤 현재 TYPE/owner와 저장한 공장군을 같은 장부에서 구분한다.</summary>
    [Fact]
    public void ActualManaDiagnosticRetainsSavedFactoryGenusAndCurrentOwner()
    {
        var s = new Scene(); s.Types[74].Flags1 = 0x10000000; s.Types[74].Flags2 = 0x4200; s.Types[74].Cost = 3;
        var state = new RawPostPopState { Depth = 1, LocalOwner = 1 }; state.Providers.Entries = new[] { (uint)s.Sid }; state.Providers.Count = 1;
        state.Factories.Entries = new uint[1]; state.OwnerFactories[3].Entries = new uint[1]; var post = new RawPostPop(s.Pool, s.Types, state); post.BindCurrentTypes(s.Types);
        int diagnoses = 0, rechecks = 0; post.BindPatchLocalOwner(() => 3);
        post.BindManaDiagnostics(new() { Checking = new(1u), Assertion = (expression, file, line) =>
        {
            Assert.Equal("!alreadyThere", expression); Assert.Equal(@"\Ns\O\Mana.cpp", file); Assert.Equal(0x44u, line); Assert.Equal(3, state.CurrentTotalCost);
            diagnoses++; s.Raw[10] = 75; s.Raw[34] = 3;
        } });
        post.BindPatchSelectionRecheck(() => { rechecks++; Assert.Equal(1u, state.GlobalCounts[75]); Assert.Equal(1u, state.LocalCounts[75]); Assert.Equal(1u, state.Depth); }); post.PostPopBase(s.Sid, 1);
        Assert.Equal(1, diagnoses); Assert.Equal(1, rechecks); Assert.Equal(1u, state.Providers.Count); Assert.Equal(1u, state.Factories.Count); Assert.Equal(1u, state.OwnerFactories[3].Count); Assert.Equal(2u, state.ProductionDirty); Assert.Equal(0u, state.Depth);
    }

    /// <summary>실제 기본 장치 함수는 값 사본이며 AI 뒤 현재 전체 owner/TYPE를 읽고 최초 비용 snapshot을 유지한다.</summary>
    [Fact]
    public void DefaultDevicesUseCurrentOwnerAndKeepLoadedCostSnapshot()
    {
        var s = new Scene(); s.Types[74].Cost = 3.75f; var state = new RawPostPopState { Depth = 1, PendingPlacement = true }; state.AiAttached[1] = true;
        var post = new RawPostPop(s.Pool, s.Types, state); post.BindCurrentTypes(s.Types); s.Types[74].Cost = 99.75f;
        uint local = 1; int ownerReads = 0, notices = 0, rechecks = 0;
        var devices = new SquidPostPopDevices
        {
            AiNotice = (owner, sid, created) => { Assert.Equal(1u, owner); Assert.Equal(s.Sid, sid); Assert.True(created); notices++; s.Raw[10] = 75; s.Raw[34] = 2; local = 2; },
            LocalOwner = () => { ownerReads++; return local; },
            RecheckSelection = () => { rechecks++; Assert.Equal(1u, state.Depth); Assert.Equal(1u, state.GlobalCounts[75]); },
        };
        post.BindDefaultPatchDevices(devices); Assert.Equal(0, ownerReads); devices.AiNotice = (_, _, _) => throw new InvalidOperationException("외부 재대입 장치"); devices.LocalOwner = () => 7;
        post.PostPopBase(s.Sid, 1); Assert.Equal(3, state.CurrentTotalCost); Assert.Equal(1, notices); Assert.Equal(1, ownerReads); Assert.Equal(1, rechecks);
        Assert.Equal(1u, state.LocalCounts[75]); Assert.Equal(1u, state.GlobalCounts[75]); Assert.Equal(0u, state.Depth);
    }

    /// <summary>실제 AI/선택 장치 실패의 위치에 따라 비용·목록 알림·통계 앞부분을 남긴다.</summary>
    [Fact]
    public void ActualLedgerKeepsPrefixAtAiAndSelectionFailure()
    {
        var s = new Scene(); s.Types[74].Flags1 = 0x10000000; s.Types[74].Flags2 = 0x4200; s.Types[74].Cost = 3;
        var state = new RawPostPopState { Depth = 1, LocalOwner = 1 }; state.AiAttached[1] = true; var post = new RawPostPop(s.Pool, s.Types, state);
        Assert.Throws<InvalidOperationException>(() => post.PostPopBase(s.Sid, 1)); Assert.Equal(3, state.CurrentTotalCost); Assert.Equal(2u, state.ProductionDirty); Assert.Equal(0u, state.GlobalCounts[74]); Assert.Equal(1u, state.Depth);
        state.AiAttached[1] = false; post.BindPatchSelectionRecheck(() => throw new InvalidOperationException("명시 실제 선택 실패"));
        Assert.Throws<InvalidOperationException>(() => post.PostPopBase(s.Sid, 1)); Assert.Equal(6, state.CurrentTotalCost); Assert.Equal(4u, state.ProductionDirty); Assert.Equal(1u, state.GlobalCounts[74]); Assert.Equal(1u, state.LocalCounts[74]); Assert.Equal(1u, state.Depth);
    }

    /// <summary>명시 사전 검사/Activate의 자산·가상 지원 관문과 직접 PATCH 공통 PostPop을 구분한다.</summary>
    [Fact]
    public void DirectPatchParentDoesNotInsertVirtualOrAssetPreflight()
    {
        var s = new Scene(); s.Raw[10] = 6; s.Raw[34] = 0; s.Types[6].Cost = 2; var state = new RawPostPopState { Depth = 1 }; var post = new RawPostPop(s.Pool, s.Types, state);
        Assert.Throws<InvalidOperationException>(() => post.Validate(s.Sid, 1, default)); Assert.Throws<InvalidOperationException>(() => post.Activate(s.Sid, 1));
        post.PostPop(s.Sid, 1); Assert.Equal(2, state.CurrentTotalCost); Assert.Equal(1u, state.LocalCounts[6]); Assert.Equal(1u, state.GlobalCounts[6]); Assert.Equal(0u, state.Depth);
    }
}
