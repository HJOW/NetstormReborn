// 원본 사제 AI의 의식/건설/탐색/회수/수집·귀환 전체 행동을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 사제 행동 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.

// 0041d7a0: 각 축에 원본 float 0.99999를 더한 뒤 signed ftol 하위 DWORD→float로 저장한다.
public static class AiPriestPoints
{
    /// <summary>각 축에 원본 float 0.99999를 더한 뒤 signed ftol 하위 DWORD를 float로 저장한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="point">자를 좌표다.</param>
    /// <returns>절삭한 좌표다.</returns>
    public static RawPathPoint Snap(RawPathPoint point)
    {
        point.X = Ftol((double)point.X + 0.99999f);
        point.Y = Ftol((double)point.Y + 0.99999f);
        return point;
    }

    /// <summary>0040e800: 두 float 모두 0보다 크고 현재 mapLimit의 float 저장값보다 작아야 한다. NaN은 거짓이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="point">검사할 좌표다.</param>
    /// <param name="mapLimit">현재 지도 상한이다.</param>
    /// <returns>유효하면 참이다.</returns>
    public static bool Valid(RawPathPoint point, int mapLimit)
    {
        float limit = mapLimit;
        return point.X > 0 && point.X < limit && point.Y > 0 && point.Y < limit;
    }

    /// <summary>00491b30: 현재 물리 frame의 signed 첫 문자-'A'가 10..14인지 조회한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="pool">현재 슬롯 풀이다.</param>
    /// <param name="frames">현재 frame 표다.</param>
    /// <param name="priest">검사할 사제 SID다.</param>
    /// <returns>의식 구간이면 참이다.</returns>
    public static bool InRitual(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, Sid priest)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ReadOnlySpan<byte> raw = pool.Slot(priest.Value);
        if (raw[RawSidPool.TypeOffset] >= frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(priest), "AI 사제 의식 현재TYPE 범위");
        }

        int phase = RawPathAnimation.Direction(frames[raw[RawSidPool.TypeOffset]], unchecked((int)BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(36, 4))));
        return phase > 9 && phase < 15;
    }

    /// <summary>00448a90: 다이스 위치의 실제 제단 argument WORD를 적 owner DWORD와 비교한다. 찾은 SID 0도 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="pool">현재 슬롯 풀이다.</param>
    /// <param name="dais">다이스 SID다.</param>
    /// <param name="enemy">비교할 적 owner DWORD다.</param>
    /// <param name="altarType">조회할 제단 TYPE다.</param>
    /// <param name="find">현재 제단 찾기다.</param>
    /// <returns>제단이 적을 품었으면 참이다.</returns>
    public static bool AltarContainsEnemy(RawSidPool pool, Sid dais, uint enemy, uint altarType, Func<float, float, uint, Sid> find)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(find);
        ReadOnlySpan<byte> raw = pool.Slot(dais.Value);
        float x = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(14, 4)));
        float y = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(18, 4)));
        Sid altar = find(x, y, altarType);
        ReadOnlySpan<byte> found = pool.Slot(altar.Value);
        uint argument = found[12] | ((uint)found[13] << 8);
        return argument == enemy;
    }

    /// <summary>원본 ftol의 signed64 절삭 하위 DWORD다. 무효 FISTP의 하위값은 0이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭 하위 DWORD의 float 해석이다.</returns>
    private static float Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((int)(long)Math.Truncate(value));
    }
}

// 0040f660 호출 직전에 읽은 원본 여덟 인자의 값이다. 하위가 현재 본체를 다시 읽어 대신하지 않는다.
public struct AiPriestSearchRequest
{
    /// <summary>건설 TYPE다.</summary>
    public uint Type;

    /// <summary>자기 경계다.</summary>
    public AiBuildSearchBounds Bounds;

    /// <summary>적 기준점이다.</summary>
    public RawPathPoint Avoid;

    /// <summary>소유자 DWORD다.</summary>
    public uint Owner;
}

// 같은 원본 전역의 현재값을 빌린다.
public sealed class AiPriestActionGlobals
{
    /// <summary>0055b4d0의 현재 게임 시각이다. 각 기한 쓰기 위치에서 재조회한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<double>? Now { get; set; }

    /// <summary>00531928의 현재 signed 지도 크기다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<int>? MapLimit { get; set; }

    /// <summary>005412ec의 현재 다이스 TYPE다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<uint>? DaisType { get; set; }
}

// 이미 복원된 실제 하위만 선택된 원문 위치에서 요청한다.
public sealed class AiPriestActionHooks
{
    /// <summary>0040f700의 같은 574 상태 탐색이며 prefix는 0/회수 뒤는 1이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Action<uint, uint>? Scan { get; set; }

    /// <summary>00411ca0의 현재 HP/최대 HP 판정이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<Sid, bool>? Vulnerable { get; set; }

