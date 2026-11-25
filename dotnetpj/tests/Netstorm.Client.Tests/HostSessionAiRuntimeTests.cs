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
        Assert.NotNull(session.AiCollectors);
        Assert.NotNull(session.AiDrySpots);
        Assert.NotNull(session.AiCarriers);
        Assert.NotNull(session.AiPriests);
        Assert.Same(session.Sp, session.AiEconomy.Store);
        Assert.Same(session.Pool, session.AiFactories.Pool);
    }

    /// <summary>실제 풀의 명시 0 슬롯을 준비한다. 게임 생성 효과 대신 검사 입력을 만드는 함수다.</summary>
    private static int EmptySlot(HostSession session, byte type)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(sid).Clear();
        session.Pool.AllocatedBytes(sid)[10] = type;
        return sid;
    }

    /// <summary>현재 Player 기술·known 내용물·시설 큐를 같은 메모리에 준비한다.</summary>
    private static (uint Brain, int Technology, int Factory) PrepareFactoryBrain(HostSession session, IReadOnlyList<OriginalTypeRecord> types, uint owner)
    {
        uint brain = PrepareBrain(session, owner);
        int technology = EmptySlot(session, 0);
        int factory = EmptySlot(session, 85);
        // 실제 연결기가 방문할 기존 기술 조각은 원본 생성자로 가상표·링크 배치를 준비한다.
        int known = new RawSquidFactory(session.Pool, types).CreateContained(2u);
        Put(session.Pool.AllocatedBytes(factory), 0, 0x509148);
        BinaryPrimitives.WriteUInt16LittleEndian(session.Pool.AllocatedBytes(known).Slice(18, 2), 100);
        BinaryPrimitives.WriteUInt16LittleEndian(session.Pool.AllocatedBytes(known).Slice(20, 2), 4);
        BinaryPrimitives.WriteUInt16LittleEndian(session.Pool.AllocatedBytes(technology).Slice(6, 2), (ushort)known);
        types[100].Theme = types[85].Theme;
        types[100].Flags2 &= ~0x800u;
        session.MissionPlayer.PlayerKnown = 1;
        session.MissionPlayer.BattleOwner = 1;
        Put(session.PlayerRecords, (int)owner * 180 + 0x9c, (uint)technology);
        byte[] body = session.AiBrainMemory.Bytes(brain);
        Put(body, 0x294, (uint)factory);
        Put(body, 0x298, 7);
        Put(body, 0x2cc, 1);
        Put(body, 0x2d8, 1200);
        Put(body, 0x228, BitConverter.SingleToUInt32Bits(1.25f));
        uint pending = session.AiBrainMemory.Allocate(16);
        Put(body, 0x30c, pending);
        Put(body, 0x310, 4);
        Put(body, 0x314, 1);
        Put(session.AiBrainMemory.Bytes(pending), 0, (uint)factory);
        Put(body, 0x318, session.AiBrainMemory.Allocate(16));
        Put(body, 0x31c, 4);
        session.ProcessTime.Value = 10;
        return (brain, technology, factory);
    }

    /// <summary>brain 없는 시설 관리는 실제 난수·풀을 바꾸지 않고 범위 밖 owner를 거부한다.</summary>
    [Fact]
    public void FactoriesWithoutBrainPreserveRandomAndRejectInvalidOwner()
    {
        (HostSession session, _) = BuildScene();
        uint before = session.Game.State();
        Assert.False(session.ManageAiFactories(2));
        Assert.Equal(before, session.Game.State());
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ManageAiFactories(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ManageAiFactories(9));
    }

    /// <summary>실제 전략·내용물 생성기로 생산을 넣고 비로컬 AI일 때 기술 부모에도 teleport 항목을 만든다.</summary>
    [Theory]
    [InlineData(1u, 0)]
    [InlineData(2u, 1)]
    public void FactoriesRegisterActualProductsAndDeadline(uint owner, int teleports)
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildScene();
        (uint brain, int technology, int factory) = PrepareFactoryBrain(session, types, owner);
        Put(session.AiBrainMemory.Bytes(brain), 0xb00, 1);
        Assert.True(session.ManageAiFactories(owner));
        Assert.Equal(1, session.AiFactories.CountProducts(factory));
        Assert.Equal(teleports, RawContainedFinder.CountContained(session.Pool, new ContainedFinderState(), technology, 8, 100));
        byte[] body = session.AiBrainMemory.Bytes(brain);
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(body.AsSpan(0x314, 4)));
        Assert.Equal(11.25, BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(body.AsSpan(0x5a8, 8))));
        Assert.Contains(session.AiRuntimeLogs, text => text.Contains(":factory:AddedProduct:100"));
        // 같은 시각 재호출은 생각 기한을 통과하지 못하므로 실제 생산을 중복하지 않는다.
        Assert.True(session.ManageAiFactories(owner));
        Assert.Equal(1, session.AiFactories.CountProducts(factory));
    }

    /// <summary>실제 시설 아닌 가상표를 level80으로 해석하지 않으며 큐·생산을 보존한다.</summary>
    [Fact]
    public void FactoriesRejectUnsupportedVirtualLevelBeforeAdding()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildScene();
        (uint brain, _, int factory) = PrepareFactoryBrain(session, types, 2);
        Put(session.Pool.AllocatedBytes(factory), 0, 0x12345678);
        Assert.Throws<InvalidOperationException>(() => session.ManageAiFactories(2));
        Assert.Equal(0, session.AiFactories.CountProducts(factory));
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x314, 4)));
    }

    /// <summary>업그레이드 실제 요청 미연결은 명시 오류이며 대기 큐·기한을 소비하지 않는다.</summary>
    [Fact]
    public void FactoriesSelectedUpgradeKeepsQueueUntilRealCommandIsConnected()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildScene();
        (uint brain, _, int factory) = PrepareFactoryBrain(session, types, 2);
        byte[] body = session.AiBrainMemory.Bytes(brain);
        Put(body, 0x314, 0);
        Put(body, 0x320, 1);
        uint queue = BinaryPrimitives.ReadUInt32LittleEndian(body.AsSpan(0x318, 4));
        Put(session.AiBrainMemory.Bytes(queue), 0, (uint)factory);
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.ManageAiFactories(2));
        Assert.Contains("업그레이드 요청 미연결", error.Message);
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(body.AsSpan(0x320, 4)));
        Assert.Equal(0ul, BinaryPrimitives.ReadUInt64LittleEndian(body.AsSpan(0x5a8, 8)));
    }

    /// <summary>brain 없는 수집자 관리는 실제 난수를 소비하지 않고 범위 밖 owner를 거부한다.</summary>
    [Fact]
    public void CollectorsWithoutBrainPreserveRandomAndRejectInvalidOwner()
    {
        (HostSession session, _) = BuildScene();
        uint before = session.Game.State();
        Assert.False(session.ManageAiCollectors(2));
        Assert.Equal(before, session.Game.State());
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ManageAiCollectors(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ManageAiCollectors(9));
    }

    /// <summary>신전이 없으면 수집자 본체는 어떤 하위도 호출하지 않고 몸체를 바꾸지 않는다.</summary>
    [Fact]
    public void CollectorsGateClosedSkipsAllHooks()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        uint before = session.Game.State();
        Assert.True(session.ManageAiCollectors(2));
        Assert.Equal(before, session.Game.State());
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x5a8, 8)));
    }

    /// <summary>미연결 참여 상태에 닿으면 원문 순서대로 명시 거부하고 그 앞 효과만 둔다.</summary>
    [Fact]
    public void CollectorsMissingLeafThrowsExplicitly()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        int temple = session.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        session.Pool.AllocatedBytes(temple).Clear();
        int enemy = session.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        session.Pool.AllocatedBytes(enemy).Clear();
        Put(session.AiBrainMemory.Bytes(brain), 0x294, unchecked((uint)temple));
        Put(session.AiBrainMemory.Bytes(brain), 0x298, 7u);
        Put(session.AiBrainMemory.Bytes(brain), 0x248, 0x100u);
        Put(session.AiBrainMemory.Bytes(brain), 0x38c, 0u);
        Put(session.AiBrainMemory.Bytes(brain), 0x388, 1u);
        Put(session.AiBrainMemory.Bytes(brain), 0x28c, 1u);
        Put(session.AiBrainMemory.Bytes(brain), 0x2a0, unchecked((uint)enemy));
        Put(session.AiBrainMemory.Bytes(brain), 0x5a8, 0u);
        Put(session.AiBrainMemory.Bytes(brain), 0x5a4, 0u);
        session.ProcessTime.Value = 10;
        session.Game.SetState(1);
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.ManageAiCollectors(2));
        Assert.Contains("미연결(R6)", error.Message);
        // 생각 기한·난수·깃발·개수·적 SID까지 원문 순서대로 진행하고 난수는 한 번만 소비한다.
        Assert.Equal(65542u, session.Game.State());
    }

    /// <summary>brain이 없으면 부족 좌표 스캔은 0이고 범위 밖 owner는 거부한다.</summary>
    [Fact]
    public void DrySpotsWithoutBrainReturnZeroAndRejectInvalidOwner()
    {
        (HostSession session, _) = BuildScene();
        Assert.Equal(0u, session.RecheckAiDrySpots(2));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.RecheckAiDrySpots(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.RecheckAiDrySpots(9));
    }

    /// <summary>빈 부족 좌표 목록은 스캔 하위를 호출하지 않고 0을 돌려준다.</summary>
    [Fact]
    public void DrySpotsEmptyListScansNothing()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        Assert.Equal(0u, session.RecheckAiDrySpots(2));
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
    }

    /// <summary>미연결 에너지 승인에 닿으면 원문 순서대로 명시 거부한다.</summary>
    [Fact]
    public void DrySpotsMissingEnergyThrowsExplicit()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        uint backing = session.AiBrainMemory.Allocate(12);
        Put(session.AiBrainMemory.Bytes(brain), 0xae4, backing);
        Put(session.AiBrainMemory.Bytes(brain), 0xae0, 1u);
        Put(session.AiBrainMemory.Bytes(brain), 0xae8, 0u);
        Put(session.AiBrainMemory.Bytes(brain), 0xaec, 2u);
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.RecheckAiDrySpots(2));
        Assert.Contains("미연결(R6)", error.Message);
    }

    /// <summary>b00이 꺼진 부족 좌표 통지는 로그를 남기지 않고 켜지면 종류·주인·가짜 여부를 남긴다.</summary>
    [Fact]
    public void DryNoticeGateAndFormat()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        Put(session.AiBrainMemory.Bytes(brain), 0x260, 2u);
        Put(session.AiBrainMemory.Bytes(brain), 0xb00, 0u);
        session.LogAiDrySpotNote(brain, 99u, false);
        Assert.Equal(["99:AI2: Killed Dry Spot"], session.AiRuntimeLogs);
        session.LogAiDrySpotNote(brain, 99u, true);
        Assert.Equal(["99:AI2: Killed Dry Spot", "99:AI2: Killed Fake Dry Spot"], session.AiRuntimeLogs);
    }

    /// <summary>brain 목록 기록을 실제 backing에 쓴다. 운반 호출자용으로 추가했다.</summary>
    private static void WriteBrainList(HostSession session, uint brain, int at, uint[] values, uint capacity = 8)
    {
        uint pointer = session.AiBrainMemory.Allocate((int)capacity * 4);
        // 명시 입력 순서만 기록하는 반복문이다.
        for (int index = 0; index < values.Length; index++)
        {
            Put(session.AiBrainMemory.Bytes(pointer), index * 4, values[index]);
        }

        Put(session.AiBrainMemory.Bytes(brain), at, pointer);
        Put(session.AiBrainMemory.Bytes(brain), at + 4, capacity);
        Put(session.AiBrainMemory.Bytes(brain), at + 8, (uint)values.Length);
    }

    /// <summary>brain이 없으면 운반 행동은 거짓으로 돌아가고 범위 밖 owner를 거부한다.</summary>
    [Fact]
    public void CarriersWithoutBrainPreserveRandomAndRejectInvalidOwner()
    {
        (HostSession session, _) = BuildScene();
        uint before = session.Game.State();
        Assert.False(session.ManageAiCarriers(2));
        Assert.Equal(before, session.Game.State());
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ManageAiCarriers(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ManageAiCarriers(9));
    }

    /// <summary>빈 운반 목록은 어떤 하위도 호출하지 않고 몸체를 바꾸지 않는다.</summary>
    [Fact]
    public void CarriersEmptyListsDoNothing()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        uint before = session.Game.State();
        Assert.True(session.ManageAiCarriers(2));
        Assert.Equal(before, session.Game.State());
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(brain).AsSpan(0x6d0, 4)));
    }

    /// <summary>정지 운반자에 닿으면 원문 순서대로 경로 조회를 명시 거부하고 그 앞 효과만 둔다.</summary>
    [Fact]
    public void CarriersMissingPathThrowsExplicitly()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        int carrier = session.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        session.Pool.AllocatedBytes(carrier).Clear();
        session.Pool.AllocatedBytes(carrier)[10] = 1;
        types[1].Flags2 &= ~0x200000u;
        WriteBrainList(session, brain, 0x2dc, [unchecked((uint)carrier)]);
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.ManageAiCarriers(2));
        Assert.Contains("미연결(R6)", error.Message);
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
    }

    /// <summary>brain이 없으면 사제 행동은 거짓으로 돌아가고 범위 밖 owner를 거부한다.</summary>
    [Fact]
    public void PriestsWithoutBrainPreserveRandomAndRejectInvalidOwner()
    {
        (HostSession session, _) = BuildScene();
        uint before = session.Game.State();
        Assert.False(session.ManageAiPriest(2, 7));
        Assert.Equal(before, session.Game.State());
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ManageAiPriest(0, 7));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ManageAiPriest(9, 7));
    }

    /// <summary>빈손 사제는 어떤 하위도 호출하지 않고 몸체를 바꾸지 않는다.</summary>
    [Fact]
    public void PriestsEmptyHandsDoNothing()
    {
        (HostSession session, _) = BuildScene();
        PrepareBrain(session, 2);
        uint before = session.Game.State();
        Assert.True(session.ManageAiPriest(2, 0));
        Assert.Equal(before, session.Game.State());
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
    }

    /// <summary>기한 앞섬에 닿으면 원문 순서대로 탐색을 명시 거부한다.</summary>
    [Fact]
    public void PriestsMissingScanThrowsExplicitly()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        Put(session.AiBrainMemory.Bytes(brain), 0x5a4, 0u);
        Put(session.AiBrainMemory.Bytes(brain), 0x5a0, 1u);
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.ManageAiPriest(2, 0));
        Assert.Contains("미연결(R6)", error.Message);
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
    }

    /// <summary>살아 있는 사제에 닿으면 실제 HP·예약 판정을 지나 경로 조회를 명시 거부한다.</summary>
    [Fact]
    public void PriestsMissingPathThrowsExplicitly()
    {
        (HostSession session, _) = BuildScene();
        PrepareBrain(session, 2);
        int priest = session.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        session.Pool.AllocatedBytes(priest).Clear();
        session.Pool.AllocatedBytes(priest)[10] = 1;
        Put(session.Pool.AllocatedBytes(priest), 26, 0x7FFFFFFFu);
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.ManageAiPriest(2, unchecked((ushort)priest)));
        Assert.Contains("미연결(R6)", error.Message);
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
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

    /// <summary>AI 장치가 없으면 명시 프레임 호출은 거부하고 정지 중에는 검사하지 않는다.</summary>
    /// <remarks>2026-11-25 1.031에서 R4-5e1 프레임 부착용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 4569행의 if (!paused) aiRuntime->Frame()과 설치 전 throw다.</remarks>
    [Fact]
    public void AiFrameWithoutDeviceThrowsOnlyWhenRunning()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.RunAiFrame(false));
        session.RunAiFrame(true);
    }

    /// <summary>brain이 없으면 명시 프레임 호출은 실제 난수를 소비하지 않는다.</summary>
    /// <remarks>2026-11-25 1.031에서 R4-5e1 프레임 부착용으로 추가했다. 빈 Player 기록은 Frame의 owner1..8 방문에서 모두 건너뛴다.</remarks>
    [Fact]
    public void AiFrameWithoutBrainPreservesRandom()
    {
        (HostSession session, _) = BuildScene();
        uint before = session.Game.State();
        session.RunAiFrame(false);
        Assert.Equal(before, session.Game.State());
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
    }

    /// <summary>정지 중에는 brain이 있어도 명시 프레임 호출을 건너뛴다.</summary>
    /// <remarks>2026-11-25 1.031에서 R4-5e1 프레임 부착용으로 추가했다. 원문 PrepareFrame의 paused 분기와 같다.</remarks>
    [Fact]
    public void AiFramePausedSkipsEvenWithBrain()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        Put(session.PlayerRecords, 2 * 180 + 0xb0, brain);
        Put(session.AiBrainMemory.Bytes(brain), 0x258, 1u);
        uint before = session.Game.State();
        session.RunAiFrame(true);
        Assert.Equal(before, session.Game.State());
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
    }

    /// <summary>초기화 완료 brain은 프레임에서 준비·목표·수집자·시설·부족·사제를 지나 배치에서 명시 실패한다.</summary>
    /// <remarks>2026-11-25 1.031에서 R4-5e1 프레임 부착용으로 추가했다. Drops·Bridges 미연결은 R6에서 잇는다.</remarks>
    [Fact]
    public void AiFrameWithReadyBrainReachesDropsExplicitly()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        Put(session.PlayerRecords, 2 * 180 + 0xb0, brain);
        Put(session.PlayerRecords, 0 * 180 + 0x78, 127u);
        Put(session.PlayerRecords, 2 * 180 + 0x78, 127u);
        Put(session.AiBrainMemory.Bytes(brain), 0x258, 1u);
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.RunAiFrame(false));
        Assert.Contains("AI 배치 행동 미연결(R6)", error.Message);
    }

    /// <summary>미션 프레임 실행은 AI 미설치 세션에서도 커널을 실행한다.</summary>
    /// <remarks>2026-11-25 1.031에서 R4-5e1 프레임 부착용으로 추가했다. 메뉴·검사 조합의 기존 호출을 보존한다.</remarks>
    [Fact]
    public void MissionFrameWithoutAiDeviceStillRunsKernel()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        uint before = session.Game.State();
        session.RunMissionFrame();
        Assert.Equal(before, session.Game.State());
        Assert.Equal(0, session.Kernel.Size());
    }

    /// <summary>미션 프레임 실행은 준비 완료 brain이 있어도 AI 프레임을 돌리지 않는다.</summary>
    /// <remarks>2026-11-25 1.031에서 R4-5e1 프레임 부착용으로 추가했다. 실제 틱 결선(매 프레임 호출)은 Drops·Bridges와 함께 R6에서 잇는다.</remarks>
    [Fact]
    public void MissionFrameDefersAiFrameUntilR6()
    {
        (HostSession session, _) = BuildScene();
        uint brain = PrepareBrain(session, 2);
        Put(session.PlayerRecords, 2 * 180 + 0xb0, brain);
        Put(session.AiBrainMemory.Bytes(brain), 0x258, 1u);
        uint before = session.Game.State();
        session.RunMissionFrame();
        Assert.Equal(before, session.Game.State());
        Assert.Empty(session.AiRuntimeLogs);
        Assert.Empty(session.AiRuntimeDiagnostics);
    }
}
