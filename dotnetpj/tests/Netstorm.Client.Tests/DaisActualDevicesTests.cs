using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// R4-4 Dais 실제 장치(로컬 SP·0x307 부착)의 세션 결선을 검증한다. 0.996에서 추가됐다.
public sealed class DaisActualDevicesTests
{
    /// <summary>실제 자료로 Dais 장면을 만든다. 지형·요새·운반체·지역·Dais를 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildDaisScene()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        session.BuildIslandPostPopDevices(table.Types, session.WorldFrames);
        session.BuildPlayerWorldHooks();
        session.BuildTerrainBinding(table.Types, session.TerrainDiagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        session.BuildRegionOwnershipDevices(table.Types, () => 1u);
        return (session, table.Types);
    }

    /// <summary>Dais 부모 슬롯을 할당하고 가상표·타입을 채운다.</summary>
    /// <param name="session">Dais 장치가 연결된 세션이다.</param>
    /// <param name="vtable">쓸 가상표 값이다. Dais는 0x507950, 제단은 0x5076b8이다.</param>
    /// <returns>채운 부모 번호다.</returns>
    private static int PrepareSid(HostSession session, uint vtable)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        BinaryPrimitives.WriteUInt32LittleEndian(raw, vtable);
        raw[RawSidPool.TypeOffset] = 165;
        return sid;
    }

    /// <summary>세션 체인 설치 뒤 Dais 정기 분배가 중복 연결을 거부한다.</summary>
    [Fact]
    public void DaisInstallGuardsDuplicateBinding()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildDaisScene();
        var error = Assert.Throws<InvalidOperationException>(() => session.BuildDaisRegularDevices(types));
        Assert.Contains("중복 연결", error.Message);
    }

    /// <summary>로컬 SP 가산 본체는 같은 SP 저장소의 owner 값에 delta를 더한다.</summary>
    [Fact]
    public void AddLocalSpWritesActualRewardStore()
    {
        var pool = new RawSidPool(24000);
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var store = new RawSpStore(new GameRandom(1u), () => 1u);
        var reward = new RawSquidReward(pool, types, new RawPostPopState(), new SquidRewardState(), store);
        reward.AddSp(1, 5000f);
        Assert.Equal(BitConverter.SingleToUInt32Bits(5000f), store.Get(1));
        reward.AddSp(1, -500f);
        Assert.Equal(BitConverter.SingleToUInt32Bits(4500f), store.Get(1));
    }

    /// <summary>0x307 공유 부착은 실제 Host에 사건을 등록하고 조회에서 보인다.</summary>
    [Fact]
    public void SharedRegularAttachRegistersActualHostEvent()
    {
        (HostSession session, _) = BuildDaisScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Assert.Null(session.PlayerProcesses.FindEvent(sid, 0x307));
        session.PlayerProcesses.AddSharedRegular(sid, 0x307, 1.5f);
        Assert.NotNull(session.PlayerProcesses.FindEvent(sid, 0x307));
    }

    /// <summary>개수 0인 0x307은 손상 분배로 넘어가 이전 처리기가 payload를 돌려준다.</summary>
    [Fact]
    public void Dais307WithoutCountFallsBackToPreviousHandler()
    {
        (HostSession session, _) = BuildDaisScene();
        int sid = PrepareSid(session, 0x00507950u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 0x307, 0, 0.7f));
    }

    /// <summary>제단 가상표는 세션 합성이 오기 전까지 이전 처리기에 payload를 돌려준다.</summary>
    [Fact]
    public void AltarVtableFallsBackPendingSessionComposition()
    {
        (HostSession session, _) = BuildDaisScene();
        int sid = PrepareSid(session, 0x005076b8u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 20, 0, 0.7f));
    }
}
