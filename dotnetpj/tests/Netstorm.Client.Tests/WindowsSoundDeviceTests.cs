using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Tests;

// Windows 파일/장치 경계를 원본 관측 자료와 실제 디스크·COM 자원으로 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>장치를 열지 않는 원본 WAVE/파일/오프라인 수명 검사다.</summary>
[SupportedOSPlatform("windows")]
public sealed class WindowsSoundDeviceTests
{
    /// <summary>이 검사만 소유하는 고유 임시 폴더를 만들고 해제한다.</summary>
    private sealed class Files : IDisposable
    {
        /// <summary>새로 만든 전용 폴더다.</summary>
        public string Root { get; } = Directory.CreateTempSubdirectory("NetstormDnSound-").FullName;
        /// <summary>전용 폴더 아래에만 입력 자료를 기록한다.</summary>
        public string Write(string name, byte[] bytes)
        {
            string path = Path.Combine(Root, name); Directory.CreateDirectory(Path.GetDirectoryName(path)!); File.WriteAllBytes(path, bytes); return path;
        }
        /// <summary>직접 만든 전용 폴더만 지운다.</summary>
        public void Dispose() => Directory.Delete(Root, true);
    }
    /// <summary>세 실제 PE의435행씩을 실제 Winmm에 공급하고 형식/위치/표본을 모두 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void WaveReaderMatchesNativeObservations(string editionName, OriginalEdition edition)
    {
        if (!OperatingSystem.IsWindows()) return;
        string[][] rows = X86Fixture.Read("sounddevice", editionName); Assert.Equal(435, rows.Length); using var files = new Files(); int reports = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            SoundWave? wave = WindowsSoundFiles.ReadWave(files.Write("case.wav", X86Fixture.Bytes(row[1])), edition, (_, _) => reports++);
            Assert.Equal(row[2] == "1", wave is not null);
            if (wave is not null) { Assert.Equal(X86Fixture.Bytes(row[3]), wave.Format); Assert.Equal(X86Fixture.UInt(row[4]), wave.DataOffset); Assert.Equal(X86Fixture.Bytes(row[5]), wave.Samples); }
            return true;
        });
        Assert.Equal(0, reports);
    }
    /// <summary>원문 짧은 fmt 보정과 불완전/비PCM 추가 자료의 경계를 검증한다.</summary>
    [Fact]
    public void TruncatedWaveAndReturningNonPcmReportKeepOriginalOrder()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var files = new Files(); byte[] bytes = Wave();
        Assert.Null(WindowsSoundFiles.ReadWave(Path.Combine(files.Root, "missing.wav"), OriginalEdition.Patch1078));
        var truncated = WindowsSoundFiles.ReadWave(files.Write("truncated.wav", bytes[..^1]), OriginalEdition.Patch1078); Assert.NotNull(truncated); Assert.Equal(new byte[] { 0x81 }, truncated.Samples);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(40), 0x7fffffff); var huge = WindowsSoundFiles.ReadWave(files.Write("huge.wav", bytes), OriginalEdition.Patch1078); Assert.NotNull(huge); Assert.Equal(2, huge.Samples.Length);
        bytes = Wave(); BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(22), 0); Assert.Null(WindowsSoundFiles.ReadWave(files.Write("zero.wav", bytes), OriginalEdition.Patch1078));
        bytes = Wave(); BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(20), 2); int reports = 0;
        var nonPcm = WindowsSoundFiles.ReadWave(files.Write("nonpcm.wav", bytes), OriginalEdition.Patch1078, (text, line) => { Assert.Equal("wfFormat.wFormatTag == WAVE_FORMAT_PCM", text); Assert.Equal(0xeb, line); reports++; });
        Assert.NotNull(nonPcm); Assert.Equal(1, reports); Assert.Equal((ushort)8, BinaryPrimitives.ReadUInt16LittleEndian(nonPcm.Format.AsSpan(14)));
        bytes = Wave(); var extended = new byte[48]; bytes.AsSpan(0, 36).CopyTo(extended); bytes.AsSpan(36).CopyTo(extended.AsSpan(38));
        BinaryPrimitives.WriteUInt32LittleEndian(extended.AsSpan(4), 40); BinaryPrimitives.WriteUInt32LittleEndian(extended.AsSpan(16), 18); BinaryPrimitives.WriteUInt16LittleEndian(extended.AsSpan(36), 22); BinaryPrimitives.WriteUInt16LittleEndian(extended.AsSpan(20), 2);
        Assert.Null(WindowsSoundFiles.ReadWave(files.Write("codec.wav", extended), OriginalEdition.Patch1078));
        BinaryPrimitives.WriteUInt16LittleEndian(extended.AsSpan(20), 1); var extra = WindowsSoundFiles.ReadWave(files.Write("extra.wav", extended), OriginalEdition.Patch1078); Assert.NotNull(extra); Assert.Equal(22, extra.Format[16]);
        Assert.Null(WindowsSoundFiles.ReadWave(files.Write("short.wav", bytes[..28]), OriginalEdition.Patch1078));
    }
    /// <summary>원문 자료와 독립인 최소 PCM 입력으로 Winmm 잘림/보고를 검증한다.</summary>
    private static byte[] Wave() => Convert.FromHexString("524946462600000057415645666d742010000000010001002256000022560000010008006461746102000000817f");
    /// <summary>원문 여덟 후보와 부분 숫자 감쇠를 결과가 나오는 모든 위치에서 확인한다.</summary>
    [Fact]
    public void SearchOrderAndAttenuationMatchSourceContracts()
    {
        if (!OperatingSystem.IsWindows()) return;
        string[] expected = [@"0:sound\english\beamOver-*.wav", @"0:sound\english\beamOver*.wav", @"0:sound\beamOver-*.wav", @"0:sound\beamOver*.wav", @"1:sound\english\beamOver-*.wav", @"1:sound\english\beamOver*.wav", @"1:sound\beamOver-*.wav", @"1:sound\beamOver*.wav"];
        // 각 후보 하나만 존재하거나 전체가 없을 때도 같은 접두 순서만 소비한다.
        for (int chosen = 0; chosen <= expected.Length; chosen++)
        {
            var visited = new List<string>(); string? path = WindowsSoundFiles.Find("beamOver.wav", "sound", "english", (pattern, alternate) => { visited.Add($"{(alternate ? 1 : 0)}:{pattern}"); return visited.Count == chosen + 1 ? "sound/beamOver-1000.wav" : null; });
            Assert.Equal(chosen < expected.Length, path is not null); Assert.Equal(expected.Take(Math.Min(chosen + 1, expected.Length)), visited);
        }
        Assert.Equal(1000, WindowsSoundFiles.Attenuation("sound/beamOver-1000.wav")); Assert.Equal(-200, WindowsSoundFiles.Attenuation("sound/a--200.wav")); Assert.Equal(17, WindowsSoundFiles.Attenuation("sound/a- 17oops.wav")); Assert.Equal(0, WindowsSoundFiles.Attenuation("sound/a.wav")); Assert.Equal(0, WindowsSoundFiles.Attenuation("root-with-hyphen/a-1000.wav"));
        Assert.Throws<ArgumentException>(() => WindowsSoundFiles.Find("", "sound", "english", (_, _) => null)); Assert.Throws<ArgumentException>(() => WindowsSoundFiles.Find("a\0b", "sound", "english", (_, _) => null));
    }
    /// <summary>실제 FindFirstFile의 대소문자/언어/주보조 결과를 확인한다.</summary>
    [Fact]
    public void RealDiskResolverUsesFirstOriginalPatternAndAlternateRoot()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var files = new Files(); files.Write("sound/a.wav", Wave()); files.Write("sound/a-1000.wav", Wave()); files.Write("sound/english/a-500.wav", Wave());
        var primary = WindowsSoundFiles.MakeDiskResolver(files.Root);
        Assert.Equal(Path.GetFullPath(Path.Combine(files.Root, "sound/english/a-500.wav")), WindowsSoundFiles.Find("A.wav", "sound", "english", primary));
        Assert.Equal(Path.GetFullPath(Path.Combine(files.Root, "sound/a-1000.wav")), WindowsSoundFiles.Find("a.wav", "sound", "korean", primary));
        Assert.Equal(Path.GetFullPath(Path.Combine(files.Root, "sound/english/a-500.wav")), WindowsSoundFiles.Find("a.wav", "sound", "english", WindowsSoundFiles.MakeDiskResolver(Path.Combine(files.Root, "absent"), files.Root)));
        Assert.Null(WindowsSoundFiles.Find("missing.wav", "sound", "english", primary));
    }
    /// <summary>오프라인 장치와 반복 종료가 원문 Buffer 외의 모든 바이트를 보존한다.</summary>
    [Fact]
    public void OfflineDeviceAndShutdownPreserveNamesChainsAndFailureOutput()
    {
        if (!OperatingSystem.IsWindows()) return;
        var list = new SoundList(OriginalEdition.Patch1078); var state = new SoundState { Enabled = true };
        Assert.Throws<ArgumentNullException>(() => new WindowsSoundDevice(list, state, null!));
        using var device = new WindowsSoundDevice(list, state, WindowsSoundFiles.MakeDiskResolver(".")); SoundDeviceHooks hooks = device.Hooks();
        Assert.Equal(0u, hooks.Load!("a.wav").Buffer); uint copy = 23; Assert.True(hooks.Duplicate!(0, ref copy) < 0); Assert.Equal(23u, copy);
        Assert.Throws<ArgumentException>(() => device.Initialize(0));
        uint sound = list.Lookup("a.wav"); list.SetField(sound, SoundField.Buffer, SoundConstants.SilentBuffer); list.SetField(sound, SoundField.Attenuation, 1000); uint duplicate = list.AppendDuplicate(sound, sound, 7); byte[] before = list.Raw().ToArray();
        state.Playing = 3; device.Shutdown(); device.Shutdown(); Assert.False(state.Initialized); Assert.False(state.Device); Assert.True(state.Enabled); Assert.Equal(0, state.Playing);
        byte[] expected = (byte[])before.Clone(); Array.Clear(expected, (int)(sound - SoundConstants.ListBase), 4); Array.Clear(expected, (int)(duplicate - SoundConstants.ListBase), 4); Assert.Equal(expected, list.Raw().ToArray());
        Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Status!(7)); Assert.Equal(0u, new SoundPlayer(list, state, hooks).Play(sound, 0, 0, 0, 0, 0));
    }
    /// <summary>배포 데이터의 모든 WAV를 Winmm으로 읽고 표본이 원본 파일의 동일 위치 바이트임을 확인한다.</summary>
    [Fact]
    public void ShippedWaveBytesRemainExactWithoutOpeningAudio()
    {
        if (!OperatingSystem.IsWindows()) return;
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); int count = 0;
        // 실제 배포 데이터의 모든 효과음 파일을 읽기 전용으로 확인한다.
        foreach (string path in Directory.EnumerateFiles(Path.Combine(data, "sound"), "*", SearchOption.AllDirectories).Where(p => Path.GetExtension(p).Equals(".wav", StringComparison.OrdinalIgnoreCase)))
        {
            SoundWave? wave = WindowsSoundFiles.ReadWave(path, OriginalEdition.Patch1078); Assert.NotNull(wave); byte[] bytes = File.ReadAllBytes(path); Assert.Equal(bytes.AsSpan((int)wave.DataOffset, wave.Samples.Length).ToArray(), wave.Samples); count++;
        }
        Assert.True(count > 100, $"실제 WAV 수가 부족합니다: {count}");
    }
}

