// AI의 간헐천 선택·신전 귀환과 포획 운반자/끊긴 간헐천 목록을 같은 본체에서 처리한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 자원 선택 모듈을 담는 네임스페이스다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.

// 이미 복원된 실제 중심/그래프/내용물/이동과 원본 로그·진단을 빌린다.
public sealed class AiResourceChoiceHooks
{
    /// <summary>004adea0의 현재 신전 발 중심이다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    public Func<Sid, RawPathPoint>? Center { get; set; }

    /// <summary>00462bc0의 실제 현재 그래프이며 SID 0 필터는 호출자 위치에서만 적용한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    public Func<Sid, uint>? Graph { get; set; }

    /// <summary>004acc60(actor,20h,현재 5412c0)의 실제 내용물 총수다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    public Func<Sid, int>? Crystals { get; set; }

    /// <summary>004116d0(token,actor,target,0)의 같은 실제 이동이다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    public Action<uint, Sid, Sid>? Target { get; set; }

    /// <summary>004118a0의 같은 실제 신전 복귀다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? Home { get; set; }

    /// <summary>004105b0(token,29,PriestGetter)의 현재 장치 로그다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? CaptureLog { get; set; }

    /// <summary>현재 532f3e4 종류로 끊긴 간헐천을 기록한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? DisconnectedLog { get; set; }

    /// <summary>원본의 유효 간헐천 진단이다. 반환하면 남은 흐름을 계속한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    public Action<string>? Assertion { get; set; }
}

// 별도 공략/대상 복사본 없이 현재 330/348/384 목록과 실제 월드를 사용한다.
// 근거: cpppj o/RawAiResourceChoice.h/.cpp의 RawAiResourceChoice다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(flags2만 쓴다).
public sealed class RawAiResourceChoice
{
    /// <summary>00410a42의 최소 거리 제곱 초기값이다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    private const float InitialDistance = 65536.0F;

    /// <summary>같은 본체/목록/현재 월드 저장소다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>같은 실제 월드 정체성이다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 TYPE flags2를 읽는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 장부의 현재 무효 그래프를 선택된 원본 위치에서 읽는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    private readonly Func<uint>? _invalidGraph;

    /// <summary>선택된 실제 중심/그래프/내용물/이동/로그 경계다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    private readonly AiResourceChoiceHooks _hooks;

    /// <summary>같은 PATCH188 저장소와 실제 하위를 빌린다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 본체/목록/현재 월드 저장소다.</param>
    /// <param name="pool">같은 실제 월드 정체성이다.</param>
    /// <param name="types">현재 TYPE flags2를 읽는다.</param>
    /// <param name="invalidGraph">같은 장부의 현재 무효 그래프를 읽는다.</param>
    /// <param name="hooks">선택된 실제 하위다.</param>
    public RawAiResourceChoice(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, Func<uint>? invalidGraph, AiResourceChoiceHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)
        {
            throw new ArgumentException("AI 자원 선택 같은PATCH188 누락");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _invalidGraph = invalidGraph;
        _hooks = hooks;
    }

