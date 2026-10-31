using System.Text;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>미션 제어 명령과 실제 큐·문자열 고리 및 Guide 수명 연결을 검사한다.</summary>
public sealed class MissionGuideInputTests
{
    /// <summary>원본 명령 이름과 인자의 독립 x86 결과 30행을 실제 파서·큐와 비교한다.</summary>
    [Fact]
    public void ControlActionsMatchIndependentX86Rows()
    {
        string[][] rows = X86Fixture.Read("guide-transitions", "S");
        Assert.Equal(30, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            string raw = Encoding.Latin1.GetString(Convert.FromHexString(row[1]));
            int comma = raw.IndexOf(',');
            var queue = new InputQueue();
            var strings = new ScriptStringRing();
            if (!MissionGuideInput.QueueControlAction(raw[..comma], raw[(comma + 1)..], queue, strings)) return false;
            InputEvent input = queue.Pop(false);
            string[] expected = row[2].Split(',');
            if (input.Code != X86Fixture.UInt(expected[0]) || input.X != input.Y || !queue.Empty || strings.Index != int.Parse(row[3])) return false;
            uint argument = unchecked((uint)input.X);
            return expected[1][0] == 's'
                ? input.X > 100 && strings.Resolve(argument) == Encoding.Latin1.GetString(Convert.FromHexString(expected[1][1..]))
                : argument == X86Fixture.UInt(expected[1][1..]);
        });
    }

    /// <summary>F8과 중단 명령은 즉시 실행 대신 정확한 조건과 인자로 큐를 사용한다.</summary>
    [Fact]
    public void FirstAndAbortUseSharedQueue()
    {
        var queue = new InputQueue();
        uint f8 = 0x77u << InputCode.VirtualKeyShift;
        Assert.False(MissionGuideInput.QueueFirstKey(new InputEvent(f8 | InputCode.Shift, 0, 0), queue, true));
        Assert.False(MissionGuideInput.QueueFirstKey(new InputEvent(f8 | InputCode.Release, 0, 0), queue, true));
        Assert.False(MissionGuideInput.QueueFirstKey(new InputEvent(f8, 0, 0), queue, false));
        Assert.True(MissionGuideInput.QueueFirstKey(new InputEvent(f8, 0, 0), queue, true));
        int first = 0;
        Assert.True(MissionGuideInput.DispatchFirst(queue.Pop(false), () => first++));
        Assert.Equal(1, first);
        string? shown = null;
        Assert.True(MissionGuideInput.DispatchAbort(new InputEvent(MissionGuideInput.AbortCommand | InputCode.Release, 0, 0), queue, name => shown = name));
        Assert.Equal("ABORT", shown);
        Assert.True(MissionGuideInput.DispatchAbort(new InputEvent(MissionGuideInput.AbortCommand, 0, 1), queue, null));
        Assert.Equal(new InputEvent(MissionGuideInput.LeaveBattleCommand, 2, 2), queue.Pop(false));
        Assert.False(MissionGuideInput.QueueControlAction("other", "value", queue, new ScriptStringRing()));
        Assert.True(queue.Empty);
    }

    /// <summary>미션 시작 명령이 실제 문자열 고리와 큐를 거쳐 현재 스크립트를 교체한다.</summary>
    [Fact]
    public void QueuedBeginReplacesActualMissionScript()
    {
        var config = new ConfigInterface(null);
        config.Configuration.Assign("missionSpec=\"d/{local.1}.english\"\n[END]\n");
        Func<string, byte[]?> source = path => path == "d/second.english" ? Encoding.Latin1.GetBytes("missionType=Tutorial\n[END]\n") : null;
        MissionScript? mission = GuideMissionScriptLifetime.Create(config, "first", ref _mode, source);
        int finished = 0;
        var transitions = new GuideMissionTransitionHooks
        {
            Current = () => mission != null,
            Finish = () => finished++,
            Release = () => GuideMissionScriptLifetime.Release(ref mission, ref _mode, config),
            Create = name => mission = GuideMissionScriptLifetime.Create(config, name, ref _mode, source),
        };
        var queue = new InputQueue();
        var strings = new ScriptStringRing();
        Assert.True(MissionGuideInput.QueueControlAction("MissionBegin", "second", queue, strings));
        var commands = new GuideMissionCommandHooks
        {
            Begin = token => GuideMissionTransitions.Begin(() => strings.Resolve(token), transitions),
            InvalidBegin = _ => throw new InvalidOperationException("문자열 토큰이 유효하지 않다"),
        };
        Assert.True(GuideMissionTransitions.Dispatch(queue.Pop(false), commands));
        Assert.Equal(1, finished);
        Assert.Equal("second", mission!.Name);
        Assert.True(mission.Loaded);
        Assert.Equal("Tutorial", mission.MissionType());
        GuideMissionScriptLifetime.Release(ref mission, ref _mode, config);
    }

    /// <summary>연결 검사에서 실제 Native 모드 슬롯으로 사용하는 값이다.</summary>
    private bool _mode;
}
