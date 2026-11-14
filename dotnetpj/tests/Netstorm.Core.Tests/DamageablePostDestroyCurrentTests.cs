using Netstorm.Core.Original;

// 실제 현재 HP/프레임과 같은 장기 역할·목록을 원본 후처리 몸체에 조립한다.
namespace Netstorm.Core.Tests;

/// <summary>독립 원본 재생 장면을 재사용해 현재 전역과 부모 대여의 추가 계약을 검증한다.</summary>
public sealed partial class X86RawDamageablePostDestroyTests
{
    /// <summary>부모160과 주변 HP/다리/잔해 후보를 원본 입력으로 준비한다.</summary>
    private static Scene CurrentScene()
    {
        var input = new uint[18]; input[4] = 160; input[9] = input[10] = 2;
        input[16] = input[17] = BitConverter.SingleToUInt32Bits(50);
        var scene = new Scene(); scene.Prepare(input);
        scene.Types[160].Flags1 = 0x10; scene.Types[160].Flags2 = 0x10000; scene.Types[160].Group = 0;
        scene.Types[160].MaxHitPoints = 100;
        return scene;
    }

    /// <summary>원본 입력의 네 BYTE 코드를 실제 프레임 부모로 준비한다.</summary>
    private static RiftTypeFrames[] CurrentCodes(Scene scene) => scene.Frames.Select(codes =>
        new RiftTypeFrames(codes.Select(code => new RiftFrameCode((byte)code.Side, (byte)code.Variant, (byte)code.Number, (byte)code.Flags)))).ToArray();

    /// <summary>같은 TYPE·풀/해시와 실제 보상 HP·원문 프레임을 빌리는 후처리기를 만든다.</summary>
    private static RawDamageablePostDestroy CurrentBody(Scene scene, DamageablePostDestroyHooks hooks, RiftTypeFrames[]? frames = null)
    {
        var store = new RawSpStore(new GameRandom(1), () => 1); store.Initialize();
        var hp = new RawSquidReward(scene.Pool, scene.Types, new RawPostPopState(), new SquidRewardState(), store);
        hp.BindCurrentTypes(scene.Types);
        var footprint = new RawSquidNeighbors(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames);
        return new(scene.Pool, scene.Hash, scene.Types, frames ?? CurrentCodes(scene), hp, footprint, scene.State, hooks);
    }

    /// <summary>전체DWORD 목록 개수를 signed로 읽으며 독립 개수와 배열 용량 검사를 섞지 않는다.</summary>
    [Theory]
    [InlineData(0u, 5)]
    [InlineData(20u, 5)]
    [InlineData(21u, 2)]
    [InlineData(uint.MaxValue, 5)]
    [InlineData(0x80000000u, 5)]
    public void CurrentPopulationKeepsSignedDwordCount(uint count, int drops)
    {
        var s = CurrentScene(); s.State.DudeCount = 999;
        s.State.SharedDudePopulation = new RawPostPopList { Count = count };
        CurrentBody(s, s.Hooks()).Drops(50, 0x4200, false);
        Assert.Equal(drops, s.Events.Count(value => value.StartsWith("D:")));
    }

