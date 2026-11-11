namespace Netstorm.Core.Original;

/// <summary>
/// 군집의 초기 군도 분기가 쓰는 현재 지역이다
/// (cpppj <c>o/RawFarmCluster.h</c>의 <c>FarmClusterState</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 호출자가 소유하며 이 모듈보다 오래 살아야 한다.</remarks>
public sealed class FarmClusterState
{
    /// <summary>원본 군도 모드이며 지역에서만 군집 범위 수량·반경을 바꾼다.</summary>
    public bool Archipelago { get; set; }

    /// <summary>위치 00542654·CD 00537188의 현재 섬 확장 범위다.</summary>
    public int ArchipelagoSpan { get; set; } = 1;

    /// <summary>로그인 bb4와 별개인 현재 bb0 DWORD를 직접 빌린다.</summary>
    public Func<uint>? SharedArchipelago { get; set; }

    /// <summary>군도 root·간헐천·보충과 같은 00542654/CD00537188을 직접 빌린다.</summary>
    public Func<int>? SharedArchipelagoSpan { get; set; }

    /// <summary>실제 FarmCluster는 로그인 bb4와 별개인 현재 bb0 DWORD를 읽고 반대 bool 사본을 무시한다.</summary>
    /// <returns>군도 모드이면 참이다.</returns>
    public bool CurrentArchipelago()
    {
        return SharedArchipelago is null ? Archipelago : SharedArchipelago() != 0;
    }

    /// <summary>빌린 확장 owner가 있으면 그 값을 읽고 없으면 자체값을 읽는다.</summary>
    /// <returns>현재 섬 확장 범위다.</returns>
    public int CurrentSpan()
    {
        return SharedArchipelagoSpan is null ? ArchipelagoSpan : SharedArchipelagoSpan();
    }
}

/// <summary>
/// CD에서도 지역 record 조회에서 보고하고 원본처럼 계속 진행되는 진단이다
/// (cpppj <c>o/RawFarmCluster.h</c>의 <c>FarmClusterAssertion</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다.</remarks>
public enum FarmClusterAssertion
{
    /// <summary>현재 IslandRuntime 요구에 존재 DWORD가 없다.</summary>
    MissingIsland,

    /// <summary>현재 ChunkMap에서 요청 지역 첫 지형 record를 찾지 못했다.</summary>
    MissingRegionRecord,
}

/// <summary>
/// 실제 단일 배치와 원본 진단이 필수 효과이며 군집의 성공을 기본값으로 성공하지 않는다
/// (cpppj <c>o/RawFarmCluster.h</c>의 <c>FarmClusterHooks</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FarmClusterHooks
{
    /// <summary>같은 local 난수를 쓰는 실제 배치 전체를 연결한다.</summary>
    public Func<GameRandom, FortRandomPlaceRequest, uint>? Place { get; set; }

    /// <summary>CD 진단의 record 0·월드 0으로 계속되는 흐름을 보존한다.</summary>
    public Action<FarmClusterAssertion>? Assertion { get; set; }
}

/// <summary>
/// 실제 가장자리 농지 군집을 지역·군도·archipelago·중심·주변·실패 예산으로 기존 단일 배치를 같은 월드에 실행한다
/// (cpppj <c>o/RawFarmCluster</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 단일 배치는 완료 몸체여야 하며 검사가 이어준다.</remarks>
public sealed class RawFarmCluster
{
    /// <summary>원본 시드 배율·수량 배율과 중심 실패 한도다.</summary>
    private const uint SeedFactor = 77u;

    /// <summary>원본 수량 배율이다.</summary>
    private const uint QuantityFactor = 20u;

    /// <summary>원본 중심 실패 한도다.</summary>
    private const uint FailureLimit = 30000u;

