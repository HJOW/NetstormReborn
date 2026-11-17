// 아이스 타워의 파괴·재성장·주기를 같은 장기 물리/TYPE/Host와 가상 부모에 연결한다.
// cpppj/src/o/RawIceTower.h·cpp 의 IceTowerTypes/RawIceTower 본체를 이식한다.
// 0.583에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Ice 우선)
namespace Netstorm.Core.Original;

/// <summary>
/// 현재 원본 TYPE/로딩 전역이다. 모든 효과 뒤에도 같은 저장소를 읽는다.
/// (원본 IceTowerTypes, 10.78).
/// </summary>
public sealed class IceTowerTypes
{
    /// <summary>원본 005412a4의 현재값이다. 독립 입력 기본값은 147이다.</summary>
    public uint Ice { get; set; } = 147;

    /// <summary>원본 005412b0의 현재값이다. 독립 입력 기본값은 150이다.</summary>
    public uint Growing { get; set; } = 150;

    /// <summary>원본 005412dc의 현재값이다. 독립 입력 기본값은 157이다.</summary>
    public uint NoIsland { get; set; } = 157;

    /// <summary>같은 장기 실제 너겟... 소리 제외가 아니라 완성형 TYPE 주소를 빌린다.</summary>
    public UIntBox? SharedIce { get; set; }

    /// <summary>같은 장기 성장형 TYPE 주소를 빌린다.</summary>
    public UIntBox? SharedGrowing { get; set; }

    /// <summary>같은 장기 무인도 TYPE 주소를 빌린다.</summary>
    public UIntBox? SharedNoIsland { get; set; }

    /// <summary>현재 완성형 TYPE는 원문 선택 위치마다 읽는다.</summary>
    public uint ReadIce() => SharedIce?.Value ?? Ice;

    /// <summary>현재 생성 직전 성장형 TYPE를 읽는다.</summary>
    public uint ReadGrowing() => SharedGrowing?.Value ?? Growing;

    /// <summary>지면 조회 뒤 현재 무인도 TYPE를 읽는다.</summary>
    public uint ReadNoIsland() => SharedNoIsland?.Value ?? NoIsland;
}

/// <summary>
/// 게임 세계의 실제 생성·공간·소유자·삭제와 공통 후처리를 공급한다.
/// (원본 IceTowerHooks, 10.78).
/// </summary>
public sealed class IceTowerHooks
{
    /// <summary>실제 new40의 성공 여부다.</summary>
    public Func<bool>? ReserveRegular { get; set; }

    /// <summary>같은 예약 메모리를 원본 사건·주기로 부착한다.</summary>
    public Action<int, uint, float>? Regular { get; set; }

    /// <summary>성장형의 마지막 공통 후처리다.</summary>
    public Action<int, uint>? BasePop { get; set; }

    /// <summary>절삭한 표면칸 첫 SID다.</summary>
    public Func<float, float, int>? Grid { get; set; }

    /// <summary>현재 일반 finder다.</summary>
    public Func<float, float, uint, int>? GenusAt { get; set; }

    /// <summary>실제 생성자/예약이다.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>가상 후처리 중에도 플래그 없이 공간만 해제한다.</summary>
    public Action<int, uint>? Unpop { get; set; }

    /// <summary>현재 WORD와 다리 표면 통지다.</summary>
    public Action<int, ushort>? Word { get; set; }

    /// <summary>가상 소유자 지정이다.</summary>
    public Action<int, uint>? Owner { get; set; }

    /// <summary>가상 같은 현재 좌표에 실제 재배치다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }

    /// <summary>성장 완료 때 자기 가상 삭제를 플래그 없이 호출한다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>실제 받침 정리다.</summary>
    public Action<float, float, uint, uint>? DropIsland { get; set; }

    /// <summary>완성형의 마지막 공통 후처리다.</summary>
    public Action<int, uint>? DamageablePost { get; set; }

    /// <summary>성장형의 마지막 공통 후처리다.</summary>
    public Action<int, uint>? BasePost { get; set; }

    /// <summary>표시/프레임 갱신이다.</summary>
    public Action<int, int, uint>? SetFrame { get; set; }
}

