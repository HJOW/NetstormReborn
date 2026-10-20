using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 건물 공간(<see cref="RawBuildingSpace"/>)의 보호 경로와 연결 지점 계약을 확인한다.
/// 원본 기대값 재생(X86RawBuildingSpaceTests)은 실제 Pop·Unpop 과 이어 전체 상태를 대조하고,
/// 이 검사들은 cpppj/tests/BuildingSpaceTests.cpp 의 보호 검사만 10.78 몫으로 옮겼다 (W02 건물 공간 조각, 2026-10-20).
/// </summary>
public sealed class RawBuildingSpaceTests
{
    /// <summary>관찰 대상 타입 번호(건물).</summary>
    private const int BuildingType = 74;

    /// <summary>섬 타입 번호.</summary>
    private const int IslandType = 82;

    /// <summary>다리 타입 번호.</summary>
    private const int BridgeType = 81;

    /// <summary>풀·타입·해시를 만들고 건물 공간을 묶는다.</summary>
    /// <param name="debug">진단 출력 여부.</param>
    private static (RawSidPool pool, RawSquidHash hash, OriginalTypeRecord[] types, RawBuildingSpace space, BuildingSpaceState state) Setup(bool debug = false)
    {
        var pool = new RawSidPool(24000, false);
        // 원본 관찰의 번호를 클라이언트 영역에 확보한다
        for (int id = 5; id <= 70; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
        var hash = new RawSquidHash();
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        types[IslandType].Flags1 = OriginalTypeRecord.Surface;
        types[BridgeType].Flags1 = OriginalTypeRecord.Surface;
        var state = new BuildingSpaceState { IslandType = IslandType, BridgeType = BridgeType, Debug = debug };
        return (pool, hash, types, new RawBuildingSpace(pool, hash, types, state), state);
    }

    /// <summary>
    /// 생성자는 10.78 타입 표와 범위 안 섬·다리 번호를 요구한다.
    /// 다른 풀·해시의 연결은 거부한다.
    /// </summary>
    [Fact]
    public void ConstructorAndBindingRejectMismatches()
    {
        var (pool, hash, types, _, state) = Setup();
        _ = new RawBuildingSpace(pool, hash, types, state);
        // 짧은 타입 표(CD 171개)는 받지 않는다
        Assert.ThrowsAny<Exception>(() => new RawBuildingSpace(pool, hash, types.Take(100).ToList(), state));
        // 섬·다리 번호가 표를 벗어나면 거부한다
        Assert.ThrowsAny<Exception>(() => new RawBuildingSpace(pool, hash, types, new BuildingSpaceState { IslandType = 500, BridgeType = BridgeType }));
        // 다른 풀·해시의 연결은 거부하고 같은 구성은 받는다
        var otherPool = new RawSidPool(24000, false);
        var otherHash = new RawSquidHash();
        var space = new RawBuildingSpace(pool, hash, types, state);
        Assert.ThrowsAny<Exception>(() => space.ValidateSpace(otherPool, hash));
        Assert.ThrowsAny<Exception>(() => space.ValidateSpace(pool, otherHash));
        space.ValidateSpace(pool, hash);
        Assert.Same(pool, space.Pool);
        Assert.Same(hash, space.Hash);
    }

    /// <summary>
    /// 지도 밖 칸의 표면은 0이며, 절삭 범위를 벗어난 좌표는 거부한다.
    /// 지면 타입이 섬이면 참이다.
    /// </summary>
    [Fact]
    public void SurfaceAndIslandFollowMapBounds()
    {
        var (pool, hash, types, space, _) = Setup();
        Assert.Equal(0, space.Surface(-1, 0));
        Assert.Equal(0, space.Surface(256, 0));
        Assert.Equal(0, space.Surface(0, 256));
        // 절삭 범위를 벗어난 좌표는 거부한다
        Assert.ThrowsAny<Exception>(() => space.HasIsland(1e20f, 2.5f));
        // 지면이 비어 있으면 거짓이다
        Assert.False(space.HasIsland(30f, 45f));
        // 섬 타입 지면이면 참이다
        hash.Bucket(0, 30f, 45f) = (ushort)60;
        Span<byte> ground = pool.RawSlot(60);
        ground.Clear();
        ground[10] = IslandType;
        Assert.True(space.HasIsland(30.9f, 45.1f));
        // 다리 타입 지면은 거짓이다
        ground[10] = BridgeType;
        Assert.False(space.HasIsland(30f, 45f));
    }

    /// <summary>
    /// 진단이 켜졌는데 다리 수명 nibble 이 7을 넘으면 멈춘다.
    /// 꺼져 있으면 바이트를 그대로 둔다.
    /// </summary>
    [Fact]
    public void NotifySurfaceDiagnosesBridgeLifeOnlyWhenDebug()
    {
        var (pool, _, _, space, state) = Setup(debug: true);
        Span<byte> raw = pool.AllocatedBytes(50);
        raw.Clear();
        raw[10] = BridgeType;
        raw[12] = 0x7e;
        raw[40] = 0;
        // 수명 15는 진단에서 멈춘다
        Assert.ThrowsAny<Exception>(() => space.NotifySurface(50));
        // 매립 추가이면 진단하지 않는다
        pool.AllocatedBytes(50)[40] = 8;
        space.NotifySurface(50);
        // 다리가 아니면 진단하지 않는다
        raw[10] = IslandType;
        raw[40] = 0;
        space.NotifySurface(50);
        // 진단을 끄면 바이트를 그대로 둔다
        state.Debug = false;
        space.NotifySurface(50);
        Assert.Equal(0x7e, pool.Slot(50)[12]);
    }

    /// <summary>
    /// 옆면 쓰기는 표면 타입·범위를 확인하고 지도 밖 번호는 조용히 넘긴다.
    /// </summary>
    [Fact]
    public void SidesGuardTypeRangeAndMapBounds()
    {
        var (pool, hash, types, space, _) = Setup();
        hash.Bucket(0, 30f, 45f) = (ushort)50;
        Span<byte> raw = pool.AllocatedBytes(50);
        raw.Clear();
        raw[10] = IslandType;
        raw[12] = 0x10;
        // 표면 타입의 옆면은 WORD 하위 비트를 정리한다
        space.UnpopSide(30, 45);
        Assert.Equal(0x10, pool.Slot(50)[12]);
        // 지도 밖 칸은 조용히 넘긴다
        space.PopSide(-1, 0, 30f, 45f);
        space.UnpopSide(256, 0);
        // 풀 범위를 벗어난 표면 번호도 조용히 넘긴다
        hash.Bucket(0, 5f, 5f) = 30000;
        string calm = Convert.ToHexStringLower(pool.Bytes);
        space.PopSide(5, 5, 30f, 45f);
        space.UnpopSide(5, 5);
        Assert.Equal(calm, Convert.ToHexStringLower(pool.Bytes));
        // 범위를 벗어난 표면 타입은 거부한다
        raw[10] = 255;
        Assert.ThrowsAny<Exception>(() => space.PopSide(30, 45, 30f, 45f));
    }
}
