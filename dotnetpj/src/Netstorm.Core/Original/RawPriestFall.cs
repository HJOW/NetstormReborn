using System.Buffers.Binary;

// 사제 낙하 요청·현재 프레임/지면·공유 Regular와 보호막 수명을 원문 순서로 실행한다.
namespace Netstorm.Core.Original;

/// <summary>타입별 낙하 시작·도착 프레임이며 현재 물리 칸에서 읽는다.</summary>
/// <param name="Falling">TYPE+0x154의 낙하 시작 프레임이다.</param>
/// <param name="Landed">TYPE+0x140의 지면 도착 프레임이다.</param>
public readonly record struct PriestFallFrames(int Falling, int Landed);

/// <summary>보호막·프로세스 효과 뒤 낙하 공간 전환이 읽는 현재 권한이다.</summary>
public sealed class PriestFallState
{
    /// <summary>독립 원본 재생의 최초 권한값이다.</summary>
    public bool Authority { get; set; } = true;
    /// <summary>실제 Client boss의 같은 장기 값을 빌리며 사본으로 고정하지 않는다.</summary>
    public Func<bool>? SharedAuthority { get; set; }
    /// <summary>실제 사용 시점의 현재 권한을 읽는다.</summary>
    public bool GetAuthority() => SharedAuthority?.Invoke() ?? Authority;
}

/// <summary>같은 풀의 실제 보호막·공유 생성·공간·소리·프레임 효과 경계다.</summary>
public sealed class PriestFallHooks
{
    /// <summary>현재 사제에게 필요한 보호막을 확보한다.</summary>
    public Action<int>? EnsureShield { get; set; }
    /// <summary>new(0x28)의 현재 확보 결과이며 실패해도 다음 공간/소리 효과를 계속한다.</summary>
    public Func<bool>? ReserveRegular { get; set; }
    /// <summary>같은 공유 Kernel에 사건/payload를 등록한다.</summary>
    public Action<int, uint, float>? AddSharedRegular { get; set; }
    /// <summary>권한이 있으면 현재 HP/위치로 실제 공간 등록을 해제한다.</summary>
    public Action<int, uint>? Unpop { get; set; }
    /// <summary>실제 공간에 pf0x800으로 다시 등록한다.</summary>
    public Action<int, uint>? Repop { get; set; }
    /// <summary>최종 현재 좌표에서 priestFall.wav를 flags0으로 요청한다.</summary>
    public Action<int, float, float>? Sound { get; set; }
    /// <summary>낙하 시작/도착의 현재 프레임을 같은 지정기로 설정한다.</summary>
    public Action<int, int, uint>? SetFrame { get; set; }
    /// <summary>낙하 방향 J의 실제 프레임을 DWORD 증분으로 진행한다.</summary>
    public Action<int, uint, uint>? AdvanceFrame { get; set; }
    /// <summary>도착 후 이동 가능하면 현재 보호막을 조회·삭제한다.</summary>
    public Action<int>? ClearShield { get; set; }
    /// <summary>원문 함수 객체처럼 값 복사하며 캡처 장기 owner는 공유한다.</summary>
    internal PriestFallHooks Copy() => new()
    {
        EnsureShield = EnsureShield, ReserveRegular = ReserveRegular, AddSharedRegular = AddSharedRegular,
        Unpop = Unpop, Repop = Repop, Sound = Sound, SetFrame = SetFrame, AdvanceFrame = AdvanceFrame, ClearShield = ClearShield,
    };
}

/// <summary>원본004941f0·00494e80·0x25b의 사제 낙하 요청/주기 본체다.</summary>
public sealed class RawPriestFall
{
    /// <summary>사제 낙하가 선택하는 실제 공유 Regular 사건이다.</summary>
    public const uint FallEvent = 0x25b;
    /// <summary>최초 공유 Regular의 정확한 단정도 payload다.</summary>
    public const float InitialPayload = 0.01f;
    /// <summary>지면이 없을 때 반환하는 정확한 단정도 반복 주기다.</summary>
    public const float RepeatPayload = 0.1f;
    /// <summary>지면 위치를 거의 올리는 원본 단정도 배율이다.</summary>
    private const float CoordinateBias = 0.9999f;
    /// <summary>모든 raw와 다른 모듈이 공유하는 실제 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 HP/TYPE/지면/강제 비트를 읽는 실제 상태 조회기다.</summary>
    private readonly RawPriestState _priest;
    /// <summary>매 호출 현재 TYPE 칸의 방향 코드를 읽는 같은 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>시작/도착 효과 직전 현재 TYPE로 조회하는 프레임 선택표다.</summary>
    private readonly IReadOnlyList<PriestFallFrames> _choices;
    /// <summary>현재 지면 SID WORD65536개이며 유효 SID 여부를 별도로 검사하지 않는다.</summary>
    private readonly ushort[] _surfaces;
    /// <summary>원문 효과 뒤 다시 읽는 현재 권한 owner다.</summary>
    private readonly PriestFallState _state;
    /// <summary>구성 시 전체 경계를 확인하고 값 복사한 효과다.</summary>
    private readonly PriestFallHooks _hooks;

