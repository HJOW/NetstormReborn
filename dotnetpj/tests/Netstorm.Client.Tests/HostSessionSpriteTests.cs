using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 스프라이트 정기 세션 결선을 검사하는 클래스다. 2026-11-19 0.651에서 R4-4 스프라이트 정기용으로 추가했다.
// 근거: cpppj client/RawCampaignSession.cpp 1772행의 스프라이트 분기다.
public sealed class HostSessionSpriteTests
{
    /// <summary>실제 자료로 장면을 만든다. 지형·요새·바인딩·지역 장치를 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표·프레임 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildSpriteScene()
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

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.651에서 추가했다.</summary>
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

    /// <summary>코드 수가 1보다 큰 첫 타입을 찾는다. 0.651에서 추가했다.</summary>
    private static int FindFramedType(HostSession session, IReadOnlyList<OriginalTypeRecord> types)
    {
        // 모든 타입 번호를 순서대로 살피는 반복문이다. 없으면 데이터 오류로 거부한다.
        for (int i = 0; i < types.Count; i++)
        {
            if (session.WorldFrames[i].Codes.Count > 1)
            {
                return i;
            }
        }

        throw new InvalidOperationException("스프라이트 검사 프레임 코드 부족");
    }

    /// <summary>스프라이트 사건을 세션 처리기로 보내면 현재 프레임 구간에서 진행된다.</summary>
    [Fact]
    public void SpriteRegularRoutesThroughSession()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildSpriteScene();
        int framed = FindFramedType(session, types);
        int count = session.WorldFrames[framed].Codes.Count;
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050cc08u);
        raw[RawSidPool.TypeOffset] = unchecked((byte)framed);
        Put(raw, 36, 1u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        float result = handler(sid, 0, 0, 0.4f);
        Assert.Equal(0.4f, result);
        Assert.Equal(2 % count, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(36, 4)));
    }

    /// <summary>스프라이트 가상표가 아니면 세션 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void SpriteRegularFallsBackForOtherSids()
    {
        (HostSession session, _) = BuildSpriteScene();
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050da88u);
        Put(raw, 36, 7u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 9, 0, 0.7f));
        Assert.Equal(7, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(36, 4)));
    }
}
