using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원본 표면 지도 순회의 원천 snapshot·현재 후보·중복 캐시를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj SurfaceNeighborCurrentTests 전체와 현재 값 원천·TYPE 대여 계약을 검사한다.</summary>
public sealed class SurfaceNeighborCurrentTests
{
    /// <summary>입력 객체만 실제 raw 풀에 놓으며 기대 이웃은 원본 fixture에서 읽는 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>관리 CD도50바이트인 기존 하네스를 사용하는 실제 풀이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>현재 단계0 표면 지도다.</summary>
        public readonly RawSquidHash Hash = new();
        /// <summary>현재 내부 bit8 필터다.</summary>
        public readonly byte[] Spots = new byte[65536];
        /// <summary>독립 원천의 현재 타입 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types;
        /// <summary>독립 원천의 프레임 입력이다.</summary>
        public readonly List<IReadOnlyList<FrameCode>> Frames;
        /// <summary>fixture 번호와 실제 할당 번호의 입력 대응이다.</summary>
        public readonly Dictionary<int, int> Ids = [];

        /// <summary>판본별 TYPE 크기와 같은 수의 빈 프레임 표를 준비한다.</summary>
        public Scene(OriginalEdition edition, int capacity = 32768)
        {
            Pool = new(capacity, edition: edition);
            Types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
            Frames = Enumerable.Range(0, Types.Length).Select(_ => (IReadOnlyList<FrameCode>)Array.Empty<FrameCode>()).ToList();
        }

        /// <summary>명시 객체와 발자국 지도를 원본 입력 순서대로 쓴다. 뒤 객체가 앞 셀을 덮는다.</summary>
        public void Add(int id, float x, float y, int width = 1, int height = 1, uint flags1 = OriginalTypeRecord.Surface, uint flags2 = 4,
            char side = 'A', char variant = 'P', bool dead = false, bool buried = false)
        {
            int sid = Pool.Allocate(SidAllocation.Client); Ids.Add(id, sid); byte type = (byte)(70 + id);
            Types[type].Flags1 = flags1; Types[type].Flags2 = flags2; Types[type].FootX = width; Types[type].FootY = height;
            Frames[type] = new[] { new FrameCode(side, variant, 1, 0) };
            Span<byte> raw = Pool.RawSlot(sid); raw.Clear(); raw[10] = type; raw[11] = dead ? (byte)2 : (byte)0;
            BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
            raw[Pool.Edition == OriginalEdition.Patch1078 ? 40 : 35] = buried ? (byte)8 : (byte)0;
            // 발자국의 각 실제 지도 행을 준비한다.
            for (int row = (int)y - height + 1; row <= (int)y; row++)
            {
                // 지도 안에 있는 칸만 입력한다.
                for (int column = (int)x - width + 1; column <= (int)x; column++)
                    if (column >= 0 && row >= 0 && column < 256 && row < 256) Hash.Cell(0, column, row) = (ushort)sid;
            }
        }

        /// <summary>같은 물리와 독립 값 사본을 사용하는 제품 탐색기를 만든다.</summary>
        public RawSquidNeighbors Neighbors() => new(Pool, Hash, Spots, Types, Frames);

