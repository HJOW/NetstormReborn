using Netstorm.Client.Audio;
using Netstorm.Core.Original;

// 미션보다 오래 사는 같은 파일·두 음악 채널·제어·worker·선택기를 실제 장치에 조립한다.
namespace Netstorm.Client.Host;

/// <summary>같은 실제 HostMissionAudio에 음악의 장기 소유자를 추가한다. SceneMusic 감독/프레임은 별도 후속이다.</summary>
public sealed partial class HostMissionAudio
{
    /// <summary>현재 옵션/파일 경로를 미션 교체와 무관하게 같은 시작 소유자에서 읽는다.</summary>
    private readonly HostStartupScene _audioStartup;
    /// <summary>실제 쓰기 실패를 제한된 저장소에 보관하고 종료/합류를 계속 허용하는 같은 로그 owner다.</summary>
    private readonly HostAudioLog _audioLog;
    /// <summary>worker 합류와 장치 종료 뒤까지 실제 열린 Winmm 파일을 소유한다.</summary>
    private readonly WindowsMusicFileStore _musicFiles;
    /// <summary>같은 실제 기본 채널의96BYTE다. 새 미션에서 다시 할당하지 않는다.</summary>
    private readonly MusicChannelState _primaryMusicState = new();
    /// <summary>원문 초기화/삭제에 함께 참여하는 두 번째96BYTE다.</summary>
    private readonly MusicChannelState _secondaryMusicState = new();
    /// <summary>현재 음악의 파일·버퍼·잠금을 소유하는 기본 채널이다.</summary>
    private readonly MusicChannel _primaryMusic;
    /// <summary>기본 채널과 다른 실제 객체인 두 번째 채널이다.</summary>
    private readonly MusicChannel _secondaryMusic;
    /// <summary>기존 효과음과 동일 SoundState 및 명시 기본 채널의 공통 명령/음소거 소유자다.</summary>
    private readonly SoundMusicControl _musicControl;
    /// <summary>두96BYTE 밖에 보관하는 현재 실제 worker 핸들/번호다.</summary>
    private readonly MusicRuntimeState _musicRuntimeState = new();
    /// <summary>같은 음악 명령 잠금과 실제 Win32 협조 worker 수명이다.</summary>
    private readonly MusicRuntime _musicRuntime;
    /// <summary>미션/장치 재개에서 초기화하지 않는 현재256BYTE 이름과 음악 옵션이다.</summary>
    private readonly MusicSelectionState _musicSelected = new();
    /// <summary>공개 Play/Stop이 항상 같은 기본 채널을 선택하도록 만든 실제 함수 슬롯 사본이다.</summary>
    private readonly MusicPlaybackDevices _musicPlayback;
    /// <summary>같은 이름/전역/기본 채널/열린 파일 저장소를 사용하는 곡 선택기다.</summary>
    private readonly MusicSelection _musicSelection;

