using Netstorm.Assets;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Scripting;

/// <summary>
/// HTML이 예약한 명령과 Config 명령을 원본 토큰/입력 큐에 연결한다.
/// 근거: cpppj/src/client/ScriptCommand.h/.cpp (원본 004c9090·00441580). (R3k-9, 2026-10-30)
/// 실제 명령 표 조회·진단과 Config 쓰기는 호출자가 후크로 공급한다.
/// ConfigInterface 직접 연결 overload는 전체 ConfigInterface 이식 때 잇는다.
/// </summary>
public static class ScriptCommand
{
    /// <summary>PATCH 원본 명령 표의 Config 코드.</summary>
    public const uint ScriptConfig = 0x0200006d;

    /// <summary>PATCH 원본 명령 표의 Warning 코드.</summary>
    public const uint ScriptWarning = 0x02000102;

    /// <summary>
    /// 원본 004c9090. 명령 이름으로 표를 조회해 입력 큐에 명령 사건으로 넣는다.
    /// 숫자로 시작하는 인자만 atol로 바꾸고 그 외는 문자열 고리에 저장한다. -1 조회도 인자 처리를 마친 뒤 거부한다.
    /// </summary>
    /// <param name="source">쉼표로 이은 "명령,인자" 원시 바이트 문자열.</param>
    /// <param name="queue">명령 사건을 받는 40칸 입력 큐.</param>
    /// <param name="strings">인자 문자열 수명을 관리하는 80칸 고리.</param>
    /// <param name="hooks">현재 명령 표 조회와 미등록 진단.</param>
    public static void QueueScriptCommand(string source, InputQueue queue, ScriptStringRing strings, ScriptCommandHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentNullException.ThrowIfNull(queue);
        ArgumentNullException.ThrowIfNull(strings);
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Lookup is null)
        {
            throw new ArgumentException("스크립트 명령 표 조회 미연결", nameof(hooks));
        }
        // 첫 필드는 원본 네 구분자를 쓰고 두 번째 필드의 쉼표는 보존한다
        RawTextTokenResult command = RawTextToken.Read(source, 0, "#,;>");
        RawTextTokenResult argument = RawTextToken.Read(source, command.Next, ";>");
        // 인자 변환 전에 실제 표를 조회한다
        uint code = hooks.Lookup(command.Text);
        // 실제 lookup 0의 진단과 lookup -1의 지연 거부는 다른 분기다
        void Diagnose()
        {
            if (hooks.Unknown is null)
            {
                throw new InvalidOperationException("스크립트 미등록 명령 진단 미연결");
            }
            hooks.Unknown(command.Text);
        }
        if (code == 0)
        {
            Diagnose();
        }
        // 부호로 시작하면 문자열 고리에 저장한다
        uint parameter = argument.Text.Length != 0 && argument.Text[0] >= '0' && argument.Text[0] <= '9'
            ? unchecked((uint)ConfigNumbers.ParseLong(argument.Text))
            : strings.Store(argument.Text);
        if (code == 0xffffffffU)
        {
            Diagnose();
            return;
        }
        queue.PushCommand(code, unchecked((int)parameter), unchecked((int)parameter));
    }

    /// <summary>
    /// 원본 00441580. 원본 '='·개행 토큰으로 한 설정만 바꿔 onChanged를 호출하고 항상 0을 반환한다.
    /// 인용 공백 키도 원본이 받은 그대로 실제 Set에 전달하며 조회 문법으로 재해석하지 않는다.
    /// </summary>
    /// <param name="source">"키=값" 원시 바이트 문자열. 값 안의 '='와 ';'는 자르지 않는다.</param>
    /// <param name="set">실제 설정 쓰기. 키·값을 받아 성공 여부를 돌려준다.</param>
    /// <param name="noKeyword">키가 비었을 때 부르는 없음 알림. 없으면 그냥 0을 반환한다.</param>
    public static int ApplyScriptConfig(string source, Func<string, string, bool> set, Action? noKeyword = null)
    {
        ArgumentNullException.ThrowIfNull(source);
        // 키는 따옴표/78입력 제한까지 공통 helper와 같다
        RawTextTokenResult keyword = RawTextToken.Read(source, 0, "=");
        RawTextTokenResult value = RawTextToken.Read(source, keyword.Next, "\n");
        if (keyword.Text.Length == 0)
        {
            noKeyword?.Invoke();
            return 0;
        }
        if (set is null)
        {
            throw new InvalidOperationException("스크립트 Config의 실제 설정 쓰기 미연결");
        }
        set(keyword.Text, value.Text);
        return 0;
    }
}
