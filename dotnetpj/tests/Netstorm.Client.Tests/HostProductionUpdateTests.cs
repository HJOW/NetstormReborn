using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 생산 패널 갱신의 원본 관문과 부분 효과를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 변경 상자와 지연 조회의 순서를 확인한다.</summary>
public sealed class HostProductionUpdateTests
{
    /// <summary>설치·모드·기술 관문이 닫혔을 때 변경 알림을 소비하지 않는다.</summary>
    [Fact]
    public void ClosedGatesPreserveDirtyAndSkipLaterDevices()
    {
        var mode = new SquidOwnerMode();
        var dirty = new UIntBox { Value = 3 };
        Assert.False(HostProductionUpdate.Update(false, mode, dirty, null, null, null));
        Assert.False(HostProductionUpdate.Update(true, mode, dirty, null, null, null));
        mode.Challenge = true;
        Assert.False(HostProductionUpdate.Update(true, mode, dirty, null, null, null));
        mode.Challenge = false;
        mode.Fort = true;
        Assert.False(HostProductionUpdate.Update(true, mode, dirty, () => 0, null, null));
        Assert.Equal(3u, dirty.Value);
        dirty.Value = 0;
        Assert.False(HostProductionUpdate.Update(true, mode, dirty, () => 4, null, null));
    }

    /// <summary>알림 삭제 뒤 재계산과 동기화를 순서대로 호출하고 재진입 변경을 보존한다.</summary>
    [Fact]
    public void DirtyClearsBeforeRefreshAndPreservesLaterChange()
    {
        var mode = new SquidOwnerMode { Battle = true };
        var dirty = new UIntBox { Value = 5 };
        var calls = new List<string>();
        Assert.True(HostProductionUpdate.Update(true, mode, dirty,
            () => { calls.Add("기술"); return 3; },
            () => { Assert.Equal(0u, dirty.Value); calls.Add("계산"); dirty.Value = 7; },
            () => { Assert.Equal(7u, dirty.Value); calls.Add("동기화"); }));
        Assert.Equal(["기술", "계산", "동기화"], calls);
        Assert.Equal(7u, dirty.Value);
    }

    /// <summary>재계산 실패 전에 이미 소비한 알림은 원본처럼 되돌리지 않는다.</summary>
    [Fact]
    public void RefreshFailureKeepsClearedDirtyAndSkipsSync()
    {
        var mode = new SquidOwnerMode { Fort = true };
        var dirty = new UIntBox { Value = 1 };
        Assert.Throws<InvalidOperationException>(() => HostProductionUpdate.Update(true, mode, dirty, () => 1,
            () => throw new InvalidOperationException("계산 실패"), () => throw new Exception("동기화 호출")));
        Assert.Equal(0u, dirty.Value);
    }

    /// <summary>부분 적재 실제 세션에서는 생산 장치 조회나 알림 소비를 시작하지 않는다.</summary>
    [Fact]
    public void PartialSessionDoesNotConsumeProductionChange()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        session.OwnerMode.Fort = true;
        session.ProductionDirty = 4;
        Assert.False(session.UpdateProductionContents());
        Assert.Equal(4u, session.ProductionDirty);
    }
}
