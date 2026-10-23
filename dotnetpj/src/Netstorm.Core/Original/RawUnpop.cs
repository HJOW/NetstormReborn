using Netstorm.Core.Rules;

namespace Netstorm.Core.Original;

/// <summary>
/// Unpop 이 공간 해제 뒤 옛 위치를 알려 주는 공통 표시 경로(원본 update88/update8c)의 연결 지점.
/// 연결하지 않으면(null) 표시 객체가 없는 경로로 보고 아무것도 하지 않는다. 표시 모듈이 생기면 구현한다.
/// </summary>
public interface IRawUnpopDisplay
{
    /// <summary>해제 전에 이 호출을 처리할 수 있는지 검사한다. 처리할 수 없으면 예외를 던져야 하며, 그 시점까지 풀·공간은 바뀌지 않았다.</summary>
    /// <param name="slot">대상 슬롯 바이트.</param>
    /// <param name="flags">Unpop 호출 플래그.</param>
    void Validate(ReadOnlySpan<byte> slot, uint flags);

    /// <summary>void 설정·spot 해제·체인 제거가 끝난 뒤 이전 위치의 변경 표를 공통 표시 경로로 전달한다.</summary>
    /// <param name="slot">대상 슬롯 바이트(좌표·next 는 보존돼 있다).</param>
    /// <param name="flags">Unpop 호출 플래그.</param>
    void Update(ReadOnlySpan<byte> slot, uint flags);
}

/// <summary>
/// 건물 옆면의 점유 바이트를 푸는 건물 공간 모듈의 연결 지점. 건물군 오브젝트(매몰 아님)의 Unpop 은 이 연결이 없으면 거부된다.
/// Pop 과 같은 풀·같은 공간 해시의 몸체만 연결할 수 있다.
/// </summary>
public interface IRawBuildingSide
{
    /// <summary>이 몸체가 쓰는 슬롯 풀.</summary>
    RawSidPool Pool { get; }

    /// <summary>이 몸체가 쓰는 공간 해시.</summary>
    RawSquidHash Hash { get; }

    /// <summary>발자국 옆면의 한 칸에 놓인 건물 점유 바이트를 푼다. Pop 의 표면 플래그 검사 없이 호출된다.</summary>
    /// <param name="cellX">칸 x.</param>
    /// <param name="cellY">칸 y.</param>
    void UnpopSide(int cellX, int cellY);
}

/// <summary>
/// 섬·다리 오브젝트를 해제하기 직전에 옛 위치를 미니맵 갱신 큐에 알리는 연결 지점.
/// 연결하지 않으면(null) 전투 중이 아닌 경로처럼 알림 없이 진행한다. 같은 풀을 쓰는 큐만 연결할 수 있다.
/// </summary>
public interface IRawMinimapQueue
{
    /// <summary>이 큐가 쓰는 슬롯 풀.</summary>
    RawSidPool Pool { get; }

    /// <summary>void 비트가 켜진 직후, spot·체인을 제거하기 전에 오브젝트의 이전 위치를 큐에 넣는다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    void Enqueue(int sid);
}

/// <summary>
/// 오브젝트를 공간 해시와 점유(spot) 지도에서 푸는 원본 공통 Unpop(<c>004afe50</c>, 10.78)의 일반 자산·매몰 경로.
/// 순서는 원본 그대로 "void 설정 → (섬·다리면) 미니맵 알림 → 발자국 spot 해제 → 현재 좌표의 0~3 단계 체인 제거 → 표시 갱신"이며,
/// 제거 대상 슬롯의 next·좌표·저장 단계·HP·섬 번호는 그대로 둔다. free/void 인 슬롯은 아무것도 바꾸지 않고, dead 는 막지 않는다.
/// 원본은 체인에서 못 찾으면 assert 로 멈춘다. 이 구현은 그 밖의 손상(순환·자기 next·범위 밖 next·유한하지 않은 좌표·잘못된 발자국)과
/// 미복원 효과(건물 옆면·표시 갱신의 파생 가상 표)를 <b>풀·공간을 바꾸기 전에</b> 예외로 거부한다. 원본 assert 앞의 부분 변경과 오류 창은 재현하지 않는다.
/// 이 경로에서 spot 지도는 바이트 단위로 칸의 점유를 지우며, 지우는 값은 <see cref="SpotRules.EffectiveGenus"/> 의 하위 바이트다(다른 비트는 보존).
/// <see cref="IRawUnpop"/> 이므로 <see cref="RawSquidFactory"/> 의 non-void Take 가 이 클래스를 쓴다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 체인 미발견을 조용히 넘기는 등 달라서 옮기지 않았다).
/// 근거: docs/exe/cpp-unpop-reconstruction.md, cpppj/src/o/SquidUnpop.cpp (W02 Unpop 조각, 2026-10-16).
/// </summary>
public sealed class RawUnpop : IRawUnpop
{
    /// <summary>슬롯 안 가상 표(4바이트)의 위치.</summary>
    private const int VtableOffset = 0;

