// AI 다리의 단계별 탐색·조각 소비·연결점 진행을 원본 본체에 적용한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 다리 경로 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.

// 원본 전역의 현재 값을 각 효과 위치에서 다시 읽는다.
public sealed class AiBridgePathGlobals
{
    /// <summary>0055b4d0의 현재 게임 시각이다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<double>? Now { get; set; }

    /// <summary>00531928의 좌표 상한이며 float로 변환해 검사한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<int>? MapLimit { get; set; }

    /// <summary>005411a0의 실제 다리 TYPE다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<uint>? BridgeType { get; set; }

    /// <summary>0052f3cc가 29이면 디버그 표식도 배치한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<uint>? LogKind { get; set; }

    /// <summary>0054121c의 시작점 표식 TYPE다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<uint>? StartMarker { get; set; }

    /// <summary>00541234의 연결점 표식 TYPE다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<uint>? EndMarker { get; set; }

    /// <summary>0054122c의 목표점 표식 TYPE다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<uint>? TargetMarker { get; set; }
}

// 탐색의 하위 알고리즘과 기존 실제 배치·기술부모·수명 경계를 연결한다.
public sealed class AiBridgePathHooks
{
    /// <summary>0040fde0의 같은 5f0 탐색 상태 초기화다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Action<uint, RawPathPoint, RawPathPoint>? Initialize { get; set; }

    /// <summary>0040fe90의 현재 후보 준비의 원래 완료값이다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<uint, bool>? Prepare { get; set; }

    /// <summary>004144b0의 현재 다리 후보 탐색의 원래 완료값이다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<uint, bool>? Search { get; set; }

    /// <summary>00410b10의 현재 그래프의 다리 시작점 탐색이다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<uint, RawPathPoint, RawPathPoint>? Start { get; set; }

    /// <summary>004108f0의 일반 다리 배치와 통지다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Action<uint, uint, RawPathPoint, uint, uint>? Deploy { get; set; }

    /// <summary>004106e0의 argument 0/방향 0의 표식 배치다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Action<uint, uint, RawPathPoint>? Marker { get; set; }

    /// <summary>0048fc90의 현재 owner의 기술 부모다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Func<uint, Sid>? Technology { get; set; }

    /// <summary>0040fcb0의 같은 실제 다리 재고를 다시 읽고 보충한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? Refresh { get; set; }

    /// <summary>첫 일치 조각의 가상+10(flags 0) 삭제다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Action<Sid, uint>? Destroy { get; set; }

    /// <summary>004105b0의 현재 종류/본체 출력 관문 뒤 실제 로그다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    public Action<uint, uint, string>? Log { get; set; }
}

// 0040fd30/004167a0/004173b0/004174c0을 같은 본체와 내용물 풀에서 수행한다.
// 근거: cpppj o/RawAiBridgePath.h/.cpp의 RawAiBridgePath다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.
public sealed class RawAiBridgePath
{
    /// <summary>같은 본체/목록/현재 월드 저장소다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>같은 현재 내용물 슬롯 풀이다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 권한·내용물 TYPE를 공유한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    private readonly ContainedFinderState _finder;

    /// <summary>현재 시각/TYPE/로그 전역의 참조다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    private readonly AiBridgePathGlobals _globals;

    /// <summary>도달한 실제 하위 효과만 호출한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    private readonly AiBridgePathHooks _hooks;

    /// <summary>PATCH 풀과 현재 원본 저장소를 빌린다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 본체/목록/현재 월드 저장소다.</param>
    /// <param name="pool">같은 현재 내용물 슬롯 풀이다.</param>
    /// <param name="finder">현재 권한·내용물 TYPE를 공유한다.</param>
    /// <param name="globals">현재 시각/TYPE/로그 전역이다.</param>
    /// <param name="hooks">도달한 실제 하위 효과다.</param>
    public RawAiBridgePath(RawAiBrainMemory memory, RawSidPool pool, ContainedFinderState finder, AiBridgePathGlobals globals, AiBridgePathHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(finder);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("AI 다리 경로 PATCH 풀 누락");
        }

