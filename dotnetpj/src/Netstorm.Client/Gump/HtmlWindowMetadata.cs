using System.Text;
using System.Text.RegularExpressions;
using Netstorm.Assets;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Gump; // 원문 HTML 메타의 고정 검색 우선순위/부분 복사/현재 UI 효과를 유지한다.

/// <summary>실제 맵 목록 작성기가 같은 부모 열 수 필드를 직접 빌린다.</summary>
public delegate void HtmlMapMenu(string label, ref uint columns);
/// <summary>현재 실제 부모/공통 UI/80칸 고리와 선택된 목록·입력 장치.</summary>
public sealed class HtmlWindowMetadataHooks
{
    /// <summary>같은 실제 HTML 부모.</summary>
    public Func<uint, HtmlWindowGumpNode?>? Node { get; set; }
    /// <summary>실제20행 현재 버튼 삭제.</summary>
    public Action<uint>? ClearButtons { get; set; }
    /// <summary>실제 label/command/parameter/Instant 돌 버튼 추가.</summary>
    public Action<uint, string, uint, uint, uint>? AddButton { get; set; }
    /// <summary>현재 원문 이름 조회/코드0 진단.</summary>
    public ScriptCommandHooks Commands { get; set; } = new();
    /// <summary>실제80칸 고리에 즉시 문자열 복사.</summary>
    public Func<string, uint>? StoreText { get; set; }
    /// <summary>현재 세로 controlled 메뉴 시작.</summary>
    public Action<uint>? BeginMenu { get; set; }
    /// <summary>현재 일반 메뉴 항목.</summary>
    public Action<string, uint, uint>? MenuText { get; set; }
    /// <summary>실제 checked/label/command/parameter/enabled 메뉴 항목.</summary>
    public Action<int, string, uint, uint, bool>? Checked { get; set; }
    /// <summary>실제 맵 목록과 같은 부모 열 수 참조.</summary>
    public HtmlMapMenu? Maps { get; set; }
    /// <summary>현재 기술 목록.</summary>
    public Action<uint>? Tech { get; set; }
    /// <summary>현재 TYPE/테마 기술 목록.</summary>
    public Action<uint, int>? ThemeTech { get; set; }
    /// <summary>@ 목록의 실제 키/명령/원시 인자.</summary>
    public Action<string, uint, string>? NamedList { get; set; }
    /// <summary># 목록의 실제 키/명령/원시 인자.</summary>
    public Action<string, uint, string>? FileList { get; set; }
    /// <summary>% 목록의 현재 키/원시 명령/인자로 얻는 문자열.</summary>
    public Func<string, string, string, string>? InlineList { get; set; }
    /// <summary>현재 벽 시각.</summary>
    public Func<double>? WallSeconds { get; set; }
    /// <summary>공통 실제 idle 등록.</summary>
    public Action<uint, bool>? Idle { get; set; }
    /// <summary>현재 원본 주소의 DWORD 전역.</summary>
    public Func<uint, uint>? Read { get; set; }
    /// <summary>현재 limit/label/argument의 실제 문자 입력창.</summary>
    public Action<int, string, string>? Input { get; set; }
    /// <summary>원문 Zsend의 선택 호출 경계.실제 멀티플레이 연결은 후순위다.</summary>
    public Action<string, string, string>? Send { get; set; }
    /// <summary>Input 효과 뒤 현재 MenuJustClosed를 쓰는 원시 큐.</summary>
    public Action<uint, uint, uint>? Queue { get; set; }
}
/// <summary>cpppj/src/client/HtmlWindowMetadata.h/.cpp의004c83b0.실제 mutable8192BYTE의 뒤쪽 물리까지 보존한다.</summary>
public sealed class HtmlWindowMetadata
{
    /// <summary>원문9개 정규식.종류가 앞선 일치는 더 이른 뒤 종류보다 우선한다.</summary>
    private static readonly Regex[] Patterns = [.. new[] { @"\$Button[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)", @"\$ButtonInstant[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)", @"\$Menu[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)", @"\$Checked[ \t]*=[ \t]*([^,]+),([^,]+),([^,]+),([^,]+),([^$ ]+)", @"\$Timeout[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)", @"\$OnExit[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)", @"\$Key[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)", @"\$Input[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)", @"\$Zsend[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)" }.Select(pattern => new Regex(pattern, RegexOptions.CultureInvariant))];
    /// <summary>같은 현재 부모/선택 효과를 빌린다.</summary>
    private readonly HtmlWindowMetadataHooks _hooks;
    /// <summary>해당 원문 분기에 도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("HTML 메타 실제 장치 미연결");
    /// <summary>실제 부모 조회만 미리 요구한다.</summary>
    public HtmlWindowMetadata(HtmlWindowMetadataHooks hooks) { ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); _hooks = hooks; }
    /// <summary>같은 현재 실제 부모를 읽는다.</summary>
    private HtmlWindowGumpNode Node(uint token) => (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("HTML 메타 현재 부모 없음");
    /// <summary>현재 물리 버퍼 안의 첫 NUL을 찾는다.</summary>
    private static int Length(Memory<byte> buffer) { int length = buffer.Span.IndexOf((byte)0); return length >= 0 ? length : throw new ArgumentException("HTML 메타 실제 버퍼 NUL 범위 초과"); }
    /// <summary>원문 캡처 지역 버퍼의 마지막 NUL 공간을 요구한다.</summary>
    private static string Capture(Group group, int capacity) => group.Length < capacity ? group.Value : throw new ArgumentException("HTML 메타 원문 지역 캡처 NUL 범위 초과");
    /// <summary>버튼 처리 수 또는 현재 메뉴 열이 있으면1.각 효과 뒤 현재 버퍼/필드를 다시 읽는다.</summary>
    public uint Process(uint token, Memory<byte> buffer)
    {
        Node(token); Require(_hooks.ClearButtons)(token); uint buttons = 0;
        // 매 제거/치환 뒤 항상 첫 종류 Button부터 다시 검색한다.
        for (;;) {
            int length = Length(buffer), kind = 0; string source = Encoding.Latin1.GetString(buffer.Span[..length]); Match? match = null;
            // 원문9개 종류의 고정 우선순위로 각각 첫 일치만 선택한다.
            for (; kind < Patterns.Length; kind++) { var candidate = Patterns[kind].Match(source); if (candidate.Success) { match = candidate; break; } }
            if (match is null) break; string label = Capture(match.Groups[1], 80), command = Capture(match.Groups[2], 80), argument = Capture(match.Groups[3], 256); var resolved = new DialogExitCommandState(); DialogExitCommand.Resolve(resolved, command, argument, new() { Commands = _hooks.Commands, StoreText = _hooks.StoreText }); string replacement = ""; bool insert = false;
            if (kind == 3) { string check = Capture(match.Groups[4], 256), enabled = Capture(match.Groups[5], 256); if (Node(token).MenuColumns == 0) { Require(_hooks.BeginMenu)(0x40000); Node(token).MenuColumns = unchecked(Node(token).MenuColumns + 1); } bool allow = ConfigNumbers.ParseLong(enabled) != 0 || Require(_hooks.Read)(0x5cab30) != 0 || Require(_hooks.Read)(0x55a480) != 0; Require(_hooks.Checked)(ConfigNumbers.ParseLong(check), label, resolved.Command, resolved.Parameter, allow); }
            else if (kind == 2) {
                if (Node(token).MenuColumns == 0) { Require(_hooks.BeginMenu)(0x40000); Node(token).MenuColumns = unchecked(Node(token).MenuColumns + 1); } string lower = string.Concat(label.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 'a' - 'A') : c));
                if (lower is "*sololist*" or "*fortlist*") Require(_hooks.Maps)(label, ref Node(token).MenuColumns);
                else if (lower == "*techlist*") Require(_hooks.Tech)(resolved.Parameter);
                else if (lower == "*themetechlist*") Require(_hooks.ThemeTech)(resolved.Parameter & 0xffff, unchecked((int)resolved.Parameter) >> 16);
                else if (label.Length != 0 && label[0] == '@') Require(_hooks.NamedList)(label[1..], resolved.Command, argument);
                else if (label.Length != 0 && label[0] == '#') Require(_hooks.FileList)(label[1..], resolved.Command, argument);
                else if (label.Length != 0 && label[0] == '%') { replacement = Require(_hooks.InlineList)(label[1..], command, argument); int end = replacement.IndexOf('\0'); if (end >= 0) replacement = replacement[..end]; insert = true; }
                else Require(_hooks.MenuText)(label, resolved.Command, resolved.Parameter);
            } else if (kind == 4) { int seconds = ConfigNumbers.ParseLong(label); double deadline = seconds + Require(_hooks.WallSeconds)(); Node(token).TimeoutCommand = resolved.Command; Node(token).TimeoutParameter = resolved.Parameter; Node(token).TimeoutSeconds = deadline; Require(_hooks.Idle)(token, true); }
            else if (kind == 5) { Node(token).OnExitCommand = resolved.Command; Node(token).OnExitParameter = resolved.Parameter; }
            else if (kind == 7) { Require(_hooks.Input)(ConfigNumbers.ParseLong(command), label, argument); Require(_hooks.Queue)(Require(_hooks.Read)(0x5429ec), 0, 0); }
            else if (kind == 8) Require(_hooks.Send)(label, command, argument);
            else if (kind != 6) { Require(_hooks.AddButton)(token, label, resolved.Command, resolved.Parameter, kind == 1 ? 1U : 0U); buttons = unchecked(buttons + 1); }
            int start = match.Index, currentLength = Length(buffer); if (start > currentLength) throw new ArgumentOutOfRangeException(nameof(buffer), "HTML 메타 현재 일치 시작 범위 초과"); int finish = start + (buffer.Span[start] == '$' ? 1 : 0);
            // 캡처 끝 대신 현재 다음 LF/$/NUL까지 지운다.
            while (finish < currentLength && buffer.Span[finish] != '\n' && buffer.Span[finish] != '$') finish++; if (finish < currentLength && buffer.Span[finish] == '\n') finish++; buffer.Span.Slice(finish, currentLength - finish + 1).CopyTo(buffer.Span[start..]);
            if (insert) { string merged = Encoding.Latin1.GetString(buffer.Span[..Length(buffer)]).Insert(start, replacement); if (replacement.Length > buffer.Length) throw new ArgumentException("HTML 메타 원문 치환 복사 범위 초과"); Encoding.Latin1.GetBytes(merged.AsSpan(0, replacement.Length), buffer.Span); }
        }
        return buttons != 0 || Node(token).MenuColumns != 0 ? 1U : 0U;
    }
    /// <summary>같은 실제 창/고리/공통 idle 장치를 연결한다.선택 목록과 명령 표는 상위가 공급한다.</summary>
    public static void BindHtmlWindowMetadataDevices(HtmlWindowMetadataHooks hooks, HtmlWindowGump window, ScriptStringRing strings, GumpLifetime lifetime) { hooks.ClearButtons = window.ClearButtons; hooks.AddButton = window.AddButton; hooks.StoreText = strings.Store; hooks.Idle = lifetime.SetIdle; }
    /// <summary>현재 controlled 메뉴의 일반/체크 항목을 원문 고정 플래그로 연결한다.</summary>
    public static void BindHtmlWindowMetadataMenus(HtmlWindowMetadataHooks hooks, MenuGumpWidgets widgets, MenuGumpControls controls) { hooks.BeginMenu = flags => widgets.BeginVertical(flags); hooks.MenuText = (label, command, parameter) => controls.AddText(Encoding.Latin1.GetBytes(label), command, parameter, true, true, false); hooks.Checked = (check, label, command, parameter, enabled) => controls.AddChecked(check != 0, Encoding.Latin1.GetBytes(label), command, parameter, true, enabled); }
}