    /// <summary>슬롯 안 x 좌표(float, 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float, 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>상태 바이트의 contained 비트(생산 칸에 들어 있는 내용물).</summary>
    private const byte ContainedBit = 8;

    /// <summary>추가 비트의 매몰(buried) 비트. 켜진 오브젝트는 발자국에 spot 을 쓰지 않았으므로 지우지도 않는다.</summary>
    private const byte BuriedBit = 8;

    /// <summary>플래그2에서 미니맵 알림을 받는 섬(2)·다리(4) 비트.</summary>
    private const uint IslandOrBridge = 6;

    /// <summary>슬롯을 읽고 쓸 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>좌표별 오브젝트 체인의 머리를 가진 4단계 공간 해시.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>칸별 점유 바이트(y × 256 + x 순서). 호출자가 공유하는 같은 배열이며 이 클래스가 해제 때 바이트를 고친다.</summary>
    private readonly byte[] _spots;

    /// <summary>공통 표시 경로. 없으면 표시 객체가 없는 경로다.</summary>
    private readonly IRawUnpopDisplay? _display;

    /// <summary>건물 옆면 해제 몸체. 연결 전이면 null 이며 건물군(매몰 아님)의 Unpop 은 거부된다.</summary>
    private IRawBuildingSide? _building;

    /// <summary>미니맵 갱신 큐. 연결 전이면 null.</summary>
    private IRawMinimapQueue? _minimap;

    /// <summary>해제 모듈을 만든다. 점유 지도는 정확히 256×256 바이트여야 한다(부분 지도 밖 쓰기를 막는다).</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">공간 해시.</param>
    /// <param name="spots">칸별 점유 바이트 65,536개. 같은 배열을 Pop 쪽과 공유한다.</param>
    /// <param name="display">공통 표시 경로. 표시를 끈 경로는 null.</param>
    public RawUnpop(RawSidPool pool, RawSquidHash hash, byte[] spots, IRawUnpopDisplay? display = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(spots);
        if (spots.Length != RawSquidHash.WorldCells * RawSquidHash.WorldCells)
        {
            throw new ArgumentException("Unpop spot 지도 크기 오류", nameof(spots));
        }
        _pool = pool;
        _hash = hash;
        _spots = spots;
        _display = display;
    }

    /// <summary>이 모듈이 쓰는 슬롯 풀. 팩토리가 같은 풀인지 확인해 서로 다른 수명 상태의 혼용을 막는다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 이 모듈이 쓰는 공간 해시. 삭제 본체가 Graph 와 같은 공간인지 확인할 때 쓴다.
    /// W03 자산 삭제 분배 조각(2026-10-23)에서 원본 SquidUnpop::ValidateGraph 의 대조를 위해 추가했다.
    /// </summary>
    public RawSquidHash Hash => _hash;

    /// <summary>
    /// 이 모듈이 쓰는 칸별 점유 지도(65,536바이트). Graph 와 같은 배열인지 확인할 때 쓴다.
    /// W03 자산 삭제 분배 조각(2026-10-23)에서 추가했다.
    /// </summary>
    public byte[] Spots => _spots;

    /// <summary>건물 옆면 해제 몸체를 연결한다. 이 모듈과 같은 풀·같은 공간 해시를 쓰는 몸체만 받는다. null 이면 연결을 푼다.</summary>
    /// <param name="building">건물 공간 몸체.</param>
    public void SetBuildingSide(IRawBuildingSide? building)
    {
        if (building is not null && (!ReferenceEquals(building.Pool, _pool) || !ReferenceEquals(building.Hash, _hash)))
        {
            throw new ArgumentException("Unpop/건물 공간의 풀 또는 해시 불일치", nameof(building));
        }
        _building = building;
    }

