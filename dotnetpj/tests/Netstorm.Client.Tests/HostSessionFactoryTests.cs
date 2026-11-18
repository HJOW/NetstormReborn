using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 공장 정기 트리거 세션 결선을 검사하는 클래스다. 2026-11-19 0.625에서 R4-2 공장 정기 트리거용으로 추가했다.
// 근거: cpppj RawCampaignAssetBindings.cpp 73·81~88행의 Factory 조립과 182행의 SetFactoryPostPop 등록이다.
public sealed class HostSessionFactoryTests
{
    /// <summary>실제 자료로 공장 후처리 장면을 만든다. 지형·요새·바인딩·지역·자산을 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildFactoryScene()
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

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.625에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>같은 장치에서는 공장 본체·경고·정기 예약이 연결된다.</summary>
    [Fact]
    public void BuildsFactoryDevices()
    {
        (HostSession session, _) = BuildFactoryScene();
        Assert.NotNull(session.FactoryPostPop);
        Assert.Empty(session.FactoryWarnings);
        // 공장 가상표는 공장 몸체가 맡고 일반 슬롯은 맡지 않는다.
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Put(session.Pool.AllocatedBytes(sid), 0, 0x00509148u);
        Assert.True(session.FactoryPostPop.Handles(sid));
        Assert.True(session.TerrainPostPop.HandlesFactory(sid));
        int plain = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(plain).Fill(0);
        Assert.False(session.FactoryPostPop.Handles(plain));
        Assert.False(session.TerrainPostPop.HandlesFactory(plain));
    }

    /// <summary>공장 정기 예약은 실제 프로세스에 부착된다.</summary>
    [Fact]
    public void FactoryRegularReservationAttachesProcess()
    {
        (HostSession session, _) = BuildFactoryScene();
        Assert.NotNull(session.PlayerProcesses);
        Assert.NotNull(session.FactoryPostPop);
        Assert.Empty(session.FactoryWarnings);
        // 공장 가상표는 세션 분배에서도 처리된다.
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Put(session.Pool.AllocatedBytes(sid), 0, 0x00509148u);
        Assert.True(session.TerrainPostPop.HandlesFactory(sid));
    }

