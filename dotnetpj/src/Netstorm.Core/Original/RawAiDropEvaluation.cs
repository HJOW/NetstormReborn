// AI의 부족 좌표 점수/시설별 평가 분배와 실제 배치 후보·에너지 승인을 복원한다.
using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Core.Original;

// AI 배치 평가 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.

// 현재 원본 마나 전역을 매 평가 위치에서 다시 읽는다.
public sealed class AiDropEvaluationGlobals
{
    /// <summary>0052f488의 현재 공급 반경이다. 제곱 반경과 구분한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    public Func<float>? ManaRadius { get; set; }

    /// <summary>00541174의 현재 공급 문자열 조회 TYPE다. 초기값 71이다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    public Func<uint>? ProbeType { get; set; }
}

// 같은 실제 마나·배치 검사·부족 좌표와 원본 로그 효과다.
public sealed class AiDropEvaluationHooks
{
    /// <summary>004730c0: 초기 빈 문자열/현재 TYPE·owner/theme-1이다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    public Action<StringBuilder, float, float, uint, uint, int>? Collect { get; set; }

    /// <summary>0049b510 전체: argument 0/flags 0/owner BYTE/mode 1이다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    public Func<CanonPlacementQuery, bool>? MayPlace { get; set; }

    /// <summary>004734d0의 실제 DWORD 승인이다. bool로 축소하지 않는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    public Func<StringBuilder, float, float, uint, uint, uint>? Energy { get; set; }

    /// <summary>0040f1c0의 같은 실제 부족 좌표 추가다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    public Func<uint, float, float, uint, bool>? Dry { get; set; }

    /// <summary>004105b0 종류 29의 현재 TYPE 이름 진단이며 b00 비영 때만 호출한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    public Action<uint, uint>? Unsupported { get; set; }

    /// <summary>004105b0의 현재 Dry Spot 종류/절삭 좌표이며 실제 추가 뒤만 호출한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    public Action<uint, int, int>? DryLog { get; set; }
}

// 별도 점수·목록 사본 없이 같은 실제 본체와 표적 평가기를 사용한다.
// 근거: cpppj o/RawAiDropEvaluation.h/.cpp의 RawAiDropEvaluation다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(flags2/group만 쓴다).
public sealed class RawAiDropEvaluation
{
    /// <summary>PE00501480의 원본 float 부족 좌표 점수 기준이다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    private const float DistanceValue = 1000.0f;

    /// <summary>ae4의 각 기록은 x/y float와 TYPE DWORD다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    private const int DryStride = 12;

    /// <summary>같은 본체/부족 좌표 backing 저장소다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>같은 실제 월드 정체성이다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 시설 flags2/group과 진단 TYPE를 읽는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 실제 Gun/지상/블로커 점수다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    private readonly RawAiDropTargets _targets;

    /// <summary>현재 반경/조회 TYPE의 참조다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    private readonly AiDropEvaluationGlobals _globals;

    /// <summary>선택된 실제 하위와 로그 장치다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    private readonly AiDropEvaluationHooks _hooks;

    /// <summary>생성 중 게임 판단/장치를 실행하지 않는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 본체/부족 좌표 backing 저장소다.</param>
    /// <param name="pool">같은 실제 월드 정체성이다.</param>
    /// <param name="types">현재 시설 flags2/group과 진단 TYPE를 읽는다.</param>
    /// <param name="targets">같은 실제 Gun/지상/블로커 점수다.</param>
    /// <param name="globals">현재 반경/조회 TYPE의 참조다.</param>
    /// <param name="hooks">선택된 실제 하위와 로그 장치다.</param>
    public RawAiDropEvaluation(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawAiDropTargets targets, AiDropEvaluationGlobals globals, AiDropEvaluationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(targets);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(pool, targets.Pool))
        {
            throw new ArgumentException("AI 배치 평가 같은 PATCH풀 누락");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _targets = targets;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>현재 본체의 정확한 크기/수명을 확인한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 배치 평가 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>지정 비정렬 DWORD를 현재 물리 블록 안에서 읽는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 평가 DWORD 범위");
        }

