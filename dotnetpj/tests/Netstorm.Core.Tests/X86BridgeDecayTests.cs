using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 다리 한 칸 붕괴 처리·수명 감소·프레임 전환·destroy 재정의·스캔 커서를 원본 x86 기대값에 대조한다.
/// 기대값은 cpppj 가 세 원본 실행 파일의 기계어를 격리 실행해 만든 bridgedecay-x86.tsv 이며 열 구성은
/// cpppj/tests/BridgeDecayTests.cpp 와 같다 (docs/exe/cpp-bridgedecay-reconstruction.md).
/// </summary>
public sealed class X86BridgeDecayTests
{
    /// <summary>기대값의 빈 목록 표기</summary>
    private const string Empty = "-";

    /// <summary>패치판 10.78 행의 판본 이름. 그 밖(CD·10.37)은 비교 자료다</summary>
    private const string PatchEdition = "originals";

    /// <summary>오브젝트 열의 종류 값: 0 = 다리, 1 = 섬</summary>
    private const int BridgeKind = 0;

    /// <summary>상태 바이트의 죽음 비트</summary>
    private const int DeadBit = 2;

    /// <summary>추가 상태 바이트의 매장 비트</summary>
    private const int BuriedBit = 8;

    /// <summary>기대값의 입력 오브젝트 하나 (13개 정수)</summary>
    /// <param name="Id">번호</param>
    /// <param name="X">기준점 x</param>
    /// <param name="Y">기준점 y</param>
    /// <param name="Width">발자국 가로</param>
    /// <param name="Height">발자국 세로</param>
    /// <param name="Flags1">타입 플래그 1</param>
    /// <param name="Flags2">타입 플래그 2</param>
    /// <param name="Kind">0 다리, 1 섬</param>
    /// <param name="Frame">프레임 번호</param>
    /// <param name="State">상태 바이트</param>
    /// <param name="Extra">추가 상태 바이트</param>
    /// <param name="Graph">그래프 번호</param>
    /// <param name="Word">상태 단어</param>
    internal readonly record struct Input(int Id, int X, int Y, int Width, int Height, uint Flags1, uint Flags2, int Kind,
        int Frame, byte State, byte Extra, int Graph, ushort Word);

    /// <summary>기대값 전체 행 (한 번만 읽는다)</summary>
    private static readonly Lazy<string[][]> Rows = new(() => X86Fixture.Read("bridgedecay"));

    /// <summary>"a,b,c;d,e,f" 형식의 정수 묶음을 읽는다. '-' 는 빈 목록이다.</summary>
    internal static long[][] Tuples(string text) => text == Empty || text.Length == 0 ? []
        : text.Split(';').Select(entry => entry.Split(',').Select(value => long.Parse(value, CultureInfo.InvariantCulture)).ToArray()).ToArray();

    /// <summary>오브젝트 열을 이름 있는 필드로 옮긴다.</summary>
    internal static Input[] Inputs(string text) => Tuples(text).Select(v => new Input((int)v[0], (int)v[1], (int)v[2], (int)v[3],
        (int)v[4], (uint)v[5], (uint)v[6], (int)v[7], (int)v[8], (byte)v[9], (byte)v[10], (int)v[11], (ushort)v[12])).ToArray();

    /// <summary>그래프 번호 → 표면 수 (레코드는 번호, 표면 수, 사용 여부)</summary>
    internal static Dictionary<int, short> Records(string text) => Tuples(text).ToDictionary(v => (int)v[0], v => (short)v[1]);

    /// <summary>기대값의 Frames 행(표 번호별 실제·변형 bridge 프레임 코드)을 읽는다.</summary>
    internal static TypeFrameTable Frames(string table)
    {
        string[] row = Rows.Value.First(row => row[0] == "Frames" && row[1] == table);
        byte[] bytes = X86Fixture.Bytes(row[2]);
        var codes = new List<FrameCode>();
        // 방향·변형·번호·플래그의 네 바이트 순서다
        for (int i = 0; i + 4 <= bytes.Length; i += 4)
        {
            codes.Add(new FrameCode((char)bytes[i], (char)bytes[i + 1], bytes[i + 2], (FrameCodeFlags)bytes[i + 3]));
        }
        return new TypeFrameTable(codes);
    }

