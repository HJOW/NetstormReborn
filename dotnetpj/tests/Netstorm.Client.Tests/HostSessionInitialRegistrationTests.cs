using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 세션 시작 Kernel/ProcessHost 초기 등록을 검사하는 클래스다. 2026-11-23 0.894에서 R4-4 Kernel/Process 초기 등록용으로 추가했다.
// 근거: 세션 적재 시 등록되는 Regular 사슬의 시작 상태와 L2 초기 덤프 비교다. 초기 Kernel은 비어 있어야 한다.
public sealed class HostSessionInitialRegistrationTests
{
    /// <summary>실제 자료로 초기 등록 장면을 만든다. 지형·요새·운반체·지역·정기 사슬을 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildInitialScene()
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

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.894에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>시작 세션의 Kernel은 비어 있고 ProcessHost는 같은 풀·커널에 묶인다.</summary>
    [Fact]
    public void FreshSessionStartsWithEmptyKernelAndBoundProcesses()
    {
        (HostSession session, _) = BuildInitialScene();
        Assert.Equal(0, session.Kernel.Size());
        Assert.True(session.Kernel.Capacity() > 0);
        Assert.Same(session.Pool, session.PlayerProcesses.Pool());
        Assert.NotNull(session.PlayerProcesses.Regular());
    }

    /// <summary>어느 정기에도 맡지 않는 부모는 초기 처리기에서 payload 그대로 돌아온다.</summary>
    [Fact]
    public void UnhandledSidFallsThroughToPayload()
    {
        (HostSession session, _) = BuildInitialScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050da88u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 9, 0, 0.7f));
    }
}
