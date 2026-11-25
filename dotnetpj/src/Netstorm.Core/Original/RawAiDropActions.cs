// AI의 현재 배치 TYPE/예산 스캔과 생산/기한/사제 관문을 실제 후보 배치 행동으로 연결한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 배치 행동 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.

// 004105b0의 일곱 원래 알림이며 TYPE/방향/좌표는 각 원문 위치의 snapshot이다.
public enum AiDropActionLog
{
    /// <summary>생산 능력 상실 알림이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    LostAbility,

    /// <summary>스캔 완료 알림이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    ScanComplete,

    /// <summary>부분 스캔 알림이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    PartialScan,

    /// <summary>구매 불가 알림이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    CannotAfford,

    /// <summary>잘못된 후보 알림이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    BadChoice,

    /// <summary>배치 알림이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    Laying,

    /// <summary>표적 알림이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    Target,
}

// 출력 여부와 별도인 원래 TYPE 조회/방향/좌표 인자다.
public sealed class AiDropActionNotice
{
    /// <summary>원래 알림 형식이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public AiDropActionLog Action { get; set; }

    /// <summary>시설 TYPE 또는 표적의 TYPE BYTE이며 표적 SID와 구분한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public uint Type { get; set; }

    /// <summary>스캔 방향 또는 y→x ftol로 구한 실제 알림 좌표다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public int Direction { get; set; }

    /// <summary>스캔 방향 또는 y→x ftol로 구한 실제 알림 좌표다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public int X { get; set; }

    /// <summary>스캔 방향 또는 y→x ftol로 구한 실제 알림 좌표다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public int Y { get; set; }
}

// 원본 현재 시각/지도 크기/진단 전역을 같은 저장소에서 빌린다.
public sealed class AiDropActionGlobals
{
    /// <summary>0055b4d0의 현재 QWORD 시각이다. 각 기한 비교에서 다시 읽는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Func<double>? Now { get; set; }

    /// <summary>00531928의 현재 strict 좌표 유효성 상한이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Func<int>? MapLimit { get; set; }

    /// <summary>005e4794의 빈 후보/점수 모순 진단 관문이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public RawCheckingFlag? Checking { get; set; }
}

// 이미 복원된 같은 실제 선택/스캔/생산·비용/배치/부족 좌표/후보 압축과 원래 로그를 연결한다.
public sealed class AiDropActionHooks
{
    /// <summary>004103c0의 현재 owner 기술 부모 mask8 TYPE 조회다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Func<uint, uint, Sid>? Product { get; set; }

    /// <summary>00413100의 같은 실제 TYPE 선택/회전이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Action<uint>? Select { get; set; }

    /// <summary>00412e10의 같은 본체 3e8/현재 후보 스캔이다. true는 미완료다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Func<uint, uint, int, bool>? Scan { get; set; }

    /// <summary>00410280의 실제 signed 비용/지갑 비교다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Func<uint, uint, bool>? CanAfford { get; set; }

    /// <summary>00411a10에서 0049b510 전체이며 argument 0/flags 0/owner BYTE/mode 1이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Func<CanonPlacementQuery, bool>? MayPlace { get; set; }

    /// <summary>004108f0의 실제 생산/비용·배치/통지/음향이다. 반환 성공값을 만들지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Action<uint, uint, RawPathPoint, uint, uint>? Deploy { get; set; }

    /// <summary>00415fe0의 같은 실제 전체 가짜 부족 좌표 갱신이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Action<uint>? Fake { get; set; }

    /// <summary>004133e0의 같은 공용 난수/8방향 좌표 선택이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Action<uint>? Seed { get; set; }

    /// <summary>0040f390의 같은 실제 30후보 안정 압축이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Action<uint, int>? Remove { get; set; }

    /// <summary>원본 Ai.cpp의 복귀 가능한 진단/줄 번호다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Action<string, uint>? Assertion { get; set; }

    /// <summary>004105b0의 원래 b00 비영 위치에서만 실제 장치를 요구한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    public Action<uint, AiDropActionNotice>? Log { get; set; }
}

// 별도 구매/좌표/성공 사본 없이 같은 실제 본체/원본 공용 난수로 행동한다.
// 근거: cpppj o/RawAiDropActions.h/.cpp의 RawAiDropActions다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(flags2만 쓴다).
public sealed class RawAiDropActions
{
    /// <summary>원본 signed 지갑 하한이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    private const int WalletMinimum = 1200;

