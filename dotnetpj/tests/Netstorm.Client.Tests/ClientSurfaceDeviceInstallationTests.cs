using Netstorm.Core.Original;

// 장기 Native 장치를 실제 Process 저장소보다 먼저 설치하는 순서와 실패 prefix를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 상태 조회 없이 첫 장치를 설치하고 실패한 설치 뒤 Host를 만들지 않는지 확인한다.</summary>
public sealed class ClientSurfaceDeviceInstallationTests
{
    /// <summary>이 검사에서 선택되지 않아야 하는 실제 전송 경계를 명시 실패로 둔다.</summary>
    private static RawSurfaceTransmissionDevices Transmission() => new()
    {
        Broadcast = _ => throw new IOException("설치 중 방송 선택 금지"), Connected = _ => throw new IOException("설치 중 연결 조회 금지"), Send = (_, _) => throw new IOException("설치 중 송신 선택 금지"),
    };

    /// <summary>같은 전체 선택 DWORD와 Frame/Kernel을 연결하고 설치 이후 늦은 재결합은 거부한다.</summary>
    [Fact]
    public void InstallsNativeDevicesBeforeProcessCreationWithoutReadingCurrentState()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var clock = new ClientClockGlobals(local); var kernel = new Kernel();
        int installations = 0, reads = 0; uint selected = 0x12345678;
        var owner = new ClientSurfaceGlobals(game, clock, kernel, () => throw new IOException("설치 중 시각 선택 금지"), installDevices: surface =>
        {
            installations++; Assert.Same(game, surface.Gameplay); Assert.Same(clock, surface.Clock); Assert.Same(kernel, surface.Kernel);
            surface.Storage.BindTransmissionDevices(Transmission());
            surface.Storage.BindProcessDevices(new() { Selected = () => { reads++; return selected; }, ClearSelection = () => throw new IOException("설치 중 선택 정리 금지") });
        });
        Assert.Equal(1, installations); Assert.Equal(0, reads); Assert.True(owner.Processes.HasDevices()); Assert.True(owner.Processes.UsesKernel(kernel));
        Assert.Equal(selected, owner.Storage.CurrentProcessSelection()); selected = 0xfedcba98; Assert.Equal(selected, owner.Storage.CurrentProcessSelection()); Assert.Equal(2, reads);
        Assert.Throws<ArgumentException>(() => owner.Storage.BindProcessDevices(new() { Selected = () => selected, ClearSelection = () => throw new IOException() }));
    }

    /// <summary>콜백의 앞선 장치는 남지만 예외 뒤 Process 저장소를 만들거나 게임 시각을 읽지 않는다.</summary>
    [Fact]
    public void FailedInstallationPreservesPrefixAndDoesNotCreateProcessStorage()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); RawSurfaceStorage? prefix = null; var failure = new IOException("장기 장치 설치 실패");
        Assert.Same(failure, Assert.Throws<IOException>(() => new ClientSurfaceGlobals(game, new(local), new(), () => throw new IOException("실패 뒤 시각 선택 금지"), installDevices: surface =>
        {
            prefix = surface.Storage; prefix.BindTransmissionDevices(Transmission()); throw failure;
        })));
        Assert.NotNull(prefix); Assert.True(prefix.UsesNativeTransmissionDevices());
        prefix.BindProcessDevices(new() { Selected = () => 0x12345678, ClearSelection = () => throw new IOException("실패 복귀 중 선택 정리 금지") });
        Assert.Equal(0x12345678u, prefix.CurrentProcessSelection()); Assert.False(game.Borrowed());
    }

    /// <summary>다른 Client의 시계는 실제 장치 콜백이나 저장소를 만들기 전에 거부한다.</summary>
    [Fact]
    public void RejectsClockFromAnotherGameplayBeforeInstallation()
    {
        var game = new ClientGameplayGlobals(); var other = new ClientLocalStateGlobals(new(), new()); int calls = 0;
        Assert.Throws<ArgumentException>(() => new ClientSurfaceGlobals(game, new(other), new(), static () => 1000, installDevices: _ => calls++));
        Assert.Equal(0, calls); Assert.False(game.Borrowed());
    }
}
