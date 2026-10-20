namespace Netstorm.Core.Original;

/// <summary>
/// 프레임 지정이 부르는 외부 효과의 호출 순서(원본 SquidFrameHooks).
/// 네 항목이 모두 연결되어야 하며, 하나라도 없으면 생성자에서 거부한다.
/// </summary>
public sealed class SquidFrameHooks
{
    /// <summary>현재 프레임의 표시 가로·세로 크기. 프레임이 표를 벗어나면 원본처럼 거부한다.</summary>
    public Func<int, int, float[]>? FrameSize { get; set; }

    /// <summary>가상 표시 갱신. 플래그에 0x2000이 있으면 +0x8c, 없으면 +0x88이다.</summary>
    public Action<int, uint>? Update { get; set; }

    /// <summary>가상 Unpop(vtable +0x48).</summary>
    public Action<int, uint>? Unpop { get; set; }

    /// <summary>가상 +0x4c(원본 0041c0d0)가 부르는 가상 Pop(+0x90). 현재 위치와 플래그로 다시 등록한다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
}

/// <summary>
/// 객체 프레임 변경을 물리 프레임·통지·갱신 요청에 반영하는 처리기(원본 SquidFrame 004acee0, 10.78).
/// 표시 갱신·Unpop·Pop은 가상 호출 순서대로 외부 효과에 연결하고, 레벨·프레임은 원본 바이트 위치에 쓴다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 프레임·레벨 바이트 위치와 쓰기 폭이 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/SquidFrame.cpp (W02 Frame 조각, 2026-10-20).
/// </summary>
public sealed class SquidFrame
{
    /// <summary>프레임 지정이 받는 플래그 중 0x2000은 +0x8c 가상 표시 갱신이다.</summary>
    public const uint FrameUpdateAlternate = 0x2000;

    /// <summary>표시 갱신이 맞지 않아 다시 등록할 때 Pop 에 붙이는 플래그(원본 or edi, 0x50).</summary>
    public const uint FrameRepopFlags = 0x50;

    /// <summary>플래그2의 섬·다리 마스크. 켜진 타입은 프레임 크기를 세지 않고 레벨 0이다.</summary>
    private const uint IslandBridgeMask = 0x6;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 현재 X 좌표(float)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 현재 Y 좌표(float)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 표시 레벨 바이트의 위치(10.78 +0x21).</summary>
    private const int LevelOffset = 0x21;

    /// <summary>슬롯 안 프레임 DWORD의 위치(10.78 +0x24).</summary>
    private const int FrameOffset = 0x24;

    /// <summary>슬롯을 읽고 쓸 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 → 타입 레코드. 표시 레벨 판정의 genus 를 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>외부 효과 연결 묶음.</summary>
    private readonly SquidFrameHooks _hooks;

    /// <summary>
    /// 풀·타입·효과를 받아 프레임 지정을 만든다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="hooks">외부 효과 연결 묶음. 네 항목이 모두 있어야 한다.</param>
    public SquidFrame(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, SquidFrameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(hooks);
        // CD 10.72(171개)는 프레임·레벨 바이트 위치와 쓰기 폭이 다르므로 받지 않는다
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        if (hooks.FrameSize is null || hooks.Update is null || hooks.Unpop is null || hooks.Pop is null)
        {
            throw new ArgumentException("프레임 지정 효과 누락", nameof(hooks));
        }
        _pool = pool;
        _types = types;
        _hooks = hooks;
    }

    /// <summary>이 모듈이 쓰는 슬롯 풀. 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 객체의 프레임 번호를 바꾼다. 표시 레벨이 맞으면 갱신만 두 번 하고,
    /// 프레임이 현재와 같으면 레벨 바이트만 고치며, 다르면 Unpop 뒤 현재 위치로 다시 등록한다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="frame">바꿀 프레임 번호(부호 있는 DWORD).</param>
    /// <param name="flags">호출 플래그. 0 또는 0x2000이 아니면 원본처럼 거부한다.</param>
    public void Set(int sid, int frame, uint flags)
    {
        // 0·0x2000 밖의 값은 판본 assert 로 거부한다
        if (flags != 0 && flags != FrameUpdateAlternate)
        {
            throw new ArgumentException("프레임 지정 flags(판본 assert)", nameof(flags));
        }
        // 레벨 바이트는 부호 있는 바이트로 읽고, 레벨 계산은 쓰기 전에 먼저 한다
        int stored = unchecked((sbyte)_pool.Slot(sid)[LevelOffset]);
        int current = Frame(sid);
        int level = StoreLevel(sid);
        if (level == stored)
        {
            // 같은 레벨: 이전·현재 그림 영역을 갱신하고 표시 등록은 그대로 둔다
            _hooks.Update!(sid, flags);
            WriteFrame(sid, frame);
            _hooks.Update!(sid, flags);
            return;
        }
        // 레벨이 맞지 않는데 프레임이 현재와 같으면 레벨 바이트만 고치고 끝난다
        if (frame == current)
        {
            return;
        }
        _hooks.Unpop!(sid, flags);
        WriteFrame(sid, frame);
        // +0x4c 재등록은 Unpop 뒤의 위치를 이어 가상 Pop 에 넘긴다
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        _hooks.Pop!(sid, X(raw, XOffset), X(raw, YOffset), flags | FrameRepopFlags);
    }

    /// <summary>
    /// 객체의 현재 프레임으로 표시 레벨을 구해 레벨 바이트(+0x21)에 쓰고 돌려준다.
    /// 섬·다리 부류는 0이며, 그 밖은 현재 프레임 크기의 큰 쪽이 2 이하이면 1, 4 이하이면 2, 그 밖이면 3이다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    public int StoreLevel(int sid)
    {
        uint number = _pool.Slot(sid)[TypeOffset];
        if (false)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), number, "프레임 지정 타입 번호 범위 오류");
        }
        uint genus = _types[(int)number].Flags2;
        int level = 0;
        // 섬·다리는 프레임을 세지 않는다. 그 밖은 현재 프레임의 크기로 판정한다
        if ((genus & IslandBridgeMask) == 0)
        {
            float[] size = _hooks.FrameSize!(sid, Frame(sid));
            level = RawSquidHash.ObjectLevel(genus, size[0], size[1]);
        }
        _pool.AllocatedBytes(sid)[LevelOffset] = (byte)level;
        return level;
    }

    /// <summary>판본의 프레임 번호 필드를 읽는다. 10.78 은 +0x24 DWORD 다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    private int Frame(int sid) => unchecked((int)Read(_pool.Slot(sid), FrameOffset));

    /// <summary>프레임 DWORD 를 쓴다. 낮은 바이트부터 4바이트를 쓴다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="frame">쓸 프레임 번호.</param>
    private void WriteFrame(int sid, int frame)
    {
        Span<byte> raw = _pool.AllocatedBytes(sid);
        uint bits = unchecked((uint)frame);
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < 4; i++)
        {
            raw[FrameOffset + i] = (byte)(bits >> (8 * i));
        }
    }

    /// <summary>현재 float 좌표의 비트 패턴을 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static float X(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(Read(raw, offset));

    /// <summary>정렬되지 않은 리틀 엔디언 값을 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        uint value = 0;
        // 필요한 4바이트만 낮은 바이트부터 조합한다
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
}