/// <summary>명시 실행 때만 실제 장치를 여는 검사이며 다른 검사와 자원을 공유하지 않는다.</summary>
[CollectionDefinition("실제 소리 장치", DisableParallelization = true)]
public sealed class WindowsSoundHardwareCollection;

/// <summary>원문과 같은 숨긴 협력 창/실제 DirectSound 음소거 실행이다.</summary>
[Collection("실제 소리 장치")]
[SupportedOSPlatform("windows")]
public sealed class WindowsSoundHardwareTests
{
    /// <summary>보이지 않는 시스템 STATIC 창을 만들며 포커스를 바꾸지 않는다.</summary>
    [DllImport("user32.dll", EntryPoint = "CreateWindowExW", CharSet = CharSet.Unicode)]
    private static extern nint CreateWindow(uint extended, string className, string name, uint style, int x, int y, int width, int height, nint parent, nint menu, nint instance, nint parameter);
    /// <summary>직접 만든 숨긴 창을 장치 종료 뒤 반납한다.</summary>
    [DllImport("user32.dll", EntryPoint = "DestroyWindow")]
    [return: MarshalAs(UnmanagedType.Bool)] private static extern bool DestroyWindow(nint window);
    /// <summary>원문 실제 검사와 같은 초기화/반복복제/정지/종료/재초기화를 전부 수행한다.</summary>
    [Fact(Explicit = true)]
    public void MutedNativeDeviceLoadsDuplicatesAndRestoresAfterReinitialize()
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Windows 실장치 검사입니다");
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); nint window = CreateWindow(0, "STATIC", "NetstormDn sound inspection", 0x80000000, 0, 0, 1, 1, 0, 0, 0, 0); Assert.NotEqual(0, window);
        try
        {
            var list = new SoundList(OriginalEdition.Patch1078); var state = new SoundState { Enabled = true, MasterVolume = SoundConstants.Minimum };
            using var device = new WindowsSoundDevice(list, state, WindowsSoundFiles.MakeDiskResolver(data)); Assert.True(device.Initialize(window, 3), device.LastInitializationFailure); Assert.False(device.Initialize(window));
            var player = new SoundPlayer(list, state, device.Hooks()); uint sound = list.Lookup("priestForceField.wav"), playing = player.Play(sound, 1, 0, 0, 1, 0), copy = player.Play(sound, 1, 0, 1000, 1, 0);
            Assert.NotEqual(0u, playing); Assert.NotEqual(0u, copy); Assert.NotEqual(playing, copy); Assert.NotEqual(0u, player.IsPlaying(playing)); Assert.NotEqual(0u, player.IsPlaying(copy));
            player.Stop(playing); player.Stop(copy); Assert.Equal(0u, player.IsPlaying(playing)); Assert.Equal(0u, player.IsPlaying(copy)); Assert.Equal(0, state.Playing);
            device.Shutdown(); Assert.True(device.Initialize(window, 0), device.LastInitializationFailure); Assert.Equal(copy, player.Play(copy, 1, 0, 0, 1, 0)); Assert.NotEqual(0u, player.IsPlaying(copy)); player.Stop(copy); device.Shutdown(); device.Shutdown();
        }
        finally { Assert.True(DestroyWindow(window)); }
    }
}
