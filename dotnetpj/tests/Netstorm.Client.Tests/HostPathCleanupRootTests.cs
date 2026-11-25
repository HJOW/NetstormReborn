using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 세션 Root급 실제 예약·의식·가상 삭제로 PathCleanup 상위 설치를 검증한다.
// 2026-11-25 1.034에서 WP-1 Cleanup 상위 설치용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>실제 세션 장치에 연결한 PathCleanup 상위 갈고리와 원문 순서 수행을 검사한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>Dais 체인까지 연결한 실제 세션과 0x6c 통지 기록기를 만든다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface, List<(uint Player, ConstructionMoneyNotice Notice)> Money) CleanupRootScene()
    {
        var (session, types, surface) = SquidPacketScene();
        session.BuildIslandPostPopDevices(types, session.WorldFrames);
        session.BuildTerrainBinding(types, session.TerrainDiagnostics, new IslandRuntimeHooks { SetResidenceOwner = (_, _) => { }, Assertion = _ => { } });
        session.BuildRegionOwnershipDevices(types);
        var money = new List<(uint Player, ConstructionMoneyNotice Notice)>();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { ConstructionMoney = (player, notice) => money.Add((player, notice)), FormPacket = _ => { } });
        return (session, types, surface, money);
    }

    /// <summary>실제 세션 해시에 원문 체인 순서대로 슬롯을 등록한다.</summary>
    private static void RegisterInHash(RawSquidHash hash, RawSidPool pool, int sid, float x, float y, int level = 1)
    {
        Span<byte> raw = pool.AllocatedBytes(sid);
        BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(4, 2), hash.Bucket(level, x, y));
        hash.Bucket(level, x, y) = unchecked((ushort)sid);
    }

    /// <summary>좌표·타입·소유자·상태를 직접 채운 원시 슬롯을 만든다. finder가 읽는 최소 필드만 둔다.</summary>
    private static int MakeRawSlot(HostSession session, uint type, float x, float y, uint owner, byte state = 0, byte extra = 0, uint vtable = 0)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, vtable);
        raw[RawSidPool.TypeOffset] = unchecked((byte)type);
        raw[RawSidPool.StateOffset] = state;
        raw[40] = extra;
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(14, 4), x);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(18, 4), y);
        raw[34] = unchecked((byte)owner);
        return sid;
    }

    /// <summary>세션 Root 정리 갈고리가 실제 예약·의식·가상 삭제 장치를 제공한다.</summary>
    [Fact]
    public void CleanupRootHooksAreRealSessionDevices()
    {
        (HostSession session, _, _, _) = CleanupRootScene();
        PathCleanupHooks root = session.BuildPathCleanupRootHooks();
        Assert.NotNull(root.ReserveShared);
        Assert.NotNull(root.Ritual);
        Assert.NotNull(root.Destroy);
        Assert.True(root.ReserveShared());
        session.ReleasePlayerProcesses();
    }

    /// <summary>장치가 없으면 세션 Root 정리 갈고리를 만들 수 없다.</summary>
    [Fact]
    public void CleanupRootHooksWithoutDevicesThrows()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.BuildPathCleanupRootHooks());
        session.ReleasePlayerRecordViews();
    }

    /// <summary>남은 helper 분기는 실제 환불·0x6c 통지·가상 삭제를 원문 순서로 수행한다.</summary>
    [Fact]
    public void CleanupRootRemainingBranchRefundsNotifiesAndDestroysInOrder()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, ClientSurfaceGlobals surface, List<(uint Player, ConstructionMoneyNotice Notice)> money) = CleanupRootScene();
        session.Gameplay.Authority.Boss = true;
        var order = new List<string>();
        PathCleanupHooks root = session.BuildPathCleanupRootHooks();
        var anchor = new RawPlayerPlacementAnchor(session.Pool, types, surface.Storage.Hash.BorrowEntries(0), session.TerrainBookkeeping, new(), new(), new() { GraphReady = 1 });
        var cleanup = session.BuildPathCleanupWorldDevices(session.PriestWorld.Carrier, anchor, new PathCleanupState(),
            root with
            {
                ReserveShared = () => throw new InvalidOperationException("예약 분기 없음"),
                Ritual = (_, _, _) => throw new InvalidOperationException("의식 분기 없음"),
                Destroy = (sid, flags) => { order.Add($"destroy:{sid},{flags}"); root.Destroy!(sid, flags); },
            });
        // 남은 helper는 extra1·helper 부류를 갖추고 범위 밖에 둔다.
        types[100].Flags2 |= 0x404200u;
        types[100].Cost = 37.75f;
        session.Sp.Set(2, BitConverter.SingleToUInt32Bits(100f));
        session.MissionPlayer.BattleOwner = 2;
        session.MissionPlayer.PlayerKnown = 0;
        int parent = MakeRawSlot(session, 133, 50, 60, 2);
        int helper = MakeRawSlot(session, 100, 200, 200, 2, extra: 1, vtable: 0x0050C3B0u);
        RegisterInHash(surface.Storage.Hash, session.Pool, helper, 200, 200);
        var runtime = new RawPathRuntime(session.Pool.Edition, false, 0);
        runtime.SetCommandFields(unchecked((uint)helper), 0, 0, new(0, 0));
        cleanup.AfterSpatial(runtime, parent);
        Assert.Equal(137f, BitConverter.UInt32BitsToSingle(session.Sp.Get(2)));
        Assert.Equal([(2u, new ConstructionMoneyNotice { Player = 2, Source = 2, Kind = 1, Amount = 37 })], money.Select(item => (item.Player, item.Notice)).ToList());
        Assert.Equal([$"destroy:{helper},0"], order);
        Assert.Throws<InvalidOperationException>(() => session.Pool.AllocatedBytes(helper));
        session.ReleasePlayerProcesses();
    }

    /// <summary>너겟 분기는 같은 owner·Graph의 반환 장소에서 실제 내용물을 찾아 가상 삭제한다.</summary>
    [Fact]
    public void CleanupRootNuggetBranchDestroysContainedNugget()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, ClientSurfaceGlobals surface, _) = CleanupRootScene();
        session.Gameplay.Authority.Boss = true;
        var order = new List<string>();
        PathCleanupHooks root = session.BuildPathCleanupRootHooks();
        var anchor = new RawPlayerPlacementAnchor(session.Pool, types, surface.Storage.Hash.BorrowEntries(0), session.TerrainBookkeeping, new(), new(), new() { GraphReady = 1 });
        var cleanup = session.BuildPathCleanupWorldDevices(session.PriestWorld.Carrier, anchor, new PathCleanupState(),
            root with
            {
                ReserveShared = () => throw new InvalidOperationException("예약 분기 없음"),
                Ritual = (_, _, _) => throw new InvalidOperationException("의식 분기 없음"),
                Destroy = (sid, flags) => { order.Add($"destroy:{sid},{flags}"); root.Destroy!(sid, flags); },
            });
        types[133].Flags2 &= ~0x200u;
        types[133].Flags1 &= ~0x800u;
        types[128].Flags2 |= 0x200u;
        types[128].Flags1 &= ~0x800u;
        int parent = MakeRawSlot(session, 133, 50, 60, 1);
        int place = MakeRawSlot(session, 128, 51, 60, 1);
        RegisterInHash(surface.Storage.Hash, session.Pool, place, 51, 60);
        int nugget = new RawSquidFactory(session.Pool, types).CreateContained(2u);
        Span<byte> parentRaw = session.Pool.AllocatedBytes(parent);
        BinaryPrimitives.WriteUInt16LittleEndian(parentRaw.Slice(6, 2), unchecked((ushort)nugget));
        Span<byte> nuggetRaw = session.Pool.AllocatedBytes(nugget);
        BinaryPrimitives.WriteUInt16LittleEndian(nuggetRaw.Slice(18, 2), 154);
        BinaryPrimitives.WriteUInt16LittleEndian(nuggetRaw.Slice(20, 2), 0x20);
        var runtime = new RawPathRuntime(session.Pool.Edition, false, 0);
        cleanup.AfterSpatial(runtime, parent);
        Assert.Equal([$"destroy:{nugget},0"], order);
        Assert.Throws<InvalidOperationException>(() => session.Pool.AllocatedBytes(nugget));
        session.ReleasePlayerProcesses();
    }

    /// <summary>사제 부모의 의식 분기는 실제 세션 신전 의식을 같은 인자로 호출한다.</summary>
    [Fact]
    public void CleanupRootRitualBranchReachesRealTempleRitual()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, ClientSurfaceGlobals surface, _) = CleanupRootScene();
        session.Gameplay.Authority.Boss = true;
        var order = new List<string>();
        PathCleanupHooks root = session.BuildPathCleanupRootHooks();
        var anchor = new RawPlayerPlacementAnchor(session.Pool, types, surface.Storage.Hash.BorrowEntries(0), session.TerrainBookkeeping, new(), new(), new() { GraphReady = 1 });
        var cleanup = session.BuildPathCleanupWorldDevices(session.PriestWorld.Carrier, anchor, new PathCleanupState(),
            root with
            {
                ReserveShared = () => throw new InvalidOperationException("예약 분기 없음"),
                Ritual = (temple, parent, mode) => { order.Add($"ritual:{temple},{parent},{mode}"); root.Ritual!(temple, parent, mode); },
                Destroy = (_, _) => throw new InvalidOperationException("삭제 분기 없음"),
            });
        int priest = PlacePriest(session, types, 0, 9);
        int temple = MakeRawSlot(session, 165, 51, 60, 1);
        RegisterInHash(surface.Storage.Hash, session.Pool, temple, 51, 60);
        var runtime = new RawPathRuntime(session.Pool.Edition, false, 0);
        cleanup.AfterSpatial(runtime, priest);
        Assert.Equal([$"ritual:{temple},{priest},0"], order);
        session.ReleasePlayerProcesses();
    }

    /// <summary>helper 예약 분기는 실제 예약·공유 부착을 원문 순서로 수행하고 helper를 지운다.</summary>
    [Fact]
    public void CleanupRootReserveBranchUsesRealDevicesInOrder()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, ClientSurfaceGlobals surface, _) = CleanupRootScene();
        session.Gameplay.Authority.Boss = true;
        var order = new List<string>();
        PathCleanupHooks root = session.BuildPathCleanupRootHooks();
        var anchor = new RawPlayerPlacementAnchor(session.Pool, types, surface.Storage.Hash.BorrowEntries(0), session.TerrainBookkeeping, new(), new(), new() { GraphReady = 1 });
        var cleanup = session.BuildPathCleanupWorldDevices(session.PriestWorld.Carrier, anchor, new PathCleanupState(),
            root with
            {
                ReserveShared = () => { order.Add("reserve"); return root.ReserveShared!(); },
                Ritual = (_, _, _) => throw new InvalidOperationException("의식 분기 없음"),
                Destroy = (_, _) => throw new InvalidOperationException("삭제 분기 없음"),
            });
        // helper 부류 비트를 실제 타입에 둔다. 같은 세션 장면 안에서만 쓴다.
        types[97].Flags2 |= 0x404200u;
        int parent = MakeRawSlot(session, 133, 50, 60, 1);
        int helper = MakeRawSlot(session, 97, 51, 60, 1);
        RegisterInHash(surface.Storage.Hash, session.Pool, helper, 51, 60);
        var runtime = new RawPathRuntime(session.Pool.Edition, false, 0);
        runtime.SetCommandFields(unchecked((uint)helper), 0, 0, new(0, 0));
        cleanup.AfterSpatial(runtime, parent);
        Assert.Equal(["reserve"], order);
        Assert.Equal(0u, runtime.Helper);
        Assert.NotNull(session.PlayerProcesses.FindEvent(helper, 0x1a2c));
        session.ReleasePlayerProcesses();
    }
}