        _memory = memory;
        _pool = pool;
        _finder = finder;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>가짜/해제 토큰은 즉시 거부한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 다리 경로 실제0xb08 본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 little endian DWORD를 원래 BYTE 순서로 조합한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 다리 경로 DWORD 읽기 범위");
        }

        // 지정 네 BYTE만 읽는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at, 4));
    }

    /// <summary>원본 DWORD의 signed 비교를 보존한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 004e49c0은 signed64로 0쪽 절삭한 낮은 DWORD를 반환한다. 비유한/범위 밖의 indefinite 하위는 0이다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 float다.</param>
    /// <returns>절삭한 낮은 DWORD다.</returns>
    private static uint Truncate(float value)
    {
        if (!float.IsFinite(value) || (double)value < -9223372036854775808.0 || (double)value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((uint)(long)value);
    }

    /// <summary>도달한 하위가 없으면 앞선 효과를 유지하고 해당 위치에서 오류를 알린다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 다리 경로 실제 하위 미연결");

    /// <summary>현재 본체의 DWORD를 다시 조회한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>본체의 지정 필드 폭만 쓴다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value)
    {
        byte[] raw = Body(token);
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 다리 경로 DWORD 쓰기 범위");
        }

        // 상위와 하위 필드를 보존하며 네 BYTE를 기록하는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at, 4), value);
    }

    /// <summary>두 좌표의 signed zero와 무효 좌표도 그대로 읽는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>원문 비트의 좌표다.</returns>
    private RawPathPoint Point(uint token, int at) => new(
        BitConverter.UInt32BitsToSingle(Field(token, at)),
        BitConverter.UInt32BitsToSingle(Field(token, at + 4)));

    /// <summary>원래 XY 순서대로 결과를 쓴다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 좌표다.</param>
    private void Point(uint token, int at, RawPathPoint value)
    {
        Field(token, at, BitConverter.SingleToUInt32Bits(value.X));
        Field(token, at + 4, BitConverter.SingleToUInt32Bits(value.Y));
    }

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static double GlobalDouble(Func<double>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static int GlobalInt(Func<int>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint GlobalUint(Func<uint>? function) => Need(function)();

    /// <summary>mapLimit은 원본처럼 float에 저장한 뒤 strict 비교한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="point">검사할 좌표다.</param>
    /// <returns>유효하면 참이다.</returns>
    private bool Valid(RawPathPoint point)
    {
        float limit = GlobalInt(_globals.MapLimit);
        return point.X > 0 && point.X < limit && point.Y > 0 && point.Y < limit;
    }

    /// <summary>this는 본체+5f0이며 네 caller float를 순서대로 절삭한다. 소스·목표·후보 개수 이외의 입력은 그대로 둔다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="source">절삭할 시작점이다.</param>
    /// <param name="target">절삭할 목표점이다.</param>
    /// <returns>항상 참이다.</returns>
    public bool Initialize(uint token, RawPathPoint source, RawPathPoint target)
    {
        uint x = Truncate(source.X);
        Field(token, 0x638, x);
        uint y = Truncate(source.Y);
        Field(token, 0x63c, y);
        Field(token, 0x640, Truncate(target.X));
        Field(token, 0x644, Truncate(target.Y));
        // 원본+a0/a4 후보 집계와+b0/b4 나선 시작 칸이다.
        Field(token, 0x690, 0);
        Field(token, 0x694, 0);
        Field(token, 0x6a0, x);
        Field(token, 0x6a4, y);
        // 원본+a8/ac/b8의 나선 단계·남은 길이·현재 최적 비용이다.
        Field(token, 0x698, 2);
        Field(token, 0x69c, 1);
        Field(token, 0x6a8, 0x10000);
        // 원본+d0/cc의 무효 결과 XY=-10000 float 비트를 그대로 쓴다.
        Field(token, 0x6c0, 0xc61c4000u);
        Field(token, 0x6bc, 0xc61c4000u);
        // 원본+d4/dc/d8의 검색 진행·현재 후보/마지막 인덱스이며 0개는 ffffffff로 감긴다.
        Field(token, 0x6c4, 0);
        Field(token, 0x6cc, 0);
        Field(token, 0x6c8, unchecked(Field(token, 0x628) - 1u));
        return true;
    }

    /// <summary>삭제 이후의 현재 시각과 rate를 더하며 소유자/TYPE/kind 필터를 추가하지 않는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="parent">내용물 부모 SID다.</param>
    /// <param name="pattern">찾을 전체 DWORD 패턴이다.</param>
    public void Consume(uint token, Sid parent, uint pattern)
    {
        _ = Body(token);
        var cursor = new RawContainedFinder(_pool, _finder);
        // 원본 contained 순서에서 사용 중인 조각을 건너뛰고 첫 패턴 일치만 소비하는 반복문이다.
        for (int sid = cursor.Begin(parent.Value); sid != 0; sid = cursor.Next())
        {
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            uint flags = (uint)raw[20] | ((uint)raw[21] << 8);
            uint word = (uint)raw[12] | ((uint)raw[13] << 8);
            if ((flags & 0x140u) != 0x40u || word != pattern)
            {
                continue;
            }

            Need(_hooks.Destroy)(new Sid(unchecked((ushort)sid)), 0);
            ulong due = BitConverter.DoubleToUInt64Bits((double)BitConverter.UInt32BitsToSingle(Field(token, 0x600)) + GlobalDouble(_globals.Now));
            Field(token, 0x630, (uint)due);
            Field(token, 0x634, (uint)(due >> 32));
            return;
        }
    }

    /// <summary>전이 이후 다음 단계를 같은 호출에서 실행하지 않는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="source">읽고 쓸 시작점이다. 상태 5에서만 바뀐다.</param>
    /// <param name="target">목표점이다.</param>
    /// <returns>상태 5에 닿았으면 참이다.</returns>
    public bool Step(uint token, ref RawPathPoint source, RawPathPoint target)
    {
        switch (Field(token, 0x5f0))
        {
            case 0:
                if (Signed(Field(token, 0x628)) >= Signed(unchecked(Field(token, 0x604) - 1u)))
                {
                    Field(token, 0x5f0, 1);
                }

                break;
            case 1:
                Need(_hooks.Initialize)(token, source, target);
                Field(token, 0x5f0, Field(token, 0x5f0) + 1u);
                break;
            case 2:
                if (Need(_hooks.Prepare)(token))
                {
                    Field(token, 0x5f0, Field(token, 0x5f0) + 1u);
                }

                break;
            case 3:
                if (Need(_hooks.Search)(token))
                {
                    Field(token, 0x5f0, Field(token, 0x5f0) + 1u);
                }

                break;
            case 4:
                if (Signed(Field(token, 0x628)) >= Signed(Field(token, 0x604)))
                {
                    Field(token, 0x5f0, 5);
                }

                break;
            case 5:
                source = Point(token, 0x6bc);
                Field(token, 0x5f0, 0);
                return true;
            default:
                break;
        }

        return false;
    }

    /// <summary>선택한 끝점은 배치/삭제가 본체를 바꾸어도 이번 반환값으로 유지한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="source">시작점이다.</param>
    /// <param name="target">목표점이다.</param>
    /// <returns>연결점이며 미완이면 시작점이다.</returns>
    public RawPathPoint Extend(uint token, RawPathPoint source, RawPathPoint target)
    {
        if (!Step(token, ref source, target))
        {
            return source;
        }

        RawPathPoint result = Point(token, 0x6bc);
        if (Valid(result))
        {
            uint direction = Field(token, 0x6b8);
            uint pattern = Field(token, 0x6b4);
            RawPathPoint point = Point(token, 0x6ac);
            Need(_hooks.Deploy)(token, GlobalUint(_globals.BridgeType), point, pattern, direction);
            Sid parent = Need(_hooks.Technology)(Field(token, 0x260));
            Consume(token, parent, Field(token, 0x6b4));
            Sid current = Need(_hooks.Technology)(Field(token, 0x260));
            Need(_hooks.Refresh)(token, current);
            if (GlobalUint(_globals.LogKind) == 29)
            {
                Need(_hooks.Marker)(token, GlobalUint(_globals.EndMarker), result);
                Need(_hooks.Marker)(token, GlobalUint(_globals.TargetMarker), target);
            }
        }

        return result;
    }

    /// <summary>시작점 부재는 즉시 timeout 실패로 바꾸지 않으며 기존 타이머도 건드리지 않는다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="target">목표점이다.</param>
    /// <returns>대기가 아니면 참이다.</returns>
    public bool Advance(uint token, RawPathPoint target)
    {
        RawPathPoint current = Point(token, 0x3d0);
        if (!Valid(current))
        {
            RawPathPoint start = Need(_hooks.Start)(token, target);
            Point(token, 0x3d0, start);
            if (Valid(start) && GlobalUint(_globals.LogKind) == 29)
            {
                Need(_hooks.Marker)(token, GlobalUint(_globals.StartMarker), start);
            }

            return true;
        }

        RawPathPoint next = Extend(token, current, target);
        Point(token, 0x3d0, next);
        if (Valid(next))
        {
            Field(token, 0x3d8, 1);
            return true;
        }

        uint timeout = unchecked(Field(token, 0x3d8) - 1u);
        Field(token, 0x3d8, timeout);
        if (Signed(timeout) > 0)
        {
            return true;
        }

        Field(token, 0x3d8, 1);
        if (Field(token, 0xb00) != 0)
        {
            Need(_hooks.Log)(token, GlobalUint(_globals.LogKind), "Bridge timeout\n");
        }

        return false;
    }

    /// <summary>같은 실제 풀의 주소를 확인한다. 2026-11-25 R4-5b 다리 경로 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