    /// <summary>각 제외/특별 역할은 현재 주소를 읽고 생성 루프는 처음 정한 수량을 유지한다.</summary>
    [Fact]
    public void CurrentDropRolesAndCapturedCountUseOriginalOrder()
    {
        var s = CurrentScene(); s.State.ExcludedDropTypes[0] = 160;
        s.State.SharedExcludedDropTypes[0] = new UIntBox { Value = 999 };
        var hooks = s.Hooks(); var post = CurrentBody(s, hooks);
        post.Drops(50, 0x4200, true); Assert.Equal(5, s.Events.Count);
        s.Events.Clear(); s.State.SharedExcludedDropTypes[4] = new UIntBox { Value = 160 };
        post.Drops(50, 0x4200, true); Assert.Empty(s.Events);
        s.State.SharedExcludedDropTypes[4]!.Value = 999;
        var special = new UIntBox { Value = 160 }; var population = new RawPostPopList();
        s.State.SharedSpecialDropType = special; s.State.SharedDudePopulation = population;
        post.Drops(50, 0x4200, true); Assert.Equal(3, s.Events.Count);
        s.Events.Clear(); special.Value = 999;
        hooks.DropDude = (sid, index, random) => { s.Events.Add($"{sid}:{index}:{random}"); special.Value = 160; population.Count = 21; };
        post = CurrentBody(s, hooks); hooks.DropDude = (_, _, _) => throw new InvalidOperationException("원문 함수 사본 변경");
        post.Drops(50, 0x4200, false); Assert.Equal(5, s.Events.Count);
        s.Events.Clear(); post.Drops(50, 0x4200, false); Assert.Single(s.Events);
    }

    /// <summary>현재 battle과 각 예약 시점의 피해 이벤트를 같은 실제 HP 부모와 연결한다.</summary>
    [Fact]
    public void CurrentBattleAndDamageEventAreReadAtTheirConsumers()
    {
        var s = CurrentScene(); s.State.Battle = false; s.State.SharedBattle = () => true;
        s.Pool.RawSlot(61)[40] = 0;
        var role = new UIntBox { Value = 0x01020304 }; s.State.DamageEvent = 999; s.State.SharedDamageEvent = role;
        var calls = new List<(uint Event, uint Count)>(); var hooks = s.Hooks();
        hooks.Regular = (_, ev, payload, packed) => { Assert.Equal(0f, payload); calls.Add((ev, packed)); role.Value++; };
        CurrentBody(s, hooks).PostDestroy(50, 5u << 16);
        Assert.Equal(2, calls.Count); Assert.Equal(0x01020304u, calls[0].Event); Assert.Equal(0x01020305u, calls[1].Event);
        Assert.All(calls, value => Assert.Equal(5u * 65536 + 51, value.Count));
    }

    /// <summary>같은 현재 섬 정리 제외 역할과 그 효과 뒤의 battle을 읽는다.</summary>
    [Fact]
    public void CurrentSilentRoleAndBattleFollowIslandEffects()
    {
        var s = CurrentScene(); s.Types[160].Flags1 |= 0x400;
        var silent = new UIntBox { Value = 160 }; bool battle = false; int islands = 0, regulars = 0;
        s.State.SharedSilentSurfaceType = silent; s.State.SharedBattle = () => battle;
        var hooks = s.Hooks(); hooks.DropIsland = (_, _, _, _) => { islands++; battle = true; };
        hooks.Regular = (_, _, _, _) => regulars++;
        var post = CurrentBody(s, hooks); post.PostDestroy(50, 0); Assert.Equal(0, islands); Assert.Equal(0, regulars);
        silent.Value = 999; post.PostDestroy(50, 0); Assert.Equal(1, islands); Assert.True(regulars > 0);
    }

    /// <summary>실제 다리 조회 효과 뒤 현재 boss를 읽으며 이후 호출에서도 같은 주소를 유지한다.</summary>
    [Fact]
    public void CurrentBossIsReadAfterBridgeFallLookup()
    {
        var s = CurrentScene(); s.State.SharedBattle = () => true; s.Types[160].Flags2 = 0; s.Types[160].Group = 2;
        Put(s.Pool.RawSlot(63), 36, 1); bool boss = true, flip = true;
        s.State.Boss = true; s.State.SharedBoss = () => boss;
        var hooks = s.Hooks(); hooks.HasBridgeFall = _ => { if (flip) boss = false; return false; };
        var post = CurrentBody(s, hooks); post.PostDestroy(50, 0); Assert.DoesNotContain(s.Events, value => value.StartsWith("X:"));
        s.Events.Clear(); boss = true; flip = false; post.PostDestroy(50, 0);
        Assert.Contains("X:63:0", s.Events); Assert.Contains("X:64:0", s.Events);
    }

