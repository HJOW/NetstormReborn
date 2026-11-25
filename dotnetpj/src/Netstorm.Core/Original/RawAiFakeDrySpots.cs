// 공급 시설/현재 부족 거리와 원본 양방향 탐색으로 AI 가짜 부족 좌표를 갱신한다.
using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Core.Original;

// AI 가짜 부족 좌표 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.

// 004105b0의 두 원래 알림을 구분한다. 종류/owner는 실제 출력 시점의 값을 사용한다.
public enum AiFakeDryLog
{
    /// <summary>버려진 후보 알림이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    Discarded,

    /// <summary>채택한 후보 알림이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    Added,
}

// 원본 공급 반경/지도 크기/디버그 종류의 현재 저장소를 빌린다.
public sealed class AiFakeDryGlobals
{
    /// <summary>0052f488의 현재 반경이며 제곱 반경이 아니다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Func<float>? ManaRadius { get; set; }

    /// <summary>00531928을 각 좌표 유효성 검사에서 float로 바꾼다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Func<int>? MapLimit { get; set; }

    /// <summary>0052f3dc가 29면 채택 뒤 실제 표식을 추가한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Func<uint>? Debug { get; set; }
}

// 00412640의 실제 부족 거리와 선택 index다.
public delegate float AiFakeDryScore(uint token, RawPathPoint at, ref int dryIndex);

// 이미 복원한 실제 거리/중심/경로/목록과 원래 출력만 연결한다.
public sealed class AiFakeDryHooks
{
    /// <summary>00412640의 실제 부족 거리와 선택 index다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public AiFakeDryScore? Score { get; set; }

    /// <summary>004adea0의 현재 실제 시설 중심이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Func<Sid, RawPathPoint>? Center { get; set; }

    /// <summary>00415bb0의 실제 양방향 탐색이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Func<RawPathPoint, RawPathPoint, RawPathPoint>? Candidate { get; set; }

    /// <summary>0040f1c0의 같은 Dry Spot Add다. 반환은 무시한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Func<uint, RawPathPoint, uint, bool>? Add { get; set; }

    /// <summary>0040f260의 같은 index 0 안정 압축이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Action<uint, int>? Remove { get; set; }

    /// <summary>004133e0의 같은 실제 공용 난수/경계 좌표 선택이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Action<uint>? Seed { get; set; }

    /// <summary>00410540의 현재 TYPE/실제 표식 수명이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Action<RawPathPoint>? Mark { get; set; }

    /// <summary>004105b0의 현재 b00 비영인 위치에서만 요청한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    public Action<uint, AiFakeDryLog, int, int>? Log { get; set; }
}

// 실제 본체/목록만 읽고 선택된 하위 뒤 현재 필드를 다시 조회한다.
// 근거: cpppj o/RawAiFakeDrySpots.h/.cpp의 RawAiFakeDrySpots다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.
public sealed class RawAiFakeDrySpots
{
    /// <summary>00501784의 float 비트 3f333333이다. 문자열 조각이 아니다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    private const float RadiusFactor = 0.7f;

    /// <summary>전략 4의 미정 좌표 -10000이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    private const uint Unset = 0xc61c4000u;

    /// <summary>원본 표식 활성 종류다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    private const uint DebugKind = 29;

    /// <summary>부족 좌표는 x/y float와 TYPE DWORD다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    private const int DryStride = 12;

    /// <summary>실제 본체/공급 시설 목록/12바이트 Dry Spot의 같은 저장소다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>같은 실제 시설/신전 슬롯의 PATCH 풀이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 반경/지도 크기/디버그 참조다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    private readonly AiFakeDryGlobals _globals;

    /// <summary>선택된 실제 하위와 원래 로그다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    private readonly AiFakeDryHooks _hooks;

    /// <summary>생성 중 지도 탐색이나 목록 효과를 수행하지 않는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="memory">실제 본체/공급 시설 목록/12바이트 Dry Spot의 같은 저장소다.</param>
    /// <param name="pool">같은 실제 시설/신전 슬롯의 PATCH 풀이다.</param>
    /// <param name="globals">현재 반경/지도 크기/디버그 참조다.</param>
    /// <param name="hooks">선택된 실제 하위와 원래 로그다.</param>
    public RawAiFakeDrySpots(RawAiBrainMemory memory, RawSidPool pool, AiFakeDryGlobals globals, AiFakeDryHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("AI 가짜좌표의 같은 PATCH 풀 누락");
        }

