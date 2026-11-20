using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 실제 Client·Player·Deck·장기 Host의 State 반납 순서와 실패 효과를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>빈 성공값 대신 실제 모듈의 증빙·단일 Clear·대여 종료를 관찰한다.</summary>
public sealed class ClientStateRuntimeRetirementTests
{
    /// <summary>실제 Kernel 소멸 위치에서만 명시 외부 효과를 실행하는 검사 프로세스다.</summary>
    private sealed class OnDispose(Action action) : BaseProcess
    {
        /// <summary>Clear의 실제 가상 소멸에 도달해야 실행되는 관측 장치다.</summary>
        private readonly Action _action = action;
        /// <summary>실제 커널 슬롯 반납에서만 호출하며 프레임 효과를 공급하지 않는다.</summary>
        public override void Dispose() => _action();
    }

    /// <summary>제품과 같은 실제 장기 Client/자산/Player/Host/Deck을 빌리는 검사 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 Gameplay/물리/시계/Kernel의 장기 소유자다.</summary>
        public ClientSurfaceGlobals Surface { get; }
        /// <summary>같은 실제 Player 고정뷰와 프로세스 대여를 가진 제품 세션이다.</summary>
        public HostSession Session { get; }
        /// <summary>실제 Gameplay·Surface 대여를 소유한 외곽 Runtime이다.</summary>
        public HostMissionRuntime Runtime { get; }
        /// <summary>현재 장면의 실제 자산을 읽고 외부 UI/소리는 미선택 입력으로 연결한다.</summary>
        public Scene()
        {
            var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
            var clock = new ClientClockGlobals(local); clock.InitializeFullTime(1000);
            Surface = new(game, clock, new(), () => 1000);
            Session = new(Surface.Kernel, new GameRandom(17), new CrtRandom(23), sharedGameplay: game,
                sharedLocal: local, sharedPlayerRecords: game.PlayerRecords, pool: Surface.Storage.Pool,
                sharedLists: Surface.Storage.SidLists, sharedProcessTime: clock.GameTime, sharedPlayerProcesses: Surface.Processes);
            _ = HostPostPopRuntimeTests.Build(Session, Surface.Storage);
            Session.BuildFlareWorldDevices((_, _, _, _) => Assert.Fail("State 반납 검사에서 미선택 소리"));
            Session.BuildProductionWorldDevices(new()
            {
                ConfigInt = _ => 0, Gump = _ => throw new InvalidOperationException("미선택 Gump 조회"),
                Handle = (_, _, _, _) => throw new InvalidOperationException("미선택 Gump 호출"),
                Command = (_, _, _) => throw new InvalidOperationException("미선택 UI 입력"),
                Log = (_, _) => throw new InvalidOperationException("미선택 생산 로그"),
            });
            Runtime = HostMissionRuntime.Create(Session, Surface.Storage);
        }
        /// <summary>원문 부모의 known0 다음 Core0 선택을 실제 장기 필드에서 실행한다.</summary>
        public void CoreZero()
        {
            Surface.Gameplay.Player.PlayerKnown = 0;
            Surface.Gameplay.CoreData.Select(Surface.Gameplay.Player, 0, 0, 1);
        }
        /// <summary>현재 살아 있는 장치만 소비자→외곽 대여 순서로 반납한다.</summary>
        public void Dispose() { Session.ReleasePlayerRecordViews(); Runtime.Dispose(); }
    }

    /// <summary>각 원문 owner의 실제 AI 정상 복귀와 동일 Deck·Kernel 증빙을 준비한다.</summary>
    private static void Children(ClientStateRuntimeRetirement retirement)
    {
        // 빈 brain의 자연 반환도 각각의 실제 참여 모듈에서 증빙을 소비해야 한다.
        for (uint owner = 1; owner < 9; owner++) retirement.ResetAi(owner);
        retirement.ClearPlayerLinks(); retirement.ClearKernel();
    }

    /// <summary>아홉 실제 Deck과 Kernel을 한 번 반납하고 Core0 뒤 모든 고정뷰가 끝나야 Pool을 비운다.</summary>
    [Fact]
    public void ActualDeckKernelAndPlayerViewsRetireBeforePoolWithoutRewritingGlobals()
    {
        using var scene = new Scene(); var game = scene.Surface.Gameplay; var session = scene.Session;
        uint playerBase = game.PlayerRecords.Base; var tokens = new List<uint>();
        // 실제 아홉 Player의 Deck을 모두 할당해 같은 링크 부모가 실제 블록을 반납하도록 한다.
        for (uint owner = 0; owner < 9; owner++) tokens.Add(session.WorldProductionDeck.Get(owner));
        int destroyed = 0; scene.Surface.Kernel.Add(new OnDispose(() => destroyed++));
        var retirement = new ClientStateRuntimeRetirement(scene.Surface); Children(retirement);
        Assert.Equal(1, destroyed); Assert.False(session.ProcessBindingsReturned());
        // 실제 블록이 사라졌으며 아직 현재 Player 고정뷰와 Surface 대여는 살아 있다.
        foreach (uint token in tokens) Assert.Throws<ArgumentOutOfRangeException>(() => game.ProductionDeckStorage.Bytes(token));
        var cleanup = new ClientStateStorageCleanup(scene.Surface, session.LocalState, new());
        Assert.Throws<InvalidOperationException>(() => cleanup.ResetPool());
        scene.CoreZero(); game.World.Editor = 9; game.FullDeckBox.Value = 0x80000001;
        retirement.Retire(scene.Runtime.RetireForState);
        Assert.True(session.PlayerRecordViewsReleased); Assert.True(scene.Runtime.Released); Assert.False(game.Borrowed()); Assert.False(scene.Surface.Storage.Borrowed());
        Assert.Equal(playerBase, game.PlayerRecords.Base); Assert.Equal(9u, game.World.Editor); Assert.Equal(0x80000001u, game.FullDeckBox.Value); Assert.Equal(1, destroyed);
        cleanup.ResetPool(); ClientPlayerRuntime.Initialize(game); Assert.NotEqual(playerBase, game.PlayerRecords.Base);
    }

    /// <summary>여덟 owner 중 각 하나를 누락해 빈 brain이나 합계만으로 반납을 인정하지 못하게 한다.</summary>
    [Theory]
    [InlineData(1u)] [InlineData(2u)] [InlineData(3u)] [InlineData(4u)]
    [InlineData(5u)] [InlineData(6u)] [InlineData(7u)] [InlineData(8u)]
    public void EveryAiProofIsRequiredAndEmptyOwnerReleaseCanRetry(uint missing)
    {
        using var scene = new Scene(); var retirement = new ClientStateRuntimeRetirement(scene.Surface);
        // 누락 owner 이외의 실제 증빙을 각각 준비한다.
        for (uint owner = 1; owner < 9; owner++) if (owner != missing) retirement.ResetAi(owner);
        retirement.ClearPlayerLinks(); retirement.ClearKernel(); scene.CoreZero(); int callbacks = 0;
        Assert.Throws<InvalidOperationException>(() => retirement.Retire(() => callbacks++)); Assert.Equal(0, callbacks); Assert.False(scene.Session.ProcessBindingsReturned());
        retirement.ResetAi(missing); Assert.Throws<InvalidOperationException>(() => retirement.Retire(null));
        Assert.Throws<InvalidOperationException>(() => retirement.Retire(() => callbacks++));
        Assert.Equal(1, callbacks); Assert.True(scene.Session.ProcessBindingsReturned()); Assert.True(scene.Surface.Storage.Borrowed());
        retirement.Retire(scene.Runtime.RetireForState); Assert.True(scene.Runtime.Released);
    }

    /// <summary>known/Core·링크·단일 Clear가 빠지면 앞선 실제 효과는 유지하고 owner 반납은 실행하지 않는다.</summary>
    [Fact]
    public void CoreLinksAndSingleClearAreIndependentRequiredBoundaries()
    {
        using var scene = new Scene(); var retirement = new ClientStateRuntimeRetirement(scene.Surface); int callbacks = 0;
        scene.Surface.Gameplay.Player.PlayerKnown = 7;
        Assert.Throws<InvalidOperationException>(() => retirement.Retire(() => callbacks++)); scene.CoreZero();
        // 여덟 AI만 정리해도 링크와 Kernel 증빙을 대신할 수 없다.
        for (uint owner = 1; owner < 9; owner++) retirement.ResetAi(owner);
        Assert.Throws<InvalidOperationException>(() => retirement.Retire(() => callbacks++)); retirement.ClearPlayerLinks();
        Assert.Throws<InvalidOperationException>(() => retirement.Retire(() => callbacks++)); Assert.Equal(0, callbacks);
        retirement.ClearKernel(); Assert.Throws<InvalidOperationException>(retirement.ClearKernel);
        var failure = new IOException("실제 owner 반납 실패"); Assert.Same(failure, Assert.Throws<IOException>(() => retirement.Retire(() => throw failure)));
        Assert.True(scene.Session.ProcessBindingsReturned()); Assert.True(scene.Surface.Gameplay.Borrowed());
        retirement.Retire(scene.Runtime.RetireForState);
    }

    /// <summary>단일 Clear 소멸 중 지난 슬롯에 새로 등록된 Regular는 전체 State 반납 뒤에도 남는다.</summary>
    [Fact]
    public void SingleClearResidualIsPreservedInsteadOfShutdownDrain()
    {
        using var scene = new Scene(); var session = scene.Session; int parent = session.BuildPlayerWorldHooks().Create!(152, 2);
        session.Pool.AllocatedBytes(parent)[11] &= unchecked((byte)~RawSidPool.VoidBit);
        RegularProcess? residual = null;
        scene.Surface.Kernel.Add(new OnDispose(() => { }));
        scene.Surface.Kernel.Add(new OnDispose(() => residual = session.PlayerProcesses.AddSharedRegular(parent, 73, 4)));
        var retirement = new ClientStateRuntimeRetirement(scene.Surface); Children(retirement);
        Assert.NotNull(residual); Assert.Same(residual, scene.Surface.Kernel.Get(1)); scene.CoreZero();
        retirement.Retire(scene.Runtime.RetireForState);
        Assert.Equal(1, scene.Surface.Kernel.Size()); Assert.Same(residual, scene.Surface.Kernel.Get(1));
        Assert.Throws<InvalidOperationException>(() => scene.Surface.Processes.Host().Runtime());
    }

    /// <summary>다른 Host 증빙과 실제 배송 중 정리는 현재 대여·Pool·Player를 바꾸기 전에 거부한다.</summary>
    [Fact]
    public void RejectsForeignClearProofAndRuntimeDeliveryReentry()
    {
        using var first = new Scene(); using var second = new Scene();
        var pass = first.Session.ClearProcessesForState();
        Assert.Throws<ArgumentException>(() => second.Session.ReturnProcessBindingsForState(pass)); Assert.False(second.Session.ProcessBindingsReturned());
        using (second.Session.PinRuntime())
        {
            Assert.Throws<InvalidOperationException>(() => new ClientStateRuntimeRetirement(second.Surface));
            Assert.Throws<InvalidOperationException>(second.Session.ClearProcessesForState);
        }
        Assert.True(second.Surface.Storage.Borrowed());
    }

    /// <summary>Clear의 실제 소멸 실패는 앞선 부분 효과를 보존하고 정상 복귀 증빙을 만들지 않는다.</summary>
    [Fact]
    public void ClearFailurePreservesPrefixAndCannotIssueNormalReturnProof()
    {
        using var scene = new Scene(); var retirement = new ClientStateRuntimeRetirement(scene.Surface);
        // 실제 AI/Deck 자식부터 정리하고 단일 Clear가 실패하는 위치만 외부 효과로 지정한다.
        for (uint owner = 1; owner < 9; owner++) retirement.ResetAi(owner);
        retirement.ClearPlayerLinks(); int destroyed = 0; var failure = new IOException("두 번째 실제 Kernel 소멸 실패");
        scene.Surface.Kernel.Add(new OnDispose(() => destroyed++));
        var failingProcess = new OnDispose(() => throw failure); scene.Surface.Kernel.Add(failingProcess);
        Assert.Same(failure, Assert.Throws<IOException>(retirement.ClearKernel)); Assert.Equal(1, destroyed); Assert.Null(scene.Surface.Kernel.Get(1));
        scene.CoreZero(); Assert.Throws<InvalidOperationException>(() => retirement.Retire(scene.Runtime.RetireForState));
        Assert.True(scene.Surface.Gameplay.Borrowed()); Assert.False(scene.Session.ProcessBindingsReturned());
        // Kernel은 소멸 중 예외가 난 같은 포인터의 재삭제를 거부한다. 정상 증빙을 합성해 복구하지 않는다.
        Assert.Same(failingProcess, scene.Surface.Kernel.Get(2));
        Assert.Throws<InvalidOperationException>(() => scene.Surface.Kernel.Remove(2));
    }

    /// <summary>적재 시작의 준비 대여를 인계해도 같은 물리/세대는 다시 초기화되지 않으며 실패는 caller 대여를 유지한다.</summary>
    [Fact]
    public void PreparedSurfaceTransferPreservesGenerationAndCallerOwnsFailedPublication()
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new(), new(17), new(), pool: storage.Pool);
        using var use = storage.AcquirePrepared(); ulong generation = storage.Generation();
        storage.GameAux().TotalCost = 73;
        using (var existing = session.Gameplay.AcquirePrepared())
        {
            Assert.Throws<InvalidOperationException>(() => HostMissionRuntime.CreatePrepared(session, storage, use));
            Assert.True(storage.Borrowed()); Assert.True(use.Uses(storage)); Assert.Equal(generation, storage.Generation());
        }
        using var runtime = HostMissionRuntime.CreatePrepared(session, storage, use);
        Assert.Equal(generation, storage.Generation()); Assert.Equal(73, storage.GameAux().TotalCost); Assert.True(storage.Borrowed());
        runtime.Dispose(); Assert.False(storage.Borrowed()); Assert.False(use.Uses(storage)); session.ReleasePlayerRecordViews();
    }

    /// <summary>첫 메뉴도 실제 Player/Kernel을 사용하고 잘못된 Deck 반납의 앞선 쓰기는 보존한다.</summary>
    [Fact]
    public void FirstMenuUsesActualPlayersAndPreservesFailedCleanupPrefix()
    {
        var game = new ClientGameplayGlobals(); ClientPlayerRuntime.Initialize(game); var local = new ClientLocalStateGlobals(game, new());
        var surface = new ClientSurfaceGlobals(game, new(local), new(), () => 1000); var retirement = new ClientStateRuntimeRetirement(surface);
        Memory<byte> records = game.PlayerRecords.Records;
        BinaryPrimitives.WriteUInt32LittleEndian(records.Span[(2 * 180 + 0xb0)..], 0x12345678);
        Assert.Throws<InvalidOperationException>(() => retirement.ResetAi(2));
        BinaryPrimitives.WriteUInt32LittleEndian(records.Span[(2 * 180 + 0xb0)..], 0);
        BinaryPrimitives.WriteUInt32LittleEndian(records.Span[0x9c..], 77);
        BinaryPrimitives.WriteUInt32LittleEndian(records.Span[(3 * 180 + 0x68)..], 0xdead1234);
        Assert.Throws<ArgumentOutOfRangeException>(retirement.ClearPlayerLinks); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(records.Span[0x9c..]));
        Assert.Equal(0xdead1234u, BinaryPrimitives.ReadUInt32LittleEndian(records.Span[(3 * 180 + 0x68)..]));
        BinaryPrimitives.WriteUInt32LittleEndian(records.Span[(3 * 180 + 0x68)..], 0); retirement.ClearPlayerLinks();
        int destroyed = 0; surface.Kernel.Add(new OnDispose(() => destroyed++)); retirement.ClearKernel();
        game.Player.PlayerKnown = 0; game.CoreData.Select(game.Player, 0, 0, 1);
        retirement.Retire(() => Assert.Fail("첫 메뉴는 실제 Runtime owner가 없음")); Assert.Equal(1, destroyed); Assert.True(game.PlayerRecords.Initialized);
    }

    /// <summary>이전 실제 세션의 종료와 새 Surface 대여 세대를 조정자가 새 대상으로 재선택하지 않는다.</summary>
    [Fact]
    public void StaleCoordinatorCannotRetargetAnotherSurfaceGeneration()
    {
        using var scene = new Scene(); var retirement = new ClientStateRuntimeRetirement(scene.Surface);
        scene.Session.ReleasePlayerRecordViews(); scene.Runtime.Dispose();
        Assert.Throws<InvalidOperationException>(() => retirement.ResetAi(1));
        using var next = scene.Surface.Storage.AcquirePrepared();
        Assert.Throws<InvalidOperationException>(retirement.ClearPlayerLinks);
    }
}
