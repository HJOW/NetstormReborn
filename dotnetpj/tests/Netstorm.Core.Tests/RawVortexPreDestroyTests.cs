using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 신전 삭제 전처리(RawVortexPreDestroy)의 합성 경계를 확인한다.
/// x86 재생이 덮지 않는 저장소 크기·미연결 효과·가림 비트·범위·로딩·Player 정리를 검사한다.
/// 기준 판본은 10.78 이다 (W03 신전 삭제 조각, 2026-10-23).
/// </summary>
public sealed class RawVortexPreDestroyTests
{
    /// <summary>장면 풀의 슬롯 수(원본 관찰과 같다).</summary>
    private const int PoolCapacity = 24000;

    /// <summary>삭제 대상 신전 번호.</summary>
    private const int Temple = 50;

    /// <summary>삭제에 쓰는 플래그(원본 관찰과 같다).</summary>
    private const uint TestFlags = 0x400000;

    /// <summary>값을 리틀 엔디언으로 Player 기록에 쓴다.</summary>
    /// <param name="bytes">대상 Player 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>Player 기록의 DWORD 를 읽는다.</summary>
    /// <param name="bytes">Player 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Get(byte[] bytes, int offset)
    {
        // 네 바이트를 낮은 바이트부터 합친다
        return (uint)(bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24));
    }

    /// <summary>합성 장면이 공유하는 풀·목록·상태·기록.</summary>
    private sealed class Scene
    {
        /// <summary>신전과 outpost 가 공유하는 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>Player 기록 1,620바이트.</summary>
        public readonly byte[] Players = new byte[9 * 0xb4];

        /// <summary>신전 등록 목록.</summary>
        public readonly RawPostPopList Temples = new();

        /// <summary>거주 구역 등록 목록.</summary>
        public readonly RawPostPopList Residences = new();

        /// <summary>outpost 목록.</summary>
        public readonly RawPostPopList Outposts = new();

        /// <summary>로딩 깊이 상태.</summary>
        public readonly VortexPreDestroyState State = new();

        /// <summary>효과 기록.</summary>
        public readonly List<string> Events = [];

        /// <summary>장면을 만든다. 소유자 1·가림 없음·로딩 0·두 참조 50이 기본값이다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(PoolCapacity, false);
            // 61까지 클라이언트 슬롯을 순서대로 확보한다
            for (;;)
            {
                int got = Pool.Allocate(SidAllocation.Client);
                // 61을 넘기면 준비 오류다
                if (got > 61)
                {
                    throw new InvalidOperationException("신전 합성 SID 준비 오류");
                }
                if (got == 61)
                {
                    break;
                }
            }
            Array.Fill(Players, (byte)0xa5);
            // 두 Player 참조를 모두 삭제 번호로 둔다
            for (int player = 0; player < 9; player++)
            {
                Put(Players, player * 0xb4 + 0x7c, 50);
                Put(Players, player * 0xb4 + 0x80, 99);
                Put(Players, player * 0xb4 + 0x84, 50);
                Put(Players, player * 0xb4 + 0x88, 99);
            }
            // 신전 50과 outpost 2개를 원본 관찰과 같은 값으로 둔다
            foreach (int sid in new[] { 50, 60, 61 })
            {
                var bytes = new byte[50];
                bytes[10] = sid == 50 ? (byte)124 : (byte)126;
                bytes[11] = sid == 50 ? (byte)2 : (byte)0;
                bytes[34] = 1;
                bytes[40] = 0;
                Put(bytes.AsSpan(), 14, BitConverter.SingleToUInt32Bits((float)sid + 0.25f));
                Put(bytes.AsSpan(), 18, BitConverter.SingleToUInt32Bits((float)sid + 0.5f));
                bytes.CopyTo(Pool.AllocatedBytes(sid));
            }
            Temples.Entries = [50, 60, 50, 61];
            Temples.Count = 4;
            Residences.Entries = [62, 50, 63, 50];
            Residences.Count = 4;
            Outposts.Entries = [60, 61];
            Outposts.Count = 2;
        }

        /// <summary>기본 여덟 경계를 기록용으로 채운다.</summary>
        public VortexPreDestroyHooks Hooks()
        {
            return new VortexPreDestroyHooks
            {
                IslandNumber = (x, y) =>
                {
                    Events.Add("island");
                    return 2;
                },
                IslandExists = _ =>
                {
                    Events.Add("exists");
                    return true;
                },
                IslandTemple = _ =>
                {
                    Events.Add("temple");
                    return 50;
                },
                ClearIslandTemple = _ => Events.Add("clear"),
                IslandOwner = (_, _) => Events.Add("owner"),
                RegionOwnership = (_, _) => Events.Add("region"),
                DamageablePre = (_, _) => Events.Add("base"),
                RefreshProduction = () => Events.Add("refresh"),
                Assertion = _ => Events.Add("assert"),
            };
        }
    }

    /// <summary>
    /// 필수 Damageable 하위가 없으면 목록·지역의 앞선 실제 효과 뒤 원본 Player 정리를 실행하지 않는다.
    /// </summary>
    [Fact]
    public void RequiredDamageableKeepsEarlierWrites()
    {
        var scene = new Scene();
        byte[] before = (byte[])scene.Players.Clone();
        var hooks = scene.Hooks();
        hooks.DamageablePre = null;
        var body = new RawVortexPreDestroy(scene.Pool, scene.Players, scene.Temples, scene.Residences, scene.Outposts, scene.State, hooks);
        Assert.Throws<InvalidOperationException>(() => body.PreDestroy(Temple, TestFlags));
        // 목록은 이미 압축됐고 Player 는 그대로다
        Assert.Equal(2u, scene.Temples.Count);
        Assert.Equal(2u, scene.Residences.Count);
        Assert.Equal(before, scene.Players);
    }

    /// <summary>PATCH Player 저장소 크기가 아니면 생성자에서 거부한다.</summary>
    [Fact]
    public void BadPlayerStorageRejected()
    {
        var scene = new Scene();
        var small = new byte[180];
        Assert.Throws<ArgumentException>(() => new RawVortexPreDestroy(scene.Pool, small, scene.Temples, scene.Residences, scene.Outposts, scene.State, scene.Hooks()));
    }

    /// <summary>가림 비트가 있으면 섬·지역·생산 효과를 건너뛰고 공통 하위와 Player 정리만 한다.</summary>
    [Fact]
    public void ExtraBitsSkipIslandAndRefresh()
    {
        var scene = new Scene();
        scene.Pool.RawSlot(Temple)[40] = 9;
        var body = new RawVortexPreDestroy(scene.Pool, scene.Players, scene.Temples, scene.Residences, scene.Outposts, scene.State, scene.Hooks());
        body.PreDestroy(Temple, TestFlags);
        // 목록 제거를 건너뛰어 개수가 그대로다
        Assert.Equal(4u, scene.Temples.Count);
        Assert.Equal(4u, scene.Residences.Count);
        Assert.Equal(new List<string> { "base" }, scene.Events);
        // Player 정리는 가림과 무관하게 수행된다
        Assert.Equal(0u, Get(scene.Players, 1 * 0xb4 + 0x7c));
    }

    /// <summary>풀을 벗어나거나 16비트를 넘는 outpost 번호는 쓰기 전에 거부한다.</summary>
    [Fact]
    public void OutpostRangeRejected()
    {
        var scene = new Scene();
        // 70000 은 16비트를 넘어 잘라내면 풀 안 번호가 되므로 범위 검사가 없으면 통과한다
        scene.Outposts.Entries = [70000, 61];
        scene.Outposts.Count = 2;
        var body = new RawVortexPreDestroy(scene.Pool, scene.Players, scene.Temples, scene.Residences, scene.Outposts, scene.State, scene.Hooks());
        Assert.Throws<ArgumentOutOfRangeException>(() => body.PreDestroy(Temple, TestFlags));
    }

    /// <summary>소유자 바이트가 9 이상이면 Player 주소를 구하기 전에 거부한다.</summary>
    [Fact]
    public void OwnerRangeRejected()
    {
        var scene = new Scene();
        scene.Pool.RawSlot(Temple)[34] = 9;
        var body = new RawVortexPreDestroy(scene.Pool, scene.Players, scene.Temples, scene.Residences, scene.Outposts, scene.State, scene.Hooks());
        Assert.Throws<ArgumentOutOfRangeException>(() => body.PreDestroy(Temple, TestFlags));
    }

    /// <summary>로딩 중에는 가림이 없어도 생산 갱신을 부르지 않는다.</summary>
    [Fact]
    public void LoadingSuppressesRefresh()
    {
        var scene = new Scene();
        scene.State.Loading = 1;
        var body = new RawVortexPreDestroy(scene.Pool, scene.Players, scene.Temples, scene.Residences, scene.Outposts, scene.State, scene.Hooks());
        body.PreDestroy(Temple, TestFlags);
        Assert.DoesNotContain("refresh", scene.Events);
        Assert.Contains("region", scene.Events);
    }

    /// <summary>삭제 번호와 같은 두 Player 참조를 지우고 같은 값을 표시 필드에 복사한다.</summary>
    [Fact]
    public void PlayerMirrorCleanup()
    {
        var scene = new Scene();
        var body = new RawVortexPreDestroy(scene.Pool, scene.Players, scene.Temples, scene.Residences, scene.Outposts, scene.State, scene.Hooks());
        body.PreDestroy(Temple, TestFlags);
        // 소유자 1의 두 참조가 0으로 지워지고 표시 필드도 0이다
        Assert.Equal(0u, Get(scene.Players, 1 * 0xb4 + 0x7c));
        Assert.Equal(0u, Get(scene.Players, 1 * 0xb4 + 0x80));
        Assert.Equal(0u, Get(scene.Players, 1 * 0xb4 + 0x84));
        Assert.Equal(0u, Get(scene.Players, 1 * 0xb4 + 0x88));
    }
}