/// <summary>
/// 아이스 타워147의 파괴·성장형150의 예약/성장/삭제를 원본 순서대로 복원한다.
/// (원본 RawIceTower, 10.78).
/// </summary>
public sealed class RawIceTower
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 추가 비트 바이트의 위치.</summary>
    private const int ExtraOffset = 40;

    /// <summary>abstract·매립 마스크다.</summary>
    private const uint BlockedMask = 9u;

    /// <summary>원문의 전체 성장 시간이다.</summary>
    private const float GrowthSeconds = 40.0f;

    /// <summary>성장형만 처리하는 예약 번호다.</summary>
    private const uint GrowthEvent = 0x3a6u;

    /// <summary>객체 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 TYPE의 코드 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>효과 뒤에도 같은 저장소를 읽는 현재 원본 TYPE이다.</summary>
    private readonly IceTowerTypes _types;

    /// <summary>재성장 생성과 최종 타워 생성의 서로 다른 현재 권한이다.</summary>
    private readonly Func<bool> _battle;

    /// <summary>재성장 생성과 최종 타워 생성의 서로 다른 현재 권한이다.</summary>
    private readonly Func<bool> _boss;

    /// <summary>완성형 파괴의 최초 로딩 관문이다.</summary>
    private readonly Func<uint> _loadingDepth;

    /// <summary>같은 월드의 실제 하위 몸체다.</summary>
    private readonly IceTowerHooks _hooks;

    /// <summary>
    /// 아이스 타워 본체를 만든다. 풀·프레임·타입·권한·로딩·훅은 이 객체보다 오래 살아야 한다.
    /// </summary>
    public RawIceTower(
        RawSidPool pool,
        IReadOnlyList<RiftTypeFrames> frames,
        IceTowerTypes types,
        Func<bool> battle,
        Func<bool> boss,
        Func<uint> loadingDepth,
        IceTowerHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(battle);
        ArgumentNullException.ThrowIfNull(boss);
        ArgumentNullException.ThrowIfNull(loadingDepth);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || frames.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException("아이스 타워 판본/프레임 오류", nameof(pool));
        }

        if (hooks.ReserveRegular is null || hooks.Regular is null || hooks.BasePop is null || hooks.Grid is null
            || hooks.GenusAt is null || hooks.Create is null || hooks.Unpop is null || hooks.Word is null
            || hooks.Owner is null || hooks.Pop is null || hooks.Destroy is null || hooks.DropIsland is null
            || hooks.DamageablePost is null || hooks.BasePost is null || hooks.SetFrame is null)
        {
            throw new ArgumentException("아이스 타워 하위 효과 누락", nameof(hooks));
        }

        _pool = pool;
        _frames = frames;
        _types = types;
        _battle = battle;
        _boss = boss;
        _loadingDepth = loadingDepth;
        _hooks = hooks;
    }

    /// <summary>같은 raw 풀이다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>각 효과 뒤의 현재 좌표를 원본 float 비트로 읽는다.</summary>
    private float[] Position(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return [BitConverter.UInt32BitsToSingle(Read(raw, 14)), BitConverter.UInt32BitsToSingle(Read(raw, 18))];
    }

    /// <summary>실제 188프레임과 같은 Host 예약/공통 장부를 성장형 후처리에 전달한다.</summary>
    public void PostPop(int sid, uint flags) =>
        PostPopGrowingIce(_pool, _frames, sid, flags, Need(_hooks.ReserveRegular), Need(_hooks.Regular), Need(_hooks.BasePop));

    /// <summary>완성형은 먼저 공간을 비우고 현재 WORD/소유자/좌표 순서대로 성장형에 전달한다.</summary>
    public void PostDestroy(int sid, uint flags)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[TypeOffset] == _types.ReadIce() && (raw[ExtraOffset] & BlockedMask) == 0 && (flags & 0x200000u) == 0 && _loadingDepth() == 0)
        {
            float[] point = Position(sid);
            int surface = Need(_hooks.Grid)(point[0], point[1]);
            if (surface != 0 && _pool.Slot(surface)[TypeOffset] == _types.ReadNoIsland() && (flags & 0x8000000u) != 0)
            {
                float[] at = Position(sid);
                Need(_hooks.DropIsland)(at[0], at[1], flags, _pool.Slot(sid)[34]);
                Need(_hooks.DamageablePost)(sid, flags);
                return;
            }

            if (_battle())
            {
                Need(_hooks.Unpop)(sid, 0);
                int born = Need(_hooks.Create)(_types.ReadGrowing(), 0);
                Need(_hooks.Word)(born, (ushort)Read(_pool.Slot(sid), 12, 2));
                _pool.RawSlot(born)[35] = _pool.Slot(sid)[35];
                Need(_hooks.Owner)(born, _pool.Slot(sid)[34]);
                WriteFrame(_pool, born, 0);
                float[] at = Position(sid);
                Need(_hooks.Pop)(born, at[0], at[1], 0);
            }
        }

        Need(_hooks.DamageablePost)(sid, flags);
    }

    /// <summary>성장형은 받침 정리 조건과 공통 후처리만 수행한다.</summary>
    public void GrowingPostDestroy(int sid, uint flags)
    {
        if ((_pool.Slot(sid)[ExtraOffset] & BlockedMask) == 0 && (flags & 0x8200000u) != 0)
        {
            float[] point = Position(sid);
            if (Need(_hooks.GenusAt)(point[0], point[1], 0x1000000u) != 0)
            {
                float[] at = Position(sid);
                Need(_hooks.DropIsland)(at[0], at[1], flags, _pool.Slot(sid)[34]);
            }
        }

        Need(_hooks.BasePost)(sid, flags);
    }

    /// <summary>사건 번호 외에는 원래 프레임 DWORD의 감김 증가와 부호 비교만 수행한다.</summary>
    public float Handle(int sid, uint @event, uint count, float payload)
    {
        if (@event != GrowthEvent)
        {
            return payload;
        }

        if ((_pool.Slot(sid)[ExtraOffset] & 0x20u) == 0)
        {
            uint next = Read(_pool.Slot(sid), 36) + 1u;
            uint last = unchecked((uint)FrameCount(_pool, _frames, sid)) - 1u;
            if (unchecked((int)next) < unchecked((int)last))
            {
                Need(_hooks.SetFrame)(sid, unchecked((int)next), 0);
            }
            else if (_boss())
            {
                int born = Need(_hooks.Create)(_types.ReadIce(), 0);
                Need(_hooks.Owner)(born, _pool.Slot(sid)[34]);
                float[] point = Position(sid);
                Need(_hooks.Destroy)(sid, 0);
                Need(_hooks.Pop)(born, point[0], point[1], 0);
                return -1.0f;
            }
        }

        return (float)((double)GrowthSeconds / FrameCount(_pool, _frames, sid));
    }

    /// <summary>최초 예약의 할당 실패는 공통 후처리를 막지 않으며 추가 관문을 두지 않는다.</summary>
    private static void PostPopGrowingIce(
        RawSidPool pool,
        IReadOnlyList<RiftTypeFrames> frames,
        int sid,
        uint flags,
        Func<bool> reserve,
        Action<int, uint, float> regular,
        Action<int, uint> @base)
    {
        if (pool.Edition != OriginalEdition.Patch1078 || frames.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException("성장형 아이스 후처리 판본/프레임 오류", nameof(pool));
        }

        if ((flags & 1u) != 0 && Need(reserve)())
        {
            float period = GrowthSeconds / FrameCount(pool, frames, sid);
            Need(regular)(sid, GrowthEvent, period);
        }

        Need(@base)(sid, flags);
    }

    /// <summary>원본 TYPE+114의 현재 프레임 수이며 부분 코드 표나 다른 TYPE를 섞지 않는다.</summary>
    private static int FrameCount(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, int sid)
    {
        byte type = pool.Slot(sid)[TypeOffset];
        if (type >= frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "아이스 현재 프레임 TYPE 오류");
        }

        return frames[type].Codes.Count;
    }

    /// <summary>비정렬 raw의 지정 바이트 폭을 주변 값과 섞지 않고 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 원본 little endian이다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)raw[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>현재 DWORD 프레임만 쓰며 표시 갱신을 추가하지 않는다.</summary>
    private static void WriteFrame(RawSidPool pool, int sid, uint value)
    {
        Span<byte> raw = pool.RawSlot(sid);
        // 네 바이트 전체 대입이다.
        for (int index = 0; index < 4; index++)
        {
            raw[36 + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>필요한 효과 경계가 없으면 빈 성공으로 대체하지 않는다.</summary>
    private static T Need<T>(T? hook) where T : class =>
        hook ?? throw new InvalidOperationException("아이스 타워 실제 하위 몸체 미연결");
}
