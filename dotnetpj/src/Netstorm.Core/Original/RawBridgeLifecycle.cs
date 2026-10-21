namespace Netstorm.Core.Original;

/// <summary>
/// 다리 사전·사후 파괴가 부르는 바깥 효과의 종류(원본 BridgeLifecycleEffect).
/// DestroyLink 는 끊긴 연결 다리의 재귀 삭제를, Base 앞뒤는 공통 삭제의 완료 호출을 바깥이 맡는다는 뜻이다.
/// </summary>
public enum BridgeLifecycleEffect
{
    /// <summary>끊긴 연결 다리의 삭제. 바깥은 공통 삭제 본체로 지운다.</summary>
    DestroyLink,

    /// <summary>다리 제거 알림. 바깥이 화면·선택 정리를 한다.</summary>
    NotifyRemoval,

    /// <summary>추락 소리. 바깥이 다리 좌표의 효과음을 낸다.</summary>
    FallSound,

    /// <summary>추락 보행자. 바깥이 그 번호에 죽음 비트를 세운다.</summary>
    FallWalker,

    /// <summary>공통 사전 파괴. 바깥은 <see cref="RawSquidDestroy.CompletePreDestroy"/> 를 정확히 한 번 불러야 한다.</summary>
    BasePreDestroy,

    /// <summary>공통 사후 파괴. 바깥은 <see cref="RawSquidDestroy.CompletePostDestroy"/> 를 정확히 한 번 불러야 한다.</summary>
    BasePostDestroy,
}

/// <summary>
/// 다리 삭제 효과 한 건의 인자(원본 BridgeLifecycleEvent).
/// 좌표는 추락 소리(FallSound)에서만 쓰이며, 그 밖은 다리 좌표를 다시 읽지 않고 0 이다.
/// </summary>
/// <param name="Effect">바깥으로 나가는 효과의 종류.</param>
/// <param name="Sid">효과 대상의 슬롯 번호.</param>
/// <param name="Flags">삭제에 적용되는 플래그 비트.</param>
/// <param name="X">추락 지점 x 좌표(FallSound 만).</param>
/// <param name="Y">추락 지점 y 좌표(FallSound 만).</param>
public readonly record struct BridgeLifecycleEvent(BridgeLifecycleEffect Effect, int Sid, uint Flags, float X = 0, float Y = 0);

/// <summary>
/// 다리 삭제가 바깥에 요구하는 콜백 묶음(원본 BridgeLifecycleHooks).
/// Emit 은 항상 있어야 하고, 일반(매립·추상 아님) 다리는 begin·next 탐색기도 있어야 한다.
/// </summary>
public sealed class BridgeLifecycleHooks
{
    /// <summary>사각형 안의 첫 번호를 찾는 콜백. 없으면 0 을 돌려준다.</summary>
    public Func<BridgeLifecycleSearch, int>? Begin { get; set; }

    /// <summary>같은 탐색의 다음 번호를 찾는 콜백. 없으면 0 을 돌려준다.</summary>
    public Func<int>? Next { get; set; }

    /// <summary>다리 삭제 효과를 바깥 처리기에 알리는 콜백.</summary>
    public Action<BridgeLifecycleEvent>? Emit { get; set; }
}

/// <summary>
/// 다리 삭제 탐색 사각형(원본 BridgeLifecycleSearch). 좌표는 칸 단위이며 경계를 포함한다.
/// </summary>
/// <param name="Left">사각형 왼쪽 칸.</param>
/// <param name="Top">사각형 위쪽 칸.</param>
/// <param name="Right">사각형 오른쪽 칸.</param>
/// <param name="Bottom">사각형 아래쪽 칸.</param>
public readonly record struct BridgeLifecycleSearch(int Left, int Top, int Right, int Bottom);

