using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 표면 연결·이웃 탐색·붕괴 방문·열린 방향·수명 감소를 원본 x86 기대값에 대조한다 (계획 3-1~3-5).
/// 기대값은 cpppj 가 원본 기계어를 격리 실행해 만든 surface-x86.tsv·bridge-x86.tsv 이며 열 구성은
/// cpppj/tests/SurfaceTests.cpp·BridgeTests.cpp 와 같다. 원본 실행 파일 없이 돈다.
/// </summary>
public sealed class X86SurfaceTests
{
    /// <summary>기대값의 빈 목록 표기</summary>
    private const string Empty = "-";

    /// <summary>Open 행에서 "칸마다 번호가 다른 시험 지도"를 뜻하는 후보 값</summary>
    private const int MixedMapMarker = 65536;

    /// <summary>시험 지도가 돌려 쓰는 표면 번호 (x + 3y 를 4 로 나눈 나머지 순서)</summary>
    private static readonly ushort[] MixedMapIds = [0, 7, 8, 9];

    /// <summary>쉼표로 나눈 번호 목록을 원본 순서대로 읽는다. 중복도 지우지 않는다.</summary>
    private static int[] Ids(string text) =>
        text.Length == 0 || text == Empty ? [] : text.Split(',').Select(X86Fixture.Int).ToArray();

    /// <summary>기대값 한 행의 표면 장면: 오브젝트 목록, 번호 지도, spot 지도</summary>
    private sealed class Scene
    {
        /// <summary>표면 오브젝트</summary>
        public List<SurfaceObject> Objects { get; } = [];

        /// <summary>256×256 표면 번호 지도</summary>
        public ushort[] Map { get; } = new ushort[SurfaceFinder.WorldCells * SurfaceFinder.WorldCells];

        /// <summary>256×256 spot 지도</summary>
        public byte[] Spots { get; } = new byte[SurfaceFinder.WorldCells * SurfaceFinder.WorldCells];

        /// <summary>오브젝트를 넣고 발자국 전체에 번호를 적는다 (기준점은 오른쪽 아래 칸). 결과를 계산하지는 않는다.</summary>
        public void Add(SurfaceObject item)
        {
            Objects.Add(item);
            // 발자국의 행을 위에서 아래로 채운다
            for (int y = item.Y - item.Height + 1; y <= item.Y; y++)
            {
                // 한 행에서 그 표면이 차지하는 칸
                for (int x = item.X - item.Width + 1; x <= item.X; x++)
                {
                    Map[y * SurfaceFinder.WorldCells + x] = (ushort)item.Id;
                }
            }
        }

        /// <summary>사방으로 이어지는 1×1 다리(또는 지정한 종류)를 넣는다. 직접 검사용 입력이다.</summary>
        public void Add(int id, int x, int y, int width = 1, int height = 1, uint flags2 = TypeFlagBits.Bridge) =>
            Add(new SurfaceObject(id, x, y, width, height, TypeFlagBits.Surface, flags2, new FrameCode('A', 'A', 1, FrameCodeFlags.None)));

        /// <summary>탐색기를 만든다</summary>
        public SurfaceFinder Finder() => new(Objects, Map, Spots);

        /// <summary>기대값 행의 오브젝트(11개 정수)·spot 변경·지도 변경 열을 읽는다.</summary>
        public static Scene Parse(string[] row)
        {
            var scene = new Scene();
            // 오브젝트마다 번호, 기준점, 발자국, 플래그 1·2, 방향·변형 글자, 죽음, 매장
            foreach (string entry in row[1].Split(';'))
            {
                int[] f = entry.Split(',').Select(X86Fixture.Int).ToArray();
                scene.Add(new SurfaceObject(f[0], f[1], f[2], f[3], f[4], (uint)f[5], (uint)f[6],
                    new FrameCode((char)f[7], (char)f[8], 1, FrameCodeFlags.None), f[9] == 1, f[10] == 1));
            }
            // 2열은 spot 변경, 3열은 지도 변경이다. 각 항목은 x, y, 값
            for (int column = 2; column <= 3; column++)
            {
                if (row[column] == Empty)
                {
                    continue;
                }
                foreach (string entry in row[column].Split(';'))
                {
                    int[] f = entry.Split(',').Select(X86Fixture.Int).ToArray();
                    int index = f[1] * SurfaceFinder.WorldCells + f[0];
                    if (column == 2)
                    {
                        scene.Spots[index] = (byte)f[2];
                    }
                    else
                    {
                        scene.Map[index] = (ushort)f[2];
                    }
                }
            }
            return scene;
        }
    }

