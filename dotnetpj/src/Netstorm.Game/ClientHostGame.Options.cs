using System.Text;
using Netstorm.Client.Chat;

// 실제 State 정리가 현재 복원 옵션 부모와 같은 Client 진단을 재사용한다.
namespace Netstorm.Client.Host;

/// <summary>장기 값/기록/창 기하/실제 Graph 진단의 공유 조립이다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>Session/콘텐츠 재연결에서 새로 만들지 않는 원문 옵션 값이다.</summary>
    private readonly ClientOptionGlobals _nativeOptionGlobals = new();
    /// <summary>첫 Config/시계/기록 장치 구성 뒤 설치하는 같은 부모다.</summary>
    private readonly ClientOptionsInterpreter? _nativeOptionsInterpreter;
    /// <summary>조회 자체로 설정/창/기록을 바꾸지 않는 같은 장기 값이다.</summary>
    public ClientOptionGlobals NativeOptions => _nativeOptionGlobals;
    /// <summary>현재 복원 CPP의435220 순서를 실행한다. 실제 FSR 장치가 연결되기 전에는 해당 출력을 허용하지 않는다.</summary>
    public void InterpretNativeOptions() => (_nativeOptionsInterpreter ?? throw new InvalidOperationException("실제 전체 옵션 부모 미설치"))
        .Interpret(false, WindowsClientWindowOptions.CreateHooks(() =>
        {
            if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 옵션 HWND 선택은 Windows 전용입니다");
            return WindowsSdlWindow.NativeHandle(Window.Handle);
        }, MoveNativeOptionWindow));
    /// <summary>SDL 고정 크기 제한을 실제 이동 동안만 풀고 원래 사용자 크기 조절 허용 상태를 정상/예외 뒤 복원한다.</summary>
    private void MoveNativeOptionWindow(Action move)
    {
        bool previous = Window.AllowUserResizing;
        try { if (!previous) Window.AllowUserResizing = true; move(); }
        finally { if (!previous) Window.AllowUserResizing = false; }
    }
    /// <summary>같은 현재 필터/FILE/공유 버퍼와 실제 현재 Chat으로 Graph 등의 원문 BYTE 로그를 출력한다.</summary>
    public void WriteNativeDiagnosticLog(int level, string text)
    {
        ClientTraceGlobals trace = _nativeOptionGlobals.Trace;
        ClientDiagnosticLog.WriteDiagnosticLog(trace, level, text, new()
        {
            Console = format => TraceChatBody.ForwardTraceToChat(trace.TraceToChat, format, new()
            {
                Color = () => _driver.Screen.Colors().Named[9].Index,
                CurrentChat = () => _chatWindowGlobals.Current.Value,
                Append = (root, value, context) => (_nativeChatWindows ?? throw new InvalidOperationException("추적의 현재 실제 Chat 장치 미설치")).Window.AppendLog(root, value, context),
            }),
            Write = (stream, value) => stream.Write(Encoding.Latin1.GetBytes(value)), Flush = stream => stream.Flush(),
        });
    }
}