        // 지정 네 BYTE만 조합하는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at, 4));
    }

    /// <summary>원본 DWORD를 signed로 읽는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 ftol의 signed64 절삭 하위 DWORD이며 무효 FISTP의 하위값은 0이다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
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

    /// <summary>선택된 실제 하위가 없으면 그 지점 앞 효과를 보존하며 거부한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 배치 평가 실제 하위 미연결");

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static float GlobalFloat(Func<float>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint GlobalUint(Func<uint>? function) => Need(function)();

    /// <summary>하위 뒤 현재 본체를 다시 빌린다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>지정 DWORD 이외의 원문을 보존한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value)
    {
        byte[] raw = Body(token);
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 평가 쓰기 범위");
        }

        // 지정 네 BYTE만 쓰는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at, 4), value);
    }

    /// <summary>TYPE 0도 원본처럼 조회하며 물리 표 밖만 거부한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="type">조회할 TYPE 번호다.</param>
    /// <returns>현재 TYPE 기록이다.</returns>
    private OriginalTypeRecord Type(uint type)
    {
        if ((ulong)type >= (ulong)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "AI 배치 평가 현재TYPE 범위");
        }

        return _types[(int)type];
    }

    /// <summary>각 개선 비교는 축소 전 값으로 하되 선택된 최고값은 원본처럼 float 저장 후 재사용한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="point">잴 좌표다.</param>
    /// <param name="index">선택한 기록 번호를 받는 출력이다.</param>
    /// <returns>최고 점수며 없으면 0이다.</returns>
    public float DryDistance(uint token, RawPathPoint point, ref int index)
    {
        float best = 0;
        // 매 기록에서 현재 포인터/count를 다시 읽고 TYPE/용량/cursor로 임의 필터하지 않는 반복문이다.
        for (int current = 0; current < Signed(Field(token, 0xae0)); current++)
        {
            byte[] raw = _memory.Bytes(Field(token, 0xae4));
            int at = current * DryStride;
            double distance = Math.Abs(BitConverter.UInt32BitsToSingle(Read(raw, at)) - point.X) + Math.Abs(BitConverter.UInt32BitsToSingle(Read(raw, at + 4)) - point.Y);
            if (distance > 0)
            {
                double value = DistanceValue - distance;
                if (value > best)
                {
                    index = current;
                    best = (float)value;
                }
            }
        }

        return best;
    }

    /// <summary>공급 문자열은 C의 첫 NUL까지 세고 실제 공급이 선택한 동일 selected 목록 효과를 유지한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="point">잴 좌표다.</param>
    /// <returns>공급 점수며 조건 불충족이면 0이다.</returns>
    public int SupplyScore(uint token, RawPathPoint point)
    {
        int unusedIndex = 0;
        float value = DryDistance(token, point, ref unusedIndex);
        float radius = GlobalFloat(_globals.ManaRadius);
        if (!((double)value > DistanceValue - ((double)radius + radius)))
        {
            return 0;
        }

        var text = new StringBuilder();
        uint owner = Field(token, 0x260);
        uint probe = GlobalUint(_globals.ProbeType);
        Need(_hooks.Collect)(text, point.X, point.Y, probe, owner, -1);
        int length = text.Length;
        // 첫 NUL까지 세는 반복문이다.
        for (int i = 0; i < text.Length; i++)
        {
            if (text[i] == '\0')
            {
                length = i;
                break;
            }
        }

        if (length > int.MaxValue - 1)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "AI 배치 공급 C문자열 길이 범위");
        }

        if (length == 0 || length >= Signed(Field(token, 0xad8)))
        {
            return 0;
        }

        return Ftol(value) / (length + 1);
    }

    /// <summary>두 TYPE 조회의 flags/group을 먼저 읽고 원본 우선순위대로 단 하나의 평가만 선택한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">평가할 TYPE 번호다.</param>
    /// <param name="point">평가할 좌표다.</param>
    /// <param name="direction">평가할 방향이다.</param>
    /// <param name="target">평가 출력을 쓰는 SID다.</param>
    /// <returns>점수며 미지원이면 0이다.</returns>
    public int Evaluate(uint token, uint type, RawPathPoint point, int direction, ref Sid target)
    {
        uint flags = Type(type).Flags2;
        int group = Type(type).Group;
        if ((flags & 0x800u) != 0)
        {
            return SupplyScore(token, point);
        }

        if ((flags & 0x4000000u) != 0)
        {
            return _targets.Blockers(token, type, point, direction, ref target);
        }

        if (group == 3)
        {
            return _targets.Ground(token, type, point, ref target);
        }

        if (group == 0 || group == 1)
        {
            return _targets.Gun(token, type, point, direction, ref target);
        }

        if (group != 6)
        {
            _ = Type(type);
            if (Field(token, 0xb00) != 0)
            {
                Need(_hooks.Unsupported)(token, type);
            }
        }

        return 0;
    }

    /// <summary>후보를 채택하지 않아도 대상 SID의 평가 효과는 보존하고 승인 뒤 목표 Y→X→점수 순서로만 쓴다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">평가할 TYPE 번호다.</param>
    /// <param name="point">평가할 좌표다.</param>
    /// <param name="direction">평가할 방향이다.</param>
    /// <param name="bestPoint">최고 좌표를 쓰는 출력이다.</param>
    /// <param name="target">평가 출력을 쓰는 SID다.</param>
    /// <param name="read">현재 최고 점수를 읽는다.</param>
    /// <param name="write">새 최고 점수를 쓴다.</param>
    /// <returns>채택했으면 참이다.</returns>
    private bool ConsiderScore(uint token, uint type, RawPathPoint point, int direction, ref RawPathPoint bestPoint, ref Sid target, Func<int> read, Action<int> write)
    {
        var query = new CanonPlacementQuery(type, 0, point.X, point.Y, 0, Field(token, 0x260) & 0xffu, 1);
        _ = Type(type);
        if (!Need(_hooks.MayPlace)(query))
        {
            return false;
        }

        int value = Evaluate(token, type, point, direction, ref target);
        int bestScore = read();
        if (!(bestScore < value || (value > 0 && value == bestScore)))
        {
            return false;
        }

        var text = new StringBuilder();
        uint owner = Field(token, 0x260);
        if (Need(_hooks.Energy)(text, point.X, point.Y, type, owner) != 0)
        {
            bestPoint.Y = point.Y;
            bestPoint.X = point.X;
            write(value);
            return true;
        }

        if (Need(_hooks.Dry)(token, point.X, point.Y, type))
        {
            int y = Ftol(point.Y);
            int x = Ftol(point.X);
            if (Field(token, 0xb00) != 0)
            {
                Need(_hooks.DryLog)(token, x, y);
            }
        }

        return false;
    }

    /// <summary>명시 외부 점수 참조도 평가가 끝난 그 시점에 읽는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">평가할 TYPE 번호다.</param>
    /// <param name="point">평가할 좌표다.</param>
    /// <param name="direction">평가할 방향이다.</param>
    /// <param name="bestPoint">최고 좌표를 쓰는 출력이다.</param>
    /// <param name="bestScore">최고 점수를 쓰고 읽는 참조다.</param>
    /// <param name="target">평가 출력을 쓰는 SID다.</param>
    /// <returns>채택했으면 참이다.</returns>
    public bool Consider(uint token, uint type, RawPathPoint point, int direction, ref RawPathPoint bestPoint, ref int bestScore, ref Sid target)
    {
        // ref 매개변수는 람다에 직접 캡처·할당할 수 없어 지역 사본으로 읽고 쓴 뒤 호출 뒤에 돌려쓴다. 단일 호출 안에서는 원문 참조와 같다.
        int current = bestScore;
        bool accepted = ConsiderScore(token, type, point, direction, ref bestPoint, ref target, () => current, value => { current = value; });
        bestScore = current;
        return accepted;
    }

    /// <summary>실제 스캔의 3e8을 임시 사본/호출 뒤 일괄 동기화로 대신하지 않는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">평가할 TYPE 번호다.</param>
    /// <param name="point">평가할 좌표다.</param>
    /// <param name="direction">평가할 방향이다.</param>
    /// <param name="bestPoint">최고 좌표를 쓰는 출력이다.</param>
    /// <param name="target">평가 출력을 쓰는 SID다.</param>
    /// <returns>채택했으면 참이다.</returns>
    public bool ConsiderStored(uint token, uint type, RawPathPoint point, int direction, ref RawPathPoint bestPoint, ref Sid target) =>
        ConsiderScore(token, type, point, direction, ref bestPoint, ref target, () => Signed(Field(token, 0x3e8)), value => Field(token, 0x3e8, unchecked((uint)value)));

    /// <summary>동일 실제 풀 연결 확인용이다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
