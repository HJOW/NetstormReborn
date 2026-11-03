using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// Fort 로딩 뒤 소유 거주 구역의 자연 반환과 실제 지역·반복 랜덤 배치 흐름을 복원한다
/// (cpppj <c>o/RawFortFinishOwner.h</c>의 상수·상태·훅·클래스).
/// </summary>
/// <remarks>2026-11-03 F2c-8에서 추가했다.</remarks>
public static class FortFinishOwnerShape
{
    /// <summary>원본 거주 구역 genus 비트다.</summary>
    public const uint Residence = 0x40000000u;

    /// <summary>각 AddRandom의 최대 시도 횟수다.</summary>
    public const uint Attempts = 1000u;

    /// <summary>residences 목록에서 읽는 raw 타입 바이트 위치다.</summary>
    public const int TypeOffset = 10;

    /// <summary>패치판 raw 소유자 바이트 위치다.</summary>
    public const int PatchOwnerOffset = 34;

    /// <summary>CD판 raw 소유자 바이트 위치다.</summary>
    public const int CdOwnerOffset = 32;

    /// <summary>시각 seed의 원본 double 배율이다.</summary>
    public const double TimeScale = 1013.0;

    /// <summary>시각 seed의 DWORD 계수다.</summary>
    public const uint SeedFactor = 33u;
}

/// <summary>
/// 최초 진입만 읽는 모드·기본 지역과 반복 생성 때마다 읽는 현재 타입 전역이다
/// (cpppj <c>o/RawFortFinishOwner.h</c>의 <c>FortFinishOwnerState</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-8에서 추가했다. 호출자가 소유하며 이 모듈보다 오래 살아야 한다.</remarks>
public sealed class FortFinishOwnerState
{
    /// <summary>00594fa4의 chaff 모드다. 켜지면 후처리 없이 자연 반환한다.</summary>
    public bool ChaffMode { get; set; }

    /// <summary>005c85a4의 special 모드다. 켜지면 후처리 없이 자연 반환한다.</summary>
    public bool Special { get; set; }

    /// <summary>00594fb8의 fort 모드다. 켜지면 지역 0을 쓴다.</summary>
    public bool Fort { get; set; }

    /// <summary>Player+0x78/CD+0x70의 9개 지역이다. 실제 미션 지역으로 초기화한다.</summary>
    public int[] PlayerRegions { get; } = [127, 127, 127, 127, 127, 127, 127, 127, 127];

    /// <summary>두 판본의 invalidRegion 전역 초기값이다.</summary>
    public int InvalidRegion { get; set; } = 127;

    /// <summary>005411bc/CD0051caa8의 실제 거주 구역 타입 초기값이다.</summary>
    public uint ResidenceType { get; set; } = 89u;
}

/// <summary>
/// 지역 사각형이다. 원본 네 float의 저장 순서를 그대로 보관한다
/// (cpppj <c>o/RawFortFinishOwner.h</c>의 <c>FortRandomRectangle</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-8에서 추가했다.</remarks>
public readonly record struct FortRandomRectangle(float Left, float Top, float Right, float Bottom)
{
    /// <summary>사각형의 왼쪽 경계다.</summary>
    public float Left { get; init; } = Left;

    /// <summary>사각형의 위쪽 경계다.</summary>
    public float Top { get; init; } = Top;

    /// <summary>사각형의 오른쪽 경계다.</summary>
    public float Right { get; init; } = Right;

    /// <summary>사각형의 아래쪽 경계다.</summary>
    public float Bottom { get; init; } = Bottom;
}

/// <summary>
/// 실제0040db90/CD0042c3d0 가상 배치 경계의 입력이다
/// (cpppj <c>o/RawFortFinishOwner.h</c>의 <c>FortRandomPlaceRequest</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-8에서 추가했다.</remarks>
public readonly record struct FortRandomPlaceRequest(uint Type, float X, float Y, int Region, uint Mode, uint Force)
{
    /// <summary>타입 표에서 객체 종류를 찾는 번호다.</summary>
    public uint Type { get; init; } = Type;

    /// <summary>X 좌표다.</summary>
    public float X { get; init; } = X;

    /// <summary>Y 좌표다.</summary>
    public float Y { get; init; } = Y;

    /// <summary>지역이다.</summary>
    public int Region { get; init; } = Region;

    /// <summary>동작 분기를 선택하는 모드다.</summary>
    public uint Mode { get; init; } = Mode;

    /// <summary>강제다.</summary>
    public uint Force { get; init; } = Force;
}

