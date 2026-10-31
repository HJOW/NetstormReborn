using Netstorm.Client.Input;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Mission;

/// <summary>미션 안내와 제어 명령을 원본 입력 큐에 연결한다.
/// 근거: cpppj/src/client/MissionGuideInput.h/.cpp의 명령 조회·Abort·First·F8 경로.</summary>
public static class MissionGuideInput
{
    /// <summary>현재 안내를 다시 표시하는 명령이다.</summary>
    public const uint FirstCommand = 0x02000091;
    /// <summary>미션 중단 확인 명령이다.</summary>
    public const uint AbortCommand = 0x0200008c;
    /// <summary>전투 떠나기 명령이다.</summary>
    public const uint LeaveBattleCommand = 0x020000c5;
    /// <summary>상위 입력 플래그 제거용 마스크다.</summary>
    private const uint CommandMask = 0x03ffffff;
    /// <summary>원본 F8 가상 키 코드다.</summary>
    private const uint F8 = 0x77;
    /// <summary>등록되지 않은 원본 명령 값이다.</summary>
    private const uint Unknown = 0xffffffff;

    /// <summary>ASCII 영문 대소문자만 무시하여 원본 제어 명령 이름을 조회한다.</summary>
    /// <param name="name">조회할 원시 명령 이름.</param>
    public static uint ControlScriptCode(string name)
    {
        char[] lower = name.ToCharArray();
        // 원본 stricmp의 ASCII 영문에 해당하는 문자만 소문자로 변환한다.
        for (int i = 0; i < lower.Length; i++)
            if (lower[i] >= 'A' && lower[i] <= 'Z') lower[i] = (char)(lower[i] + ('a' - 'A'));
        return new string(lower) switch
        {
            "missionrestart" => GuideMissionTransitions.ReloadCommand,
            "missionabort" => AbortCommand,
            "leavebattle" => LeaveBattleCommand,
            "missionbegin" => GuideMissionTransitions.BeginCommand,
            "missionfork" => GuideMissionTransitions.ForkCommand,
            "missionend" => GuideMissionTransitions.EndCommand,
            _ => Unknown,
        };
    }

    /// <summary>여섯 미션 제어 이름만 공통 명령 파서·문자열 고리·입력 큐에 예약한다.</summary>
    /// <param name="name">명령 이름.</param><param name="argument">원시 인자.</param>
    /// <param name="queue">실제 40칸 입력 큐.</param><param name="strings">실제 80칸 문자열 고리.</param>
    public static bool QueueControlAction(string name, string argument, InputQueue queue, ScriptStringRing strings)
    {
        if (ControlScriptCode(name) == Unknown) return false;
        ScriptCommand.QueueScriptCommand(name + "," + argument, queue, strings, new ScriptCommandHooks
        {
            Lookup = ControlScriptCode,
            Unknown = _ => throw new InvalidOperationException("확인한 미션 제어 이름의 조회 실패"),
        });
        return true;
    }

    /// <summary>Abort를 소비한다. 인자 0은 확인 Tell이고 나머지는 LeaveBattle을 큐에 예약한다.</summary>
    /// <param name="input">처리할 사건.</param><param name="queue">후속 사건을 받는 큐.</param><param name="tell">실제 확인 창 연결.</param>
    public static bool DispatchAbort(InputEvent input, InputQueue queue, Action<string>? tell)
    {
        if ((input.Code & CommandMask) != AbortCommand) return false;
        if (input.Y == 0) (tell ?? throw new InvalidOperationException("미션 중단 확인의 실제 Tell 연결 누락"))("ABORT");
        else queue.PushCommand(LeaveBattleCommand, 2, 2);
        return true;
    }

    /// <summary>First 명령을 소비하고 현재 미션이 있으면 해당 안내를 호출한다.</summary>
    /// <param name="input">처리할 사건.</param><param name="currentFirst">현재 미션의 First. 없으면 현재 미션이 없다.</param>
    public static bool DispatchFirst(InputEvent input, Action? currentFirst)
    {
        if ((input.Code & CommandMask) != FirstCommand) return false;
        currentFirst?.Invoke();
        return true;
    }

    /// <summary>활성 미션의 수정 키 없는 F8 누름을 즉시 실행하지 않고 First 명령으로 예약한다.</summary>
    /// <param name="input">키 입력 사건.</param><param name="queue">명령을 받을 큐.</param><param name="activeMission">현재 미션 활성 여부.</param>
    public static bool QueueFirstKey(InputEvent input, InputQueue queue, bool activeMission)
    {
        if (!activeMission || (input.Code & InputCode.Release) != 0 || (input.Code & CommandMask) != (F8 << InputCode.VirtualKeyShift) ||
            (input.Code & (InputCode.Control | InputCode.Shift)) != 0) return false;
        queue.PushCommand(FirstCommand, 0, 0);
        return true;
    }
}
