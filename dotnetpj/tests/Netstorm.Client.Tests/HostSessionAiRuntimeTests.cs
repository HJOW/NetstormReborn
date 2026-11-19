using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// AI 런타임 세션 결선(준비·전략·통지·등록·전체 프레임 장치와 PrepareAi·InitializeAi)을 검사하는 클래스다.
// 2026-11-19 0.650에서 R4-5 AI 런타임 결선용으로 추가했다.
// 근거: cpppj src/client/RawCampaignSession.cpp 461~462행의 PrepareAi·InitializeAi와 2831~2870행의 통지·등록 조립이다.
public sealed class HostSessionAiRuntimeTests
{
    /// <summary>실제 자료로 장면을 만든다. 적재·틱 없이 풀은 비워 둔다.</summary>
    /// <returns>세션이다. TYPE 표는 세션 장치와 같은 인스턴스다.</returns>
    private static HostSession BuildScene()
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
        return session;
    }

    /// <summary>값을 리틀 엔디언으로 바이트에 쓴다. 0.650에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>AI 본체를 세션 메모리에 확보하고 소유자에 등록한다. 0.650에서 추가했다.</summary>
    private static uint PrepareBrain(HostSession session, uint owner)
    {
        uint brain = session.AiBrainMemory.Allocate(RawAiBrainConstructor.BytesCount);
        session.AiBrains[owner] = brain;
        Put(session.AiBrainMemory.Bytes(brain), 0x260, owner);
        return brain;
    }

    /// <summary>런타임 장치가 모두 연결된다.</summary>
    [Fact]
    public void AiRuntimeDevicesAreBuilt()
    {
        HostSession session = BuildScene();
        Assert.NotNull(session.AiPreparation);
        Assert.NotNull(session.AiStrategySetup);
        Assert.NotNull(session.AiObjects);
        Assert.NotNull(session.AiWorld);
        Assert.NotNull(session.AiRuntime);
    }

    /// <summary>brain이 없으면 준비·초기는 거짓으로 돌아간다.</summary>
    [Fact]
    public void PrepareAndInitializeWithoutBrainReturnFalse()
    {
        HostSession session = BuildScene();
        Assert.False(session.PrepareAi(2));
        Assert.False(session.InitializeAi(2));
        Assert.Empty(session.AiRuntimeDiagnostics);
    }

    /// <summary>적·지역이 없으면 준비는 진단·초기 스캔까지만 진행한다.</summary>
    [Fact]
    public void PrepareAiRunsUpdateAndScan()
    {
        HostSession session = BuildScene();
        uint brain = PrepareBrain(session, 1);
        // 소유자·0번 기록의 지역을 없음으로 두어 지역 경계 조회를 피한다.
        Put(session.PlayerRecords, 1 * 180 + 0x78, 127u);
        Put(session.PlayerRecords, 0 * 180 + 0x78, 127u);
        Assert.True(session.PrepareAi(1));
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x25c, 4)));
        Assert.Contains(session.AiRuntimeDiagnostics, line => line.Contains("enemyPlayerId"));
    }

    /// <summary>양쪽 지역이 없으면 초기는 258을 쓰지 않고 거짓으로 돌아간다.</summary>
    [Fact]
    public void InitializeAiSkipsWithoutRegions()
    {
        HostSession session = BuildScene();
        uint brain = PrepareBrain(session, 1);
        Put(session.PlayerRecords, 1 * 180 + 0x78, 127u);
        Assert.False(session.InitializeAi(1));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x258, 4)));
    }
}
