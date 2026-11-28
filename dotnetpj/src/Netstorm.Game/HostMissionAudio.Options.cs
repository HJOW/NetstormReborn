using Netstorm.Client.Audio;

// 최초 장치 준비와 실제 옵션 적용을 구분하고 같은 음악/효과음/예약 음량 소유자를 유지한다.
namespace Netstorm.Client.Host;

/// <summary>CPP ClientAudio의 초기화·옵션 재해석·재개용 닫기·최종 반납을 같은 장기 owner로 연결한다.</summary>
public sealed partial class HostMissionAudio
{
    /// <summary>마지막으로 적용 요청한 원문 값 사본이다. 장치 실패가 바꾼 Enabled를 이 값으로 다시 덮지 않는다.</summary>
    private ClientAudioOptions _appliedAudioOptions = new();
    /// <summary>실제 Runtime.Initialize의 정상 복귀 이후부터 종료까지 유지하는 수명 표식이다.</summary>
    private bool _musicRuntimeRunning;
    /// <summary>최종 반납 뒤 새 장치/파일 요청을 거부하며 옵션 CloseDevice와 구별한다.</summary>
    private bool _audioDisposed;

    /// <summary>같은 실제 장치 준비 상태다. mute 인자나 sound 옵션으로 대신 판정하지 않는다.</summary>
    public bool DeviceReady => _state.Initialized;
    /// <summary>현재 실제 효과음 옵션이다. 장치 실패가 끈 값을 그대로 읽는다.</summary>
    public bool SoundEnabled => _state.Enabled;
    /// <summary>마지막 요청한 불변 옵션 값이다. 조회 자체로 현재 설정이나 장치를 적용하지 않는다.</summary>
    public ClientAudioOptions AppliedAudioOptions => _appliedAudioOptions;
    /// <summary>현재 같은 Config에서 기본값/명시 값만 읽고 장치·곡·Scene 상태를 변경하지 않는다.</summary>
    public ClientAudioOptions ReadOptions() { EnsureAudioAlive(); return ClientAudioOptions.Read(_audioStartup.Configuration); }
    /// <summary>폐기한 장기 owner를 옵션 재개로 오인하지 않게 한다.</summary>
    private void EnsureAudioAlive() => ObjectDisposedException.ThrowIf(_audioDisposed, this);

    /// <summary>기존 window 생성자가 호출할 실제 최초 준비 몸체다. HWND 준비 뒤 초기화 반환을 기록하고 mute를 별도로 적용한다.</summary>
    private void InitializeStartup(nint window, bool muted)
    {
        try
        {
            bool initialized = Initialize(window, ReadOptions());
            LogMusic(9, "장기 음향 최초 Initialize 실제 반환: " + initialized);
            if (muted) PushMute();
        }
        catch (Exception error)
        {
            LogMusic(9, "장기 음향 최초 초기화 예외: " + error.GetType().FullName);
            PreserveConstructionFailure(DisposeAudioOwners, "장기 음향");
            throw;
        }
    }

    /// <summary>최초 전역/두 음량을 먼저 적용한 뒤 sound와 무관하게 같은 실제 장치·worker를 연다.</summary>
    public bool Initialize(nint window, ClientAudioOptions options)
    {
        EnsureAudioAlive(); ArgumentNullException.ThrowIfNull(options);
        _appliedAudioOptions = options;
        ClientAudioOptionPolicy.ApplyInitialValues(options, OptionHooks(window));
        return OpenDevice(window, options.Quality, true);
    }

    /// <summary>실제 옵션 행/설정 callback의 음향 적용이다. State 전체 InterpretOptions의 대체가 아니다.</summary>
    public void Interpret(nint window, ClientAudioOptions options)
    {
        EnsureAudioAlive(); ArgumentNullException.ThrowIfNull(options);
        _appliedAudioOptions = options;
        ClientAudioOptionPolicy.Reinterpret(options, OptionHooks(window));
    }

    /// <summary>현재 장기 실제 함수만 결합한다. 순수 정책은 선택된 위치에서만 이 함수를 사용한다.</summary>
    private ClientAudioInterpretHooks OptionHooks(nint window) => new()
    {
        Globals = ApplyOptionGlobals,
        Open = quality => { _ = OpenDevice(window, quality, false); },
        Close = CloseDeviceCore,
        EffectVolume = _player.SetMasterVolume,
        MusicVolume = _musicControl.SetVolume,
        SelectMusic = SelectCurrentMusic,
    };

