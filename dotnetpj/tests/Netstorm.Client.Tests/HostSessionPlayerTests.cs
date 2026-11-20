using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// Player 정기 세션 결선을 검사하는 클래스다. 2026-11-19 0.643에서 R4-4 Player 정기용으로 추가했다.
// 근거: cpppj o/RawPlayerRegular.cpp의 RunPlayerRegular와 client/RawCampaignSession.cpp 1799행의 Native 분기다.
public sealed class HostSessionPlayerTests
{
    /// <summary>실제 자료로 Player 장면을 만든다. 지형·요새·바인딩·지역·Player를 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션이다. TYPE 표는 세션 장치와 같은 인스턴스다.</returns>
    private static HostSession BuildPlayerScene()
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

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.643에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>Player 부모 슬롯을 할당하고 가상표·소유자를 채운다. 0.643에서 추가했다.</summary>
    /// <param name="session">Player 장치가 연결된 세션이다.</param>
    /// <param name="owner">적을 소유자 바이트다. 기본 1은 현재 전투 소유자와 같다.</param>
    /// <returns>채운 Player 부모 번호다.</returns>
    private static int PreparePlayerSid(HostSession session, byte owner = 1)
    {
        Assert.Equal(1u, session.MissionPlayer.BattleOwner);
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050ee08u);
        raw[34] = owner;
        return sid;
    }

    /// <summary>사건 1을 세션 처리기로 보내면 소어 루저 로그 뒤 사건 2를 예약하고 0을 돌려준다.</summary>
    [Fact]
    public void PlayerRegularRoutesEventOneThroughSession()
    {
        HostSession session = BuildPlayerScene();
        int sid = PreparePlayerSid(session);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        float result = handler(sid, 1, 1, 5.0f);
        Assert.Equal(0.0f, result);
        Assert.Contains("29:SORE LOSER TEST: ", session.PlayerRegularLogs);
        Assert.Contains("29:STARTING.\n", session.PlayerRegularLogs);
        Assert.NotNull(session.PlayerProcesses.FindEvent(sid, 2));
    }

    /// <summary>사건2는현재실제manager의State에표시비트를쓰고헤더는보존한다.</summary>
    [Fact]
    public void PlayerRegularRoutesEventTwoThroughSession()
    {
        HostSession session = BuildPlayerScene();
        int sid = PreparePlayerSid(session);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        uint slot = session.MissionPlayer.TemplateOwner;
        Assert.True(slot <= 8, "Template 칸 번호가 header 범위를 벗어났다");
        session.Templates.Install(slot, TemplateStorageTestSupport.RealFort("thewarbegins"), "named");
        BinaryPrimitives.WriteUInt32LittleEndian(session.WorldTemplates.AsSpan((int)slot * 20 + 8, 4), 1);
        byte[] headers = session.WorldTemplates.ToArray();
        float result = handler(sid, 2, 1, 60.0f);
        Assert.Equal(0.0f, result);
        Assert.NotEqual(0, session.Templates.State(slot).Bytes.Span[0] & 4);
        Assert.Equal(headers, session.WorldTemplates);
        Assert.Contains(slot, session.PlayerTemplateMarks);
    }

    /// <summary>로컬 소유자가 아니면 세션 Player 분배가 원래 payload를 돌려준다.</summary>
    [Fact]
    public void PlayerRegularSkipsNonLocalOwner()
    {
        HostSession session = BuildPlayerScene();
        int sid = PreparePlayerSid(session, 2);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(5.0f, handler(sid, 1, 1, 5.0f));
        Assert.Empty(session.PlayerRegularLogs);
        Assert.Null(session.PlayerProcesses.FindEvent(sid, 2));
    }

    /// <summary>Player 가상표가 아니면 세션 Player 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void PlayerRegularFallsBackForOtherSids()
    {
        HostSession session = BuildPlayerScene();
        int sid = PreparePlayerSid(session);
        Put(session.Pool.AllocatedBytes(sid), 0, 0x0050da88u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(1.5f, handler(sid, 9, 1, 1.5f));
        Assert.Empty(session.PlayerRegularLogs);
        Assert.Empty(session.PlayerTemplateMarks);
    }
}
