using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Tests;

/// <summary>미션 시작의 전체 상태와 외부 효과 순서를 독립 x86 결과와 비교한다.</summary>
public sealed class RawMissionStartTests
{
    /// <summary>원본 행의 입력과 실제 효과만 보관하는 검사 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>원본 입력 정수다.</summary>
        private readonly long[] _numbers;
        /// <summary>현재 미션 설정 값이다.</summary>
        private readonly Dictionary<string, string> _config = new(StringComparer.Ordinal);
        /// <summary>실제 발생한 조회 및 효과 순서다.</summary>
        private readonly List<string> _trace = [];

        /// <summary>원본 행의 정수와 설정 문자열을 복원한다.</summary>
        /// <param name="input">원본 fixture의 입력 칸.</param>
        public Scene(string input)
        {
            string[] fields = input.Split('|');
            _numbers = fields[0].Split(',').Select(long.Parse).ToArray();
            // 세미콜론이 포함된 설정은 원본 바이트 hex로 복원한다.
            foreach (string field in fields.Skip(1).Where(x => x.Length != 0))
            {
                int equal = field.IndexOf('=');
                _config[field[..equal]] = Encoding.Latin1.GetString(Convert.FromHexString(field[(equal + 1)..]));
            }
        }

        /// <summary>관찰할 DWORD 입력을 원본처럼 감아 변환한다.</summary>
        /// <param name="index">입력 인덱스.</param>
        private uint Number(int index) => unchecked((uint)_numbers[index]);
        /// <summary>원본 문자열을 소문자 바이트 hex로 직렬화한다.</summary>
        /// <param name="value">변환할 문자열.</param>
        private static string Hex(string value) => Convert.ToHexString(Encoding.Latin1.GetBytes(value)).ToLowerInvariant();
        /// <summary>double의 리틀엔디언 메모리 비트를 직렬화한다.</summary>
        /// <param name="value">관찰할 값.</param>
        private static string DoubleHex(double value) => Convert.ToHexString(BitConverter.GetBytes(value)).ToLowerInvariant();
        /// <summary>원본 설정 조회의 키와 순서를 기록한다.</summary>
        /// <param name="key">조회 키.</param>
        private string Get(string key) { _trace.Add("G," + key); return _config.GetValueOrDefault(key, ""); }

