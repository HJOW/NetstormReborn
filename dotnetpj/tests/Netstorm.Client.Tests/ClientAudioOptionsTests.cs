using Netstorm.Client.Audio;
using Netstorm.Core.Original;

// 순수 옵션 읽기·음량 점프표와 최초/재해석의 선택 효과 순서를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>CPP ReadClientAudioOptions·VolumeFromStep·ApplyValues·ReinterpretClientAudioOptions와 같은 입력/순서 검사다.</summary>
public sealed class ClientAudioOptionsTests
{
    /// <summary>누락 키에서 실행 파일 초기값을 유지하고 매 읽기마다 같은 기본값에서 명시 키만 덮는지 확인한다.</summary>
    [Fact]
    public void MissingKeysKeepExecutableDefaultsAndReadsDoNotApplyState()
    {
        var configuration = new ConfigInterface(null);
        ClientAudioOptions initial = ClientAudioOptions.Read(configuration);
        Assert.Equal(new ClientAudioOptions(), initial);
        configuration.Set("sound", "0"); configuration.Set("musicVolume", "2");
        ClientAudioOptions changed = ClientAudioOptions.Read(configuration);
        Assert.False(changed.Sound); Assert.True(changed.Music); Assert.Equal(2, changed.MusicVolumeStep);
        Assert.Equal(5, changed.SoundVolumeStep); Assert.Equal(3, changed.Quality); Assert.Equal(8, changed.MaxSimulSounds);
        Assert.True(initial.Sound); Assert.Equal(5, initial.MusicVolumeStep);
    }

    /// <summary>원문 비영 bool과 signed 인자를 보존하며 실제 장치의 품질/음량 정책을 읽기 단계에서 앞당기지 않는다.</summary>
    [Fact]
    public void ExplicitZeroNegativeAndOutOfRangeInputsRemainUnclamped()
    {
        var configuration = new ConfigInterface(null);
        configuration.Configuration.Assign("sound=-1\nmusic=0\nsoundQuality=-7\nsoundVolume=0\nmusicVolume=99\nmaxSimulSounds=-3\nswapLeftRightSpeakers=-2\n");
        ClientAudioOptions actual = ClientAudioOptions.Read(configuration);
        Assert.True(actual.Sound); Assert.False(actual.Music); Assert.True(actual.SwapSpeakers);
        Assert.Equal(-7, actual.Quality); Assert.Equal(0, actual.SoundVolumeStep); Assert.Equal(99, actual.MusicVolumeStep); Assert.Equal(-3, actual.MaxSimulSounds);
    }

    /// <summary>CPP unsigned(step-1) 범위 분기와 점프표의 실제 모든 유효 값·대표 범위 밖 값을 확인한다.</summary>
    [Theory]
    [InlineData(int.MinValue, 0)]
    [InlineData(-1, 0)]
    [InlineData(0, 0)]
    [InlineData(1, -4000)]
    [InlineData(2, -2000)]
    [InlineData(3, -1000)]
    [InlineData(4, -500)]
    [InlineData(5, 0)]
    [InlineData(6, 0)]
    [InlineData(int.MaxValue, 0)]
    public void VolumeUsesNativeUnsignedJumpTable(int step, int expected) => Assert.Equal(expected, ClientAudioOptions.VolumeFromStep(step));

    /// <summary>최초 값 적용은 sound/music과 무관하게 두 음량을 쓰며 아직 Open/Close/Select를 호출하지 않는다.</summary>
    [Fact]
    public void InitialValuesApplyGlobalsThenBothVolumesWithoutDeviceOrSelection()
    {
        var events = new List<string>();
        ClientAudioOptionPolicy.ApplyInitialValues(new() { Sound = false, Music = false, SoundVolumeStep = 1, MusicVolumeStep = 4 }, new()
        {
            Globals = values => events.Add($"G:{values.Sound}:{values.Music}"),
            EffectVolume = volume => events.Add("E:" + volume), MusicVolume = volume => events.Add("M:" + volume),
        });
        Assert.Equal(new[] { "G:False:False", "E:-4000", "M:-500" }, events);
    }

