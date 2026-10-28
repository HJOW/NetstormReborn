namespace Netstorm.Core.Original;

/// <summary>
/// 원본 TYPE+1bc와 118의 현재 기도 속성이다.
/// 기도 완료 횟수와 완료 뒤 현재 사제의 기본 프레임을 담는다.
/// 근거: cpppj/src/o/RawPriestPrayer.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class PriestPrayerType
{
    /// <summary>기도 완료 횟수.</summary>
    public int PrayTime { get; set; }

    /// <summary>완료 뒤 현재 사제의 기본 프레임.</summary>
    public int DefaultFrame { get; set; }
}

/// <summary>
/// 사제 기도 명령·수신·진행의 외부 동작 연결(원본 <c>PriestPrayerHooks</c>).
/// 해당 분기에서 선택된 큰 하위만 요구하며 원본의 재조회 순서를 유지한다.
/// 도달한 실제 하위가 없으면 명시 오류로 멈춘다.
/// 근거: cpppj/src/o/RawPriestPrayer.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class PriestPrayerHooks
{
    /// <summary>현재 첫 605 Regular의 존재를 조회한다. null이면 도달할 때 거부한다.</summary>
    public Func<int, uint, bool>? HasEvent { get; set; }

    /// <summary>현재 프레임 방향 0..15 또는 -1(원본 4ad710). null이면 도달할 때 거부한다.</summary>
    public Func<int, int>? Direction { get; set; }

    /// <summary>현재 TYPE에 따른 이동 프로세스 종류(원본 488fe0). null이면 도달할 때 거부한다.</summary>
    public Func<uint, uint>? MovementType { get; set; }

    /// <summary>첫 Form 조회 또는 실제 가상 삭제(원본 4b1a90). null이면 도달할 때 거부한다.</summary>
    public Func<int, uint, uint, bool, int>? Movement { get; set; }

    /// <summary>실제 요청 전송(원본 4e2dc0). null이면 도달할 때 거부한다.</summary>
    public Action<int, byte[]>? Request { get; set; }

    /// <summary>boss가 이동 Form을 제거한 뒤 실행(원본 4e2e30). null이면 도달할 때 거부한다.</summary>
    public Action<byte[]>? Broadcast { get; set; }

    /// <summary>원본 new 0x28 한 번의 성공·실패. null이면 도달할 때 거부한다.</summary>
    public Func<bool>? ReserveShared { get; set; }

    /// <summary>확보한 같은 객체의 605·payload1 SharedRegular 생성. null이면 도달할 때 거부한다.</summary>
    public Action<int, uint, float>? AddShared { get; set; }

    /// <summary>현재 프레임·공유 예약(원본 491cf0). null이면 도달할 때 거부한다.</summary>
    public Action<int, uint>? Ritual { get; set; }

    /// <summary>현재 마비·사제 HP·공간 검사(원본 427030). null이면 도달할 때 거부한다.</summary>
    public Func<int, bool>? Immobile { get; set; }

    /// <summary>첫 mask1 내용물 종류 조회(원본 4acac0). null이면 도달할 때 거부한다.</summary>
    public Func<int, uint, uint, uint, uint>? ContainedType { get; set; }

    /// <summary>현재 첫 mask1 내용물 삭제와 필요한 너겟 보충(원본 4264d0). null이면 도달할 때 거부한다.</summary>
    public Action<int>? Discard { get; set; }

    /// <summary>현재 희망 종류 생성·부착(원본 4afa70). null이면 도달할 때 거부한다.</summary>
    public Action<int, uint, uint, uint, uint, uint>? Create { get; set; }

    /// <summary>현재 사제 TYPE의 기본 프레임과 flags 0. null이면 도달할 때 거부한다.</summary>
    public Action<int, int, uint>? SetFrame { get; set; }

    /// <summary>다른 명령 0x03000103의 491db0이며 미복원일 때 성공으로 대신하지 않는다. null이면 도달할 때 거부한다.</summary>
    public Action<int>? Panic { get; set; }

    /// <summary>같은 코드·원래 사건 y의 위임(원본 42aa50). null이면 도달할 때 거부한다.</summary>
    public Func<int, uint, uint, uint>? Parent { get; set; }
}

