using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 표면 그래프의 등록·flood·할당·반납·감소를 원본 x86 기대값(graph-x86.tsv)에 대조한다 (계획 4-1).
/// 열 구성은 cpppj/tests/GraphTests.cpp 와 같다. 기대값은 그래프 표 전체(255 레코드 × 16비트 3개)와
/// flood 스택 전체(4096 × 32비트)의 Adler-32, 그리고 표면마다의 그래프 번호·상태다.
/// 삭제 준비·전체 재구성은 원본 슬롯 바이트를 입력으로 삼는 기대값이라 직접 대조하지 못하고 cpppj 의 단위 검사를 옮겼다.
/// </summary>
public sealed class X86GraphTests
{
    /// <summary>기대값의 빈 목록 표기</summary>
    private const string Empty = "-";

    /// <summary>Adler-32 의 나머지 연산 값</summary>
    private const uint AdlerPrime = 65521;

    /// <summary>합성 표면이 쓰는 타입 플래그 2 (섬·다리가 아닌 표면)</summary>
    private const uint PlainSurface = 8;

    /// <summary>"a,b;c,d" 형식의 정수 묶음을 읽는다.</summary>
    private static int[][] Tuples(string text) => text == Empty ? []
        : text.Split(';').Select(entry => entry.Split(',').Select(value => int.Parse(value, CultureInfo.InvariantCulture)).ToArray()).ToArray();

    /// <summary>Python zlib 와 같은 Adler-32.</summary>
    private static uint Adler(IEnumerable<byte> bytes)
    {
        uint a = 1;
        uint b = 0;
        // 모든 바이트를 누적한다
        foreach (byte value in bytes)
        {
            a = (a + value) % AdlerPrime;
            b = (b + a) % AdlerPrime;
        }
        return (b << 16) | a;
    }

    /// <summary>그래프 표 전체를 원본 폭(리틀 엔디언 16비트 3개)으로 늘어놓아 체크섬을 낸다. Reserved 와 쓰지 않는 칸도 포함한다.</summary>
    private static uint RecordsHash(SurfaceGraph graph) => Adler(graph.Records.SelectMany(record =>
        new[] { record.Surfaces, record.InUse, record.Reserved }.SelectMany(value => BitConverter.GetBytes(value))));

    /// <summary>flood 스택 전체를 원본 폭(32비트)으로 늘어놓아 체크섬을 낸다. 쓰고 난 흔적도 포함한다.</summary>
    private static uint StackHash(SurfaceGraph graph) => Adler(graph.FloodStack.SelectMany(BitConverter.GetBytes));

    /// <summary>발자국 전체에 번호를 적은 지도. 뒤의 오브젝트가 같은 칸을 덮어쓴다 (기대값의 준비 순서).</summary>
    private static ushort[] Map(IEnumerable<SurfaceObject> objects)
    {
        ushort[] map = new ushort[SurfaceFinder.WorldCells * SurfaceFinder.WorldCells];
        // 오브젝트마다 오른쪽 아래 기준 사각형을 채운다
        foreach (SurfaceObject item in objects)
        {
            // 발자국의 행
            for (int y = item.Y - item.Height + 1; y <= item.Y; y++)
            {
                // 그 행의 칸
                for (int x = item.X - item.Width + 1; x <= item.X; x++)
                {
                    map[y * SurfaceFinder.WorldCells + x] = (ushort)item.Id;
                }
            }
        }
        return map;
    }

    /// <summary>표 hex 를 레코드로 읽는다 (레코드마다 리틀 엔디언 16비트 3개).</summary>
    private static GraphRecord[] Records(string hex)
    {
        byte[] bytes = Convert.FromHexString(hex);
        var records = new GraphRecord[SurfaceGraph.TableSize];
        // 6바이트씩 읽는다
        for (int i = 0; i < records.Length; i++)
        {
            records[i] = new GraphRecord
            {
                Surfaces = BitConverter.ToInt16(bytes, i * 6),
                InUse = BitConverter.ToInt16(bytes, i * 6 + 2),
                Reserved = BitConverter.ToInt16(bytes, i * 6 + 4),
            };
        }
        return records;
    }

    /// <summary>합성 한 칸(또는 가로로 긴) 표면</summary>
    private static SurfaceObject Node(int id, int x, int y, int width = 1, bool dead = false) => new(id, x, y, width, 1,
        TypeFlagBits.Surface, PlainSurface, new FrameCode('A', 'P', 1, FrameCodeFlags.None), dead);

