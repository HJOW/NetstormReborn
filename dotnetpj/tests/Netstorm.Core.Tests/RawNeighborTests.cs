using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 이웃 탐색기(<see cref="RawSquidNeighbors"/>)의 보호 경로와 기하 계약을 확인한다.
/// 원본 기대값 재생(X86RawNeighborTests)은 첫 이웃 번호를 대조하고,
/// 이 검사들은 cpppj/tests/NeighborTests.cpp 의 보호·기하 검사만 10.78 몫으로 옮겼다 (W02 이웃 조각, 2026-10-20).
/// </summary>
public sealed class RawNeighborTests
{
    /// <summary>검사 대상 번호.</summary>
    private const int Source = 50;

    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>분석 요구와 같은 생성 코드 목록을 만든다(변형 0).</summary>
    private static List<FrameCode> Frames()
    {
        const string letters = "JKALMNOJKLBJPPCD";
        var codes = new List<FrameCode>();
        // 변형 0은 하드 비트가 없다
        for (int i = 0; i < letters.Length; i++)
        {
            var flags = (FrameCodeFlags)(letters[i] == 'L' || letters[i] == 'M' ? 0x20 : 0);
            codes.Add(new FrameCode(letters[i], 'P', i + 1, flags));
        }
        return codes;
    }

