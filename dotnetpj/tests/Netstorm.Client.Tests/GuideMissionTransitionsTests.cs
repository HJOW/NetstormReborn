using System.Text;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;

namespace Netstorm.Tests;

/// <summary>원본 Guide의 미션 수명과 입력 분배를 독립 x86 기대값과 대조한다.</summary>
public sealed class GuideMissionTransitionsTests
{
    /// <summary>독립 입력의 실제 현재·재진입·해제·공장 효과를 재현한다.</summary>
    private sealed class Scene
    {
        /// <summary>원본 독립 입력 아홉 DWORD다.</summary>
        private readonly uint[] _input;
        /// <summary>현재 미션과 해제 중 재진입할 미션 식별자다.</summary>
        private uint _active, _reentrant;
        /// <summary>현재 미션의 모드와 데모 값이다.</summary>
        private uint _mode, _demo;
        /// <summary>현재 이름, 시작 이름, 분기 결과다.</summary>
        private string _name = "TheWarBegins", _beginName, _target;
        /// <summary>관측한 실제 효과 순서다.</summary>
        private readonly List<string> _trace = [];

        /// <summary>독립 입력으로 실제 상태만 설정한다.</summary>
        /// <param name="values">쉼표로 나눈 아홉 DWORD.</param>
        public Scene(string values)
        {
            _input = values.Split(',').Select(X86Fixture.UInt).ToArray();
            _active = _input[1]; _reentrant = _input[4]; _mode = _input[2]; _demo = _input[3];
            _beginName = _input[6] != 0 ? "MasterOfWhirligigs" : "Tutorial2"; _target = _beginName;
        }

        /// <summary>문자열 바이트를 기대값과 같은 소문자 16진수로 표현한다.</summary>
        /// <param name="value">변환할 원본 바이트 문자열.</param>
        private static string Hex(string value) => Convert.ToHexString(Encoding.Latin1.GetBytes(value)).ToLowerInvariant();

        /// <summary>현재 상태의 실제 수명 효과를 연결한다.</summary>
        private GuideMissionTransitionHooks Hooks() => new()
        {
            Current = () => _active != 0,
            Finish = () => _trace.Add("F," + _active),
            Release = () => { _trace.Add("L," + _active); _active = 0; _name = ""; _target = "invalidated"; if (_reentrant != 0) { _active = 2; _reentrant = 0; } },
            Create = name => { _trace.Add("C," + Hex(name)); _name = name; _active = 3; _mode = _demo = 0; },
            Get = key => { _trace.Add("G," + Hex(key)); return _target; },
            Mode = value => _mode = value,
            Demo = value => _demo = value,
        };

        /// <summary>독립 입력에 해당하는 제품 분기를 실행하고 관측 상태를 직렬화한다.</summary>
        public string Run()
        {
            GuideMissionTransitionHooks hooks = Hooks();
            void Restart() => GuideMissionTransitions.Restart(GuideMissionTransitions.CaptureReload(_name, _input[2], _input[3]), hooks);
            string result = "void";
            if (_input[0] == 0) GuideMissionTransitions.Begin(_beginName, hooks);
            else if (_input[0] == 1) GuideMissionTransitions.Fork(_input[5], hooks);
            else if (_input[0] == 2) Restart();
            else
            {
                uint[] codes = [GuideMissionTransitions.BeginCommand, GuideMissionTransitions.ForkCommand, GuideMissionTransitions.ReloadCommand, GuideMissionTransitions.EndCommand];
                var commands = new GuideMissionCommandHooks
                {
                    Begin = _ => GuideMissionTransitions.Begin(_beginName, hooks),
                    Fork = value => GuideMissionTransitions.Fork(value, hooks),
                    Restart = Restart,
                    End = () => GuideMissionTransitions.End(hooks),
                    InvalidBegin = _ => _trace.Add("A"),
                };
                result = GuideMissionTransitions.Dispatch(new InputEvent(codes[_input[0] - 3] | _input[7], unchecked((int)_input[8]), unchecked((int)_input[5])), commands) ? "1" : "0";
            }
            return result + "|" + _active + "|" + Hex(_name) + "|" + _mode + "," + _demo + "|" + (_trace.Count == 0 ? "-" : string.Join(';', _trace));
        }
    }

    /// <summary>시작·분기·재시작·종료와 재진입의 독립 원본 결과 256행을 검사한다.</summary>
    [Fact]
    public void NativeTransitionsMatchIndependentX86Rows()
    {
        string[][] rows = X86Fixture.Read("guide-transitions", "T");
        Assert.Equal(256, rows.Length);
        X86Fixture.CheckRows(rows, row => new Scene(row[1]).Run() == row[2]);
    }

    /// <summary>미등록 입력은 효과 없이 남기고 Close는 현재가 없으면 반납하지 않는다.</summary>
    [Fact]
    public void UnknownAndEmptyCloseDoNotInvokeEffects()
    {
        Assert.False(GuideMissionTransitions.Dispatch(new InputEvent(0, 0, 0), new GuideMissionCommandHooks()));
        GuideMissionTransitions.Close(new GuideMissionTransitionHooks { Current = () => false });
    }
}