    /// <summary>표면 목록으로 탐색기를 만든다 (spot 은 지정한 칸만).</summary>
    private static SurfaceFinder Finder(IReadOnlyList<SurfaceObject> objects, params (int X, int Y, byte Value)[] spots)
    {
        byte[] bytes = new byte[SurfaceFinder.WorldCells * SurfaceFinder.WorldCells];
        // 지정한 spot 비트만 넣는다
        foreach ((int x, int y, byte value) in spots)
        {
            bytes[y * SurfaceFinder.WorldCells + x] = value;
        }
        return new SurfaceFinder(objects, Map(objects), bytes);
    }

    /// <summary>
    /// 두 판본·두 x87 정밀도의 준비 512개와 그 뒤의 연산 2,560개를 원본 실행 순서대로 재생한다.
    /// 오브젝트 열은 번호, 기준점, 발자국, 플래그 1·2, 방향·변형 글자, 죽음, 매장이다. 그래프는 소속 정보의 상태 바이트와
    /// 오브젝트의 죽음이 같아야 하므로(생성자 검사) 죽음은 소속 정보의 dead 비트에서 읽는다.
    /// </summary>
    [Fact]
    public void Operations_MatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("graph");
        Assert.Equal(4, rows.Count(row => row[0] == "Begin"));
        Assert.Equal(512, rows.Count(row => row[0] == "Setup"));
        string[][] steps = rows.Where(row => row[0] != "Begin").ToArray();
        Assert.Equal(2560, steps.Count(row => row[0] != "Setup"));
        SurfaceGraph? graph = null;
        X86Fixture.CheckRows(steps, row =>
        {
            if (row[0] == "Setup")
            {
                GraphMembership[] members = Tuples(row[3]).Select(v => new GraphMembership(v[0], (byte)v[1], (byte)v[2])).ToArray();
                SurfaceObject[] objects = Tuples(row[1]).Select(v => new SurfaceObject(v[0], v[1], v[2], v[3], v[4], (uint)v[5], (uint)v[6],
                    new FrameCode((char)v[7], (char)v[8], 1, FrameCodeFlags.None),
                    (members.First(member => member.Id == v[0]).State & 2) != 0)).ToArray();
                byte[] spots = new byte[SurfaceFinder.WorldCells * SurfaceFinder.WorldCells];
                // 지정한 안쪽 비트만 준비한다 (x, y, 값)
                foreach (int[] v in Tuples(row[2]))
                {
                    spots[v[1] * SurfaceFinder.WorldCells + v[0]] = (byte)v[2];
                }
                graph = new SurfaceGraph(new SurfaceFinder(objects, Map(objects), spots), members, Records(row[4]));
                return true;
            }
            int[] args = row[1] == Empty ? [] : Tuples(row[1])[0];
            bool result = true;
            switch (row[0])
            {
                case "Add":
                    graph!.Add(args[0]);
                    break;
                case "Flood":
                    result = graph!.Flood(args[0], (byte)args[1]) == X86Fixture.UInt(row[2]);
                    break;
                case "Allocate":
                    result = graph!.Allocate() == X86Fixture.Int(row[2]);
                    break;
                case "Free":
                    graph!.Free((byte)args[0]);
                    break;
                case "Remove":
                    graph!.Remove((byte)args[0]);
                    break;
                default:
                    return false;
            }
            return result && RecordsHash(graph) == X86Fixture.UInt(row[3]) && StackHash(graph) == X86Fixture.UInt(row[4])
                && Tuples(row[5]).All(v => graph.Number(v[0]) == v[1] && graph.State(v[0]) == v[2]);
        });
    }

    /// <summary>표면 수가 같으면 먼저 탐색한(위쪽) 그래프가 남고, 두 표면짜리 왼쪽 무리가 합쳐진다. Reserved 는 보존된다.</summary>
    [Fact]
    public void Add_TieKeepsFirstNeighborAndMergesLoser()
    {
        SurfaceObject[] objects = [Node(5, 20, 20), Node(6, 19, 20), Node(7, 21, 19), Node(8, 20, 19), Node(9, 18, 20)];
        GraphMembership[] members = [new(5), new(6, 1), new(7, 0), new(8, 0), new(9, 1)];
        var records = new GraphRecord[SurfaceGraph.TableSize];
        records[0] = new GraphRecord { Surfaces = 2, InUse = 1, Reserved = 71 };
        records[1] = new GraphRecord { Surfaces = 2, InUse = 1, Reserved = 83 };
        var graph = new SurfaceGraph(Finder(objects), members, records);
        graph.Add(5);
        Assert.Equal(1, graph.InUse);
        Assert.Equal(5, graph.Records[0].Surfaces);
        Assert.Equal((0, 0, 0), (graph.Number(5), graph.Number(6), graph.Number(9)));
        Assert.Equal((0, 71, 83), (graph.Records[1].Surfaces, graph.Records[0].Reserved, graph.Records[1].Reserved));
    }

    /// <summary>여러 칸 발자국도 표면 하나로 센다. 표면 수의 증가·감소는 16비트로 감긴다.</summary>
    [Fact]
    public void MultiCellCountsOnceAndWordWraps()
    {
        SurfaceObject[] objects = [Node(5, 30, 30, 3)];
        GraphMembership[] members = [new(5)];
        var graph = new SurfaceGraph(Finder(objects), members);
        graph.Add(5);
        Assert.Equal((1, 1), (graph.Records[0].Surfaces, graph.InUse));
        Assert.Equal((short)1, graph.Surfaces(5));
        graph.Remove(0);
        Assert.Equal((0, 0, 0), (graph.Records[0].Surfaces, graph.InUse, graph.Number(5)));
        var records = new GraphRecord[SurfaceGraph.TableSize];
        records[0] = new GraphRecord { Surfaces = short.MaxValue, InUse = 1, Reserved = 123 };
        var wrap = new SurfaceGraph(Finder(objects), members, records);
        Assert.Equal(1u, wrap.Flood(5, 0));
        Assert.Equal(short.MinValue, wrap.Records[0].Surfaces);
        Assert.Equal(1u, wrap.Flood(5, SurfaceGraph.Invalid));
        Assert.Equal((short.MaxValue, 123), (wrap.Records[0].Surfaces, wrap.Records[0].Reserved));
        Assert.Null(wrap.Surfaces(5));
    }

    /// <summary>소진·손상·소속 정보 누락은 표·스택·번호를 바꾸지 않고 예외로 알린다.</summary>
    [Fact]
    public void InvalidAndExhaustedInputs_RejectWithoutMutation()
    {
        SurfaceObject[] objects = [Node(5, 20, 20), Node(6, 21, 20)];
        GraphMembership[] members = [new(5)];
        var missing = new SurfaceGraph(Finder(objects), members);
        (uint records, uint stack) before = (RecordsHash(missing), StackHash(missing));
        Assert.Throws<KeyNotFoundException>(() => missing.Add(5));
        Assert.Equal(before, (RecordsHash(missing), StackHash(missing)));
        Assert.Equal(SurfaceGraph.Invalid, missing.Number(5));
        Assert.Throws<ArgumentOutOfRangeException>(() => missing.Flood(5, 251));
        Assert.Throws<ArgumentOutOfRangeException>(() => missing.Free(253));
        Assert.Throws<ArgumentOutOfRangeException>(() => missing.Remove(SurfaceGraph.Invalid));
        var table = new GraphRecord[SurfaceGraph.TableSize];
        // 정상 번호를 모두 사용 중으로 만든다
        for (int i = 0; i < SurfaceGraph.Count; i++)
        {
            table[i] = new GraphRecord { Surfaces = 1, InUse = 1, Reserved = 123 };
        }
        var full = new SurfaceGraph(Finder(objects), members, table);
        uint fullBefore = RecordsHash(full);
        Assert.Throws<InvalidOperationException>(() => full.Allocate());
        Assert.Equal((fullBefore, SurfaceGraph.Count), (RecordsHash(full), full.InUse));
    }

    /// <summary>삭제 준비: 마지막 이웃 무리는 원래 번호에 남는다. 잘못된 감소 수와 번호 소진은 아무것도 바꾸지 않는다.</summary>
    [Fact]
    public void Detach_KeepsLastComponentAndRejectsAtomically()
    {
        SurfaceObject[] objects = [Node(5, 20, 20, dead: true), Node(6, 19, 20), Node(7, 21, 20), Node(8, 18, 20)];
        GraphMembership[] members = [new(5, 0, 2), new(6, 0), new(7, 0), new(8, 0)];
        var records = new GraphRecord[SurfaceGraph.TableSize];
        records[0] = new GraphRecord { Surfaces = 4, InUse = 1, Reserved = 71 };
        records[1].Reserved = 83;
        int[] connections = [6, 7];
        var graph = new SurfaceGraph(Finder(objects), members, records);
        graph.Detach(5, connections);
        Assert.Equal((0, 2), (graph.Number(5), graph.State(5)));
        Assert.Equal((1, 1, 0), (graph.Number(6), graph.Number(8), graph.Number(7)));
        Assert.Equal((1, 2, 71, 83), (graph.Records[0].Surfaces, graph.Records[1].Surfaces, graph.Records[0].Reserved, graph.Records[1].Reserved));
        (uint, uint) hash = (RecordsHash(graph), StackHash(graph));
        Assert.Throws<ArgumentException>(() => graph.Detach(5, connections, false, 2));
        Assert.Equal(hash, (RecordsHash(graph), StackHash(graph)));
        // 첫 새 무리를 만들 번호가 없으면 일부만 flood 한 결과도 남기지 않는다
        for (int i = 0; i < SurfaceGraph.Count; i++)
        {
            records[i] = new GraphRecord { Surfaces = 4, InUse = 1, Reserved = 71 };
        }
        var full = new SurfaceGraph(Finder(objects), members, records);
        (uint, uint) fullBefore = (RecordsHash(full), StackHash(full));
        Assert.Throws<InvalidOperationException>(() => full.Detach(5, connections, true));
        Assert.Equal(fullBefore, (RecordsHash(full), StackHash(full)));
        Assert.Equal((0, 0, 0, 0), (full.Number(5), full.Number(6), full.Number(7), full.Number(8)));
    }

    /// <summary>
    /// 전체 재구성: 예약 0번과 임시 번호 때문에 한 번에 독립 무리 249개까지 확보한다. 250개는 소진으로 거부하며 아무것도 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void Rebuild_Assigns249ComponentsAndRollsBackOnExhaustion()
    {
        var objects = new List<SurfaceObject>();
        var members = new List<GraphMembership>();
        // 표면을 세 칸 간격으로 떨어뜨려 서로 이어지지 않게 한다
        for (int i = 0; i < 250; i++)
        {
            objects.Add(Node(5 + i, 20 + i % 25 * 3, 20 + i / 25 * 3));
            members.Add(new GraphMembership(5 + i));
        }
        var records = new GraphRecord[SurfaceGraph.TableSize];
        // 모든 정상 번호를 사용 중으로 만들고 Reserved 에 서로 다른 값을 둔다
        for (int i = 0; i < SurfaceGraph.Count; i++)
        {
            records[i] = new GraphRecord { Surfaces = 7, InUse = 1, Reserved = (short)(0x1234 + i) };
        }
        var tooMany = new SurfaceGraph(Finder(objects), members, records);
        (uint, uint) before = (RecordsHash(tooMany), StackHash(tooMany));
        Assert.Throws<InvalidOperationException>(() => tooMany.Rebuild(true));
        Assert.Throws<InvalidOperationException>(() => tooMany.AllocateWithRecovery());
        Assert.Equal(before, (RecordsHash(tooMany), StackHash(tooMany)));
        Assert.Equal((SurfaceGraph.Invalid, SurfaceGraph.Invalid), (tooMany.Number(5), tooMany.Number(254)));
        objects.RemoveAt(objects.Count - 1);
        members.RemoveAt(members.Count - 1);
        var graph = new SurfaceGraph(Finder(objects), members, records);
        Assert.Equal(1, graph.AllocateWithRecovery());
        Assert.Equal(SurfaceGraph.Count, graph.InUse);
        Assert.Equal((0x1201, 0), (graph.Records[0].Reserved, graph.Records[1].Surfaces));
        // 번호 오름차순으로 무리 번호 2~250 을 받고 크기는 1 이다
        for (int i = 0; i < 249; i++)
        {
            Assert.Equal((i + 2, 1, records[i + 2].Reserved), (graph.Number(5 + i), graph.Records[i + 2].Surfaces, graph.Records[i + 2].Reserved));
        }
    }

    /// <summary>기준점이 안쪽인 표면은 재구성의 첫 수집에서 빠지고 그 번호는 그대로 남는다.</summary>
    [Fact]
    public void Rebuild_SkipsInteriorOriginAndRejectsMissingMembership()
    {
        SurfaceObject[] objects = [Node(5, 20, 20, 3), Node(6, 40, 40), Node(7, 21, 20)];
        GraphMembership[] members = [new(5, 3), new(6)];
        SurfaceFinder finder = Finder(objects, (20, 20, SurfaceFinder.InteriorBit));
        Assert.True(finder.OriginInterior(5));
        var graph = new SurfaceGraph(finder, members);
        graph.Rebuild(true);
        Assert.Equal((3, 2), (graph.Number(5), graph.Number(6)));
        // 안쪽 비트가 없으면 표면 5 의 이웃 7 이 소속 정보에 없어 flood 가 거부된다
        var missing = new SurfaceGraph(Finder(objects), members);
        (uint, uint) before = (RecordsHash(missing), StackHash(missing));
        Assert.Throws<KeyNotFoundException>(() => missing.Rebuild(true));
        Assert.Equal(before, (RecordsHash(missing), StackHash(missing)));
        Assert.Equal((3, SurfaceGraph.Invalid), (missing.Number(5), missing.Number(6)));
    }
}
