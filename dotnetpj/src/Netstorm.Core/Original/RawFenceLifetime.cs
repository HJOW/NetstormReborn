// PATCH 썬 바리케이드의 연결 정리·이웃 갱신·가상 preDestroy를 같은 SID 풀에 적용한다.
// cpppj/src/o/RawFenceLifetime.h·cpp 의 RawFenceLifetime 본체를 이식한다.
// 0.587에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 우선)
namespace Netstorm.Core.Original;

/// <summary>
/// 실제 현재 이웃·연결선·Pop/Regular/Damageable 몸체를 제공하는 효과 경계다.
/// (원본 FenceLifetimeHooks, 10.78).
/// </summary>
public sealed class FenceLifetimeHooks
{
    /// <summary>동맹이면 455fb0, 아니면 4556220이다.</summary>
    public Func<int, uint, bool, int>? Neighbor { get; set; }

    /// <summary>실제 두 끝점 표식 삭제다.</summary>
    public Action<int, int>? ClearMarkers { get; set; }

    /// <summary>현재 좌표/소유자/끝점의 연결선 정리다.</summary>
    public Action<float[], uint, uint, int, int>? ClearLine { get; set; }

    /// <summary>현재 가상+48이다.</summary>
    public Action<int, uint>? Unpop { get; set; }

    /// <summary>현재 가상+4c이다.</summary>
    public Action<int, uint>? Repop { get; set; }

    /// <summary>현재 동맹/사거리/연결 재생성이다.</summary>
    public Action<int>? Rebuild { get; set; }

    /// <summary>실제 new(28)의 성공 여부다.</summary>
    public Func<bool>? ReserveRegular { get; set; }

    /// <summary>이웃 부모에 사건·지연을 부착한다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }

    /// <summary>현재 같은 플래그 전처리다.</summary>
    public Action<int, uint>? Damageable { get; set; }
}

