// 실제 기술 내용물과 직전 후보/표적·방향·쿨다운으로 AI의 다음 배치 TYPE를 선택한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 배치 선택 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.

// 원본 세 로그의 의미이며 출력 여부/현재 로그 종류는 원래 위치에서 판단한다.
public enum AiDropSelectionLog
{
    /// <summary>직전 표적 선호다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    Hated,

    /// <summary>후보 재사용이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    Retried,

    /// <summary>현재 TYPE·회전 알림이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    Scanning,
}

// 이미 복원된 같은 기술 목록/전체 배치·후보 압축과 실제 로그를 연결한다.
public sealed class AiDropSelectionHooks
{
    /// <summary>0048fc90의 현재 owner 기술 부모다. 선택 전체에서는 최초 부모를 보존한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    public Func<uint, Sid>? Technology { get; set; }

    /// <summary>004acc60(mask8,kind0)의 실제 모든 일치 개수다. 중복도 센다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    public Func<Sid, int>? Count { get; set; }

    /// <summary>004acb80(mask8,genus4000000,index0)의 현재 실제 블로커 생산 조각이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    public Func<Sid, Sid>? Blocker { get; set; }

    /// <summary>004acac0(mask8,kind0,index)의 TYPE WORD다. SID가 아니다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    public Func<Sid, uint, uint>? Nth { get; set; }

    /// <summary>0049b510 전체: argument 0/flags 0/현재 owner BYTE/mode 1이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    public Func<CanonPlacementQuery, bool>? MayPlace { get; set; }

    /// <summary>0040f390의 같은 30후보 안정 압축이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    public Action<uint, int>? Remove { get; set; }

    /// <summary>004105b0의 현재 출력이다. b00 비영인 실제 위치에서만 요구한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    public Action<uint, AiDropSelectionLog, uint, int>? Log { get; set; }
}

// 실제 본체/기술/공용 난수와 현재 네 방향 TYPE 전역만 사용한다.
// 근거: cpppj o/RawAiDropSelection.h/.cpp의 RawAiDropSelection다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(flags2만 쓴다).
public sealed class RawAiDropSelection
{
    /// <summary>블로커 우선 선택과 현재 생산 조각을 판단하는 원본 flags2다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private const uint BlockerFlag = 0x4000000u;

    /// <summary>같은 30후보 XY의 본체 시작/행 크기다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private const int FirstChoice = 0x408;

    /// <summary>같은 30후보 XY의 행 크기다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private const int ChoiceStride = 8;

    /// <summary>같은 본체/기술 순번·후보/직전 표적 저장소다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>같은 실제 월드 정체성이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 flags2와 진단 TYPE를 읽는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>네 현재 원본 방향 TYPE 전역의 참조다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private readonly uint[] _directionalTypes;

    /// <summary>SP/시설/스캔과 같은 공용 난수다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private readonly GameRandom _random;

    /// <summary>0055b4d0의 현재 QWORD 시각이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private readonly Func<double> _now;

    /// <summary>선택된 실제 하위/로그다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private readonly AiDropSelectionHooks _hooks;

    /// <summary>현재 네 TYPE는 00541264/25c/288/290의 순서이며 기본 131/129/140/142다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 본체/기술 순번·후보/직전 표적 저장소다.</param>
    /// <param name="pool">같은 실제 월드 정체성이다.</param>
    /// <param name="types">현재 flags2와 진단 TYPE를 읽는다.</param>
    /// <param name="directionalTypes">네 현재 원본 방향 TYPE 전역의 참조다.</param>
    /// <param name="random">SP/시설/스캔과 같은 공용 난수다.</param>
    /// <param name="now">0055b4d0의 현재 QWORD 시각이다.</param>
    /// <param name="hooks">선택된 실제 하위/로그다.</param>
    public RawAiDropSelection(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, uint[] directionalTypes, GameRandom random, Func<double> now, AiDropSelectionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(directionalTypes);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(now);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || directionalTypes.Length != 4)
        {
            throw new ArgumentException("AI 배치 TYPE 선택 같은PATCH/현재TYPE누락");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _directionalTypes = directionalTypes;
        _random = random;
        _now = now;
        _hooks = hooks;
    }