    /// <summary>실제 Notify 뒤 같은 현재 프레임 배열을 다시 읽고 원문 금간 검색을 수행한다.</summary>
    [Fact]
    public void CurrentRiftFramesRemainBorrowedAcrossNotify()
    {
        var s = CurrentScene(); RiftTypeFrames[] frames = CurrentCodes(s); var hooks = s.Hooks();
        hooks.NotifySurface = _ => frames[82] = new RiftTypeFrames([new((byte)'G', (byte)'P', 1, 0), new((byte)'G', (byte)'P', 9, 0), new((byte)'G', (byte)'P', 11, 0x20)]);
        var post = CurrentBody(s, hooks, frames); post.Weaken(63);
        Assert.Equal(2u, Get(s.Pool.Slot(63), 36)); Assert.Contains("U:63:2", s.Events);
    }

    /// <summary>HP를 선택하지 않는 실제 CD 경로에서 제외4개·extra35·프레임34의1BYTE 쓰기를 검증한다.</summary>
    [Fact]
    public void CdDropAndFrameWidthsPreserveAdjacentBytesWithoutSelectingHp()
    {
        var pool = new RawSidPool(24000, false, edition: OriginalEdition.Cd1072);
        int sid = pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client); pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[10] = 82;
        OriginalTypeRecord[] physical = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        OriginalTypeRecord[] types = physical.Take(171).ToArray(); var hash = new RawSquidHash();
        var codes = new Netstorm.Assets.FrameCode[] { new('A', 'P', 1, 0), new('A', 'P', 11, (Netstorm.Assets.FrameCodeFlags)0x20) };
        Netstorm.Assets.FrameCode[][] frames = Enumerable.Range(0, 171).Select(_ => codes).ToArray();
        // 기존 HP 부모만188칸이며 이 검사는 HP 조회에 도달하지 않는다. 현재 CD HP 조립 완료로 세지 않는다.
        var hp = new RawHitPoints(pool, physical, new RawHitPointState());
        var neighbors = new RawSquidNeighbors(pool, hash, new byte[65536], types, frames);
        var state = new DamageablePostDestroyState(); state.ExcludedDropTypes[4] = 82; int dropped = 0, refreshed = 0;
        var hooks = new DamageablePostDestroyHooks
        {
            DropDude = (_, _, _) => dropped++, DropIsland = (_, _, _, _) => Assert.Fail("CD HP 미선택 검사"),
            Regular = (_, _, _, _) => Assert.Fail("CD HP 미선택 검사"), HasBridgeFall = _ => false,
            NotifySurface = _ => { }, DisplayUpdate = _ => refreshed++, Destroy = (_, _) => Assert.Fail("CD 삭제 미선택"), Base = (_, _) => { },
        };
        var post = new RawDamageablePostDestroy(pool, hash, types, frames, hp, neighbors, state, hooks);
        post.Drops(sid, 0x4200, false); Assert.Equal(5, dropped);
        pool.RawSlot(sid)[35] = 1; post.Drops(sid, 0x4200, false); Assert.Equal(5, dropped);
        pool.RawSlot(sid)[35] = 0x80; pool.RawSlot(sid)[34] = 0;
        pool.RawSlot(sid)[36..].Fill(0xa7); Put(pool.RawSlot(sid), 12, 0x7355, 2);
        post.Weaken(sid); Assert.Equal(1, pool.Slot(sid)[34]); Assert.Equal(0x80, pool.Slot(sid)[35]);
        Assert.Equal(0x7325u, Get(pool.Slot(sid), 12, 2)); Assert.Equal(1, refreshed);
        Assert.All(pool.Slot(sid)[36..].ToArray(), value => Assert.Equal(0xa7, value));
    }
}
