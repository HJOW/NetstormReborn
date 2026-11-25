// 원본 네 다리 전략의 공통 관문·목록 이동·현재 목표·확장 순서를 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 다리 전략 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.

// 같은 현재 전역을 각 원본 조회 위치에서 읽는다.
public sealed class AiBridgeStrategyGlobals
{
    /// <summary>0055b4d0의 현재 QWORD 게임 시각이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<double>? Now { get; set; }

    /// <summary>00531928을 float로 저장한 뒤 좌표 유효성을 검사한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<int>? MapLimit { get; set; }

    /// <summary>00540410의 현재 DWORD이며 BYTE 장부도 같은 getter로 연결한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<uint>? InvalidGraph { get; set; }

    /// <summary>005e4794의 단락 관문이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<bool>? Checking { get; set; }

    /// <summary>0054db88의 단락 관문이다. 후자는 checking일 때만 읽는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<bool>? Disabled { get; set; }

    /// <summary>0052f3e4의 현재 간헐천 로그 종류다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<uint>? GeyserLog { get; set; }
}

// 기존 간헐천의 재등록/포기/시도 세 원본 문구다.
public enum AiBridgeStrategyLog
{
    /// <summary>재등록 로그다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    Requeued,

    /// <summary>포기 로그다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    Abandoned,

    /// <summary>시도 로그다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    Trying,
}

// 이미 복원된 실제 월드/계획/경로 하위와 현재 장치 경계만 연결한다.
public sealed class AiBridgeStrategyHooks
{
    /// <summary>0048fc90의 현재 owner 기술 부모다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<uint, Sid>? Technology { get; set; }

    /// <summary>0040fcb0의 현재 다리 재고 스캔/보충이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? Refresh { get; set; }

    /// <summary>00462bc0의 현재 실제 object graph다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<Sid, uint>? Graph { get; set; }

    /// <summary>004adea0의 현재 float 중심이다. 원본 0040f330의 거의 올림은 전략 몸체가 적용한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<Sid, RawPathPoint>? Center { get; set; }

    /// <summary>00410d40의 현재 간헐천 가장자리 연결이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<uint, Sid, bool>? Connected { get; set; }

    /// <summary>00411310의 현재 count별 40/120초 기한이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Action<uint>? Delay { get; set; }

    /// <summary>004112b0의 현재 신전 중심/fallback이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<uint, RawPathPoint>? Home { get; set; }

    /// <summary>00411350의 현재 연결 해제 간헐천 재분류다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Action<uint>? Reclassify { get; set; }

    /// <summary>00410a30의 같은 348 목록 선택이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<uint, RawPathPoint, uint, uint, Sid>? Select { get; set; }

    /// <summary>00411470의 현재 열린 지면 20회 탐색이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<uint, AiBuildSearchBounds, RawPathPoint>? RandomStart { get; set; }

    /// <summary>004174c0의 현재 3d0 다리 시작/연장/timeout이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<uint, RawPathPoint, bool>? Advance { get; set; }

    /// <summary>004173b0의 동일 배치/소비/보충이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<uint, RawPathPoint, RawPathPoint, RawPathPoint>? Extend { get; set; }

    /// <summary>004119b0의 inclusive 빈 spot 사각형이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Func<int, int, int, int, bool>? Rectangle { get; set; }

    /// <summary>004e0620의 선택된 현재 진단이며 반환하면 원본처럼 계속한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Action<string, uint>? Assertion { get; set; }

    /// <summary>004105b0의 현재 B00/장치 관문에 세 로그를 전달한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    public Action<uint, uint, AiBridgeStrategyLog, uint>? Log { get; set; }
}

// 00417bd0 입구와 네 case 전체를 동일 실제 본체/목록/난수에 적용한다.
// 근거: cpppj o/RawAiBridgeStrategies.h/.cpp의 RawAiBridgeStrategies다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.
public sealed class RawAiBridgeStrategies
{
    /// <summary>0041d7a0의 00502310 실제 float 상수다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    private const float PointBias = 0.99999F;

