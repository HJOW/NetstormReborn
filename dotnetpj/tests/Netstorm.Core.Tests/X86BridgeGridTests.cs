using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Tests;
using Input = Netstorm.Core.Tests.X86BridgeDecayTests.Input;

namespace Netstorm.Core.Tests;

/// <summary>
/// 게임이 실제로 쓰는 <see cref="BridgeGrid"/> 의 한 칸 붕괴 처리를 원본 x86 기대값(bridgedecay-x86.tsv 의 Cell 행)에 대조한다.
/// 격자는 섬을 칸 단위로 보고 수명 단어·프레임 번호 대신 수명 값·칸 상태를 가지므로, 격자가 표현할 수 있는 입력만 고른다:
/// 서버(싱글 플레이), 디버그 꺼짐, spot 지정 없음. 그래프 표면 수는 기대값의 레코드를 그대로 준다.
/// </summary>
public sealed class X86BridgeGridTests
{
    /// <summary>오브젝트 열의 종류 값: 0 = 다리</summary>
    private const int BridgeKind = 0;

    /// <summary>상태 바이트의 죽음 비트</summary>
    private const int DeadBit = 2;

    /// <summary>격자가 표현할 수 있는 Cell 행의 수 (전체 4,241개 가운데 서버·디버그 꺼짐·spot 지정 없음)</summary>
    private const int RepresentableRows = 2692;

    /// <summary>그 가운데 여러 칸 섬이 들어 있는 행의 수</summary>
    private const int MultiCellRows = 180;

    /// <summary>격자가 표현할 수 있는 Cell 행인지: 서버, 디버그 갱신·유지 꺼짐, spot 지정 없음</summary>
    private static bool Representable(string[] row) => row[4] == "1" && row[5] == "0" && row[6] == "0" && row[3] == "-";

    /// <summary>모든 오브젝트가 한 칸짜리인지</summary>
    private static bool CellSized(string[] row) => X86BridgeDecayTests.Inputs(row[1]).All(input => input.Width == 1 && input.Height == 1);