    /// <summary>같은 풀·판본별 프레임 길이·지도·모든 실제 경계를 구성 때 확인한다.</summary>
    public RawPriestFall(RawSidPool pool, RawPriestState priest, IReadOnlyList<RiftTypeFrames> frames,
        IReadOnlyList<PriestFallFrames> choices, ushort[] surfaces, PriestFallState state, PriestFallHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(priest); ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(choices); ArgumentNullException.ThrowIfNull(surfaces); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, priest.Pool) || frames.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171)
            || choices.Count != frames.Count || surfaces.Length != 65536 || hooks.EnsureShield is null || hooks.ReserveRegular is null
            || hooks.AddSharedRegular is null || hooks.Unpop is null || hooks.Repop is null || hooks.Sound is null
            || hooks.SetFrame is null || hooks.AdvanceFrame is null || hooks.ClearShield is null)
            throw new ArgumentException("사제 낙하 연결 오류");
        _pool = pool; _priest = priest; _frames = frames; _choices = choices; _surfaces = surfaces; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>분배기와 진행 어댑터가 같은 물리 풀을 사용하는지 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>두 판본의 프레임 필드 폭을 구별해 현재 signed 번호를 읽는다.</summary>
    private int CurrentFrame(ReadOnlySpan<byte> raw) => _pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34];
    /// <summary>보호막→공유 확보→현재 권한 공간 전환→최종 현재 좌표 소리 순서다.</summary>
    public void Begin(int sid)
    {
        _ = _pool.Slot(sid); _hooks.EnsureShield!(sid);
        if (_hooks.ReserveRegular!()) _hooks.AddSharedRegular!(sid, FallEvent, InitialPayload);
        if (_state.GetAuthority()) { _hooks.Unpop!(sid, 0); _hooks.Repop!(sid, 0x800); }
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        _hooks.Sound!(sid, BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]));
    }
    /// <summary>현재 방향이 J일 때만 중복 요청을 생략하며 언제나 참을 반환한다.</summary>
    public bool TryFall(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[10] >= _frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "사제 낙하 요청 프레임 타입 오류");
        if (RawPathAnimation.Direction(_frames[raw[10]], CurrentFrame(raw)) != 9) Begin(sid);
        return true;
    }
    /// <summary>float 합을 double로 유지해 절삭하며 범위 밖은 지면 없음, 비유한 값은 명시 실패한다.</summary>
    private bool SurfaceAt(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        double x = Math.Truncate((double)BinaryPrimitives.ReadSingleLittleEndian(raw[14..]) + CoordinateBias);
        double y = Math.Truncate((double)BinaryPrimitives.ReadSingleLittleEndian(raw[18..]) + CoordinateBias);
        if (!double.IsFinite(x) || !double.IsFinite(y)) throw new ArgumentOutOfRangeException(nameof(sid), "사제 낙하 좌표 오류");
        if (x < 0 || y < 0 || x >= 256 || y >= 256) return false;
        return _surfaces[(int)y * 256 + (int)x] != 0;
    }
    /// <summary>프레임 효과 뒤 위치/TYPE를 다시 읽고 도착 프레임 뒤 이동 가능 여부도 다시 판정한다.</summary>
    public float Handle(int sid, uint ev, uint count, float payload)
    {
        if (ev != FallEvent) throw new ArgumentException("사제 낙하 이벤트 번호 오류", nameof(ev));
        if (_priest.Immobile(sid)) _hooks.EnsureShield!(sid);
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[10] >= _frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "사제 낙하 프레임 타입 오류");
        if (RawPathAnimation.Direction(_frames[raw[10]], CurrentFrame(raw)) == 9) _hooks.AdvanceFrame!(sid, 1, 0);
        else _hooks.SetFrame!(sid, _choices[raw[10]].Falling, 0);
        if (!SurfaceAt(sid)) return RepeatPayload;
        byte current = _pool.Slot(sid)[10];
        if (current >= _choices.Count) throw new ArgumentOutOfRangeException(nameof(sid), "사제 도착 프레임 타입 오류");
        _hooks.SetFrame!(sid, _choices[current].Landed, 0);
        if (!_priest.Immobile(sid)) _hooks.ClearShield!(sid);
        return 0;
    }

    /// <summary>현재 실제 사제 VT의0x25b만 처리하고 다른 사건은 명시 fallback 또는 원문 실패로 분배한다.</summary>
    public static RegularHandler MakePriestFallHandler(RawSidPool pool, RawPriestFall fall, RegularHandler? fallback = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(fall);
        if (!ReferenceEquals(pool, fall.Pool)) throw new ArgumentException("사제 낙하 분배 풀 불일치");
        return (sid, ev, count, payload) =>
        {
            bool priest = BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid)) == (pool.Edition == OriginalEdition.Patch1078 ? 0x0050f210u : 0x005003e0u);
            if (priest && ev == FallEvent) return fall.Handle(sid, ev, count, payload);
            if (fallback is not null) return fallback(sid, ev, count, payload);
            if (priest) throw new ArgumentException("미복원 사제 이벤트 분배");
            return payload;
        };
    }
    /// <summary>현재 사제 VT의 walker 낙하 요청만 공유하고 다른 클래스는 명시 fallback을 요구한다.</summary>
    public static Func<int, bool> MakePriestFallWalker(RawSidPool pool, RawPriestFall fall, Func<int, bool>? fallback = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(fall);
        if (!ReferenceEquals(pool, fall.Pool)) throw new ArgumentException("walker 낙하 분배 풀 불일치");
        return sid =>
        {
            if (BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid)) == (pool.Edition == OriginalEdition.Patch1078 ? 0x0050f210u : 0x005003e0u)) return fall.TryFall(sid);
            if (fallback is not null) return fallback(sid);
            throw new ArgumentException("미복원 walker 낙하 가상 표");
        };
    }
    /// <summary>실제 공유 Regular/보호막 생성/현재 조회·삭제를 합치며 나머지 공간/소리 효과는 그대로 둔다.</summary>
    public static PriestFallHooks MakePriestFallHooks(SquidProcessHost host, RawPriestShield shield, RawPriestForcefield lookup,
        Action<int, uint> destroy, PriestFallHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(host); ArgumentNullException.ThrowIfNull(shield); ArgumentNullException.ThrowIfNull(lookup);
        ArgumentNullException.ThrowIfNull(destroy); ArgumentNullException.ThrowIfNull(hooks);
        RawSidPool pool = host.Pool();
        if (!ReferenceEquals(pool, shield.Pool()) || !ReferenceEquals(pool, lookup.Pool())) throw new ArgumentException("사제 낙하 효과 풀 불일치");
        PriestFallHooks result = hooks.Copy(); result.EnsureShield = sid => shield.Ensure(sid); result.ReserveRegular = () => true;
        result.AddSharedRegular = (sid, ev, payload) => host.AddSharedRegular(sid, ev, payload);
        result.ClearShield = sid => { int found = lookup.Find(sid); if (found > 0 && found < pool.Capacity) destroy(found, 0); };
        return result;
    }
    /// <summary>지정기와 진행기의 같은 실체를 확인하고 DWORD 증분을 signed 비트로 전달한다.</summary>
    public static PriestFallHooks MakePriestFallFrameHooks(SquidFrame setter, RawFrameAdvance advance, PriestFallHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(setter); ArgumentNullException.ThrowIfNull(advance); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(setter.Pool, advance.Pool) || !ReferenceEquals(setter, advance.Setter)) throw new ArgumentException("사제 낙하 프레임 연결 오류");
        PriestFallHooks result = hooks.Copy(); result.SetFrame = (sid, frame, flags) => setter.Set(sid, frame, flags);
        result.AdvanceFrame = (sid, delta, flags) => advance.Advance(sid, unchecked((int)delta), flags); return result;
    }
}
