// 원본 양방향 탐색/편향 절삭/현재 경계·표식 효과를 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 가짜 좌표 탐색을 검사하는 클래스다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.
// 근거: cpppj tests/AiDryGeometryTests.cpp의 12건 전부다.
public sealed class RawAiDryGeometryTests
{
    /// <summary>같은 실제 풀과 명시 지도/표식 효과 입력을 준비한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private sealed class DryGeometryScene
    {
        /// <summary>원본 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>현재 전역이다.</summary>
        public AiBuildSearchBounds Bounds = new(10, 10, 246, 246);

        /// <summary>현재 디버그 종류다.</summary>
        public uint Debug = 99;

        /// <summary>현재 표식 TYPE다.</summary>
        public uint Marker = 104;

        /// <summary>실제 WORD 지도다.</summary>
        public readonly ushort[] Surface = new ushort[65536];

        /// <summary>실제 방문 순서다.</summary>
        public readonly List<(int X, int Y)> Cells = [];

        /// <summary>선택된 원문 수명 효과다.</summary>
        public AiDryGeometryHooks Hooks = new();

        /// <summary>선택된 원문 수명 효과를 관찰한다.</summary>
        public readonly List<RawPathPoint> Points = [];

        /// <summary>선택된 원문 수명 효과를 관찰한다.</summary>
        public readonly List<Sid> Created = [];

        /// <summary>선택된 원문 수명 효과를 관찰한다.</summary>
        public readonly List<Sid> Destroyed = [];

        /// <summary>선택된 원문 수명 효과를 관찰한다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>다음에 할당할 SID 번호다.</summary>
        private ushort _nextSid = 5;

        /// <summary>각 실제 효과 뒤 명시 현재 상태 변경이다.</summary>
        public Action<int>? DuringPop;

        /// <summary>각 실제 효과 뒤 명시 현재 상태 변경이다.</summary>
        public Action<Sid>? DuringDestroy;

        /// <summary>외부 효과의 결과를 주입하지 않고 각 실제 슬롯/현재 지도를 읽고 쓴다.</summary>
        public DryGeometryScene()
        {
            Hooks.Surface = (x, y) =>
            {
                Cells.Add((x, y));
                return Surface[y * 256 + x];
            };
            Hooks.Create = (type, flags) =>
            {
                Assert.Equal(2u, flags);
                Sid sid = Alloc();
                Pool.RawSlot(sid.Value)[10] = (byte)type;
                Created.Add(sid);
                Trace.Add("create");
                return sid;
            };
            Hooks.Word = (sid, word) =>
            {
                Assert.Equal(93, word);
                BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(sid.Value).Slice(12, 2), word);
                Trace.Add("word");
            };
            Hooks.Pop = (sid, point, flags) =>
            {
                Assert.Equal(0u, flags);
                Assert.Equal(93, BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(sid.Value).Slice(12, 2)));
                BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(sid.Value).Slice(14, 4), BitConverter.SingleToUInt32Bits(point.X));
                BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(sid.Value).Slice(18, 4), BitConverter.SingleToUInt32Bits(point.Y));
                Points.Add(point);
                Trace.Add("pop");
                DuringPop?.Invoke(Points.Count);
            };
            Hooks.Destroy = (sid, flags) =>
            {
                Assert.Equal(0u, flags);
                Destroyed.Add(sid);
                Trace.Add("destroy");
                DuringDestroy?.Invoke(sid);
            };
        }

        /// <summary>빈 실제 슬롯을 확보한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
        /// <returns>확보한 SID다.</returns>
        private Sid Alloc()
        {
            ushort sid = _nextSid++;
            Span<byte> raw = Pool.RawSlot(sid);
            raw.Fill(0);
            raw[RawSidPool.StateOffset] &= unchecked((byte)~RawSidPool.FreeBit);
            return new Sid(sid);
        }

        /// <summary>같은 현재 풀/전역/선택 효과를 빌린다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
        /// <returns>검사할 가짜 좌표 탐색 몸체다.</returns>
        public RawAiDryGeometry Make() => new(Pool, new AiDryGeometryGlobals
        {
            Bounds = () => Bounds,
            Debug = () => Debug,
            MarkerType = () => Marker,
        }, Hooks);

        /// <summary>현재 풀에 TYPE/WORD가 다른 실제 표식 후보를 준비한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
        /// <param name="type">쓸 TYPE 번호다.</param>
        /// <param name="word">쓸 WORD다.</param>
        /// <returns>준비한 SID다.</returns>
        public Sid Object(uint type, ushort word)
        {
            Sid sid = Alloc();
            Span<byte> raw = Pool.RawSlot(sid.Value);
            raw[10] = (byte)type;
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(12, 2), word);
            return sid;
        }
    }

    /// <summary>원본 두 float의 차이/제곱합은 중간 float 저장 없이 계산한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void DistanceFloatOperands()
    {
        Assert.Equal(5, AiPointDistanceHelper.Distance(new RawPathPoint(0, 0), new RawPathPoint(3, 4)));
        Assert.Equal(5, AiPointDistanceHelper.Distance(new RawPathPoint(-1, -1), new RawPathPoint(-1, 4)));
        float huge = float.MaxValue;
        Assert.True(double.IsFinite(AiPointDistanceHelper.Distance(new RawPathPoint(huge, 0), new RawPathPoint(-huge, 0))));
        Assert.True(double.IsNaN(AiPointDistanceHelper.Distance(new RawPathPoint(float.NaN, 0), new RawPathPoint(0, 0))));
    }

    /// <summary>동률은 왼쪽 우선이며 두 방향을 각 반복에서 모두 방문한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void HorizontalVerticalTie()
    {
        var input = new DryGeometryScene();
        RawPathPoint point = input.Make().Candidate(new RawPathPoint(30, 40), new RawPathPoint(34, 40));
        Assert.Equal(32f, point.X);
        Assert.Equal(44f, point.Y);
        Assert.Equal(8, input.Cells.Count);
        Assert.Empty(input.Created);
        Assert.Empty(input.Destroyed);
        Assert.Equal((32, 41), input.Cells[0]);
        Assert.Equal((32, 39), input.Cells[1]);
        Assert.Equal((32, 44), input.Cells[6]);
        Assert.Equal((32, 36), input.Cells[7]);
        var vertical = new DryGeometryScene();
        RawPathPoint other = vertical.Make().Candidate(new RawPathPoint(40, 40), new RawPathPoint(40, 44));
        Assert.Equal(36f, other.X);
        Assert.Equal(42f, other.Y);
    }

    /// <summary>ceil이나 사제용 0.99999가 아닌 float 0.9999 편향 합 뒤 ftol을 사용한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void RoundsCellAfterBias()
    {
        var input = new DryGeometryScene();
        input.Make().Candidate(new RawPathPoint(30.00005f, 40.00005f), new RawPathPoint(34.00005f, 40.00005f));
        Assert.Equal((32, 41), input.Cells[0]);
        var next = new DryGeometryScene();
        next.Make().Candidate(new RawPathPoint(30.0002f, 40.0002f), new RawPathPoint(34.0002f, 40.0002f));
        Assert.Equal((33, 42), next.Cells[0]);
    }

    /// <summary>점유 칸은 각 방향 연속 횟수를 0으로 만들며 오른쪽만 먼저 4에 도달할 수 있다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void ResetsBothRunsAndChoosesRight()
    {
        var input = new DryGeometryScene();
        input.Surface[42 * 256 + 32] = 5;
        RawPathPoint point = input.Make().Candidate(new RawPathPoint(30, 40), new RawPathPoint(34, 40));
        Assert.Equal(32f, point.X);
        Assert.Equal(36f, point.Y);
        Assert.Equal(8, input.Cells.Count);
        var both = new DryGeometryScene();
        both.Surface[42 * 256 + 32] = 5;
        both.Surface[39 * 256 + 32] = 6;
        RawPathPoint other = both.Make().Candidate(new RawPathPoint(30, 40), new RawPathPoint(34, 40));
        Assert.Equal(32f, other.X);
        Assert.Equal(35f, other.Y);
        Assert.Equal(10, both.Cells.Count);
    }

    /// <summary>현재 점이 경계 끝값에 있으면 이동하고 이동 뒤 경계 밖 한 칸도 원본처럼 조회한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void IncludesBoundsBeforeMoving()
    {
        var input = new DryGeometryScene();
        RawPathPoint point = input.Make().Candidate(new RawPathPoint(30, 10), new RawPathPoint(34, 10));
        Assert.Equal(32f, point.X);
        Assert.Equal(14f, point.Y);
        Assert.Equal(5, input.Cells.Count);
        Assert.Equal((32, 9), input.Cells[1]);
    }

    /// <summary>물리 256 범위 밖은 빈칸으로 계산하고 현재 지도 효과를 요청하지 않는다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void OutOfGridAvoidsSurface()
    {
        var input = new DryGeometryScene();
        input.Bounds = new AiBuildSearchBounds(-500, -500, 500, 500);
        input.Hooks.Surface = null;
        RawPathPoint point = input.Make().Candidate(new RawPathPoint(256, 40), new RawPathPoint(260, 40));
        Assert.Equal(258f, point.X);
        Assert.Equal(44f, point.Y);
        Assert.Empty(input.Cells);
    }

    /// <summary>디버그 표식은 초기 두 점→선분 네 점→수직 양쪽 여덟 점 순서다. 현재 종류도 재조회한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void DebugTraceOrderAndCurrentKind()
    {
        var input = new DryGeometryScene();
        Sid old = input.Object(104, 93);
        input.Debug = 29;
        input.Make().Candidate(new RawPathPoint(30, 40), new RawPathPoint(34, 40));
        Assert.Equal([old], input.Destroyed);
        Assert.Equal(14, input.Points.Count);
        Assert.Equal(14, input.Created.Count);
        Assert.Equal("destroy", input.Trace[0]);
        Assert.Equal(30f, input.Points[0].X);
        Assert.Equal(34f, input.Points[1].X);
        Assert.Equal(31f, input.Points[2].X);
        Assert.Equal(34f, input.Points[5].X);
        Assert.Equal(41f, input.Points[6].Y);
        Assert.Equal(39f, input.Points[7].Y);
        Assert.Equal(44f, input.Points[12].Y);
        Assert.Equal(36f, input.Points[13].Y);
        var changed = new DryGeometryScene();
        changed.Debug = 29;
        changed.DuringPop = count =>
        {
            if (count == 1)
            {
                changed.Debug = 99;
                changed.Marker = 105;
            }
        };
        changed.Make().Candidate(new RawPathPoint(30, 40), new RawPathPoint(34, 40));
        Assert.Equal(2, changed.Points.Count);
        Assert.Equal(104, changed.Pool.Slot(changed.Created[0].Value)[10]);
        Assert.Equal(105, changed.Pool.Slot(changed.Created[1].Value)[10]);
    }

    /// <summary>원본 선택 위치에서 누락 효과를 거부하고 생성/kind 쓰기 등 이미 수행한 상태는 보존한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void MarkPartialWrites()
    {
        var input = new DryGeometryScene();
        input.Hooks.Word = null;
        Assert.ThrowsAny<Exception>(() => input.Make().Mark(new RawPathPoint(1, 2)));
        Assert.Single(input.Created);
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(input.Pool.Slot(input.Created[0].Value).Slice(12, 2)));
        Assert.Empty(input.Points);
        var after = new DryGeometryScene();
        after.Hooks.Pop = null;
        Assert.ThrowsAny<Exception>(() => after.Make().Mark(new RawPathPoint(1, 2)));
        Assert.Single(after.Created);
        Assert.Equal(93, BinaryPrimitives.ReadUInt16LittleEndian(after.Pool.Slot(after.Created[0].Value).Slice(12, 2)));
        Assert.Empty(after.Points);
    }

    /// <summary>TYPE BYTE와 WORD 93이 모두 일치하는 것만 지우며 dead는 방문하고 free는 건너뛴다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void ClearMatchesWordTypeAndDead()
    {
        var input = new DryGeometryScene();
        Sid first = input.Object(104, 93);
        input.Object(104, 92);
        input.Object(105, 93);
        Sid dead = input.Object(104, 93);
        input.Pool.RawSlot(dead.Value)[11] |= 2;
        Sid free = input.Object(104, 93);
        input.Pool.RawSlot(free.Value)[11] |= 1;
        input.Make().ClearMarkers();
        Assert.Equal([first, dead], input.Destroyed);
    }

    /// <summary>삭제 전에 다음 SID를 저장하므로 삭제 효과에서 다음이 free가 되어도 저장 SID를 방문한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void ClearCachesNextBeforeDelete()
    {
        var input = new DryGeometryScene();
        Sid first = input.Object(104, 93);
        Sid second = input.Object(104, 93);
        input.DuringDestroy = sid =>
        {
            if (sid == first)
            {
                input.Pool.RawSlot(second.Value)[11] |= 1;
            }
        };
        input.Make().ClearMarkers();
        Assert.Equal([first, second], input.Destroyed);
    }

    /// <summary>각 삭제 뒤 현재 전체 DWORD TYPE를 읽고 상위 비트가 있으면 BYTE를 잘라 일치시키지 않는다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void ClearUsesCurrentMarkerType()
    {
        var input = new DryGeometryScene();
        Sid first = input.Object(104, 93);
        Sid second = input.Object(105, 93);
        input.Object(104, 93);
        input.DuringDestroy = sid =>
        {
            if (sid == first)
            {
                input.Marker = 105;
            }
        };
        input.Make().ClearMarkers();
        Assert.Equal([first, second], input.Destroyed);
        var upper = new DryGeometryScene();
        upper.Object(104, 93);
        upper.Marker = 0x168;
        upper.Make().ClearMarkers();
        Assert.Empty(upper.Destroyed);
    }

    /// <summary>처음 도달하는 지도 경계만 요구하며 미연결이면 후보를 위조하지 않고 거부한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingSelectedSurface()
    {
        var input = new DryGeometryScene();
        input.Hooks.Surface = null;
        Assert.ThrowsAny<Exception>(() => input.Make().Candidate(new RawPathPoint(30, 40), new RawPathPoint(34, 40)));
        Assert.Empty(input.Created);
    }
}
