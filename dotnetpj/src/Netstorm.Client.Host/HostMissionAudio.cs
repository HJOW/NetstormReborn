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
    /// <summary>현재 미션의 보호막과 건설이 공유하는 실제 소리 프로세스 생성기다.</summary>
    private SoundProcessSystem? _soundProcesses;
    /// <summary>다른 세션의 소리 프로세스를 빌리지 않도록 현재 실제 미션을 식별한다.</summary>
    private HostSession? _soundSession;

    /// <summary>장치보다오래사는실제장면음악상태다. State의songEnd쓰기와동일8BYTE를사용한다.</summary>
    public SceneMusicState Scene { get; }

    /// <summary>실제 파일 경계를 조립하고 음소거가 아니며 옵션이 켜진 Windows에서만 장치를 연다.</summary>
    public HostMissionAudio(HostStartupScene startup, nint window, bool muted, Func<double> wallSeconds, SceneMusicState? scene = null)
    {
        ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(wallSeconds);
        Scene = scene ?? new SceneMusicState();
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
        _soundProcesses = system; _soundSession = session;
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

    /// <summary>실제 연소가 소유한 비영 소리만 같은 원문 재생기로 정지한다.</summary>
    public void StopEffect(uint sound) { if (sound != 0) _player.Stop(sound); }

    /// <summary>저HP 벌프의 실제 파일명/현재좌표를 같은 이름표·위치 재생기로 보낸다.</summary>
    public void BulfHurt(int sid, float x, float y) => _player.PlayOnceAt(x, y, _list.Lookup("bulfHurt.wav"), 0, 0);

    /// <summary>플레어/건설의현재위치·실제이름·flags를같은원본재생기로전달한다. 음소거도원문관문을사용한다.</summary>
    public void PlayEffectAt(float x, float y, string name, uint flags) => _player.PlayOnceAt(x, y, _list.Lookup(name), flags, 0);

    /// <summary>같은 실제47번 프로세스·이름표·위치/전역 재생기를 건설 진행에 빌린다.</summary>
    public ConstructionProcessHooks ConstructionHooks(HostSession session, IReadOnlyList<RiftTypeDefinition> definitions)
    {
        ArgumentNullException.ThrowIfNull(session); ArgumentNullException.ThrowIfNull(definitions);
        if (!ReferenceEquals(session, _soundSession) || _soundProcesses is null)
            throw new InvalidOperationException("건설 소리는 같은 미션의 소리 프로세스 연결이 필요합니다");
        SoundProcessSystem sounds = _soundProcesses;
        return new()
        {
            LoadSound = _list.Lookup, ReserveSound = static () => true,
            AttachSound = (parent, sound, alternate, flags) =>
            {
                if (sounds.Add(parent, sound, alternate, flags) is null) throw new InvalidOperationException("건설 실제 진행음 부착 실패");
            },
            PlayAt = PlayEffectAt,
            ReadySound = type =>
            {
                if (type < 70 || type - 70 >= definitions.Count) throw new ArgumentOutOfRangeException(nameof(type), "건설 준비 소리 TYPE 오류");
                string? name = definitions[(int)type - 70].String("buildDoneSound");
                return string.IsNullOrEmpty(name) ? 0 : _list.Lookup(name);
            },
            Play = sound => _player.Play(sound, 0, 0, 0, 0, 0),
        };
    }

    /// <summary>같은 이름표/실제재생기로 업그레이드 위치음과priority1 전역완료음을 연결한다.</summary>
    public FactoryUpgradeHooks FactoryUpgradeHooks(Action<uint, string> log)
    {
        ArgumentNullException.ThrowIfNull(log);
        return new()
        {
            SoundAt = (x, y, name) => PlayEffectAt(x, y, name, 0),
            Sound = name => _player.Play(_list.Lookup(name), 0, 0, 0, 1, 0), Log = log,
        };
    }

    /// <summary>모든 미션 프로세스 반납 뒤 실제 장치·버퍼 소유권을 반납한다.</summary>
    public void Dispose() { if (OperatingSystem.IsWindows()) _device.Dispose(); }
}
