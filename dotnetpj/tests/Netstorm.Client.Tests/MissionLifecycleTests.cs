using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 원본 Guide 단계와 프레임 정지의 통합 흐름을 검증한다.
namespace Netstorm.Tests;

/// <summary>Guide 반복과 단계·정지 연결을 독립 x86 기대값과 비교한다.</summary>
public sealed class MissionLifecycleTests
{
    /// <summary>한 원본 행의 실제 상태와 외부 효과 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>연산·상태·외부 효과를 지정하는 원본 입력이다.</summary>
        private readonly ulong[] _input;
        /// <summary>Guide와 단계가 공유하는 미션 상태다.</summary>
        private readonly MissionStartState _state = new();
        /// <summary>정지·시작이 공유하는 프레임 상태다.</summary>
        private readonly MissionFrameGateState _gate = new();
        /// <summary>현재 미션 유무다.</summary>
        private bool _active;
        /// <summary>외부 정지 깊이와 프레임 완료 요청이다.</summary>
        private uint _depth, _finish;
        /// <summary>실제 호출 순서다.</summary>
        private readonly List<string> _trace = [];
        /// <summary>실제 Pause와 Postpone 처리기다.</summary>
        private readonly MissionFrameGate _frameGate;
        /// <summary>실제 Guide 반복 처리기다.</summary>
        private readonly RawMissionGuide _guide;

        /// <summary>원본 입력만 현재 상태로 복원하고 실제 처리기를 연결한다.</summary>
        /// <param name="values">원본 입력 DWORD와 double 비트.</param>
        public Scene(string values)
        {
            _input = values.Split(',').Select(value => ulong.Parse(value, CultureInfo.InvariantCulture)).ToArray();
            _state.Flags = (uint)_input[1]; _state.Sequence = (uint)_input[2]; _active = _input[3] != 0;
            _depth = (uint)_input[7]; _finish = (uint)_input[9];
            _gate.NextStageSeconds = BitConverter.UInt64BitsToDouble(_input[11]); _gate.ScriptPaused = (uint)_input[8];
            _frameGate = new MissionFrameGate(_gate, new MissionFrameGateHooks
            {
                GameSeconds = () => 100.125,
                Paused = () => { _trace.Add("Q"); return _depth != 0; },
                Pause = () => { _trace.Add("H"); _depth++; },
                Resume = Unexpected, CheckOutcome = Unexpected, TellStage = Unexpected, TellAt = Unexpected, EndDemo = Unexpected,
                DialogsClear = UnexpectedBool, KnowledgeOpen = UnexpectedBool, GuideOpen = UnexpectedBool, BlockingGumpOpen = UnexpectedBool,
                Tutorial = (_, _) => throw new InvalidOperationException("선택되지 않은 튜토리얼 효과"),
            });
            _guide = new RawMissionGuide(OriginalEdition.Patch1078, new MissionGuideHooks
            {
                Current = Current,
                Pending = () => { _trace.Add("P"); return (uint)_input[4]; },
                DialogsClear = () => { _trace.Add("D"); return _input[5] != 0; },
                Stage = (target, sequence) => { Assert.Same(_state, target); MissionLifecycle.RunTutorialStage(target, sequence, StageHooks()); },
                Postpone = _ => _frameGate.Postpone(),
                Release = () => { _trace.Add("L"); _active = false; },
                InvalidStage = () => _trace.Add("A"),
            });
        }

        /// <summary>선택되면 안 되는 공통 프레임 효과를 거부한다.</summary>
        private static void Unexpected() => throw new InvalidOperationException("선택되지 않은 프레임 효과");
        /// <summary>선택되면 안 되는 공통 프레임 조회를 거부한다.</summary>
        private static bool UnexpectedBool() => throw new InvalidOperationException("선택되지 않은 UI 조회");
        /// <summary>현재 포인터 조회와 관찰을 같은 시점에 수행한다.</summary>
        private MissionStartState? Current() { _trace.Add(_active ? "C,1" : "C,0"); return _active ? _state : null; }
        /// <summary>원본 문자열 바이트를 소문자 hex로 표현한다.</summary>
        /// <param name="value">표현할 문자열.</param>
        private static string Hex(string value) => Convert.ToHexString(Encoding.Latin1.GetBytes(value)).ToLowerInvariant();

        /// <summary>실제 단계가 선택하는 외부 효과를 같은 현재 상태에 연결한다.</summary>
        private TutorialMissionStageHooks StageHooks() => new()
        {
            Initialize = () => { _trace.Add("I"); _state.Flags |= 1u; MissionLifecycle.PauseWorldEntry(() => Current() != null, _frameGate.PauseForDialog); },
            TellGlobal = (name, mode) => { Assert.Equal(1u, mode); _trace.Add("G," + Hex(name)); },
            Start = () => { _trace.Add("S"); _gate.Started = 1; RawMissionGuide.Advance(_state); },
            Frame = () => { _trace.Add("F"); if (_finish != 0) { _state.Flags |= 1u; _finish = 0; } },
            ShowStage = () => _trace.Add("Show"),
            TellMission = name => { _trace.Add("T," + Hex(name)); return (uint)_input[6]; },
            CloseTransient = () => _trace.Add("K"),
            Pause = _frameGate.PauseForDialog,
        };

        /// <summary>지정 횟수의 Guide 또는 진입 Pause를 실행하고 전체 관찰을 직렬화한다.</summary>
        public string Run()
        {
            // 원본 입력이 지정한 횟수만큼 같은 장기 상태에서 실행한다.
            for (ulong call = 0; call < _input[10]; call++)
            {
                if (_input[0] != 0) MissionLifecycle.PauseWorldEntry(() => Current() != null, _frameGate.PauseForDialog);
                else _guide.Run();
            }
            var raw = new byte[0xd0];
            // 무관한 원본 stale 바이트를 바꾸지 않고 관찰한다.
            for (int i = 0; i < raw.Length; i++) raw[i] = unchecked((byte)(i * 37 + 41));
            Put(raw, 0, 0x12002000); Put(raw, 0x54, _state.Flags); Put(raw, 0x58, _state.Sequence);
            Put(raw, 0x80, 0x3f000000); Put(raw, 0x88, _gate.Started); Put(raw, 0xb8, _gate.ScriptPaused);
            BinaryPrimitives.WriteUInt64LittleEndian(raw.AsSpan(0x78, 8), BitConverter.DoubleToUInt64Bits(_gate.NextStageSeconds));
            return (_active ? "1" : "0") + "|" + _depth + "|" + Convert.ToHexString(raw).ToLowerInvariant() + "|" + (_trace.Count == 0 ? "-" : string.Join(';', _trace));
        }
    }

    /// <summary>원본 메모리의 little-endian DWORD를 쓴다.</summary>
    /// <param name="raw">원본 바이트.</param><param name="offset">위치.</param><param name="value">값.</param>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset, 4), value);

    /// <summary>Guide의 단계·현재 포인터 재조회·대화창 관문과 정지 원본 831행을 검사한다.</summary>
    [Fact]
    public void LifecycleMatchesNative831()
    {
        string[][] rows = X86Fixture.Read("mission-lifecycle");
        Assert.Equal(831, rows.Length);
        X86Fixture.CheckRows(rows, row => new Scene(row[0]).Run() == row[1]);
    }
}