/// <summary>
/// 외부 결과를 생략하지 않는다. place가 성공하면 실제 Pop/postPop이 같은 residences 목록에 등록해야 한다
/// (cpppj <c>o/RawFortFinishOwner.h</c>의 <c>FortFinishOwnerHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-8에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortFinishOwnerHooks
{
    /// <summary>실제0046f130/CD004bd8a0의 지역 사각형 조회다.</summary>
    public Func<int, FortRandomRectangle>? RegionBounds { get; set; }

    /// <summary>00460d70/CD004011b0의 현재 호출별 double 시각이다.</summary>
    public Func<double>? WallSeconds { get; set; }

    /// <summary>실제 placement 전체다. local 난수도 전달한다.</summary>
    public Func<NetstormRandom, FortRandomPlaceRequest, uint>? Place { get; set; }

    /// <summary>AddRandom의 타입0·-1 진단이며 반환값0을 유지한다.</summary>
    public Action? InvalidType { get; set; }
}

/// <summary>
/// 전체 entry·시각 seed·현재 거주 구역 재조회·하위 1000회 배치 재시도를 한 월드에서 실행한다
/// (cpppj <c>o/RawFortFinishOwner</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-8에서 추가했다. 풀·타입·목록·상태는 이 객체보다 오래 살아야 한다.</remarks>
public sealed class RawFortFinishOwner
{
    /// <summary>객체 슬롯의 생성·조회·반납에 사용하는 SID 풀이다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>거주 건물이다. AssetPostPopState.residences 자체를 빌린다.</summary>
    private readonly RawPostPopList _residences;

    /// <summary>최초 진입만 읽는 모드·기본 지역과 반복 생성 때마다 읽는 현재 타입 전역이다.</summary>
    private readonly FortFinishOwnerState _state;

    /// <summary>외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음이다.</summary>
    private readonly FortFinishOwnerHooks _hooks;

    /// <summary>신전·IslandRuntime이 함께 갱신하는 같은 9개 Player 기록이다. 비어 있으면 상태 표를 쓴다.</summary>
    private byte[] _players = [];

