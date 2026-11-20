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
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildScene()
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
        (HostSession session, _) = BuildScene();
        Assert.NotNull(session.AiPreparation);
        Assert.NotNull(session.AiStrategySetup);
        Assert.NotNull(session.AiObjects);
        Assert.NotNull(session.AiWorld);
        Assert.NotNull(session.AiRuntime);
        Assert.Same(session.Sp, session.AiEconomy.Store);
    }

    /// <summary>brain이 없으면 준비·초기는 거짓으로 돌아간다.</summary>
    [Fact]
    public void PrepareAndInitializeWithoutBrainReturnFalse()
    {
        (HostSession session, _) = BuildScene();
        Assert.False(session.PrepareAi(2));
        Assert.False(session.InitializeAi(2));
        Assert.Empty(session.AiRuntimeDiagnostics);
    }

    /// <summary>brain이 없으면 SP 초기화·난수 소비 없이 돌아가고 범위 밖 소유자는 거부한다.</summary>
    [Fact]
    public void EconomyWithoutBrainPreservesRandomAndRejectsInvalidOwner()
    {
        (HostSession session, _) = BuildScene();
        uint before = session.Game.State();
        Assert.False(session.ReconcileAiEconomy(2));
        Assert.Equal(before, session.Game.State());
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReconcileAiEconomy(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReconcileAiEconomy(9));
    }

    /// <summary>매초 지원금을 같은 brain 지갑과 실제 SP에 쓰며 같은 시각에는 중복 지원하지 않는다.</summary>
    [Fact]
    public void EconomyUpdatesSessionWalletAndSpOncePerDeadline()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 1);
        Put(session.AiBrainMemory.Bytes(brain), 0x264, 2);
        Put(session.AiBrainMemory.Bytes(brain), 0x290, 100);
        Put(session.AiBrainMemory.Bytes(brain), 0x24c, 500);
        session.ProcessTime.Value = 10;
        session.BindSpReseed(() => 0x12345678u);
        Assert.True(session.ReconcileAiEconomy(1));
        Assert.Equal(BitConverter.SingleToUInt32Bits(500f), session.Sp.Get(1));
        Assert.True(session.ReconcileAiEconomy(1));
        Assert.Equal(BitConverter.SingleToUInt32Bits(500f), session.Sp.Get(1));
        session.ProcessTime.Value = 11;
        Assert.True(session.ReconcileAiEconomy(1));
        Assert.Equal(BitConverter.SingleToUInt32Bits(1000f), session.Sp.Get(1));
    }

    /// <summary>참여 마스크와 현재 공유 상태의 조합을 판정한다. 포획 행동 재현 없이 메모리 입력만 대조한다.</summary>
    [Theory]
    [InlineData(0x22, 1u, 2300f)]
    [InlineData(0x02, 1u, 2300f)]
    [InlineData(0x20, 1u, 2300f)]
    [InlineData(0x00, 1u, 500f)]
    [InlineData(0x22, 2u, 500f)]
    [InlineData(0x22, 0u, 500f)]
    public void EconomyReadsCurrentParticipationAndSharedPriestState(int flags, uint state, float expected)
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 1);
        Put(session.AiBrainMemory.Bytes(brain), 0x264, 2);
        Put(session.AiBrainMemory.Bytes(brain), 0x290, 100);
        Put(session.AiBrainMemory.Bytes(brain), 0x24c, 500);
        session.PlayerRecords[2 * 180 + 0x50] = (byte)flags;
        var shared = new uint[9];
        shared[2] = state;
        session.PriestState.Priests.SharedOwnerState = shared;
        session.BindSpReseed(() => 0x12345678u);
        Assert.True(session.ReconcileAiEconomy(1));
        Assert.Equal(BitConverter.SingleToUInt32Bits(expected), session.Sp.Get(1));
        // 같은 시각에 공유 상태를 바꿔도 새 getter는 즉시 변경값을 읽는다.
        shared[2] = 0;
        Put(session.AiBrainMemory.Bytes(brain), 0x2d8, 7);
        Assert.True(session.ReconcileAiEconomy(1));
        Assert.Equal(BitConverter.SingleToUInt32Bits(7f), session.Sp.Get(1));
    }

    /// <summary>조립 뒤 바인딩한 현재 데모 값을 다시 읽고 지원 때만 하한을 적용한다.</summary>
    [Fact]
    public void EconomyUsesLiveDemoGetter()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 1);
        Put(session.AiBrainMemory.Bytes(brain), 0x264, 2);
        Put(session.AiBrainMemory.Bytes(brain), 0x24c, 1);
        session.BindSpReseed(() => 0x12345678u);
        uint demo = 0;
        session.BindAiDemo(() => demo);
        Assert.True(session.ReconcileAiEconomy(1));
        Assert.Equal(BitConverter.SingleToUInt32Bits(1f), session.Sp.Get(1));
        demo = 3;
        session.ProcessTime.Value = 1;
        Assert.True(session.ReconcileAiEconomy(1));
        Assert.Equal(BitConverter.SingleToUInt32Bits(1190f), session.Sp.Get(1));
        Assert.Throws<ArgumentNullException>(() => session.BindAiDemo(null!));
    }

    /// <summary>적·지역이 없으면 준비는 진단·초기 스캔까지만 진행한다.</summary>
    [Fact]
    public void PrepareAiRunsUpdateAndScan()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 1);
        // 소유자·0번 기록의 지역을 없음으로 두어 지역 경계 조회를 피한다.
        Put(session.PlayerRecords, 1 * 180 + 0x78, 127u);
        Put(session.PlayerRecords, 0 * 180 + 0x78, 127u);
        Assert.True(session.PrepareAi(1));
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x25c, 4)));
        Assert.Contains(session.AiRuntimeDiagnostics, line => line.Contains("enemyPlayerId"));
    }

    /// <summary>지갑이 상하면 실제 SP 저장소와의 차이를 가산한다.</summary>
    [Fact]
    public void PrepareAiAddsWalletDelta()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 1);
        Put(session.PlayerRecords, 1 * 180 + 0x78, 127u);
        Put(session.PlayerRecords, 0 * 180 + 0x78, 127u);
        Put(session.AiBrainMemory.Bytes(brain), 0x260, 1u);
        Put(session.AiBrainMemory.Bytes(brain), 0x264, 2u);
        Put(session.AiBrainMemory.Bytes(brain), 0x2d8, 30000u);
        session.MissionPlayer.PlayerKnown = 1u;
        session.BindSpReseed(() => 0x12345678u);
        session.Sp.Set(1, BitConverter.SingleToUInt32Bits(5000.0f));
        Assert.True(session.PrepareAi(1));
        Assert.Equal(
            BitConverter.SingleToUInt32Bits(30000.0f),
            session.Sp.Get(1));
    }

    /// <summary>유효 특정 좌표이면 목표 4를 고른다. 지도 상한은 독립 기본값을 쓴다.</summary>
    [Fact]
    public void SelectGoalUsesMapLimit()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 1);
        Put(session.AiBrainMemory.Bytes(brain), 0x2d0, 0u);
        Put(session.AiBrainMemory.Bytes(brain), 0x2d8, 0u);
        Put(session.AiBrainMemory.Bytes(brain), 0x290, 100000u);
        Put(session.AiBrainMemory.Bytes(brain), 0x248, 0u);
        Put(session.AiBrainMemory.Bytes(brain), 0xaf0, BitConverter.SingleToUInt32Bits(50.0f));
        Put(session.AiBrainMemory.Bytes(brain), 0xaf4, BitConverter.SingleToUInt32Bits(60.0f));
        Put(session.AiBrainMemory.Bytes(brain), 0x2d4, 9u);
        session.AiStrategySetup.SelectGoal(brain);
        Assert.Equal(4u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x2d4, 4)));
    }

    /// <summary>신전이 있으면 그래프 번호를 읽는다. 표면 타입은 자신의 raw 번호를 쓴다.</summary>
    [Fact]
    public void PrepareAiReadsTempleGraph()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildScene();
        types[40].Flags1 |= 0x800u;
        int temple = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(temple).Fill(0);
        session.Pool.AllocatedBytes(temple)[10] = 40;
        session.Pool.AllocatedBytes(temple)[34] = 0;
        session.Pool.AllocatedBytes(temple)[30] = 5;
        for (int i = 0; i < 4; i++)
        {
            session.AiIslands.Add(1, unchecked((byte)i));
        }

        session.AiIslands.Temple(3, unchecked((uint)temple));
        Put(session.PlayerRecords, 1 * 180 + 0x78, 3u);
        Put(session.PlayerRecords, 2 * 180 + 0x78, 127u);
        Put(session.PlayerRecords, 0 * 180 + 0x78, 127u);
        uint brain = PrepareBrain(session, 1);
        Put(session.AiBrainMemory.Bytes(brain), 0x260, 1u);
        Put(session.AiBrainMemory.Bytes(brain), 0x264, 2u);
        Assert.True(session.PrepareAi(1));
        Assert.Equal(5u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x298, 4)));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x2a8, 4)));
    }

    /// <summary>신전이 있으면 세션 신전 생산물을 만들고 theme을 기록한다. 생산 TYPE은 실제 GameAux 상자에서 읽는다.</summary>
    [Fact]
    public void InitializeAiAddsSessionTempleProduct()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildScene();
        int technology = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(technology).Fill(0);
        int temple = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(temple).Fill(0);
        session.Pool.AllocatedBytes(temple)[10] = 40;
        types[40].Flags1 |= 0x800u;
        types[40].Theme = 7;
        session.GameAux.TempleProductTypeBox.Value = 60u;
        for (int i = 0; i < 4; i++)
        {
            session.AiIslands.Add(1, unchecked((byte)i));
        }

        session.AiIslands.Temple(3, unchecked((uint)temple));
        Put(session.PlayerRecords, 1 * 180 + 0x78, 3u);
        Put(session.PlayerRecords, 2 * 180 + 0x78, 4u);
        Put(session.PlayerRecords, 0 * 180 + 0x78, 127u);
        Put(session.PlayerRecords, 1 * 180 + 0x9c, unchecked((uint)technology));
        session.MissionPlayer.PlayerKnown = 1u;
        uint brain = PrepareBrain(session, 1);
        Put(session.AiBrainMemory.Bytes(brain), 0x264, 2u);
        Assert.True(session.InitializeAi(1));
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x258, 4)));
        Assert.Equal(7u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x390, 4)));
    }

    /// <summary>양쪽 지역이 없으면 초기는 258을 쓰지 않고 거짓으로 돌아간다.</summary>
    [Fact]
    public void InitializeAiSkipsWithoutRegions()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 1);
        Put(session.PlayerRecords, 1 * 180 + 0x78, 127u);
        Assert.False(session.InitializeAi(1));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x258, 4)));
    }

    /// <summary>지역이 있으면 준비는 적 경계 중심까지 진행한다. 섬 경계는 실제 섬 런타임에서 읽는다.</summary>
    [Fact]
    public void PrepareAiWritesEnemyBoundsCenter()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 1);
        Put(session.PlayerRecords, 1 * 180 + 0x78, 3u);
        Put(session.PlayerRecords, 2 * 180 + 0x78, 4u);
        Put(session.AiBrainMemory.Bytes(brain), 0x260, 1u);
        Put(session.AiBrainMemory.Bytes(brain), 0x264, 2u);
        // 섬 기록 0~4를 두어 4번 경계를 쓴다.
        for (int i = 0; i < 5; i++)
        {
            session.AiIslands.Add(1, unchecked((byte)i));
        }

        session.AiIslands.Bounds(4, new FortRandomRectangle(0, 0, 100, 50));
        Assert.True(session.PrepareAi(1));
        Assert.Equal(
            BitConverter.SingleToUInt32Bits(50.0f),
            BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x2b0, 4)));
    }

    /// <summary>지역이 있으면 초기는 자기 영역·첫 재고·258을 기록한다.</summary>
    [Fact]
    public void InitializeAiWritesHomeBoundsAndFlag()
    {
        (HostSession session, _) = BuildScene();
        int technology = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(technology).Fill(0);
        Put(session.PlayerRecords, 1 * 180 + 0x78, 3u);
        Put(session.PlayerRecords, 2 * 180 + 0x78, 4u);
        Put(session.PlayerRecords, 1 * 180 + 0x9c, unchecked((uint)technology));
        session.MissionPlayer.PlayerKnown = 1u;
        // 섬 기록 0~3을 두어 3번 경계를 쓴다.
        for (int i = 0; i < 4; i++)
        {
            session.AiIslands.Add(1, unchecked((byte)i));
        }

        session.AiIslands.Bounds(3, new FortRandomRectangle(0, 0, 100, 50));
        uint brain = PrepareBrain(session, 1);
        Put(session.AiBrainMemory.Bytes(brain), 0x264, 2u);
        Assert.True(session.InitializeAi(1));
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x258, 4)));
        Assert.Equal(
            BitConverter.SingleToUInt32Bits(50.0f),
            BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x268, 4)));
    }
}
