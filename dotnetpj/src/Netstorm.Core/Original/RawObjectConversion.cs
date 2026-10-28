namespace Netstorm.Core.Original;

/// <summary>
/// 같은 객체/SID의 비석·주문 전환을 실제 공간·타입 변경 하위와 조립하는 장치 연결(원본 <c>ObjectConversionHooks</c>).
/// 현재 가상 48/4c의 flags 0 공간 해제·재등록, 실제 4b1620 타입 변경, 현재 타입의 기본 프레임,
/// raw+36 직접 DWORD와 QA WORD 쓰기, 미처리 명령의 실제 부모 입력을 연결한다.
/// 도달한 효과가 없으면 앞선 쓰기를 보존하고 중단한다.
/// 근거: cpppj/src/o/RawObjectConversion.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class ObjectConversionHooks
{
    /// <summary>현재 가상 48/4c의 flags 0 공간 해제. null이면 도달할 때 거부한다.</summary>
    public Action<int, uint>? Unpop { get; set; }

    /// <summary>현재 가상 48/4c의 flags 0 공간 재등록. null이면 도달할 때 거부한다.</summary>
    public Action<int, uint>? Repop { get; set; }

    /// <summary>실제 4b1620 타입 변경. 재할당·초기화를 대신하지 않는다. null이면 도달할 때 거부한다.</summary>
    public Action<int, uint>? Retype { get; set; }

    /// <summary>현재 타입의 기본 프레임(타입+118의 전체 DWORD). null이면 도달할 때 거부한다.</summary>
    public Func<uint, uint>? DefaultFrame { get; set; }

    /// <summary>raw+36 직접 DWORD 쓰기. null이면 도달할 때 거부한다.</summary>
    public Action<int, uint>? Frame { get; set; }

    /// <summary>QA WORD 쓰기(원본 4ac220). null이면 도달할 때 거부한다.</summary>
    public Action<int, uint>? Qa { get; set; }

    /// <summary>미처리 명령의 실제 부모 입력(원본 4ae010). null이면 도달할 때 거부한다.</summary>
    public Func<int, uint, uint>? Base { get; set; }
}

