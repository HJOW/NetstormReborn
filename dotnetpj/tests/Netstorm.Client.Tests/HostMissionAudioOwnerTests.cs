using System.Buffers.Binary;
using System.Reflection;
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;

// 실제 장기 음향 소유자의 준비·미준비 관문·공통 음소거·파일 저장소 수명을 창과 장치 없이 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 HostMissionAudio 준비 생성자를 사용한다. 하드웨어 성공이나 worker 가동을 합성하지 않는다.</summary>
[SupportedOSPlatform("windows")]
public sealed class HostMissionAudioOwnerTests
{
    /// <summary>검사 전용 공개 owner나 별도 구현을 만들지 않고 실제 동일 private 필드 identity만 읽는다.</summary>
    private static T Field<T>(HostMissionAudio audio, string name) => (T)(typeof(HostMissionAudio)
        .GetField(name, BindingFlags.NonPublic | BindingFlags.Instance)?.GetValue(audio)
        ?? throw new InvalidOperationException("실제 음향 소유자 필드 없음: " + name));
    /// <summary>보호 사본 읽기 장치를 사용하는 같은 실제 시작 문맥을 준비하며 설정 파일을 저장하지 않는다.</summary>
    private static HostStartupScene Startup()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        return new(data);
    }

    /// <summary>같은 Scene/전역/player/명시 primary를 실제로 조립하되 시각·곡·장치·worker를 시작하지 않는지 확인한다.</summary>
    [Fact]
    public void PreparedOwnerBorrowsActualStateAndPrimaryWithoutStartingResources()
    {
        if (!OperatingSystem.IsWindows()) return;
        var scene = new SceneMusicState(); scene.SongEnd.Value = BitConverter.UInt64BitsToDouble(0x8000000000000000);
        int clockReads = 0;
        using var audio = new HostMissionAudio(Startup(), () => { clockReads++; return 25; }, scene);
        SoundState state = Field<SoundState>(audio, "_state"); SoundPlayer player = Field<SoundPlayer>(audio, "_player");
        MusicChannel primary = Field<MusicChannel>(audio, "_primaryMusic"), secondary = Field<MusicChannel>(audio, "_secondaryMusic");
        SoundMusicControl control = Field<SoundMusicControl>(audio, "_musicControl");
        MusicRuntimeState runtime = Field<MusicRuntimeState>(audio, "_musicRuntimeState");
        Assert.Same(scene, audio.Scene); Assert.True(player.UsesState(state)); Assert.True(control.UsesRuntime(state, primary));
        Assert.Same(primary, control.Primary); Assert.NotSame(primary, secondary);
        Assert.Same(primary.State, Field<MusicChannelState>(audio, "_primaryMusicState"));
        Assert.False(audio.DeviceReady); Assert.False(state.Device); Assert.False(state.MusicInitialized);
        Assert.Equal((nint)0, runtime.Thread); Assert.Equal((nint)0, runtime.Shutdown);
        Assert.Empty(audio.CurrentMusicName); Assert.Equal(0.0, audio.CurrentMusicDuration); Assert.Equal(0, clockReads);
        Assert.Equal(0x8000000000000000UL, BitConverter.DoubleToUInt64Bits(scene.SongEnd.Value));
        audio.SelectMusic("sun22.mus");
        Assert.Equal("sun22.mus", audio.CurrentMusicName); Assert.False(primary.Active()); Assert.Equal(0u, primary.State.Field(MusicField.File));
        Assert.Equal((nint)0, runtime.Thread); Assert.Equal(0, clockReads);
    }

    /// <summary>sound/music 네 조합 모두 실제 최초 Open에 도달한다. 잘못된 HWND 검증은 DLL/장치/worker 생성보다 먼저 실패한다.</summary>
    [Theory]
    [InlineData(false, false)]
    [InlineData(false, true)]
    [InlineData(true, false)]
    [InlineData(true, true)]
    public void InitialOpenIsUnconditionalBeforeActualWindowValidation(bool sound, bool music)
    {
        if (!OperatingSystem.IsWindows()) return;
        using var audio = new HostMissionAudio(Startup(), () => throw new InvalidOperationException("초기 Open의 미선택 시각"));
        var options = new ClientAudioOptions { Sound = sound, Music = music, SoundVolumeStep = 1, MusicVolumeStep = 2 };
        Assert.Throws<ArgumentException>(() => audio.Initialize(0, options));
        Assert.Same(options, audio.AppliedAudioOptions); Assert.Equal(sound, audio.SoundEnabled);
        SoundState state = Field<SoundState>(audio, "_state"); Assert.Equal(-4000, state.MasterVolume); Assert.Equal(-2000, state.MusicVolume);
        Assert.False(audio.DeviceReady); Assert.False(state.Device); Assert.False(state.MusicInitialized);
        Assert.Equal((nint)0, Field<MusicRuntimeState>(audio, "_musicRuntimeState").Thread);
    }

    /// <summary>미준비 실제 소유자에서도 이름/길이 façade가 같은 저장소를 읽고 Scene 기한이나 시각을 대신 갱신하지 않는다.</summary>
    [Fact]
    public void NameAndDurationFacadesReadSameCurrentStorage()
    {
        if (!OperatingSystem.IsWindows()) return;
        var scene = new SceneMusicState(); scene.SongEnd.Value = 77;
        using var audio = new HostMissionAudio(Startup(), () => throw new InvalidOperationException("façade의 미선택 시각"), scene);
        Field<MusicSelectionState>(audio, "_musicSelected").Assign("FANFARE.MUS");
        const ulong duration = 0x7ff8000012345678;
        BinaryPrimitives.WriteUInt64LittleEndian(Field<MusicChannelState>(audio, "_primaryMusicState").Raw().Span.Slice(0x18, 8), duration);
        Assert.Equal("FANFARE.MUS", audio.CurrentMusicName); Assert.Equal(duration, BitConverter.DoubleToUInt64Bits(audio.CurrentMusicDuration));
        Assert.Equal(77.0, scene.SongEnd.Value); Assert.False(audio.DeviceReady);
    }

    /// <summary>sound0 재해석은 실제 장치를 열지 않으며 같은 두 예약 음량/깊이를 보존하고 마지막 Pop에서 복원한다.</summary>
    [Fact]
    public void ReinterpretWhileMutedUsesSamePendingVolumesAndOwners()
    {
        if (!OperatingSystem.IsWindows()) return;
        var scene = new SceneMusicState(); scene.SongEnd.Value = 99;
        using var audio = new HostMissionAudio(Startup(), () => throw new InvalidOperationException("옵션 적용의 미선택 시각"), scene);
        SoundState state = Field<SoundState>(audio, "_state");
        SoundMusicControl control = Field<SoundMusicControl>(audio, "_musicControl"); WindowsMusicFileStore files = Field<WindowsMusicFileStore>(audio, "_musicFiles");
        audio.SelectMusic("wind22.mus"); audio.PushMute(); audio.PushMute();
        Assert.Equal(2, state.VolumeHoldDepth); Assert.Equal(-10000, state.MasterVolume); Assert.Equal(-10000, state.MusicVolume);
        audio.Interpret(0, new() { Sound = false, Music = false, SoundVolumeStep = 1, MusicVolumeStep = 2, MaxSimulSounds = 0, SwapSpeakers = true });
        Assert.Equal(-4000, state.PendingMasterVolume); Assert.Equal(-2000, state.PendingMusicVolume);
        Assert.Equal(-10000, state.MasterVolume); Assert.Equal(-10000, state.MusicVolume); Assert.Equal(2, state.VolumeHoldDepth);
        Assert.Equal(0, state.MaxPlaying); Assert.True(state.SwapSpeakers); Assert.False(audio.SoundEnabled); Assert.False(audio.DeviceReady);
        Assert.Same(control, Field<SoundMusicControl>(audio, "_musicControl")); Assert.Same(files, Field<WindowsMusicFileStore>(audio, "_musicFiles"));
        Assert.Equal("wind22.mus", audio.CurrentMusicName); Assert.Equal(99.0, scene.SongEnd.Value);
        audio.PopMute(); Assert.Equal(1, state.VolumeHoldDepth); Assert.Equal(-10000, state.MasterVolume);
        audio.PopMute(); Assert.Equal(0, state.VolumeHoldDepth); Assert.Equal(-4000, state.MasterVolume); Assert.Equal(-2000, state.MusicVolume);
    }

    /// <summary>재개용 Close는 파일 저장소를 유지하고 최종 Dispose만 실제 열린 Winmm 파일을 닫는다.</summary>
    [Fact]
    public void CloseDeviceKeepsFileStoreAliveAndFinalDisposeClosesOwnedFiles()
    {
        if (!OperatingSystem.IsWindows()) return;
        HostStartupScene startup = Startup();
        using var audio = new HostMissionAudio(startup, () => 0);
        WindowsMusicFileStore files = Field<WindowsMusicFileStore>(audio, "_musicFiles");
        string path = Path.Combine(startup.Files.BaseDirectory, "music", "anticipation.mus");
        var first = new MusicFileHeader(); Assert.True(files.Open(path, first));
        audio.CloseDevice(); Assert.Equal((int)first.DataOffset, files.Position(first.File));
        var second = new MusicFileHeader(); Assert.True(files.Open(path, second)); Assert.True(second.File > first.File);
        audio.Dispose(); audio.Dispose();
        Assert.Equal(-1, files.Position(first.File)); Assert.Equal(-1, files.Position(second.File));
        Assert.Throws<ObjectDisposedException>(() => files.Open(path, new()));
        Assert.Throws<ObjectDisposedException>(() => audio.SelectMusic("sun22.mus"));
    }
}