/// <summary>
/// 사제 기도 4940b0 입력·493f80 수신·494580 605 진행을 같은 현재 풀과 권한에 연결한다(원본 <c>RawPriestPrayer</c>).
/// 10.78의 기도 세 경로를 복원하며 나머지 사제 Regular를 대신 처리하지 않는다.
/// 권한은 호출 시점의 현재 값을 읽으므로 세션·검사가 빌린 bool을 바꾸면 다음 분기에 반영된다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawPriestPrayer.cpp (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class RawPriestPrayer
{
    /// <summary>기도 605 Regular 번호.</summary>
    private const uint Prayer = 605;

    /// <summary>파괴 주문 종류 번호.</summary>
    private const uint Devastation = 168;

    /// <summary>기도 명령 코드.</summary>
    private const uint PrayCommand = 0x030000deu;

    /// <summary>panic 명령 코드.</summary>
    private const uint PanicCommand = 0x03000103u;

    /// <summary>명령 비교에서 제외하는 상위 2비트 마스크.</summary>
    private const uint CommandMask = 0x03ffffffu;

    /// <summary>같은 실제 사제·내용물·Form을 보관하는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 TYPE의 기도 속성 표.</summary>
    private readonly IReadOnlyList<PriestPrayerType> _types;

    /// <summary>각 원본 분기 위치에서 읽는 현재 권한.</summary>
    private readonly Func<bool> _boss;

    /// <summary>실제 전송·이동·공유예약·의식·내용물 소비자.</summary>
    private readonly PriestPrayerHooks _hooks;

    /// <summary>
    /// 사제 기도 처리기를 만든다. 풀·타입 표·권한은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">같은 실제 사제·내용물·Form을 보관하는 풀. 10.78 풀만 받는다.</param>
    /// <param name="types">현재 TYPE의 기도 속성 표 188개.</param>
    /// <param name="boss">각 분기에서 읽는 현재 권한. null이면 거부한다.</param>
    /// <param name="hooks">실제 소비자. null이면 빈 묶음을 쓴다.</param>
    public RawPriestPrayer(RawSidPool pool, IReadOnlyList<PriestPrayerType> types, Func<bool> boss, PriestPrayerHooks? hooks = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(boss);
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"사제 기도 TYPE 표가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _types = types;
        _boss = boss;
        _hooks = hooks ?? new PriestPrayerHooks();
    }

    /// <summary>
    /// packed y의 상위 WORD 종류를 6d 요청으로 보낸다. 원본 입력의 상위 6개 비트는 명령 비교에서 제외한다.
    /// </summary>
    /// <param name="parent">대상 부모 슬롯 번호.</param>
    /// <param name="code">원본 명령 코드.</param>
    /// <param name="parameter">packed y.</param>
    public uint Command(int parent, uint code, uint parameter)
    {
        uint command = code & CommandMask;
        if (command == PrayCommand)
        {
            uint kind = parameter >> 16;
            // 원본 조회 전에 원본이 읽는 사건+10 WORD를 보존한다
            if (!Need(_hooks.HasEvent, "사제 기도의 실제 하위 미연결")(parent, Prayer))
            {
                int direction = Need(_hooks.Direction, "사제 기도의 실제 하위 미연결")(parent);
                if (direction < 10 || direction > 14)
                {
                    byte[] packet =
                    [
                        1, 0x6d, 8, 0,
                        (byte)parent, (byte)(parent >> 8),
                        (byte)kind, (byte)(kind >> 8),
                    ];
                    Need(_hooks.Request, "사제 기도의 실제 하위 미연결")(9, packet);
                }
            }
            return 1;
        }
        if (command == PanicCommand)
        {
            Need(_hooks.Panic, "사제 기도의 실제 하위 미연결")(parent);
        }
        return Need(_hooks.Parent, "사제 기도의 실제 하위 미연결")(parent, code, parameter);
    }

    /// <summary>
    /// 희망 종류 쓰기→상태·605·방향→boss 이동 제거·배포→605·604다.
    /// </summary>
    /// <param name="packet">수신 패킷. 8바이트 뒤 추가 자료는 읽지 않는다.</param>
    public void Receive(byte[] packet)
    {
        ArgumentNullException.ThrowIfNull(packet);
        if (packet.Length < 8)
        {
            throw new ArgumentException("기도 6d 부모·종류 WORD 부족", nameof(packet));
        }
        int parent = packet[4] | (packet[5] << 8);
        uint kind = (uint)packet[6] | ((uint)packet[7] << 8);
        Write(parent, 42, kind);
        if ((Read(parent, 11, 1) & 3u) != 0)
        {
            return;
        }
        if (Need(_hooks.HasEvent, "사제 기도의 실제 하위 미연결")(parent, Prayer))
        {
            return;
        }
        int direction = Need(_hooks.Direction, "사제 기도의 실제 하위 미연결")(parent);
        if (direction >= 10 && direction <= 14)
        {
            return;
        }
        if (_boss())
        {
            uint movement = Need(_hooks.MovementType, "사제 기도의 실제 하위 미연결")(Read(parent, 10, 1));
            Need(_hooks.Movement, "사제 기도의 실제 하위 미연결")(parent, movement, 0, true);
            Need(_hooks.Broadcast, "사제 기도의 실제 하위 미연결")(packet);
        }
        if (Need(_hooks.ReserveShared, "사제 기도의 실제 하위 미연결")())
        {
            Need(_hooks.AddShared, "사제 기도의 실제 하위 미연결")(parent, Prayer, 1.0f);
        }
        Need(_hooks.Ritual, "사제 기도의 실제 하위 미연결")(parent, 4);
    }

    /// <summary>
    /// 현재 count 쓰기→이동·정지→정확한 praytime 일치→내용물·기본 프레임이다.
    /// </summary>
    /// <param name="parent">대상 부모 슬롯 번호.</param>
    /// <param name="count">쓸 count.</param>
    /// <param name="payload">반환 payload.</param>
    public float Tick(int parent, uint count, float payload)
    {
        Write(parent, 46, count);
        uint movementType = Need(_hooks.MovementType, "사제 기도의 실제 하위 미연결")(Read(parent, 10, 1));
        int movement = Need(_hooks.Movement, "사제 기도의 실제 하위 미연결")(parent, movementType, 0, false);
        if (Need(_hooks.Immobile, "사제 기도의 실제 하위 미연결")(parent) || movement != 0)
        {
            return 0;
        }
        if (unchecked((int)count) != Type(Read(parent, 42)).PrayTime)
        {
            return payload;
        }
        if (_boss() && Need(_hooks.ContainedType, "사제 기도의 실제 하위 미연결")(parent, 1, Devastation, 0) == 0)
        {
            Need(_hooks.Discard, "사제 기도의 실제 하위 미연결")(parent);
            Need(_hooks.Create, "사제 기도의 실제 하위 미연결")(parent, Read(parent, 42), 0, 1, 0, 0);
        }
        Need(_hooks.SetFrame, "사제 기도의 실제 하위 미연결")(parent, Type(Read(parent, 10, 1)).DefaultFrame, 0);
        return 0;
    }

    /// <summary>현재 물리 슬롯의 little endian 필드다.</summary>
    /// <param name="parent">대상 부모 슬롯 번호.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private uint Read(int parent, int offset, int width = 4)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(parent);
        uint result = 0;
        // 요청한 원문 폭만 읽는다
        for (int i = 0; i < width; i++)
        {
            result |= (uint)raw[offset + i] << (8 * i);
        }
        return result;
    }

    /// <summary>현재 DWORD만 쓴다. 예약·free 슬롯에도 원본처럼 현재 DWORD를 쓴다.</summary>
    /// <param name="parent">대상 부모 슬롯 번호.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 DWORD 값.</param>
    private void Write(int parent, int offset, uint value)
    {
        Span<byte> raw = _pool.RawSlot(parent);
        // 정확히 현재 DWORD만 쓴다
        for (int i = 0; i < 4; i++)
        {
            raw[offset + i] = (byte)(value >> (8 * i));
        }
    }

    /// <summary>현재 TYPE 표 밖 손상 입력은 명시 오류다.</summary>
    /// <param name="kind">조회할 종류 번호.</param>
    private PriestPrayerType Type(uint kind)
    {
        if (kind >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(kind), kind, "사제 기도 현재 TYPE 범위");
        }
        return _types[(int)kind];
    }

    /// <summary>선택되지 않은 경계는 요구하지 않으며 선택한 누락만 오류로 남긴다.</summary>
    /// <typeparam name="T">장치 델리게이트 형태.</typeparam>
    /// <param name="hook">요구할 장치. null이면 거부한다.</param>
    /// <param name="message">거부할 때의 진단 문구.</param>
    private static T Need<T>(T? hook, string message) where T : class
    {
        if (hook is null)
        {
            throw new InvalidOperationException(message);
        }
        return hook;
    }
}
