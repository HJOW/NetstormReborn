// 실제 수집자의 수집/포획/귀환/경로 취소와 필요한 원본 genus 조회/다이스 접근점을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 운송 행동 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.

// 004acb80/004487e0의 현재 내용물 genus 조회와 다이스 접근점을 복원한다.
// 근거: cpppj o/RawAiCarrierActions.h/.cpp의 FindContainedGenus/DaisApproachPoint다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.
public static class AiContainedGenus
{
    /// <summary>00501908의 다이스 접근 좌표 보정이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    private const float DaisOffset = 3.0F;

    /// <summary>선택된 실제 하위 누락을 가짜 결과로 처리하지 않는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 운반 행동 실제 하위 미연결");

    /// <summary>후보 kind는 BYTE로 자르지 않고 전체 인덱스로 검사한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="types">현재 TYPE 표다.</param>
    /// <param name="type">조회할 TYPE 번호다.</param>
    /// <returns>현재 flags2다.</returns>
    private static uint TypeGenus(IReadOnlyList<OriginalTypeRecord> types, uint type)
    {
        if (type >= (uint)types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "AI 운반 행동 현재TYPE 범위");
        }

        return types[(int)type].Flags2;
    }

    /// <summary>원본은 mask 탈락 후보라도 먼저 kind TYPE를 조회한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="pool">현재 슬롯 풀이다.</param>
    /// <param name="state">현재 내용물 검색기 상태다.</param>
    /// <param name="types">현재 TYPE 표다.</param>
    /// <param name="parent">종속 체인의 부모 SID다.</param>
    /// <param name="mask">조회할 WORD mask다.</param>
    /// <param name="genus">모두 포함해야 하는 genus 비트다.</param>
    /// <param name="index">건너뛸 일치 순번이다.</param>
    /// <returns>순번에 맞는 후보 SID며 없으면 0이다.</returns>
    public static Sid Find(RawSidPool pool, ContainedFinderState state, IReadOnlyList<OriginalTypeRecord> types, Sid parent, uint mask, uint genus, uint index)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(types);
        var finder = new RawContainedFinder(pool, state);
        // 현재 종속 체인을 원래 순서로 방문하고 일치 순번만 DWORD로 감소시키는 반복문이다.
        for (int sid = finder.Begin(parent.Value); sid != 0; sid = finder.Next())
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            uint kind = (uint)raw[18] | ((uint)raw[19] << 8);
            bool matches = (TypeGenus(types, kind) & genus) == genus;
            uint candidate = (uint)raw[20] | ((uint)raw[21] << 8);
            if ((mask == 0 || (candidate & mask) != 0) && matches)
            {
                if (index == 0)
                {
                    return new Sid(unchecked((ushort)sid));
                }

                index--;
            }
        }

        return new Sid(0);
    }

    /// <summary>진단 callback 뒤 현재 genus/좌표/실패점도 다시 읽으며 free/dead 필터를 추가하지 않는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="pool">현재 슬롯 풀이다.</param>
    /// <param name="types">현재 TYPE 표다.</param>
    /// <param name="checking">현재 진단 값이다.</param>
    /// <param name="invalid">무효 시 돌려줄 실패점의 읽기 전용 참조다. 진단 뒤 반환 시점에 다시 읽는다.</param>
    /// <param name="assertion">진단 장치다.</param>
    /// <param name="dais">다이스 SID다.</param>
    /// <returns>다이스 접근점이며 무효하면 실패점이다.</returns>
    public static RawPathPoint DaisApproachPoint(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawCheckingFlag? checking, in RawPathPoint invalid, Action<string>? assertion, Sid dais)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        // 원본의 현재 물리 SID 범위만 검사하는 함수다.
        bool Valid() => dais.Value != 0 && dais.Value < pool.Capacity;
        // 현재 TYPE genus를 다시 읽는 함수다.
        bool IsDais() => Valid() && (TypeGenus(types, pool.Slot(dais.Value)[RawSidPool.TypeOffset]) & 0x400000u) != 0;
        if (checking is not null && (bool)checking && (!Valid() || !IsDais()))
        {
            Need(assertion)("dais.isValid() && dais.getGenus() & gDAIS");
        }

        if (!Valid() || !IsDais())
        {
            return invalid;
        }

        ReadOnlySpan<byte> raw = pool.Slot(dais.Value);
        float x = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(14, 4)));
        float y = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(18, 4)));
        return new RawPathPoint((float)((double)x - DaisOffset), (float)((double)y - DaisOffset));
    }
}