        /// <summary>실제 Start를 실행하고 전체 관찰 상태 및 효과를 돌려준다.</summary>
        /// <param name="currentInputs">반대 fallback과 실제 getter를 사용할지.</param>
        public (string State, string Trace) Run(bool currentInputs)
        {
            var gate = new MissionFrameGateState { Demo = Number(3), Started = 0, StartSeconds = 77.25, DemoEndSeconds = 77.25, TellAtSeconds = 77.25 };
            var state = new MissionStartState { Flags = uint.MaxValue, Sequence = Number(5), TellAtIndex = Number(6), FieldC0 = 0xa5a5a5a5, FieldC4 = 0xa5a5a5a5 };
            var aux = new MissionStartAux { AllowAnyCapture = -71, OrdinaryGeysers = 73 };
            var globals = new MissionStartGlobals { LocalOwner = Number(0), PlayerKnown = Number(1), Editor = Number(2), Aux = aux };
            var hooks = new MissionStartHooks
            {
                Get = Get,
                SectionNames = prefix => { _trace.Add("N," + prefix); return _config.GetValueOrDefault("@sections", ""); },
                Set = (key, value) => { _trace.Add("W," + key + "," + Hex(value)); _config[key] = value; },
                GameSeconds = () => 16777217.25,
                SetSp = (owner, value) => _trace.Add("S," + owner + "," + BitConverter.SingleToUInt32Bits(value)),
                RefreshSp = () => _trace.Add("R"),
                ClearTechnology = owner => _trace.Add("C," + owner),
                AddTechnology = (owner, tech) => _trace.Add("T," + owner + "," + Hex(tech)),
                HasPriest = owner => { _trace.Add("P," + owner); return (Number(4) & (1u << (int)owner)) != 0; },
                CreateAi = owner => _trace.Add("I," + owner),
                RefreshCommandGump = () => _trace.Add("Q"),
                RandomGeysers = () => _trace.Add("Y"),
                ResetCamera = () => _trace.Add("Z"),
            };
            var start = new RawMissionStart(OriginalEdition.Patch1078, gate, state, globals, hooks);
            if (currentInputs)
            {
                globals.LocalOwner = Number(0) == 1 ? 8u : 1u;
                globals.PlayerKnown = Number(1) == 0 ? 1u : 0u;
                globals.Editor = Number(2) == 0 ? 1u : 0u;
                start.BindCurrentInputs(new MissionStartReads { LocalOwner = () => Number(0), PlayerKnown = () => Number(1), Editor = () => Number(2) });
            }
            start.Start();
            uint[] values = [state.Flags, state.Sequence, state.TellAtIndex, state.FieldC0, state.FieldC4, gate.Started,
                unchecked((uint)globals.DenySalvage), unchecked((uint)globals.DenyAscend), unchecked((uint)aux.AllowAnyCapture), aux.OrdinaryGeysers];
            string result = string.Join(',', values) + "|" + DoubleHex(gate.StartSeconds) + "|" + DoubleHex(gate.DemoEndSeconds) + "|" + DoubleHex(gate.TellAtSeconds);
            return (result, _trace.Count == 0 ? "-" : string.Join(';', _trace));
        }
    }

    /// <summary>원본 Start 48행의 상태·실제 효과와 반대 fallback을 둔 현재 getter 재생을 대조한다.</summary>
    [Fact]
    public void StartMatchesNative48AndCurrentReads()
    {
        string[][] rows = X86Fixture.Read("mission-start", "start");
        Assert.Equal(48, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var fallback = new Scene(row[1]).Run(false);
            var current = new Scene(row[1]).Run(true);
            return fallback.State == row[2] && fallback.Trace == row[4] && current == fallback;
        });
    }

    /// <summary>필수 실제 효과가 없으면 시작 완료를 쓰지 않고 이미 수행한 앞부분은 유지한다.</summary>
    [Fact]
    public void MissingEffectStopsBeforeStarted()
    {
        var gate = new MissionFrameGateState();
        var state = new MissionStartState { Flags = 1 };
        var hooks = new MissionStartHooks { Get = _ => "", SectionNames = _ => "", Set = (_, _) => { } };
        var start = new RawMissionStart(OriginalEdition.Patch1078, gate, state, new MissionStartGlobals(), hooks);
        Assert.Throws<InvalidOperationException>(start.Start);
        Assert.Equal(0u, gate.Started);
        Assert.Equal(1u, state.Sequence);
        Assert.Equal(0u, state.Flags);
    }
    /// <summary>SP·기술·AI 효과가 소유자를 바꿔도 각각 원본 위치에서 현재 값을 다시 읽는다.</summary>
    [Fact]
    public void CurrentOwnerIsReadAfterEachEffect()
    {
        var gate = new MissionFrameGateState();
        var state = new MissionStartState();
        var globals = new MissionStartGlobals { LocalOwner = 8 };
        uint owner = 1;
        var trace = new List<string>();
        var hooks = new MissionStartHooks
        {
            Get = key => { if (key == "myStartMoney") { trace.Add("M"); return "12"; } if (key == "myTech") { trace.Add("T"); owner = 5; } return ""; },
            SectionNames = _ => "",
            Set = (_, _) => { },
            SetSp = (id, amount) => { Assert.Equal(1u, id); Assert.Equal(194162f, amount); trace.Add("S1"); owner = 2; },
            RefreshSp = () => { trace.Add("R"); owner = 3; },
            ClearTechnology = id => { Assert.Equal(3u, id); trace.Add("C3"); owner = 4; },
            AddTechnology = (id, _) => { Assert.Equal(4u, id); trace.Add("A4"); owner = 6; },
            HasPriest = id => { trace.Add("P" + id); return id == 2; },
            CreateAi = id => { Assert.Equal(2u, id); trace.Add("I2"); owner = 7; },
            RefreshCommandGump = () => trace.Add("Q"),
            ResetCamera = () => trace.Add("Z"),
            GameSeconds = () => { trace.Add("V"); return 17.25; },
        };
        var start = new RawMissionStart(OriginalEdition.Patch1078, gate, state, globals, hooks);
        Assert.Throws<ArgumentException>(() => start.BindCurrentInputs(new MissionStartReads { LocalOwner = () => owner, Editor = () => 1 }));
        start.BindCurrentInputs(new MissionStartReads
        {
            LocalOwner = () => { trace.Add("O" + owner); return owner; },
            PlayerKnown = () => { trace.Add("K"); return 1; },
            Editor = () => { trace.Add("E"); return 1; },
        });
        start.Start();
        Assert.Equal("M;E;K;O1;S1;R;O3;C3;O4;T;A4;O6;P1;O6;P2;I2;O7;P3;O7;P4;O7;P5;O7;P6;O7;O7;P8;Q;Z;V", string.Join(';', trace));
        Assert.Equal(8u, globals.LocalOwner);
        Assert.Equal(17.25, gate.StartSeconds);
        Assert.Equal(1u, gate.Started);
    }

}