    /// <summary>가짜/해제 토큰은 즉시 거부한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 자원 선택 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 little endian DWORD만 읽는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 자원 선택 DWORD 읽기 범위");
        }

        // 지정 네 BYTE만 조합하는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at, 4));
    }

    /// <summary>원본 DWORD의 네 BYTE만 쓴다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] raw, int at, uint value)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 자원 선택 쓰기 범위");
        }

        // 주변 원문을 보존하는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at, 4), value);
    }

    /// <summary>원본 signed DWORD 비교의 비트 해석이다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 CRT ftol의 signed64 절삭 하위 DWORD다. 무효 FISTP의 하위 DWORD도 0이다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭한 signed DWORD다.</returns>
    private static int Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((int)(long)Math.Truncate(value));
    }

    /// <summary>선택된 실제 하위 누락을 가짜 행동 성공으로 처리하지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 자원 선택 실제 하위 미연결");

    /// <summary>같은 현재 무효 그래프 값을 읽는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <returns>현재 무효 그래프다.</returns>
    private uint InvalidGraph() => Need(_invalidGraph)();

    /// <summary>현재 본체의 값을 읽는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>지정한 본체 필드만 바꾼다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value) => Put(Body(token), at, value);

    /// <summary>비양수 count는 원본의 자연 루프 종료를 유지한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <returns>현재 count다.</returns>
    private int Count(uint token, int list) => Signed(Field(token, list + 8));

    /// <summary>손상된 음수 인덱스의 호스트 주소 접근만 명시 거부한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <param name="index">고를 번호다.</param>
    /// <returns>현재 항목 값이다.</returns>
    private uint Item(uint token, int list, int index)
    {
        if (index < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "AI 자원 선택 음수 인덱스");
        }

        return Read(_memory.Bytes(Field(token, list)), index * 4);
    }

    /// <summary>용량이 없으면 실제 목록 포인터에 접근하지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <param name="value">추가할 값이다.</param>
    private void Append(uint token, int list, uint value)
    {
        int count = Count(token, list);
        if (count >= Signed(Field(token, list + 4)))
        {
            return;
        }

        if (count < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 자원 선택 음수 추가 인덱스");
        }

        Put(_memory.Bytes(Field(token, list)), count * 4, value);
        Field(token, list + 8, unchecked((uint)count) + 1u);
    }

    /// <summary>제거한 중복 수만 count에서 빼며 남은 원소의 상대 순서를 유지한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <param name="value">지울 값이다.</param>
    private void Remove(uint token, int list, uint value)
    {
        int removed = 0;
        // 현재 count/포인터로 전체 active 목록을 안정 압축하는 반복문이다.
        for (int index = 0; index < Count(token, list); index++)
        {
            uint current = Item(token, list, index);
            if (current == value)
            {
                removed++;
            }
            else
            {
                Put(_memory.Bytes(Field(token, list)), (index - removed) * 4, current);
            }
        }

        Field(token, list + 8, unchecked((uint)Count(token, list)) - unchecked((uint)removed));
    }

    /// <summary>잘못된 DWORD를 다른 SID로 잘라서 사용하지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 DWORD 값이다.</param>
    /// <returns>물리 검사를 통과한 SID다. 0도 실제 슬롯이다.</returns>
    private Sid Object(uint value)
    {
        if (value >= (uint)_pool.Capacity || value > 65535)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 자원 선택 실제SID 범위");
        }

        return new Sid(unchecked((ushort)value));
    }

    /// <summary>free/dead/소유자 필터를 추가하지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="value">조회할 목록 DWORD다.</param>
    /// <returns>현재 TYPE flags2다.</returns>
    private uint Genus(uint value)
    {
        byte type = _pool.Slot(Object(value).Value)[RawSidPool.TypeOffset];
        if (type >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 자원 선택 TYPE 범위");
        }

        return _types[type].Flags2;
    }

    /// <summary>신전이 없으면 center 하위를 요구하지 않고 두 raw float를 그대로 반환한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 신전 중심 또는 본체 원문 좌표다.</returns>
    public RawPathPoint HomeCenter(uint token)
    {
        uint temple = Field(token, 0x294);
        if (temple != 0)
        {
            return Need(_hooks.Center)(Object(temple));
        }

        return new RawPathPoint(
            BitConverter.UInt32BitsToSingle(Field(token, 0x268)),
            BitConverter.UInt32BitsToSingle(Field(token, 0x26c)));
    }

    /// <summary>거리값이 같으면 먼저 방문한 대상을 유지한다. 그래프는 거리 탈락 후보에서도 조회한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">고를 목록 기록 위치다.</param>
    /// <param name="point">거리를 잴 기준점이다.</param>
    /// <param name="excludedGraph">빼낼 그래프다. 무효 sentinel이면 끄지 않는다.</param>
    /// <param name="requiredGraph">요구할 그래프다. 무효 sentinel이면 끄지 않는다.</param>
    /// <returns>고른 SID며 없으면 0이다.</returns>
    public Sid Select(uint token, int list, RawPathPoint point, uint excludedGraph, uint requiredGraph)
    {
        float best = InitialDistance;
        uint selected = 0;
        // 그래프 getter의 효과 뒤 다음 원소/현재 count와 포인터를 다시 읽는 반복문이다.
        for (int index = 0; index < Count(token, list); index++)
        {
            uint value = Item(token, list, index);
            ReadOnlySpan<byte> raw = _pool.Slot(Object(value).Value);
            double x = (double)point.X - BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(14, 4)));
            double y = (double)point.Y - BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(18, 4)));
            int distance = Ftol(x * x + y * y);
            uint graph = Need(_hooks.Graph)(Object(value));
            if (!((double)distance < best))
            {
                continue;
            }

            uint invalid = InvalidGraph();
            if ((excludedGraph == invalid || graph != excludedGraph) && (requiredGraph == invalid || requiredGraph == graph))
            {
                best = distance;
                selected = value;
            }
        }

        return Object(selected);
    }

    /// <summary>첫 관문 뒤에는 초기 신전/그래프 상태를 다시 검사하지 않고 원래 순서의 실제 하위를 호출한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="actor">이동할 수집자 SID다.</param>
    public void CollectOrHome(uint token, Sid actor)
    {
        if (Field(token, 0x294) == 0 || Field(token, 0x298) == InvalidGraph())
        {
            return;
        }

        if (Need(_hooks.Crystals)(actor) == 0 && (Field(token, 0x248) & 0x10u) != 0)
        {
            RawPathPoint center = HomeCenter(token);
            uint graph = Need(_hooks.Graph)(actor);
            uint excluded = InvalidGraph();
            Sid target = Select(token, 0x330, center, excluded, graph);
            if (target.Value != 0)
            {
                Need(_hooks.Target)(token, actor, target);
                return;
            }
        }

        Need(_hooks.Home)(token, actor);
    }

    /// <summary>물리 슬롯 0이나 free 항목도 원본처럼 현재 TYPE로만 분류한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>비행 포획 운반자가 있으면 참이다.</returns>
    public bool HasFlyingCaptureCarrier(uint token)
    {
        // 현재 포획 대기 목록만 방문하는 반복문이다.
        for (int index = 0; index < Count(token, 0x384); index++)
        {
            if ((Genus(Item(token, 0x384, index)) & 0x20000u) != 0)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>중복은 로그도 생략하고 full은 로그까지 진행한다. 기한/사제 HP를 갱신하지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="actor">등록할 수집자 SID다.</param>
    public void RememberCaptureCarrier(uint token, Sid actor)
    {
        int count = Count(token, 0x384);
        if (count > 0)
        {
            byte[] list = _memory.Bytes(Field(token, 0x384));
            // 최초 count/포인터의 중복을 확인하는 반복문이다.
            for (int index = 0; index < count; index++)
            {
                if (Read(list, index * 4) == actor.Value)
                {
                    return;
                }
            }
        }

        Append(token, 0x384, actor.Value);
        if (Field(token, 0xb00) != 0)
        {
            Need(_hooks.CaptureLog)(token, actor);
        }
    }

    /// <summary>로그 뒤 현재 원문을 재조회하며 연속된 제거 항목을 건너뛰지 않는다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void ReclassifyDisconnected(uint token)
    {
        int index = 0;
        // 다른 그래프 항목은 같은 인덱스에서 중복 모두 제거한 뒤 다음 현재 원소를 검사하는 반복문이다.
        while (index < Count(token, 0x330))
        {
            uint value = Item(token, 0x330, index);
            if (value == 0 || value >= (uint)_pool.Capacity || (Genus(value) & 0x10000000u) == 0)
            {
                Need(_hooks.Assertion)("s.isValid() && (s.getGenus()&gGEYSER)");
            }

            uint graph = value != 0 ? Need(_hooks.Graph)(Object(value)) : InvalidGraph();
            if (graph == Field(token, 0x298))
            {
                index++;
            }
            else
            {
                if (Field(token, 0xb00) != 0)
                {
                    Need(_hooks.DisconnectedLog)(token, Object(value));
                }

                Remove(token, 0x330, value);
                Append(token, 0x348, value);
            }
        }
    }

    /// <summary>같은 실제 월드 연결을 확인한다. 2026-11-26 R4-5b 자원 선택 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