    /// <summary>기존 AI와 같은 본체/목록의 유일 소유자다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 종류/생사/좌표를 읽는 동일 풀이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 실제 188 TYPE의 genus다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>다른 모듈과 같은 게임 공용 난수다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    private readonly GameRandom _random;

    /// <summary>현재 전역의 참조/getter다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    private readonly AiBridgeStrategyGlobals _globals;

    /// <summary>선택된 실제 하위 효과다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    private readonly AiBridgeStrategyHooks _hooks;

    /// <summary>같은 PATCH188 세계와 현재 전역을 빌린다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 본체/목록의 저장소다.</param>
    /// <param name="pool">현재 종류/생사/좌표를 읽는 풀이다.</param>
    /// <param name="types">현재 188 TYPE의 genus 표다.</param>
    /// <param name="random">같은 게임 공용 난수다.</param>
    /// <param name="globals">현재 전역이다.</param>
    /// <param name="hooks">선택된 실제 하위다.</param>
    public RawAiBridgeStrategies(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, GameRandom random, AiBridgeStrategyGlobals globals, AiBridgeStrategyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)
        {
            throw new ArgumentException("AI 다리 전략 실제 PATCH188 누락");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _random = random;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>비정렬 DWORD의 주변을 읽지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트 범위다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int at)
    {
        if (at < 0 || at > raw.Length || raw.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 다리 전략 DWORD 범위");
        }

        // 네 BYTE만 합치는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(at, 4));
    }

    /// <summary>현재 지정 DWORD만 쓴다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Write(Span<byte> raw, int at, uint value)
    {
        if (at < 0 || at > raw.Length || raw.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 다리 전략 DWORD 쓰기 범위");
        }

        // 주변 바이트를 보존하는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(at, 4), value);
    }

    /// <summary>원문 DWORD의 signed 비교다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>실제 __ftol signed64 하위 DWORD이며 indefinite 하위는 0이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭한 낮은 DWORD다.</returns>
    private static uint Truncate(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((uint)(long)value);
    }

    /// <summary>선택된 하위 누락을 빈 성공으로 바꾸지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 다리 전략 실제 하위 미연결");

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static double GlobalDouble(Func<double>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static bool GlobalBool(Func<bool>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static int GlobalInt(Func<int>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint GlobalUint(Func<uint>? function) => Need(function)();

    /// <summary>0041d610은 사각형을 수정하지 않고 고정 256 경계에서 margin 5를 엄격히 검사한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="bounds">검사할 사각형이다.</param>
    /// <returns>안쪽이면 참이다.</returns>
    private static bool Interior(AiBuildSearchBounds bounds) => bounds.Left > 5.0f && bounds.Top > 5.0f && bounds.Right < 251.0f && bounds.Bottom < 251.0f;

    /// <summary>현재 살아 있는 같은 본체를 다시 조회한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 다리 전략 실제0xb08 본체 누락");
        }

        return raw;
    }

    /// <summary>외부 효과 뒤의 현재 DWORD다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>원본이 쓰는 네 BYTE만 바꾼다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value) => Write(Body(token), at, value);

    /// <summary>NaN/부호 0도 비트 그대로 읽는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>원문 비트의 좌표다.</returns>
    private RawPathPoint Point(uint token, int at) => new(
        BitConverter.UInt32BitsToSingle(Field(token, at)),
        BitConverter.UInt32BitsToSingle(Field(token, at + 4)));

    /// <summary>같은 실제 본문의 두 축만 갱신한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 좌표다.</param>
    private void Point(uint token, int at, RawPathPoint value)
    {
        Field(token, at, BitConverter.SingleToUInt32Bits(value.X));
        Field(token, at + 4, BitConverter.SingleToUInt32Bits(value.Y));
    }

    /// <summary>잘못된 번호를 다른 정상 WORD로 감싸지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 DWORD 값이다.</param>
    /// <returns>물리 검사를 통과한 SID다.</returns>
    private Sid Object(uint value)
    {
        if (value >= (uint)_pool.Capacity || value > 65535)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 다리 전략 실제 SID 범위");
        }

        return new Sid(unchecked((ushort)value));
    }

