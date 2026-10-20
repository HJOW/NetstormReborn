using Netstorm.Assets;

namespace Netstorm.Core.Original;

/// <summary>
/// 다리 사건 처리기가 읽는 전역 상태(원본 BridgeEventState).
/// </summary>
public sealed class BridgeEventState
{
    /// <summary>사건을 처리할 권한이 있는지(원본 00540bc4). 거짓이면 파괴·변환을 하지 않는다.</summary>
    public bool Authority { get; set; } = true;

    /// <summary>칸 변환을 막는 진단 플래그(원본 0054db80).</summary>
    public bool DebugKeep { get; set; }

    /// <summary>단어 갱신 때 표면 알림을 부를 다리 타입 번호(원본 005411a0).</summary>
    public uint BridgeType { get; set; }
}

/// <summary>
/// 칸 변환이 부르는 외부 효과(원본 BridgeEventHooks). 일곱 항목이 모두 있어야 한다.
/// </summary>
public sealed class BridgeEventHooks
{
    /// <summary>타입 번호의 SHP 프레임 코드 표.</summary>
    public Func<uint, IReadOnlyList<FrameCode>>? Frames { get; set; }

    /// <summary>임시 프레임을 세운 채 flag 0 이웃 탐색을 돌려 첫 이웃 번호(+0x34)를 준다. 0이면 이웃 없음.</summary>
    public Func<int, int>? FirstNeighbor { get; set; }

    /// <summary>표면 알림(원본 004214a0). 값은 바꾸지 않는다.</summary>
    public Action<int>? NotifySurface { get; set; }

    /// <summary>객체 생성(원본 004af530). 타입 목록의 그 타입 표 칸을 가진 새 번호를 준다.</summary>
    public Func<uint, int>? Create { get; set; }

    /// <summary>가상 삭제(vtable +0x10).</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>가상 소유자 지정(vtable +0x74). 소유자는 그 객체의 소유자 바이트다.</summary>
    public Action<int, byte>? SetOwner { get; set; }

    /// <summary>가상 Pop(vtable +0x90). 좌표는 객체의 위치 float 그대로다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
}

/// <summary>
/// 다리 사건 처리기(원본 00422740과 칸 변환 004215d0의 10.78 경로).
/// 0x2691은 자기 삭제, 0x2692는 payload(목표 칸 float)로 칸 변환을 하고, 두 사건 모두 유지(−1)를 돌려준다.
/// 다른 사건은 payload를 그대로 돌려준다. count(둘째 인자)는 쓰지 않는다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 프레임·소유자·플래그 위치가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawBridgeEvents.cpp (W02 다리 사건 조각, 2026-10-20).
/// </summary>
public sealed class RawBridgeEvents
{
    /// <summary>자기 파괴 사건 번호(원본 0x2691).</summary>
    public const uint DestroyEvent = 0x2691;

    /// <summary>낙하·칸 변환 사건 번호(원본 kBridgeFallEvent = 0x2692).</summary>
    public const uint FallEvent = 0x2692;

    /// <summary>처리기가 돌려주는 "끝" 값. 사건 뒤 예약을 남기지 않는다.</summary>
    public const float EventEnd = 0.0f;

    /// <summary>처리기가 돌려주는 "유지" 값. 사건을 계속 둔다.</summary>
    public const float EventKeep = -1.0f;

    /// <summary>슬롯 안 부모 번호(16비트)의 위치.</summary>
    private const int ParentOffset = 8;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 표면 단어(16비트)의 위치.</summary>
    private const int WordOffset = 12;

    /// <summary>슬롯 안 현재 x 좌표(float)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 현재 y 좌표(float)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 소유자 바이트의 위치(10.78).</summary>
    private const int OwnerOffset = 0x22;

    /// <summary>슬롯 안 프레임 DWORD의 위치(10.78).</summary>
    private const int FrameOffset = 0x24;

    /// <summary>슬롯 안 플래그 바이트의 위치(10.78).</summary>
    private const int FlagOffset = 0x28;

    /// <summary>다리 수명 마스크(원본 0x78).</summary>
    private const ushort LifeMask = 0x78;

    /// <summary>다리 수명 최대값(원본 kBridgeCrackLife = 5). 변환 뒤 수명은 (5×8−8)을 마스크한 값이다.</summary>
    private const int CrackLife = 5;

    /// <summary>하드 프레임 코드 플래그(원본 0x40). 켜져 있으면 새 객체를 만들지 않는다.</summary>
    private const byte HardFrameFlag = 0x40;

    /// <summary>새 객체에 복사하는 플래그 비트(원본 0x10).</summary>
    private const byte CopiedFlagBit = 0x10;

    /// <summary>객체 타입의 첫 번호(원본 0x46). 그보다 작으면 form 이라 변환하지 않는다.</summary>
    private const uint FirstObjectType = 0x46;

    /// <summary>세로 칸 코드 'J'.</summary>
    private const sbyte SideJ = (sbyte)'J';

    /// <summary>가로 칸 코드 'K'.</summary>
    private const sbyte SideK = (sbyte)'K';

