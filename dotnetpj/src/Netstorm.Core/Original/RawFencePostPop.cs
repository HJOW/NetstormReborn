// PATCH10.78 썬 바리케이드의 최초 후처리를 실제 이웃/연결선/Regular 소비자와 조합한다.
// cpppj/src/o/RawFencePostPop.h·cpp 의 RawFencePostPop 본체를 이식한다.
// 0.592에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 후처리)
namespace Netstorm.Core.Original;

// 펜스 후처리 모듈을 담는 네임스페이스다. 0.592에서 추가됐다. (R4-1 펜스 후처리 효과)

/// <summary>공통 장부 사전 확인에 전달하는 후처리 효과다. 0.592에서 추가됐다.</summary>
/// <param name="sid">확인할 슬롯 번호.</param>
/// <param name="flags">후처리 플래그.</param>
/// <param name="projected">현재 공통 후처리 검사에 그대로 전달하는 투영 위치.</param>
public delegate void FenceValidateBase(int sid, uint flags, PopProjection projected);

/// <summary>
/// 00457560의 하위 몸체다. 선택된 효과는 반드시 연결하며 빈 성공으로 대체하지 않는다.
/// (원본 FencePostPopHooks, 10.78).
/// </summary>
public sealed class FencePostPopHooks
{
    /// <summary>현재 TYPE+98의 원소 번호를 조회한다. 0.592에서 추가됐다.</summary>
    public Func<int, uint>? Theme { get; set; }

    /// <summary>00456220의 방향 0/2/4/6 최근접 펜스다. 0.592에서 추가됐다.</summary>
    public Func<int, uint, int>? Neighbor { get; set; }

    /// <summary>각 new(0x28)의 독립 성공 여부다. 0.592에서 추가됐다.</summary>
    public Func<bool>? ReserveRegular { get; set; }

    /// <summary>동일 부모의 실제 사건을 예약한다. 0.592에서 추가됐다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }

    /// <summary>00455b20(mode0)의 기존 연결선 삭제다. 0.592에서 추가됐다.</summary>
    public Action<float[]>? ClearLine { get; set; }

    /// <summary>004573d0의 현재 유효 연결 개수다. 0.592에서 추가됐다.</summary>
    public Func<int, uint>? Connections { get; set; }

    /// <summary>Pop 쓰기 전 공통 장부를 확인한다. 비우면 검사를 건너뛴다. 0.592에서 추가됐다.</summary>
    public FenceValidateBase? ValidateBase { get; set; }

    /// <summary>0044bf80의 같은 비가상 후처리 사슬이다. 0.592에서 추가됐다.</summary>
    public Action<int, uint>? Damageable { get; set; }
}

/// <summary>
/// 입력 풀은 이 객체보다 오래 살아야 하며 후처리 중 효과가 바꾼 바이트를 다시 읽는다.
/// (원본 RawFencePostPop, 10.78).
/// </summary>
public sealed class RawFencePostPop
{
    /// <summary>PATCH 썬 바리케이드 생성자의 실제 가상 주소다. 0.592에서 추가됐다.</summary>
    private const uint FenceVtable = 0x00509470u;

    /// <summary>PostPop 사건 payload 0.1f의 비트다. 0.592에서 추가됐다.</summary>
    private const uint RegularPayloadBits = 0x3dcccccdU;

    /// <summary>슬롯 안 소유자 바이트의 위치. 0.592에서 추가됐다.</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 X 좌표 DWORD(4바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 Y 좌표 DWORD(4바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int YOffset = 18;

    /// <summary>현재 owner 캐시 DWORD(4바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int OwnerCacheOffset = 42;

    /// <summary>현재 연결 수 DWORD(4바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int ConnectionCountOffset = 46;

    /// <summary>현재 원본 50바이트 슬롯을 소유한 실제 월드 풀. 0.592에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>조회/힙/예약/연결선/공통 후처리의 실제 소비자다. 0.592에서 추가됐다.</summary>
    private readonly FencePostPopHooks _hooks;

    /// <summary>
    /// 펜스 후처리 본체를 만든다. 풀과 효과 묶음은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">현재 원본 슬롯을 소유한 실제 월드 풀.</param>
    /// <param name="hooks">조회/힙/예약/연결선/공통 후처리의 실제 소비자.</param>
    public RawFencePostPop(RawSidPool pool, FencePostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("썬 바리케이드 후처리는 PATCH10.78만 복원했습니다", nameof(pool));
        }

