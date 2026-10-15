using System.Globalization;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 10.78 의 공통 미션 프레임·단계 증가·안내 정지·재지연 실제 기계어 관찰 328개를
/// <see cref="MissionFrameGate"/>의 상태 14필드 전체 비트·정지 깊이·외부 조회/효과 순서·반환값으로 대조한다.
/// </summary>
/// <remarks>
/// 기대값은 cpppj/tests/fixtures/missionframe-x86.tsv(원본 PE 독립 실행, 링크 공유)이며 C# 에서 다시 계산하지 않는다.
/// 외부 승패/안내/튜토리얼/UI 는 fixture 가 명시한 입력과 변경(mutation)만 공급한다. 2026-10-15 감사 D02 에서 추가.
/// </remarks>
public sealed class X86MissionFrameGateTests
{
    /// <summary>fixture 의 중복 없는 고유 입력 개수. 같은 사례를 다른 검사에서 다시 더하지 않는다.</summary>
    private const int ExpectedRows = 328;

    /// <summary>fixture 열 수: 연산·사건·시각·깊이·다이얼로그·지식·안내·팝업·변경·입력상태·출력상태·출력깊이·추적·반환.</summary>
    private const int ColumnCount = 14;

    /// <summary>상태 문자열의 필드 수(원본 Tutorial 공통 필드 14개).</summary>
    private const int StateFieldCount = 14;

    /// <summary>TSV 의 부호 없는 64비트 십진수 칸을 문화권과 무관하게 읽는다. double 비트는 DWORD 로 줄이지 않는다.</summary>
    private static ulong Number(string text) => ulong.Parse(text, CultureInfo.InvariantCulture);

    /// <summary>fixture 의 DWORD 비트를 부호 있는 상태 필드로 되돌린다.</summary>
    private static int Signed(ulong value) => unchecked((int)(uint)value);

    /// <summary>fixture 의 14필드 입력 상태를 구성한다. 기대 결과를 계산하지 않고 비트만 옮긴다.</summary>
    private static MissionFrameGateState StateOf(string text)
    {
        string[] fields = text.Split(',');
        Assert.Equal(StateFieldCount, fields.Length);
        var v = new ulong[StateFieldCount];
        // 모든 칸은 원본 입력의 IEEE/DWORD 비트다.
        for (int i = 0; i < v.Length; i++) v[i] = Number(fields[i]);
        return new MissionFrameGateState
        {
            Stage = Signed(v[0]),
            StageDirty = (uint)v[1],
            TutorialNumber = (uint)v[2],
            Demo = (uint)v[3],
            StartSeconds = BitConverter.UInt64BitsToDouble(v[4]),
            NextStageSeconds = BitConverter.UInt64BitsToDouble(v[5]),
            DelaySeconds = BitConverter.UInt32BitsToSingle((uint)v[6]),
            LockFrames = Signed(v[7]),
            Started = (uint)v[8],
            DemoEndSeconds = BitConverter.UInt64BitsToDouble(v[9]),
            TimerSeconds = BitConverter.UInt64BitsToDouble(v[10]),
            TellAtSeconds = BitConverter.UInt64BitsToDouble(v[11]),
            ScriptPaused = (uint)v[12],
            ClearFrames = Signed(v[13]),
        };
    }

    /// <summary>현재 상태의 실제 비트를 fixture 와 같은 순서·형식의 문자열로 만든다.</summary>
    private static string Snapshot(MissionFrameGateState s)
    {
        ulong[] values =
        [
            unchecked((uint)s.Stage), s.StageDirty, s.TutorialNumber, s.Demo,
            BitConverter.DoubleToUInt64Bits(s.StartSeconds), BitConverter.DoubleToUInt64Bits(s.NextStageSeconds),
            BitConverter.SingleToUInt32Bits(s.DelaySeconds), unchecked((uint)s.LockFrames), s.Started,
            BitConverter.DoubleToUInt64Bits(s.DemoEndSeconds), BitConverter.DoubleToUInt64Bits(s.TimerSeconds),
            BitConverter.DoubleToUInt64Bits(s.TellAtSeconds), s.ScriptPaused, unchecked((uint)s.ClearFrames),
        ];
        return string.Join(',', values.Select(value => value.ToString(CultureInfo.InvariantCulture)));
    }

    /// <summary>
    /// 원본 실행기의 명시한 외부 입력과 효과만 공급하는 환경. 게이트 분기 자체는 복제하지 않는다.
    /// 각 외부 효과의 첫 호출에서만 fixture 가 정한 입력 변경(mutation)을 적용한다.
    /// </summary>
    private sealed class Environment(MissionFrameGateState state)
    {
        /// <summary>게이트와 같은 단계/타이머/잠금 저장소(복사본이 아니다).</summary>
        public MissionFrameGateState State { get; } = state;