    /// <summary>
    /// 같은 판본의 현재 타입·실제 거주 구역 목록과 모든 외부 효과를 요구한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="types">타입 번호 순서의 타입 표다.</param>
    /// <param name="residences">거주 건물 목록이다.</param>
    /// <param name="state">모드·지역·현재 타입 전역이다.</param>
    /// <param name="hooks">외부 효과 경계다.</param>
    /// <exception cref="ArgumentException">Fort owner 후처리의 실제 자료·경계 누락이다.</exception>
    public RawFortFinishOwner(
        RawSidPool pool,
        IReadOnlyList<OriginalTypeRecord> types,
        RawPostPopList residences,
        FortFinishOwnerState state,
        FortFinishOwnerHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(residences);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        uint expected = pool.Edition == OriginalEdition.Patch1078 ? 188u : 171u;
        if ((uint)types.Count != expected ||
            hooks.RegionBounds is null || hooks.WallSeconds is null || hooks.Place is null || hooks.InvalidType is null)
        {
            throw new ArgumentException("Fort owner 후처리의 실제 자료·경계 누락");
        }
        _pool = pool;
        _types = types;
        _residences = residences;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>
    /// __ftol의 64비트 절삭 하위 DWORD다. 비유한·불확정 정수는 원본 하위 DWORD 0으로 처리한다.
    /// </summary>
    /// <param name="value">자를 double 값이다.</param>
    /// <returns>원본 절삭의 하위 32비트다.</returns>
    private static int Truncate(double value)
    {
        if (!double.IsFinite(value) || Math.Abs(value) >= 9223372036854775808.0)
        {
            return 0;
        }
        long cut = (long)value;
        return unchecked((int)unchecked((uint)cut));
    }

    /// <summary>
    /// limit이 1보다 작으면 local 난수를 소비하지 않는 원본 Random helper의 분기다.
    /// </summary>
    /// <param name="random">소비할 local 난수다.</param>
    /// <param name="limit">상한이다.</param>
    /// <returns>0 이상 limit 미만의 값이다.</returns>
    private static uint Next(NetstormRandom random, int limit)
    {
        return limit < 1 ? 0u : unchecked((uint)random.Next(limit));
    }

    /// <summary>
    /// 실제004bcab0/CD004a5a10은 wallSeconds*1013을 CRT 절삭한 DWORD의 SAR4+33배로 local seed를 만든다.
    /// SAR의 음수 반올림과 DWORD 감김을 그대로 유지한다. 글로벌 난수 seed를 변경하지 않는다.
    /// </summary>
    /// <param name="wallSeconds">현재 벽시각이다.</param>
    /// <returns>local seed다.</returns>
    public static uint TimeSeed(double wallSeconds)
    {
        int value = Truncate(wallSeconds * FortFinishOwnerShape.TimeScale);
        return unchecked(unchecked((uint)(value >> 4)) + unchecked((uint)value) * FortFinishOwnerShape.SeedFactor);
    }

    /// <summary>
    /// 0040e1a0/CD0042ca70: mode·invalid region이면 자연 반환하고 없으면 실제 region→seed→AddRandom을 반복한다.
    /// 처음 정한 region은 반복 중 모드·Player가 바뀌어도 다시 정하지 않는다. 현재 residences·type은 매 순회 읽는다.
    /// 원본은 실패 1000회 뒤에도 다음 region·시각 seed를 구해 계속한다. 가짜 외부 타이머·성공을 넣지 않는다.
    /// </summary>
    /// <param name="owner">후처리할 소유자다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort owner 기본 지역·목록 범위 오류다.</exception>
    public void Finish(uint owner)
    {
        if (_state.ChaffMode || _state.Special)
        {
            return;
        }
        if (owner >= (uint)_state.PlayerRegions.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "Fort owner 기본 지역 범위 오류");
        }
        int region = _state.Fort ? 0 : PlayerRegion(owner);
        if (region == _state.InvalidRegion)
        {
            return;
        }
        int ownerOffset = _pool.Edition == OriginalEdition.Patch1078
            ? FortFinishOwnerShape.PatchOwnerOffset
            : FortFinishOwnerShape.CdOwnerOffset;
        // 전체 목록을 끝까지 읽은 뒤 일치 여부를 판단한다
        for (; ; )
        {
            bool found = false;
            // free·dead·void를 추가로 제외하지 않는다
            for (uint index = 0; unchecked((int)index) < unchecked((int)_residences.Count); index++)
            {
                if (index >= (uint)_residences.Entries.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(owner), owner, "Fort residence 현재 목록 저장소 부족");
                }
                uint number = _residences.Entries[(int)index];
                if (number >= (uint)_pool.Capacity)
                {
                    throw new ArgumentOutOfRangeException(nameof(owner), owner, "Fort residence SID 범위 오류");
                }
                ReadOnlySpan<byte> raw = _pool.Slot((int)number);
                if (raw[FortFinishOwnerShape.TypeOffset] >= (uint)_types.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(owner), owner, "Fort residence 현재 타입 오류");
                }
                if ((_types[raw[FortFinishOwnerShape.TypeOffset]].Flags2 & FortFinishOwnerShape.Residence) != 0
                    && raw[ownerOffset] == owner)
                {
                    found = true;
                }
            }
            if (found)
            {
                return;
            }
            FortRandomRectangle rectangle = Need(_hooks.RegionBounds)(region);
            NetstormRandom random = new(TimeSeed(Need(_hooks.WallSeconds)()));
            PlaceRandom(random, rectangle, region, 1, _state.ResidenceType, 0);
        }
    }

    /// <summary>
    /// 0040ded0/CD0042c7a0: type0·-1 진단·높이 다음 폭의 local 난수·성공 quantity·1000회 상한·마지막 결과다.
    /// 원본이 전달한 rectangle·value 인자는 snapshot이다. placement는 같은 local random을 소비·변경할 수 있다.
    /// 각 성공에서만 quantity를 감소시키고 현재 last result를 반환한다. quantity 0도 첫 시도는 실행한다.
    /// </summary>
    /// <param name="random">소비·변경할 local 난수다.</param>
    /// <param name="rectangle">배치 사각형이다.</param>
    /// <param name="region">지역이다.</param>
    /// <param name="quantity">성공 목표 수량이다.</param>
    /// <param name="type">배치할 타입 번호다.</param>
    /// <param name="force">강제다.</param>
    /// <returns>마지막 배치 결과다.</returns>
    public uint PlaceRandom(
        NetstormRandom random,
        FortRandomRectangle rectangle,
        int region,
        uint quantity,
        uint type,
        uint force)
    {
        ArgumentNullException.ThrowIfNull(random);
        if (type == 0 || type == 0xffffffffu)
        {
            Need(_hooks.InvalidType)();
            return 0;
        }
        FortRandomRectangle area = rectangle;
        int height = Truncate((double)area.Bottom - area.Top);
        int width = Truncate((double)area.Right - area.Left);
        uint result = 0;
        // 각 시도마다 높이·너비 순서로 난수를 소비한다
        for (uint attempt = 0; attempt < FortFinishOwnerShape.Attempts; attempt++)
        {
            uint dy = Next(random, height);
            uint dx = Next(random, width);
            float x = (float)((double)dx + area.Left);
            float y = (float)((double)dy + area.Top);
            result = Need(_hooks.Place)(random, new FortRandomPlaceRequest(type, x, y, region, 0, force));
            if (result != 0)
            {
                quantity = unchecked(quantity - 1);
            }
            if (quantity == 0)
            {
                break;
            }
        }
        return result;
    }

    /// <summary>
    /// loader hook과 같은 pool인지 검사하는 참조다.
    /// </summary>
    /// <returns>이 모듈이 쓰는 풀이다.</returns>
    public RawSidPool Pool()
    {
        return _pool;
    }

    /// <summary>
    /// 같은 9개 Player 바이트의 PATCH+78·CD+70을 공유하며 최초 지역을 현재 원문에서 읽는다.
    /// </summary>
    /// <param name="players">원본 9개 Player 기록이다. 이 객체보다 오래 살아야 한다.</param>
    /// <exception cref="ArgumentException">FortFinishOwner 실제 9개 Player 크기 오류다.</exception>
    public void BindPlayerRecords(byte[] players)
    {
        ArgumentNullException.ThrowIfNull(players);
        uint stride = _pool.Edition == OriginalEdition.Patch1078 ? 0xb4u : 0xacu;
        if ((uint)players.Length != 9 * stride)
        {
            throw new ArgumentException("FortFinishOwner 실제9 Player 크기 오류", nameof(players));
        }
        _players = players;
    }

    /// <summary>
    /// entry 최초의 지역 snapshot만 같은 원문에서 읽는다. 후속 반복에서 지역을 다시 고르지 않는다.
    /// 미연결 독립 모듈은 기존 상태, 캠페인은 공유 Player 첫 지역 DWORD를 조회한다.
    /// </summary>
    /// <param name="owner">조회할 소유자다.</param>
    /// <returns>최초 지역이다.</returns>
    private int PlayerRegion(uint owner)
    {
        if (_players.Length == 0)
        {
            return _state.PlayerRegions[(int)owner];
        }
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        uint field = owner * (patch ? 0xb4u : 0xacu) + (patch ? 0x78u : 0x70u);
        uint value = 0;
        // 정확 네 바이트를 조합하여 stale 주변 필드를 보존한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)_players[(int)(field + (uint)index)] << (index * 8);
        }
        return unchecked((int)value);
    }

    /// <summary>
    /// 도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort owner 후처리의 실제 장치가 연결되지 않았다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort owner 후처리의 실제 장치 누락");
        }
        return function;
    }

    /// <summary>
    /// 누락된 finishOwner를 자연 반환·반복의 실제 몸체로만 교체한다.
    /// </summary>
    /// <param name="pool">검사할 풀이다.</param>
    /// <param name="finish">같은 월드의 후처리 몸체다.</param>
    /// <param name="hooks">물을 훅 묶음이다.</param>
    /// <returns>후처리가 연결된 훅 묶음이다.</returns>
    /// <exception cref="ArgumentException">Fort loader·owner 후처리 SID 풀이 다르다.</exception>
    public static FortObjectLoadHooks MakeFortFinishOwnerHooks(RawSidPool pool, RawFortFinishOwner finish, FortObjectLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(finish);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, finish.Pool()))
        {
            throw new ArgumentException("Fort loader·owner 후처리 SID 풀이 다릅니다");
        }
        hooks.FinishOwner = owner => finish.Finish(owner);
        return hooks;
    }
}
