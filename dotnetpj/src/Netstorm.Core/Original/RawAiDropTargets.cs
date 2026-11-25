// AI 건물 배치의 실제 표적 점수와 Gun/장애물 후보 평가를 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 배치 표적 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.

// 원본 현재 TYPE 전역을 같은 저장소에서 빌린다.
public sealed class AiDropTargetGlobals
{
    /// <summary>005412e8의 현재 제단 TYPE다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Func<uint>? AltarType { get; set; }

    /// <summary>00541290의 현재 윈드 블로커 TYPE다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Func<uint>? DirectionalType { get; set; }
}

// 이미 복원된 실제 내용물/TYPE+48/HP/검색/중심 조회만 연결한다.
public sealed class AiDropTargetHooks
{
    /// <summary>004acb80(mask20,genus200000,0)의 실제 첫 사제 내용물이다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Func<Sid, Sid>? Captive { get; set; }

    /// <summary>004ac550에서 0049acf0의 현재 TYPE+48 hpPerSec다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Func<Sid, float>? Rate { get; set; }

    /// <summary>004adcb0의 현재 최대 HP다. 현재 HP는 먼저 저장한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Func<Sid, int>? MaxHitPoints { get; set; }

    /// <summary>0046b6f0의 실제 Archer/Cannon finder다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Func<float, float, uint, int, uint, Sid>? Gun { get; set; }

    /// <summary>0046b540의 별도 실제 지상 검색이다. 일반 Gun 검색으로 대신하지 않는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Func<float, float, uint, uint, Sid>? Ground { get; set; }

    /// <summary>00469590의 같은 실제 Gun 수집이며 원본 임시 용량은 30이다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Func<uint, RawPathPoint, int, List<uint>>? Blockers { get; set; }

    /// <summary>004adea0의 현재 실제 발자국 중심이다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Func<Sid, RawPathPoint>? Center { get; set; }

    /// <summary>SID 0 운반체에서 원본 진단 후 계속한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    public Action<string, uint>? Assertion { get; set; }
}

// 같은 본체/물리 풀/현재 TYPE와 실제 검색 하위로만 점수·출력 SID를 계산한다.
// 근거: cpppj o/RawAiDropTargets.h/.cpp의 RawAiDropTargets다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(flags2/group만 쓴다).
public sealed class RawAiDropTargets
{
    /// <summary>PE005012d0의 두 축 보정값이며 발자국 절반과 다르다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    private const float BlockerOffset = 1.0f;

    /// <summary>00412950의 실제 임시 목록 용량이다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    private const int BlockerCapacity = 30;

    /// <summary>다른 AI 행동과 같은 본체/목록 저장소다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 TYPE/HP/소유자/내용물의 유일한 물리 풀이다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 flags2/group을 읽는 실제 TYPE 표다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>현재 제단/방향 블로커 TYPE 전역의 참조다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    private readonly AiDropTargetGlobals _globals;

    /// <summary>선택된 실제 원본 하위다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    private readonly AiDropTargetHooks _hooks;

    /// <summary>같은 PATCH 저장소를 빌리며 생성 중 검색하지 않는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="memory">다른 AI 행동과 같은 본체/목록 저장소다.</param>
    /// <param name="pool">현재 TYPE/HP/소유자/내용물의 유일한 물리 풀이다.</param>
    /// <param name="types">현재 flags2/group을 읽는 실제 TYPE 표다.</param>
    /// <param name="globals">현재 제단/방향 블로커 TYPE 전역의 참조다.</param>
    /// <param name="hooks">선택된 실제 원본 하위다.</param>
    public RawAiDropTargets(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, AiDropTargetGlobals globals, AiDropTargetHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("AI 배치 표적 같은 PATCH풀 누락");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>현재 본체의 정확한 크기와 수명을 확인한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 배치 표적 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 little endian DWORD만 조합한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int at)
    {
        if ((uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 표적 DWORD 범위");
        }

        // 지정 네 BYTE를 읽는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(at, 4));
    }

    /// <summary>DWORD 감김 뒤 signed 해석이다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 ftol의 signed64 절삭 하위 DWORD를 읽으며 무효 FISTP의 하위값은 0이다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
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

    /// <summary>선택되지 않은 하위는 요구하지 않고 선택된 미연결은 명시 실패한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 배치 표적 실제 하위 미연결");

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint Global(Func<uint>? function) => Need(function)();

    /// <summary>하위 효과 뒤 현재 본체를 다시 읽는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>원본 BYTE TYPE의 현재 flags/group을 읽는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="target">조회할 SID다.</param>
    /// <returns>현재 TYPE 기록이다.</returns>
    private OriginalTypeRecord Type(Sid target)
    {
        byte type = _pool.Slot(target.Value)[RawSidPool.TypeOffset];
        if ((ulong)type >= (ulong)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(target), "AI 배치 표적 현재TYPE 범위");
        }

        return _types[type];
    }

    /// <summary>손상된 DWORD를 정상 SID로 잘라 쓰지 않는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 DWORD 값이다.</param>
    /// <returns>물리 검사를 통과한 SID다.</returns>
    private Sid Object(uint value)
    {
        if ((ulong)value >= (ulong)_pool.Capacity || value > 65535)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 배치 표적 물리SID 범위");
        }

