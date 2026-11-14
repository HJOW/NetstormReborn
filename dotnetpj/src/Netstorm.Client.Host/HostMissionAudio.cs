using Netstorm.Client.Audio;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 미션 소리의 장기 장치/이름 표와 같은 Host의 실제 Sound47을 연결한다.
namespace Netstorm.Client.Host;

/// <summary>Windows 실제 소리 장치를 미션보다 오래 소유한다. 음소거는 원문 재생 관문으로 처리한다.</summary>
public sealed class HostMissionAudio : IDisposable
{
    /// <summary>클라이언트 수명 동안 이름/복제 항목을 보존하는 실제 표다.</summary>
    private readonly SoundList _list = new(OriginalEdition.Patch1078);
    /// <summary>실제 장치 준비·소리 옵션·음량을 같은 재생기가 읽는다.</summary>
    private readonly SoundState _state = new();
    /// <summary>실제 파일·DirectSound 경계다. 음소거 시 초기화하지 않는다.</summary>
    private readonly WindowsSoundDevice _device;
    /// <summary>같은 이름/전역/실제 장치를 사용하는 재생 몸체다.</summary>
    private readonly SoundPlayer _player;
    /// <summary>소리 프로세스의 원문 벽시계 공급이다.</summary>
    private readonly Func<double> _wallSeconds;
    /// <summary>미션마다 새 프로세스 상태를 빌리되 마지막 연결만 현재 프레임을 갱신한다.</summary>
    private SoundProcessState? _processState;

    /// <summary>실제 파일 경계를 조립하고 음소거가 아니며 옵션이 켜진 Windows에서만 장치를 연다.</summary>
    public HostMissionAudio(HostStartupScene startup, nint window, bool muted, Func<double> wallSeconds)
    {
        ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(wallSeconds);
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 미션 소리 장치는 Windows 전용이다");
        _wallSeconds = wallSeconds; _list.Initialize();
        _state.Enabled = !muted && startup.Configuration.GetInt("sound") != 0;
        _state.MasterVolume = startup.Configuration.GetInt("soundVolume") switch
        { 1 => -4000, 2 => -2000, 3 => -1000, 4 => -500, _ => 0 };
        _device = new WindowsSoundDevice(_list, _state,
            WindowsSoundFiles.MakeDiskResolver(startup.Files.BaseDirectory, startup.Files.SecondaryDirectory ?? ""),
            startup.Configuration.Configuration.Get("currentLanguage") ?? "english");
        if (_state.Enabled && OperatingSystem.IsWindows() && !_device.Initialize(window))
            ErrorLog.Note("미션 소리 장치 초기화 실패: " + _device.LastInitializationFailure);
        _player = new SoundPlayer(_list, _state, _device.Hooks());
    }

    /// <summary>실제 소리 프로세스를 같은 미션 Host에 붙이고 보호막/낙하에 같은 이름·재생기를 제공한다.</summary>
    public RawPriestWorldEffects BindPriest(HostSession session)
    {
        ArgumentNullException.ThrowIfNull(session); _ = session.BuildPlayerWorldHooks();
        _processState = new SoundProcessState
        {
            SharedProcessType = session.GameAux.SoundProcessTypeBox,
            Debug = new RawCheckingFlag(() => session.AiChecking.Read()),
        };
        var system = new SoundProcessSystem(session.PlayerProcesses, _processState,
            SoundProcessSystem.MakeSoundProcessHooks(_player, _wallSeconds));
        var shield = new PriestShieldHooks
        {
            // cpppj 조립과 같이 실제 Add에서 관리 객체를 할당하며 선행 예약은 성공 관문이다.
            ReserveSound = static () => true, LoadSound = _list.Lookup,
            AttachSound = (parent, sound, alternate, flags) =>
            {
                if (system.Add(parent, sound, alternate, flags) is null)
                    throw new InvalidOperationException("실제 보호막 Sound47 부착 실패");
            },
        };
        shield = SoundProcessSystem.MakePriestShieldNoticeHooks(_player, shield);
        return new RawPriestWorldEffects { Shield = shield, FallSound = SoundProcessSystem.MakePriestFallSound(_player) };
    }

    /// <summary>같은 Client 프레임 카운터를 연결된 소리 상태에 반영한다.</summary>
    public void SetFrame(uint frame) { if (_processState is not null) _processState.Frame = frame; }

    /// <summary>모든 미션 프로세스 반납 뒤 실제 장치·버퍼 소유권을 반납한다.</summary>
    public void Dispose() { if (OperatingSystem.IsWindows()) _device.Dispose(); }
}
