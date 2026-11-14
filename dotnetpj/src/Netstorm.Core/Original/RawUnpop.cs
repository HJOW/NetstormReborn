using Netstorm.Core.Rules;

namespace Netstorm.Core.Original;

/// <summary>
/// Unpop 이 공간 해제 뒤 옛 위치를 알려 주는 공통 표시 경로(원본 update88/update8c)의 연결 지점.
/// 연결하지 않으면(null) 표시 객체가 없는 경로로 보고 아무것도 하지 않는다. 표시 모듈이 생기면 구현한다.
/// </summary>
public interface IRawUnpopDisplay
{
    /// <summary>명시 결선 검사에 사용할 수 있는 표시 검사다. 공통 Unpop 본문은 선검사로 호출하지 않고 공간 쓰기 뒤 현재 Update를 선택한다.</summary>
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
/// PATCH004afe50/CD004ad0b0의 현재 공통 공간 해제를 같은 실제 Pool/Hash/점유/표시에 연결한다.
/// 원문 진단이 정상 복귀하면 현재 TYPE/상태를 다시 읽고 void·점유·옆면·해시의 이미 실행한 쓰기를 보존한다.
/// 제거 대상 자신의 next/좌표는 보존하며 표시와 실제 종속 분리는 선택된 위치에서만 요구한다.
/// CD 전체 관리풀/월드 복원과 구별하고 여기서는 판본별 부모/필드 위치만 실행한다.
/// </summary>
public sealed partial class RawUnpop : IRawUnpop
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

    /// <summary>건물 옆면 해제 몸체. 실제 표면 중간 옆면을 선택할 때만 연결을 요구한다.</summary>
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

    /// <summary>실제 프레임 연결이 같은 풀·표시·해시·지도 대여를 공유하는지 구성 때 확인한다.</summary>
    public void ValidateFrameBinding(RawSidPool pool, SquidDisplay display, RawSquidHash hash, byte[] spots)
    {
        if (!ReferenceEquals(pool, _pool) || !ReferenceEquals(display, _display) || !ReferenceEquals(hash, _hash) || !ReferenceEquals(spots, _spots))
            throw new ArgumentException("프레임 Unpop 풀·표시·공간 연결 불일치");
    }


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

    /// <summary>현재 진단/TYPE와 실제 순차 공간 해제를 공통 부모에 전달하며 앞선 효과를 되돌리지 않는다.</summary>
    /// <param name="sid">같은 실제 풀의 물리 SID다.</param>
    /// <param name="type">독립 입력의 TYPE 참조다. 현재 표를 연결하면 그 표를 소비 위치에서 고른다.</param>
    /// <param name="flags">현재 가상88/8c 선택에 쓰는 전체 flags다.</param>
    public void Unpop(int sid, OriginalTypeRecord type, uint flags = 0) => UnpopCurrent(sid, type, flags);

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