    /// <summary>미니맵 갱신 큐를 연결한다. 같은 풀을 쓰는 큐만 받는다. null 이면 연결을 푼다.</summary>
    /// <param name="queue">미니맵 큐.</param>
    public void SetMinimapQueue(IRawMinimapQueue? queue)
    {
        if (queue is not null && !ReferenceEquals(queue.Pool, _pool))
        {
            throw new ArgumentException("Unpop/미니맵 큐 SID 풀 불일치", nameof(queue));
        }
        _minimap = queue;
    }

    /// <summary>플래그 0 으로 해제한다(<see cref="RawSquidFactory"/> 의 Take 가 부르는 경로).</summary>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="type">그 오브젝트의 타입 레코드.</param>
    void IRawUnpop.Unpop(int sid, OriginalTypeRecord type) => Unpop(sid, type, 0);

    /// <summary>
    /// 오브젝트를 공간에서 푼다. 끝나면 슬롯은 void 상태이고 spot 의 발자국 비트와 체인 연결이 지워져 있다.
    /// 자산 타입(70 이상) 밖이거나 contained 이거나 번호 0 이면 예외다. 이미 free 또는 void 면 아무것도 바꾸지 않고 돌아온다.
    /// </summary>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="type">그 오브젝트의 타입 레코드(플래그·발자국).</param>
    /// <param name="flags">호출 플래그. 0x2000 은 소용돌이 경로를 뜻한다.</param>
    public void Unpop(int sid, OriginalTypeRecord type, uint flags = 0)
    {
        ArgumentNullException.ThrowIfNull(type);
        Span<byte> bytes = _pool.RawSlot(sid);
        if (sid == 0 || bytes[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType || (bytes[RawSidPool.StateOffset] & ContainedBit) != 0)
        {
            throw new InvalidOperationException("Unpop 자산/SID 상태 오류");
        }
        if ((bytes[RawSidPool.StateOffset] & (RawSidPool.FreeBit | RawSidPool.VoidBit)) != 0)
        {
            return;
        }
        bool buried = (bytes[ExtraOffset] & BuriedBit) != 0;
        if (!buried && (type.Flags2 & OriginalTypeRecord.BuildingMask) != 0 && _building is null)
        {
            throw new InvalidOperationException("Unpop 건물 부착 공간이 연결되지 않았습니다");
        }
        if (!OriginalUnpopDisplay.Supports(Read(bytes, VtableOffset, 4), flags))
        {
            throw new InvalidOperationException("Unpop의 파생 표시 갱신 효과는 아직 지원하지 않습니다");
        }
        float x = BitConverter.UInt32BitsToSingle(Read(bytes, XOffset, 4));
        float y = BitConverter.UInt32BitsToSingle(Read(bytes, YOffset, 4));
        // 버킷 조회에서 유한 좌표·지도 범위를 검증한다. 제거는 저장된 단계를 믿지 않는다
        RawSquidHash.BucketIndex(0, x, y);
        bool writesSpots = !buried && ((type.Flags2 & OriginalTypeRecord.SpotWriteMask) != 0 || (type.Flags1 & OriginalTypeRecord.Surface) != 0);
        int right = (int)x;
        int bottom = (int)y;
        int left = right;
        int top = bottom;
        if (writesSpots)
        {
            if (type.FootX < 1 || type.FootY < 1 || type.FootX > RawSquidHash.WorldCells || type.FootY > RawSquidHash.WorldCells)
            {
                throw new InvalidOperationException("Unpop 발자국 크기 오류");
            }
            left = right - type.FootX + 1;
            top = bottom - type.FootY + 1;
            if (left < 0 || top < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(type), "Unpop 지도 밖 발자국");
            }
        }
        (int foundLevel, int previous) = FindInChains(sid, x, y);
        // 원본은 10.78 에서 체인에 없으면 assert 로 멈춘다
        if (foundLevel < 0)
        {
            throw new InvalidOperationException("Unpop 해시에서 SID를 찾지 못했습니다");
        }
        _display?.Validate(bytes, flags);
        bytes[RawSidPool.StateOffset] |= RawSidPool.VoidBit;
        // 원본은 void 전환 뒤 spot/해시를 제거하기 전에 이전 칸을 미니맵에 알린다
        if (!buried && (type.Flags2 & IslandOrBridge) != 0)
        {
            _minimap?.Enqueue(sid);
        }
        if (writesSpots)
        {
            ClearFootprint(type, x, y, left, top, right, bottom);
        }
        ushort next = (ushort)Read(bytes, RawSidPool.NextOffset, 2);
        if (previous == 0)
        {
            _hash.Bucket(foundLevel, x, y) = next;
        }
        else
        {
            // 이전 슬롯의 next 만 바꾼다. 제거 대상 자신의 next 는 보존한다
            Span<byte> before = _pool.RawSlot(previous);
            before[RawSidPool.NextOffset] = (byte)next;
            before[RawSidPool.NextOffset + 1] = (byte)(next >> 8);
        }
        // 원본은 void/spot/체인 제거 뒤 이전 위치를 갱신한다
        _display?.Update(bytes, flags);
    }