    /// <summary>다리 오브젝트의 가변 상태를 만든다. 섬은 방문 목록에 들어가지 않으므로 상태가 없다.</summary>
    private static List<BridgeDecayState> States(Input[] inputs, Dictionary<int, short> records) => inputs
        .Where(input => input.Kind == BridgeKind).Select(input => new BridgeDecayState
        {
            Id = input.Id,
            Word = input.Word,
            Frame = input.Frame,
            GraphSurfaces = records.GetValueOrDefault(input.Graph),
            GraphValid = records.ContainsKey(input.Graph),
            Extra = input.Extra,
            Dead = (input.State & DeadBit) != 0,
        }).ToList();

    /// <summary>효과를 원본 표기로 바꾼다: D:번호:플래그, R:번호, C:칸x:칸y, S:x비트:y비트.</summary>
    private static string EventText(IEnumerable<BridgeDecayEvent> events, Input[] inputs)
    {
        var parts = new List<string>();
        // 효과를 호출 순서대로 적는다
        foreach (BridgeDecayEvent item in events)
        {
            Input input = inputs.First(candidate => candidate.Id == item.Id);
            parts.Add(item.Kind switch
            {
                BridgeDecayEventKind.Destroy => $"D:{item.Id}:{item.Flags}",
                BridgeDecayEventKind.Redraw => $"R:{item.Id}",
                BridgeDecayEventKind.Carriers => $"C:{input.X}:{input.Y}",
                _ => $"S:{BitConverter.SingleToUInt32Bits(input.X)}:{BitConverter.SingleToUInt32Bits(input.Y)}",
            });
        }
        return parts.Count == 0 ? Empty : string.Join(' ', parts);
    }

    /// <summary>결과 상태를 기대값의 after 열 표기(번호,단어,프레임,state)로 바꾼다. 섬은 입력 값 그대로다.</summary>
    private static string AfterText(Input[] inputs, IReadOnlyList<BridgeDecayState> states) => string.Join(';', inputs.Select(input =>
    {
        BridgeDecayState? found = states.FirstOrDefault(state => state.Id == input.Id);
        return found == null ? $"{input.Id},{input.Word},{input.Frame},{input.State}"
            : $"{input.Id},{found.Word},{found.Frame},{(found.Dead ? input.State | DeadBit : input.State)}";
    }));

    /// <summary>표면 탐색기 입력을 만든다. 다리는 실제 프레임 코드, 섬은 사방 연결 코드 하나를 쓴다.</summary>
    internal static SurfaceFinder Finder(Input[] inputs, TypeFrameTable frames, string spotText)
    {
        ushort[] map = new ushort[SurfaceFinder.WorldCells * SurfaceFinder.WorldCells];
        byte[] spots = new byte[map.Length];
        var objects = new List<SurfaceObject>();
        // 오브젝트마다 발자국 전체에 번호를 적는다 (원본에 넣은 입력 규약과 같다)
        foreach (Input input in inputs)
        {
            FrameCode code = input.Kind == BridgeKind ? frames.Codes[input.Frame] : new FrameCode('A', 'A', 1, FrameCodeFlags.None);
            objects.Add(new SurfaceObject(input.Id, input.X, input.Y, input.Width, input.Height, input.Flags1, input.Flags2, code,
                (input.State & DeadBit) != 0, (input.Extra & BuriedBit) != 0));
            // 발자국의 행
            for (int y = input.Y - input.Height + 1; y <= input.Y; y++)
            {
                // 그 행의 칸
                for (int x = input.X - input.Width + 1; x <= input.X; x++)
                {
                    map[y * SurfaceFinder.WorldCells + x] = (ushort)input.Id;
                }
            }
        }
        // 지정한 spot 비트만 넣는다 (x, y, 값)
        foreach (long[] v in Tuples(spotText))
        {
            spots[v[1] * SurfaceFinder.WorldCells + v[0]] = (byte)v[2];
        }
        return new SurfaceFinder(objects, map, spots);
    }

