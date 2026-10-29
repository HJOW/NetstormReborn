using Netstorm.Assets;
using Netstorm.Client.Input;

namespace Netstorm.Client.Scripting; // 메타 해석 시 명령/인자를 고정하고 실제 페이지 반납에 예약한다.

/// <summary>원본 HTML 부모+21e0/+21e4와 같은 해석 결과.0xffffffff를 별도 거부하지 않는다.</summary>
public sealed class DialogExitCommandState
{
    /// <summary>0일 때 소멸 예약을 하지 않는 원시 명령.</summary>
    public uint Command { get; set; }
    /// <summary>숫자 또는 현재80칸 고리의 문자열 토큰.</summary>
    public uint Parameter { get; set; }
}
/// <summary>같은 실제 명령 표와 문자열 고리의 선택 경계.</summary>
public sealed class DialogExitResolveHooks
{
    /// <summary>현재 이름 조회/코드0 진단.</summary>
    public ScriptCommandHooks Commands { get; set; } = new();
    /// <summary>현재 고리에 즉시 소유 복사한다.</summary>
    public Func<string, uint>? StoreText { get; set; }
}
/// <summary>cpppj/src/client/DialogExitCommand.h/.cpp의 캡처/현재 조회/인자 해석/단일 반납.</summary>
public static class DialogExitCommand
{
    /// <summary>첫 NUL 앞만 남기고 실제 원문 지역 버퍼의 NUL 한 칸을 확보한다.</summary>
    private static string Captured(string source, int capacity) { int end = source.IndexOf('\0'); if (end >= 0) source = source[..end]; if (source.Length >= capacity) throw new ArgumentException("OnExit 메타 캡처의 원문 지역 NUL 범위 초과"); return source; }
    /// <summary>lookup→코드0 진단→첫 BYTE 숫자 관문→atol/고리→두 필드 저장.공백/따옴표를 다듬지 않는다.</summary>
    public static void Resolve(DialogExitCommandState state, string command, string argument, DialogExitResolveHooks hooks)
    {
        command = Captured(command, 80); argument = Captured(argument, 256); uint code = (hooks.Commands.Lookup ?? throw new InvalidOperationException("OnExit 현재 명령 표 조회 누락"))(command); if (code == 0) (hooks.Commands.Unknown ?? throw new InvalidOperationException("OnExit 코드0 진단 미연결"))(command); uint parameter = argument.Length != 0 && argument[0] is >= '0' and <= '9' ? unchecked((uint)ConfigNumbers.ParseLong(argument)) : (hooks.StoreText ?? throw new InvalidOperationException("OnExit 현재 문자열 고리 미연결"))(argument); state.Command = code; state.Parameter = parameter;
    }
    /// <summary>선택된 큐를 먼저 검증하고 재진입 전에 두 필드를 비운다.원본 창 소멸자 자체와는 별도 페이지 수명 helper다.</summary>
    public static void Dispose(DialogExitCommandState state, Action<uint, uint, uint>? queue) { if (state.Command != 0 && queue is null) throw new InvalidOperationException("OnExit 실제 입력 큐 미연결"); uint command = state.Command, parameter = state.Parameter; state.Command = state.Parameter = 0; if (command != 0) queue!(command, parameter, parameter); }
    /// <summary>같은40칸 원시 입력 큐에 두 DWORD의 비트를 보존한다.</summary>
    public static void Dispose(DialogExitCommandState state, InputQueue queue) => Dispose(state, (command, first, second) => queue.Push(command, unchecked((int)first), unchecked((int)second)));
    /// <summary>실제80칸 고리를 빌리는 현재 해석 경계.</summary>
    public static DialogExitResolveHooks MakeHooks(ScriptStringRing strings, ScriptCommandHooks commands) => new() { Commands = commands, StoreText = strings.Store };
}
