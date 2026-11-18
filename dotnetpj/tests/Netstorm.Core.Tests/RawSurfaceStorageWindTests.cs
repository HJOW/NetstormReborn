// 미션 저장소의 매 프레임 주변 바람 진행을 검사하는 네임스페이스다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 미션 저장소 바람 진행을 검사하는 클래스다. 2026-11-19 0.632에서 R4-4 정기 처리용으로 추가했다.
// 근거: cpppj ClientMain.cpp 603행의 매 프레임 UpdateAmbientWind 호출이다.
public sealed class RawSurfaceStorageWindTests
{
    /// <summary>잡음에 들지 않으면 감쇠만 하고 난수를 한 번 소비한다. 2026-11-19 0.632에서 추가했다.</summary>
    [Fact]
    public void UpdateAmbientWindConsumesOneDrawWhenGateClosed()
    {
        var storage = new RawSurfaceStorage();
        var random = new GameRandom(0u);
        storage.UpdateAmbientWind(random);
        Assert.Equal(0.0f, storage.Wind.X);
        Assert.Equal((float)(1.0 * 0.9), storage.Wind.Y);
        Assert.Equal(0.0f, storage.Wind.Angle);
        Assert.Equal(0x2eb4230au, random.State());
    }

    /// <summary>잡음에 들면 두 번째 난수로 바람·각도를 갱신한다. 2026-11-19 0.632에서 추가했다.</summary>
    [Fact]
    public void UpdateAmbientWindConsumesTwoDrawsWhenGateOpen()
    {
        var storage = new RawSurfaceStorage();
        var random = new GameRandom(1u);
        storage.UpdateAmbientWind(random);
        Assert.Equal(0x90015u, random.State());
        Assert.Equal(BitConverter.UInt32BitsToSingle(0xbd75b147u), storage.Wind.X);
        Assert.Equal(BitConverter.UInt32BitsToSingle(0x3f666666u), storage.Wind.Y);
        Assert.True(Math.Abs(storage.Wind.Angle - BitConverter.UInt32BitsToSingle(0x438517ecu)) < 1e-3);
    }

    /// <summary>없는 난수는 실제 호출 전에 거부한다. 2026-11-19 0.632에서 추가했다.</summary>
    [Fact]
    public void UpdateAmbientWindRejectsMissingRandom()
    {
        var storage = new RawSurfaceStorage();
        Assert.Throws<ArgumentNullException>(() => storage.UpdateAmbientWind(null!));
    }
}