    /// <summary>끝 칸 코드 'L'·'M'·'N'·'O'.</summary>
    private const char EndL = 'L', EndM = 'M', EndN = 'N', EndO = 'O';

    /// <summary>슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>처리기가 읽는 전역 상태.</summary>
    private readonly BridgeEventState _state;

    /// <summary>외부 효과 연결 지점.</summary>
    private readonly BridgeEventHooks _hooks;

    /// <summary>
    /// 풀·상태·효과를 받아 다리 사건 처리기를 만든다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="state">전역 상태.</param>
    /// <param name="hooks">효과 연결 지점. 일곱 항목이 모두 있어야 한다.</param>
    public RawBridgeEvents(RawSidPool pool, BridgeEventState state, BridgeEventHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        // 빠진 효과로는 만들지 않는다
        if (hooks.Frames is null || hooks.FirstNeighbor is null || hooks.NotifySurface is null || hooks.Create is null
            || hooks.Destroy is null || hooks.SetOwner is null || hooks.Pop is null)
        {
            throw new ArgumentException("다리 사건 효과 누락", nameof(hooks));
        }
        _pool = pool;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 모듈이 쓰는 슬롯 풀.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 사건을 처리하고 Regular 갱신값을 돌려준다. 0x2691은 자기 삭제, 0x2692는 칸 변환이며 둘 다 유지(−1)다.
    /// 다른 사건은 payload를 그대로 돌려준다.
    /// </summary>
    /// <param name="bridge">다리 번호.</param>
    /// <param name="eventCode">사건 번호.</param>
    /// <param name="count">사건 횟수. 쓰지 않는다.</param>
    /// <param name="payload">사건 값. 0x2692 에서는 목표 칸 float 비트다.</param>
    public float Handle(int bridge, uint eventCode, uint count, float payload)
    {
        _ = count;
        if (eventCode == DestroyEvent)
        {
            // 권한이 없으면 아무것도 하지 않고 끝난다
            if (!_state.Authority)
            {
                return EventEnd;
            }
            _hooks.Destroy!(bridge, 0);
            return EventKeep;
        }
        if (eventCode == FallEvent)
        {
            if (!_state.Authority)
            {
                return EventEnd;
            }
            // payload 의 낮은 8비트가 x, 그 위 부호 있는 8비트가 y 인 절삭값이다
            uint low = FtolLow(payload);
            ConvertEnd(bridge, low & 0xffU, (uint)((int)low >> 8));
            return EventKeep;
        }
        // 처리하지 않는 사건은 payload를 그대로 돌려준다
        return payload;
    }

    /// <summary>
    /// 다리 칸을 끝 칸(L·M·N·O)으로 바꾼다. (x, y)는 절삭 없이 비교하는 목표 칸 좌표다.
    /// 권한이 없거나 진단 플래그가 켜져 있거나 끝 칸이 아니면 아무것도 바꾸지 않는다.
    /// </summary>
    /// <param name="bridge">다리 번호.</param>
    /// <param name="x">목표 칸 x.</param>
    /// <param name="y">목표 칸 y.</param>
    public void ConvertEnd(int bridge, float x, float y)
    {
        // 진단 플래그가 켜져 있거나 권한이 없으면 그대로 둔다
        if (_state.DebugKeep || !_state.Authority)
        {
            return;
        }
        uint type = _pool.Slot(bridge)[TypeOffset];
        // form(객체 타입 미만)은 원본 assert 경로다
        if (type < FirstObjectType)
        {
            throw new InvalidOperationException("다리 타입이 form 입니다(004ad680 assert)");
        }
        IReadOnlyList<FrameCode> codes = _hooks.Frames!(type);
        int oldFrame = Frame(bridge);
        if (oldFrame < 0 || oldFrame >= codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(bridge), oldFrame, "다리 현재 프레임 범위 오류");
        }
        ReadOnlySpan<byte> raw = _pool.Slot(bridge);
        float objectX = BitConverter.UInt32BitsToSingle(Read(raw, XOffset));
        float objectY = BitConverter.UInt32BitsToSingle(Read(raw, YOffset));
        // 현재 프레임 코드의 방향 글자를 부호 있는 byte 로 읽는다. 비교가 거짓이거나 NaN 이면 각각 L, O 쪽이다
        sbyte side = (sbyte)codes[oldFrame].Side;
        char letter;
        if (side == SideJ)
        {
            letter = objectY < y ? EndN : EndL;
        }
        else if (side == SideK)
        {
            letter = objectX < x ? EndM : EndO;
        }
        else
        {
            return;
        }
        int first = FirstLetterIndex(codes, letter);
        // 그 글자의 프레임이 없으면 원본 assert 경로다
        if (first < 0)
        {
            throw new InvalidOperationException("다리 타입에 그 방향 글자가 없습니다");
        }
        uint newFrame = (uint)first;
        // 임시로 새 프레임을 세워 첫 이웃 탐색을 돌린 뒤 바로 이전 프레임으로 되돌린다
        SetFrame(bridge, newFrame);
        int neighbor;
        try
        {
            neighbor = _hooks.FirstNeighbor!(bridge);
        }
        catch
        {
            SetFrame(bridge, (uint)oldFrame);
            throw;
        }
        SetFrame(bridge, (uint)oldFrame);
        // 이웃이 없으면 그 칸이 끝 칸이므로 지운다
        if (neighbor == 0)
        {
            _hooks.Destroy!(bridge, 0);
            return;
        }
        // 수명 비트(+0xc의 3~6비트)를 (최대 수명 − 1)로 맞춘 뒤 표면 알림을 부른다
        ushort lifeBits = (ushort)((CrackLife * 8 - 8) & LifeMask);
        ushort word = (ushort)((Read(_pool.Slot(bridge), WordOffset, 2) & ~LifeMask) | lifeBits);
        Put(_pool.AllocatedBytes(bridge), WordOffset, word, 2);
        _hooks.NotifySurface!(bridge);
        // 하드 프레임이면 새 객체를 만들지 않고 끝낸다
        int current = Frame(bridge);
        if (current < 0 || current >= codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(bridge), current, "다리 현재 프레임 범위 오류");
        }
        if ((codes[current].Flags & (FrameCodeFlags)HardFrameFlag) != 0)
        {
            return;
        }
        // 새 다리는 같은 타입으로 만들어 프레임·소유자·단어·부모 단어·플래그 비트를 옮긴다
        int born = _hooks.Create!(type);
        SetFrame(born, newFrame);
        _hooks.SetOwner!(born, _pool.Slot(bridge)[OwnerOffset]);
        SetWord(born, (ushort)Read(_pool.Slot(bridge), WordOffset, 2));
        Put(_pool.AllocatedBytes(born), ParentOffset, Read(_pool.Slot(bridge), ParentOffset, 2), 2);
        Span<byte> newRaw = _pool.AllocatedBytes(born);
        newRaw[FlagOffset] = (byte)((newRaw[FlagOffset] & ~CopiedFlagBit) | (_pool.Slot(bridge)[FlagOffset] & CopiedFlagBit));
        // 두 칸의 위치를 읽고 이전 칸을 지운 뒤 새 객체를 그 위치에 둔다
        float atX = BitConverter.UInt32BitsToSingle(Read(_pool.Slot(bridge), XOffset));
        float atY = BitConverter.UInt32BitsToSingle(Read(_pool.Slot(bridge), YOffset));
        _hooks.Destroy!(bridge, 0);
        _hooks.Pop!(born, atX, atY, 0);
    }

    /// <summary>판본의 프레임 번호를 읽는다. 10.78 은 +0x24 DWORD 다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    private int Frame(int sid) => unchecked((int)Read(_pool.Slot(sid), FrameOffset));

    /// <summary>프레임 번호를 쓴다. 낮은 바이트부터 4바이트를 쓴다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="frame">쓸 프레임 번호.</param>
    private void SetFrame(int sid, uint frame)
    {
        Span<byte> raw = _pool.AllocatedBytes(sid);
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < 4; i++)
        {
            raw[FrameOffset + i] = (byte)(frame >> (8 * i));
        }
    }

    /// <summary>표면 단어를 쓰고, 그 객체가 다리 타입이면 표면 알림을 부른다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="word">쓸 단어.</param>
    private void SetWord(int sid, ushort word)
    {
        Put(_pool.AllocatedBytes(sid), WordOffset, word, 2);
        // 다리 타입일 때만 표면 알림을 부른다
        if (_pool.Slot(sid)[TypeOffset] == _state.BridgeType)
        {
            _hooks.NotifySurface!(sid);
        }
    }

    /// <summary>방향 글자의 첫 프레임 번호를 찾는다(원본 RiftTypeFrames::Run의 first). 없으면 −1이다.</summary>
    /// <param name="codes">프레임 코드 표.</param>
    /// <param name="letter">방향 글자 A~P.</param>
    private static int FirstLetterIndex(IReadOnlyList<FrameCode> codes, char letter)
    {
        // 글자는 A~P 만 받는다
        if (letter < 'A' || letter > 'P')
        {
            throw new ArgumentOutOfRangeException(nameof(letter), letter, "방향 글자는 A~P 이어야 합니다");
        }
        // 앞에서부터 훑어 그 글자의 첫 번호를 찾는다
        for (int i = 0; i < codes.Count; i++)
        {
            if (codes[i].Side == letter)
            {
                return i;
            }
        }
        return -1;
    }

    /// <summary>
    /// 원본 CRT 절삭의 64비트 하위 DWORD. 비유한·64비트 밖이면 0이다.
    /// </summary>
    /// <param name="value">자를 실수.</param>
    private static uint FtolLow(float value)
    {
        double wide = value;
        // 범위 밖은 원본처럼 0이다
        if (!(wide > -9223372036854775808.0 && wide < 9223372036854775808.0))
        {
            return 0;
        }
        return unchecked((uint)(long)wide);
    }

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }

    /// <summary>슬롯 안에 정렬되지 않은 리틀 엔디언 값을 폭만큼 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (8 * i));
        }
    }
}