    /// <summary>00425ed0에서 004afb40(actor,493h) 예약 존재다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<Sid, bool>? Busy { get; set; }

    /// <summary>00411630의 현재 실제 경로다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<Sid, Sid>? Path { get; set; }

    /// <summary>004adcb0의 실제 getter다. 결과가 버려져도 실행한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<Sid, int>? MaxHitPoints { get; set; }

    /// <summary>00491b30의 현재 물리 frame 의식 구간이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<Sid, bool>? Ritual { get; set; }

    /// <summary>00448a90의 현재 적/다이스 제단 argument 판정이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<uint, Sid, bool>? AltarEnemy { get; set; }

    /// <summary>004116d0의 실제 대상 이동/희생 요청이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Action<uint, Sid, Sid, uint>? Target { get; set; }

    /// <summary>0041d840의 같은 현재 발자국 signed 거리다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<RawPathPoint, Sid, double>? Distance { get; set; }

    /// <summary>00411b40의 실제 다음 건설 TYPE다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<uint, uint>? Select { get; set; }

    /// <summary>004118a0의 실제 신전 복귀다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? Home { get; set; }

    /// <summary>00411900의 실제 수집/귀환 판단이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? Collect { get; set; }

    /// <summary>004108f0의 현재 사제 raw 좌표/나머지 인자 0 실제 배치·로그·소리다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Action<uint, uint, RawPathPoint>? Deploy { get; set; }

    /// <summary>0040f660의 같은 실제 탐색 초기화다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Action<uint, AiPriestSearchRequest>? Begin { get; set; }

    /// <summary>004117e0의 실제 좌표 이동/희생 0이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Action<uint, Sid, RawPathPoint>? Point { get; set; }

    /// <summary>00411ad0의 현재 실제 적 자금 조건이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Func<uint, bool>? EnemyLowFunds { get; set; }

    /// <summary>0044c420의 실제 회수 요청이며 직접 환불/삭제로 우회하지 않는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    public Action<Sid>? Salvage { get; set; }
}

// 원본 00411cd0 전체를 같은 0xb08/실제 풀과 같은 행동 하위에서 수행한다. 자동 프레임 순서는 별도다.
// 근거: cpppj o/RawAiPriestActions.h/.cpp의 RawAiPriestActions다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.
public sealed class RawAiPriestActions
{
    /// <summary>PE005012d8/00501448/00501450의 원본 double 기한이다. 순서대로 건설 20초·탐색 3초·회수 10초다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    private static readonly double[] Delays = [20.0, 3.0, 10.0];

    /// <summary>원본 목표 미정 -10000.0f의 DWORD 비트다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    private const uint Unset = 0xc61c4000u;

    /// <summary>다이스 거리 경계다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    private const float DaisDistance = 2.0f;

    /// <summary>다른 AI와 같은 본체/목록 저장소다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 raw 위치/HP/목록 대상 원문이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 시각/지도/다이스 전역의 참조다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    private readonly AiPriestActionGlobals _globals;

    /// <summary>선택된 실제 하위 효과다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    private readonly AiPriestActionHooks _hooks;

    /// <summary>같은 PATCH 풀/실제 본체를 빌린다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="memory">다른 AI와 같은 본체/목록 저장소다.</param>
    /// <param name="pool">현재 raw 위치/HP/목록 대상 원문이다.</param>
    /// <param name="globals">현재 시각/지도/다이스 전역의 참조다.</param>
    /// <param name="hooks">선택된 실제 하위 효과다.</param>
    public RawAiPriestActions(RawAiBrainMemory memory, RawSidPool pool, AiPriestActionGlobals globals, AiPriestActionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("AI 사제 행동 같은PATCH풀 누락");
        }

