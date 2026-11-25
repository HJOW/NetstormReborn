// 원문 세 일반 명령을 기존 세계 간헐천 생성과 실제 AI 직접 상태 writer에 전달한다.
namespace Netstorm.Client.Input;

// Guide/Gump/뗌 뒤이며 Console/공통 정지보다 앞이다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.
// 근거: cpppj/src/client/AiUtilityCommandInput.h/.cpp다.
public static class AiUtilityCommandInput
{
    /// <summary>5429a8의 원문 초기 DWORD다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    public const uint CreateInitialGeysersCommand = 0x020000adU;

    /// <summary>5429f8의 원문 초기 DWORD다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    public const uint BrainCrankMoneyCommand = 0x020000c1U;

    /// <summary>5428d8의 원문 초기 DWORD다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    public const uint AiParamSetCommand = 0x02000078U;

    /// <summary>원문 이름 lookup의 ASCII 대소문자 무시 계약을 사용한다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    /// <param name="value">검사할 문자다.</param>
    /// <returns>소문자로 바꾼 문자다.</returns>
    private static char AsciiLower(char value) => value is >= 'A' and <= 'Z' ? (char)(value + 32) : value;

    /// <summary>실제 세 이름만 ASCII 대소문자를 무시하고 조회한다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    /// <param name="name">조회할 이름이다.</param>
    /// <returns>세 명령값 중 하나며 없으면 0xffffffff다.</returns>
    public static uint AiUtilityCommandScriptCode(string name)
    {
        ArgumentNullException.ThrowIfNull(name);
        // 원문 이름 lookup과 같이 ASCII 대소문자만 무시하는 반복문이다.
        char[] lowered = new char[name.Length];
        for (int index = 0; index < name.Length; index++)
        {
            lowered[index] = AsciiLower(name[index]);
        }

        string key = new(lowered);
        if (key == "createinitialgeysers")
        {
            return CreateInitialGeysersCommand;
        }

        if (key == "braincrankmoney")
        {
            return BrainCrankMoneyCommand;
        }

        if (key == "aiparamset")
        {
            return AiParamSetCommand;
        }

        return 0xffffffffU;
    }

    /// <summary>호출자가 modifier/outside를 제거한 원문 명령값 세 개만 분류한다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    /// <param name="code">검사할 명령값이다.</param>
    /// <returns>세 명령 중 하나면 참이다.</returns>
    public static bool IsAiUtilityCommand(uint code) => code == CreateInitialGeysersCommand || code == BrainCrankMoneyCommand || code == AiParamSetCommand;

    /// <summary>선택된 실제 하위 누락을 빈 성공으로 바꾸지 않는다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    /// <param name="hook">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static Action Need(Action? hook) => hook ?? throw new InvalidOperationException("AI/간헐천 일반 명령의 실제 부모 미연결");

    /// <summary>선택된 실제 하위 누락을 빈 성공으로 바꾸지 않는다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    /// <param name="hook">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static Action<uint> Need(Action<uint>? hook) => hook ?? throw new InvalidOperationException("AI/간헐천 일반 명령의 실제 부모 미연결");

    /// <summary>x는 원문 세 명령에서 사용하지 않으며 parameter만 y의 전체 32비트를 사용한다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    /// <param name="event">받은 입력 사건이다.</param>
    /// <param name="hooks">선택한 분기에 도달한 실제 부모다.</param>
    /// <returns>소비했으면 참이다.</returns>
    public static bool DispatchAiUtilityCommandInput(InputEvent @event, AiUtilityCommandHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint code = @event.Code & 0x03ffffffU;
        if (!IsAiUtilityCommand(code))
        {
            return false;
        }

        if ((@event.Code & InputCode.Release) != 0)
        {
            return true;
        }

        if (code == CreateInitialGeysersCommand)
        {
            Need(hooks.InitialGeysers)();
        }
        else if (code == BrainCrankMoneyCommand)
        {
            Need(hooks.CrankMoney)();
        }
        else
        {
            Need(hooks.Parameter)(unchecked((uint)@event.Y));
        }

        return true;
    }
}

// 선택한 분기에 도달한 실제 부모만 필수로 요구한다.
public sealed class AiUtilityCommandHooks
{
    /// <summary>00486440의 기존 전체 GenerateWorldGeysers이며 local 생성 하나로 줄이지 않는다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    public Action? InitialGeysers { get; set; }

    /// <summary>00413c60의 현재 Player1..8과 실제 brain+2d8 직접 쓰기다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    public Action? CrankMoney { get; set; }

    /// <summary>004143b0에 param2 전체 DWORD를 전달하며 low3 owner를 바꾸지 않는다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    public Action<uint>? Parameter { get; set; }
}