    /// <summary>장치가 없으면 명시 예외로 멈춘다.</summary>
    [Fact]
    public void RejectsUnbuiltFactory()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.FactoryPostPop);
        Assert.Empty(session.FactoryWarnings);
    }

    /// <summary>세션 공장 Pop은 타이머 종류에서 정기 예약을 두 건 부착한다.</summary>
    [Fact]
    public void FactoryRegularFiresInSession()
    {
        (HostSession session, _) = BuildFactoryScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x00509148u);
        raw[10] = 85;
        raw[11] = 0;
        raw[40] = 0;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(50.75f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(52.5f));
        Put(raw, 36, 7u);
        uint depth = session.PostPopDepth;
        session.PostPopDepth = depth + 1;
        session.FactoryPostPop.PostPop(sid, 1u);
        Assert.Equal(depth, session.PostPopDepth);
        Assert.Empty(session.FactoryWarnings);
    }

    /// <summary>공장 부모 번호에 타이머 종류·레벨·프레임을 채운다. 2026-11-19 0.629에서 공장 정기 분배용으로 추가했다.</summary>
    /// <param name="session">공장 장치가 연결된 세션이다.</param>
    /// <returns>채운 공장 부모 번호다.</returns>
    private static int PrepareFactorySid(HostSession session)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x00509148u);
        raw[10] = 85;
        raw[11] = 0;
        raw[40] = 0;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(50.75f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(52.5f));
        Put(raw, 36, 7u);
        return sid;
    }

    /// <summary>공장 가상표의 사건2를 세션 처리기로 보내면 현재 조회·난수·프레임 지정이 실행된다.</summary>
    [Fact]
    public void FactoryRegularRoutesEvent2ThroughSession()
    {
        (HostSession session, _) = BuildFactoryScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareFactorySid(session);
        int first = session.WorldFrames[85].Run((byte)'A').First;
        int size = session.WorldFrames[85].Run((byte)'A').Count;
        Assert.True(size > 0, "공장 정기 사건2의 실제 프레임 구간이 비었다");
        uint before = session.Game.State();
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        float result = handler(sid, 2, 0, 8.0f);
        Assert.Equal(8.0f, result);
        // 같은 seed의 독립 난수로 기대 난수 소비와 지정 프레임을 계산한다.
        var probe = new GameRandom(before);
        uint offset = probe.Next(unchecked((uint)size));
        Assert.Equal(probe.State(), session.Game.State());
        int expected = unchecked((int)unchecked((uint)first + offset));
        Assert.Equal(expected, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid)[36..]));
    }

    /// <summary>공장 가상표의 사건1을 세션 처리기로 보내면 프레임이 한 번 진행된다.</summary>
    [Fact]
    public void FactoryRegularRoutesEvent1ThroughSession()
    {
        (HostSession session, _) = BuildFactoryScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareFactorySid(session);
        int first = session.WorldFrames[85].Run((byte)'A').First;
        int size = session.WorldFrames[85].Run((byte)'A').Count;
        Assert.True(size > 0, "공장 정기 사건1의 실제 프레임 구간이 비었다");
        Put(session.Pool.AllocatedBytes(sid), 36, unchecked((uint)first));
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        float result = handler(sid, 1, 0, 0.5f);
        Assert.Equal(0.5f, result);
        int expected = size == 1 ? first : first + 1;
        Assert.Equal(expected, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid)[36..]));
    }

    /// <summary>공장 가상표가 아니면 세션 공장 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void FactoryRegularFallsBackForOtherSids()
    {
        (HostSession session, _) = BuildFactoryScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(sid).Fill(0);
        byte[] before = session.Pool.Slot(sid).ToArray();
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(3.25f, handler(sid, 2, 0, 3.25f));
        Assert.Equal(before, session.Pool.Slot(sid).ToArray());
    }

    /// <summary>레벨이 2 이하면 사건5가 플레어 없이 이전 처리기로 넘어간다.</summary>
    [Fact]
    public void FactoryRegularEvent5BelowLevelSkipsFlare()
    {
        (HostSession session, _) = BuildFactoryScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareFactorySid(session);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.5f, handler(sid, 5, 0, 0.5f));
    }

    /// <summary>사건3의 완성은 큰 실제 몸체가 없으면 명시 예외로 멈춘다.</summary>
    [Fact]
    public void FactoryRegularEvent3FinishNeedsRuntime()
    {
        (HostSession session, _) = BuildFactoryScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareFactorySid(session);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Throws<InvalidOperationException>(() => handler(sid, 3, 1, 9.0f));
    }

    /// <summary>사건4의 개수 삭제는 실제 장치 없이 0을 돌려준다.</summary>
    [Fact]
    public void FactoryRegularEvent4KillNeedsNoDevices()
    {
        (HostSession session, _) = BuildFactoryScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareFactorySid(session);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.0f, handler(sid, 4, 1, 9.0f));
    }

    /// <summary>미션 틱은 공유 시각을 전진시키고 공장 정기 사건2로 프레임을 갱신한다.</summary>
    [Fact]
    public void AdvanceMissionFrameTicksFactoryRegular()
    {
        (HostSession session, _) = BuildFactoryScene();
        session.TerrainBookkeeping.Suppressed = true;
        int sid = PrepareFactorySid(session);
        int first = session.WorldFrames[85].Run((byte)'A').First;
        int size = session.WorldFrames[85].Run((byte)'A').Count;
        Assert.True(size > 0, "공장 틱의 실제 프레임 구간이 비었다");
        // 실제 경로는 RawPop이 공간·첫Pop 비트를 먼저 두므로 직접 Pop 전에 같은 상태를 둔다.
        // 비트가 없으면 AdjustFrame 재Pop이 첫Pop으로 다시 예약을 붙여 실제와 달라진다.
        session.Pool.AllocatedBytes(sid)[40] |= 0x80;
        uint depth = session.PostPopDepth;
        session.PostPopDepth = depth + 1;
        session.FactoryPostPop.PostPop(sid, 1u);
        Assert.Equal(depth, session.PostPopDepth);
        Assert.Equal(2, session.Kernel.Size());
        uint before = session.Game.State();
        session.AdvanceMissionFrame(0.025);
        Assert.Equal(0.025, session.ProcessTime.Value);
        // 틱에서 사건2만 난수를 소비하므로 같은 seed의 독립 난수로 기대값을 계산한다.
        var probe = new GameRandom(before);
        uint offset = probe.Next(unchecked((uint)size));
        Assert.Equal(probe.State(), session.Game.State());
        int expected = unchecked((int)unchecked((uint)first + offset));
        Assert.Equal(expected, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid)[36..]));
        session.AdvanceMissionFrame(0.025);
        Assert.Equal(0.05, session.ProcessTime.Value);
        Assert.Equal(expected, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid)[36..]));
    }
}
