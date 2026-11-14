using System.Buffers.Binary;
using Netstorm.Core.Original;

// 원문 부모 부착 소리의 현재 타입/프레임·벽시계·재생·삭제 수명을 보존한다.
namespace Netstorm.Client.Audio;

/// <summary>소리 프로세스가 읽고 쓰는 같은 Client 장기 전역이다.</summary>
public sealed class SoundProcessState
{
    /// <summary>현재 프레임 카운터의 전체DWORD다.</summary>
    public uint Frame { get; set; }
    /// <summary>독립 원본 입력의 소리 프로세스 TYPE다.</summary>
    public uint ProcessType { get; set; } = 47;
    /// <summary>실제 GameAux의 같은 현재 역할DWORD를 빌린다.</summary>
    public UIntBox? SharedProcessType { get; set; }
    /// <summary>부착 시점의 현재 전체 역할DWORD다.</summary>
    public uint CurrentProcessType() => SharedProcessType?.Value ?? ProcessType;
    /// <summary>PATCH 진단이 두 보고 사이에서도 다시 읽는 현재 checking 뷰다.</summary>
    public RawCheckingFlag Debug { get; set; } = new();
    /// <summary>다른 소리 요청과 공유하는 타입별 최근 재생 프레임DWORD다.</summary>
    public uint[] TypeFrames { get; } = new uint[256];
}

/// <summary>소리 프로세스의 벽시계·위치/전역 재생·정지/상태·선택 보고 효과다.</summary>
public sealed class SoundProcessHooks
{
    /// <summary>게임 정지와 무관하게 흐르는 현재 double 벽시계다.</summary>
    public Func<double>? WallSeconds { get; set; }
    /// <summary>현재 위치/재생중 항목/소리를 반복 재생기에 전달한다.</summary>
    public Func<float, float, uint, uint, uint>? PlayLoopAt { get; set; }
    /// <summary>현재 위치/소리/반복/우선의 한 번 재생을 요청한다.</summary>
    public Func<float, float, uint, uint, uint, uint>? PlayOnceAt { get; set; }
    /// <summary>소리/반복/음량/좌우/우선/한도를 원문DWORD로 전달한다.</summary>
    public Func<uint, uint, uint, uint, uint, uint, uint>? Play { get; set; }
    /// <summary>현재 재생 중인 실제 항목을 정지한다.</summary>
    public Action<uint>? Stop { get; set; }
    /// <summary>정지 뒤 같은 항목의 실제 재생 상태를 읽는다.</summary>
    public Func<uint, uint>? IsPlaying { get; set; }
    /// <summary>선택적 원문 식/줄 보고이며 정상 복귀하면 계속한다.</summary>
    public Action<string, int>? Report { get; set; }
    /// <summary>함수 객체는 값 복사하고 캡처 장기 장치는 같은 수명으로 빌린다.</summary>
    internal SoundProcessHooks Copy() => new()
    {
        WallSeconds = WallSeconds, PlayLoopAt = PlayLoopAt, PlayOnceAt = PlayOnceAt, Play = Play,
        Stop = Stop, IsPlaying = IsPlaying, Report = Report,
    };
}

