using Netstorm.Client.Input;

namespace Netstorm.Client.Mission;

/// <summary>재시작 전에 소유 복사한 이름과 0 또는 1로 정규화된 모드·데모 값이다.</summary>
/// <param name="Name">해제 뒤에도 사용할 미션 이름.</param>
/// <param name="Mode">정규화된 모드.</param>
/// <param name="Demo">정규화된 데모 값.</param>
public readonly record struct MissionReloadRequest(string Name, uint Mode, uint Demo);

/// <summary>Guide 교체가 실제 현재 미션과 공장에 수행하는 효과를 제공한다.</summary>
public sealed class GuideMissionTransitionHooks
{
    /// <summary>현재 미션 유무를 매번 조회한다.</summary>
    public Func<bool>? Current { get; init; }
    /// <summary>현재 미션의 가상 종료를 수행한다.</summary>
    public Action? Finish { get; init; }
    /// <summary>현재 미션을 반납한다.</summary>
    public Action? Release { get; init; }
    /// <summary>이름으로 새 미션을 생성한다.</summary>
    public Action<string>? Create { get; init; }
    /// <summary>현재 미션의 분기 설정을 조회한다.</summary>
    public Func<string, string>? Get { get; init; }
    /// <summary>새 미션의 모드를 복원한다.</summary>
    public Action<uint>? Mode { get; init; }
    /// <summary>현재 미션의 데모 값을 복원한다.</summary>
    public Action<uint>? Demo { get; init; }
}

/// <summary>입력 명령을 현재 Guide 수명과 진단에 전달하는 효과다.</summary>
public sealed class GuideMissionCommandHooks
{
    /// <summary>문자열 토큰으로 미션을 시작한다.</summary>
    public Action<uint>? Begin { get; init; }
    /// <summary>DWORD 인자로 미션을 분기한다.</summary>
    public Action<uint>? Fork { get; init; }
    /// <summary>현재 미션을 재시작한다.</summary>
    public Action? Restart { get; init; }
    /// <summary>현재 미션을 종료한다.</summary>
    public Action? End { get; init; }
    /// <summary>잘못된 Begin 인자를 진단한다. 반환하면 Begin을 계속한다.</summary>
    public Action<uint>? InvalidBegin { get; init; }
}

/// <summary>원본 Guide의 미션 교체와 입력 분배 순서다. 실제 수명은 호출자가 효과로 제공한다.
/// 근거: cpppj/src/client/GuideMissionTransitions.cpp, MissionGuideInput.cpp.</summary>
public static class GuideMissionTransitions
{
    /// <summary>Begin 명령 코드다.</summary>
    public const uint BeginCommand = 0x0200008b;
    /// <summary>재시작 명령 코드다.</summary>
    public const uint ReloadCommand = 0x0200008d;
    /// <summary>분기 명령 코드다.</summary>
    public const uint ForkCommand = 0x0200008e;
    /// <summary>종료 명령 코드다.</summary>
    public const uint EndCommand = 0x0200008f;
    /// <summary>상위 입력 플래그를 제거하는 마스크다.</summary>
    private const uint CommandMask = 0x03ffffff;

    /// <summary>실제로 도달한 효과가 연결되지 않았으면 명시적으로 실패한다.</summary>
    /// <param name="effect">호출할 효과.</param>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Guide 교체의 실제 수명 효과 연결 누락");

    /// <summary>해제 전에 이름과 정규화된 필드를 캡처한다.</summary>
    /// <param name="name">현재 이름.</param><param name="mode">현재 모드.</param><param name="demo">현재 데모 값.</param>
    public static MissionReloadRequest CaptureReload(string name, uint mode, uint demo) => new(name, mode != 0 ? 1u : 0u, demo != 0 ? 1u : 0u);

    /// <summary>현재 미션을 끝내고 반납한 다음 새 이름을 NUL까지 복사해 생성한다.</summary>
    /// <param name="name">새 미션 이름.</param><param name="hooks">실제 수명 효과.</param>
    public static void Begin(string name, GuideMissionTransitionHooks hooks) => Begin(() => name, hooks);

    /// <summary>빌린 이름을 현재 미션 반납 뒤 읽고 NUL까지 복사해 생성한다.</summary>
    /// <param name="readName">해제 중 내용이 바뀔 수 있는 이름을 읽는 함수.</param><param name="hooks">실제 수명 효과.</param>
    public static void Begin(Func<string> readName, GuideMissionTransitionHooks hooks)
    {
        if (Need(hooks.Current)()) { Need(hooks.Finish)(); Need(hooks.Release)(); }
        string name = readName();
        int nul = name.IndexOf('\0');
        Need(hooks.Create)(nul < 0 ? name : name[..nul]);
    }

    /// <summary>현재 미션이 있으면 가상 종료 없이 한 번 반납한다.</summary>
    /// <param name="hooks">실제 수명 효과.</param>
    public static void Close(GuideMissionTransitionHooks hooks)
    {
        if (Need(hooks.Current)()) Need(hooks.Release)();
    }

    /// <summary>현재 분기 이름을 반납 전에 복사하고 재진입한 현재까지 Begin 규칙으로 처리한다.</summary>
    /// <param name="parameter">부호 있는 DWORD로 해석하는 분기 번호.</param><param name="hooks">실제 수명 효과.</param>
    public static void Fork(uint parameter, GuideMissionTransitionHooks hooks)
    {
        string target = Need(hooks.Get)("fork" + unchecked((int)parameter).ToString(System.Globalization.CultureInfo.InvariantCulture));
        Need(hooks.Finish)(); Need(hooks.Release)(); Begin(target, hooks);
    }

    /// <summary>현재 미션을 반납하고 캡처한 이름을 재생성한 뒤 모드와 데모 값을 복원한다.</summary>
    /// <param name="request">해제 전에 캡처한 값.</param><param name="hooks">실제 수명 효과.</param>
    public static void Restart(MissionReloadRequest request, GuideMissionTransitionHooks hooks)
    {
        Need(hooks.Release)(); Begin(request.Name, hooks); Need(hooks.Mode)(request.Mode);
        if (Need(hooks.Current)()) Need(hooks.Demo)(request.Demo);
    }

    /// <summary>현재 미션을 한 번 종료하고 반납한다.</summary>
    /// <param name="hooks">실제 수명 효과.</param>
    public static void End(GuideMissionTransitionHooks hooks) { Need(hooks.Finish)(); Need(hooks.Release)(); }

    /// <summary>Begin·Restart·Fork·End 입력만 처리하고 처리 여부를 반환한다.</summary>
    /// <param name="input">코드와 세 번째 DWORD를 담은 입력.</param><param name="hooks">각 명령의 실제 처리기.</param>
    public static bool Dispatch(InputEvent input, GuideMissionCommandHooks hooks)
    {
        uint command = input.Code & CommandMask, parameter = unchecked((uint)input.Y);
        switch (command)
        {
            case BeginCommand:
                if (input.Y <= 100) Need(hooks.InvalidBegin)(parameter);
                Need(hooks.Begin)(parameter); return true;
            case ReloadCommand: Need(hooks.Restart)(); return true;
            case ForkCommand: Need(hooks.Fork)(parameter); return true;
            case EndCommand: Need(hooks.End)(); return true;
            default: return false;
        }
    }
}
