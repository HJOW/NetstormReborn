using System.Runtime.CompilerServices;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests; // 실제 생산 문맥/검색 owner와 Session 수명을 검사한다.

/// <summary>준비 대여·현재 전역·장기 참조의 실제 공유 경계를 확인한다.</summary>
public sealed class ProductionControlContextTests
{
    /// <summary>현재 장기 전역을 빌리는 최소 실제 세션이다.</summary>
    private static HostSession Session(ClientGameplayGlobals game) => new(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: game);
    /// <summary>검색 뷰만 남긴 경우 이전 Session 전체가 회수되는지 확인할 입력이다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (ClientGameplayGlobals Game, ContainedFinderState Finder, WeakReference<HostSession> Old) Released()
    {
        var game = new ClientGameplayGlobals(); var session = Session(game); return (game, session.ProductionFinder, new(session));
    }
    /// <summary>같은 장기 뷰가 Session을 보유하지 않고 현재 권한/종류/checking을 계속 읽는다.</summary>
    [Fact]
    public void FinderIsSharedAndDoesNotRetainOldSession()
    {
        var value = Released(); Assert.Same(value.Game.ProductionFinder, value.Finder);
        // JIT 지역변수 생존과 finalizer 처리가 끝난 뒤 실제 weak 대상을 관측한다.
        for (int index = 0; index < 3; index++) { GC.Collect(2, GCCollectionMode.Forced, true, true); GC.WaitForPendingFinalizers(); }
        Assert.False(value.Old.TryGetTarget(out _)); value.Game.Authority.Boss = false; value.Game.ContentFormTypeBox.Value = 199; value.Game.AiDebug.Checking.Value = int.MinValue; Assert.False(value.Finder.ReadBoss()); Assert.Equal(199u, value.Finder.ContainedType); Assert.Equal(0x80000000u, value.Finder.CheckingDead.Read());
        var second = Session(value.Game); Assert.Same(value.Finder, second.ProductionFinder); Assert.Equal(199u, second.ProductionFinder.ContainedType);
    }
    /// <summary>준비 대여는 원문 finder를 재초기화하지 않으며 일반 fresh reset만 같은 주소의 기본값을 쓴다.</summary>
    [Fact]
    public void PreparedUsePreservesCurrentFinderAndGeneralResetKeepsIdentity()
    {
        var game = new ClientGameplayGlobals(); var finder = game.ProductionFinder; game.ContentFormTypeBox.Value = 177; game.AiDebug.Checking.Value = 3; game.Authority.Boss = false; Action<string, string, uint> assertion = (_, _, _) => { }; finder.Assertion = assertion;
        using (game.AcquirePrepared()) { Assert.Same(finder, game.ProductionFinder); Assert.Same(assertion, finder.Assertion); Assert.Equal(177u, finder.ContainedType); Assert.Equal(3u, finder.CheckingDead.Read()); }
        Assert.Same(assertion, finder.Assertion); game.ResetProductionFinderForGeneralWorld(); Assert.Same(finder, game.ProductionFinder); Assert.Equal(6u, finder.ContainedType); Assert.Equal(6u, game.ContentFormTypeBox.Value); Assert.Null(finder.Assertion); Assert.False(finder.ReadBoss()); Assert.Equal(3u, finder.CheckingDead.Read());
    }
    /// <summary>문맥은 실제 배열/저장소를 복사하지 않고 현재 물리를 빌린다.</summary>
    [Fact]
    public void ContextBorrowsActualOptionsPlayersStorageAndColorOwner()
    {
        var game = new ClientGameplayGlobals(); var storage = new RawSurfaceStorage(authority: game.Authority, gameAux: game.GameAux); var colors = new OwnerColorTableState(); var context = new ProductionControlContext { Storage = storage, Gameplay = game, Players = game.PlayerRecords, Options = game.BattleOptions, OwnerColorTables = colors };
        Assert.Same(storage, context.Storage); Assert.Same(game, context.Gameplay); Assert.Same(game.PlayerRecords, context.Players); Assert.Same(game.ProductionDeckStorage, context.Players.ProductionDeckStorage); Assert.Same(game.BattleOptions, context.Options); Assert.Same(colors, context.OwnerColorTables); game.BattleOptions[1] = 17; Assert.Equal(17, context.Options[1]); colors.EnabledBox.Value = uint.MaxValue; Assert.Equal(uint.MaxValue, context.OwnerColorTables.Enabled); Assert.Null(context.Sp);
    }
}