/// <summary>
/// 비석·주문 전환 명령 처리기(원본 <c>RawObjectConversion</c>).
/// 실제 가상 6c가 41e400/462290인 몸체만 실행하고 그 부모 반환을 보존한다.
/// 0번·예약 슬롯도 원문 범위 안이면 읽으며 free/dead 여부는 선택한 하위 처리에 맡긴다.
/// 비석 타입은 장기 주소를 빌리는 대신 값을 받으며(R4 세션 조립 때 공유로 바꾼다), C# 풀은 패치판 범위이므로 판본 검사는 생략한다.
/// 근거: cpppj/src/o/RawObjectConversion.cpp (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class RawObjectConversion
{
    /// <summary>원본 6c가 41e400인 생성자 표. 현재 실제 표에 처음 일치하면 끝낸다.</summary>
    private static readonly uint[] SpellTables =
    [
        0x502370u, 0x502448u, 0x502510u, 0x5025d8u, 0x5026a0u, 0x502778u, 0x502840u, 0x502908u,
        0x5029d0u, 0x502aa8u, 0x502b70u, 0x502e68u, 0x502f30u, 0x502ff8u, 0x5030c0u, 0x503188u,
    ];

    /// <summary>비석 몸체의 실제 가상 6c 값.</summary>
    private const uint ObeliskBody = 0x462290u;

    /// <summary>주문 몸체의 실제 가상 6c 값.</summary>
    private const uint SpellBody = 0x41e400u;

    /// <summary>전환 명령 코드(상위 입력 비트 제거 뒤). 이와 다르면 같은 부모로 전달한다.</summary>
    private const uint ConversionCode = 0x030000ceu;

    /// <summary>상위 입력 비트 마스크. 원문 Command의 상위 입력 비트를 제거한다.</summary>
    private const uint CodeMask = 0x03ffffffu;

    /// <summary>실제 현재 타입·QA를 읽는 유일한 저장소.</summary>
    private readonly RawSidPool _pool;

    /// <summary>주문 Unpop 뒤 읽는 현재 비석 타입.</summary>
    private readonly uint _buriedType;

    /// <summary>선택된 실제 하위만 요구하는 장치 경계.</summary>
    private readonly ObjectConversionHooks _hooks;

    /// <summary>
    /// 객체 전환 처리기를 만든다. 풀과 비석 타입은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">현재 타입·QA를 읽는 슬롯 풀.</param>
    /// <param name="buriedType">주문 Unpop 뒤 읽는 현재 비석 타입.</param>
    /// <param name="hooks">선택된 실제 하위 장치. null이면 빈 묶음을 쓴다.</param>
    public RawObjectConversion(RawSidPool pool, uint buriedType, ObjectConversionHooks? hooks = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        _pool = pool;
        _buriedType = buriedType;
        _hooks = hooks ?? new ObjectConversionHooks();
    }

    /// <summary>실제 가상 6c가 41e400/462290인 표만 반환하며 다른 클래스는 0이다. 모체·자산 이름이나 Bomb 종류만으로 다른 원문 가상 입력을 합치지 않는다.</summary>
    /// <param name="vtable">판별할 가상 6c 값.</param>
    public static uint ObjectConversionBody(uint vtable)
    {
        if (vtable == 0x50a748u)
        {
            return ObeliskBody;
        }
        // 현재 실제 표에 처음 일치하면 끝낸다
        foreach (uint table in SpellTables)
        {
            if (table == vtable)
            {
                return SpellBody;
            }
        }
        return 0;
    }

    /// <summary>
    /// 선택한 실제 6c 몸체를 실행하고 그 부모 반환을 보존한다. 두 원문은 release를 별도 거부하지 않으며 미처리 명령은 같은 부모로 전달한다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="code">원문 명령 코드.</param>
    /// <param name="body">실제 가상 6c 몸체(41e400/462290).</param>
    public uint Command(int sid, uint code, uint body)
    {
        if (body != ObeliskBody && body != SpellBody)
        {
            throw new InvalidOperationException("선택한 객체 전환의 실제 가상 입력 미복원");
        }
        if ((code & CodeMask) != ConversionCode)
        {
            return Need(_hooks.Base, "객체 전환의 실제 하위 미연결")(sid, code);
        }
        Need(_hooks.Unpop, "객체 전환의 실제 하위 미연결")(sid, 0);
        if (body == ObeliskBody)
        {
            ReadOnlySpan<byte> raw = Raw(sid);
            uint type = (uint)(raw[12] | (raw[13] << 8));
            Need(_hooks.Retype, "객체 전환의 실제 하위 미연결")(sid, type);
            uint frame = Need(_hooks.DefaultFrame, "객체 전환의 실제 하위 미연결")(Raw(sid)[10]);
            Need(_hooks.Frame, "객체 전환의 실제 하위 미연결")(sid, frame);
        }
        else
        {
            // 원문은 Unpop 뒤 현재 타입을 보관해 마지막 QA에 쓴다
            byte previous = Raw(sid)[10];
            Need(_hooks.Retype, "객체 전환의 실제 하위 미연결")(sid, _buriedType);
            Need(_hooks.Frame, "객체 전환의 실제 하위 미연결")(sid, 0);
            Need(_hooks.Qa, "객체 전환의 실제 하위 미연결")(sid, previous);
        }
        Need(_hooks.Repop, "객체 전환의 실제 하위 미연결")(sid, 0);
        return 1;
    }

    /// <summary>원문의 물리 범위만 확인하고 상태·소유자 필터를 추가하지 않는다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private ReadOnlySpan<byte> Raw(int sid) => _pool.Slot(sid);

    /// <summary>실제 선택한 장치만 그 자리에서 요구한다. 비어 있으면 명시 경계 오류로 멈춘다.</summary>
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