// 같은 실제 현재 전역을 참조하고 호출 위치에서 다시 읽는다.
public sealed class AiCarrierActionGlobals
{
    /// <summary>0055b4d0의 현재 시각이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<double>? Now { get; set; }

    /// <summary>00540bc4의 현재 권한이며 첫 크리스털 조회 뒤 판정한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<bool>? Boss { get; set; }

    /// <summary>005e4794의 free 진단 활성값이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public RawCheckingFlag? Checking { get; set; }

    /// <summary>같은 장부의 현재 00540410이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<uint>? InvalidGraph { get; set; }
}

// 각 분기에 도달한 실제 하위만 호출하며 빈 효과를 성공으로 바꾸지 않는다.
public sealed class AiCarrierActionHooks
{
    /// <summary>004adcb0의 prefix이며 결과는 버려져도 실행한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<Sid, int>? MaxHitPoints { get; set; }

    /// <summary>004acb80(mask20,genus200000,index0)의 실제 첫 사제 내용물이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<Sid, Sid>? Prisoner { get; set; }

    /// <summary>00411630과 같은 현재 TYPE/경로/종속 조회다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<Sid, Sid>? Path { get; set; }

    /// <summary>004487e0의 실제 다이스 접근점이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<Sid, RawPathPoint>? DaisPoint { get; set; }

    /// <summary>004117e0의 같은 실제 좌표 이동이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<uint, Sid, RawPathPoint>? Point { get; set; }

    /// <summary>004118a0의 실제 신전 귀환이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? Home { get; set; }

    /// <summary>00492280의 현재 참여/포획 상태다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<uint, bool>? EnemyAvailable { get; set; }

    /// <summary>00411ca0의 현재 HP/최대 HP 비교다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<Sid, bool>? Vulnerable { get; set; }

    /// <summary>00410980의 같은 384 목록 비행 여부다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<uint, bool>? FlyingGetter { get; set; }

    /// <summary>SID 0 아닌 00410260에서 00462bc0 실제 그래프다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<Sid, uint>? Graph { get; set; }

    /// <summary>004acc60(mask20,현재 크리스털 TYPE)의 실제 총수다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<Sid, int>? Crystals { get; set; }

    /// <summary>004ac9f0(mask20,현재 크리스털 TYPE,index0)이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<Sid, Sid>? FirstCrystal { get; set; }

    /// <summary>00427320에서 00427070(actor,내용물 0)의 실제 요청이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<Sid>? DropRequest { get; set; }

    /// <summary>첫 크리스털 Form의 실제 가상+10(flags0)이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<Sid>? Destroy { get; set; }

    /// <summary>004116d0의 같은 실제 대상 이동이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<uint, Sid, Sid>? Target { get; set; }

    /// <summary>004109d0의 같은 384 등록/로그다. 이동 실패해도 호출한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? Remember { get; set; }

    /// <summary>00411900의 실제 수집 또는 귀환 판단이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<uint, Sid>? Collect { get; set; }

    /// <summary>00426150의 실제 현재 경로 TargetA/B다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Func<Sid, uint[]>? Targets { get; set; }

    /// <summary>00411350의 같은 330/348 재분류다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<uint>? Reclassify { get; set; }

    /// <summary>00411680의 현재 경로 가상 삭제다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<Sid>? Stop { get; set; }

    /// <summary>Ai.cpp의 복귀 가능한 진단과 줄 번호다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    public Action<string, uint>? Assertion { get; set; }
}

// 원본 00412130 전체를 같은 본체/목록/현재 월드에서 수행한다. 자동 프레임 순서는 별도다.
// 근거: cpppj o/RawAiCarrierActions.h/.cpp의 RawAiCarrierActions다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(flags2만 쓴다).
public sealed class RawAiCarrierActions
{
    /// <summary>0040e9d0의 현재 목록 포인터다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    private const int ActiveList = 0x384;

