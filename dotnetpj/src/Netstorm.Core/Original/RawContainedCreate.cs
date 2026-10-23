using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 contained/bridge 타입 전역(원본 ContainedCreateState). 외부 효과가 바꾼 값은 사용 시점에 다시 읽는다.
/// </summary>
public sealed class ContainedCreateState
{
    /// <summary>내용물(contained Form) 타입 번호. 원본 생성자는 이 타입만 받는다.</summary>
    public uint ContainedType { get; set; } = 6;

    /// <summary>다리 자산을 타입 표에서 찾는 번호. 이 타입이면 표면 통지를 부른다.</summary>
    public uint BridgeType { get; set; } = 82;
}

/// <summary>
/// contained 생성이 바깥에 요구하는 경계 묶음(원본 ContainedCreateHooks). 세 경계는 모두 필수다.
/// </summary>
public sealed class ContainedCreateHooks
{
    /// <summary>타입과 생성 플래그(1 예측·2 클라이언트·0 서버)를 받아 새 대상을 만드는 콜백.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>표면 객체에 변경을 통지하는 콜백(다리 타입일 때만 부른다).</summary>
    public Action<int>? NotifySurface { get; set; }

    /// <summary>대상을 부모(또는 처리 목록)에 부착하는 콜백(Form, 부모, 부착 플래그).</summary>
    public Action<int, int, uint>? Attach { get; set; }
}