/// <summary>
/// 4577f0/4578f0/457990의 원본 순서·스냅샷·현재 raw 재조회를 보존한다.
/// (원본 RawFenceLifetime, 10.78).
/// </summary>
public sealed class RawFenceLifetime
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 추가 비트 바이트의 위치.</summary>
    private const int ExtraOffset = 40;

    /// <summary>abstract·매립·contained 마스크다.</summary>
    private const uint BlockedMask = 9u;

    /// <summary>삭제 대상 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>계산을 대신하지 않는 실제 월드 효과다.</summary>
    private readonly FenceLifetimeHooks _hooks;

    /// <summary>
    /// 펜스 수명 본체를 만든다. 풀과 효과 묶음은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">삭제·재생성이 변경하는 단일 raw 풀.</param>
    /// <param name="hooks">실제 월드 효과 묶음이다.</param>
    public RawFenceLifetime(RawSidPool pool, FenceLifetimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("펜스 수명10.78 풀 계약 오류", nameof(pool));
        }

        if (hooks.Neighbor is null || hooks.ClearMarkers is null || hooks.ClearLine is null
            || hooks.Unpop is null || hooks.Repop is null || hooks.Rebuild is null
            || hooks.ReserveRegular is null || hooks.AddRegular is null || hooks.Damageable is null)
        {
            throw new ArgumentException("펜스 수명 효과 누락", nameof(hooks));
        }

        _pool = pool;
        _hooks = hooks;
    }

    /// <summary>실제 단일 풀을 연결 단계에서 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>원본의 예약 슬롯도 범위 안에서만 읽는다.</summary>
    private ReadOnlySpan<byte> Raw(int sid)
    {
        // Refresh의 포인터 유효성 검사에서 SID1도 허용하던 원본 범위를 유지한다.
        if (sid <= 0 || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "펜스 수명 SID 범위 오류");
        }

        return _pool.Slot(sid);
    }

    /// <summary>앞선 표식 삭제/재생성이 좌표를 바꿀 수 있으므로 호출마다 현재 슬롯을 읽는다.</summary>
    private float[] Endpoints(int first, int second)
    {
        ReadOnlySpan<byte> a = Raw(first);
        ReadOnlySpan<byte> b = Raw(second);
        return [ReadSingle(a, 14), ReadSingle(a, 18), ReadSingle(b, 14), ReadSingle(b, 18)];
    }

    /// <summary>최초 extra 검사는 이 함수 한 번의 진입에서만 수행한다.</summary>
    /// <param name="parent">정리할 펜스 번호다.</param>
    public void Clear(int parent)
    {
        if ((Raw(parent)[ExtraOffset] & BlockedMask) != 0)
        {
            return;
        }

        // 현재 동맹 이웃을0/2/4/6 순서로 찾고 효과 후 현재 인자를 다시 읽는다.
        for (uint direction = 0; direction < 8; direction += 2)
        {
            int other = Need(_hooks.Neighbor)(parent, direction, true);
            if (other == 0)
            {
                continue;
            }

            Need(_hooks.ClearMarkers)(parent, other);
            float[] points = Endpoints(parent, other);
            ReadOnlySpan<byte> a = Raw(parent);
            ReadOnlySpan<byte> b = Raw(other);
            Need(_hooks.ClearLine)(points, a[34], b[34], parent, other);
        }
    }

    /// <summary>네 이웃 스냅샷은 자기 Unpop 전에 만들고 각 이웃의 유효성/dead/타입은 처리 시 다시 읽는다.</summary>
    /// <param name="parent">갱신할 펜스 번호다.</param>
    public void Refresh(int parent)
    {
        Raw(parent);
        int[] neighbors = new int[4];
        // 원본은 네 방향을 모두 저장한 후에야 정리 효과를 시작한다.
        for (uint direction = 0; direction < 8; direction += 2)
        {
            neighbors[direction / 2] = Need(_hooks.Neighbor)(parent, direction, true);
        }

        Clear(parent);
        if ((Raw(parent)[StateOffset] & 2) == 0)
        {
            Need(_hooks.Unpop)(parent, 0);
        }

        // 재생성이 다음 이웃 또는 부모의 타입/상태를 바꾸면 원본처럼 다음 처리에 반영한다.
        foreach (int other in neighbors)
        {
            if (other == 0 || other >= _pool.Capacity)
            {
                continue;
            }

            ReadOnlySpan<byte> raw = Raw(other);
            if ((raw[StateOffset] & 2) == 0 && raw[TypeOffset] == Raw(parent)[TypeOffset])
            {
                Clear(other);
                Need(_hooks.Rebuild)(other);
            }
        }

        if ((Raw(parent)[StateOffset] & 2) == 0)
        {
            Need(_hooks.Repop)(parent, 0x40);
        }
    }

    /// <summary>마지막 일반 이웃은 네 개를 읽되 원본은 첫 두 칸에만 지연 Regular를 붙인다.</summary>
    /// <param name="parent">삭제할 펜스 번호다.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트다.</param>
    public void PreDestroy(int parent, uint flags)
    {
        Refresh(parent);
        int[] neighbors = new int[4];
        // 실제 비동맹 제한 없는 현재 이웃을 따로 저장한다.
        for (uint direction = 0; direction < 8; direction += 2)
        {
            neighbors[direction / 2] = Need(_hooks.Neighbor)(parent, direction, false);
        }

        // 이 원본의 2회 조건을 네 방향으로 고쳐 쓰지 않는다.
        for (int index = 0; index < 2; index++)
        {
            if (neighbors[index] != 0 && Need(_hooks.ReserveRegular)())
            {
                Need(_hooks.AddRegular)(neighbors[index], 2, 0.10000000149011612f);
            }
        }

        Need(_hooks.Damageable)(parent, flags);
    }

    /// <summary>비정렬 float 좌표의 비트를 그대로 읽는다.</summary>
    private static float ReadSingle(ReadOnlySpan<byte> raw, int offset) =>
        BitConverter.UInt32BitsToSingle(ReadUint(raw, offset));

    /// <summary>비정렬 raw DWORD를 낮은 바이트부터 조립한다.</summary>
    private static uint ReadUint(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)raw[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>실제 선택된 효과가 빠졌을 때 빈 처리로 넘어가지 않는다.</summary>
    private static T Need<T>(T? hook) where T : class =>
        hook ?? throw new InvalidOperationException("실제 펜스 수명 효과 미연결");
}