    /// <summary>정수 열에 저장한 배정밀도 비트를 복원한다.</summary>
    private static double Bits(string text) => BitConverter.UInt64BitsToDouble(ulong.Parse(text, CultureInfo.InvariantCulture));

    /// <summary>스캔 한 칸 처리(원본 004227e0)의 효과 순서와 결과 상태 4,241개.</summary>
    [Fact]
    public void DecayCell_MatchesOriginalX86()
    {
        TypeFrameTable frames = Frames("0");
        string[][] rows = Rows.Value.Where(row => row[0] == "Cell").ToArray();
        Assert.Equal(4241, rows.Length);
        var outcomes = new Dictionary<BridgeDecayOutcome, int>();
        X86Fixture.CheckRows(rows, row =>
        {
            Input[] inputs = Inputs(row[1]);
            var mode = new BridgeDecayMode { Server = row[4] == "1", DebugRedraw = row[5] == "1", DebugKeep = row[6] == "1" };
            BridgeDecayCell result = BridgeDecayRules.DecayCell(Finder(inputs, frames, row[3]), frames,
                States(inputs, Records(row[2])), X86Fixture.Int(row[7]), mode);
            outcomes[result.Outcome] = outcomes.GetValueOrDefault(result.Outcome) + 1;
            return result.Outcome is not (BridgeDecayOutcome.Incomplete or BridgeDecayOutcome.Faulted)
                && EventText(result.Events, inputs) == row[8] && AfterText(inputs, result.States) == row[9];
        });
        // 기대값이 주요 분기를 모두 지났는지 확인한다
        foreach (BridgeDecayOutcome outcome in new[] { BridgeDecayOutcome.SmallGraph, BridgeDecayOutcome.Hard,
            BridgeDecayOutcome.Isolated, BridgeDecayOutcome.Closed, BridgeDecayOutcome.Blocked, BridgeDecayOutcome.Decayed })
        {
            Assert.True(outcomes.GetValueOrDefault(outcome) > 0, $"분기 {outcome} 를 지난 입력이 없습니다");
        }
    }