    /// <summary>장치 변경보다 먼저 원문 옵션 전역만 쓰며 공통 보류 깊이와 예약 음량은 초기화하지 않는다.</summary>
    private void ApplyOptionGlobals(ClientAudioOptions options)
    {
        _state.Enabled = options.Sound; _state.SwapSpeakers = options.SwapSpeakers; _state.MaxPlaying = options.MaxSimulSounds;
        _musicSelected.Enabled = options.Music ? 1 : 0;
    }

    /// <summary>현재 이름이 비어 있지 않을 때만 같은 선택기에 현재 음악 옵션을 전달한다. Scene 난수/기한을 새로 시작하지 않는다.</summary>
    private void SelectCurrentMusic(bool enabled)
    {
        string current = _musicSelected.Current();
        if (current.Length != 0) _musicSelection.Select(current, enabled ? 1 : 0);
    }

    /// <summary>실제 장치→최초 천둥 preload→Runtime 순서다. 재해석 재열기는 preload를 반복하지 않는다.</summary>
    private bool OpenDevice(nint window, int quality, bool preload)
    {
        // 준비 생성자와 같은 Windows 장치 경계를 명시해 실제 native 호출 범위를 보존한다.
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 장기 음향 장치 열기는 Windows 전용이다");
        bool initialized = _device.Initialize(window, quality);
        // 1.209의 실제 장치 반환/주소 관측은 유지하고 기록 실패가 장치·worker 수명을 중단하지 않게 한다.
        if (initialized) LogMusic(9, $"미션 소리 장치 초기화 완료: HWND=0x{window.ToInt64():X}");
        else if (!_state.Initialized) LogMusic(9, "미션 소리 장치 초기화 실패: " + _device.LastInitializationFailure);
        // 이미 열린 장치의 false와 현재 준비 상태를 구별하며 상위 Initialize 반환을 대신 보고하지 않는다.
        LogMusic(9, $"장기 음향 Device.Initialize 실제 반환: {initialized}, HWND=0x{window.ToInt64():X}, 현재 Initialized={_state.Initialized}");
        if (!initialized) return false;
        if (preload) _player.Preload("thunderCrack.wav");
        try { _musicRuntime.Initialize(); }
        catch (Exception error)
        {
            _device.Shutdown(); _state.Enabled = false;
            LogMusic(9, "장기 음악 Runtime 초기화 실패: " + error.GetType().FullName);
            return false;
        }
        // 원문 정상 복귀도 핸들 생성을 보장하지 않으므로 같은 실제 저장소의 현재값을 별도로 관측한다.
        LogMusic(9, $"장기 음악 Runtime.Initialize 정상 복귀: Thread=0x{_musicRuntimeState.Thread.ToInt64():X}, Shutdown=0x{_musicRuntimeState.Shutdown.ToInt64():X}, MusicInitialized={_state.MusicInitialized}");
        _musicRuntimeRunning = true;
        return true;
    }

    /// <summary>재개용 닫기다. 같은 현재 곡 이름·Scene·파일 저장소·음소거 깊이/예약값은 유지한다.</summary>
    public void CloseDevice() { EnsureAudioAlive(); CloseDeviceCore(); }

    /// <summary>Stop의 예외도 Runtime이 합류/반납한 뒤 장치를 닫는다. 실제 파일 저장소는 최종 Dispose까지 유지한다.</summary>
    private void CloseDeviceCore()
    {
        // 생성 가능한 소유자와 같은 플랫폼만 허용하며 다른 OS의 장치를 대신 닫지 않는다.
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 장기 음향 장치 반납은 Windows 전용이다");
        try
        {
            if (_musicRuntimeRunning)
            {
                try { _musicRuntime.Shutdown(); }
                finally
                {
                    _musicRuntimeRunning = false;
                    // Stop 예외도 실제 합류/반납 뒤의 핸들·준비값을 같은 nonthrow 기록으로 남긴다.
                    LogMusic(9, $"장기 음악 Runtime.Shutdown 반납 관측: Thread=0x{_musicRuntimeState.Thread.ToInt64():X}, Shutdown=0x{_musicRuntimeState.Shutdown.ToInt64():X}, MusicInitialized={_state.MusicInitialized}");
                }
            }
        }
        finally { _device.Shutdown(); }
    }

    /// <summary>기존 Dispose 교체 블록이 호출할 최종 반납이다. 같은 Runtime 합류→장치 종료 뒤에만 파일 저장소를 폐기한다.</summary>
    private void DisposeAudioOwners()
    {
        if (_audioDisposed) return;
        _audioDisposed = true;
        try { CloseDeviceCore(); }
        finally { if (OperatingSystem.IsWindows()) _musicFiles.Dispose(); }
    }
}
