using Netstorm.Core.Original;

// 실제 Client의 장기 Surface와 같은 Kernel/Frame 연결을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>새 미션과 무관한 물리 주소 및 기존 게임값 보존을 검사한다.</summary>
public sealed class ClientSurfaceGlobalsTests
{
    /// <summary>현재 커널의 실제 슬롯을 유지하는 관측 프로세스다.</summary>
    private sealed class Probe : BaseProcess
    {
        /// <summary>시각 대입이나 추가 효과 없이 같은 슬롯의 수명만 검사한다.</summary>
        public override void RunFrame() { }
    }

    /// <summary>Surface의 Pool/보조 전역과 Process의 실제 Kernel/Frame은 같은 장기 Client에서 빌린다.</summary>
    [Fact]
    public void UsesSameGameplayKernelAndFrameDevices()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var clock = new ClientClockGlobals(local); var kernel = new Kernel();
        uint milliseconds = 1000; var owner = new ClientSurfaceGlobals(game, clock, kernel, () => milliseconds);
        Assert.Same(game, owner.Gameplay); Assert.Same(clock, owner.Clock); Assert.Same(kernel, owner.Kernel);
        Assert.Same(game.Authority, owner.Storage.Authority()); Assert.Same(game.GameAux, owner.Storage.GameAux());
        Assert.Same(owner.Processes, owner.Storage.Processes()); Assert.True(owner.Processes.HasDevices()); Assert.True(owner.Processes.UsesKernel(kernel));
        Assert.Same(game.GameAux.GeyserTypeBox, owner.Processes.State().SharedPollType);
        clock.Frame.Value.Game = 17.5; Assert.Equal(17.5, owner.Processes.State().SharedNow!.Value);
        owner.Processes.State().SharedNow!.Value = 19.25; Assert.Equal(19.25, clock.Frame.Value.Game);
        clock.InitializeFullTime(1000); _ = clock.CaptureFrame(1025); Assert.Equal(0.025, owner.Processes.State().SharedNow!.Value);
        milliseconds = 1050; Assert.Equal(0.05, owner.GameSeconds()); Assert.Equal(0.025, clock.Frame.Value.Game);
        local.GameTicks.Value = 10; Assert.Equal(0.04, owner.GameSeconds()); Assert.Equal(0.025, clock.Frame.Value.Game);
    }

    /// <summary>장기 장치 설치는 이미 존재한 프레임/커널/권한을 초기화하지 않으며 반복 조회도 같은 저장소를 준다.</summary>
    [Fact]
    public void BindingPreservesCurrentFrameKernelAndAuthority()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var clock = new ClientClockGlobals(local); clock.Frame.Value = new FrameTime { Game = 37.25, Wall = 41.5, Delta = 0.125, Number = 73 };
        game.Authority.Server = false; game.Authority.Boss = false;
        var kernel = new Kernel(); var probe = new Probe(); kernel.Add(probe); var owner = new ClientSurfaceGlobals(game, clock, kernel, () => throw new IOException("설치중현재시각조회금지"));
        Assert.Same(probe, kernel.Get(1)); Assert.Equal(37.25, clock.Frame.Value.Game); Assert.Equal(41.5, clock.Frame.Value.Wall); Assert.Equal(0.125, clock.Frame.Value.Delta); Assert.Equal(73ul, clock.Frame.Value.Number);
        Assert.False(game.Authority.Server); Assert.False(game.Authority.Boss); Assert.False(clock.Initialized);
        RawSurfaceStorage same = owner.Storage; owner.Storage.Spots[21] = 0x6d; Assert.Same(same, owner.Storage); Assert.Equal(0x6d, same.Spots[21]);
    }
}