        _memory = memory;
        _pool = pool;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>작은 가짜 본체/해제 토큰을 성공으로 취급하지 않는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 가짜좌표 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>실제 물리 블록 안의 비정렬 little endian DWORD를 읽는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 가짜좌표 DWORD 읽기 범위");
        }

        // 지정 네 BYTE만 합치는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at, 4));
    }

    /// <summary>원본 DWORD의 signed 비교를 보존한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>CRT signed64 절삭의 하위 DWORD이며 무효 FISTP의 하위값은 0이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
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

    /// <summary>도달한 실제 하위가 없으면 앞선 쓰기를 유지하고 그 위치에서 거부한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 가짜좌표 실제 하위 미연결");

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static float GlobalFloat(Func<float>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static int GlobalInt(Func<int>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint GlobalUint(Func<uint>? function) => Need(function)();

    /// <summary>현재 본체를 하위 효과 뒤 다시 빌린다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>현재 지정 필드의 네 바이트만 기록한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value)
    {
        byte[] raw = Body(token);
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 가짜좌표 DWORD 쓰기 범위");
        }

        // 주변 미정 바이트를 보존하는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at, 4), value);
    }

    /// <summary>풀 밖 SID를 잘린 WORD의 다른 객체로 바꾸지 않는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="number">바꿀 DWORD 값이다.</param>
    /// <returns>물리 검사를 통과한 SID다.</returns>
    private Sid Object(uint number)
    {
        if (number > 65535 || number >= (uint)_pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(number), "AI 가짜좌표 현재 SID 물리 범위");
        }

        return new Sid(unchecked((ushort)number));
    }

    /// <summary>중심 계산과 raw 위치 조회의 원래 차이를 유지한다. 2026-11-25 R4-5b 가짜 좌표 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 SID다.</param>
    /// <returns>현재 슬롯의 raw X/Y 좌표다.</returns>
    private RawPathPoint Position(Sid sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid.Value);
        uint x = BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(14, 4));
        uint y = BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(18, 4));
        return new RawPathPoint(BitConverter.UInt32BitsToSingle(x), BitConverter.UInt32BitsToSingle(y));
    }

    /// <summary>0040e800: 두 float 모두 0보다 크고 현재 mapLimit의 float 저장값보다 작아야 한다. NaN은 거짓이다. 근거는 cpppj o/RawAiPriestActions.h의 AiPriestPointValid다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="point">검사할 좌표다.</param>
    /// <param name="mapLimit">현재 지도 상한이다.</param>
    /// <returns>유효하면 참이다.</returns>
    private static bool PriestPointValid(RawPathPoint point, int mapLimit)
    {
        float limit = mapLimit;
        return point.X > 0 && point.X < limit && point.Y > 0 && point.Y < limit;
    }

    /// <summary>원본은 로그가 꺼져도 두 ftol 인자를 먼저 평가한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="action">기록할 알림 종류다.</param>
    /// <param name="point">기록할 좌표다.</param>
    private void Log(uint token, AiFakeDryLog action, RawPathPoint point)
    {
        int y = Ftol(point.Y);
        int x = Ftol(point.X);
        if (Field(token, 0xb00) != 0)
        {
            Need(_hooks.Log)(token, action, x, y);
        }
    }

    /// <summary>현재 TYPE가 없으면 full 검사조차 하지 않으며, full 제거는 다른 모든 하위보다 먼저다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void Extend(uint token)
    {
        if (Field(token, 0x280) == 0)
        {
            return;
        }

        if (Signed(Field(token, 0xae0)) >= Signed(Field(token, 0xadc)))
        {
            Need(_hooks.Remove)(token, 0);
        }

        var point = new RawPathPoint();
        if (Field(token, 0x2fc) == 0)
        {
            uint temple = Field(token, 0x294);
            if (temple == 0)
            {
                return;
            }

            point = Position(Object(temple));
            _ = Need(_hooks.Add)(token, point, Field(token, 0x280));
        }
        else
        {
            bool added = false;
            float best = 0;
            int dryIndex = 0;
            int selectedIndex = 0;
            uint selected = 0;
            if (Signed(Field(token, 0x2fc)) > 0)
            {
                int count = Signed(Field(token, 0x2fc));
                uint pointer = Field(token, 0x2f4);
                // 진입 count/포인터를 유지하며 raw 위치/부족 거리는 각 항목에서 다시 조회하는 반복문이다.
                for (int index = 0; index < count; index++)
                {
                    uint sid = Read(_memory.Bytes(pointer), index * 4);
                    float score = Need(_hooks.Score)(token, Position(Object(sid)), ref dryIndex);
                    if (score > best)
                    {
                        best = score;
                        selectedIndex = dryIndex;
                        selected = sid;
                    }
                }

                if (best > 0)
                {
                    RawPathPoint center = Need(_hooks.Center)(Object(selected));
                    byte[] records = _memory.Bytes(Field(token, 0xae4));
                    if (selectedIndex < 0)
                    {
                        throw new ArgumentOutOfRangeException(nameof(token), "AI 가짜좌표 선택 Dry Spot 음수 index");
                    }

                    int at = selectedIndex * DryStride;
                    var target = new RawPathPoint((float)((double)BitConverter.UInt32BitsToSingle(Read(records, at)) - 1.0), (float)((double)BitConverter.UInt32BitsToSingle(Read(records, at + 4)) - 1.0));
                    point = Need(_hooks.Candidate)(new RawPathPoint(center.Y, center.Y), target);
                    if (PriestPointValid(point, GlobalInt(_globals.MapLimit)))
                    {
                        if (AiPointDistanceHelper.Distance(point, center) > (double)GlobalFloat(_globals.ManaRadius) * RadiusFactor)
                        {
                            _ = Need(_hooks.Add)(token, point, Field(token, 0x280));
                            bool wandering = Field(token, 0x2d4) == 4;
                            Field(token, 0xaf0, BitConverter.SingleToUInt32Bits(point.X));
                            Field(token, 0xaf4, BitConverter.SingleToUInt32Bits(point.Y));
                            if (wandering)
                            {
                                Field(token, 0x3cc, 0);
                                Field(token, 0x3d4, Unset);
                                Field(token, 0x3d0, Unset);
                                Field(token, 0x3d8, 1);
                            }

                            if (GlobalUint(_globals.Debug) == DebugKind)
                            {
                                Need(_hooks.Mark)(point);
                            }

                            added = true;
                        }
                        else
                        {
                            Log(token, AiFakeDryLog.Discarded, point);
                        }
                    }
                }
            }

            RawPathPoint current = new(BitConverter.UInt32BitsToSingle(Field(token, 0xaf0)), BitConverter.UInt32BitsToSingle(Field(token, 0xaf4)));
            if (!PriestPointValid(current, GlobalInt(_globals.MapLimit)))
            {
                Need(_hooks.Seed)(token);
            }

            if (!added)
            {
                return;
            }
        }

        Log(token, AiFakeDryLog.Added, point);
    }

    /// <summary>같은 실제 월드의 풀인지 확인한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