        _pool = pool;
        _hooks = hooks;
    }

    /// <summary>상위 분배기가 같은 풀인지 확인할 때 사용한다. 0.592에서 추가됐다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>생성자 00457e80의 실제 가상 표 00509470을 확인한다.</summary>
    /// <param name="sid">확인할 슬롯 번호.</param>
    public bool Handles(int sid) => ReadUint(_pool.Slot(sid), 0) == FenceVtable;

    /// <summary>같은 공간/장부를 사전 확인한다. projected 위치는 현재 공통 후처리 검사에 그대로 전달한다.</summary>
    /// <param name="sid">확인할 슬롯 번호.</param>
    /// <param name="flags">후처리 플래그.</param>
    /// <param name="projected">공통 검사에 전달하는 투영 위치.</param>
    public void Validate(int sid, uint flags, PopProjection projected = default)
    {
        if (!Handles(sid))
        {
            throw new ArgumentException("썬 바리케이드 실제 가상 표 불일치", nameof(sid));
        }

        _hooks.ValidateBase?.Invoke(sid, flags, projected);
    }

    /// <summary>
    /// 원본 00457560의 전체 접두와 Damageable을 순서대로 실행한다.
    /// flags 1에서만 최초 사건과 연결 수를 준비하고 모든 flags에서 같은 Damageable로 끝난다.
    /// </summary>
    /// <param name="sid">후처리할 슬롯 번호.</param>
    /// <param name="flags">후처리 플래그.</param>
    public void PostPop(int sid, uint flags)
    {
        // 실제 가상 진입은 선택된 몸체를 바로 실행하며 명시 Validate의 별도 사전 계약을 삽입하지 않는다.
        if ((flags & 1U) != 0)
        {
            if (Need(_hooks.Theme)(sid) == 3)
            {
                Regular(sid, 0);
            }

            // 원문 local_1c는 네 방향을 모두 조회한 뒤 두 쌍에서 재사용한다. 방향 순서를 0/2/4/6으로 고정한다.
            int[] neighbors = new int[4];
            for (uint direction = 0; direction < 8; direction += 2)
            {
                neighbors[direction / 2] = Need(_hooks.Neighbor)(sid, direction);
            }

            WriteUint(_pool.RawSlot(sid), OwnerCacheOffset, _pool.Slot(sid)[OwnerOffset]);
            if (neighbors[0] != 0 && neighbors[2] != 0)
            {
                Regular(neighbors[0], 3);
                Regular(neighbors[2], 3);
                ClearPair(neighbors[0], neighbors[2]);
            }

            if (neighbors[1] != 0 && neighbors[3] != 0)
            {
                Regular(neighbors[1], 3);
                Regular(neighbors[3], 3);
                ClearPair(neighbors[1], neighbors[3]);
            }

            // 조회의 외부 효과 뒤 반환값만 원본 +2e에 기록한다.
            uint count = Need(_hooks.Connections)(sid);
            WriteUint(_pool.RawSlot(sid), ConnectionCountOffset, count);
            Regular(sid, 2);
        }

        Need(_hooks.Damageable)(sid, flags);
    }

    /// <summary>매 할당의 독립 실패는 해당 생성자만 생략한다. float 0.1의 비트도 유지한다.</summary>
    /// <param name="sid">예약할 부모 번호.</param>
    /// <param name="event">예약할 사건 번호.</param>
    private void Regular(int sid, uint @event)
    {
        if (Need(_hooks.ReserveRegular)())
        {
            Need(_hooks.AddRegular)(sid, @event, BitConverter.UInt32BitsToSingle(RegularPayloadBits));
        }
    }

    /// <summary>확보/예약 효과가 변경한 두 슬롯의 현재 좌표를 네 float 값으로 복사한다.</summary>
    /// <param name="first">첫 끝점 번호.</param>
    /// <param name="second">둘째 끝점 번호.</param>
    private void ClearPair(int first, int second)
    {
        // 각 쌍의 두 끝점은 이 호출 직전에 조회한다.
        ReadOnlySpan<byte> a = _pool.Slot(first);
        ReadOnlySpan<byte> b = _pool.Slot(second);
        Need(_hooks.ClearLine)([ReadSingle(a, XOffset), ReadSingle(a, YOffset), ReadSingle(b, XOffset), ReadSingle(b, YOffset)]);
    }

    /// <summary>비정렬 DWORD를 읽으며 원본 포인터를 호스트 주소로 실행하지 않는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint ReadUint(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 원본 little endian 네 바이트의 결합 결과다. 필드 밖 바이트는 읽지 않는다.
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)raw[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>원본이 쓴 DWORD 폭만 바꾸고 이웃 필드와 padding을 보존한다.</summary>
    /// <param name="raw">쓸 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void WriteUint(System.Span<byte> raw, int offset, uint value)
    {
        // 낮은 바이트부터 저장한다.
        for (int index = 0; index < 4; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>비정렬 float 좌표의 비트를 그대로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static float ReadSingle(ReadOnlySpan<byte> raw, int offset) =>
        BitConverter.UInt32BitsToSingle(ReadUint(raw, offset));

    /// <summary>미연결 효과는 선택 위치에서 실패시키며 앞선 쓰기와 예약을 보존한다.</summary>
    /// <typeparam name="T">효과 대리자 형식.</typeparam>
    /// <param name="hook">호출할 효과.</param>
    private static T Need<T>(T? hook) where T : class =>
        hook ?? throw new InvalidOperationException("썬 바리케이드 후처리의 실제 하위 소비자 미연결");
}
