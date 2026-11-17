// 썬 바리케이드의 실제 Handle00457a50에서 프레임/연결 변경 순서를 보존한다.
// cpppj/src/o/RawFenceRegular.h·cpp 의 RawFenceRegular 본체를 이식한다.
// 0.592에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 정기 처리)
namespace Netstorm.Core.Original;

// 펜스 정기 처리 모듈을 담는 네임스페이스다. 0.592에서 추가됐다. (R4-1 펜스 정기 처리 효과)

/// <summary>
/// 각 선택 위치에서 현재 실제 소비자를 호출하며 누락은 성공으로 취급하지 않는다.
/// (원본 FenceRegularHooks, 10.78).
/// </summary>
public sealed class FenceRegularHooks
{
    /// <summary>00456220의 방향 0/2/4/6 최근접 검색이다. 0.592에서 추가됐다.</summary>
    public Func<int, uint, int>? Neighbor { get; set; }

    /// <summary>004566d0의 두 끝점 사이 기존 fencemark 삭제다. 0.592에서 추가됐다.</summary>
    public Action<int, int>? ClearMarkers { get; set; }

    /// <summary>00455b20(mode0)의 현재 연결선 삭제다. 0.592에서 추가됐다.</summary>
    public Action<float[]>? ClearLine { get; set; }

    /// <summary>004573d0의 현재 활성 연결 수다. 변경 시 두 번 조회한다. 0.592에서 추가됐다.</summary>
    public Func<int, uint>? Connections { get; set; }

    /// <summary>각 new(0x28)의 독립 성공 여부다. 0.592에서 추가됐다.</summary>
    public Func<bool>? ReserveRegular { get; set; }

    /// <summary>같은 Kernel에 사건 2와 0.1f payload를 예약한다. 0.592에서 추가됐다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }

    /// <summary>004578f0의 현재 이웃 스냅샷/재Pop 갱신이다. 0.592에서 추가됐다.</summary>
    public Action<int>? RefreshNeighbors { get; set; }

    /// <summary>00457460의 현재 동맹 이웃과 연결선/mark 재생성이다. 0.592에서 추가됐다.</summary>
    public Action<int>? Rebuild { get; set; }

    /// <summary>차단 bit20 또는 다른 사건은 실제 Damageable Handle로 넘긴다. 0.592에서 추가됐다.</summary>
    public RegularHandler? Damageable { get; set; }
}

/// <summary>
/// 같은 원본 Pool/TYPE 프레임/지정기와 현재 연결 소비자를 참조하는 처리기다.
/// (원본 RawFenceRegular, 10.78).
/// </summary>
public sealed class RawFenceRegular
{
    /// <summary>10.78 썬 바리케이드의 실제 가상 표다. 0.592에서 추가됐다.</summary>
    private const uint FenceHandleTable = 0x00509470u;

    /// <summary>실제 496cb0이 사건 3의 빈 x87 반환을 FST32할 때 생성하는 음수 indefinite NaN이다. 0.592에서 추가됐다.</summary>
    private const uint EmptyX87ResultBits = 0xffc00000U;

    /// <summary>연결선 삭제 요청의 조기 종료를 검사하는 비트(0x20)다. 0.592에서 추가됐다.</summary>
    private const uint EarlyExitMask = 0x20u;

    /// <summary>슬롯 안 추가 비트 바이트의 위치. 0.592에서 추가됐다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 프레임 DWORD(4바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int FrameOffset = 36;

    /// <summary>현재 owner 캐시 DWORD(4바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int OwnerCacheOffset = 42;

    /// <summary>현재 연결 수 DWORD(4바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int ConnectionCountOffset = 46;

    /// <summary>슬롯 안 소유자 바이트의 위치. 0.592에서 추가됐다.</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 X 좌표 DWORD(4바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 Y 좌표 DWORD(4바이트)의 위치. 0.592에서 추가됐다.</summary>
    private const int YOffset = 18;

    /// <summary>원본 글자 구간의 개수(16)다. 0.592에서 추가됐다.</summary>
    private const int LetterKinds = 16;

    /// <summary>현재 frame/owner/+2a/+2e를 소유한 단일 raw 풀. 0.592에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>원본 TYPE+170의 현재 글자 구간 수. 0.592에서 추가됐다.</summary>
    private readonly System.Collections.Generic.IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>같은 풀의 실제 프레임/표시 지정기. 0.592에서 추가됐다.</summary>
    private readonly SquidFrame _setter;

