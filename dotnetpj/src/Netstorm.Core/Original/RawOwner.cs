namespace Netstorm.Core.Original;

/// <summary>
/// 소유자 지정이 보는 기본 모드 영역(원본 SquidOwnerMode).
/// </summary>
public sealed class SquidOwnerMode
{
    /// <summary>현재(편집) 모드. 켜져 있으면 작업장 목록을 고친다.</summary>
    public bool Fort { get; set; }

    /// <summary>전투 모드. 편집 모드와 같이 켜져 있어야 작업장 목록을 고친다.</summary>
    public bool Battle { get; set; }

    /// <summary>도전 모드. 켜지면 유효 소유자 번호가 1~39로 바뀐다(원본 CL_NUM_CHAL_PLAYERS).</summary>
    public bool Challenge { get; set; }
}

/// <summary>
/// 소유자 변경을 객체 슬롯과 소유자별 작업장 목록에 반영하는 처리기(원본 SquidOwner, 10.78).
/// 편집·전투 모드에서 신전·공장 부류의 이전 소유자 목록에서 번호를 빼고,
/// 빈·매립이 아닌 새 소유자의 목록에 자리가 차면 생략한다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 소유자·추가 바이트 위치가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/SquidOwner.cpp (W02 Owner 조각, 2026-10-20).
/// </summary>
public sealed class SquidOwner
{
    /// <summary>작업장 목록 부류 마스크(소용돌이·공장, 원본 0x4200).</summary>
    private const uint FactoryMask = 0x4200;

    /// <summary>도전 모드에서 유효한 소유자 번호의 상한(원본 CL_NUM_CHAL_PLAYERS = 0x28). 1~39가 유효하다.</summary>
    private const uint ChallengePlayers = 0x28;

    /// <summary>상태 바이트의 빈(void) 비트. 켜져 있으면 새 소유자의 목록에 넣지 않는다.</summary>
    private const byte VoidBit = 4;

    /// <summary>추가 비트의 매립(buried) 비트. 켜져 있으면 새 소유자의 목록에 넣지 않는다.</summary>
    private const byte BuriedBit = 8;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 소유자 바이트의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>슬롯을 읽고 쓸 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 → 타입 레코드. 작업장 부류를 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>객체 종류·소유자별 등록 목록을 관리하는 장부. 호출자가 소유하며 이 클래스가 직접 바꾼다.</summary>
    private readonly RawPostPopState _bookkeeping;

    /// <summary>소유자 지정이 보는 기본 모드 영역.</summary>
    private readonly SquidOwnerMode _mode;

    /// <summary>
    /// 풀·타입·장부·모드를 받아 소유자 지정을 만든다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="bookkeeping">소유자별 작업장 목록을 가진 장부.</param>
    /// <param name="mode">기본 모드 영역.</param>
    public SquidOwner(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPostPopState bookkeeping, SquidOwnerMode mode)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(bookkeeping);
        ArgumentNullException.ThrowIfNull(mode);
        // CD 10.72(171개)는 소유자·추가 바이트 위치가 다르므로 받지 않는다
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _types = types;
        _bookkeeping = bookkeeping;
        _mode = mode;
    }

    /// <summary>이 모듈이 쓰는 슬롯 풀. 같은 풀의 목록인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 객체의 소유자 바이트를 바꾸고 작업장 목록을 갱신한다.
    /// 목록에 드는 부류가 아니면 소유자 바이트만 바꾼다. 소유자 0은 목록에 들지 않는다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="player">새 소유자 번호. 일반 모드 0~8, 도전 모드 1~39다.</param>
    public void Set(int sid, uint player)
    {
        // 판본 assert: 일반 모드의 범위는 목록 개수(0~8)이며 도전 모드에서는 1~39다
        bool valid = _mode.Challenge ? player > 0 && player < ChallengePlayers : player < (uint)_bookkeeping.OwnerFactories.Length;
        if (!valid)
        {
            throw new ArgumentException("소유자 번호 범위 오류(판본 assert)", nameof(player));
        }
        // 목록이 드는 번호는 0~8이다. 도전 모드의 9~39는 목록에 넣지 않는다
        if (player >= (uint)_bookkeeping.OwnerFactories.Length)
        {
            throw new ArgumentException("목록이 없는 소유자 번호는 지정하지 않는다", nameof(player));
        }
        Span<byte> raw = _pool.AllocatedBytes(sid);
        uint type = raw[TypeOffset];
        if (type >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), type, "소유자 지정 타입 번호 범위 오류");
        }
        bool listed = (_mode.Fort || _mode.Battle) && (_types[(int)type].Flags2 & FactoryMask) != 0;
        // 이전 소유자의 목록에서 번호를 뺀다. 소유자 0은 목록에 없다
        if (listed)
        {
            byte previous = raw[OwnerOffset];
            if (previous != 0)
            {
                if (previous >= _bookkeeping.OwnerFactories.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(sid), previous, "이전 소유자 번호 범위 오류");
                }
                Remove(_bookkeeping.OwnerFactories[previous], sid);
            }
        }
        raw[OwnerOffset] = (byte)player;
        // 빈·매립이 아니면 새 소유자의 목록에 넣는다. 소유자 0은 넣지 않는다
        if (listed && (raw[StateOffset] & VoidBit) == 0 && (raw[ExtraOffset] & BuriedBit) == 0 && player != 0)
        {
            Append(_bookkeeping.OwnerFactories[player], sid);
        }
    }

    /// <summary>원본 00490050: 목록에 자리가 남았을 때만 번호를 덧붙인다.</summary>
    /// <param name="list">대상 목록.</param>
    /// <param name="sid">덧붙일 번호.</param>
    private static void Append(RawPostPopList list, int sid)
    {
        // 자리가 남았을 때만 쓰고 개수를 늘린다
        if (list.Count < (uint)list.Entries.Length)
        {
            list.Entries[(int)list.Count++] = (uint)sid;
        }
    }

    /// <summary>원본 004900a0: 같은 번호를 모두 빼고 나머지를 앞으로 당긴다. 개수 밖의 값은 건드리지 않는다.</summary>
    /// <param name="list">대상 목록.</param>
    /// <param name="sid">뺄 번호.</param>
    private static void Remove(RawPostPopList list, int sid)
    {
        // 손상된 개수는 목록 쓰기 전에 거부한다
        _ = list.Items();
        uint removed = 0;
        // 남는 항목을 앞에서부터 당긴다
        for (uint i = 0; i < list.Count; i++)
        {
            uint value = list.Entries[i];
            if (value == (uint)sid)
            {
                removed++;
            }
            else
            {
                list.Entries[i - removed] = value;
            }
        }
        list.Count -= removed;
    }
}
