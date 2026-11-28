using System.Reflection;
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 로그 adapter의 실패 보관이 음악 종료 정책·동시 호출·최초 생성 예외와 자원 반납을 보존하는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 ErrorLog의 전역 구현을 바꾸지 않고 선택된 외부 쓰기 경계에만 실패를 입력한다.</summary>
public sealed class HostAudioLogTests
{
    /// <summary>정상 기록은 실제 지정 writer를 호출하고 fallback이나 실패 저장을 만들지 않는다.</summary>
    [Fact]
    public void SuccessfulWriterRemainsTheSelectedActualBoundary()
    {
        var lines = new List<string>();
        var log = new HostAudioLog(lines.Add, _ => throw new InvalidOperationException("미선택 fallback"), _ => throw new InvalidOperationException("미선택 최후 기록"));
        log.Write("실제 주 기록");
        Assert.Equal(new[] { "실제 주 기록" }, lines); Assert.Null(log.FirstFailure); Assert.Equal(0, log.FailureCount);
        Assert.Null(log.FirstFallbackFailure); Assert.Null(log.FirstEmergencyFailure);
    }

    /// <summary>세 단계가 모두 실패해도 실패를 별도로 보관하고 로그 호출 경계 밖으로 다시 던지지 않는다.</summary>
    [Fact]
    public void EveryFailedWriterIsBoundedAndObservableWithoutRethrowing()
    {
        var primary = new IOException("주 기록 실패"); var fallback = new IOException("fallback 실패"); var emergency = new IOException("최후 기록 실패");
        var log = new HostAudioLog(_ => throw primary, _ => throw fallback, _ => throw emergency);
        log.Write("첫 문장"); log.Write("다음 문장");
        Assert.Same(primary, log.FirstFailure); Assert.Same(fallback, log.FirstFallbackFailure); Assert.Same(emergency, log.FirstEmergencyFailure);
        Assert.Equal(2, log.FailureCount); Assert.Equal(2, log.FallbackFailureCount); Assert.Equal(2, log.EmergencyFailureCount);
    }

    /// <summary>worker/주 스레드가 동시에 실패해도 첫 예외를 덮지 않고 모든 실패 횟수만 고정 크기 저장소에 누적한다.</summary>
    [Fact]
    public void ConcurrentFailuresKeepFirstExceptionAndExactCounts()
    {
        int primaryCalls = 0, fallbackCalls = 0, emergencyCalls = 0;
        var log = new HostAudioLog(
            _ => { Interlocked.Increment(ref primaryCalls); throw new IOException("동시 주 기록 실패"); },
            _ => { Interlocked.Increment(ref fallbackCalls); throw new IOException("동시 fallback 실패"); },
            _ => Interlocked.Increment(ref emergencyCalls));
        log.Write("최초 기록");
        Exception? first = log.FirstFailure, firstFallback = log.FirstFallbackFailure;
        // 실제 두 종류 호출자가 경쟁할 때와 같은 공유 adapter에 반복 진입한다.
        Parallel.For(0, 400, _ => log.Write("경쟁 기록"));
        Assert.NotNull(first); Assert.NotNull(firstFallback); Assert.Same(first, log.FirstFailure); Assert.Same(firstFallback, log.FirstFallbackFailure);
        Assert.Equal(401, log.FailureCount); Assert.Equal(401, log.FallbackFailureCount); Assert.Equal(0, log.EmergencyFailureCount);
        Assert.Equal(401, primaryCalls); Assert.Equal(401, fallbackCalls); Assert.Equal(401, emergencyCalls); Assert.Null(log.FirstEmergencyFailure);
    }