/// <summary>
/// 생산 칸 등에 넣는 contained Form 생성(원본 Squid.cpp 004afa70 / CD 004ac4d0, RawContainedCreate).
/// 순서는 원본 그대로 "부모 검사 → flags 기본값 0x20 → 생성 → 저장 타입·인자 쓰기 → 다리 표면 통지 →
/// 품질 BYTE → 목록 플래그 WORD → 부착" 이다. 생성 콜백이 슬롯을 바꿀 수 있으므로 필드는 매번 다시 받는다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 품질 BYTE 위치가 +26 이고 플래그보다 먼저 쓰지만 옮기지 않았다).
/// 근거: cpppj/src/o/RawContainedCreate.cpp (W03 내용물 생성 조각, 2026-10-23).
/// </summary>
public sealed class RawContainedCreate
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치. 다리 타입 판정에 쓴다.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 생성 인자(16비트)의 위치.</summary>
    private const int ArgumentOffset = 12;

    /// <summary>슬롯 안 저장 내용물 타입(16비트)의 위치.</summary>
    private const int StoredTypeOffset = 18;

    /// <summary>슬롯 안 목록 플래그(16비트)의 위치.</summary>
    private const int FlagsOffset = 20;

    /// <summary>10.78 목록 품질 BYTE 의 위치. 플래그 WORD 뒤에 쓴다.</summary>
    private const int QualityOffset = 30;

    /// <summary>일반 번호의 첫 값. 이보다 작은 번호는 생성 결과로 받지 않는다.</summary>
    private const int FirstOrdinary = 5;

    /// <summary>contained 생성이 팩토리에 요구하는 타입 번호.</summary>
    private const uint ContainedType = 6;

    /// <summary>flags 가 0 일 때 원본이 쓰는 기본 목록 플래그.</summary>
    private const uint DefaultFlags = 0x20;

    /// <summary>부착 플래그의 예측 생성 비트. 클라이언트 비트보다 우선한다.</summary>
    private const uint PredictableAttach = 0x20;

    /// <summary>부착 플래그의 클라이언트 생성 비트.</summary>
    private const uint ClientAttach = 0x10;

    /// <summary>내용물 슬롯을 할당·조회하는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>원본 contained/bridge 타입 전역.</summary>
    private readonly ContainedCreateState _state;

    /// <summary>생성·표면 통지·부착 경계. 생성자에서 null 을 거부한 뒤 값으로 보관한다.</summary>
    private readonly Func<uint, uint, int> _create;

    /// <summary>다리 타입일 때 부르는 표면 통지 경계.</summary>
    private readonly Action<int> _notifySurface;

    /// <summary>생성 뒤 부모에 부착하는 경계.</summary>
    private readonly Action<int, int, uint> _attach;

    /// <summary>
    /// contained 생성기를 만든다. 풀·상태는 이 객체보다 오래 살아야 하며 세 경계가 모두 있어야 한다.
    /// </summary>
    /// <param name="pool">내용물 슬롯을 가진 풀.</param>
    /// <param name="state">contained/bridge 타입 전역.</param>
    /// <param name="hooks">생성·표면 통지·부착 경계.</param>
    public RawContainedCreate(RawSidPool pool, ContainedCreateState state, ContainedCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        _create = hooks.Create ?? throw new ArgumentException("contained 생성 경계 누락", nameof(hooks));
        _notifySurface = hooks.NotifySurface ?? throw new ArgumentException("contained 표면 통지 경계 누락", nameof(hooks));
        _attach = hooks.Attach ?? throw new ArgumentException("contained 부착 경계 누락", nameof(hooks));
        _pool = pool;
        _state = state;
    }

    /// <summary>다른 어댑터가 동일한 풀에 연결되는지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 부모의 생산 칸에 넣을 내용물을 만들고 새 번호를 돌려준다.
    /// 부착 플래그 0x20 은 예측 생성, 0x10 은 클라이언트 생성이며 0x20 이 우선한다.
    /// flags 가 DWORD 전체로 0 일 때만 0x20 을 기본값으로 쓰고, 슬롯에는 하위 WORD/BYTE 만 쓴다.
    /// </summary>
    /// <param name="parent">내용물을 담을 부모 번호. 0 이거나 범위 밖이면 거부한다.</param>
    /// <param name="type">저장할 내용물 타입(슬롯 +18).</param>
    /// <param name="argument">생성 인자(슬롯 +12).</param>
    /// <param name="flags">목록 플래그(슬롯 +20). 0 이면 0x20.</param>
    /// <param name="attachFlags">부착 플래그. 생성 영역을 정한다.</param>
    /// <param name="quality">목록 품질 BYTE(슬롯 +30). 하위 8비트만 쓴다.</param>
    public int Create(int parent, uint type, uint argument, uint flags, uint attachFlags, uint quality)
    {
        if (parent == 0 || parent >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(parent), parent, "contained 부모 SID 범위 오류");
        }
        if (flags == 0)
        {
            flags = DefaultFlags;
        }
        uint factoryFlags = (attachFlags & PredictableAttach) != 0 ? 1u
            : (attachFlags & ClientAttach) != 0 ? 2u : 0u;
        int sid = _create(_state.ContainedType, factoryFlags);
        if (sid < FirstOrdinary || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "contained 생성 SID 범위 오류");
        }
        Span<byte> raw = _pool.AllocatedBytes(sid);
        PutWord(raw, StoredTypeOffset, type);
        PutWord(raw, ArgumentOffset, argument);
        // 다리 타입은 지금 슬롯의 타입 바이트로 판정한다
        if (raw[TypeOffset] == _state.BridgeType)
        {
            _notifySurface(sid);
        }
        // 통지 콜백이 슬롯을 바꿨을 수 있으므로 필드 쓰기 전에 다시 받는다
        raw = _pool.AllocatedBytes(sid);
        raw[QualityOffset] = (byte)quality;
        PutWord(raw, FlagsOffset, flags);
        _attach(sid, parent, attachFlags);
        return sid;
    }

    /// <summary>16비트 값을 리틀 엔디언으로 써서 주변 payload 를 보존한다.</summary>
    /// <param name="raw">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값. 하위 16비트만 쓴다.</param>
    private static void PutWord(Span<byte> raw, int offset, uint value)
    {
        raw[offset] = (byte)value;
        raw[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>
    /// 실제 팩토리의 contained 전용 생성자를 연결한다(원본 MakeContainedCreateHooks).
    /// 다른 풀의 팩토리는 쓰기 전에 거부하며, 생성 타입이 저장 전역과 다르면 예외다.
    /// </summary>
    /// <param name="pool">생성기가 쓰는 풀.</param>
    /// <param name="factory">생성 몸체. 같은 풀이어야 한다.</param>
    /// <param name="hooks">표면 통지·부착 경계를 채운 묶음.</param>
    public static ContainedCreateHooks ConnectFactory(RawSidPool pool, RawSquidFactory factory, ContainedCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(factory);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(factory.Pool, pool))
        {
            throw new ArgumentException("contained 생성/factory SID 풀이 다릅니다", nameof(factory));
        }
        hooks.Create = (type, flags) =>
        {
            if (type != ContainedType)
            {
                throw new ArgumentException("contained 전용 생성 타입은 6이어야 합니다", nameof(type));
            }
            return factory.CreateContained(flags);
        };
        return hooks;
    }
}