    /// <summary>
    /// 현재 좌표의 0~3 단계 체인을 차례로 훑어 오브젝트를 찾는다. 읽기만 하며 손상된 체인은 쓰기 전에 거부한다:
    /// 유효 번호 밖의 next·순환·자기 next·제거 뒤 자기를 가리키게 될 next 가 그 대상이다.
    /// 찾지 못하면 단계 -1 을 돌려준다.
    /// </summary>
    /// <param name="sid">찾을 슬롯 번호.</param>
    /// <param name="x">오브젝트 x 좌표.</param>
    /// <param name="y">오브젝트 y 좌표.</param>
    /// <returns>찾은 단계(-1 은 없음)와 그 체인에서 바로 앞 슬롯 번호(머리면 0).</returns>
    private (int Level, int Previous) FindInChains(int sid, float x, float y)
    {
        // 큰 표면 배열(단계 0)부터 작은 배열 순서로 찾는다
        for (int level = 0; level < RawSquidHash.Levels; level++)
        {
            int current = _hash.Bucket(level, x, y);
            int previous = 0;
            int visited = 0;
            // 체인의 슬롯을 한 칸씩 따라간다
            while (current != 0)
            {
                if (++visited > _pool.Capacity)
                {
                    throw new InvalidOperationException("Unpop 순환 체인");
                }
                int next = (int)Read(_pool.Slot(current), RawSidPool.NextOffset, 2);
                if (current == sid)
                {
                    if (next == current || (previous != 0 && next == previous))
                    {
                        throw new InvalidOperationException("Unpop 제거 뒤 자기 next");
                    }
                    if (next >= _pool.Capacity)
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), next, "Unpop next SID 범위 오류");
                    }
                    return (level, previous);
                }
                if (next == current)
                {
                    throw new InvalidOperationException("Unpop 자기 next");
                }
                previous = current;
                current = next;
            }
        }
        return (-1, 0);
    }

    /// <summary>
    /// 발자국의 칸마다 점유 바이트에서 이 타입이 적었던 값을 AND 로 지운다(위쪽 행부터, 행 안은 왼쪽부터). 다른 비트는 보존한다.
    /// 건물군이면 발자국의 왼쪽·오른쪽 가장자리 열(맨 위·맨 아래 행 제외)에서 건물 옆면도 함께 푼다.
    /// </summary>
    /// <param name="type">타입 레코드.</param>
    /// <param name="x">오브젝트 x 좌표.</param>
    /// <param name="y">오브젝트 y 좌표.</param>
    /// <param name="left">발자국 왼쪽 칸.</param>
    /// <param name="top">발자국 위쪽 칸.</param>
    /// <param name="right">발자국 오른쪽 칸.</param>
    /// <param name="bottom">발자국 아래쪽 칸.</param>
    private void ClearFootprint(OriginalTypeRecord type, float x, float y, int left, int top, int right, int bottom)
    {
        bool building = (type.Flags2 & OriginalTypeRecord.BuildingMask) != 0;
        // 위쪽 행부터 한 줄씩 지운다
        for (int cy = top; cy <= bottom; cy++)
        {
            // 행 안의 칸을 왼쪽부터 지운다. 발자국 루프의 정수 절삭과 EffectiveGenus 의 소수 편향 계산은 다르다
            for (int cx = left; cx <= right; cx++)
            {
                byte genus = (byte)SpotRules.EffectiveGenus(type.Flags2, x, y, type.FootX, type.FootY, cx, cy);
                _spots[cy * RawSquidHash.WorldCells + cx] &= (byte)~genus;
                // Unpop 은 Pop 의 표면 플래그 검사 없이 옆면 SID 의 BYTE 비트를 푼다
                if (building && (cx == left || cx == right) && cy != top && cy != bottom)
                {
                    _building!.UnpopSide(cx, cy);
                }
            }
        }
    }

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 필요한 폭만큼 읽는다(이웃 필드를 읽지 않는다).</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수(1~4).</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
}