    /// <summary>free/owner 필터를 genus 조회에 추가하지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="value">조회할 목록 DWORD다.</param>
    /// <returns>현재 raw 타입의 flags2다.</returns>
    private uint Genus(uint value)
    {
        byte kind = _pool.Slot(Object(value).Value)[RawSidPool.TypeOffset];
        if (kind >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 다리 전략 TYPE 범위");
        }

        return _types[kind].Flags2;
    }

    /// <summary>목록 외부의 호스트 메모리 접근만 명시 거부한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <param name="index">고를 번호다.</param>
    /// <returns>현재 항목 값이다.</returns>
    private uint Item(uint token, int list, int index)
    {
        if (index < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "AI 다리 전략 음수 목록 인덱스");
        }

        return Read(_memory.Bytes(Field(token, list)), index * 4);
    }

    /// <summary>native 0040e9b0과 같이 count/용량을 먼저 읽고 full이면 포인터를 읽지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <param name="value">추가할 값이다.</param>
    private void Append(uint token, int list, uint value)
    {
        int count = Signed(Field(token, list + 8));
        if (count >= Signed(Field(token, list + 4)))
        {
            return;
        }

        if (count < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 다리 전략 음수 추가 위치");
        }

        Write(_memory.Bytes(Field(token, list)), count * 4, value);
        Field(token, list + 8, unchecked((uint)count) + 1u);
    }

    /// <summary>네이티브 안정 압축과 미사용 꼬리 보존을 유지한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <param name="value">지울 값이다.</param>
    private void Remove(uint token, int list, uint value)
    {
        uint removed = 0;
        // 현재 active 전체에서 모든 동일 값을 제거하는 반복문이다.
        for (int index = 0; index < Signed(Field(token, list + 8)); index++)
        {
            uint item = Item(token, list, index);
            if (item == value)
            {
                removed++;
            }
            else
            {
                Write(_memory.Bytes(Field(token, list)), unchecked((index - (int)removed) * 4), item);
            }
        }

        Field(token, list + 8, Field(token, list + 8) - removed);
    }

    /// <summary>네 case는 refresh 뒤 현재 신전/graph/기한을 다시 검사하며 unordered는 실행하지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="deadline">읽을 기한 위치다.</param>
    /// <returns>도달했으면 참이다.</returns>
    private bool Ready(uint token, int deadline)
    {
        if (Field(token, 0x294) == 0 || Field(token, 0x298) == GlobalUint(_globals.InvalidGraph))
        {
            return false;
        }

        double due = BitConverter.UInt64BitsToDouble(((ulong)Field(token, deadline + 4) << 32) | Field(token, deadline));
        return GlobalDouble(_globals.Now) >= due;
    }

    /// <summary>원본 지도 상한은 float로 저장한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="point">검사할 좌표다.</param>
    /// <returns>유효하면 참이다.</returns>
    private bool Valid(RawPathPoint point)
    {
        float limit = GlobalInt(_globals.MapLimit);
        return point.X > 0 && point.X < limit && point.Y > 0 && point.Y < limit;
    }

    /// <summary>실제 중심 float를 더 정밀한 중간 합으로 거의 올림한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="value">중심을 잴 SID DWORD다.</param>
    /// <returns>절삭한 중심 좌표다.</returns>
    private RawPathPoint RoundedCenter(uint value)
    {
        RawPathPoint point = Need(_hooks.Center)(Object(value));
        return new RawPathPoint(
            (float)Signed(Truncate((double)point.X + PointBias)),
            (float)Signed(Truncate((double)point.Y + PointBias)));
    }

    /// <summary>원본 필드 쓰기 순서를 유지한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    private void ResetPoint(uint token)
    {
        Field(token, 0x3d4, 0xc61c4000u);
        Field(token, 0x3d0, 0xc61c4000u);
    }

    /// <summary>목표 1: 유효한 기존 적 신전을 진행한 호출에서는 성공값으로 추가 상태를 만들지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    private void Temple(uint token)
    {
        if (!Ready(token, 0x5a8))
        {
            return;
        }

        uint target = Field(token, 0x3cc);
        if (target != 0 && (_pool.Slot(Object(target).Value)[RawSidPool.StateOffset] & 2) == 0 && (Genus(target) & 0x200u) != 0 && Need(_hooks.Graph)(Object(Field(token, 0x3cc))) != Field(token, 0x298))
        {
            RawPathPoint point = RoundedCenter(Field(token, 0x3cc));
            _ = Need(_hooks.Advance)(token, point);
            return;
        }

        Field(token, 0x3cc, Field(token, 0x2a8));
        ResetPoint(token);
    }

    /// <summary>목표 2: 현재 대상 연결→포기 목록 재등록→진행/실패 또는 새 대상 선택의 원래 순서다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    private void Geysers(uint token)
    {
        if (!Ready(token, 0x5a8))
        {
            return;
        }

        uint target = Field(token, 0x3cc);
        bool valid = target != 0 && (_pool.Slot(Object(target).Value)[RawSidPool.StateOffset] & 2) == 0 && (Genus(target) & 0x10000000u) != 0;
        bool connected = false;
        if (valid && (connected = Need(_hooks.Connected)(token, Object(Field(token, 0x3cc)))))
        {
            Remove(token, 0x348, Field(token, 0x3cc));
            Append(token, 0x330, Field(token, 0x3cc));
        }

        double due = BitConverter.UInt64BitsToDouble(((ulong)Field(token, 0x5d4) << 32) | Field(token, 0x5d0));
        if (Signed(Field(token, 0x35c)) > 0 && GlobalDouble(_globals.Now) >= due)
        {
            uint retry = Item(token, 0x354, 0);
            Need(_hooks.Delay)(token);
            Remove(token, 0x354, retry);
            Append(token, 0x348, retry);
            Need(_hooks.Log)(token, GlobalUint(_globals.GeyserLog), AiBridgeStrategyLog.Requeued, retry);
        }

        if (valid && !connected)
        {
            RawPathPoint point = RoundedCenter(Field(token, 0x3cc));
            if (Need(_hooks.Advance)(token, point))
            {
                return;
            }

            uint abandoned = Field(token, 0x3cc);
            Append(token, 0x354, abandoned);
            Need(_hooks.Delay)(token);
            Remove(token, 0x348, abandoned);
            Field(token, 0x3cc, 0);
            ResetPoint(token);
            Field(token, 0x3d8, 1);
            Need(_hooks.Log)(token, GlobalUint(_globals.GeyserLog), AiBridgeStrategyLog.Abandoned, abandoned);
            return;
        }

        RawPathPoint home = Need(_hooks.Home)(token);
        Need(_hooks.Reclassify)(token);
        uint invalid = GlobalUint(_globals.InvalidGraph);
        Sid selected = Need(_hooks.Select)(token, home, invalid, invalid);
        Field(token, 0x3cc, selected.Value);
        if (selected.Value != 0)
        {
            Need(_hooks.Log)(token, GlobalUint(_globals.GeyserLog), AiBridgeStrategyLog.Trying, selected.Value);
        }

        ResetPoint(token);
    }

    /// <summary>목표 3: 주변 3칸/자기 영역의 열린 시작점에서 무작위 목표로 연장한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    private void Expand(uint token)
    {
        if (!Ready(token, 0x5b0) || Field(token, 0x2a8) == 0 || Signed(Field(token, 0x3c8)) < 1)
        {
            return;
        }

        int index = Signed(_random.Next(unchecked(Field(token, 0x2f0) + 1u)));
        RawPathPoint source = default;
        if (index < Signed(Field(token, 0x2f0)))
        {
            uint target = Item(token, 0x2e8, index);
            ReadOnlySpan<byte> raw = _pool.Slot(Object(target).Value);
            float x = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(14, 4)));
            float y = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(18, 4)));
            var bounds = new AiBuildSearchBounds(
                (float)((double)x - 3.0),
                (float)((double)y - 3.0),
                (float)((double)x + 3.0),
                (float)((double)y + 3.0));
            source = new RawPathPoint(bounds.Right, bounds.Bottom);
            if (Interior(bounds))
            {
                source = Need(_hooks.RandomStart)(token, bounds);
            }
            else
            {
                Remove(token, 0x2e8, target);
            }
        }
        else
        {
            var bounds = new AiBuildSearchBounds(
                BitConverter.UInt32BitsToSingle(Field(token, 0x270)),
                BitConverter.UInt32BitsToSingle(Field(token, 0x274)),
                BitConverter.UInt32BitsToSingle(Field(token, 0x278)),
                BitConverter.UInt32BitsToSingle(Field(token, 0x27c)));
            source = Need(_hooks.RandomStart)(token, bounds);
        }

        if (!Valid(source))
        {
            return;
        }

        int dy = Signed(_random.Next(16u)) - 8;
        int dx = Signed(_random.Next(16u)) - 8;
        var targetPoint = new RawPathPoint((float)((double)source.X + (float)dx), (float)((double)source.Y + (float)dy));
        RawPathPoint next = Need(_hooks.Extend)(token, source, targetPoint);
        Point(token, 0x3d0, next);
        RawPathPoint current = Point(token, 0x3d0);
        if (!(current.X == source.X && current.Y == source.Y))
        {
            Field(token, 0x3c8, Field(token, 0x3c8) - 1u);
        }
    }

    /// <summary>진단 뒤 현재 좌표를 다시 읽어 inclusive 사각형과 실제 Advance를 호출한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    private void Coordinate(uint token)
    {
        if (!Ready(token, 0x5b0))
        {
            return;
        }

        if (!Valid(Point(token, 0xaf0)))
        {
            Need(_hooks.Assertion)("sendBridgeHere.isValid()", 0xa3a);
        }

        uint x = Truncate(BitConverter.UInt32BitsToSingle(Field(token, 0xaf0)));
        uint y = Truncate(BitConverter.UInt32BitsToSingle(Field(token, 0xaf4)));
        if (!Need(_hooks.Rectangle)(Signed(unchecked(x - 1u)), Signed(unchecked(y - 1u)), Signed(unchecked(x + 1u)), Signed(unchecked(y + 1u))) || !Need(_hooks.Advance)(token, Point(token, 0xaf0)))
        {
            Field(token, 0xaf4, 0xc61c4000u);
            Field(token, 0xaf0, 0xc61c4000u);
        }
    }

    /// <summary>현재 목표는 재고 갱신의 효과 뒤 조회한다. 미지원 목표도 선행 재고 효과는 보존한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void Update(uint token)
    {
        _ = Body(token);
        if (Field(token, 0x294) == 0 || Field(token, 0x298) == GlobalUint(_globals.InvalidGraph) || (Field(token, 0x248) & 0x1000u) == 0 || (GlobalBool(_globals.Checking) && GlobalBool(_globals.Disabled)))
        {
            return;
        }

        Sid parent = Need(_hooks.Technology)(Field(token, 0x260));
        Need(_hooks.Refresh)(token, parent);
        switch (Field(token, 0x2d4))
        {
            case 1:
                Temple(token);
                break;
            case 2:
                Geysers(token);
                break;
            case 3:
                Expand(token);
                break;
            case 4:
                Coordinate(token);
                break;
            default:
                break;
        }
    }

    /// <summary>실제 월드 조립 확인용 같은 풀이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