    /// <summary>풀·해시·타입·프레임을 만들고 두 노드를 심는다.</summary>
    /// <param name="nodes">12칸 노드 설명.</param>
    private static (RawSidPool pool, RawSquidHash hash, byte[] spots, OriginalTypeRecord[] types, List<IReadOnlyList<FrameCode>> frames) Scene(string nodes)
    {
        var pool = new RawSidPool(24000, false);
        // 원본 관찰의 번호 5~74번을 클라이언트 영역에 확보한다
        for (int id = 5; id <= 74; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
        var hash = new RawSquidHash();
        var spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호에 빈 타입 레코드를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        var frames = new List<IReadOnlyList<FrameCode>>();
        // 모든 타입 번호마다 프레임 목록을 하나씩 넣는다
        for (int i = 0; i < types.Length; i++)
        {
            frames.Add(Frames());
        }
        hash.Reset();
        // 기준 번호(Source)와 60번 슬롯을 차례로 비운다
        foreach (int sid in new[] { Source, 60 })
        {
            pool.RawSlot(sid).Clear();
        }
        // 각 노드의 raw·타입·프레임·해시 체인을 채운다
        foreach (string entry in nodes.Split(';'))
        {
            string[] n = entry.Split(',');
            int sid = X86Fixture.Int(n[0]);
            uint typ = X86Fixture.UInt(n[1]);
            Span<byte> raw = pool.AllocatedBytes(sid);
            raw.Clear();
            Put(raw, 0, 0x11010000, 4);
            raw[RawSidPool.TypeOffset] = (byte)typ;
            raw[RawSidPool.StateOffset] = (byte)X86Fixture.UInt(n[2]);
            raw[40] = (byte)X86Fixture.UInt(n[3]);
            Put(raw, 14, X86Fixture.UInt(n[4]), 4);
            Put(raw, 18, X86Fixture.UInt(n[5]), 4);
            Put(raw, 36, X86Fixture.UInt(n[10]), 4);
            types[typ].FootX = X86Fixture.Int(n[6]);
            types[typ].FootY = X86Fixture.Int(n[7]);
            types[typ].Flags1 = X86Fixture.UInt(n[8]);
            types[typ].Flags2 = X86Fixture.UInt(n[9]);
            ref ushort head = ref hash.Bucket(X86Fixture.Int(n[11]), BitConverter.UInt32BitsToSingle(X86Fixture.UInt(n[4])), BitConverter.UInt32BitsToSingle(X86Fixture.UInt(n[5])));
            Put(raw, 4, head, 2);
            head = (ushort)sid;
        }
        return (pool, hash, spots, types, frames);
    }

    /// <summary>
    /// 첫 이웃은 끊긴 다음 번호의 버킷으로 넘어가지 않으며 풀을 바꾸지 않는다.
    /// 빈 번호·빈 지도 조회는 거부한다.
    /// </summary>
    [Fact]
    public void FirstStopsBeforeUnvisitedDamageAndDoesNotMutate()
    {
        var (pool, hash, spots, types, frames) = Scene(
            "50,82,0,0,1101004803,1101529088,1,1,2048,4,2,0;70,83,0,0,1101529088,1101529088,1,1,2048,4,2,0");
        Span<byte> candidate = pool.RawSlot(70);
        Put(candidate, 4, 65535, 2);
        hash.Cell(3, 1, 1) = 65535;
        byte[] before = pool.Bytes.ToArray();
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        // 20/21 체인의 이웃이 단계 0에서 반환된다
        Assert.Equal(70, neighbors.First(Source));
        Assert.Equal(before, pool.Bytes.ToArray());
        // 빈 번호는 거부한다
        Assert.ThrowsAny<Exception>(() => neighbors.First(0));
        // 빈 지도로는 만들지 않는다
        Assert.ThrowsAny<Exception>(() => new RawSquidNeighbors(pool, hash, [], types, frames));
        // 지원 밖 플래그(표면 지도 조회)는 거부한다
        Assert.ThrowsAny<Exception>(() => new RawSquidNeighborWalk(neighbors, Source, 8));
    }

    /// <summary>
    /// abstract 후보는 SkipAbstract 플래그가 있을 때만 건너뛴다.
    /// </summary>
    [Fact]
    public void SkipAbstractFlagSkipsAbstractCandidates()
    {
        var (pool, hash, spots, types, frames) = Scene(
            "50,82,0,0,1101004803,1101529088,1,1,2048,4,2,0;70,83,0,0,1101529088,1101529088,1,1,2048,4,2,0");
        pool.RawSlot(70)[40] = 1;
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        // 플래그 없이 abstract 후보를 받는다
        Assert.Equal(70, neighbors.First(Source));
        // SkipAbstract이면 abstract 후보를 건너뛰고 끝난다
        Assert.Equal(0, new RawSquidNeighborWalk(neighbors, Source, NeighborFlags.SkipAbstract).Current);
    }

    /// <summary>
    /// 죽은 후보는 건너뛴다. 다음 후보·다음 버킷으로 이어간다.
    /// </summary>
    [Fact]
    public void DeadCandidatesAreSkipped()
    {
        var (pool, hash, spots, types, frames) = Scene(
            "50,82,0,0,1101004803,1101529088,1,1,2048,4,2,0;70,83,0,0,1101529088,1101529088,1,1,2048,4,2,0");
        pool.RawSlot(70)[RawSidPool.StateOffset] |= 2;
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        // 죽은 70번을 건너뛰면 남은 후보가 없어 끝난다
        Assert.Equal(0, neighbors.First(Source));
    }

    /// <summary>
    /// 후보 기준점의 spot 칸은 0.9999를 더하고 자른다. 다음 칸의 간헐천 비트를 본다.
    /// </summary>
    [Fact]
    public void SpotBiasSelectsNextCell()
    {
        var (pool, hash, spots, types, frames) = Scene(
            "50,82,0,0,1101004803,1101529088,1,1,2048,4,2,0;70,83,0,0,1101529088,1101529088,1,1,2048,4,2,0");
        // 70번을 소수 좌표로 옮긴다(해시 체인은 그대로 둔다)
        Put(pool.RawSlot(70), 14, BitConverter.SingleToUInt32Bits(20.75f), 4);
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        // 0.9999를 더한 칸(21,21)에 간헐천 비트가 있으면 제외한다
        spots[21 * RawSquidHash.WorldCells + 21] = 8;
        Assert.Equal(0, neighbors.First(Source));
    }

    /// <summary>
    /// 범위를 벗어난 프레임·타입 번호는 sid 기준으로 거부한다.
    /// </summary>
    [Fact]
    public void OutOfRangeFrameAndTypeReportSid()
    {
        var (pool, hash, spots, types, frames) = Scene(
            "50,82,0,0,1101004803,1101529088,1,1,2048,4,2,0;70,83,0,0,1101529088,1101529088,1,1,2048,4,2,0");
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        // 프레임 번호가 표를 벗어나면 sid 기준으로 거부한다
        pool.RawSlot(70)[36] = 99;
        var frameError = Assert.Throws<ArgumentOutOfRangeException>(() => neighbors.First(Source));
        Assert.Equal("sid", frameError.ParamName);
        // 타입 번호가 표를 벗어나면 sid 기준으로 거부한다
        pool.RawSlot(70)[36] = 0;
        pool.RawSlot(50)[RawSidPool.TypeOffset] = 250;
        var typeError = Assert.Throws<ArgumentOutOfRangeException>(() => neighbors.Footprint(Source));
        Assert.Equal("sid", typeError.ParamName);
    }

    /// <summary>
    /// 후보 기준점의 spot 칸 순서는 y·256+x 이다. 다음 후보·다음 버킷으로 이어간다.
    /// </summary>
    [Fact]
    public void SpotIndexFollowsRowMajorOrder()
    {
        var (pool, hash, spots, types, frames) = Scene(
            "50,82,0,0,1101004803,1101529088,1,1,2048,4,2,0;70,83,0,0,1101529088,1101529088,1,1,2048,4,2,0");
        // 70번을 (20.5, 22.0) 인근으로 옮긴다(해시 체인은 그대로 둔다)
        Put(pool.RawSlot(70), 14, BitConverter.SingleToUInt32Bits(20.5f), 4);
        Put(pool.RawSlot(70), 18, BitConverter.SingleToUInt32Bits(22.0f), 4);
        types[83].FootX = 1;
        types[83].FootY = 2;
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        // 0.9999를 더한 칸(22,21)에 간헐천 비트가 있으면 제외한다
        spots[22 * RawSquidHash.WorldCells + 21] = 8;
        Assert.Equal(0, new RawSquidNeighborWalk(neighbors, Source, NeighborFlags.NoJoint | NeighborFlags.AnyType).Current);
    }

    /// <summary>
    /// 보드 밖 x 좌표의 발자국은 보정값으로 자르고 유효 검사를 건너뛴다.
    /// </summary>
    [Fact]
    public void OutOfBoardFootprintClampsAndSkips()
    {
        var (pool, hash, spots, types, frames) = Scene(
            "50,82,0,0,1101004803,1101529088,1,1,2048,4,2,0;70,83,0,0,1101529088,1101529088,1,1,2048,4,2,0");
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        // 음수 x 는 1로 자르고 유효 검사를 건너뛴다
        Put(pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(-5f), 4);
        Assert.Equal(new SquidFootprint(1f, 21.0f, 1f, 21.0f), neighbors.Footprint(Source));
        // 256 이상 x 는 255로 자르고 유효 검사를 건너뛴다
        Put(pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(300f), 4);
        Assert.Equal(new SquidFootprint(255f, 21.0f, 255f, 21.0f), neighbors.Footprint(Source));
    }

    /// <summary>
    /// 타입 번호가 자산 범위 밖이면 sid 기준으로 거부한다.
    /// </summary>
    [Fact]
    public void AssetTypeRangeRejectsNonAssetTypes()
    {
        var (pool, hash, spots, types, frames) = Scene(
            "50,82,0,0,1101004803,1101529088,1,1,2048,4,2,0");
        types[5].FootX = 1;
        types[5].FootY = 1;
        pool.RawSlot(50)[RawSidPool.TypeOffset] = 5;
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        var error = Assert.Throws<ArgumentOutOfRangeException>(() => neighbors.Footprint(Source));
        Assert.Equal("sid", error.ParamName);
    }

    /// <summary>
    /// 안쪽이면 회귀를 시작하지 않고 끝난다.
    /// </summary>
    [Fact]
    public void InteriorSourceSkipsWalk()
    {
        var (pool, hash, spots, types, frames) = Scene(
            "50,82,0,0,1101004803,1101529088,1,1,2048,4,2,0");
        Array.Fill(spots, (byte)8);
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        var walk = new RawSquidNeighborWalk(neighbors, Source, 0);
        Assert.Equal(0, walk.Current);
        Assert.Equal(0, walk.Next());
    }

    /// <summary>
    /// 방향은 가로·세로 차이의 대소로 정한다. 같으면 세로, 위쪽 0·오른쪽 2·아래 4·왼쪽 6이다.
    /// </summary>
    [Fact]
    public void DirectionFollowsAxisDifferences()
    {
        Assert.Equal(2, RawSquidNeighbors.Direction(0, 0, 1, 0));
        Assert.Equal(6, RawSquidNeighbors.Direction(0, 0, -1, 0));
        Assert.Equal(4, RawSquidNeighbors.Direction(0, 0, 0, 1));
        Assert.Equal(0, RawSquidNeighbors.Direction(0, 0, 0, -1));
        Assert.Equal(4, RawSquidNeighbors.Direction(0, 0, 1, 1));
        Assert.Equal(2, RawSquidNeighbors.Direction(0, 0, 2, 1));
    }

    /// <summary>
    /// 발자국 사각형과 중심은 원본 float 계산과 같다.
    /// </summary>
    [Fact]
    public void FootprintAndCenterMatchFloatGeometry()
    {
        var (pool, hash, spots, types, frames) = Scene(
            $"50,82,0,0,{BitConverter.SingleToUInt32Bits(20.75f)},{BitConverter.SingleToUInt32Bits(21.9f)},1,1,2048,4,2,0");
        var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
        SquidFootprint footprint = neighbors.Footprint(Source);
        Assert.Equal(20.75f, footprint.Left);
        Assert.Equal(21.9f, footprint.Top);
        Assert.Equal(20.75f, footprint.Right);
        Assert.Equal(21.9f, footprint.Bottom);
        float[] center = neighbors.Center(Source);
        Assert.Equal(20.25f, center[0]);
        Assert.Equal(21.4f, center[1]);
    }
}
