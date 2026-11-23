// 사제 K~P 프레임 선택과 604 공유 예약을 실제 프레임 표/Host에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawPriestRitualActions.h의 실제 frame setter·new0x28 성공·SharedRegular 생성의 호출 경계다.</summary>
public sealed class PriestRitualActionHooks
{
    /// <summary>기존 SquidFrame::Set(sid,type+0x158+stage*4,flags0)이다.</summary>
    public Action<int, int, uint>? SetFrame { get; set; }
    /// <summary>frame 요청 뒤 원본 new0x28 성공/실패를 공급한다.</summary>
    public Func<bool>? ReserveShared { get; set; }
    /// <summary>같은 Host.AddSharedRegular(sid,604,0.01f)다.</summary>
    public Action<int, uint, float>? AddShared { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다. 외부 재연결을 캡처된 장치와 구별한다.</summary>
    internal PriestRitualActionHooks Copy() => (PriestRitualActionHooks)MemberwiseClone();
}

/// <summary>RawPriestRitualActions.cpp의 00491ca0/00491cf0 ↔ CD0040cf10/0040cf60 실제 범위/프레임/예약 순서다.</summary>
public sealed class RawPriestRitualActions
{
    /// <summary>계속604 사건과 K..P first-table의 마지막 단계 경계다.</summary>
    private const uint RitualFollowEvent = 0x25c;
    /// <summary>계속604 사건과 K..P first-table의 마지막 단계 경계다.</summary>
    private const uint LastRitualTableStage = 5;
    /// <summary>기본604 SharedRegular payload의 float 비트3c23d70a다.</summary>
    private const float RitualFollowPayload = 0.01F;
    /// <summary>현재 부모 raw 타입을 읽는 유일한 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>원본 type+0x130..0x16c의 현재 첫 프레임 표를 재사용한다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>같은 월드의 실제 frame setter와 공유 예약이다.</summary>
    private readonly PriestRitualActionHooks _hooks;
    /// <summary>누락된 원문 효과는 선택한 시점에만 진단한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("사제 의식 실제 효과 미연결");
    /// <summary>현재 타입별 원본 LetterRun 표는 참조한다. 풀/표/효과 대상은 이 객체보다 오래 살아야 한다.</summary>
    public RawPriestRitualActions(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, PriestRitualActionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(hooks);
        if (frames.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hooks.SetFrame is null || hooks.ReserveShared is null || hooks.AddShared is null)
            throw new ArgumentException("사제 의식 프레임/예약 연결 누락", nameof(hooks));
        _pool = pool; _frames = frames; _hooks = hooks.Copy();
    }
    /// <summary>원본 pointer 범위와 같은 정상 DWORD SID 범위다. SID0은 자연 반환한다.</summary>
    private bool Valid(uint sid) => sid != 0 && sid < (uint)_pool.Capacity && sid <= ushort.MaxValue;
    /// <summary>유효1..capacity-1 번호만 현재 raw 타입의 K+stage 첫 프레임으로 바꾼다. free/dead/void를 별도 조건으로 쓰지 않는다. stage0..5는 실제 타입 first-table의 K..P 구간이다. 그 밖의 손상 단계는 명시적으로 거부한다.</summary>
    public void Begin(uint sid, uint stage)
    {
        if (!Valid(sid)) return;
        if (stage > LastRitualTableStage) throw new ArgumentOutOfRangeException(nameof(stage), "사제 의식 단계가 first-table 밖이다");
        ushort parent = unchecked((ushort)sid); byte type = _pool.Slot(parent)[10];
        if (type >= _frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "사제 의식 현재 raw 타입 범위");
        Need(_hooks.SetFrame)(parent, _frames[type].Run((byte)('K' + stage)).First, 0);
    }
    /// <summary>현재 범위 검사→Begin→new0x28→성공 시604(payload0.01) 공유 예약이다. 기존 같은 사건을 취소하지 않는다.</summary>
    public void Continue(uint sid, uint stage)
    {
        if (!Valid(sid)) return;
        Begin(sid, stage);
        if (Need(_hooks.ReserveShared)()) Need(_hooks.AddShared)((ushort)sid, RitualFollowEvent, RitualFollowPayload);
    }
    /// <summary>연결 어댑터가 같은 raw 월드를 사용하는지 검사한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>기존 SquidFrame/Host의 실제 몸체를 연결한다. 예약 확보 경계는 caller가 공급하며 비울 수 없다.</summary>
    public static PriestRitualActionHooks MakePriestRitualActionHooks(RawSidPool pool, SquidFrame frame, SquidProcessHost host, PriestRitualActionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(frame); ArgumentNullException.ThrowIfNull(host); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(frame.Pool, pool) || !ReferenceEquals(host.Pool(), pool)) throw new ArgumentException("사제 의식 범위 SID 풀 불일치", nameof(pool));
        hooks.SetFrame = (sid, index, flags) => frame.Set(sid, index, flags);
        hooks.AddShared = (sid, @event, payload) => { host.AddSharedRegular(sid, @event, payload); };
        return hooks;
    }
}
