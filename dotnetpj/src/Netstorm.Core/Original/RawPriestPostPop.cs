using System.Buffers.Binary;

// PATCH 사제의 Carrier 진입 전 목록·회복 예약·소유자 상태를 처리한다.
namespace Netstorm.Core.Original;

/// <summary>사제 회복의 실제 Regular 검색·할당 관문·부착 효과다.</summary>
public sealed class PriestPostPopHooks
{
    /// <summary>같은 부모의 실제 회복 Regular가 있는지 조회한다.</summary>
    public Func<int, uint, bool>? FindRegular { get; set; }
    /// <summary>원본 new40바이트 성공 여부를 도달 시점에 결정한다.</summary>
    public Func<bool>? ReserveRegular { get; set; }
    /// <summary>확보 성공 뒤 계산한 payload를 실제 Regular에 전달한다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }
}

/// <summary>원본004950f0의 접두를 실제 HP·공유 목록에 연결한다. 전체 가상 PostPop은 후반 조합에서 연결한다.</summary>
public sealed class RawPriestPostPop
{
    /// <summary>원본 사제 회복 이벤트 번호다.</summary>
    public const uint PriestRegenEvent = 0x25a;
    /// <summary>현재 사제TYPE의 독립 입력 기본값이다.</summary>
    public const uint InitialPriestType = 158;
    /// <summary>현재 물리 슬롯을 읽는 동일 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 TYPE·객체 최대HP를 원문 순서로 조회하는 몸체다.</summary>
    private readonly RawSquidReward _hp;
    /// <summary>등록·조회·삭제가 공유하는 실제 목록/상태다.</summary>
    private readonly PriestPostPopState _state;
    /// <summary>도달한 실제 외부 효과만 요구한다.</summary>
    private readonly PriestPostPopHooks _hooks;
    /// <summary>분자 HP를 읽을 때마다 현재 사제TYPE DWORD를 조회한다.</summary>
    private readonly Func<uint> _priestType;

    /// <summary>같은 PATCH 풀·HP·상태를 연결한다. 아직 도달하지 않은 훅은 요구하지 않는다.</summary>
    public RawPriestPostPop(RawSidPool pool, RawSquidReward hp, PriestPostPopState state,
        PriestPostPopHooks hooks, Func<uint>? priestType = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hp);
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(pool, hp.Pool))
            throw new ArgumentException("사제 후처리 실제 PATCH 풀·최대HP 계약 위반");
        _pool = pool; _hp = hp; _state = state; _hooks = hooks;
        _priestType = priestType ?? (() => InitialPriestType);
    }

    /// <summary>도달한 필수 효과가 연결되지 않았으면 임의 결과를 만들지 않고 거부한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function
        ?? throw new InvalidOperationException("사제 postPop 선택된 실제 하위 미연결");

    /// <summary>현재 signed 개수·용량으로 중복을 확인하고 최초 사용 위치에 등록한다.</summary>
    private static void AppendUnique(RawPostPopList list, int sid)
    {
        uint used = list.Count;
        if (unchecked((int)used) >= list.Entries.Length) return;
        if (unchecked((int)used) > 0)
        {
            uint[] storage = list.Entries; uint index = 0;
            // 최초 양수 개수로 들어온 뒤 매번 현재 signed 개수와 비교한다.
            do
            {
                if (index >= storage.Length) throw new ArgumentOutOfRangeException(nameof(list), "사제 목록 중복 검색 실제 범위 오류");
                if (storage[index] == (uint)sid) return;
                index++;
            } while (unchecked((int)index) < unchecked((int)list.Count));
        }
        if (unchecked((int)used) < 0 || used >= list.Entries.Length)
            throw new ArgumentOutOfRangeException(nameof(list), "사제 목록 실제 저장 범위 오류");
        list.Entries[used] = (uint)sid; list.Count = unchecked(list.Count + 1u);
    }

    /// <summary>int/6 절삭·double50배·float 저장·현재 객체HP 나눗셈 순서로 payload를 계산한다.</summary>
    private float RegenPayload(int sid)
    {
        float numerator = (float)((double)(_hp.TypeHitPoints(_priestType()) / 6) * 50.0);
        return (float)((double)numerator / _hp.MaxHitPoints(sid));
    }

    /// <summary>사전 검사 없이 현재 extra→목록→검색/확보→HP/생성→현재 owner 쓰기를 실행한다.</summary>
    public void Prefix(int sid, uint flags)
    {
        if ((_pool.Slot(sid)[40] & 9) != 0) return;
        if ((flags & 1) != 0)
        {
            AppendUnique(_state.GetPriests(), sid);
            if (!Need(_hooks.FindRegular)(sid, PriestRegenEvent) && Need(_hooks.ReserveRegular)())
            {
                float payload = RegenPayload(sid);
                Need(_hooks.AddRegular)(sid, PriestRegenEvent, payload);
            }
        }
        ReadOnlySpan<byte> current = _pool.Slot(sid); byte owner = current[34];
        if ((current[12] & 0x7f) == owner && owner > 0 && owner <= 8)
            _state.GetOwnerState()[owner] = 0;
    }

    /// <summary>공간 변경 전 명시 검사다. 실제 PATCH Prefix는 이 검사를 먼저 호출하지 않는다.</summary>
    public void Validate(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[10] != InitialPriestType || BinaryPrimitives.ReadUInt32LittleEndian(raw) != 0x0050f210u)
            throw new ArgumentException("사제 postPop 가상 표·타입 불일치");
        RawPostPopList list = _state.GetPriests();
        if (list.Count > list.Entries.Length) throw new ArgumentException("사제 목록 개수 오류");
    }

    /// <summary>전체 PostPop 조합이 같은 풀의 접두인지 확인하는 참조다.</summary>
    public RawSidPool Pool() => _pool;

    /// <summary>원본 ProcessHost 어댑터처럼 실제 검색·생성에 연결한다. 관리 객체 할당 실패는 AddRegular에서 전파한다.</summary>
    public static PriestPostPopHooks MakePriestPostPopProcessHooks(SquidProcessHost host)
    {
        ArgumentNullException.ThrowIfNull(host);
        return new PriestPostPopHooks
        {
            FindRegular = (sid, ev) => host.FindEvent(sid, ev) is not null,
            ReserveRegular = () => true,
            AddRegular = (sid, ev, payload) => host.AddRegular(sid, ev, payload),
        };
    }

    /// <summary>유효1..8 한DWORD만 지우며 무효 입력은 실제 진단 뒤 쓰기를 생략한다.</summary>
    public static void ResetPriestOwnerState(uint[] states, uint player, Action<uint>? assertion = null)
    {
        ArgumentNullException.ThrowIfNull(states);
        if (states.Length != 9) throw new ArgumentException("사제 소유자 상태 실제9DWORD 누락", nameof(states));
        if (player < 1 || player > 8) { Need(assertion)(player); return; }
        states[player] = 0;
    }
}