        /// <summary>독립 입력 게임 시각(double 비트). 벽시각이나 합성 틱으로 바꾸지 않는다.</summary>
        public double Now { get; init; }

        /// <summary>외부 시계 정지 깊이.</summary>
        public uint Depth { get; set; }

        /// <summary>현재 처리 중인 사건 값. 튜토리얼 분배에 그대로 전달돼야 한다.</summary>
        public uint Event { get; init; }

        /// <summary>fixture 가 지정한 재진입 변경 번호(0이면 없음).</summary>
        public uint Mutation { get; init; }

        /// <summary>다이얼로그가 없으면 참(004c8e90 반환).</summary>
        public bool Dialogs { get; set; }

        /// <summary>지식 창이 열렸는지.</summary>
        public bool Knowledge { get; init; }

        /// <summary>안내 창이 열렸는지.</summary>
        public bool Guide { get; init; }

        /// <summary>0x4000 자식 창이 있는지.</summary>
        public bool Blocking { get; init; }

        /// <summary>외부 호출·효과의 순서 기록(원본 추적 문자열과 비교한다).</summary>
        public List<string> Trace { get; } = [];

        /// <summary>입력 변경을 이미 적용한 효과 종류. 각 종류는 한 번만 적용한다.</summary>
        private readonly HashSet<string> _changed = [];

        /// <summary>
        /// 외부 경계 호출을 기록하고, 처음 호출될 때만 fixture 의 변경을 적용한다.
        /// </summary>
        /// <param name="kind">원본 추적 글자(O 승패, Q 정지 조회, D 다이얼로그 …).</param>
        private void Effect(string kind)
        {
            Trace.Add(kind);
            if (!_changed.Add(kind)) return;
            if (Mutation == 1 && kind == "O") { State.LockFrames = 0; State.TutorialNumber = 2; }
            else if (Mutation == 2 && kind == "S") { State.ScriptPaused = 1; Depth++; Dialogs = false; }
            else if (Mutation == 3 && kind == "L") { State.LockFrames = 0; State.Demo = 3; }
            else if (Mutation == 4 && kind == "D") State.LockFrames = 2;
            else if (Mutation == 5 && kind == "G") State.ScriptPaused = 0;
            else if (Mutation == 6 && kind == "E") { State.DemoEndSeconds = 100.0; State.Demo = 0; }
        }

        /// <summary>원본 관찰 경계와 같은 이름·현재값을 제공하는 훅 묶음.</summary>
        public MissionFrameGateHooks Hooks() => new()
        {
            GameSeconds = () => Now,
            Paused = () => { Effect("Q"); return Depth != 0; },
            CheckOutcome = () => Effect("O"),
            DialogsClear = () => { Effect("D"); return Dialogs; },
            KnowledgeOpen = () => { Effect("K"); return Knowledge; },
            GuideOpen = () => { Effect("G"); return Guide; },
            BlockingGumpOpen = () => { Effect("B"); return Blocking; },
            TellStage = () => Effect("S"),
            TellAt = () => Effect("L"),
            EndDemo = () => Effect("E"),
            Tutorial = (number, current) =>
            {
                Assert.Equal(Event, current);
                Effect("T" + number.ToString(CultureInfo.InvariantCulture));
                return 0xa000U + number;
            },
            Pause = () => { Effect("P"); Depth++; },
            Resume = () => { Effect("R"); Depth--; },
        };
    }

    /// <summary>원본 네 몸체 328개의 정상 반환 입력에 대해 상태·정지 깊이·호출 순서·반환값을 모두 대조한다.</summary>
    [Fact]
    public void NativeFrameGate_MatchesAllStateEffectsAndReturns()
    {
        string[][] rows = X86Fixture.Read("missionframe");
        Assert.Equal(ExpectedRows, rows.Length);
        Assert.Equal(ExpectedRows, rows.Select(row => string.Join('\t', row.Take(ColumnCount - 1))).Distinct().Count());
        var operations = new Dictionary<string, int>();
        // 각 행은 독립 native 와 같은 새 상태·새 외부 입력에서 시작한다.
        bool Matches(string[] row)
        {
            Assert.Equal(ColumnCount, row.Length);
            operations[row[0]] = operations.GetValueOrDefault(row[0]) + 1;
            MissionFrameGateState state = StateOf(row[9]);
            var environment = new Environment(state)
            {
                Event = (uint)Number(row[1]),
                Now = BitConverter.UInt64BitsToDouble(Number(row[2])),
                Depth = (uint)Number(row[3]),
                Dialogs = Number(row[4]) != 0,
                Knowledge = Number(row[5]) != 0,
                Guide = Number(row[6]) != 0,
                Blocking = Number(row[7]) != 0,
                Mutation = (uint)Number(row[8]),
            };
            var gate = new MissionFrameGate(state, environment.Hooks());
            Assert.Same(state, gate.State);
            uint result = 0;
            switch (row[0])
            {
                case "Frame": result = gate.RunFrame(environment.Event); break;
                case "Advance": gate.Advance(); break;
                case "Pause": gate.PauseForDialog(); break;
                case "Postpone": gate.Postpone(); break;
                default: throw new InvalidDataException("알 수 없는 미션 프레임 fixture 연산입니다.");
            }
            return Snapshot(state) == row[10] && environment.Depth == (uint)Number(row[11])
                && string.Join(',', environment.Trace) == row[12] && result == (uint)Number(row[13]);
        }
        X86Fixture.CheckRows(rows, Matches);
        // 원본 관찰 분포를 고정해 한 연산만 재생되거나 행이 조용히 줄어드는 일을 막는다.
        Assert.Equal(265, operations["Frame"]);
        Assert.Equal(48, operations["Advance"]);
        Assert.Equal(9, operations["Pause"]);
        Assert.Equal(6, operations["Postpone"]);
    }

