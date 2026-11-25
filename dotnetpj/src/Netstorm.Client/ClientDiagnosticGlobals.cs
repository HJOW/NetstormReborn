// Client 진단 전역과 실제 필터·FILE·공유 버퍼·Chat 연결을 둔다.
using Netstorm.Client.Chat;

namespace Netstorm.Client;

// 진단 전역을 담는 네임스페이스다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.

// 선택 trace와 개발자 F7 덤프가 공유하는 Client 수명의 원문 진단 전역이다.
// cpppj client/ClientDiagnostics.h의 ClientTraceGlobals에서 로그·깊이·억제·Chat 연결만 옮겼다.
// 건설 기록·재생(빌드 기록 FILE·재생 객체)은 메뉴·시각·recorder 장치와 함께 후속 단위에서 잇는다.
// 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.
public sealed class ClientTraceGlobals
{
    /// <summary>00542658·005ca8d0 이후의 같은 FILE·버퍼·들여쓰기·필터다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public TraceLogState Log { get; } = new();

    /// <summary>00555a4d0의 최초 BSS0이며 미션 전환 때 보존한다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public uint Depth { get; set; }

    /// <summary>005c8454의 최초 BSS0이며 미션 전환 때 보존한다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public uint Suppress { get; set; }

    /// <summary>00531854의 최초 BSS0이며 미션 전환 때 보존한다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.</summary>
    public uint TraceToChat { get; set; }
}

// 실제 Client의 현재 필터·FILE을 쓰는 진단 로그 진입점이다. 2026-11-25 R4-7c3 진단 전체 Client 연결에서 추가됐다.
public static class ClientDiagnosticLog
{
    /// <summary>같은 장기 Client 전역을 빌리는 실제 필터·FILE 출력이다.</summary>
    /// <param name="globals">같은 수명의 진단 전역이다.</param>
    /// <param name="level">출력 수준이다.</param>
    /// <param name="text">서식 변환된 본문이다.</param>
    /// <param name="hooks">같은 수명의 실제 출력 장치다.</param>
    /// <param name="prefixBytes">원문 format의 선행 색상 쌍 BYTE 수다.</param>
    public static void WriteDiagnosticLog(ClientTraceGlobals globals, int level, string text, TraceLogHooks hooks, int prefixBytes = 0)
    {
        ArgumentNullException.ThrowIfNull(globals);
        RawTraceLog.Write(globals.Log, level, text, prefixBytes, hooks);
    }

    /// <summary>실제 Chat 없이 trace→Chat 전달을 건너뛰는 출력 장치를 만든다.</summary>
    /// <param name="globals">현재 traceToChat을 읽는 진단 전역이다.</param>
    /// <param name="write">현재 공유 버퍼를 받는 쓰기 효과다.</param>
    /// <param name="flush">현재 Stream의 flush 효과다.</param>
    /// <returns>NULL Chat용 출력 장치 묶음이다.</returns>
    public static TraceLogHooks NullChatHooks(ClientTraceGlobals globals, Action<Stream, string> write, Action<Stream> flush)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(write);
        ArgumentNullException.ThrowIfNull(flush);
        return new TraceLogHooks
        {
            Console = text => TraceChatBody.ForwardTraceToChat(globals.TraceToChat, text, new TraceChatHooks
            {
                Color = () => 0u,
                CurrentChat = () => 0u,
                Append = (_, _, _) => throw new InvalidOperationException("NULL Chat의 실제 UI 덧붙임 미연결"),
            }),
            Write = write,
            Flush = flush,
        };
    }
}