    /// <summary>
    /// 자료와 필수 효과가 서로 다른 월드에 연결되지 않게 검사한다.
    /// </summary>
    /// <param name="pool">같은 판본의 같은 raw다.</param>
    /// <param name="chunks">실제 지역 record와 단일 지역 검사에 같은 지도다.</param>
    /// <param name="islands">실제 CD Exists·유효 번호가 있는 같은 3204바이트 mutable 목록이다.</param>
    /// <param name="placementState">매시 현재 가장자리 농지 타입을 읽는다. 장치 중 바뀐 뒤 다음 호출도 반영한다.</param>
    /// <param name="state">군집 진입의 군도·확장 범위·지역이다.</param>
    /// <param name="hooks">실제 단일 배치와 원본 지역 진단이다.</param>
    /// <exception cref="ArgumentException">FarmCluster의 같은 월드·필수 효과 누락이다.</exception>
    public RawFarmCluster(
        RawSidPool pool,
        ChunkMap chunks,
        RawIslandRuntime islands,
        FortRandomPlacementState placementState,
        FarmClusterState state,
        FarmClusterHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(chunks);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(placementState);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, islands.Pool()) || hooks.Place is null || hooks.Assertion is null)
        {
            throw new ArgumentException("FarmCluster 같은 월드·필수 효과 누락");
        }
        _pool = pool;
        _chunks = chunks;
        _islands = islands;
        _placementState = placementState;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>같은 판본의 같은 raw 저장소·진단 규칙을 제공한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>실제 지역 record와 단일 배치 지역 검사가 공유하는 지도다.</summary>
    private readonly ChunkMap _chunks;

    /// <summary>실제 CD Exists·유효 번호가 있는 같은 3204바이트 mutable 목록이다.</summary>
    private readonly RawIslandRuntime _islands;

    /// <summary>매시 현재 가장자리 농지 타입을 읽는다.</summary>
    private readonly FortRandomPlacementState _placementState;

    /// <summary>군집 진입의 군도·확장 범위·지역이다.</summary>
    private readonly FarmClusterState _state;

    /// <summary>실제 단일 배치와 원본 지역 진단이다.</summary>
    private readonly FarmClusterHooks _hooks;

    /// <summary>두 판본의 localRandom 도우미가 쓰는 CRT 64비트 절삭의 하위 DWORD다.</summary>
    /// <param name="value">자를 double 값이다.</param>
    /// <returns>자를 수 없으면 0, 아니면 하위 32비트다.</returns>
    private static int Truncate(double value)
    {
        if (!double.IsFinite(value) || Math.Abs(value) >= 9223372036854775808.0)
        {
            return 0;
        }
        long cut = (long)value;
        return unchecked((int)unchecked((uint)cut));
    }

    /// <summary>한도가 1보다 작으면 원본 local 난수를 소비하지 않고 0을 반환한다.</summary>
    /// <param name="random">소비할 local 난수다.</param>
    /// <param name="limit">상한이다.</param>
    /// <returns>0 이상 한도 미만의 값이다.</returns>
    private static uint Next(GameRandom random, int limit)
    {
        return limit < 1 ? 0u : random.Next(unchecked((uint)limit));
    }

    /// <summary>실제 지역 record 조회는 현재 owner가 아니라 덩어리의 원래 지형 record를 읽는다.</summary>
    /// <param name="region">조회할 지역이다.</param>
    /// <returns>원본 시드다.</returns>
    private uint RegionSeed(int region)
    {
        bool cd = _pool.Edition == OriginalEdition.Cd1072;
        if (cd && _islands.Exists(region) == 0)
        {
            Need(_hooks.Assertion)(FarmClusterAssertion.MissingIsland);
        }
        // 원본 선형 검색은 x가 먼저 증가하고 끝에서 y가 증가한다
        for (int y = 0; y < _chunks.YLength(); y++)
        {
            // 같은 현재 ChunkMap 조회 getter를 재사용한다
            for (int x = 0; x < _chunks.XLength(); x++)
            {
                ChunkEntry entry = _chunks.At(x, y);
                if ((int)unchecked((sbyte)entry.Island) == region)
                {
                    return (uint)entry.Record[3] * SeedFactor;
                }
            }
        }
        if (cd)
        {
            Need(_hooks.Assertion)(FarmClusterAssertion.MissingRegionRecord);
        }
        return 0;
    }

    /// <summary>수량×20 성공 목표로 중심과 주변을 배치한다. 수량 0은 원본처럼 첫 중심을 시도한다.</summary>
    /// <param name="area">군집 사각형이다.</param>
    /// <param name="region">군집 지역이다.</param>
    /// <param name="quantity">성공 목표 수량이다.</param>
    public void Place(FortRandomRectangle area, int region, uint quantity)
    {
        GameRandom random = new(RegionSeed(region));
        uint remaining = unchecked(quantity * QuantityFactor);
        uint force = 0;
        int radius = 1;
        if (_state.CurrentArchipelago() && region == 0)
        {
            uint span = unchecked((uint)_state.CurrentSpan());
            uint extension = unchecked(span * 16u);
            int start = unchecked((int)(extension - 16u));
            int end = unchecked((int)extension);
            area = new FortRandomRectangle(
                (float)((double)area.Left - start),
                (float)((double)area.Top - start),
                (float)((double)area.Right + end),
                (float)((double)area.Bottom + end));
            uint width = unchecked(span * 2u + 2u);
            remaining = unchecked(unchecked((uint)(unchecked((int)remaining) / 9)) * width * width);
            force = 1;
            radius = 2;
        }
        int height = Truncate((double)area.Bottom - area.Top);
        int width2 = Truncate((double)area.Right - area.Left);
        uint failures = FailureLimit;
        // 원본은 최초 중심을 항상 시도하며 성공 중심마다 실패 한도를 재설정한다
        do
        {
            uint dy = Next(random, height);
            uint dx = Next(random, width2);
            float x = (float)((double)dx + area.Left);
            float y = (float)((double)dy + area.Top);
            if (Need(_hooks.Place)(random, new FortRandomPlaceRequest(_placementState.EdgeFarmType, x, y, region, 1, force)) != 0)
            {
                remaining = unchecked(remaining - 1);
                failures = FailureLimit;
                // 원본 주변 순회는 x 바깥·y 안쪽이며 중심 좌표도 다시 시도한다
                for (int ox = -radius; ox <= radius && remaining != 0; ox++)
                {
                    // 성공할 때만 남은 수를 감소시키고 0이면 남은 주변 시도를 멈춘다
                    for (int oy = -radius; oy <= radius && remaining != 0; oy++)
                    {
                        float nx = (float)((double)x + ox);
                        float ny = (float)((double)y + oy);
                        if (Need(_hooks.Place)(random, new FortRandomPlaceRequest(_placementState.EdgeFarmType, nx, ny, region, 1, force)) != 0)
                        {
                            remaining = unchecked(remaining - 1);
                        }
                    }
                }
            }
            if (remaining == 0)
            {
                break;
            }
        } while ((failures = unchecked(failures - 1)) != 0);
    }

    /// <summary>단일 월드와 실제 단일 배치 묶음을 후속 실제 월드 소비자에게 제공한다.</summary>
    /// <returns>이 모듈이 쓰는 풀이다.</returns>
    public RawSidPool Pool()
    {
        return _pool;
    }

    /// <summary>완료된 단일 배치의 실제 공간 검사·owner·타입 변경·등록을 군집에서 그대로 소비한다.</summary>
    /// <param name="pool">검사할 풀이다.</param>
    /// <param name="placement">같은 월드의 단일 배치 몸체다.</param>
    /// <param name="hooks">물을 훅 묶음이다.</param>
    /// <returns>배치가 연결된 훅 묶음이다.</returns>
    /// <exception cref="ArgumentException">FarmCluster 단일 배치 SID 풀이 다르다.</exception>
    public static FarmClusterHooks MakeFarmClusterPlacementHook(RawSidPool pool, RawFortRandomPlacement placement, FarmClusterHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(placement);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, placement.Pool()))
        {
            throw new ArgumentException("FarmCluster 단일 배치 SID 풀이 다릅니다");
        }
        hooks.Place = (random, request) => placement.Place(random, request);
        return hooks;
    }

    /// <summary>도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">FarmCluster 실제 장치 누락이다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("FarmCluster 실제 장치 누락");
        }
        return function;
    }
}