    /// <summary>명시 커널 입력에서 종료 신호 실패의 실제 로그가 모두 실패해도 Runtime의 thread/event 닫기 정책을 중단하지 않는다.</summary>
    [Fact]
    public void FailedLoggingDoesNotInterruptRuntimeShutdownPolicy()
    {
        var events = new List<string>(); var failure = new IOException("종료 로그 쓰기 실패");
        var log = new HostAudioLog(_ => throw failure, _ => throw new IOException("종료 fallback 실패"), _ => throw new IOException("종료 최후 기록 실패"));
        var sound = new SoundState { Initialized = true }; // 원문 fixture의 명시 준비 입력이며 실제 음향 장치 성공을 뜻하지 않는다.
        var effects = new SoundPlayer(new SoundList(OriginalEdition.Patch1078), sound, new()
        {
            Status = _ => throw new InvalidOperationException("미선택 효과음 상태"), Play = (_, _) => throw new InvalidOperationException("미선택 효과음 재생"),
            SetPosition = (_, _) => throw new InvalidOperationException("미선택 효과음 위치"), SetVolume = (_, _) => throw new InvalidOperationException("미선택 효과음 음량"),
            SetPan = (_, _) => throw new InvalidOperationException("미선택 효과음 좌우"), Stop = _ => throw new InvalidOperationException("미선택 효과음 정지"),
            Duplicate = (uint _, ref uint _) => throw new InvalidOperationException("미선택 효과음 복제"), Load = _ => throw new InvalidOperationException("미선택 효과음 적재"),
        });
        var channels = new MusicChannelHooks
        {
            SetVolume = (_, _) => throw new InvalidOperationException("미선택 음악 음량"), Stop = _ => throw new InvalidOperationException("미선택 음악 정지"),
            Release = _ => throw new InvalidOperationException("미선택 음악 COM 반납"), Seek = (_, _) => throw new InvalidOperationException("미선택 파일 이동"),
            Read = (_, _) => throw new InvalidOperationException("미선택 파일 읽기"), Close = file => events.Add("file:" + file),
        };
        var primary = new MusicChannel(OriginalEdition.Patch1078, new(), channels);
        var secondary = new MusicChannel(OriginalEdition.Patch1078, new(), channels);
        var music = SoundMusicControl.ForPrimary(OriginalEdition.Patch1078, sound, effects, primary);
        var state = new MusicRuntimeState(); var waits = new Queue<uint>(new[] { MusicRuntime.WaitTimeout, 0u });
        using var runtime = new MusicRuntime(sound, state, primary, secondary, music, new(), new()
        {
            CreateEvent = () => 1, CreateThread = (Func<uint> _, ref uint id) => { id = 77; return 2; },
            Wait = (_, _) => waits.Dequeue(), Signal = _ => false, Terminate = _ => events.Add("terminate"),
            Close = handle => events.Add("close:" + handle), Log = (_, text) => log.Write(text),
        });
        runtime.Initialize(); runtime.Shutdown();
        Assert.Same(failure, log.FirstFailure); Assert.Equal(1, log.FailureCount); Assert.Equal(1, log.FallbackFailureCount); Assert.Equal(1, log.EmergencyFailureCount);
        Assert.Equal(new[] { "file:0", "terminate", "close:2", "close:1" }, events);
        Assert.False(sound.MusicInitialized); Assert.Equal((nint)0, state.Thread); Assert.Equal((nint)0, state.Shutdown);
    }

    /// <summary>생성자의 같은 실제 몸체에서 기록 세 단계가 실패해도 최초 HWND 예외와 같은 파일/장치 반납을 보존한다.</summary>
    [Fact]
    [SupportedOSPlatform("windows")]
    public void FaultedLogPreservesStartupExceptionAndSameActualOwnerCleanup()
    {
        if (!OperatingSystem.IsWindows()) return;
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var firstLogFailure = new IOException("실제 owner 로그 쓰기 실패");
        var log = new HostAudioLog(_ => throw firstLogFailure, _ => throw new IOException("owner fallback 실패"), _ => throw new IOException("owner 최후 기록 실패"));
        using var audio = new HostMissionAudio(startup, () => throw new InvalidOperationException("미선택 시각"), audioLog: log);
        const BindingFlags flags = BindingFlags.NonPublic | BindingFlags.Instance;
        var files = (WindowsMusicFileStore)typeof(HostMissionAudio).GetField("_musicFiles", flags)!.GetValue(audio)!;
        var device = (WindowsSoundDevice)typeof(HostMissionAudio).GetField("_device", flags)!.GetValue(audio)!;
        var header = new MusicFileHeader(); string path = Path.Combine(data, "music", "anticipation.mus"); Assert.True(files.Open(path, header));
        MethodInfo initialize = typeof(HostMissionAudio).GetMethod("InitializeStartup", flags)!;
        TargetInvocationException outer = Assert.Throws<TargetInvocationException>(() => initialize.Invoke(audio, new object[] { (nint)0, false }));
        ArgumentException original = Assert.IsType<ArgumentException>(outer.InnerException); Assert.Equal("window", original.ParamName);
        Assert.Same(log, audio.AudioLog); Assert.Same(firstLogFailure, audio.AudioLoggingFailure);
        Assert.Equal(1, log.FailureCount); Assert.Equal(1, log.FallbackFailureCount); Assert.Equal(1, log.EmergencyFailureCount);
        Assert.Same(files, typeof(HostMissionAudio).GetField("_musicFiles", flags)!.GetValue(audio));
        Assert.Same(device, typeof(HostMissionAudio).GetField("_device", flags)!.GetValue(audio));
        Assert.Equal(-1, files.Position(header.File)); Assert.Throws<ObjectDisposedException>(() => files.Open(path, new()));
        Assert.False(audio.DeviceReady); Assert.Equal((nint)0, (nint)typeof(WindowsSoundDevice).GetField("_device", flags)!.GetValue(device)!);
        Assert.Equal((nint)0, (nint)typeof(WindowsSoundDevice).GetField("_module", flags)!.GetValue(device)!);
        Assert.Throws<ObjectDisposedException>(() => audio.PushMute());
    }
}