        _memory = memory;
        _pool = pool;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>해제/가짜 본체를 거부한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 사제 행동 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 little endian DWORD를 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 사제 행동 DWORD 범위");
        }

        // 지정 네 BYTE만 조합하는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at, 4));
    }

    /// <summary>원본 DWORD의 signed 해석이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 ftol의 signed64 절삭 하위 DWORD다. 무효 FISTP의 하위값은 0이다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭 하위 DWORD의 signed 해석이다.</returns>
    private static int Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return Signed(unchecked((uint)(long)Math.Truncate(value)));
    }

    /// <summary>선택된 실제 하위가 없으면 앞선 효과를 보존하고 명시 실패한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 사제 행동 실제 하위 미연결");

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static double GlobalDouble(Func<double>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static int GlobalInt(Func<int>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint GlobalUint(Func<uint>? function) => Need(function)();

    /// <summary>현재 DWORD를 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>지정 필드만 갱신한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value)
    {
        byte[] raw = Body(token);
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 사제 행동 DWORD 쓰기 범위");
        }

        // 지정 네 BYTE만 쓰는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at, 4), value);
    }

    /// <summary>두 DWORD를 원본 QWORD로 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 QWORD 시각이다.</returns>
    private double Time(uint token, int at) => BitConverter.UInt64BitsToDouble(((ulong)Field(token, at + 4) << 32) | Field(token, at));

    /// <summary>현재 시각은 각 기한 쓰기 위치에서 새로 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="delay">더할 지연이다.</param>
    private void Deadline(uint token, int at, double delay)
    {
        ulong value = BitConverter.DoubleToUInt64Bits(GlobalDouble(_globals.Now) + delay);
        Field(token, at, (uint)value);
        Field(token, at + 4, (uint)(value >> 32));
    }

    /// <summary>native now→due/C0 관문과 같이 less/unordered를 모두 거부한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">기한 위치다.</param>
    /// <returns>도달했으면 참이다.</returns>
    private bool Ready(uint token, int at) => GlobalDouble(_globals.Now) >= Time(token, at);

    /// <summary>잘못된 DWORD를 다른 정상 SID로 잘라 쓰지 않는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 DWORD 값이다.</param>
    /// <returns>물리 검사를 통과한 SID다.</returns>
    private Sid Object(uint value)
    {
        if (value >= (uint)_pool.Capacity || value > 65535)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 사제 행동 실제SID 범위");
        }

        return new Sid(unchecked((ushort)value));
    }

    /// <summary>현재 raw 두 float를 값으로 저장한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="actor">조회할 SID다.</param>
    /// <returns>현재 raw 좌표다.</returns>
    private RawPathPoint Position(Sid actor)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(actor.Value);
        float x = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(14, 4)));
        float y = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(18, 4)));
        return new RawPathPoint(x, y);
    }

    /// <summary>현재 목표 원문을 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 목표 좌표다.</returns>
    private RawPathPoint Target(uint token) => new(BitConverter.UInt32BitsToSingle(Field(token, 0xad0)), BitConverter.UInt32BitsToSingle(Field(token, 0xad4)));

    /// <summary>같은 목표 원문 두 성분만 쓴다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="point">쓸 목표 좌표다.</param>
    private void Target(uint token, RawPathPoint point)
    {
        Field(token, 0xad0, BitConverter.SingleToUInt32Bits(point.X));
        Field(token, 0xad4, BitConverter.SingleToUInt32Bits(point.Y));
    }

    /// <summary>좌표 미정값은 원본처럼 y→x로 지운다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    private void ClearTarget(uint token)
    {
        Field(token, 0xad4, Unset);
        Field(token, 0xad0, Unset);
    }

    /// <summary>각 초기화 호출 시 현재 여덟 인자를 같은 본체에서 읽고 하위에 값으로 넘긴다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">초기화할 TYPE 번호다.</param>
    private void BeginSearch(uint token, uint type)
    {
        var request = new AiPriestSearchRequest
        {
            Type = type,
            Bounds = new AiBuildSearchBounds(BitConverter.UInt32BitsToSingle(Field(token, 0x270)), BitConverter.UInt32BitsToSingle(Field(token, 0x274)), BitConverter.UInt32BitsToSingle(Field(token, 0x278)), BitConverter.UInt32BitsToSingle(Field(token, 0x27c))),
            Avoid = new RawPathPoint(BitConverter.UInt32BitsToSingle(Field(token, 0x2b0)), BitConverter.UInt32BitsToSingle(Field(token, 0x2b4))),
            Owner = Field(token, 0x260),
        };
        Need(_hooks.Begin)(token, request);
    }

    /// <summary>HP는 최대 HP getter 전에 저장하고 기능 800h는 그 효과 뒤 현재값을 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="priest">돌볼 사제 SID다.</param>
    private void Fallback(uint token, Sid priest)
    {
        if (Field(token, 0x2cc) != 0)
        {
            Need(_hooks.Home)(token, priest);
            return;
        }

        int hp = Signed(BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(priest.Value).Slice(26, 4)));
        int maximum = Need(_hooks.MaxHitPoints)(priest);
        if (maximum <= hp && (Field(token, 0x248) & 0x800u) != 0)
        {
            Need(_hooks.Collect)(token, priest);
        }
        else
        {
            Need(_hooks.Home)(token, priest);
        }
    }

    /// <summary>원본 최초 count/pointer를 사용하고 같은 HP이면 앞선 SID를 유지한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>고른 건물 SID며 없으면 빈 SID다.</returns>
    private Sid BestBuilding(uint token)
    {
        int count = Signed(Field(token, 0x308));
        uint selected = 0;
        if (count > 0)
        {
            byte[] list = _memory.Bytes(Field(token, 0x300));
            // 처음 목록을 정순으로 비교하며 추가 생존 필터를 넣지 않는 반복문이다.
            for (int index = 0; index < count; index++)
            {
                uint value = Read(list, index * 4);
                if (selected == 0 || Signed(BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(Object(selected).Value).Slice(26, 4))) < Signed(BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(Object(value).Value).Slice(26, 4))))
                {
                    selected = value;
                }
            }
        }

        return Object(selected);
    }

    /// <summary>전체 prefix/각 관문/현재 재조회와 원래 요청 순서를 보존한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="priest">행동할 사제 SID다.</param>
    public void Act(uint token, Sid priest)
    {
        if (Signed(Field(token, 0x5a4)) < Signed(Field(token, 0x5a0)))
        {
            Need(_hooks.Scan)(token, 0);
            return;
        }

        if (priest.Value == 0 || !Ready(token, 0x5a8) || Need(_hooks.Vulnerable)(priest) || Need(_hooks.Busy)(priest))
        {
            return;
        }

        if (Need(_hooks.Path)(priest).Value != 0)
        {
            _ = Need(_hooks.MaxHitPoints)(priest);
            return;
        }

        if (!Ready(token, 0x5c0))
        {
            return;
        }

        if (Field(token, 0x2a4) != 0 && (Field(token, 0x248) & 0x400u) != 0)
        {
            if (Need(_hooks.Ritual)(Object(Field(token, 0x29c))))
            {
                return;
            }

            if (Need(_hooks.AltarEnemy)(Field(token, 0x264), Object(Field(token, 0x2a4))))
            {
                Sid dais = Object(Field(token, 0x2a4));
                Sid self = Object(Field(token, 0x29c));
                Need(_hooks.Target)(token, self, dais, 1);
                return;
            }

            double distance = Need(_hooks.Distance)(Position(priest), Object(Field(token, 0x2a4)));
            if (distance > DaisDistance)
            {
                Need(_hooks.Target)(token, priest, Object(Field(token, 0x2a4)), 0);
                return;
            }
        }

        uint type = Need(_hooks.Select)(token);
        if (Field(token, 0x294) != 0 && Field(token, 0x2cc) == 0 && Signed(Field(token, 0x2d8)) < 400)
        {
            Fallback(token, priest);
            return;
        }

        RawPathPoint target = Target(token);
        if (!AiPriestPoints.Valid(target, GlobalInt(_globals.MapLimit)))
        {
            if (type == 0)
            {
                Fallback(token, priest);
                return;
            }
        }
        else if (type == 0)
        {
            ClearTarget(token);
            return;
        }

        RawPathPoint snapped = AiPriestPoints.Snap(Position(priest));
        RawPathPoint current = Target(token);
        if (snapped.X == current.X && snapped.Y == current.Y)
        {
            Deadline(token, 0x5c0, Delays[0]);
            Need(_hooks.Deploy)(token, type, Position(priest));
            ClearTarget(token);
            return;
        }

        if (!Ready(token, 0x5c8))
        {
            return;
        }

        if (type == Field(token, 0x574))
        {
            if (Signed(Field(token, 0x5a0)) <= Signed(Field(token, 0x5a4)))
            {
                Target(token, new RawPathPoint(BitConverter.UInt32BitsToSingle(Field(token, 0x57c)), BitConverter.UInt32BitsToSingle(Field(token, 0x580))));
                Field(token, 0x574, 0);
                Target(token, AiPriestPoints.Snap(Target(token)));
                if (AiPriestPoints.Valid(Target(token), GlobalInt(_globals.MapLimit)))
                {
                    Need(_hooks.Point)(token, priest, Target(token));
                    return;
                }
            }
        }
        else
        {
            BeginSearch(token, type);
        }

        if (type == GlobalUint(_globals.DaisType) && Signed(Field(token, 0x5a4)) >= Signed(Field(token, 0x5a0)) && !AiPriestPoints.Valid(Target(token), GlobalInt(_globals.MapLimit)) && Time(token, 0x5e8) == 0.0)
        {
            Sid selected = BestBuilding(token);
            if (selected.Value == 0)
            {
                if (!Need(_hooks.EnemyLowFunds)(token))
                {
                    Deadline(token, 0x5c8, Delays[1]);
                    return;
                }

                selected = Object(Field(token, 0x294));
                if (selected.Value == 0)
                {
                    Deadline(token, 0x5c8, Delays[1]);
                    return;
                }
            }

            Need(_hooks.Target)(token, priest, selected, 0);
            Need(_hooks.Salvage)(selected);
            Deadline(token, 0x5e8, Delays[2]);
            BeginSearch(token, type);
            Need(_hooks.Scan)(token, 1);
        }

        Deadline(token, 0x5c8, Delays[1]);
    }

    /// <summary>같은 실제 월드 연결을 확인한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
