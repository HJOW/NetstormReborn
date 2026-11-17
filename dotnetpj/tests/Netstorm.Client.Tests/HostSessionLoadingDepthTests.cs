using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 세션 적재 깊이가 섬 후처리 Prefix의 프레임 선택을 여는지 검사한다.
// cpppj/src/o/Construction.cpp 의 적재 중 방향 선택과 같은 순서다.
// R4-2 적재 중 postPop의 네 번째 단계(깊이 흐름)다. 0.604에서 추가됐다.
namespace Netstorm.Client.Tests;

/// <summary>적재 깊이에 따라 표면 Pop이 서·북 칸으로 프레임을 고르는지 검사한다. 0.604에서 추가됐다.</summary>
public sealed class HostSessionLoadingDepthTests
{
    /// <summary>실제 자료로 Pop 장면을 만든다. 157번 받침의 프레임 코드만 호출자가 정한다.</summary>
    /// <param name="codes">157번 받침의 프레임 코드다. 서·북·선택 글자를 고정한다.</param>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) LoadRealPopScene(IReadOnlyList<RiftFrameCode> codes)
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
        // 실제 프레임 표를 복사해 157번 받침의 코드만 고정한다. 이웃 탐색이 읽는 다른 타입 표는 실파일 값 그대로 둔다.
        RiftTypeFrames[] frames = session.WorldFrames.ToArray();
        frames[157] = new RiftTypeFrames(codes);
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

    /// <summary>슬롯의 패치판 프레임 DWORD를 읽는다.</summary>
    private static int ReadFrame(RawSidPool pool, int sid)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        return unchecked((int)(raw[36] | ((uint)raw[37] << 8) | ((uint)raw[38] << 16) | ((uint)raw[39] << 24)));
    }

    /// <summary>서·북 글자가 C·C인 받침 장면을 만든다. 서·북 머리와 표면을 두고 호출자에게 돌려준다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, int Surface) BuildLetterScene()
    {
        var codes = new RiftFrameCode[]
        {
            new((byte)'C', (byte)'P', 1, 0),
            new((byte)'G', (byte)'P', 1, 0),
        };
        var (session, types) = LoadRealPopScene(codes);
        AllocateScene(session.Pool);
        // 받침(NoIsland 157)은 0단계에 등록되어야 하므로 해시 단계를 확인한다.
        Assert.Equal(0, Netstorm.Core.Rules.SpotRules.HashLevel(types[157].Flags2, 1f, 1f));
        RawSquidHash hash = session.TerrainPopHash!;
        // 서쪽 (9,10)과 북쪽 (10,9) 칸에 C 글자 머리를 둔다.
        Place(session.Pool, 60, 157, 9.0f, 10.0f, 0, 0x00506f08u);
        ChainHead(hash, session.Pool, 60, 9.0f, 10.0f);
        Place(session.Pool, 61, 157, 10.0f, 9.0f, 0, 0x00506f08u);
        ChainHead(hash, session.Pool, 61, 10.0f, 9.0f);
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        int surface = hooks.Create!(157u, 1u);
        // 공장 생성 표면의 가상 표가 표면 분기를 타는지 확인한다.
        Assert.Equal(0x00506f08u, unchecked((uint)(session.Pool.Slot(surface)[0] | ((uint)session.Pool.Slot(surface)[1] << 8) | ((uint)session.Pool.Slot(surface)[2] << 16) | ((uint)session.Pool.Slot(surface)[3] << 24))));
        return (session, types, surface);
    }

    /// <summary>적재 깊이가 1이면 서·북 칸으로 프레임을 골라 쓴다.</summary>
    [Fact]
    public void DepthOneSelectsFrameFromNeighbors()
    {
        var (session, types, surface) = BuildLetterScene();
        session.SetWorldLoadingDepth(1u);
        try
        {
            TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
            hooks.Pop!(surface, 10f, 10f, 0x02000061u);
        }
        finally
        {
            session.SetWorldLoadingDepth(0u);
        }

        // 서 C·북 C에서 고른 G 글자의 프레임 번호 1이 기록된다.
        Assert.Equal(1, ReadFrame(session.Pool, surface));
        // Pop 호출이 분배 Pop으로 기록된다.
        Assert.Contains(session.TerrainPopCalls, entry => entry.Contains("island:ready") && entry.Contains("Registered"));
        // G 글자는 F·H 분기를 타지 않으므로 스탬프 흔적이 남지 않는다.
        Assert.DoesNotContain(session.IslandPostPopDiagnostics, entry => entry.Contains("No island"));
    }

    /// <summary>적재 깊이가 0이면 프레임 선택을 건너뛰고 요새 읽기 프레임을 유지한다.</summary>
    [Fact]
    public void DepthZeroKeepsFortFrame()
    {
        var (session, types, surface) = BuildLetterScene();
        TerrainStampHooks hooks = session.BuildTerrainStampHooks(types);
        hooks.Pop!(surface, 10f, 10f, 0x02000061u);
        // 깊이가 0이면 프레임 블록을 건너뛰어 공장 초기 프레임 0이 남는다.
        Assert.Equal(0, ReadFrame(session.Pool, surface));
        // 같은 호출이 분배 Pop으로 기록된다.
        Assert.Contains(session.TerrainPopCalls, entry => entry.Contains("island:ready") && entry.Contains("Registered"));
    }
}
