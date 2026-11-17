using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 세션 섬 후처리 서비스의 결선과 받침 소유자 스탬프 계약을 검사한다.
// cpppj/src/o/RawIslandPostPop.cpp 의 SetSupportOwner 3×3·finder 이웃을 대조한다.
// 0.601에서 추가됐다. (R4-2 적재 중 postPop 섬 후처리)
public sealed class HostSessionIslandPostPopTests
{
    /// <summary>검사 크기의 풀 슬롯을 세션 풀에 확보한다.</summary>
    /// <param name="pool">확보할 세션 풀.</param>
    private static void AllocateScene(RawSidPool pool)
    {
        // 5번부터 200번까지 클라이언트 영역에 확보한다.
        for (int id = 5; id <= 200; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
    }

    /// <summary>슬롯 좌표·타입·owner를 기록한다.</summary>
    private static void Place(RawSidPool pool, int sid, byte type, float x, float y, byte owner = 0, uint vtable = 0x0050C3B0u)
    {
        System.Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        raw[0] = (byte)vtable;
        raw[1] = (byte)(vtable >> 8);
        raw[2] = (byte)(vtable >> 16);
        raw[3] = (byte)(vtable >> 24);
        raw[RawSidPool.TypeOffset] = type;
        raw[RawSidPool.StateOffset] = 0;
        raw[34] = owner;
        raw[40] = 0;
        Write(raw, 14, BitConverter.SingleToUInt32Bits(x));
        Write(raw, 18, BitConverter.SingleToUInt32Bits(y));
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

    /// <summary>섬 후처리용 188 타입·프레임 표를 만든다.</summary>
    private static (OriginalTypeRecord[] Types, RiftTypeFrames[] Frames) Tables()
    {
        // 188개 타입 번호 각각에 빈 타입 레코드를 채운다.
        var types = new OriginalTypeRecord[188];
        for (int index = 0; index < types.Length; index++)
        {
            types[index] = new OriginalTypeRecord { FootX = 1, FootY = 1 };
        }

        // 188개 타입 번호 각각에 빈 프레임 표를 채운다.
        var frames = new RiftTypeFrames[188];
        for (int index = 0; index < frames.Length; index++)
        {
            frames[index] = new RiftTypeFrames([]);
        }

        return (types, frames);
    }

    /// <summary>장치 순서와 표 계약을 어기면 만들지 않는다.</summary>
    [Fact]
    public void BuildRequiresTerrainDevicesAndTables()
    {
        var (types, frames) = Tables();
        var shapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new SquidDisplayShape()).ToList();
        var bare = new Netstorm.Client.Host.HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        // 지형 Pop 장치 전에는 거부한다.
        Assert.Throws<System.InvalidOperationException>(() => bare.BuildIslandPostPopDevices(types, frames));
        var session = new Netstorm.Client.Host.HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        session.BuildTerrainPopDevices(types, shapes);
        // 표 크기가 다르면 거부한다.
        Assert.Throws<System.ArgumentException>(() => session.BuildIslandPostPopDevices(types, [.. frames.SkipLast(1)]));
        session.BuildIslandPostPopDevices(types, frames);
        Assert.Same(session.Pool, session.IslandPostPop.Pool);
        // 중복 연결은 거부한다.
        Assert.Throws<System.InvalidOperationException>(() => session.BuildIslandPostPopDevices(types, frames));
        // 미연결 조회는 명시 실패한다.
        Assert.Throws<System.InvalidOperationException>(() => bare.IslandPostPop);
    }

    /// <summary>받침 소유자 스탬프는 3×3 머리와 섬·종유석에 owner를 준다.</summary>
    [Fact]
    public void SetSupportOwnerStampsThreeByThree()
    {
        var (types, frames) = Tables();
        var shapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new SquidDisplayShape()).ToList();
        var session = new Netstorm.Client.Host.HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        AllocateScene(session.Pool);
        session.BuildTerrainPopDevices(types, shapes);
        session.BuildIslandPostPopDevices(types, frames);
        RawSquidHash hash = session.TerrainPopHash!;
        // (10,10) 받침(NoIsland 157)을 둔다.
        Place(session.Pool, 50, 157, 10.0f, 10.0f);
        // 3×3 각 칸에 머리 슬롯을 둔다. (10,10)은 받침 자체다.
        var heads = new System.Collections.Generic.List<int> { 50 };
        int next = 60;
        for (int cx = 8; cx <= 10; cx++)
        {
            for (int cy = 8; cy <= 10; cy++)
            {
                if (cx == 10 && cy == 10)
                {
                    continue;
                }

                int sid = next++;
                Place(session.Pool, sid, 1, cx, cy);
                ref ushort head = ref hash.Bucket(0, (float)cx, (float)cy);
                Write(session.Pool.AllocatedBytes(sid), 4, head, 2);
                head = unchecked((ushort)sid);
                heads.Add(sid);
            }
        }

        // 받침 칸 체인에 받침을 맨 앞에 잇는다.
        {
            ref ushort head = ref hash.Bucket(0, 10.0f, 10.0f);
            Write(session.Pool.AllocatedBytes(50), 4, head, 2);
            head = 50;
        }

        // 같은 칸의 섬(94)·종유석(95)은 1단계 finder가 찾는다.
        Place(session.Pool, 70, 94, 10.0f, 10.0f);
        Place(session.Pool, 71, 95, 10.0f, 10.0f);
        hash.Bucket(1, 10.0f, 10.0f) = 71;
        Write(session.Pool.AllocatedBytes(71), 4, 70u, 2);
        Write(session.Pool.AllocatedBytes(70), 4, 0u, 2);
        int diagnostics = session.IslandPostPopDiagnostics.Count;
        session.IslandPostPop.SetSupportOwner(10.0f, 10.0f, 3u);
        // 3×3 머리 전부에 owner 3이 찍힌다.
        foreach (int sid in heads)
        {
            Assert.Equal(3, session.Pool.Slot(sid)[34]);
        }

        // finder가 찾은 섬·종유석에도 owner 3이 찍힌다.
        Assert.Equal(3, session.Pool.Slot(70)[34]);
        Assert.Equal(3, session.Pool.Slot(71)[34]);
        // 전부 찾으면 진단을 남기지 않고 끝난다.
        Assert.Equal(diagnostics, session.IslandPostPopDiagnostics.Count);
    }

    /// <summary>못 찾은 받침·섬·종유석은 누락 진단을 남긴다.</summary>
    [Fact]
    public void SetSupportOwnerReportsMissingLinks()
    {
        var (types, frames) = Tables();
        var shapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new SquidDisplayShape()).ToList();
        var session = new Netstorm.Client.Host.HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        AllocateScene(session.Pool);
        session.BuildTerrainPopDevices(types, shapes);
        session.BuildIslandPostPopDevices(types, frames);
        // 받침만 두고 1단계 체인은 비워 둔다.
        Place(session.Pool, 50, 157, 10.0f, 10.0f);
        session.TerrainPopHash!.Bucket(0, 10.0f, 10.0f) = 50;
        int diagnostics = session.IslandPostPopDiagnostics.Count;
        session.IslandPostPop.SetSupportOwner(10.0f, 10.0f, 3u);
        // 섬·종유석을 못 찾으면 누락 상세를 남긴다.
        Assert.True(session.IslandPostPopDiagnostics.Count > diagnostics);
        Assert.Contains(session.IslandPostPopDiagnostics, entry => entry.Contains("No island"));
    }
}