    /// <summary>3-1: A~P 모든 쌍·8방향, 별도 축 글자, 타입 마스크와 보정 순서 3,016개.</summary>
    [Fact]
    public void Connect_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("surface").Where(row => row[0] == "Connect").ToArray();
        Assert.Equal(3016, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var first = new FrameCode((char)X86Fixture.Int(row[1]), (char)X86Fixture.Int(row[2]), 1, FrameCodeFlags.None);
            var second = new FrameCode((char)X86Fixture.Int(row[4]), (char)X86Fixture.Int(row[5]), 1, FrameCodeFlags.None);
            return BridgeSurfaceRules.Connects(first, X86Fixture.UInt(row[3]), second, X86Fixture.UInt(row[6]),
                X86Fixture.Int(row[7])) == (row[8] == "1");
        });
    }

    /// <summary>3-2: 지도 가장자리·큰 발자국·중복·죽음/표면/안쪽 필터와 이웃 순서 125개.</summary>
    [Fact]
    public void Neighbors_MatchOriginalOrderAndFilters()
    {
        string[][] rows = X86Fixture.Read("surface").Where(row => row[0] == "Neighbor").ToArray();
        Assert.Equal(125, rows.Length);
        X86Fixture.CheckRows(rows, row =>
            Scene.Parse(row).Finder().Neighbors(X86Fixture.Int(row[4])).SequenceEqual(Ids(row[5])));
    }

    /// <summary>3-5: 고리 없는 표면에서 원본 재귀가 정상 복귀한 방문 목록·두 플래그·용량 처리 309개.</summary>
    [Fact]
    public void CollectDecay_MatchesOriginalTraversal()
    {
        string[][] rows = X86Fixture.Read("surface").Where(row => row[0] == "Collect").ToArray();
        Assert.Equal(309, rows.Length);
        int shortCases = 0;
        int progressCases = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            BridgeDecayWalk walk = BridgeSurfaceRules.CollectDecay(Scene.Parse(row).Finder(), X86Fixture.Int(row[4]),
                X86Fixture.Int(row[5]));
            shortCases += walk.ShortJunction ? 1 : 0;
            progressCases += walk.CanDecay ? 1 : 0;
            return walk.Complete && walk.ShortJunction == (row[6] == "1") && walk.CanDecay == (row[7] == "1")
                && walk.Visited.SequenceEqual(Ids(row[8]));
        });
        Assert.True(shortCases > 0 && progressCases > 0);
    }

    /// <summary>3-3: A~P·8방향·지도 가장자리·소수 좌표·번호 0·이웃 포함 여부 5,760개 (한 방향 검사와 열린 방향).</summary>
    [Fact]
    public void OpenDirection_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("bridge").Where(row => row[0] == "Open").ToArray();
        Assert.Equal(5760, rows.Length);
        var maps = new Dictionary<int, ushort[]>();
        X86Fixture.CheckRows(rows, row =>
        {
            int candidate = X86Fixture.Int(row[5]);
            if (!maps.TryGetValue(candidate, out ushort[]? surface))
            {
                surface = new ushort[SurfaceFinder.WorldCells * SurfaceFinder.WorldCells];
                // 후보 값마다 지도를 한 번만 만든다. 표시 값이면 칸마다 번호를 바꾸고 아니면 같은 번호로 채운다
                for (int index = 0; index < surface.Length; index++)
                {
                    int x = index % SurfaceFinder.WorldCells;
                    int y = index / SurfaceFinder.WorldCells;
                    surface[index] = candidate == MixedMapMarker ? MixedMapIds[(x + 3 * y) % 4] : (ushort)candidate;
                }
                maps.Add(candidate, surface);
            }
            int[] members = Ids(row[6]);
            char side = row[1][0];
            float px = X86Fixture.FloatBits(row[3]);
            float py = X86Fixture.FloatBits(row[4]);
            return BridgeSurfaceRules.IsOpen(side, X86Fixture.Int(row[2]), px, py, surface, members) == (row[7] == "1")
                && BridgeSurfaceRules.OpenDirection(side, px, py, surface, members) == X86Fixture.Int(row[8]);
        });
    }

    /// <summary>3-4: 수명 감소의 접두 구간 480개 — 저장 단어(다른 비트 보존), 제거 여부, 판자 제거 플래그.</summary>
    [Fact]
    public void ReduceLife_MatchesOriginalPrefix()
    {
        string[][] rows = X86Fixture.Read("bridge").Where(row => row[0] == "Life").ToArray();
        Assert.Equal(480, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            BridgeLifeChange change = BridgeDecayRules.ReduceLife((ushort)X86Fixture.UInt(row[1]), X86Fixture.Int(row[2]),
                row[3] == "1", row[4][0]);
            return change.Word == X86Fixture.UInt(row[5]) && change.Remove == (row[6] == "1")
                && change.RemovalFlags == X86Fixture.UInt(row[7]);
        });
    }

    /// <summary>같은 큰 섬을 여러 칸에서 만나도 한 번만 돌려주고, 만든 뒤 입력이 바뀌어도 스냅샷은 그대로다.</summary>
    [Fact]
    public void Neighbors_ReturnMultiCellObjectOnceAndKeepSnapshot()
    {
        var scene = new Scene();
        scene.Add(1, 20, 20, 3, 3, TypeFlagBits.Island);
        scene.Add(2, 20, 17, 3, 3, TypeFlagBits.Island);
        SurfaceFinder finder = scene.Finder();
        Array.Clear(scene.Map);
        Array.Fill(scene.Spots, SurfaceFinder.InteriorBit);
        Assert.Equal(new[] { 2 }, finder.Neighbors(1));
        Assert.Equal(3, finder.Object(2).Width);
    }

    /// <summary>
    /// 원본이 끝없이 재귀하는 접합 칸 고리에서는 방문이 "불완전"으로 멈추고 붕괴가 진행되지 않는다 (cpppj 와 같은 안전 처리).
    /// </summary>
    [Fact]
    public void CollectDecay_StopsOnJunctionRing()
    {
        var scene = new Scene();
        scene.Add(1, 20, 20);
        scene.Add(2, 21, 20);
        scene.Add(3, 21, 21);
        scene.Add(4, 20, 21);
        SurfaceFinder finder = scene.Finder();
        IReadOnlyList<int> before = finder.Neighbors(1);
        BridgeDecayWalk walk = BridgeSurfaceRules.CollectDecay(finder, 1);
        Assert.False(walk.Complete);
        Assert.False(walk.CanDecay);
        Assert.Equal(before, finder.Neighbors(1));
    }

    /// <summary>잘못된 지도 크기·중복 번호·없는 번호·범위 밖 글자·방향·용량·수명은 조용히 넘기지 않고 예외로 알린다.</summary>
    [Fact]
    public void InvalidInputs_AreRejected()
    {
        var scene = new Scene();
        scene.Add(1, 20, 20);
        Assert.ThrowsAny<ArgumentException>(() => new SurfaceFinder(scene.Objects, [], scene.Spots));
        Assert.ThrowsAny<ArgumentException>(() => new SurfaceFinder([.. scene.Objects, scene.Objects[0]], scene.Map, scene.Spots));
        scene.Map[19 * SurfaceFinder.WorldCells + 20] = 7;
        SurfaceFinder finder = scene.Finder();
        Assert.Throws<KeyNotFoundException>(() => finder.Neighbors(1));
        Assert.Throws<KeyNotFoundException>(() => finder.Object(0));
        Assert.ThrowsAny<ArgumentException>(() => BridgeSurfaceRules.CollectDecay(finder, 1, 0));
        var plain = new FrameCode('A', 'P', 1, FrameCodeFlags.None);
        Assert.ThrowsAny<ArgumentException>(() => BridgeSurfaceRules.Connects(new FrameCode('Q', 'P', 1, FrameCodeFlags.None),
            TypeFlagBits.Bridge, plain, TypeFlagBits.Bridge, 0));
        Assert.ThrowsAny<ArgumentException>(() => BridgeSurfaceRules.Connects(plain, TypeFlagBits.Bridge, plain, TypeFlagBits.Bridge, 8));
        ushort[] surface = new ushort[SurfaceFinder.WorldCells * SurfaceFinder.WorldCells];
        Assert.ThrowsAny<ArgumentException>(() => BridgeSurfaceRules.IsOpen('J', 8, 0, 0, surface, []));
        Assert.ThrowsAny<ArgumentException>(() => BridgeSurfaceRules.IsOpen('J', 0, 0, 0, [], []));
        Assert.ThrowsAny<ArgumentException>(() => BridgeSurfaceRules.IsOpen('J', 0, float.PositiveInfinity, 0, surface, []));
        Assert.ThrowsAny<ArgumentException>(() => BridgeSurfaceRules.OpenDirection('Q', 0, 0, surface, []));
        Assert.ThrowsAny<ArgumentException>(() => BridgeDecayRules.ReduceLife(8 << 3, 1, false, 'J'));
        Assert.ThrowsAny<ArgumentException>(() => BridgeDecayRules.ReduceLife(7 << 3, -1, false, 'J'));
    }
}
