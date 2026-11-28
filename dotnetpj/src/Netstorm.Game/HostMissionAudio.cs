using Netstorm.Client.Audio;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 미션 소리의 장기 장치/이름 표와 같은 Host의 실제 Sound47을 연결한다.
namespace Netstorm.Client.Host;

/// <summary>Windows 실제 소리 장치를 미션보다 오래 소유한다. 음소거는 원문 재생 관문으로 처리한다.</summary>
public sealed partial class HostMissionAudio : IDisposable
{
    /// <summary>클라이언트 수명 동안 이름/복제 항목을 보존하는 실제 표다.</summary>
    private readonly SoundList _list = new(OriginalEdition.Patch1078);
    /// <summary>실제 장치 준비·소리 옵션·음량을 같은 재생기가 읽는다.</summary>
    private readonly SoundState _state = new();
    /// <summary>실제 파일·DirectSound 경계다. 최초에는 sound와 무관하게 열고 음소거는 같은 공통 음량 보류로 적용한다.</summary>
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

    /// <summary>생존하는 실제 HWND로 최초 장치·worker를 열고 별도 음소거는 같은 공통 음량 보류로 적용한다.</summary>
    public HostMissionAudio(HostStartupScene startup, nint window, bool muted, Func<double> wallSeconds, SceneMusicState? scene = null)
        : this(startup, wallSeconds, scene)
    {
        InitializeStartup(window, muted);
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

    /// <summary>같은 현재 미션의 실제 프로세스 처리기 반납 뒤에만 음향의 세션 참조를 끊는다. 다른 미션이나 늦은 이전 반납은 현재 연결을 보존하고 false다.</summary>
    public bool UnbindMissionForState(HostSession expected)
    {
        ArgumentNullException.ThrowIfNull(expected);
        if (!ReferenceEquals(expected, _soundSession)) return false;
        expected.EnsureStateRuntimeIdle();
        if (!expected.ProcessBindingsReturned()) throw new InvalidOperationException("음향 미션 반납 전 실제 단일 Kernel Clear 처리기 반납 필요");
        _soundSession = null;
        _soundProcesses = null;
        _processState = null;
        return true;
    }

    /// <summary>같은 Client 프레임 카운터를 연결된 소리 상태에 반영한다.</summary>
    public void SetFrame(uint frame) { if (_processState is not null) _processState.Frame = frame; }

    /// <summary>실제 연소가 소유한 비영 소리만 같은 원문 재생기로 정지한다.</summary>
    public void StopEffect(uint sound) { if (sound != 0) _player.Stop(sound); }

    /// <summary>장기 이름표와 같은 실제 재생기에서 반복 중인 버퍼만 정지한다. 음향 장치와 미션 연결 수명은 유지한다.</summary>
    public void StopLoops() => _player.StopLoops();

    /// <summary>저HP 벌프의 실제 파일명/현재좌표를 같은 이름표·위치 재생기로 보낸다.</summary>
    public void BulfHurt(int sid, float x, float y) => _player.PlayOnceAt(x, y, _list.Lookup("bulfHurt.wav"), 0, 0);

    /// <summary>플레어/건설의현재위치·실제이름·flags를같은원본재생기로전달한다. 음소거도원문관문을사용한다.</summary>
    public void PlayEffectAt(float x, float y, string name, uint flags) => _player.PlayOnceAt(x, y, _list.Lookup(name), flags, 0);

    /// <summary>Chat 등 위치 없는 전역 요청을 같은 장기 이름표/원문 재생기에 우선순위0으로 전달한다.</summary>
    public void PlayGlobal(string name) => _player.Play(_list.Lookup(name), 0, 0, 0, 0, 0);

    /// <summary>주변음의 이름·반복·음량·좌우·우선순위·한도를 같은 장기 재생기에 전달한다. 음소거·장치 없음이면 0이다.</summary>
    /// <param name="name">재생할 소리 이름이다.</param>
    /// <param name="loop">반복 재생이면 0이 아니다.</param>
    /// <param name="volume">음량이다.</param>
    /// <param name="pan">좌우다.</param>
    /// <param name="priority">우선순위다.</param>
    /// <param name="limit">동시 한도다.</param>
    /// <returns>재생 항목이며 재생하지 않으면 0이다.</returns>
    /// <remarks>2026-11-26 1.056에서 SL-3c 주변음용으로 추가했다. 근거: cpppj 주변음 004180f0의 Sound 호출이다. 메뉴에서는 미션 음향이 없어 조용하며 R6에서 메뉴 수명으로 잇는다.</remarks>
    public uint PlayAmbient(string name, uint loop, int volume, int pan, uint priority, int limit) => _player.Play(_list.Lookup(name), loop, volume, pan, priority, limit);

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

    /// <summary>모든 미션 프로세스 반납 뒤 실제 worker 완전 합류→장치 종료→파일 저장소 폐기 순서로 반납한다.</summary>
    public void Dispose() => DisposeAudioOwners();
}