    /// <summary>원본 물리 후보 배열 크기다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    private const int Choices = 30;

    /// <summary>기술/선택·스캔/후보/지갑과 같은 실제 본체다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>생산 조각과 표적 raw 슬롯의 같은 PATCH 풀이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 TYPE flags2와 로그 이름의 실제 표다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 게임 공용 난수다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    private readonly GameRandom _random;

    /// <summary>현재 시각/지도 크기/진단 참조다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    private readonly AiDropActionGlobals _globals;

    /// <summary>선택된 실제 하위/원래 알림 장치다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    private readonly AiDropActionHooks _hooks;

    /// <summary>생성 중 선택/난수/게임 장치를 실행하지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="memory">기술/선택·스캔/후보/지갑과 같은 실제 본체다.</param>
    /// <param name="pool">생산 조각과 표적 raw 슬롯의 같은 PATCH 풀이다.</param>
    /// <param name="types">현재 TYPE flags2와 로그 이름의 실제 표다.</param>
    /// <param name="random">같은 게임 공용 난수다.</param>
    /// <param name="globals">현재 시각/지도 크기/진단 참조다.</param>
    /// <param name="hooks">선택된 실제 하위/원래 알림 장치다.</param>
    public RawAiDropActions(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, GameRandom random, AiDropActionGlobals globals, AiDropActionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)
        {
            throw new ArgumentException("AI 배치 행동 같은PATCH188 TYPE 누락");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _random = random;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>해제 토큰/작은 가짜 본체를 성공으로 취급하지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 배치 행동 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 little endian DWORD를 실제 블록 안에서 읽는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, int at)
    {
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 행동 DWORD 읽기 범위");
        }