        /// <summary>실제 반환 번호를 fixture의 원문 번호로 되돌린다.</summary>
        public int Original(int sid) => Ids.Single(pair => pair.Value == sid).Key;
    }

    /// <summary>쉼표 목록의 순서/중복을 보존하고 원본 빈 표기를 빈 배열로 읽는다.</summary>
    private static int[] IdList(string text) => text is "-" or "" ? [] : text.Split(',').Select(X86Fixture.Int).ToArray();

    /// <summary>현재 결과를 읽고 Next를 끝까지 호출한다. 무한 순회가 통과하지 않게 상한을 둔다.</summary>
    private static int[] Walk(Scene scene, RawSquidNeighborWalk walk)
    {
        var result = new List<int>();
        // 다음 후보는 실제 Next가 현재 지도에서 새로 선택한다.
        for (int sid = walk.Current; sid != 0; sid = walk.Next())
        {
            Assert.True(result.Count <= 128); result.Add(scene.Original(sid));
        }
        Assert.Equal(0, walk.Next()); return result.ToArray();
    }

    /// <summary>두 원본의 공통125장면을 판본별 필드·프레임 폭으로 재생해250결과를 확인한다.</summary>
    [Fact]
    public void ReplaysExistingNativeFootprintsAndFilters()
    {
        int observations = 0;
        // PATCH/CD 모두 원본과 같은 발자국 지도 입력을 사용한다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            // 기존 surface 자료 중 Neighbor의 순서/중복 결과만 선택한다.
            foreach (string[] row in X86Fixture.Read("surface").Where(row => row[0] == "Neighbor"))
            {
                var scene = new Scene(edition);
                // 원문 객체를 입력 순서대로 적재한다.
                foreach (string entry in row[1].Split(';'))
                {
                    string[] f = entry.Split(','); scene.Add(X86Fixture.Int(f[0]), X86Fixture.Int(f[1]), X86Fixture.Int(f[2]), X86Fixture.Int(f[3]), X86Fixture.Int(f[4]),
                        X86Fixture.UInt(f[5]), X86Fixture.UInt(f[6]), (char)X86Fixture.Int(f[7]), (char)X86Fixture.Int(f[8]), f[9] == "1", f[10] == "1");
                }
                // 명시 spot과 지도 덮어쓰기를 각각 원문 순서대로 적용한다.
                for (int column = 2; column <= 3; column++)
                {
                    if (row[column] == "-") continue;
                    // x/y/번호 또는 spot 비트의 원문 입력이다.
                    foreach (string entry in row[column].Split(';'))
                    {
                        string[] f = entry.Split(','); int x = X86Fixture.Int(f[0]), y = X86Fixture.Int(f[1]), value = X86Fixture.Int(f[2]);
                        if (column == 2) scene.Spots[y * 256 + x] = (byte)value;
                        else scene.Hash.Cell(0, x, y) = (ushort)(value == 0 ? 0 : scene.Ids[value]);
                    }
                }
                RawSquidNeighbors neighbors = scene.Neighbors();
                Assert.Equal(IdList(row[5]), Walk(scene, new(neighbors, scene.Ids[X86Fixture.Int(row[4])], NeighborFlags.SurfaceMap))); observations++;
            }
        }
        Assert.Equal(250, observations);
    }

    /// <summary>첫 반환 뒤 지도/후보/원천을 바꾸는 독립 원본32행을 전수 재생한다.</summary>
    [Fact]
    public void ReplaysNativeMutationDuringWalk()
    {
        int observations = 0;
        // 원천은 생성 시점, 나머지는 Next 시점이라는 원문 경계를 확인한다.
        X86Fixture.CheckRows(X86Fixture.Read("surface-neighbor-current"), row =>
        {
            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; var scene = new Scene(edition);
            (int X, int Y)[] positions = [(20, 20), (20, 19), (19, 20), (21, 20), (20, 21), (19, 20)];
            // 동일한 코드 여섯 표면을 원문 순서로 입력한다.
            for (int index = 0; index < positions.Length; index++) scene.Add(index + 1, positions[index].X, positions[index].Y);
            scene.Hash.Cell(0, 19, 20) = (ushort)scene.Ids[3]; RawSquidNeighbors neighbors = scene.Neighbors();
            var walk = new RawSquidNeighborWalk(neighbors, scene.Ids[1], X86Fixture.UInt(row[1])); Assert.Equal(2, scene.Original(walk.Current));
            uint action = X86Fixture.UInt(row[2]);
            if (action == 1) { scene.Hash.Cell(0, 19, 20) = (ushort)scene.Ids[6]; scene.Hash.Cell(0, 20, 19) = (ushort)scene.Ids[5]; }
            else if (action == 2) { scene.Pool.RawSlot(scene.Ids[3])[11] = 2; scene.Pool.RawSlot(scene.Ids[4])[edition == OriginalEdition.Patch1078 ? 40 : 35] = 1; }
            else if (action == 3) { BinaryPrimitives.WriteSingleLittleEndian(scene.Pool.RawSlot(scene.Ids[1])[14..], 80f); BinaryPrimitives.WriteSingleLittleEndian(scene.Pool.RawSlot(scene.Ids[1])[18..], 80f); }
            Assert.Equal(IdList(row[3]), Walk(scene, walk)); observations++; return true;
        });
        Assert.Equal(32, observations);
    }

    /// <summary>원본 경계 좌표의 보정 발자국16BYTE와 첫 이웃을 그대로 비교한다.</summary>
    [Fact]
    public void NativeBoundaryFootprintsAndFirstNeighbor()
    {
        X86Fixture.CheckRows(X86Fixture.Read("neighbor-boundary"), row =>
        {
            var scene = new Scene(OriginalEdition.Patch1078); scene.Add(1, BitConverter.UInt32BitsToSingle(X86Fixture.UInt(row[0])),
                BitConverter.UInt32BitsToSingle(X86Fixture.UInt(row[1])), X86Fixture.Int(row[2]), X86Fixture.Int(row[2]));
            RawSquidNeighbors neighbors = scene.Neighbors(); SquidFootprint footprint = neighbors.Footprint(scene.Ids[1]); byte[] raw = new byte[16];
            BinaryPrimitives.WriteSingleLittleEndian(raw, footprint.Left); BinaryPrimitives.WriteSingleLittleEndian(raw.AsSpan(4), footprint.Top);
            BinaryPrimitives.WriteSingleLittleEndian(raw.AsSpan(8), footprint.Right); BinaryPrimitives.WriteSingleLittleEndian(raw.AsSpan(12), footprint.Bottom);
            Assert.Equal(row[3], Convert.ToHexStringLower(raw)); Assert.Equal(X86Fixture.Int(row[4]), neighbors.First(scene.Ids[1])); return true;
        });
    }

    /// <summary>독립 TYPE/코드는 값 사본이며 명시 대여만 현재 TYPE를 읽고 빈 입력은 사본으로 복귀한다.</summary>
    [Fact]
    public void IndependentSnapshotsAndExplicitCurrentTypes()
    {
        var scene = new Scene(OriginalEdition.Patch1078); scene.Add(1, 20, 20); scene.Add(2, 21, 20); RawSquidNeighbors neighbors = scene.Neighbors();
        scene.Types[72].Flags1 = 0; scene.Frames[72] = [];
        Assert.Equal(scene.Ids[2], neighbors.First(scene.Ids[1])); Assert.Single(neighbors.Frames(72));
        neighbors.BindCurrentTypes(scene.Types); Assert.Equal(0, neighbors.First(scene.Ids[1])); scene.Types[72].Flags1 = OriginalTypeRecord.Surface;
        Assert.Equal(scene.Ids[2], neighbors.First(scene.Ids[1])); neighbors.BindCurrentTypes([]); Assert.Equal(scene.Ids[2], neighbors.First(scene.Ids[1]));
        Assert.Throws<ArgumentException>(() => neighbors.BindCurrentTypes(scene.Types.Take(187).ToArray()));
        var cd = new Scene(OriginalEdition.Cd1072); Assert.Throws<ArgumentException>(() => cd.Neighbors().BindCurrentTypes(scene.Types));
    }

    /// <summary>값 원천은 raw 원천 변경 뒤에도 유지하고 지연 코드는 실제 접합을 건너뛰는 분기까지 허용한다.</summary>
    [Fact]
    public void ValueSourceAndLazyFramePreserveCallerInformation()
    {
        var scene = new Scene(OriginalEdition.Patch1078); scene.Add(1, 20, 20); scene.Add(2, 21, 20); RawSquidNeighbors neighbors = scene.Neighbors();
        SquidNeighborSource source = neighbors.Source(scene.Ids[1]); BinaryPrimitives.WriteSingleLittleEndian(scene.Pool.RawSlot(scene.Ids[1])[14..], 80);
        // 원문은 자기 번호를 따로 제외하지 않으므로 이동한 원천도 현재 지도 후보로 반환한다.
        Assert.Equal(new[] { 1, 2 }, Walk(scene, new(neighbors, source, NeighborFlags.SurfaceMap)));
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawSquidNeighborWalk(neighbors, source with { Frame = uint.MaxValue }, 9));
        Assert.Equal(new[] { 1, 2 }, Walk(scene, new(neighbors, source with { Frame = uint.MaxValue }, 9, true)));
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawSquidNeighborWalk(neighbors, source with { Type = 0x10047 }, 9, true));
    }

    /// <summary>원본 signed short 캐시와 전체 DWORD 후보 비교를 보존해 상위 비트 후보는 서로 다른 셀에서 다시 반환된다.</summary>
    [Fact]
    public void SurfaceCacheKeepsSignedShortComparison()
    {
        var scene = new Scene(OriginalEdition.Patch1078, 50000); scene.Add(1, 20, 20); scene.Add(2, 21, 20);
        scene.Pool.RawSlot(scene.Ids[2]).CopyTo(scene.Pool.RawSlot(40000)); scene.Ids[2] = 40000;
        scene.Hash.Cell(0, 19, 20) = 40000; scene.Hash.Cell(0, 21, 20) = 40000;
        RawSquidNeighbors neighbors = scene.Neighbors(); Assert.Equal(new[] { 2, 2 }, Walk(scene, new(neighbors, scene.Ids[1], 9)));
    }

    /// <summary>후보 spot은 보정 발자국의 오른쪽 아래가 아닌 원본 raw0 좌표의 선형 주소를 읽는다.</summary>
    [Fact]
    public void CandidateSpotUsesRawCoordinateBeforeFootprintClamp()
    {
        var scene = new Scene(OriginalEdition.Patch1078); scene.Add(1, 1, 21); scene.Add(2, 0, 20);
        scene.Hash.Cell(0, 0, 21) = (ushort)scene.Ids[2]; scene.Spots[20 * 256 + 1] = 8; RawSquidNeighbors neighbors = scene.Neighbors();
        Assert.Equal(new[] { 2 }, Walk(scene, new(neighbors, scene.Ids[1], 9)));
        scene.Spots[20 * 256] = 8; Assert.Empty(Walk(scene, new(neighbors, scene.Ids[1], 9)));
    }
}