/// <summary>현재 부모에 붙어 원문 재생/정지 동작을 수행하는 실제 Kernel 프로세스다.</summary>
public sealed class SoundProcess : SquidProcess
{
    /// <summary>부모 좌표를 쓰는 원문 플래그다.</summary>
    public const uint PlayAt = 1;
    /// <summary>부모 상태WORD가 비영이면 대체 소리를 선택하는 플래그다.</summary>
    public const uint UseAlternate = 2;
    /// <summary>한 번 재생 요청 뒤 실제 Form을 지우는 플래그다.</summary>
    public const uint PlayOnce = 4;
    /// <summary>위치 한 번 재생에서는 반복 인자로 전달하는 원문8이다.</summary>
    public const uint PlayPriority = 8;
    /// <summary>같은 풀/장기 전역/장치가 소유하는 실행 시스템이다.</summary>
    private readonly SoundProcessSystem _system;
    /// <summary>원본+0xc의 현재 재생 항목이다.</summary>
    private uint _current;
    /// <summary>원본 기본 소리 항목이다.</summary>
    private readonly uint _sound;
    /// <summary>원본 대체 소리 항목이다.</summary>
    private readonly uint _alternate;
    /// <summary>생성 시 저장한 원본 전체DWORD 플래그다.</summary>
    private readonly uint _flags;
    /// <summary>원본+0x20의 최초0.0 double 시작 시각이다.</summary>
    private double _startTime;
    /// <summary>같은 장기 시스템을 빌리고 원문 필드 초기값을 보존한다.</summary>
    public SoundProcess(SoundProcessSystem system, uint sound, uint alternate, uint flags)
    {
        ArgumentNullException.ThrowIfNull(system); _system = system; _sound = sound; _alternate = alternate; _flags = flags;
    }
    /// <summary>재생 효과 이후 현재 부모 TYPE와 현재 프레임을 다시 읽어 기록한다.</summary>
    private void MarkTypeFrame() => _system.State.TypeFrames[_system.Host.Pool().Slot(Parent())[10]] = _system.State.Frame;
    /// <summary>정상 복귀 가능한 원문 보고를 현재 장치에 전달한다.</summary>
    private void Report(string expression, int line) => _system.Hooks.Report?.Invoke(expression, line);
    /// <summary>소리선택→벽시계→저장한타입프레임→현재진단→현재좌표→재생→선택삭제 순서다.</summary>
    public override void RunFrame()
    {
        SquidProcessHost? host = Host(); if (host is null) return;
        SoundProcessState state = _system.State; SoundProcessHooks hooks = _system.Hooks; bool patch = host.Pool().Edition == OriginalEdition.Patch1078;
        ReadOnlySpan<byte> raw = host.Pool().Slot(Parent()); uint sound = _sound;
        if ((_flags & UseAlternate) != 0 && (raw[12] | raw[13]) != 0) sound = _alternate;
        double now = hooks.WallSeconds!(); if (patch ? !(now >= _startTime) : _startTime > now) return;
        bool at = (_flags & PlayAt) != 0, once = (_flags & PlayOnce) != 0; uint priority = _flags & PlayPriority;
        bool played = state.TypeFrames[raw[10]] == state.Frame;
        if (patch && state.Debug.Read() != 0)
        {
            if (!once && priority != 0) Report("playOnce || !playPriority", 0x35);
            if (state.Debug.Read() != 0 && !once && !at) Report("playOnce || playAt", 0x36);
        }
        if (at)
        {
            float x = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), y = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
            if (!once)
            {
                if (_current != 0 || !played) _current = hooks.PlayLoopAt!(x, y, _current, sound);
                if (_current != 0) MarkTypeFrame(); return;
            }
            if (!played) { _ = hooks.PlayOnceAt!(x, y, sound, priority, 0); MarkTypeFrame(); }
        }
        else if (_current == 0 && !played)
        {
            _current = hooks.Play!(sound, once ? 0u : 1u, 0, 0, priority, 0); MarkTypeFrame();
        }
        // 실제 Kill이 같은 Form/프로세스를 지울 수 있으므로 이후 멤버를 읽거나 쓰지 않는다.
        if (once) host.Kill(this, 0);
    }
    /// <summary>한 번 재생은 끝까지 두고 반복 현재 항목만 정지/상태 진단한다.</summary>
    public override void OnFormDestroy(uint flags)
    {
        if (_current == 0 || (_flags & PlayOnce) != 0) return;
        SoundProcessHooks hooks = _system.Hooks; hooks.Stop!(_current);
        if (hooks.IsPlaying!(_current) != 0) Report("!isSoundPlaying( currentSound )", 0x15);
    }
    /// <summary>원본 현재 재생 항목을 읽는다.</summary>
    public uint Current() => _current;
    /// <summary>독립 재생/수신 입력의 현재 재생 항목을 쓴다.</summary>
    public void SetCurrent(uint current) => _current = current;
    /// <summary>원본 기본 소리 항목을 읽는다.</summary>
    public uint Sound() => _sound;
    /// <summary>원본 대체 소리 항목을 읽는다.</summary>
    public uint Alternate() => _alternate;
    /// <summary>저장한 원문 플래그DWORD를 읽는다.</summary>
    public uint Flags() => _flags;
    /// <summary>현재 double 시작 시각을 읽는다.</summary>
    public double StartTime() => _startTime;
    /// <summary>독립 재생 입력의 double 시작 시각을 쓴다.</summary>
    public void SetStartTime(double time) => _startTime = time;
    /// <summary>float 지연을 현재 double 벽시각에 넓게 더해 보관한다.</summary>
    public void Delay(float seconds) => _startTime = (double)seconds + _system.Hooks.WallSeconds!();
}