/// <summary>
/// 다리 사전·사후 파괴(원본 RawBridgeLifecycle 00422290·004221b0·00422300, 10.78).
/// 사전은 주변 3×3 칸에서 끊긴 연결을 찾아 재귀 삭제를 알리고, 사후는 제거 알림·추락 소리·보행자 추락을 순서대로 알린다.
/// 매립·추상 다리는 탐색을 건너뛰고 공통 사전·사후만 알린다. 탐색은 일반 찾기(<see cref="RawSquidFinder"/>)를 그대로 쓴다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 추가 비트 위치·함수 주소가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawBridgeLifecycle.cpp (W03 삭제·수명 둘째 조각, 2026-10-21).
/// </summary>
public sealed class RawBridgeLifecycle
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 첫 참조 번호(16비트)의 위치. 연결 다리가 가리키는 첫 칸이다.</summary>
    private const int FirstReferenceOffset = 12;

    /// <summary>슬롯 안 둘째 참조 번호(16비트)의 위치. 연결 다리가 가리키는 둘째 칸이다.</summary>
    private const int SecondReferenceOffset = 8;

    /// <summary>슬롯 안 x 좌표(float)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78 +40).</summary>
    private const int ExtraOffset = 40;

    /// <summary>상태 바이트의 죽음 비트.</summary>
    private const byte DeadBit = 2;

    /// <summary>추가 비트의 매립·추상 마스크. 켜지면 다리 삭제 효과를 건너뛴다.</summary>
    private const byte AbstractOrBuriedMask = 9;

    /// <summary>플래그2의 보행자(walker) 비트. 켜진 타입은 사후 탐색에서 추락한다.</summary>
    private const uint WalkerFlag = 0x10000;

    /// <summary>사전 탐색 사각형을 구할 때 더하는 좌표 편향의 비트(원본 0x3f7fff58, 약 0.99998).</summary>
    private const uint AreaBiasBits = 0x3f7fff58;

    /// <summary>삭제 대상 슬롯을 읽는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 → 타입 레코드. 보행자 비트를 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>다리 삭제를 만든다. 풀과 타입 표를 묶는다.</summary>
    /// <param name="pool">삭제 대상 슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드.</param>
    public RawBridgeLifecycle(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        _pool = pool;
        _types = types;
    }

    /// <summary>일반 찾기로 다리 탐색기를 만드는 훅 묶음. 같은 찾기를 두 탐색이 동시에 쓰면 안 된다.</summary>
    /// <param name="finder">일반 찾기. 탐색이 끝날 때까지 바깥이 따로 쓰면 안 된다.</param>
    /// <param name="emit">다리 삭제 효과를 받는 콜백.</param>
    public static BridgeLifecycleHooks MakeHooks(RawSquidFinder finder, Action<BridgeLifecycleEvent> emit)
    {
        ArgumentNullException.ThrowIfNull(finder);
        ArgumentNullException.ThrowIfNull(emit);
        return new BridgeLifecycleHooks
        {
            Begin = area => finder.Begin(new SquidSearchArea(area.Left, area.Top, area.Right, area.Bottom)),
            Next = () => finder.Next(),
            Emit = emit,
        };
    }

    /// <summary>연결 다리가 끊겨서 지워야 하는지 본다(원본 00422290). 첫·둘째 참조가 살아 있으면 거짓이다.</summary>
    /// <param name="link">연결 다리 번호.</param>
    public bool LinkNeedsDestroy(int link)
    {
        ReadOnlySpan<byte> raw = Object(link);
        int first = unchecked((int)Read(raw, FirstReferenceOffset, 2));
        int second = unchecked((int)Read(raw, SecondReferenceOffset, 2));
        if (first == 0 || first >= _pool.Capacity || second == 0 || second >= _pool.Capacity)
        {
            return false;
        }
        return Ordinary(first) && ((Object(first)[StateOffset] | Object(second)[StateOffset]) & DeadBit) != 0;
    }

    /// <summary>
    /// 다리 사전 파괴(원본 004221b0). 일반 다리는 주변 3×3 칸에서 끊긴 연결(지정 타입)을 찾아 삭제를 알린 뒤 공통 사전을 알린다.
    /// </summary>
    /// <param name="bridge">다리 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    /// <param name="linkType">찾을 연결 타입 번호.</param>
    /// <param name="hooks">바깥 효과 콜백 묶음.</param>
    public void PreDestroy(int bridge, uint flags, uint linkType, BridgeLifecycleHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        bool ordinary = Ordinary(bridge);
        ValidateHooks(hooks, ordinary);
        if (ordinary)
        {
            ReadOnlySpan<byte> raw = Object(bridge);
            float bias = BitConverter.UInt32BitsToSingle(AreaBiasBits);
            int x = Cell(Coordinate(raw, XOffset), bias);
            int y = Cell(Coordinate(raw, YOffset), bias);
            // 찾기 사각형은 중심 칸의 상하좌우 한 칸씩이다
            for (int current = hooks.Begin!(new BridgeLifecycleSearch(x - 1, y - 1, x + 1, y + 1)); current != 0; current = hooks.Next!())
            {
                if (Object(current)[TypeOffset] == linkType && LinkNeedsDestroy(current))
                {
                    hooks.Emit!(new BridgeLifecycleEvent(BridgeLifecycleEffect.DestroyLink, current, 0));
                }
            }
        }
        hooks.Emit!(new BridgeLifecycleEvent(BridgeLifecycleEffect.BasePreDestroy, bridge, flags));
    }

    /// <summary>
    /// 다리 사후 파괴(원본 00422300). 일반 다리는 제거 알림·추락 소리·같은 칸 보행자 추락을 순서대로 알린 뒤 공통 사후를 알린다.
    /// </summary>
    /// <param name="bridge">다리 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    /// <param name="hooks">바깥 효과 콜백 묶음.</param>
    public void PostDestroy(int bridge, uint flags, BridgeLifecycleHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        bool ordinary = Ordinary(bridge);
        ValidateHooks(hooks, ordinary);
        if (ordinary)
        {
            hooks.Emit!(new BridgeLifecycleEvent(BridgeLifecycleEffect.NotifyRemoval, bridge, 0));
            ReadOnlySpan<byte> raw = Object(bridge);
            float x = Coordinate(raw, XOffset);
            float y = Coordinate(raw, YOffset);
            hooks.Emit!(new BridgeLifecycleEvent(BridgeLifecycleEffect.FallSound, bridge, 0, x, y));
            raw = Object(bridge);
            int cellX = Cell(Coordinate(raw, XOffset));
            int cellY = Cell(Coordinate(raw, YOffset));
            // 같은 칸에서 보행자 타입만 추락시킨다
            for (int current = hooks.Begin!(new BridgeLifecycleSearch(cellX, cellY, cellX, cellY)); current != 0; current = hooks.Next!())
            {
                int type = Object(current)[TypeOffset];
                if (type >= _types.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(bridge), type, "다리 수명주기 타입 번호 오류");
                }
                if ((_types[type].Flags2 & WalkerFlag) != 0)
                {
                    hooks.Emit!(new BridgeLifecycleEvent(BridgeLifecycleEffect.FallWalker, current, 0));
                }
            }
        }
        hooks.Emit!(new BridgeLifecycleEvent(BridgeLifecycleEffect.BasePostDestroy, bridge, flags));
    }

    /// <summary>
    /// 지연 추락 사건(원본 0x2692)의 실수 페이로드(원본 00421530·00421f90).
    /// x·y 를 각각 ftol 로 자른 값의 하위 바이트(x)·상위 3바이트(y)를 한 DWORD 로 묶어 float 비트로 돌려준다.
    /// </summary>
    /// <param name="x">추락 지점 x 좌표.</param>
    /// <param name="y">추락 지점 y 좌표.</param>
    public static float DelayedFallPayload(float x, float y)
    {
        uint packed = (FtolLow(x) & 0xff) | (FtolLow(y) << 8);
        return BitConverter.Int32BitsToSingle(unchecked((int)packed));
    }

    /// <summary>번호의 슬롯을 읽기 전용으로 돌려준다. 0 번·범위 밖 번호는 거부한다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    private ReadOnlySpan<byte> Object(int sid)
    {
        if (sid == 0 || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "다리 수명주기 번호 오류");
        }
        return _pool.Slot(sid);
    }

    /// <summary>매립·추상 비트가 모두 꺼진 일반 다리인지 본다.</summary>
    /// <param name="sid">볼 슬롯 번호.</param>
    private bool Ordinary(int sid) => (Object(sid)[ExtraOffset] & AbstractOrBuriedMask) == 0;

    /// <summary>슬롯의 리틀 엔디언 값을 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="size">읽는 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int size)
    {
        uint value = 0;
        // 낮은 바이트부터 폭만큼 합친다
        for (int i = 0; i < size; i++)
        {
            value |= (uint)raw[offset + i] << (i * 8);
        }
        return value;
    }

    /// <summary>슬롯의 float 비트를 그대로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(Read(raw, offset, 4));

    /// <summary>좌표를 원본 ftol 처럼 칸으로 바꾼다. 유한하지 않거나 int 를 벗어나면 거부한다.</summary>
    /// <param name="value">좌표 값.</param>
    /// <param name="bias">더하는 편향(사전 탐색은 약 0.99998).</param>
    private static int Cell(float value, float bias = 0)
    {
        double wide = (double)value + bias;
        if (!double.IsFinite(wide) || wide <= int.MinValue || wide >= int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "다리 수명주기 좌표 오류");
        }
        return (int)wide;
    }

    /// <summary>원본 CRT ftol 의 하위 DWORD. 64비트 범위를 벗어나면 0 이다.</summary>
    /// <param name="value">자를 float 값.</param>
    private static uint FtolLow(float value)
    {
        double wide = value;
        if (!(wide > -9223372036854775808.0 && wide < 9223372036854775808.0))
        {
            return 0;
        }
        return unchecked((uint)(ulong)(long)wide);
    }

    /// <summary>훅 묶음이 비었는지 확인한다. 일반 다리는 탐색기도 있어야 한다.</summary>
    /// <param name="hooks">바깥 효과 콜백 묶음.</param>
    /// <param name="ordinary">일반 다리인지.</param>
    private static void ValidateHooks(BridgeLifecycleHooks hooks, bool ordinary)
    {
        if (hooks.Emit is null || (ordinary && (hooks.Begin is null || hooks.Next is null)))
        {
            throw new ArgumentException("다리 수명주기 효과 콜백이 연결되지 않았습니다", nameof(hooks));
        }
    }
}