    /// <summary>
    /// 수명 감소 전체(제거·비트 갱신·금 간 프레임·디버그 갱신·이동체 확인) 3,264개.
    /// 결과 수명이 7 을 넘는 400개는 원본이 4비트로 그대로 쓰는 입력이며 새 코드는 쓰기 전에 거부한다.
    /// </summary>
    [Fact]
    public void ApplyLife_MatchesOriginalX86()
    {
        TypeFrameTable frames = Frames("0");
        string[][] rows = Rows.Value.Where(row => row[0] == "Life").ToArray();
        Assert.Equal(3264, rows.Length);
        int guarded = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Input[] inputs = Inputs(row[1]);
            List<BridgeDecayState> states = States(inputs, Records(row[2]));
            var mode = new BridgeDecayMode { Server = row[3] == "1", DebugRedraw = row[4] == "1", DebugKeep = row[5] == "1" };
            int reduction = X86Fixture.Int(row[6]);
            var events = new List<BridgeDecayEvent>();
            if (((inputs[0].Word & BridgeDecayRules.LifeMask) >> BridgeDecayRules.LifeShift) - reduction > BridgeDecayRules.MaximumLife)
            {
                guarded++;
                BridgeDecayState before = states[0] with { };
                try
                {
                    BridgeDecayRules.ApplyLife(frames, states[0], reduction, mode, events);
                    return false;
                }
                catch (ArgumentOutOfRangeException)
                {
                    return states[0] == before && events.Count == 0;
                }
            }
            return BridgeDecayRules.ApplyLife(frames, states[0], reduction, mode, events)
                && EventText(events, inputs) == row[7] && AfterText(inputs, states) == row[8];
        });
        Assert.Equal(400, guarded);
    }

    /// <summary>다리 destroy 재정의(원본 004220f0)가 기본 destroy 로 넘어가는 조건 1,152개.</summary>
    [Fact]
    public void DestroyOverride_MatchesOriginalX86()
    {
        TypeFrameTable frames = Frames("0");
        string[][] rows = Rows.Value.Where(row => row[0] == "Destroy").ToArray();
        Assert.Equal(1152, rows.Length);
        int blocked = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Input[] inputs = Inputs(row[1]);
            BridgeDecayState state = States(inputs, Records(row[2]))[0];
            var mode = new BridgeDecayMode { Editor = row[3] == "1", Authority = row[4] == "1", DebugKeep = row[5] == "1" };
            bool proceeds = BridgeDecayRules.DestroyProceeds(mode, state.Extra, state.GraphSurfaces, frames.Codes[state.Frame].Flags);
            blocked += proceeds ? 0 : 1;
            return proceeds ? row[7] == $"D:{inputs[0].Id}:{row[6]}" : row[7] == Empty;
        });
        Assert.True(blocked > 0);
    }

    /// <summary>
    /// 금 간·보통·약화·복구 전환 1,914개 (Crack 348, Normal 174, Weaken 696, Restore 696).
    /// 표 0 은 실제 bridge.type, 1·2 는 일부 프레임을 뺀 표다.
    /// </summary>
    [Fact]
    public void FrameTransitions_MatchOriginalX86()
    {
        var tables = new Dictionary<string, TypeFrameTable>();
        string[][] rows = Rows.Value.Where(row => row[0] is "Crack" or "Normal" or "Weaken" or "Restore").ToArray();
        Assert.Equal(348, rows.Count(row => row[0] == "Crack"));
        Assert.Equal(174, rows.Count(row => row[0] == "Normal"));
        Assert.Equal(696, rows.Count(row => row[0] == "Weaken"));
        Assert.Equal(696, rows.Count(row => row[0] == "Restore"));
        // 기대값이 쓰는 화면 갱신 표기 (시험 오브젝트 번호 6100)
        const string redraw = "R:6100";
        X86Fixture.CheckRows(rows, row =>
        {
            if (!tables.TryGetValue(row[1], out TypeFrameTable? frames))
            {
                tables.Add(row[1], frames = Frames(row[1]));
            }
            int frame = X86Fixture.Int(row[2]);
            if (row[0] == "Crack")
            {
                int found = BridgeDecayRules.CrackedFrame(frames, frame, row[3] == "1");
                return (found != -1) == (row[4] == "1") && (found == -1 ? frame : found) == X86Fixture.Int(row[5])
                    && row[6] == (found == -1 ? Empty : redraw);
            }
            if (row[0] == "Normal")
            {
                int found = BridgeDecayRules.NormalFrame(frames, frame);
                return (found == -1 ? frame : found) == X86Fixture.Int(row[5]) && row[6] == (found == -1 ? Empty : redraw);
            }
            ushort word = (ushort)X86Fixture.UInt(row[3]);
            BridgeFrameChange change = row[0] == "Weaken" ? BridgeDecayRules.Weaken(frames, word, frame)
                : BridgeDecayRules.Restore(frames, word, frame);
            return (row[0] != "Weaken" || change.Changed == (row[4] == "1")) && change.Word == X86Fixture.UInt(row[5])
                && change.Frame == X86Fixture.Int(row[6]) && row[7] == (change.Changed ? redraw : Empty);
        });
    }

    /// <summary>
    /// 스캔 초기화 12개와 프레임별 커서 전진·주기 되돌림·처리 대상 번호 3,941개.
    /// C# 의 기준 범위는 10.78(15000~23001)이다. CD·10.37 행은 그 판본의 범위를 행에서 읽어 같은 진행 규칙만 대조한다.
    /// </summary>
    [Fact]
    public void ScanCursor_MatchesOriginalX86()
    {
        string[][] rows = Rows.Value.Where(row => row[0] is "Init" or "ScanScene" or "Scan").ToArray();
        Assert.Equal(12, rows.Count(row => row[0] == "Init"));
        Assert.Equal(3941, rows.Count(row => row[0] == "Scan"));
        var state = new BridgeDecayScanState();
        var objects = new Dictionary<int, Input>();
        int visitedTotal = 0;
        // 행은 Init → ScanScene → Scan 들의 순서로 저장돼 있다
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "Init")
            {
                double now = Bits(row[3]);
                state = row[1] == PatchEdition ? BridgeDecayScan.Reset(now) : new BridgeDecayScanState
                {
                    First = X86Fixture.Int(row[4]),
                    Last = X86Fixture.Int(row[5]),
                    Cursor = X86Fixture.Int(row[4]),
                    Next = (double)BridgeDecayScan.Period + now,
                };
                return state.First == X86Fixture.Int(row[4]) && state.Last == X86Fixture.Int(row[5])
                    && state.Cursor == X86Fixture.Int(row[6]) && BitConverter.DoubleToUInt64Bits(state.Next) == ulong.Parse(row[7]);
            }
            if (row[0] == "ScanScene")
            {
                objects = Inputs(row[3]).ToDictionary(input => input.Id);
                return true;
            }
            var mode = new BridgeDecayMode { Editor = row[6] == "1", Authority = row[7] == "1" };
            // 각 행은 실행 전 커서·다음 시각을 가진 독립 입력이다
            state.Cursor = X86Fixture.Int(row[8]);
            state.Next = Bits(row[9]);
            (int begin, int end) = BridgeDecayScan.Advance(state, Bits(row[3]), Bits(row[4]), X86Fixture.Int(row[5]) > 0, mode);
            var visited = new List<int>();
            // 구간 안의 번호를 오름차순으로 보며 처리 대상만 모은다
            for (int id = begin; id < end; id++)
            {
                if (objects.TryGetValue(id, out Input found) && BridgeDecayScan.Eligible(found.State, found.Flags2, found.Extra))
                {
                    visited.Add(id);
                }
            }
            visitedTotal += visited.Count;
            return (visited.Count == 0 ? Empty : string.Join(',', visited)) == row[10] && state.Cursor == X86Fixture.Int(row[11])
                && BitConverter.DoubleToUInt64Bits(state.Next) == ulong.Parse(row[12]);
        });
        Assert.True(visitedTotal > 0);
    }

    /// <summary>원본 기본 프레임 간격(14ms)에서 스캔은 프레임마다 11개 번호를 전진하고 주기 끝 프레임에 나머지를 모두 훑는다.</summary>
    [Fact]
    public void Scan_SpreadsRangeOverPeriod()
    {
        BridgeDecayScanState state = BridgeDecayScan.Reset(0.0);
        Assert.Equal((15000, 23001, 10.0), (state.First, state.Last, state.Next));
        double now = 0.0;
        int covered = 0;
        // 14ms 프레임으로 한 주기를 지난다
        while (now < 10.0)
        {
            now += 0.014;
            (int begin, int end) = BridgeDecayScan.Advance(state, now, 0.014, false);
            if (now < 10.0)
            {
                Assert.Equal(11, end - begin);
            }
            covered += end - begin;
        }
        Assert.Equal(8002, covered);
        Assert.Equal((15000, BridgeDecayScan.Period + now), (state.Cursor, state.Next));
        // 정지·편집기·권한 없음은 커서와 다음 시각을 건드리지 않는다
        BridgeDecayScan.Advance(state, now + 50.0, 0.014, true);
        BridgeDecayScan.Advance(state, now + 50.0, 0.014, false, new BridgeDecayMode { Editor = true });
        BridgeDecayScan.Advance(state, now + 50.0, 0.014, false, new BridgeDecayMode { Authority = false });
        Assert.Equal((15000, BridgeDecayScan.Period + now), (state.Cursor, state.Next));
        Assert.True(BridgeDecayScan.Eligible(0, TypeFlagBits.Bridge, 8));
        Assert.False(BridgeDecayScan.Eligible(0, TypeFlagBits.Bridge, 1));
        Assert.False(BridgeDecayScan.Eligible(4, TypeFlagBits.Bridge, 0));
        Assert.False(BridgeDecayScan.Eligible(0, TypeFlagBits.Island, 0));
    }

    /// <summary>원본이 정상 반환하지 않는 입력(접합 고리, 그래프 254)은 상태를 바꾸지 않고 구분해 알린다.</summary>
    [Fact]
    public void DecayCell_GuardsRingAndInvalidGraph()
    {
        TypeFrameTable frames = Frames("0");
        // 방향 글자의 번호 1 프레임 (입력 구성용)
        int FrameOf(char side) => frames.Find(side, TypeFrameTable.DefaultVariant, 1);
        // 2×2 접합 고리: C(동·남·서) G(남·서) / I(북·동) H(북·서), 그리고 고리 왼쪽에 붙은 열린 끝 판자 K
        (char Side, int X, int Y)[] ring = [('C', 100, 100), ('G', 101, 100), ('I', 100, 101), ('H', 101, 101), ('K', 99, 100)];
        Input[] inputs = ring.Select((cell, index) => new Input(200 + index, cell.X, cell.Y, 1, 1, TypeFlagBits.Surface,
            TypeFlagBits.Bridge, BridgeKind, FrameOf(cell.Side), 0, 0, 1, 3 << 3)).ToArray();
        SurfaceFinder finder = Finder(inputs, frames, Empty);
        List<BridgeDecayState> states = States(inputs, new Dictionary<int, short> { [1] = 9 });
        BridgeDecayCell ringResult = BridgeDecayRules.DecayCell(finder, frames, states, 204);
        Assert.Equal(BridgeDecayOutcome.Incomplete, ringResult.Outcome);
        Assert.Empty(ringResult.Events);
        Assert.Equal(AfterText(inputs, states), AfterText(inputs, ringResult.States));
        // 시작 칸의 그래프가 254 면 아무것도 하지 않는다
        states[^1] = states[^1] with { GraphValid = false };
        BridgeDecayCell invalid = BridgeDecayRules.DecayCell(finder, frames, states, 204);
        Assert.Equal(BridgeDecayOutcome.InvalidGraph, invalid.Outcome);
        Assert.Empty(invalid.Events);
        // 없앨 칸의 그래프가 254 면 그 지점에서 멈추고 죽음 비트를 켜지 않는다
        var events = new List<BridgeDecayEvent>();
        var lost = new BridgeDecayState { Id = 300, Word = 1 << 3, Frame = FrameOf('K'), GraphValid = false };
        Assert.False(BridgeDecayRules.ApplyLife(frames, lost, 1, BridgeDecayMode.SinglePlayer, events));
        Assert.False(lost.Dead);
        Assert.Empty(events);
        // 서버가 아니면 수명 0 에서도 destroy 재정의에 닿지 않는다
        Assert.True(BridgeDecayRules.ApplyLife(frames, lost, 1, new BridgeDecayMode { Server = false }, events));
        Assert.Equal(0, lost.Word);
        // 입력 누락·범위 밖 프레임은 예외다
        Assert.Throws<KeyNotFoundException>(() => BridgeDecayRules.DecayCell(finder, frames, [], 204));
        Assert.ThrowsAny<ArgumentException>(() => BridgeDecayRules.CrackedFrame(frames, frames.Codes.Count, false));
        Assert.ThrowsAny<ArgumentException>(() => BridgeDecayRules.NormalFrame(frames, -1));
    }
}