        // 지정 네 BYTE만 조합하는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(at, 4));
    }

    /// <summary>원본 DWORD의 signed 비교/방향을 보존한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>CRT signed64 절삭의 하위 DWORD이며 무효 FISTP의 하위값은 0이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
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

    /// <summary>도달한 실제 하위가 없으면 앞선 상태 쓰기를 유지하고 그 위치에서 거부한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 배치 행동 실제 하위 미연결");

    /// <summary>하위 뒤 현재 본체를 다시 빌린다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int at) => Read(Body(token), at);

    /// <summary>주변 필드를 보존하며 지정한 네 BYTE만 기록한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int at, uint value)
    {
        byte[] raw = Body(token);
        if (at < 0 || (uint)at > (uint)raw.Length || (uint)raw.Length - (uint)at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 행동 DWORD 쓰기 범위");
        }

        // 지정 폭만 쓰는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(at, 4), value);
    }

    /// <summary>TYPE 0도 원래 getter처럼 조회하며 상위 비트가 있는 번호를 BYTE로 자르지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="type">조회할 TYPE 번호다.</param>
    /// <returns>현재 TYPE 기록이다.</returns>
    private OriginalTypeRecord Type(uint type)
    {
        if ((ulong)type >= (ulong)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "AI 배치 행동 현재 TYPE 범위");
        }

        return _types[(int)type];
    }

    /// <summary>두 기한은 각 비교 위치의 현재 시각이며 unordered도 거부한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">기한 위치다.</param>
    /// <returns>현재 시각이 기한에 도달했으면 참이다.</returns>
    private bool Ready(uint token, int at)
    {
        double due = BitConverter.UInt64BitsToDouble(((ulong)Field(token, at + 4) << 32) | Field(token, at));
        double now = Need(_globals.Now)();
        return now >= due;
    }

    /// <summary>생산 조각은 free/dead/새 소유자 필터 없이 pending bit100만 제외한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">조회할 TYPE 번호다.</param>
    /// <returns>유효한 생산 조각 SID이며 pending이면 빈 SID다.</returns>
    private Sid AvailableProduct(uint token, uint type)
    {
        Sid product = Need(_hooks.Product)(token, type);
        if (product.Value != 0 && (_pool.Slot(product.Value)[21] & 1u) != 0)
        {
            return new Sid(0);
        }

        return product;
    }

    /// <summary>회전을 배치 쿼리 flags로 넣지 않으며 owner의 하위 BYTE를 사용한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">검사할 TYPE 번호다.</param>
    /// <param name="point">검사할 좌표다.</param>
    /// <returns>배치 가능하면 참이다.</returns>
    private bool MayPlace(uint token, uint type, RawPathPoint point)
    {
        var query = new CanonPlacementQuery(type, 0, point.X, point.Y, 0, Field(token, 0x260) & 0xffu, 1);
        _ = Type(type);
        return Need(_hooks.MayPlace)(query);
    }

    /// <summary>손상 count/index를 다른 유효 후보로 clamp하지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="index">고를 후보 번호다.</param>
    /// <returns>좌표 기록 위치다.</returns>
    private static int PointOffset(int index)
    {
        if (index < 0 || index >= Choices)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "AI 배치 행동30후보 물리 범위");
        }

        return 0x408 + index * 8;
    }

    /// <summary>현재 후보를 하위/로그 뒤 다시 조회한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="index">고를 후보 번호다.</param>
    /// <returns>현재 후보 좌표다.</returns>
    private RawPathPoint Point(uint token, int index)
    {
        int at = PointOffset(index);
        return new RawPathPoint(BitConverter.UInt32BitsToSingle(Field(token, at)), BitConverter.UInt32BitsToSingle(Field(token, at + 4)));
    }

    /// <summary>같은 후보의 현재 표적 번호다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="index">고를 후보 번호다.</param>
    /// <returns>현재 표적 번호다.</returns>
    private uint Target(uint token, int index)
    {
        _ = PointOffset(index);
        return Field(token, 0x4f8 + index * 4);
    }

    /// <summary>원본 0 슬롯 조회는 허용하며 물리 풀 밖을 잘라 다른 객체로 바꾸지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="number">바꿀 DWORD 값이다.</param>
    /// <returns>물리 검사를 통과한 SID다.</returns>
    private Sid Object(uint number)
    {
        if (number > 65535 || number >= (uint)_pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(number), "AI 배치 행동 현재 표적 SID 물리 범위");
        }

        return new Sid(unchecked((ushort)number));
    }

    /// <summary>출력 여부와 별도로 원래 TYPE 검사의 범위 검사는 앞서 수행한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="notice">기록할 알림이다.</param>
    private void Log(uint token, AiDropActionNotice notice)
    {
        if (notice.Action != AiDropActionLog.PartialScan)
        {
            _ = Type(notice.Type);
        }

        if (Field(token, 0xb00) != 0)
        {
            Need(_hooks.Log)(token, notice);
        }
    }

    /// <summary>후보 좌표는 MayPlace 인자의 사본이 아니라 그 이후 현재 값이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="action">기록할 알림 종류다.</param>
    /// <param name="index">고를 후보 번호다.</param>
    private void PointLog(uint token, AiDropActionLog action, int index)
    {
        int at = PointOffset(index);
        int y = Ftol(BitConverter.UInt32BitsToSingle(Field(token, at + 4)));
        int x = Ftol(BitConverter.UInt32BitsToSingle(Field(token, at)));
        Log(token, new AiDropActionNotice { Action = action, Type = Field(token, 0x3e0), Direction = 0, X = x, Y = y });
    }

    /// <summary>0040e800: 두 float 모두 0보다 크고 현재 mapLimit의 float 저장값보다 작아야 한다. NaN은 거짓이다. 근거는 cpppj o/RawAiPriestActions.h의 AiPriestPointValid다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="point">검사할 좌표다.</param>
    /// <param name="mapLimit">현재 지도 상한이다.</param>
    /// <returns>유효하면 참이다.</returns>
    private static bool PriestPointValid(RawPathPoint point, int mapLimit)
    {
        float limit = mapLimit;
        return point.X > 0 && point.X < limit && point.Y > 0 && point.Y < limit;
    }

    /// <summary>같은 현재 전역 값을 읽는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static int GlobalInt(Func<int>? function) => Need(function)();

    /// <summary>모든 관문/출력·선택 효과 뒤 현재 필드를 원문 위치에서 재조회한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void Run(uint token)
    {
        uint type = Field(token, 0x3e0);
        if (type != 0 && Need(_hooks.Product)(token, type).Value == 0)
        {
            Log(token, new AiDropActionNotice { Action = AiDropActionLog.LostAbility, Type = Field(token, 0x3e0) });
        }

        type = Field(token, 0x3e0);
        if (type == 0 || Field(token, 0x3f0) != 0)
        {
            Need(_hooks.Select)(token);
            type = Field(token, 0x3e0);
            if (type == 0)
            {
                return;
            }
        }

        if (Field(token, 0x3ec) == 0 && !Need(_hooks.Scan)(token, type, Signed(Field(token, 0x3e4))))
        {
            int direction = Signed(Field(token, 0x3e4));
            type = Field(token, 0x3e0);
            Field(token, 0x3ec, 1);
            Log(token, new AiDropActionNotice { Action = AiDropActionLog.ScanComplete, Type = type, Direction = direction });
        }

        if (!Ready(token, 0x5a8) || !Ready(token, 0x5e8) || Signed(Field(token, 0x2d8)) < WalletMinimum || (Field(token, 0x400) != 0 && Field(token, 0x3ec) == 0))
        {
            return;
        }

        if (AvailableProduct(token, Field(token, 0x3e0)).Value == 0 || Signed(Field(token, 0x5a4)) < Signed(Field(token, 0x5a0)))
        {
            return;
        }

        if (PriestPointValid(new RawPathPoint(BitConverter.UInt32BitsToSingle(Field(token, 0xad0)), BitConverter.UInt32BitsToSingle(Field(token, 0xad4))), GlobalInt(_globals.MapLimit)))
        {
            return;
        }

        if (Field(token, 0x3ec) == 0)
        {
            Log(token, new AiDropActionNotice { Action = AiDropActionLog.PartialScan });
        }

        if (Signed(Field(token, 0x570)) < 1)
        {
            RawCheckingFlag? checking = Need(_globals.Checking);
            if (checking is not null && (bool)checking && Field(token, 0x3e8) != 0)
            {
                Need(_hooks.Assertion)("drop.value == 0", 0xf8b);
            }

            if ((Type(Field(token, 0x3e0)).Flags2 & 0x800u) != 0)
            {
                Need(_hooks.Fake)(token);
                Field(token, 0x3f0, 1);
                return;
            }

            Need(_hooks.Seed)(token);
        }
        else
        {
            if (!Need(_hooks.CanAfford)(token, Field(token, 0x3e0)))
            {
                Log(token, new AiDropActionNotice { Action = AiDropActionLog.CannotAfford, Type = Field(token, 0x3e0) });
                Field(token, 0x3f0, 1);
                return;
            }

            Field(token, 0x3f4, 0);
            Field(token, 0x3fc, 0);
            // 실패 후보도 실제 압축하고 압축 뒤 현재 성공 플래그로만 중단하는 반복문이다.
            while (Signed(Field(token, 0x570)) > 0)
            {
                int index = Signed(_random.Next(Field(token, 0x570)));
                uint selectedType = Field(token, 0x3e0);
                RawPathPoint point = Point(token, index);
                if (!MayPlace(token, selectedType, point))
                {
                    PointLog(token, AiDropActionLog.BadChoice, index);
                }
                else
                {
                    PointLog(token, AiDropActionLog.Laying, index);
                    ReadOnlySpan<byte> target = _pool.Slot(Object(Target(token, index)).Value);
                    uint targetType = target[10];
                    int y = Ftol(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(target.Slice(18, 4))));
                    int x = Ftol(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(target.Slice(14, 4))));
                    Log(token, new AiDropActionNotice { Action = AiDropActionLog.Target, Type = targetType, Direction = 0, X = x, Y = y });
                    uint direction = Field(token, 0x3e4);
                    RawPathPoint currentPoint = Point(token, index);
                    uint currentType = Field(token, 0x3e0);
                    Need(_hooks.Deploy)(token, currentType, currentPoint, 0, direction);
                    bool focused = (Field(token, 0x248) & 0x40u) != 0;
                    Field(token, 0x3f4, 1);
                    Field(token, 0x3fc, Target(token, index));
                    if (focused)
                    {
                        Field(token, 0x3f8, Target(token, index));
                    }
                }

                Need(_hooks.Remove)(token, index);
                if (Field(token, 0x3f4) != 0)
                {
                    Field(token, 0x3f0, 1);
                    return;
                }
            }
        }

        Field(token, 0x3f0, 1);
    }

    /// <summary>같은 실제 월드 연결을 확인한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
