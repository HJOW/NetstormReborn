using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 0.644 일괄 회귀에서 드러난 미연결 갈고리(AI 색 프레임·펜스 표식·포스 필드)의 실제 결선을 검사하는 클래스다.
// 근거: cpppj src/client/RawCampaignSession.cpp 2809행의 색 변경 조립과 client/RawSurfaceFence.cpp 26~50행의 생성 결선이다.
public sealed class HostSessionLiveHookTests
{
    /// <summary>실제 자료로 장면을 만든다. 지형·요새·바인딩·지역·프레임을 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표·자산 정의다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, IReadOnlyList<RiftTypeDefinition> Definitions) BuildScene()
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
        return (session, table.Types, definitions);
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.644에서 추가했다.</summary>
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

    /// <summary>펜스 부모 슬롯을 할당하고 가상표·타입·좌표·소유자를 채운다. 0.644에서 추가했다.</summary>
    private static int PrepareFenceSid(HostSession session, float x, float y)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x00509470u);
        raw[RawSidPool.TypeOffset] = 136;
        raw[RawSidPool.StateOffset] = 0;
        raw[40] = 0;
        raw[34] = 1;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
        Put(raw, 36, 0u);
        return sid;
    }

    /// <summary>AI 색 변경은 같은 실제 색 표를 쓰고 섬 받침 프레임을 갱신한다.</summary>
    [Fact]
    public void AiChangeColorUpdatesIslandFramesLive()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeDefinition> definitions) = BuildScene();
        // 섬 객체를 두어 프레임 갈고리에 닿게 한다. free 비트는 지운다.
        int island = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(island);
        raw.Fill(0);
        raw[RawSidPool.TypeOffset] = (byte)session.TerrainGlobals.IslandType;
        raw[34] = 1;
        AiCreateHooks hooks = session.BuildLiveAiCreateHooks(types, definitions);
        Assert.NotNull(hooks.ChangeColor);
        hooks.ChangeColor(1, 7);
        Assert.Equal(7, session.BridgeState.OwnerColors[1]);
    }

    /// <summary>펜스 표식 생성은 같은 owner 표식을 이웃 사이에 Pop한다.</summary>
    [Fact]
    public void FenceMarkerCreatePopulatesMarkersWithOwner()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, _) = BuildScene();
        int parent = PrepareFenceSid(session, 50.75f, 52.5f);
        int other = PrepareFenceSid(session, 62.75f, 52.5f);
        session.FenceMarkerCreate.Create(parent, other);
        Assert.NotNull(session.TerrainPopHash);
        var finder = new RawSquidFinder(session.Pool, session.TerrainPopHash, types);
        byte markerType = unchecked((byte)session.FenceMarkerCreate.MarkerType);
        int found = 0;
        // 끝점 사이 구간에서 같은 owner 표식을 세는 반복문이다.
        for (int sid = finder.Begin(new SquidSearchArea(51, 51, 63, 54)); sid != 0; sid = finder.Next())
        {
            if (session.Pool.Slot(sid)[RawSidPool.TypeOffset] == markerType && session.Pool.Slot(sid)[34] == 1)
            {
                found++;
            }
        }

        Assert.True(found > 0, "표식 생성 뒤 같은 owner 표식이 있어야 한다");
    }

    /// <summary>펜스 포스 필드 생성은 수평 구간의 J/K 첫 프레임을 쓴다.</summary>
    [Fact]
    public void FenceShieldCreateUsesLetterFirstFrame()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, _) = BuildScene();
        int parent = PrepareFenceSid(session, 50.75f, 52.5f);
        int other = PrepareFenceSid(session, 62.75f, 52.5f);
        int shieldType = -1;
        // 같은 TYPE 표에서 fenceShield 번호를 찾는 반복문이다.
        for (int i = 0; i < types.Count; i++)
        {
            if (types[i].Name == "fenceShield")
            {
                shieldType = i;
            }
        }

        Assert.True(shieldType >= 0, "fenceShield 타입이 있어야 한다");
        float[] endpoints = [50.75f, 52.5f, 62.75f, 52.5f];
        session.FenceShieldCreate.Create(endpoints, 1, 1, parent, other);
        Assert.NotNull(session.TerrainPopHash);
        var finder = new RawSquidFinder(session.Pool, session.TerrainPopHash, types);
        int expected = session.WorldFrames[shieldType].Run((byte)'K').First;
        int matched = 0, examined = 0;
        // 수평 구간에서 포스 필드를 찾아 첫 프레임을 대조하는 반복문이다.
        for (int sid = finder.Begin(new SquidSearchArea(49, 50, 64, 55)); sid != 0; sid = finder.Next())
        {
            if (session.Pool.Slot(sid)[RawSidPool.TypeOffset] == shieldType)
            {
                examined++;
                if (BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(36, 4)) == expected)
                {
                    matched++;
                }
            }
        }

        Assert.True(examined > 0, "포스 필드 생성 뒤 실드가 있어야 한다");
        Assert.Equal(examined, matched);
    }
}
