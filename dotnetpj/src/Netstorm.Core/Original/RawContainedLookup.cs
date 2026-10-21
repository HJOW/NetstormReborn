namespace Netstorm.Core.Original;

/// <summary>
/// 타입과 순번으로 종속 객체를 찾거나 그 객체의 가상 삭제를 요청한다(원본 RawContainedLookup, 004b1a90 ↔ CD 004eb310).
/// 범용 내용물 커서와 달리 다음 링크를 읽는 시점이 다르다(삭제 뒤 현재 슬롯에서 읽는다).
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawContainedLookup.cpp (W03 삭제·수명 열째 조각, 2026-10-21).
/// </summary>
public sealed class RawContainedLookup
{
    /// <summary>슬롯 안 종속 다음 번호(16비트)의 위치.</summary>
    private const int NextOffset = 4;

    /// <summary>슬롯 안 종속 머리(16비트)의 위치.</summary>
    private const int HeadOffset = 6;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>상태 바이트의 죽음 비트.</summary>
    private const byte DeadBit = 2;

    /// <summary>일반 client 번호의 첫 번호.</summary>
    private const int FirstOrdinary = 5;

    /// <summary>현재 raw 종속 체인과 서버 권한을 공유하는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>선택한 객체의 가상 삭제 경계.</summary>
    private readonly Action<int, uint> _destroy;

    /// <summary>
    /// 종속 조회 몸체를 만든다. 삭제 경계가 없으면 만들 수 없다.
    /// </summary>
    /// <param name="pool">종속 체인을 가진 풀.</param>
    /// <param name="destroy">가상 삭제 경계. 같은 풀의 raw 를 바꿀 수 있다.</param>
    public RawContainedLookup(RawSidPool pool, Action<int, uint> destroy)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(destroy);
        _pool = pool;
        _destroy = destroy;
    }

    /// <summary>어댑터 연결 확인용 읽기 전용 참조다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 부모의 head WORD 부터 타입이 같고 죽지 않은 항목 가운데 ordinal 번째(0부터)를 찾는다.
    /// 제거를 요청하면 서버이거나 일반 client 번호일 때만 플래그 0 으로 삭제하고 계속 진행해 0 을 돌려준다.
    /// </summary>
    /// <param name="parent">부모 슬롯 번호.</param>
    /// <param name="type">찾을 타입 번호.</param>
    /// <param name="ordinal">앞에서 건너뛸 일치 개수.</param>
    /// <param name="remove">찾은 항목을 삭제할지.</param>
    /// <returns>찾은 번호. 없으면 0.</returns>
    public int Find(int parent, uint type, uint ordinal = 0, bool remove = false)
    {
        int current = Link(_pool.Slot(parent), HeadOffset);
        var visited = new HashSet<int>();
        // 삭제가 슬롯을 반납하거나 next 를 바꿀 수 있으므로 다음 링크는 효과가 끝난 뒤 읽는다.
        while (current != 0)
        {
            if (current >= _pool.Capacity || !visited.Add(current))
            {
                throw new InvalidOperationException("contained 조회 종속 체인 손상");
            }
            ReadOnlySpan<byte> raw = _pool.Slot(current);
            if (raw[TypeOffset] == type && (raw[StateOffset] & DeadBit) == 0)
            {
                if (ordinal == 0)
                {
                    if (!remove)
                    {
                        return current;
                    }
                    if (_pool.IsServer || (current >= FirstOrdinary && current < RawSidPool.ServerFirst))
                    {
                        _destroy(current, 0);
                    }
                }
                // 순번은 DWORD 로 감소하며 0 에서 감긴다.
                ordinal = unchecked(ordinal - 1);
            }
            current = Link(_pool.Slot(current), NextOffset);
        }
        return 0;
    }

    /// <summary>정렬되지 않은 링크 16비트 값을 낮은 바이트부터 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static int Link(ReadOnlySpan<byte> raw, int offset) => raw[offset] | (raw[offset + 1] << 8);
}