    /// <summary>완성된 훅 묶음에서 이름이 같은 경계 하나만 비워 새 묶음을 만든다(init 전용 속성이라 다시 구성한다).</summary>
    /// <param name="hooks">모든 경계가 채워진 묶음.</param>
    /// <param name="skip">비울 속성 이름.</param>
    private static MissionFrameGateHooks Without(MissionFrameGateHooks hooks, string skip) => new()
    {
        GameSeconds = skip == nameof(hooks.GameSeconds) ? null : hooks.GameSeconds,
        Paused = skip == nameof(hooks.Paused) ? null : hooks.Paused,
        CheckOutcome = skip == nameof(hooks.CheckOutcome) ? null : hooks.CheckOutcome,
        DialogsClear = skip == nameof(hooks.DialogsClear) ? null : hooks.DialogsClear,
        KnowledgeOpen = skip == nameof(hooks.KnowledgeOpen) ? null : hooks.KnowledgeOpen,
        GuideOpen = skip == nameof(hooks.GuideOpen) ? null : hooks.GuideOpen,
        BlockingGumpOpen = skip == nameof(hooks.BlockingGumpOpen) ? null : hooks.BlockingGumpOpen,
        TellStage = skip == nameof(hooks.TellStage) ? null : hooks.TellStage,
        TellAt = skip == nameof(hooks.TellAt) ? null : hooks.TellAt,
        EndDemo = skip == nameof(hooks.EndDemo) ? null : hooks.EndDemo,
        Tutorial = skip == nameof(hooks.Tutorial) ? null : hooks.Tutorial,
        Pause = skip == nameof(hooks.Pause) ? null : hooks.Pause,
        Resume = skip == nameof(hooks.Resume) ? null : hooks.Resume,
    };

    /// <summary>외부 경계가 하나라도 비면 실행하지 않는 정상 미션처럼 숨기지 않고 생성 단계에서 거부한다.</summary>
    [Theory]
    [InlineData("GameSeconds")]
    [InlineData("Paused")]
    [InlineData("CheckOutcome")]
    [InlineData("DialogsClear")]
    [InlineData("KnowledgeOpen")]
    [InlineData("GuideOpen")]
    [InlineData("BlockingGumpOpen")]
    [InlineData("TellStage")]
    [InlineData("TellAt")]
    [InlineData("EndDemo")]
    [InlineData("Tutorial")]
    [InlineData("Pause")]
    [InlineData("Resume")]
    public void Constructor_RejectsMissingBoundary(string missing)
    {
        var state = new MissionFrameGateState();
        MissionFrameGateHooks complete = new Environment(state).Hooks();
        Assert.NotNull(new MissionFrameGate(state, complete));
        Assert.Throws<ArgumentException>(() => new MissionFrameGate(state, Without(complete, missing)));
    }

    /// <summary>상태·경계 인자가 없으면 생성하지 않는다.</summary>
    [Fact]
    public void Constructor_RejectsNullArguments()
    {
        var state = new MissionFrameGateState();
        Assert.Throws<ArgumentNullException>(() => new MissionFrameGate(state, null!));
        Assert.Throws<ArgumentNullException>(() => new MissionFrameGate(null!, new Environment(state).Hooks()));
    }

    /// <summary>원본 004c2990 의 상태 기본값(65/1/0.5/10, 나머지 0)을 그대로 쓴다.</summary>
    [Fact]
    public void State_StartsWithOriginalDefaults()
    {
        var state = new MissionFrameGateState();
        Assert.Equal("65,1,0,0,0,0,1056964608,10,0,0,0,0,0,0", Snapshot(state));
    }
}