    /// <summary>재해석은 sound의 선택된 장치 하나만 호출하고 뒤 두 음량/현재곡의 순서를 보존한다.</summary>
    [Theory]
    [InlineData(true, "O:2")]
    [InlineData(false, "C")]
    public void ReinterpretUsesSelectedDeviceThenBothVolumesAndCurrentMusic(bool sound, string device)
    {
        var events = new List<string>();
        ClientAudioOptionPolicy.Reinterpret(new() { Sound = sound, Music = false, Quality = 2, SoundVolumeStep = 2, MusicVolumeStep = 3 }, new()
        {
            Globals = _ => events.Add("G"),
            Open = sound ? quality => events.Add("O:" + quality) : null,
            Close = sound ? null : () => events.Add("C"),
            EffectVolume = volume => events.Add("E:" + volume), MusicVolume = volume => events.Add("M:" + volume),
            SelectMusic = enabled => events.Add("S:" + enabled),
        });
        Assert.Equal(new[] { "G", device, "E:-2000", "M:-1000", "S:False" }, events);
    }

    /// <summary>실제 열기 실패의 자연 반환 뒤에도 후속 적용을 진행하되 이미 장치가 끈 Enabled를 전역으로 다시 덮지 않는다.</summary>
    [Fact]
    public void ReturningOpenFailureKeepsItsGlobalWriteAndStillAppliesVolumesAndMusic()
    {
        bool enabled = false; int globalWrites = 0;
        var events = new List<string>();
        ClientAudioOptionPolicy.Reinterpret(new(), new()
        {
            Globals = values => { globalWrites++; enabled = values.Sound; events.Add("G"); },
            Open = _ => { enabled = false; events.Add("O:failure-return"); },
            EffectVolume = _ => events.Add("E"), MusicVolume = _ => events.Add("M"), SelectMusic = _ => events.Add("S"),
        });
        Assert.False(enabled); Assert.Equal(1, globalWrites); Assert.Equal(new[] { "G", "O:failure-return", "E", "M", "S" }, events);
    }

    /// <summary>필수 선택 hook을 미리 요구하지 않고 해당 위치의 이전 효과만 보존한다.</summary>
    [Fact]
    public void MissingSelectedHooksFailAtTheirOwnPosition()
    {
        var events = new List<string>();
        Assert.Throws<InvalidOperationException>(() => ClientAudioOptionPolicy.Reinterpret(new() { Sound = false }, new() { Globals = _ => events.Add("G") }));
        Assert.Equal(new[] { "G" }, events);
        events.Clear();
        Assert.Throws<InvalidOperationException>(() => ClientAudioOptionPolicy.Reinterpret(new(), new()
        {
            Globals = _ => events.Add("G"), Open = _ => events.Add("O"), EffectVolume = _ => events.Add("E"),
        }));
        Assert.Equal(new[] { "G", "O", "E" }, events);
    }

    /// <summary>실제 하위가 예외를 던지면 앞선 효과를 되돌리지 않고 뒤 음량/선택을 실행하지 않는다.</summary>
    [Fact]
    public void ThrowingEffectKeepsPrefixAndStopsLaterHooks()
    {
        var events = new List<string>();
        Assert.Throws<IOException>(() => ClientAudioOptionPolicy.Reinterpret(new(), new()
        {
            Globals = _ => events.Add("G"), Open = _ => events.Add("O"),
            EffectVolume = _ => { events.Add("E"); throw new IOException("명시 음량 적용 예외"); },
            MusicVolume = _ => events.Add("M"), SelectMusic = _ => events.Add("S"),
        }));
        Assert.Equal(new[] { "G", "O", "E" }, events);
    }
}