    /// <summary>실제 장기 소유자만 준비한다. 파일/음향 장치/worker를 열거나 준비 성공을 합성하지 않는다.</summary>
    public HostMissionAudio(HostStartupScene startup, Func<double> wallSeconds, SceneMusicState? scene = null, HostAudioLog? audioLog = null)
    {
        ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(wallSeconds);
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 미션 소리 장치는 Windows 전용이다");
        _audioStartup = startup; _audioLog = audioLog ?? new HostAudioLog(); _wallSeconds = wallSeconds; Scene = scene ?? new SceneMusicState();
        _list.Initialize();
        _device = new WindowsSoundDevice(_list, _state,
            WindowsSoundFiles.MakeDiskResolver(startup.Files.BaseDirectory, startup.Files.SecondaryDirectory ?? ""),
            startup.Configuration.Configuration.Get("currentLanguage") ?? "english", report: ReportMusicAssertion);
        _player = new SoundPlayer(_list, _state, _device.Hooks());
        WindowsMusicFileStore? pendingFiles = null;
        MusicRuntime? pendingRuntime = null;
        try
        {
            pendingFiles = new WindowsMusicFileStore(ReportMusicAssertion);
            MusicChannelHooks channels = _device.BindMusicBuffers(pendingFiles.FileHooks(new() { Log = LogMusic }));
            _primaryMusic = new(OriginalEdition.Patch1078, _primaryMusicState, channels);
            _secondaryMusic = new(OriginalEdition.Patch1078, _secondaryMusicState, channels);
            _musicControl = SoundMusicControl.ForPrimary(OriginalEdition.Patch1078, _state, _player, _primaryMusic);
            MusicBufferHooks buffers = _device.MusicBuffers();
            pendingRuntime = new(_state, _musicRuntimeState, _primaryMusic, _secondaryMusic, _musicControl,
                buffers, WindowsMusicRuntime.CreateHooks(LogMusic, ReportMusicAssertion));
            MusicOpenHooks files = pendingFiles.OpenHooks(WindowsSoundFiles.MakeDiskResolver(startup.Files.BaseDirectory));
            _musicPlayback = _primaryMusic.BindPlaybackDevices(files, buffers);
            _musicSelection = new(_musicSelected, _musicControl, new("music", null), files.Find!, _musicPlayback);
            _musicFiles = pendingFiles; _musicRuntime = pendingRuntime;
        }
        catch
        {
            // 준비 실패에서도 자기 Runtime→장치→파일만 반납하고 생성 중 원래 예외를 보존한다.
            if (pendingRuntime is not null) PreserveConstructionFailure(pendingRuntime.Dispose, "음악 Runtime");
            PreserveConstructionFailure(_device.Dispose, "음향 장치");
            if (pendingFiles is not null) PreserveConstructionFailure(pendingFiles.Dispose, "음악 파일");
            throw;
        }
    }

    /// <summary>원문 음악 채널과 문장을 실제 공통 로그에 전달한다.</summary>
    private void LogMusic(uint channel, string text) => _audioLog.Write($"음향 원문 채널{channel}: {text}");
    /// <summary>원문 Sound.cpp의 진단을 보고하되 실제 Runtime의 정상 복귀 정책은 바꾸지 않는다.</summary>
    private static void ReportMusicAssertion(string expression, int line) => ErrorLog.RecordAssertion(expression, @".\Sound.cpp", unchecked((uint)line));
    /// <summary>부분 생성의 반납 오류를 기록하고 최초 생성 오류를 다른 정리 예외로 바꾸지 않는다.</summary>
    private void PreserveConstructionFailure(Action cleanup, string owner)
    {
        try { cleanup(); }
        catch (Exception error) { LogMusic(9, owner + " 생성 실패 정리: " + error.GetType().FullName); }
    }

    /// <summary>같은 실제 로그 owner의 단계별 첫 실패/횟수를 조회한다. 실패를 정상 기록 완료로 세지 않는다.</summary>
    public HostAudioLog AudioLog => _audioLog;
    /// <summary>worker/주 스레드가 관측한 첫 실제 주 기록 실패다. 장치 종료 뒤에도 보존한다.</summary>
    public Exception? AudioLoggingFailure => _audioLog.FirstFailure;
    /// <summary>같은 실제 현재 이름을 읽는다. 장치 준비나 새 곡 요청을 수행하지 않는다.</summary>
    public string CurrentMusicName => _musicSelected.Current();
    /// <summary>같은 기본 채널+18의 현재 길이를 읽는다. 별도 Scene 길이 사본을 만들지 않는다.</summary>
    public double CurrentMusicDuration => _primaryMusicState.Duration();
    /// <summary>같은 실제 선택기와 기본 채널을 사용하며 미준비 때의 자연 이름 쓰기/재생 거부를 유지한다.</summary>
    public void SelectMusic(string name) { EnsureAudioAlive(); _musicSelection.Select(name); }
    /// <summary>같은 음악 준비 관문과 명시 기본 채널의 Stop을 호출한다.</summary>
    public void StopMusic() { EnsureAudioAlive(); _musicControl.Stop(_musicPlayback); }
    /// <summary>같은 효과음/음악 전역의 보류 깊이를 하나 늘리고 최초 깊이에서 두 예약 음량을 보관한다.</summary>
    public void PushMute() { EnsureAudioAlive(); _musicControl.PushMute(); }
    /// <summary>같은 실제 보류 깊이를 하나 반납하고 마지막 해제에서 최신 예약 음량을 복원한다.</summary>
    public void PopMute() { EnsureAudioAlive(); _musicControl.PopMute(); }
}