    /// <summary>해제/작은 가짜 본체를 거부한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 배치 TYPE 선택 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 DWORD를 실제 블록 범위 안에서 읽는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 TYPE 선택 DWORD 범위");
        }

        // 지정 네 BYTE만 조합하는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at, 4));
    }

    /// <summary>DWORD 감김 뒤 signed 해석이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 signed 나눗셈은 0쪽 절삭이며 나머지도 부호를 보존한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="value">나눌 값이다.</param>
    /// <param name="count">나눌 수다.</param>
    /// <returns>signed 나머지다.</returns>
    private static int Remainder(int value, int count)
    {
        if (count == 0 || (count == -1 && value == int.MinValue))
        {
            throw new OverflowException("AI 배치 기술 순환 signed 나눗셈 범위");
        }

        return value % count;
    }

    /// <summary>선택된 실제 하위만 그 자리에서 요구한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 배치 TYPE 선택 실제 하위 미연결");

    /// <summary>현재 같은 본체를 매 원문 위치에서 읽는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>지정 DWORD만 쓴다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value)
    {
        byte[] raw = Body(token);
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 TYPE 선택 쓰기 범위");
        }

        // 지정 네 BYTE만 쓰는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at, 4), value);
    }

    /// <summary>TYPE 0도 원본처럼 조회하며 범위 밖만 명시 거부한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="type">조회할 TYPE 번호다.</param>
    /// <returns>현재 TYPE 기록이다.</returns>
    private OriginalTypeRecord Type(uint type)
    {
        if ((ulong)type >= (ulong)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "AI 배치 TYPE 선택 현재TYPE 범위");
        }

        return _types[(int)type];
    }

    /// <summary>현재 네 전역과 DWORD 그대로 비교하며 TYPE 배열은 아직 조회하지 않는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="type">비교할 TYPE 번호다.</param>
    /// <returns>네 방향 중 하나면 참이다.</returns>
    public bool Directional(uint type)
    {
        // 원본 순서의 첫 일치를 반환하는 반복문이다.
        foreach (uint current in _directionalTypes)
        {
            if (type == current)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>진입 시 개수와 목록 포인터를 값으로 보존한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="target">찾을 표적 값이다.</param>
    /// <returns>들어 있으면 참이다.</returns>
    private bool Contains(uint token, uint target)
    {
        int count = Signed(Field(token, 0x374));
        if (count <= 0)
        {
            return false;
        }

        byte[] list = _memory.Bytes(Field(token, 0x36c));
        int index = 0;
        // 최초 count 안의 첫 일치까지만 진행하는 반복문이다.
        do
        {
            if (Read(list, index * 4) == target)
            {
                return true;
            }

            index++;
        }
        while (index < count);

        return false;
    }

    /// <summary>b00은 로그 위치마다 현재 다시 읽으며 읽지 않을 TYPE/방향은 인자로 요구하지 않는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="action">기록할 로그 종류다.</param>
    /// <param name="type">기록할 TYPE 번호다.</param>
    /// <param name="direction">기록할 방향이다.</param>
    private void Log(uint token, AiDropSelectionLog action, uint type = 0, int direction = 0)
    {
        if (Field(token, 0xb00) != 0)
        {
            Need(_hooks.Log)(token, action, type, direction);
        }
    }

    /// <summary>원본 QWORD 시각을 재구성한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 QWORD 시각이다.</returns>
    private double Time(uint token, int at) => BitConverter.UInt64BitsToDouble(((ulong)Field(token, at + 4) << 32) | Field(token, at));

    /// <summary>실제 기술 개수/부모는 최초값이고 다른 본체 필드는 각 하위 효과 뒤 현재값이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void Select(uint token)
    {
        Field(token, 0xaf8, 10);
        Field(token, 0xafc, 10);
        Field(token, 0x3ec, 0);
        Sid technology = Need(_hooks.Technology)(Field(token, 0x260));
        int count = Need(_hooks.Count)(technology);
        if (count == 0)
        {
            Field(token, 0x3e0, 0);
            Field(token, 0x3f0, 1);
            return;
        }

        bool repeat = false;
        if ((Field(token, 0x248) & 0x80u) != 0)
        {
            uint draw = _random.Next(100u);
            if (Field(token, 0x3f4) != 0 && Field(token, 0x3e0) != 0)
            {
                if (Contains(token, Field(token, 0x3fc)))
                {
                    if (draw < 80)
                    {
                        Log(token, AiDropSelectionLog.Hated);
                        repeat = true;
                    }
                }
                else if (Field(token, 0x374) == 0 && draw < 30)
                {
                    repeat = true;
                }
            }
        }

        bool forceBlocker = false;
        if (Field(token, 0x404) != 0 && (Type(Field(token, 0x3e0)).Flags2 & BlockerFlag) == 0)
        {
            if (Need(_hooks.Blocker)(technology).Value != 0)
            {
                forceBlocker = true;
            }
            else
            {
                Field(token, 0x404, 0);
            }
        }

        if (repeat && !forceBlocker)
        {
            int index = 0;
            // 제거 시 같은 index/좌표 위치에서 압축된 현재 항목을 재검사하는 반복문이다.
            while (index < Signed(Field(token, 0x570)))
            {
                if (index < 0 || index >= 30)
                {
                    throw new ArgumentOutOfRangeException(nameof(token), "AI 배치 재사용 후보 인덱스");
                }

                int at = FirstChoice + index * ChoiceStride;
                var query = new CanonPlacementQuery(Field(token, 0x3e0), 0, BitConverter.UInt32BitsToSingle(Field(token, at)), BitConverter.UInt32BitsToSingle(Field(token, at + 4)), 0, Field(token, 0x260) & 0xffu, 1);
                _ = Type(query.Type);
                if (!Need(_hooks.MayPlace)(query))
                {
                    Need(_hooks.Remove)(token, index);
                }
                else
                {
                    index++;
                }
            }

            Log(token, AiDropSelectionLog.Retried);
        }
        else
        {
            uint remaining = unchecked((uint)count) + 2u;
            // 원본은 기술 개수+2번까지 회전/다음 TYPE를 시도하고 블로커 우선도 이 예산 안에서만 수행하는 반복문이다.
            do
            {
                --remaining;
                uint current = Field(token, 0x3e0);
                if (current != 0 && Directional(current) && Signed(Field(token, 0x3e4)) < 6)
                {
                    Field(token, 0x3e4, Field(token, 0x3e4) + 2u);
                }
                else
                {
                    int next = Remainder(Signed(Field(token, 0x3dc)) + 1, count);
                    Field(token, 0x3dc, unchecked((uint)next));
                    uint selected = Need(_hooks.Nth)(technology, unchecked((uint)next));
                    Field(token, 0x3e0, selected);
                    Field(token, 0x3e4, 0);
                }

                if (remaining == 0 || Field(token, 0x404) == 0)
                {
                    break;
                }

                if ((Type(Field(token, 0x3e0)).Flags2 & BlockerFlag) != 0)
                {
                    break;
                }
            }
            while (true);

            Field(token, 0x404, 0);
            Field(token, 0x570, 0);
        }

        uint choices = Field(token, 0x570);
        if (choices == 0)
        {
            Field(token, 0x3e8, 0);
        }

        Field(token, 0x3f0, 0);
        Field(token, 0x3f4, 0);
        Field(token, 0x3fc, 0);
        Field(token, 0x400, Signed(choices) <= 0 ? 1u : 0u);
        uint flags = Type(Field(token, 0x3e0)).Flags2;
        if ((flags & 0x30000u) != 0)
        {
            Field(token, 0x3e0, 0);
            Field(token, 0x3f0, 1);
        }

        if ((flags & 0x800u) != 0 && !(Need(_now)() >= Time(token, 0x5b8)))
        {
            Field(token, 0x3e0, 0);
            Field(token, 0x3f0, 1);
        }

        if (Field(token, 0x3e0) != 0)
        {
            uint type = Field(token, 0x3e0);
            int direction = Signed(Field(token, 0x3e4));
            _ = Type(type);
            Log(token, AiDropSelectionLog.Scanning, type, direction);
        }
    }

    /// <summary>같은 실제 풀 연결 확인용이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