    /// <summary>같은 본체/목록/현재 월드 저장소다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>같은 실제 월드 정체성이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 TYPE flags2를 읽는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 실제 현재 전역을 참조하고 호출 위치에서 다시 읽는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    private readonly AiCarrierActionGlobals _globals;

    /// <summary>각 분기에 도달한 실제 하위만 호출한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    private readonly AiCarrierActionHooks _hooks;

    /// <summary>같은 PATCH188 저장소와 실제 하위를 빌린다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 본체/목록/현재 월드 저장소다.</param>
    /// <param name="pool">같은 실제 월드 정체성이다.</param>
    /// <param name="types">현재 TYPE flags2를 읽는다.</param>
    /// <param name="globals">같은 실제 현재 전역을 참조한다.</param>
    /// <param name="hooks">각 분기에 도달한 실제 하위다.</param>
    public RawAiCarrierActions(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, AiCarrierActionGlobals globals, AiCarrierActionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)
        {
            throw new ArgumentException("AI 운반 행동 같은PATCH188 누락");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>가짜/해제 토큰은 즉시 거부한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 운반 행동 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 little endian DWORD만 읽는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 운반 행동 DWORD 읽기 범위");
        }

        // 지정 네 BYTE만 조합하는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at, 4));
    }

    /// <summary>원본 DWORD의 네 BYTE만 쓴다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] raw, int at, uint value)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 운반 행동 쓰기 범위");
        }

        // 주변 원문을 보존하는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at, 4), value);
    }

    /// <summary>원본 WORD의 상위 BYTE도 유지한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 WORD다.</returns>
    private static uint Word(ReadOnlySpan<byte> raw, int at) => (uint)raw[at] | ((uint)raw[at + 1] << 8);

    /// <summary>원본 signed DWORD 비교/나머지의 비트 해석이다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>선택된 실제 하위 누락을 가짜 행동 성공으로 처리하지 않는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 운반 행동 실제 하위 미연결");

    /// <summary>후보 kind는 BYTE로 자르지 않고 전체 인덱스로 검사한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="types">현재 TYPE 표다.</param>
    /// <param name="type">조회할 TYPE 번호다.</param>
    /// <returns>현재 flags2다.</returns>
    private static uint TypeGenus(IReadOnlyList<OriginalTypeRecord> types, uint type)
    {
        if (type >= (uint)types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "AI 운반 행동 현재TYPE 범위");
        }

        return types[(int)type].Flags2;
    }

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static double GlobalDouble(Func<double>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static bool GlobalBool(Func<bool>? function) => Need(function)();

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint GlobalUint(Func<uint>? function) => Need(function)();

    /// <summary>현재 본체의 값을 읽는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>지정한 본체 필드만 바꾼다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value) => Put(Body(token), at, value);

    /// <summary>현재 목록의 signed count다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <returns>현재 count다.</returns>
    private int Count(uint token, int list) => Signed(Field(token, list + 8));

    /// <summary>현재 포인터를 사용하되 손상된 음수 주소 산술은 명시 거부한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <param name="index">고를 번호다.</param>
    /// <returns>현재 항목 값이다.</returns>
    private uint Item(uint token, int list, int index)
    {
        if (index < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "AI 운반 행동 음수 인덱스");
        }

        return Read(_memory.Bytes(Field(token, list)), index * 4);
    }

    /// <summary>일치하지 않으면 원문을 전혀 쓰지 않는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <param name="value">찾을 값이다.</param>
    /// <returns>들어 있으면 참이다.</returns>
    private bool Contains(uint token, int list, uint value)
    {
        // 현재 active 목록만 검사하는 반복문이다.
        for (int index = 0; index < Count(token, list); index++)
        {
            if (Item(token, list, index) == value)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>full은 추가를 생략하며 꼬리를 정리하지 않는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
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
            throw new ArgumentOutOfRangeException(nameof(value), "AI 운반 행동 음수 추가 인덱스");
        }

        Put(_memory.Bytes(Field(token, list)), count * 4, value);
        Field(token, list + 8, unchecked((uint)count) + 1u);
    }

    /// <summary>모든 중복을 안정 제거하고 비활성 꼬리를 보존한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="list">목록 기록 위치다.</param>
    /// <param name="value">지울 값이다.</param>
    private void Remove(uint token, int list, uint value)
    {
        int removed = 0;
        // 원본 순서로 현재 목록을 압축하는 반복문이다.
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

    /// <summary>원본 isValid에는 free/dead/종류 조건이 없다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="value">검사할 DWORD 값이다.</param>
    /// <returns>물리 SID면 참이다.</returns>
    private bool Valid(uint value) => value != 0 && value < (uint)_pool.Capacity;

    /// <summary>잘못된 DWORD를 다른 SID로 잘라서 사용하지 않는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 DWORD 값이다.</param>
    /// <returns>물리 검사를 통과한 SID다.</returns>
    private Sid Object(uint value)
    {
        if (value >= (uint)_pool.Capacity || value > 65535)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 운반 행동 물리SID 범위");
        }

        return new Sid(unchecked((ushort)value));
    }

    /// <summary>현재 실제 TYPE의 genus다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="actor">조회할 SID다.</param>
    /// <returns>현재 TYPE flags2다.</returns>
    private uint Genus(Sid actor)
    {
        byte type = _pool.Slot(actor.Value)[RawSidPool.TypeOffset];
        if (type >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(actor), "AI 운반 행동 현재TYPE 범위");
        }

        return _types[type].Flags2;
    }

    /// <summary>원본 0 wrapper만 sentinel을 반환하며 그 외는 현재 실제 그래프다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="actor">조회할 SID다.</param>
    /// <returns>현재 그래프 번호다.</returns>
    private uint Graph(Sid actor) => actor.Value != 0 ? Need(_hooks.Graph)(actor) : GlobalUint(_globals.InvalidGraph);

    /// <summary>C0가 1인 less/unordered는 모두 미도달이다. 디컴파일의 단순 now＞due와 구별한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">기한 위치다.</param>
    /// <returns>도달했으면 참이다.</returns>
    private bool Ready(uint token, int at)
    {
        double due = BitConverter.UInt64BitsToDouble(((ulong)Field(token, at + 4) << 32) | Field(token, at));
        return GlobalDouble(_globals.Now) >= due;
    }

    /// <summary>현재 다이스 유무는 실제 이동 직전에 읽는다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="actor">이동할 수집자 SID다.</param>
    private void Deliver(uint token, Sid actor)
    {
        uint dais = Field(token, 0x2a4);
        if (dais != 0)
        {
            RawPathPoint point = Need(_hooks.DaisPoint)(Object(dais));
            Need(_hooks.Point)(token, actor, point);
        }
        else
        {
            Need(_hooks.Home)(token, actor);
        }
    }

    /// <summary>각 효과 뒤 원문의 현재 적 SID/기한/분류를 다시 조회한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="actor">이동할 수집자 SID다.</param>
    /// <returns>포획할 수 있으면 참이다.</returns>
    private bool MayCapture(uint token, Sid actor)
    {
        if ((Field(token, 0x248) & 0x100u) == 0 || Count(token, ActiveList) >= Signed(Field(token, 0x388)) || !Valid(Field(token, 0x2a0)) || !Need(_hooks.EnemyAvailable)(Field(token, 0x264)) || !Need(_hooks.Vulnerable)(Object(Field(token, 0x2a0))) || !Ready(token, 0x5e0))
        {
            return false;
        }

        if ((Genus(actor) & 0x20000u) != 0 && !Need(_hooks.FlyingGetter)(token))
        {
            return true;
        }

        uint source = Graph(actor);
        return source == Graph(Object(Field(token, 0x2a0)));
    }

    /// <summary>첫 크리스털 조회 뒤 현재 boss를 읽는다. 비서버 요청은 이동/등록 없이 반환한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="actor">이동할 수집자 SID다.</param>
    private void Capture(uint token, Sid actor)
    {
        if (Need(_hooks.Crystals)(actor) != 0)
        {
            Sid crystal = Need(_hooks.FirstCrystal)(actor);
            if (!GlobalBool(_globals.Boss))
            {
                Need(_hooks.DropRequest)(actor);
                return;
            }

            Need(_hooks.Destroy)(crystal);
        }

        Need(_hooks.Target)(token, actor, Object(Field(token, 0x2a0)));
        Need(_hooks.Remember)(token, actor);
    }

    /// <summary>전체 원본의 prefix/포획 귀환/순환 한 항목/정지 우선순위를 유지한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void Act(uint token)
    {
        uint enemy = Field(token, 0x2a0);
        if (Valid(enemy))
        {
            _ = Need(_hooks.MaxHitPoints)(Object(enemy));
        }

        // 포획 사제를 든 정지 운반자를 가장 먼저 귀환시키며 생각 기한/일반 수집자보다 우선하는 반복문이다.
        for (int index = 0; index < Count(token, ActiveList); index++)
        {
            uint value = Item(token, ActiveList, index);
            if (value != 0 && Need(_hooks.Prisoner)(Object(value)).Value != 0 && Need(_hooks.Path)(Object(value)).Value == 0)
            {
                Sid actor = Object(value);
                Deliver(token, actor);
                Field(token, 0x38c, 0);
                Append(token, ActiveList, value);
                return;
            }
        }

        int count = Count(token, 0x2dc);
        if (count == 0 || !Ready(token, 0x5a8))
        {
            return;
        }

        int next = Signed(unchecked(Field(token, 0x6d0) + 1u));
        if (next == int.MinValue && count == -1)
        {
            throw new OverflowException("AI 운반 행동 원본IDIV overflow");
        }

        int cursor = next % count;
        Field(token, 0x6d0, unchecked((uint)cursor));
        Sid carrier = Object(Item(token, 0x2dc, cursor));
        if (Need(_globals.Checking) is RawCheckingFlag flag && (bool)flag && (_pool.Slot(carrier.Value)[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0)
        {
            Need(_hooks.Assertion)("!s.isFree()", 0xc04);
        }

        if ((_pool.Slot(carrier.Value)[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0)
        {
            Remove(token, 0x2dc, carrier.Value);
            return;
        }

        if ((Genus(carrier) & 0x200000u) != 0)
        {
            Need(_hooks.Assertion)("!( s.getGenus() & gPRIEST )", 0xc0a);
        }

        if (Need(_hooks.Path)(carrier).Value == 0)
        {
            if (MayCapture(token, carrier))
            {
                Capture(token, carrier);
                return;
            }

            if (carrier.Value == 0)
            {
                Need(_hooks.Assertion)("sid", 0xaeb);
            }

            if (Need(_hooks.Prisoner)(carrier).Value != 0)
            {
                Deliver(token, carrier);
                return;
            }

            Need(_hooks.Collect)(token, carrier);
            if (Contains(token, ActiveList, carrier.Value))
            {
                Remove(token, ActiveList, carrier.Value);
            }

            return;
        }

        if (Contains(token, ActiveList, carrier.Value))
        {
            return;
        }

        if (Field(token, 0x294) != 0 && Field(token, 0x298) != GlobalUint(_globals.InvalidGraph))
        {
            uint[] targets = Need(_hooks.Targets)(carrier);
            if (targets.Length == 0 || targets[0] == 0)
            {
                return;
            }

            Sid target = Object(targets[0]);
            if ((_pool.Slot(target.Value)[RawSidPool.StateOffset] & 3u) == 0 && (Genus(target) & 0x10000000u) != 0 && Contains(token, 0x330, target.Value))
            {
                if ((Genus(carrier) & 0x20000u) != 0)
                {
                    return;
                }

                uint destination = Graph(target);
                if (destination == Graph(carrier))
                {
                    return;
                }
            }

            Need(_hooks.Reclassify)(token);
        }

        Need(_hooks.Stop)(carrier);
    }

    /// <summary>같은 실제 월드 연결을 확인한다. 2026-11-25 R4-5b 운반 행동 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