    /// <summary>선택된 하위의 실제 소비자이며 이 객체가 호출 수명을 소유한다. 0.592에서 추가됐다.</summary>
    private readonly FenceRegularHooks _hooks;

    /// <summary>
    /// 펜스 정기 처리 본체를 만든다. 풀·프레임 표·지정기·효과 묶음은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">현재 frame/owner/+2a/+2e를 소유한 단일 raw 풀.</param>
    /// <param name="frames">원본 TYPE+170의 현재 글자 구간 수.</param>
    /// <param name="setter">같은 풀의 실제 프레임/표시 지정기.</param>
    /// <param name="hooks">선택된 하위의 실제 소비자.</param>
    public RawFenceRegular(RawSidPool pool, System.Collections.Generic.IReadOnlyList<RiftTypeFrames> frames, SquidFrame setter, FenceRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(setter);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || frames.Count != 188 || !ReferenceEquals(setter.Pool, pool))
        {
            throw new ArgumentException("썬 바리케이드 Handle 판본/프레임/풀 불일치", nameof(pool));
        }

        _pool = pool;
        _frames = frames;
        _setter = setter;
        _hooks = hooks;
    }

    /// <summary>실제 단일 풀을 연결 단계에서 확인한다. 0.592에서 추가됐다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>TYPE 이름만 같은 다른 부모를 이 처리기로 호출하지 않는다.</summary>
    /// <param name="sid">확인할 슬롯 번호.</param>
    public bool Handles(int sid) => ReadUint(_pool.Slot(sid), 0) == FenceHandleTable;

    /// <summary>
    /// 현재 프레임/소유자/연결 수의 읽기·쓰기·호출 순서로 사건을 처리한다.
    /// 보호 bit20은 사건 번호보다 먼저 검사한다. 사건 0의 두 변경 분기는 독립이다.
    /// </summary>
    /// <param name="sid">처리할 슬롯 번호.</param>
    /// <param name="event">사건 번호.</param>
    /// <param name="count">개수.</param>
    /// <param name="payload">전달 값.</param>
    public float Handle(int sid, uint @event, uint count, float payload)
    {
        if (!Handles(sid))
        {
            throw new ArgumentException("썬 바리케이드 Handle 실제 가상 표 불일치", nameof(sid));
        }

        if ((_pool.Slot(sid)[ExtraOffset] & EarlyExitMask) != 0)
        {
            return Need(_hooks.Damageable)(sid, @event, count, payload);
        }

        if (@event == 0)
        {
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            int type = raw[TypeOffset];
            if (type < 0 || type >= _frames.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), "썬 바리케이드 Handle TYPE 범위 오류");
            }

            int current = unchecked((int)ReadUint(raw, FrameOffset));
            int direction = RawPathAnimation.Direction(_frames[type], current);
            if (direction < 0 || direction >= LetterKinds)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), "썬 바리케이드 Handle 글자 범위 오류");
            }

            int size = _frames[type].Run((byte)('A' + direction)).Count;
            int next = unchecked(current + 1);
            if (size == 0)
            {
                throw new DivideByZeroException("썬 바리케이드 Handle 원본 idiv 0 제수");
            }

            if (next == int.MinValue && size == -1)
            {
                throw new OverflowException("썬 바리케이드 Handle 원본 idiv 범위 오류");
            }

            // 원본은 구간의 첫 번호를 더하지 않고 절대 나머지를 지정한다.
            _setter.Set(sid, next % size, 0);
            if (ReadUint(_pool.Slot(sid), OwnerCacheOffset) != _pool.Slot(sid)[OwnerOffset])
            {
                ClearNeighbors(sid);
            }

            // 원본은 하위 호출을 마친 뒤 캐시 DWORD를 읽는다.
            uint observed = Need(_hooks.Connections)(sid);
            if (observed != ReadUint(_pool.Slot(sid), ConnectionCountOffset))
            {
                // 원본은 비교 때와 기록 때 두 번 현재 값을 조회한다.
                uint changed = Need(_hooks.Connections)(sid);
                WriteUint(_pool.AllocatedBytes(sid), ConnectionCountOffset, changed);
                ClearNeighbors(sid);
            }

            return 0.1f;
        }

        if (@event == 1)
        {
            Need(_hooks.RefreshNeighbors)(sid);
            return 0.0f;
        }

        if (@event == 2)
        {
            Need(_hooks.Rebuild)(sid);
            return 0.0f;
        }

        if (@event == 3)
        {
            ClearNeighbors(sid, false);
            // 원문은 float를 정의하지 않고 반환한다. 정상 ABI의 빈 x87을 실제 496cb0이 FST한 결과를 표현한다.
            // 실제 CPU의 일반 271사례에서 확인한 NaN payload/다음 시각·count 증가를 같은 ProcessHost가 계속 소비한다.
            return BitConverter.UInt32BitsToSingle(EmptyX87ResultBits);
        }

        return Need(_hooks.Damageable)(sid, @event, count, payload);
    }

    /// <summary>
    /// 이미 복원한 다른 부모는 순서와 예외를 유지한 채 fallback으로 넘긴다.
    /// </summary>
    /// <param name="fallback">이 본체가 맡지 않는 부모의 기존 처리기.</param>
    public RegularHandler Handler(RegularHandler? fallback = null)
    {
        return (int sid, uint @event, uint count, float payload) =>
        {
            if (Handles(sid))
            {
                return Handle(sid, @event, count, payload);
            }

            if (fallback is null)
            {
                throw new InvalidOperationException("썬 바리케이드 외 Regular 부모 미연결");
            }

            return fallback(sid, @event, count, payload);
        };
    }

    /// <summary>
    /// 네 이웃을 먼저 모으며 사건 0의 owner 캐시만 그 뒤에 쓴다. 사건 3은 캐시를 갱신하지 않는다.
    /// </summary>
    /// <param name="sid">정리할 슬롯 번호.</param>
    /// <param name="updateOwner">참이면 현재 owner 캐시를 쓴다.</param>
    private void ClearNeighbors(int sid, bool updateOwner = true)
    {
        // 원본 local_1c의 스냅샷이며 효과 뒤 다시 검색하지 않는다. 원본 0/2/4/6 순서다.
        int[] neighbors = new int[4];
        for (uint direction = 0; direction < 8; direction += 2)
        {
            neighbors[direction / 2] = Need(_hooks.Neighbor)(sid, direction);
        }

        if (updateOwner)
        {
            WriteUint(_pool.AllocatedBytes(sid), OwnerCacheOffset, _pool.Slot(sid)[OwnerOffset]);
        }

        // 먼저 모은 네 SID를 같은 순서로 처리한다.
        foreach (int neighbor in neighbors)
        {
            if (neighbor == 0)
            {
                continue;
            }

            Need(_hooks.ClearMarkers)(sid, neighbor);
            // mark 삭제 효과 뒤의 현재 좌표를 읽는다.
            ReadOnlySpan<byte> a = _pool.Slot(sid);
            ReadOnlySpan<byte> b = _pool.Slot(neighbor);
            Need(_hooks.ClearLine)([ReadSingle(a, XOffset), ReadSingle(a, YOffset), ReadSingle(b, XOffset), ReadSingle(b, YOffset)]);
        }

        Schedule(sid);
    }

    /// <summary>개별 할당 실패는 그 예약만 생략한다.</summary>
    /// <param name="sid">예약할 부모 번호.</param>
    private void Schedule(int sid)
    {
        if (Need(_hooks.ReserveRegular)())
        {
            Need(_hooks.AddRegular)(sid, 2, 0.1f);
        }
    }

    /// <summary>비정렬 DWORD의 실제 네 바이트만 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint ReadUint(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 현재 원본 기록의 little endian 결합값이다. 이웃 필드로 읽기를 넓히지 않는다.
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)raw[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>+2a/+2e의 원본 DWORD 폭만 쓴다.</summary>
    /// <param name="raw">쓸 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void WriteUint(System.Span<byte> raw, int offset, uint value)
    {
        // 낮은 바이트부터 기록한다.
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

    /// <summary>미선택 효과는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존한 채 거부한다.</summary>
    /// <typeparam name="T">효과 대리자 형식.</typeparam>
    /// <param name="hook">호출할 효과.</param>
    private static T Need<T>(T? hook) where T : class =>
        hook ?? throw new InvalidOperationException("썬 바리케이드 Handle의 실제 하위 미연결");
}
