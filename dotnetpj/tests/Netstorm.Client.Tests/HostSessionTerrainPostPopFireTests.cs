using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 세션 지형 Pop의 섬·표면 접두 실측 발화를 검사한다.
// cpppj/src/client/RawCampaignWorldRuntime.cpp 46행의 world.Pop 결선과
// o/SquidPostPop.cpp·o/Construction.cpp·o/Bridge.cpp의 접두·연결 순서에 대응한다.
// R4-2 적재 중 postPop의 세 번째 단계(접두 발화)다. 0.603에서 추가됐다.
namespace Netstorm.Client.Tests;

/// <summary>지형 Pop 호출에서 섬 후처리 Prefix와 연결 순회가 실제로 실행되는지 검사한다. 0.603에서 추가됐다.</summary>
public sealed class HostSessionTerrainPostPopFireTests
{
    /// <summary>실제 자료로 Pop 장면을 만든다. 타입 표·정의·SHP·프레임은 실파일 값이며 157번 받침의 방향 글자만 호출자가 정한다.</summary>
    /// <param name="surfaceLetter">157번 받침 프레임의 방향 글자다. Prefix 분기 선택을 고정한다.</param>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) LoadRealPopScene(byte surfaceLetter)
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shpDb = new ShapeDatabase(shpRaw);
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, shpDb);
        session.BuildTerrainPopDevices(table.Types, shapes);
        // 실제 프레임 표를 복사해 157번 받침의 방향 글자만 고정한다. 이웃 탐색이 읽는 다른 타입 표는 실파일 값 그대로 둔다.
        RiftTypeFrames[] frames = session.WorldFrames.ToArray();
        frames[157] = new RiftTypeFrames([new RiftFrameCode(surfaceLetter, (byte)'P', 1, 0)]);
        session.BuildIslandPostPopDevices(table.Types, frames);
        return (session, table.Types);
    }

    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
    private static void Write(System.Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다.
        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>슬롯 좌표·타입·owner를 기록한다.</summary>
    private static void Place(RawSidPool pool, int sid, byte type, float x, float y, byte owner, uint vtable)
    {
        System.Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        Write(raw, 0, vtable);
        raw[RawSidPool.TypeOffset] = type;
        raw[RawSidPool.StateOffset] = 0;
        raw[34] = owner;
        raw[40] = 0;
        Write(raw, 14, BitConverter.SingleToUInt32Bits(x));
        Write(raw, 18, BitConverter.SingleToUInt32Bits(y));
    }

    /// <summary>검사 크기의 풀 슬롯을 세션 풀에 확보한다.</summary>
    private static void AllocateScene(RawSidPool pool)
    {
        // 5번부터 200번까지 클라이언트 영역에 확보한다.
        for (int id = 5; id <= 200; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
    }

    /// <summary>해시 0단계 머리에 슬롯을 잇는다.</summary>
    private static void ChainHead(RawSquidHash hash, RawSidPool pool, int sid, float x, float y)
    {
        ref ushort head = ref hash.Bucket(0, x, y);
        Write(pool.AllocatedBytes(sid), 4, head, 2);
        head = unchecked((ushort)sid);
    }

    /// <summary>받침 표면 Pop이 3×3 머리와 섬·종유석에 소유자를 찍고 연결 순회를 실행한다.</summary>
    [Fact]
    public void SurfaceHPrefixStampsSupportAndConnects()
    {
        var (session, types) = LoadRealPopScene((byte)'H');
        AllocateScene(session.Pool);
        // 받침(NoIsland 157)은 0단계에 등록되어야 하므로 해시 단계를 확인한다.
        Assert.Equal(0, Netstorm.Core.Rules.SpotRules.HashLevel(types[157].Flags2, 1f, 1f));
        RawSquidHash hash = session.TerrainPopHash!;
        // 3×3 각 칸에 머리 슬롯을 둔다. (10,10)은 Pop되는 표면이 차지하고 (8,8)은 스탬프 원본 50이 차지한다.
        var heads = new System.Collections.Generic.List<int> { 50 };
        Place(session.Pool, 50, 157, 8.0f, 8.0f, 5, 0x00506f08u);
        ChainHead(hash, session.Pool, 50, 8.0f, 8.0f);
        int next = 60;
        for (int cx = 8; cx <= 10; cx++)
        {
            for (int cy = 8; cy <= 10; cy++)
            {
                if ((cx == 10 && cy == 10) || (cx == 8 && cy == 8))
                {
                    continue;
                }

                int sid = next++;
                Place(session.Pool, sid, 157, cx, cy, 0, 0x00506f08u);
                ChainHead(hash, session.Pool, sid, cx, cy);
                heads.Add(sid);
            }
        }

        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        int surface = hooks.Create!(157u, 1u);
        // 공장 생성 표면의 가상 표가 표면 분기를 타는지 확인한다.
        Assert.Equal(0x00506f08u, unchecked((uint)(session.Pool.Slot(surface)[0] | ((uint)session.Pool.Slot(surface)[1] << 8) | ((uint)session.Pool.Slot(surface)[2] << 16) | ((uint)session.Pool.Slot(surface)[3] << 24))));
        hooks.Pop!(surface, 10f, 10f, 0x02000061u);
        // Pop 호출이 분배 Pop으로 기록된다.
        Assert.Contains(session.TerrainPopCalls, entry => entry.Contains("island:ready") && entry.Contains("Registered"));
        // 3×3 머리 전부에 받침 소유자 5가 찍힌다.
        foreach (int sid in heads)
        {
            Assert.Equal(5, session.Pool.Slot(sid)[34]);
        }

        Assert.Equal(5, session.Pool.Slot(surface)[34]);
        // 같은 칸의 섬(94)·종유석(95)은 없어 누락 상세를 남긴다.
        Assert.Contains(session.IslandPostPopDiagnostics, entry => entry.Contains("No island"));
    }

    /// <summary>받침 표면 Pop의 F 분기가 섬을 만들고 등록한다.</summary>
    [Fact]
    public void SurfaceFPrefixCreatesIsland()
    {
        var (session, types) = LoadRealPopScene((byte)'F');
        AllocateScene(session.Pool);
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        int surface = hooks.Create!(157u, 1u);
        Assert.Equal(0x00506f08u, unchecked((uint)(session.Pool.Slot(surface)[0] | ((uint)session.Pool.Slot(surface)[1] << 8) | ((uint)session.Pool.Slot(surface)[2] << 16) | ((uint)session.Pool.Slot(surface)[3] << 24))));
        int usedBefore = UsedSlots(session);
        hooks.Pop!(surface, 10f, 10f, 0x02000061u);
        // F 분기가 섬(94)을 만들어 등록하므로 슬롯이 늘어난다.
        Assert.True(UsedSlots(session) > usedBefore);
        int island = FindType(session, 94u);
        Assert.NotEqual(0, island);
        Assert.Equal(0x00506d98u, unchecked((uint)(session.Pool.Slot(island)[0] | ((uint)session.Pool.Slot(island)[1] << 8) | ((uint)session.Pool.Slot(island)[2] << 16) | ((uint)session.Pool.Slot(island)[3] << 24))));
        // 중첩 Pop 호출도 순서대로 기록된다.
        Assert.True(session.TerrainPopCalls.Count >= 2);
    }

    /// <summary>실제 지형 생성의 Pop 호출열이 분배 뒤에도 결정적이고 풀 바이트와 같다.</summary>
    [Fact]
    public void LiveTerrainFirstPopCallsStayDeterministic()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        HostSession first = BuildLiveTerrainScene(startup, fort);
        HostSession second = BuildLiveTerrainScene(startup, fort);
        // 실제 지형 생성이 Pop을 호출한다.
        Assert.NotEmpty(first.TerrainPopCalls);
        Assert.Equal(first.TerrainPopCalls.Count, second.TerrainPopCalls.Count);
        // 호출열 전체가 순서대로 같다.
        for (int index = 0; index < first.TerrainPopCalls.Count; ++index)
        {
            Assert.Equal(first.TerrainPopCalls[index], second.TerrainPopCalls[index]);
        }

        // 기록과 분배는 풀 바이트에 결정적으로만 영향을 준다.
        Assert.Equal(first.Pool.Bytes.ToArray(), second.Pool.Bytes.ToArray());
    }

    /// <summary>사용한 슬롯 수를 센다. 빈 칸은 건너뛴다.</summary>
    /// <param name="session">셀 세션이다.</param>
    private static int UsedSlots(HostSession session)
    {
        int count = 0;
        // 일반 번호부터 빈 칸을 건너뛰고 실제 슬롯을 세는 반복문이다.
        for (int sid = RawSidPool.FirstOrdinary; sid < session.Pool.Capacity; ++sid)
        {
            if ((session.Pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0) continue;
            ++count;
        }

        return count;
    }

    /// <summary>해당 타입의 첫 슬롯을 찾는다. 없으면 0이다.</summary>
    private static int FindType(HostSession session, uint type)
    {
        // 번호 순서대로 타입 바이트를 비교하는 반복문이다.
        for (int sid = RawSidPool.FirstOrdinary; sid < session.Pool.Capacity; ++sid)
        {
            if ((session.Pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0) continue;
            if (session.Pool.Slot(sid)[RawSidPool.TypeOffset] == type) return sid;
        }

        return 0;
    }

    /// <summary>실제 지형 생성 장면을 만든다. live 시작 흐름과 같은 순서다.</summary>
    /// <param name="startup">실제 자료 시작 화면이다.</param>
    /// <param name="fort">적재할 미션 요새다.</param>
    /// <returns>지형 생성이 끝난 세션이다.</returns>
    private static HostSession BuildLiveTerrainScene(HostStartupScene startup, MissionFortLoadResult fort)
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        session.BuildIslandPostPopDevices(table.Types, session.WorldFrames);
        HostFortObjectLiveTests.BuildLiveTerrainFirst(session, fort.Fort, table.Types);
        return session;
    }
}