        return new Sid(unchecked((ushort)value));
    }

    /// <summary>원본 최초 포인터/count만 사용하며 중복 가산은 하지 않는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="pointer">목록 기록 위치다.</param>
    /// <param name="target">찾을 SID다.</param>
    /// <returns>들어 있으면 참이다.</returns>
    private bool Contains(uint token, int pointer, Sid target)
    {
        int count = Signed(Field(token, pointer + 8));
        if (count <= 0)
        {
            return false;
        }

        byte[] list = _memory.Bytes(Field(token, pointer));
        // 최초 개수에서 첫 일치까지만 읽는 반복문이다.
        for (int index = 0; index < count; index++)
        {
            if (Read(list, index * 4) == target.Value)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>정밀 C가 누락한 group 가산을 실제 모든 점수와 기계어 순서대로 보존한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="target">채점할 SID다.</param>
    /// <returns>최종 점수다.</returns>
    public int Score(uint token, Sid target)
    {
        uint flags = Type(target).Flags2;
        uint value = (flags & 0x4000000u) != 0 ? 5u : 10u;
        if ((flags & 0x200000u) != 0)
        {
            value += Field(token, 0x2a8) != 0 ? 5u : 200u;
        }

        if ((flags & 0x200u) != 0)
        {
            value += 6;
        }

        if (Type(target).Group == 6)
        {
            value += 7;
        }

        if (_pool.Slot(target.Value)[RawSidPool.TypeOffset] == Global(_globals.AltarType))
        {
            value += 8;
        }

        if ((flags & 0x30000u) != 0)
        {
            if (target.Value == 0)
            {
                Need(_hooks.Assertion)("sid", 0xaeb);
            }

            if (Need(_hooks.Captive)(target).Value != 0)
            {
                value += 400;
            }
        }

        value = unchecked((uint)Ftol((double)Need(_hooks.Rate)(target) + Signed(value)));
        if (Contains(token, 0x36c, target))
        {
            value += 100;
        }

        if (Contains(token, 0x378, target))
        {
            value += 100;
        }

        if (Field(token, 0x3f8) == target.Value)
        {
            int hp = Signed(Read(_pool.Slot(target.Value), 26));
            int maximum = Need(_hooks.MaxHitPoints)(target);
            if (hp >= maximum / 2)
            {
                value += 50;
            }
        }

        int result = Signed(value);
        return _pool.Slot(target.Value)[34] == Field(token, 0x264) ? result : result / 2;
    }

    /// <summary>출력은 검색 뒤 점수 계산 전이며 0도 기록한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">검색할 TYPE 번호다.</param>
    /// <param name="point">검색할 좌표다.</param>
    /// <param name="direction">검색할 방향이다.</param>
    /// <param name="result">검색 결과를 먼저 쓰는 출력 SID다.</param>
    /// <returns>점수이며 빈 검색은 0이다.</returns>
    public int Gun(uint token, uint type, RawPathPoint point, int direction, ref Sid result)
    {
        uint owner = Field(token, 0x260);
        Sid found = Need(_hooks.Gun)(point.X, point.Y, type, direction, owner);
        result = found;
        return found.Value != 0 ? Score(token, found) : 0;
    }

    /// <summary>별도 지상 검색도 출력 0/점수 하위 실패 전 쓰기 순서를 보존한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">검색할 TYPE 번호다.</param>
    /// <param name="point">검색할 좌표다.</param>
    /// <param name="result">검색 결과를 먼저 쓰는 출력 SID다.</param>
    /// <returns>점수이며 빈 검색은 0이다.</returns>
    public int Ground(uint token, uint type, RawPathPoint point, ref Sid result)
    {
        uint owner = Field(token, 0x260);
        Sid found = Need(_hooks.Ground)(point.X, point.Y, type, owner);
        result = found;
        return found.Value != 0 ? Score(token, found) : 0;
    }

    /// <summary>특수 flags는 중심 조회 전에 반환하며 다른 방향값은 네 방향 중 하나와 정확히 같아야 한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="point">볼 좌표다.</param>
    /// <param name="direction">볼 방향이다.</param>
    /// <param name="target">볼 SID다.</param>
    /// <returns>마주보면 참이다.</returns>
    public bool Facing(RawPathPoint point, int direction, Sid target)
    {
        if ((Type(target).Flags2 & 0x100000u) != 0)
        {
            return false;
        }

        RawPathPoint center = Need(_hooks.Center)(target);
        int facing = RawSquidNeighbors.Direction(point.X, point.Y, center.X, center.Y);
        return (direction == 0 || direction == 2 || direction == 4 || direction == 6) && facing == direction;
    }

    /// <summary>모든 등장마다 점수를 계산하고 같은 양의 최고값은 출력 SID를 다시 쓰지 않는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">방향 필터에 쓸 TYPE 번호다.</param>
    /// <param name="point">수집할 좌표다.</param>
    /// <param name="direction">방향 필터에 쓸 방향이다.</param>
    /// <param name="result">최고 SID를 쓰는 출력이다.</param>
    /// <returns>모든 점수의 감김 합이다.</returns>
    public int Blockers(uint token, uint type, RawPathPoint point, int direction, ref Sid result)
    {
        uint owner = Field(token, 0x260);
        List<uint> list = Need(_hooks.Blockers)(owner, point, BlockerCapacity);
        uint total = 0;
        int best = 0;
        // 원본 임시 목록의 순서/중복을 보존하고 방향 필터도 점수 계산 뒤 수행하는 반복문이다.
        foreach (uint value in list)
        {
            Sid target = Object(value);
            int score = Score(token, target);
            if (type == Global(_globals.DirectionalType) && !Facing(new RawPathPoint((float)((double)point.X - BlockerOffset), (float)((double)point.Y - BlockerOffset)), direction, target))
            {
                score = 0;
            }

            if (score > best)
            {
                best = score;
                result = Object(list[0]);
            }

            total += unchecked((uint)score);
        }

        return Signed(total);
    }

    /// <summary>동일 실제 풀 연결 확인용이다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