/// <summary>현재 소리 역할로 실제 부모 Form/Kernel에 소리 프로세스를 붙이는 장기 시스템이다.</summary>
public sealed class SoundProcessSystem
{
    /// <summary>모든 소리 프로세스가 공유하는 실제 같은 Host다.</summary>
    internal SquidProcessHost Host { get; }
    /// <summary>같은 Client의 현재 타입/프레임/진단을 빌린다.</summary>
    internal SoundProcessState State { get; }
    /// <summary>생성 때 값 복사한 실제 재생/정지 효과다.</summary>
    internal SoundProcessHooks Hooks { get; }
    /// <summary>report를 제외한 필수 여섯 장치를 구성 시 확인한다.</summary>
    public SoundProcessSystem(SquidProcessHost host, SoundProcessState state, SoundProcessHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(host); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.WallSeconds is null || hooks.PlayLoopAt is null || hooks.PlayOnceAt is null || hooks.Play is null || hooks.Stop is null || hooks.IsPlaying is null)
            throw new ArgumentException("소리 프로세스 필수 효과 누락", nameof(hooks));
        Host = host; State = state; Hooks = hooks.Copy();
    }
    /// <summary>현재 전체 역할DWORD와 client Form 플래그0x10으로 실제 Host에 부착한다.</summary>
    public SoundProcess? Add(int parent, uint sound, uint alternate, uint flags) => (SoundProcess?)Host.Attach(new SoundProcess(this, sound, alternate, flags), State.CurrentProcessType(), parent, 0x10);
    /// <summary>다른 월드와 혼용되지 않도록 실제 풀을 반환한다.</summary>
    public RawSidPool Pool() => Host.Pool();
    /// <summary>보호막의 소리 이름/부착만 같은 이름 표/프로세스로 교체하고 나머지 효과는 보존한다.</summary>
    public static PriestShieldHooks MakePriestShieldSoundHooks(RawSidPool pool, SoundList sounds, SoundProcessSystem system, PriestShieldHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(sounds); ArgumentNullException.ThrowIfNull(system); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, system.Pool())) throw new ArgumentException("보호막 소리 프로세스 풀 불일치");
        var result = new PriestShieldHooks
        {
            Create = hooks.Create, SetOwner = hooks.SetOwner, Pop = hooks.Pop, ReserveSound = hooks.ReserveSound,
            LoadSound = sounds.Lookup, AttachSound = (sid, sound, alternate, flags) => system.Add(sid, sound, alternate, flags),
            NoticeSound = hooks.NoticeSound, Translate = hooks.Translate, Tell = hooks.Tell,
        };
        return result;
    }
    /// <summary>같은 실제 재생 계층의 다섯 함수와 현재 벽시계를 원문 인자 순서로 연결한다.</summary>
    public static SoundProcessHooks MakeSoundProcessHooks(SoundPlayer player, Func<double> wallSeconds, Action<string, int>? report = null)
    {
        ArgumentNullException.ThrowIfNull(player); ArgumentNullException.ThrowIfNull(wallSeconds);
        return new SoundProcessHooks
        {
            WallSeconds = wallSeconds, PlayLoopAt = player.PlayLoopAt, PlayOnceAt = player.PlayOnceAt,
            Play = (sound, loop, volume, pan, priority, limit) => player.Play(sound, loop, unchecked((int)volume), unchecked((int)pan), priority, unchecked((int)limit)),
            Stop = player.Stop, IsPlaying = player.IsPlaying, Report = report,
        };
    }
    /// <summary>로컬 보호막 안내의 원문5인자를 같은 이름 기반 전역 재생에 전달한다.</summary>
    public static PriestShieldHooks MakePriestShieldNoticeHooks(SoundPlayer player, PriestShieldHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(player); ArgumentNullException.ThrowIfNull(hooks);
        return new PriestShieldHooks
        {
            Create = hooks.Create, SetOwner = hooks.SetOwner, Pop = hooks.Pop, ReserveSound = hooks.ReserveSound, LoadSound = hooks.LoadSound, AttachSound = hooks.AttachSound,
            NoticeSound = notice => player.PlayByName(notice.File, notice.Arguments[0], unchecked((int)notice.Arguments[1]), unchecked((int)notice.Arguments[2]), notice.Arguments[3], unchecked((int)notice.Arguments[4])),
            Translate = hooks.Translate, Tell = hooks.Tell,
        };
    }
    /// <summary>사제 SID는 소리에 쓰지 않고 최종 현재 좌표만 priestFall.wav 위치 재생에 전달한다.</summary>
    public static Action<int, float, float> MakePriestFallSound(SoundPlayer player)
    {
        ArgumentNullException.ThrowIfNull(player); return (_, x, y) => player.PlayNameAt(x, y, "priestFall.wav", 0);
    }
}