    /// <summary>
    /// 기대값 한 행을 격자로 재현해 결과가 같은지 본다: 없어진 칸과 순서, 금 간 칸과 순서, 칸마다 남은 수명·프레임.
    /// 이동체 확인(C)과 금 가는 소리(S) 효과는 격자가 내지 않으므로 비교하지 않는다.
    /// </summary>
    private static bool Replay(string[] row, TypeFrameTable frames)
    {
        Input[] inputs = X86BridgeDecayTests.Inputs(row[1]);
        Dictionary<int, short> records = X86BridgeDecayTests.Records(row[2]);
        var island = new HashSet<(int X, int Y)>();
        var graphs = new Dictionary<(int X, int Y), int>();
        // 섬은 발자국의 모든 칸을 섬 칸으로 넣는다 (기준점은 오른쪽 아래 칸)
        foreach (Input input in inputs.Where(input => input.Kind != BridgeKind))
        {
            // 발자국의 행
            for (int y = input.Y - input.Height + 1; y <= input.Y; y++)
            {
                // 그 행의 칸
                for (int x = input.X - input.Width + 1; x <= input.X; x++)
                {
                    island.Add((x, y));
                }
            }
        }
        var grid = new BridgeGrid((x, y) => island.Contains((x, y)), graphSurfaces: cell => records[graphs[(cell.X, cell.Y)]],
            frames: frames);
        var cells = new Dictionary<int, BridgeCellState>();
        var conditions = new Dictionary<int, BridgeCondition>();
        // 다리는 입력 순서대로 저장 프레임에서 되살리고 수명을 단어의 비트에서 옮긴다
        foreach (Input input in inputs.Where(input => input.Kind == BridgeKind))
        {
            BridgeCellState cell = grid.AddStored(frames, input.Frame, input.X, input.Y, 1);
            cell.TimeLeft = (input.Word & BridgeDecayRules.LifeMask) >> BridgeDecayRules.LifeShift;
            cells.Add(input.Id, cell);
            conditions.Add(input.Id, cell.Condition);
            graphs.Add((input.X, input.Y), input.Graph);
        }
        BridgeDecayResult result = grid.ScanCell(cells[X86Fixture.Int(row[7])]);
        string[] events = row[8] == "-" ? [] : row[8].Split(' ');
        int[] destroyed = events.Where(text => text.StartsWith("D:")).Select(text => X86Fixture.Int(text.Split(':')[1])).ToArray();
        int[] redrawn = events.Where(text => text.StartsWith("R:")).Select(text => X86Fixture.Int(text.Split(':')[1])).ToArray();
        if (!result.Removed.Select(cell => cells.First(pair => ReferenceEquals(pair.Value, cell)).Key).SequenceEqual(destroyed)
            || !result.Cracked.Select(cell => cells.First(pair => ReferenceEquals(pair.Value, cell)).Key).SequenceEqual(redrawn))
        {
            return false;
        }
        // after 열: 번호, 단어, 프레임, state
        foreach (long[] after in X86BridgeDecayTests.Tuples(row[9]))
        {
            if (!cells.TryGetValue((int)after[0], out BridgeCellState? cell))
            {
                continue;
            }
            int id = (int)after[0];
            bool dead = (after[3] & DeadBit) != 0;
            // 상태가 그대로면 입력 프레임, 바뀌었으면 그 상태의 프레임 번호와 비교한다
            int frame = cell.Condition == conditions[id] ? inputs.First(input => input.Id == id).Frame
                : BridgeFrames.Find(frames, cell.Cell, cell.Condition);
            if (dead != !ReferenceEquals(grid.At(cell.X, cell.Y), cell) || frame != after[2]
                || cell.TimeLeft != (after[1] & BridgeDecayRules.LifeMask) >> BridgeDecayRules.LifeShift)
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>
    /// 격자의 한 칸 처리가 원본과 같다 (2,692개). 여러 칸 섬이 든 180개도 같다: 직사각형 섬 하나는 한 칸짜리 다리와
    /// 한 면에서만 닿으므로 섬을 칸 단위로 보아도 이웃 수가 달라지지 않는다. 남은 근사는 "어느 칸에 어떤 표면 오브젝트가 있는가"다.
    /// </summary>
    [Fact]
    public void ScanCell_MatchesOriginalX86()
    {
        TypeFrameTable frames = X86BridgeDecayTests.Frames("0");
        string[][] rows = X86Fixture.Read("bridgedecay").Where(row => row[0] == "Cell" && Representable(row)).ToArray();
        Assert.Equal(RepresentableRows, rows.Length);
        Assert.Equal(MultiCellRows, rows.Count(row => !CellSized(row)));
        X86Fixture.CheckRows(rows, row => Replay(row, frames));
    }

    /// <summary>
    /// 접합 칸 고리(원본이 끝없이 재귀하는 입력)에 붙은 열린 판자를 스캔해도 아무 칸도 바뀌지 않는다.
    /// 고리 밖의 다른 구동자는 평소대로 처리된다.
    /// </summary>
    [Fact]
    public void ScanCell_LeavesJunctionRingUntouched()
    {
        var grid = new BridgeGrid((_, _) => false, graphSurfaces: _ => BridgeGrid.MinSurfaceGraphSize);
        TypeFrameTable frames = X86BridgeDecayTests.Frames("0");
        // 2×2 접합 고리: C(동·남·서) G(남·서) / I(북·동) H(북·서), 그리고 고리 왼쪽의 열린 끝 판자 K
        (char Side, int X, int Y)[] ring = [('C', 100, 100), ('G', 101, 100), ('I', 100, 101), ('H', 101, 101), ('K', 99, 100)];
        BridgeCellState[] cells = ring.Select(cell =>
            grid.AddStored(frames, frames.Find(cell.Side, TypeFrameTable.DefaultVariant, 1), cell.X, cell.Y, 1)).ToArray();
        // 고리의 칸마다 수명 3 을 준다
        foreach (BridgeCellState cell in cells)
        {
            cell.TimeLeft = 3;
        }
        BridgeDecayResult result = grid.ScanCell(cells[^1]);
        Assert.Empty(result.Cracked);
        Assert.Empty(result.Removed);
        Assert.All(cells, cell => Assert.Equal(3, cell.TimeLeft));
        Assert.Equal(6, grid.OpenDirection(cells[^1]));
    }

    /// <summary>열린 방향: 세 갈래는 두 번째, 판자는 첫 번째 열린 방향이고 사방 칸은 −1 이다.</summary>
    [Fact]
    public void OpenDirection_FollowsOriginalLetterRules()
    {
        // x ≤ 9 가 섬이다
        var grid = new BridgeGrid((x, _) => x <= 9);
        TypeFrameTable frames = X86BridgeDecayTests.Frames("0");
        // 섬 → K(10,5) → E(11,5: 북·동·서) → 위로 J(11,4). 따로 떨어진 사방 칸 A(30,30)
        BridgeCellState plank = grid.AddStored(frames, frames.Find('K', TypeFrameTable.DefaultVariant, 1), 10, 5, 1);
        BridgeCellState fork = grid.AddStored(frames, frames.Find('E', TypeFrameTable.DefaultVariant, 1), 11, 5, 1);
        BridgeCellState tail = grid.AddStored(frames, frames.Find('J', TypeFrameTable.DefaultVariant, 1), 11, 4, 1);
        BridgeCellState cross = grid.AddStored(frames, frames.Find('A', TypeFrameTable.DefaultVariant, 1), 30, 30, 1);
        // 판자는 섬과 갈래에 모두 이어져 열린 쪽이 없다
        Assert.Equal(-1, grid.OpenDirection(plank));
        // 갈래 E 는 북·서가 이어지고 동쪽만 열려 있다. 세 갈래는 열린 쪽이 둘이어야 하므로 −1 이다
        Assert.Equal(-1, grid.OpenDirection(fork));
        // 위 판자는 남쪽이 갈래와 이어지고 북쪽(0)이 열려 있다
        Assert.Equal(0, grid.OpenDirection(tail));
        // 위 판자가 없어지면 갈래의 북·동이 열려 두 번째인 동(2)을 돌려준다
        grid.WeakenAround(11, 4);
        grid.WeakenAround(11, 4);
        Assert.Null(grid.At(11, 4));
        Assert.Equal(2, grid.OpenDirection(fork));
        Assert.Equal(-1, grid.OpenDirection(cross));
    }
}
