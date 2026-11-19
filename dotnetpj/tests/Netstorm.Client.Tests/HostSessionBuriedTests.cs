using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 매몰 정기 세션 결선(난수 소비 포함)을 검사하는 클래스다. 2026-11-19 0.659에서 R4-4 매몰 정기용으로 추가했다.
// 근거: cpppj client/RawSurfaceStorage.cpp 300행의 Native 분기 0x50a748이다.
public sealed class HostSessionBuriedTests
{
    /// <summary>실제 자료로 장면을 만든다. 지형·요새·바인딩·지역 장치를 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static HostSession BuildBuriedScene(out IReadOnlyList<OriginalTypeRecord> types)
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
        types = table.Types;
        return session;
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 0.659에서 추가했다.</summary>
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

    /// <summary>B구간이 있고 레벨이 0인 첫 타입을 찾는다. 0.659에서 추가했다.</summary>
    /// <remarks>레벨이 0이면 프레임 지정이 재등록 없이 끝나므로 전용 Pop 분배가 없어도 된다.</remarks>
    private static int FindBuriedType(HostSession session, IReadOnlyList<OriginalTypeRecord> types)
    {
        // 모든 타입 번호를 순서대로 살피는 반복문이다. 없으면 데이터 오류로 거부한다.
        for (int i = 0; i < types.Count; i++)
        {
            if (session.WorldFrames[i].Run((byte)'B').Count > 0 && (types[i].Flags2 & 0x6u) != 0)
            {
                return i;
            }
        }

        throw new InvalidOperationException("매몰 검사 B구간·레벨 부족");
    }

    /// <summary>매몰 사건 0을 세션 처리기로 보내면 나머지에 따라 프레임이 진행된다.</summary>
    [Fact]
    public void BuriedRegularRoutesEventZeroThroughSession()
    {
        HostSession session = BuildBuriedScene(out IReadOnlyList<OriginalTypeRecord> types);
        int buried = FindBuriedType(session, types);
        int first = session.WorldFrames[buried].Run((byte)'B').First;
        int count = session.WorldFrames[buried].Run((byte)'B').Count;
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050a748u);
        raw[RawSidPool.TypeOffset] = unchecked((byte)buried);
        Put(raw, 36, 1u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        // 개수 1·제수(count+1)의 나머지로 B구간 첫 프레임부터 진행한다.
        int expected = unchecked((int)(unchecked((uint)first) + unchecked((uint)(1 % (count + 1))) - 1u));
        Assert.Equal(0.1f, handler(sid, 0, 1, 0.4f));
        Assert.Equal(expected, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(36, 4)));
    }

    /// <summary>매몰 가상표가 아니면 세션 분배가 이전 처리기로 넘긴다.</summary>
    [Fact]
    public void BuriedRegularFallsBackForOtherSids()
    {
        HostSession session = BuildBuriedScene(out _);
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0, 0x0050f540u);
        Put(raw, 36, 7u);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        Assert.Equal(0.7f, handler(sid, 9, 0, 0.7f));
        Assert.Equal(7, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(36, 4)));
    }
}
