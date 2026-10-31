using System.Buffers.Binary;
using System.Text;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 원본 미션 안내 입력과 데모 설정을 검증한다.
namespace Netstorm.Tests;

/// <summary>First·현재 안내·F8과 데모 및 재시작 입력을 독립 원본 결과와 비교한다.</summary>
public sealed class MissionStageInputFixtureTests
{
    /// <summary>원본 입력 한 행의 동일 미션 상태와 실제 경계다.</summary>
    private sealed class Scene
    {
        /// <summary>연산·상태·외부 효과를 지정한 원본 입력이다.</summary>
        private readonly uint[] _input;
        /// <summary>First와 공유하는 현재 미션 상태다.</summary>
        private readonly MissionStartState _state;
        /// <summary>안내가 읽는 데모·단계 상태다.</summary>
        private readonly MissionFrameGateState _gate;
        /// <summary>마지막 Tell 결과다.</summary>
        private uint _shown;
        /// <summary>외부 효과 순서다.</summary>
        private readonly List<string> _trace = [];

        /// <summary>원본 입력만 관련 상태에 복원한다.</summary>
        /// <param name="values">원본 입력 DWORD 목록.</param>
        public Scene(string values)
        {
            _input = values.Split(',').Select(X86Fixture.UInt).ToArray();
            _state = new MissionStartState { Flags = _input[1], Sequence = _input[2] };
            _gate = new MissionFrameGateState { Demo = _input[3], Stage = unchecked((int)_input[4]) };
            _shown = _input[5];
        }

        /// <summary>실제 현재 단계 안내와 Tell·팝업·정지 효과를 연결한다.</summary>
        private void Show() => MissionGuideInput.TellStage(_gate, ref _shown,
            section => { _trace.Add("tell:" + Convert.ToHexString(Encoding.Latin1.GetBytes(section)).ToLowerInvariant()); return _input[6]; },
            () => _trace.Add("close"), () => _trace.Add("pause"));

        /// <summary>입력에 지정된 실제 분기를 실행하고 전체 원본 바이트와 효과를 직렬화한다.</summary>
        public string Run()
        {
            string result = "void";
            if (_input[0] == 0) MissionGuideInput.MissionFirst(_state, Show);
            else if (_input[0] == 1)
            {
                var input = new InputEvent(_input[8], 0x12345678, unchecked((int)0x87654321));
                Action? first = _input[7] != 0 ? () => MissionGuideInput.MissionFirst(_state, Show) : null;
                if (MissionGuideInput.DispatchFirst(input, first)) result = "1";
                else if (_input[7] != 0) { _trace.Add("input:" + _input[8] + ":305419896:2271560481"); result = _input[9].ToString(); }
                else result = "0";
            }
            else
            {
                var queue = new InputQueue();
                MissionGuideInput.QueueFirstKey(new InputEvent(_input[8], 0, 0), queue, _input[7] != 0);
                if (!queue.Empty) { InputEvent input = queue.Pop(false); _trace.Add("queue:" + input.Code + ":" + input.X + ":" + input.Y); }
                result = "slice";
            }
            var raw = new byte[0xd0];
            // 무관한 원본 stale 바이트도 그대로 보존한다.
            for (int i = 0; i < raw.Length; i++) raw[i] = unchecked((byte)(i * 37 + 41));
            Put(raw, 0, 0x12002000); Put(raw, 0x54, _state.Flags); Put(raw, 0x58, _state.Sequence); Put(raw, 0x68, _input[3]); Put(raw, 0xb4, _shown);
            return result + "|" + Convert.ToHexString(raw).ToLowerInvariant() + "|" + (_trace.Count == 0 ? "-" : string.Join(';', _trace));
        }
    }

    /// <summary>원본 바이트에 little-endian DWORD를 쓴다.</summary>
    /// <param name="raw">원본 바이트.</param><param name="offset">위치.</param><param name="value">값.</param>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset, 4), value);

    /// <summary>First와 F8의 원본 278행을 미션 전체 바이트·효과로 검사한다.</summary>
    [Fact]
    public void FirstAndF8MatchNative278()
    {
        string[][] rows = X86Fixture.Read("mission-first");
        Assert.Equal(278, rows.Length);
        X86Fixture.CheckRows(rows, row => new Scene(row[0]).Run() == row[1]);
    }

    /// <summary>데모 입력과 재시작 캡처의 원본 108행을 검사한다.</summary>
    [Fact]
    public void DemoAndReloadMatchNative108()
    {
        string[][] rows = X86Fixture.Read("mission-common");
        Assert.Equal(108, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            string text = Encoding.Latin1.GetString(Convert.FromHexString(row[3]));
            if (row[0] == "I")
            {
                uint demo = X86Fixture.UInt(row[1]);
                MissionGuideInput.ApplyDemo(text, ref demo);
                return demo.ToString() == row[4];
            }
            MissionReloadRequest request = GuideMissionTransitions.CaptureReload(text, X86Fixture.UInt(row[1]), X86Fixture.UInt(row[2]));
            string hex = Convert.ToHexString(Encoding.Latin1.GetBytes(request.Name)).ToLowerInvariant();
            return request.Mode + "," + request.Demo + "," + hex == row[4];
        });
    }
}
