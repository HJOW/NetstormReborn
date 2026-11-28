using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Client.Chat; // 시간 알림과 임시 Chat 서식의 같은 현재 상태를 처리한다.

/// <summary>Client의 00544d68/60·0054db9c/dba0이다. 미션/장치 교체에서 초기화하지 않는다.</summary>
public sealed class MonitorTimeState
{
    /// <summary>최초 bit0만 켜며 상위 비트는 보존하는 원문 초기화 표식이다.</summary>
    public uint Initialized;
    /// <summary>같은 실제 기한의 저장소다. 최초 guard와 만료 후만 쓴다.</summary>
    public DoubleBox DeadlineBox { get; } = new();
    /// <summary>독립 원문 검사와 실제 프레임이 같은8BYTE를 읽고 쓴다.</summary>
    public double Deadline { get => DeadlineBox.Value; set => DeadlineBox.Value = value; }
    /// <summary>현재 monitor 옵션의 전체 signed DWORD다.</summary>
    public int Monitor;
    /// <summary>현재 alert 옵션의 전체 signed DWORD다.</summary>
    public int Alert;
}

/// <summary>원문 소비 위치에서만 요구하는 현재 시계/달력/이름/설정/제출 경계다.</summary>
public sealed class MonitorTimeHooks
{
    /// <summary>호출마다 현재 벽시각을 읽는다. 프레임 캐시나 게임 정지 시각과 구별한다.</summary>
    public Func<double>? WallSeconds { get; init; }
    /// <summary>실제 time32→localtime32→asctime의 끝개행 포함 문자열이다.</summary>
    public Func<string>? Calendar { get; init; }
    /// <summary>달력 변환 뒤 현재 이름을 다시 읽는다.</summary>
    public Func<string>? Handle { get; init; }
    /// <summary>원문 고정 서식 경계다. 기본은 같은256BYTE 문자열 본체다.</summary>
    public Func<string, string, string>? Line { get; init; } = RawMonitorTime.FormatLine;
    /// <summary>현재 monitorFormat을 세 치환 인자0으로 읽는다.</summary>
    public Func<string>? Format { get; init; }
    /// <summary>현재 실제 Chat manager의 임시 서식 제출 부모다.</summary>
    public Action<string, string>? Submit { get; init; }
}

/// <summary>현재 manager 토큰의 같은 물리와 일반 제출만 빌린다.</summary>
public sealed class TemporaryChatFormatHooks
{
    /// <summary>호출마다 현재 토큰의 살아 있는 실제 바이트를 빌린다.</summary>
    public Func<uint, Memory<byte>>? Memory { get; init; }
    /// <summary>서식 변경 뒤 현재 manager 토큰으로 일반 제출을 실행한다.</summary>
    public Action<uint, string>? Submit { get; init; }
}

/// <summary>cpppj RawMonitorTime.cpp의 guard·180초 기한·정상 복귀에서만 서식 복원하는 본체다.</summary>
public static class RawMonitorTime
{
    /// <summary>두 판본의 원문 double180초 간격이다.</summary>
    public const double Interval = 180.0;
    /// <summary>미선택 경계를 조기 요구하거나 빈 성공으로 채우지 않는다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("시간 알림의 실제 장치 미연결");
    /// <summary>CRT 문자열처럼 첫NUL 앞의 원래 BYTE 문자열만 빌린다.</summary>
    private static string CString(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>이름과 영문 달력을 원문 %s: !%s\n으로 결합한다. 달력 자체의 끝개행도 보존한다.</summary>
    public static string FormatLine(string handle, string calendar)
    {
        string line = CString(handle) + ": !" + CString(calendar) + '\n';
        if (line.Length >= 256) throw new ArgumentOutOfRangeException(nameof(line), "시간 알림 원문 지역256BYTE 범위");
        return line;
    }

    /// <summary>초기화→활성→현재 시계→달력/현재 이름/서식/제출→현재 시계 재예약 순서다. 예외 접두를 되돌리지 않는다.</summary>
    public static void Step(MonitorTimeState state, MonitorTimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if ((state.Initialized & 1) == 0) { state.Initialized |= 1; state.Deadline = 0.0; }
        if (state.Monitor == 0 && state.Alert == 0) { if (state.Deadline == 0.0) state.Deadline = 0.0; return; }
        if (state.Deadline == 0.0) state.Deadline = Need(hooks.WallSeconds)() + Interval;
        double now = Need(hooks.WallSeconds)();
        if (!(state.Deadline <= now)) return;
        string calendar = Need(hooks.Calendar)(); string handle = Need(hooks.Handle)();
        string line = Need(hooks.Line)(handle, calendar); string format = Need(hooks.Format)();
        Need(hooks.Submit)(format, line);
        state.Deadline = Need(hooks.WallSeconds)() + Interval;
    }

    /// <summary>현재 manager 범위에서 NUL 앞 문자열을 읽고 지역80BYTE 초과를 거부한다.</summary>
    private static string Front(Memory<byte> memory, int offset)
    {
        if (offset >= memory.Length) throw new ArgumentOutOfRangeException(nameof(offset), "임시 Chat front 물리 부족");
        ReadOnlySpan<byte> view = memory.Span[offset..]; int end = view.IndexOf((byte)0);
        if (end < 0 || end >= 80) throw new ArgumentOutOfRangeException(nameof(memory), "임시 Chat front 지역80BYTE 범위");
        return Encoding.Latin1.GetString(view[..end]);
    }
    /// <summary>같은 실제 물리에 strncpy60/16 padding과 마지막BYTE0을 적용한다.</summary>
    private static void SetFront(Memory<byte> memory, int offset, int count, string text)
    {
        if (offset > memory.Length || count > memory.Length - offset) throw new ArgumentOutOfRangeException(nameof(memory), "임시 Chat front setter 범위");
        RawChatManagerPrefix.CopyChatManagerFormat(memory.Span[offset..], text, (uint)count); memory.Span[offset + count - 1] = 0;
    }

    /// <summary>첫 front를 보관한 뒤 임시 서식으로 제출한다. 정상 복귀 뒤 현재 manager에만 복원하며 실패에서는 임시 변경이 남는다.</summary>
    public static void SubmitTemporary(UIntBox current, string format, string line, OriginalEdition edition, TemporaryChatFormatHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(current); ArgumentNullException.ThrowIfNull(hooks);
        int offset = edition == OriginalEdition.Patch1078 ? 0x14 : 0x10, count = edition == OriginalEdition.Patch1078 ? 60 : 16;
        uint initial = current.Value; string saved = Front(Need(hooks.Memory)(initial), offset);
        SetFront(Need(hooks.Memory)(initial), offset, count, format);
        Need(hooks.Submit)(current.Value, line);
        SetFront(Need(hooks.Memory)(current.Value), offset, count, saved);
    }
}
