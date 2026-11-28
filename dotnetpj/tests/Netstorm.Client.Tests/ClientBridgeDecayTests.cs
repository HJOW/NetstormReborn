using Netstorm.Core.Original;

// 메뉴와 실제 세션이 공유하는 전역 붕괴 관문을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>원본 CPP의 메뉴 저장소·현재 모드·권한 identity 관측을 같은 입력으로 재생한다.</summary>
public sealed class ClientBridgeDecayTests
{
    /// <summary>빈 메뉴에서는 World를 빌리지 않고 예산만큼 커서를 진행하며 정지·편집·권한 관문은 유지한다.</summary>
    [Fact]
    public void MenuStorageProgressesWithoutWorldAndHonorsCurrentGates()
    {
        var globals = new ClientGameplayGlobals();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, globals.Authority, globals.GameAux);
        storage.Types.AddRange(Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()));
        RawStateBridgeReset.ResetDecayScan(storage.BridgeDecayScan, 10, 0);
        var time = new FrameTimeBox { Value = new FrameTime { Game = 1, Wall = 1, Delta = 0.5, Number = 1 } };
        ClientBridgeDecay.Run(storage, globals, time, false);
        Assert.Equal(15400, storage.BridgeDecayScan.Cursor); Assert.Equal(10, storage.BridgeDecayScan.Next);
        Assert.False(globals.Borrowed()); Assert.Throws<InvalidOperationException>(() => globals.RequireSession());
        globals.Authority.Boss = false; ClientBridgeDecay.Run(storage, globals, time, false);
        globals.Authority.Boss = true; globals.World.Editor = 1; ClientBridgeDecay.Run(storage, globals, time, false);
        globals.World.Editor = 0; ClientBridgeDecay.Run(storage, globals, time, true);
        Assert.Equal(15400, storage.BridgeDecayScan.Cursor); Assert.Equal(10, storage.BridgeDecayScan.Next);
    }

    /// <summary>같은 값의 다른 권한 상자를 허용하지 않으며 초기 커서도 변경하지 않는다.</summary>
    [Fact]
    public void RejectsForeignAuthorityBeforeScanning()
    {
        var globals = new ClientGameplayGlobals(); var storage = new RawSurfaceStorage();
        Assert.Throws<InvalidOperationException>(() => ClientBridgeDecay.Run(storage, globals, new(), false));
        Assert.Equal(-1, storage.BridgeDecayScan.First); Assert.Equal(0, storage.BridgeDecayScan.Cursor);
    }
}
