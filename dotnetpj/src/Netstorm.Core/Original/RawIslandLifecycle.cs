// 섬 받침의 사전 파괴 재정의(00442240)와 무인도 표면의 사후 파괴 재정의(00442640)를 복원한다.
using Netstorm.Assets;
using Netstorm.Core.Bridges;

namespace Netstorm.Core.Original;

// 섬 수명주기 모듈을 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.

// 섬 받침 사전 파괴가 둘레 다리의 지연 낙하 예약을 건너뛰는 삭제 플래그 비트 값이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public static class IslandLifecycleFlags
{
    /// <summary>원본 flags 0x1000이다. 켜지면 다리를 건드리지 않고 공통 사전 파괴만 부른다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public const uint KeepsBridges = 0x1000u;
}

// 두 재정의가 부르는 외부 효과다. 호출 순서는 원본과 같다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class IslandLifecycleHooks
{
    /// <summary>다리 칸에 지연 낙하 예약을 요청한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int, float, float>? ScheduleFall { get; set; }

    /// <summary>칸 위 걷는 유닛의 낙하를 요청한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int>? FallWalker { get; set; }

    /// <summary>공통 사전 파괴다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int, uint>? BasePreDestroy { get; set; }

    /// <summary>공통 사후 파괴다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int, uint>? BasePostDestroy { get; set; }
}

// 섬 받침의 사전 파괴 재정의와 무인도 표면의 사후 파괴 재정의를 복원한 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class RawIslandLifecycle
{
    /// <summary>슬롯 안 타입 번호의 위치다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 현재 가로 좌표의 위치다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 현재 세로 좌표의 위치다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int YOffset = 18;

    /// <summary>상태 바이트의 죽음 비트다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const byte DeadBit = 2;

    /// <summary>추가 비트의 추상·매립 마스크다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const byte AbstractOrBuriedMask = 9;

    /// <summary>한 칸 탐색기에 넘기는 호출 플래그다. 해시 0단계를 건너뛴다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const uint SkipSurfaceLevel = 1u;

    /// <summary>슬롯을 읽는 풀이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>발자국에 인접한 표면·객체를 조회하는 검색기다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly RawSquidNeighbors _neighbors;

    /// <summary>외부 저장소 조회와 가상 동작을 연결하는 묶음이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IslandLifecycleHooks _hooks;

    /// <summary>
    /// 풀과 이웃 탐색기는 이 객체보다 오래 살아야 하고 같은 풀을 읽어야 한다. 훅은 모두 연결돼 있어야 한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="neighbors">이웃 탐색기다.</param>
    /// <param name="hooks">외부 효과 묶음이다.</param>
    /// <exception cref="ArgumentException">섬 삭제 훅의 풀 불일치·미연결이다.</exception>
    public RawIslandLifecycle(RawSidPool pool, RawSquidNeighbors neighbors, IslandLifecycleHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(neighbors);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(neighbors.Pool, pool))
        {
            throw new ArgumentException("섬 삭제 훅의 이웃 탐색기 풀이 다릅니다", nameof(neighbors));
        }

        if (hooks.ScheduleFall is null || hooks.FallWalker is null || hooks.BasePreDestroy is null || hooks.BasePostDestroy is null)
        {
            throw new ArgumentException("섬 삭제 훅이 모두 연결되어야 합니다", nameof(hooks));
        }

        _pool = pool;
        _neighbors = neighbors;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 00442240 받침 사전 파괴다. 받침이 추상·매립이 아니면 중심을 구하고 이웃을 순회한다.
    /// 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
    /// </summary>
    /// <param name="island">받침 번호다.</param>
    /// <param name="flags">삭제 플래그다.</param>
    public void IslandPreDestroy(int island, uint flags)
    {
        int extraOffset = _pool.Edition == OriginalEdition.Patch1078 ? 0x28 : 0x23;
        if ((_pool.Slot(island)[extraOffset] & AbstractOrBuriedMask) == 0)
        {
            // 중심은 탐색기를 만들기 전에 한 번 구한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            float[] center = _neighbors.Center(island);
            bool keepBridges = (flags & IslandLifecycleFlags.KeepsBridges) != 0u;
            RawSquidNeighborWalk walk = new(_neighbors, island, 0u);
            // 예약 훅이 풀을 바꿀 수 있으므로 다음 이웃은 그 뒤의 상태에서 읽는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (int other = walk.Current; other != 0; other = walk.Next())
            {
                if (!keepBridges && (Genus(other) & TypeFlag2.Bridge) != 0u && (_pool.Slot(other)[StateOffset] & DeadBit) == 0)
                {
                    _hooks.ScheduleFall!(other, center[0], center[1]);
                }
            }
        }

        _hooks.BasePreDestroy!(island, flags);
    }

    /// <summary>
    /// 00442640 표면 사후 파괴다. 표면 칸이 추상·매립이 아니면 그 칸에 걸린 걷는 유닛마다 가상 낙하를 부른다.
    /// 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
    /// </summary>
    /// <param name="surface">표면 칸 번호다.</param>
    /// <param name="flags">삭제 플래그다.</param>
    public void SurfacePostDestroy(int surface, uint flags)
    {
        int extraOffset = _pool.Edition == OriginalEdition.Patch1078 ? 0x28 : 0x23;
        if ((_pool.Slot(surface)[extraOffset] & AbstractOrBuriedMask) == 0)
        {
            ReadOnlySpan<byte> raw = _pool.Slot(surface);
            int x = Cell(Coordinate(raw, XOffset));
            int y = Cell(Coordinate(raw, YOffset));
            RawSquidFinder finder = new(_pool, _neighbors.Hash, _neighbors.Types);
            // 그 칸에 걸린 객체를 일반 탐색 순서로 훑는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (int current = finder.Begin(new SquidSearchArea(x, y, x, y), SkipSurfaceLevel); current != 0; current = finder.Next())
            {
                if ((Genus(current) & TypeFlag2.Walker) != 0u)
                {
                    _hooks.FallWalker!(current);
                }
            }
        }

        _hooks.BasePostDestroy!(surface, flags);
    }

    /// <summary>객체 타입의 분류 비트다. 표 밖이면 원본은 표 뒤를 읽지만 여기서는 거부한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>분류 비트다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">섬 삭제 훅 타입 범위 오류다.</exception>
    private uint Genus(int sid)
    {
        IReadOnlyList<OriginalTypeRecord> types = _neighbors.Types;
        uint number = _pool.Slot(sid)[TypeOffset];
        if (number >= (uint)types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "섬 삭제 훅 타입 범위 오류");
        }

        return types[(int)number].Flags2;
    }

    /// <summary>정렬에 기대지 않고 바이트를 합쳐 단정밀도 좌표를 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>좌표 값이다.</returns>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset)
    {
        uint bits = 0u;
        // 낮은 바이트부터 합치는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < 4; ++i)
        {
            bits |= (uint)raw[offset + i] << (8 * i);
        }

        return BitConverter.UInt32BitsToSingle(bits);
    }

    /// <summary>원본 자름·_ftol의 칸 번호다. 비유한·64비트 밖은 하위 0이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>칸 번호다.</returns>
    private static int Cell(float value)
    {
        double wide = value;
        if (!(wide > -9223372036854775808.0 && wide < 9223372036854775808.0))
        {
            return 0;
        }

        return unchecked((int)unchecked((uint)(long)wide));
    }
}
