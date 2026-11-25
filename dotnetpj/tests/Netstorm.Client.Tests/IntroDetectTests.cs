using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 시작 detect intro의 현재 경로·파일 존재 판정을 대조한다.
// 2026-11-26 1.052에서 SL-2a intro 감지용으로 추가했다.
public sealed class IntroDetectTests
{
    /// <summary>실제 배포 자산과 장기 Intro 표시값을 묶는다.</summary>
    /// <returns>시작 장면과 표시값이다.</returns>
    private static (HostStartupScene Startup, UIntBox Available) Scene()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        return (new HostStartupScene(data), new UIntBox());
    }

    /// <summary>빈 경로는 파일 장치를 요구하지 않고 0을 둔다.</summary>
    [Fact]
    public void EmptyPathKeepsZeroWithoutProbe()
    {
        var (_, available) = Scene();
        bool probed = false;
        Assert.Equal(0u, RawIntroMenu.DetectIntroMovie(available, () => "", name => { probed = true; return 1u; }));
        Assert.Equal(0u, available.Value);
        Assert.False(probed);
    }

    /// <summary>없는 영상은 0을 둔다.</summary>
    [Fact]
    public void MissingMovieKeepsZero()
    {
        (HostStartupScene startup, UIntBox available) = Scene();
        Assert.Equal(0u, RawIntroMenu.DetectIntroMovie(available, () => "d/no-such-movie.avi",
            name => startup.Files.TryReadAllBytes(name) is not null ? 1u : 0u));
        Assert.Equal(0u, available.Value);
    }

    /// <summary>있는 파일은 NUL 뒤를 잘라도 1을 쓴다.</summary>
    [Fact]
    public void PresentMovieWritesOne()
    {
        (HostStartupScene startup, UIntBox available) = Scene();
        // 실제 배포 SHP가 존재하므로 영상처럼 감지된다.
        Assert.Equal(1u, RawIntroMenu.DetectIntroMovie(available, () => "d/_shapes.shp\0stale",
            name => startup.Files.TryReadAllBytes(name) is not null ? 1u : 0u));
        Assert.Equal(1u, available.Value);
    }

    /// <summary>장치 누락은 명시 실패다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var (_, available) = Scene();
        Assert.Throws<ArgumentNullException>(() => RawIntroMenu.DetectIntroMovie(null!, () => "", _ => 0u));
        Assert.Throws<ArgumentNullException>(() => RawIntroMenu.DetectIntroMovie(available, null!, _ => 0u));
        Assert.Throws<ArgumentNullException>(() => RawIntroMenu.DetectIntroMovie(available, () => "d/x.avi", null!));
    }

    /// <summary>PlayIntro가 아니면 소비하지 않는다.</summary>
    [Fact]
    public void ForeignCommandIsNotConsumed()
    {
        var state = new LocalMissionStateStorage { Next = new byte[0x90] };
        Assert.False(RawIntroMenu.DispatchPlayIntroInput(new(0x020000d9u, 0, 0), state));
        Assert.False(RawIntroMenu.DispatchPlayIntroInput(new(0x1b0000u, 0, 0), state));
        Assert.All(state.Next.ToArray(), value => Assert.Equal(0, value));
    }

    /// <summary>뗌은 소비만 하고 다음 블록을 건드리지 않는다.</summary>
    [Fact]
    public void ReleaseConsumesWithoutWritingNext()
    {
        var state = new LocalMissionStateStorage { Next = new byte[0x90] };
        Assert.True(RawIntroMenu.DispatchPlayIntroInput(new(RawIntroMenu.PlayIntroCommand | InputCode.Release, 0, 0), state));
        Assert.All(state.Next.ToArray(), value => Assert.Equal(0, value));
    }

    /// <summary>누름은 다음 블록에 39·35번 DWORD를 쓴다.</summary>
    [Fact]
    public void PressWritesIntroNextState()
    {
        var state = new LocalMissionStateStorage { Next = new byte[0x90] };
        Assert.True(RawIntroMenu.DispatchPlayIntroInput(new(RawIntroMenu.PlayIntroCommand, 0, 0), state));
        Assert.Equal(39u, System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(state.Next.Span));
        Assert.Equal(1u, System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(state.Next.Span.Slice(35 * 4)));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawIntroMenu.DispatchPlayIntroInput(new(RawIntroMenu.PlayIntroCommand, 0, 0),
            new LocalMissionStateStorage { Next = new byte[8] }));
    }
}
