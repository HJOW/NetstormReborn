using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 다중 줄 텍스트의 표시/스크롤/선택 영역이다.

/// <summary>같은 기본 Gump 저장소의 다중 줄 파생 필드. 생성/수명/입력 조립은 Widgets 계층이 맡는다.</summary>
public class ScrollingTextGumpNode : GumpCreateNode
{
    /// <summary>현재 슬라이더 토큰.</summary>
    public uint Slider { get; set; }
    /// <summary>현재 연결 버퍼 토큰.</summary>
    public uint Buffer { get; set; }
    /// <summary>현재 적용 스크롤.</summary>
    public int Scroll { get; set; }
    /// <summary>-1없음/-2아래/그 외 위치 요청.</summary>
    public int ScrollRequest { get; set; } = -1;
    /// <summary>아래 요청의 원본 인자. 원본 ScrollBottom 몸체는 읽지 않는다.</summary>
    public int BottomPadding { get; set; }
    /// <summary>1변경/2슬라이더 사용.</summary>
    public uint TextFlags { get; set; }
    /// <summary>연결 버퍼 소유 여부.</summary>
    public uint OwnsBuffer { get; set; }
    /// <summary>원본 미작성 필드.</summary>
    public uint Unwritten { get; set; }
    /// <summary>두 원본 보조 DWORD.</summary>
    public uint[] Reserved { get; } = new uint[2];
    /// <summary>마지막 배치 폭.</summary>
    public float LayoutWidth { get; set; }
    /// <summary>이전 드래그 Y.</summary>
    public int DragY { get; set; }
    /// <summary>현재 본문 태그.</summary>
    public uint Metadata { get; set; }
    /// <summary>본문 콜백 식별자.</summary>
    public uint Callback { get; set; }
}
/// <summary>같은 실제 생성에 사용한 상태/처리기. 소유권은 생성 계층에 있다.</summary>
/// <param name="State">실제 버퍼 상태.</param><param name="Handler">같은 상태의 처리기.</param>
public readonly record struct TextGumpBufferBinding(StyleTextBufferState State, StyleTextBuffer Handler);
/// <summary>같은 노드/버퍼 저장소와 현재 슬라이더 가상 동작.</summary>
public sealed class ScrollingTextGumpHooks
{
    /// <summary>현재 다중 줄 파생 노드.</summary>
    public Func<uint, ScrollingTextGumpNode?>? Node { get; set; }
    /// <summary>같은 기본 노드 또는 슬라이더.</summary>
    public Func<uint, GumpCreateNode?>? BaseNode { get; set; }
    /// <summary>현재 연결 버퍼.</summary>
    public Func<uint, TextGumpBufferBinding>? Buffer { get; set; }
    /// <summary>슬라이더 범위 쓰기.</summary>
    public Action<uint, int, int>? SliderRange { get; set; }
    /// <summary>슬라이더 페이지 쓰기.</summary>
    public Action<uint, int>? SliderPage { get; set; }
    /// <summary>슬라이더 현재 위치 쓰기.</summary>
    public Action<uint, int>? SliderSetPosition { get; set; }
    /// <summary>현재 슬라이더 위치 읽기.</summary>
    public Func<uint, int>? SliderPosition { get; set; }
    /// <summary>현재 가상 Redraw(false).</summary>
    public Action<uint, bool>? Redraw { get; set; }
}
/// <summary>cpppj/src/client/TextGump.cpp의 다중 줄 부분. 저장 y/누적 높이, 현재 필드 재조회, 판본별 float/NaN 분기를 보존한다.</summary>
public sealed class ScrollingTextGump
{
    /// <summary>실제 등록/판본 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>같은 노드 위치/경계 수집기.</summary>
    private readonly GumpCollection _collection;
    /// <summary>같은 현재 논리/VFX 클립.</summary>
    private readonly GumpDrawState _clip;
    /// <summary>버퍼/StyleText와 같은 공통 조각 표.</summary>
    private readonly StyleTextState _fragments;
    /// <summary>실제 글자/그림 가상 처리기.</summary>
    private readonly StyleText _style;
    /// <summary>현재 조회/슬라이더/재그리기 경계.</summary>
    private readonly ScrollingTextGumpHooks _hooks;
    /// <summary>실제 도달하는 가상 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("다중 줄 TextGump 가상 경계 필요");
    /// <summary>현재 같은 파생/기본/버퍼 조회를 빌린다. 슬라이더는 실제 사용 시 요구한다.</summary>
    public ScrollingTextGump(GumpCreateState create, GumpCollection collection, GumpDrawState clip, StyleTextState fragments, StyleText style, ScrollingTextGumpHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(clip); ArgumentNullException.ThrowIfNull(fragments); ArgumentNullException.ThrowIfNull(style); ArgumentNullException.ThrowIfNull(hooks);
        Require(hooks.Node); Require(hooks.BaseNode); Require(hooks.Buffer); _create = create; _collection = collection; _clip = clip; _fragments = fragments; _style = style; _hooks = hooks;
    }
    /// <summary>기본 생성/수집과 같은 실제 객체를 사용한다.</summary>
    private ScrollingTextGumpNode Node(uint token)
    { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.BaseNode!(token))) throw new InvalidOperationException("다중 줄 TextGump 공유 노드 없음"); return n; }
    /// <summary>슬라이더의 현재 기본 폭을 같은 저장소에서 읽는다.</summary>
    private GumpCreateNode BaseNode(uint token) => (token != 0 ? _hooks.BaseNode!(token) : null) ?? throw new InvalidOperationException("TextGump slider 노드 없음");
    /// <summary>상태/처리기가 실제 같은 살아 있는 버퍼인지 확인한다.</summary>
    private TextGumpBufferBinding Buffer(uint token)
    { var b = _hooks.Buffer!(Node(token).Buffer); if (b.State is null || b.Handler is null || !b.State.Initialized || !ReferenceEquals(b.State, b.Handler.State)) throw new InvalidOperationException("TextGump 연결 버퍼 없음"); return b; }
    /// <summary>판본별 dirty 저장 순서를 지킨다. 이후 배치 실패도 이미 쓴 값은 남는다.</summary>
    private void ClearDirty(uint token)
    { if (_create.Collection.Edition == OriginalEdition.Patch1078) { Buffer(token).State.Dirty = 0; Node(token).TextFlags &= ~1U; } else { Node(token).TextFlags &= ~1U; Buffer(token).State.Dirty = 0; } }
    /// <summary>현재 기본 폭에서 선택적 slider 폭을 빼고 CD만 float32 중간 저장 후 절삭한다.</summary>
    private void Layout(uint token)
    { var n = Node(token); double width = n.BaseWidth; if ((n.TextFlags & 2) != 0) { width -= BaseNode(n.Slider).BaseWidth; if (_create.Collection.Edition == OriginalEdition.Cd1072) width = (float)width; } Buffer(token).Handler.Layout(GumpCollection.Ftol(width)); }
    /// <summary>노드/버퍼 중 하나가 변경된 경우만 플래그를 먼저 지우고 배치한다.</summary>
    public void EnsureLayout(uint token) { if ((Node(token).TextFlags & 1) == 0 && Buffer(token).State.Dirty == 0) return; ClearDirty(token); Layout(token); }
    /// <summary>강제 배치 뒤 전체 높이+1을 정수 감김 후 기본/배율 높이에 쓴다.</summary>
    public void FitHeight(uint token)
    { ClearDirty(token); Layout(token); int height = unchecked(Buffer(token).Handler.TotalHeight() + 1); var n = Node(token); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale); }
    /// <summary>임시 폭10000 배치 뒤 최대 폭+여백+slider+1에 맞춘다. 높이는 전체 높이+여백이다.</summary>
    public void FitContent(uint token, int padWidth = 0, int padHeight = 0)
    {
        var n = Node(token); n.BaseWidth = 10000; n.Width = (float)(10000.0 * n.Scale); ClearDirty(token); Layout(token);
        double extra = (Node(token).TextFlags & 2) != 0 ? BaseNode(Node(token).Slider).BaseWidth : 0; double width = (double)Buffer(token).Handler.MaximumWidth() + padWidth + extra + 1; n.BaseWidth = (float)width; n.Width = (float)(width * n.Scale);
        int height = unchecked(Buffer(token).Handler.TotalHeight() + padHeight); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale);
    }
    /// <summary>명시 문맥으로 추가한 뒤 현재 dirty와 가상 재그리기를 갱신한다.</summary>
    public void AppendText(uint token, ReadOnlySpan<byte> bytes, uint[] context, int length = -1, uint metadata = 0)
    { Buffer(token).Handler.AppendText(bytes, context, length, metadata); Node(token).TextFlags |= 1; Redraw(token); }
    /// <summary>추가 통지 이전에 기본32바이트 문맥을 값 복사한다.</summary>
    public void AppendText(uint token, ReadOnlySpan<byte> bytes, int length = -1, uint metadata = 0) => AppendText(token, bytes, (uint[])Node(token).TextContext.Clone(), length, metadata);
    /// <summary>그림은 문자열 통지 없이 추가하고 동일한 dirty/재그리기를 쓴다.</summary>
    public void AppendPicture(uint token, Markup pictures, uint type, uint frame, uint metadata = 0, int fitWidth = 0, int fitHeight = 0)
    { Buffer(token).Handler.AppendPicture(pictures, type, frame, metadata, fitWidth, fitHeight); Node(token).TextFlags |= 1; Redraw(token); }
    /// <summary>완료 입력만 지우고 새 일반 문자열을 추가한다. 미완료 끝/표시 복제는 남는다.</summary>
    public void ReplaceText(uint token, ReadOnlySpan<byte> bytes, uint metadata = 0)
    {
        var context = (uint[])Node(token).TextContext.Clone(); var b = Buffer(token);
        // 매 삭제 뒤 같은 버퍼의 현재 완료 줄 수를 다시 읽는다.
        while (b.State.LineCount != 0) b.Handler.RemoveLine(0); b.Handler.AppendText(bytes, context, -1, metadata); Node(token).TextFlags |= 1; Redraw(token);
    }
    /// <summary>저장 높이/페이지로 제한하고 range→page→position→현재 slider 재조회 순서를 따른다.</summary>
    public void SetScroll(uint token, int position)
    {
        int height = Buffer(token).Handler.TotalHeight(); double viewport = Node(token).BaseHeight; int page = GumpCollection.Ftol(viewport < 0 ? 0 : viewport);
        if (height < unchecked(page + position)) position = unchecked(height - page); if (position < 0) position = 0; Node(token).Scroll = position;
        Require(_hooks.SliderRange)(Node(token).Slider, 0, height); Require(_hooks.SliderPage)(Node(token).Slider, page); Require(_hooks.SliderSetPosition)(Node(token).Slider, Node(token).Scroll);
        int current = Require(_hooks.SliderPosition)(Node(token).Slider); Node(token).Scroll = current;
    }
    /// <summary>extra는 원본처럼 읽지 않는다. 실수 내용-viewport를 먼저 절삭한다.</summary>
    public void ScrollBottom(uint token, int extra = 0) { double content = Buffer(token).Handler.TotalHeight(); SetScroll(token, GumpCollection.Ftol(content - Node(token).BaseHeight)); }
    /// <summary>요청을 쓴 뒤 가상+8에 erase=false를 전달한다.</summary>
    private void Redraw(uint token) => Require(_hooks.Redraw)(token, false);
    /// <summary>음수도 그대로 저장하며 유효성은 적용 시 검사한다.</summary>
    public void RequestScroll(uint token, int position) { Node(token).ScrollRequest = position; Redraw(token); }
    /// <summary>현재 적용 scroll에 delta를 감김으로 더한다. 대기 요청은 사용하지 않는다.</summary>
    public void RequestRelativeScroll(uint token, int delta) => RequestScroll(token, Math.Max(unchecked(delta + Node(token).Scroll), 0));
    /// <summary>판본별 -2/여백 저장 순서를 지키고 재그리기를 요청한다.</summary>
    public void RequestBottom(uint token, int extra = 0)
    { if (_create.Collection.Edition == OriginalEdition.Patch1078) { Node(token).ScrollRequest = -2; Node(token).BottomPadding = extra; } else { Node(token).BottomPadding = extra; Node(token).ScrollRequest = -2; } Redraw(token); }
    /// <summary>정상 반환 뒤 현재 요청을-1로 덮는다. 콜백 예외에서는 기존 요청을 남긴다.</summary>
    public void ProcessRequest(uint token)
    { int request = Node(token).ScrollRequest; if (request == -1) return; if (request == -2) ScrollBottom(token, Node(token).BottomPadding); else { if (request < 0) throw new ArgumentException("TextGump scrollRequest 음수"); SetScroll(token, Node(token).ScrollRequest); } Node(token).ScrollRequest = -1; }
    /// <summary>ordered 비교의 내용-viewport-10 <= scroll. NaN은false다.</summary>
    public bool AtBottom(uint token) => (double)Buffer(token).Handler.TotalHeight() - Node(token).BaseHeight - 10 <= Node(token).Scroll;
    /// <summary>같은 표시 센티널/현재 줄.</summary>
    private static StyleTextBufferLine Line(StyleTextBufferState s, uint token) => token == 0 ? s.Sentinel : s.SoftLines.TryGetValue(token, out var n) ? n : throw new InvalidOperationException("TextGump 표시 줄 없음");
    /// <summary>같은 공통 표의 현재 조각.</summary>
    private StyleTextFragment Fragment(uint token) => token != 0 && _fragments.Fragments.TryGetValue(token, out var n) ? n : throw new InvalidOperationException("TextGump 표시 조각 없음");
    /// <summary>정상 호출 수를 바꾸지 않고 손상 순환만 거부한다.</summary>
    private static void Visit(HashSet<uint> seen, uint token) { if (!seen.Add(token)) throw new InvalidOperationException("TextGump 표시 목록 순환"); }
    /// <summary>CD만 NaN viewport에서도 행을 순회하는 원본 비교.</summary>
    private bool BelowViewport(int y, float height) => (double)y < height || (_create.Collection.Edition == OriginalEdition.Cd1072 && float.IsNaN(height));
    /// <summary>저장 y+높이와 scroll이 같으면 해당 줄을 유지한다.</summary>
    private uint FirstVisible(uint token, StyleTextBufferState s)
    {
        uint line = s.Sentinel.Next; var seen = new HashSet<uint>();
        // 첫 가시 줄만 저장 y를 쓰며 이후 출력은 높이를 누적한다.
        while (unchecked(Line(s, line).Y + Line(s, line).Height) < Node(token).Scroll) { if (line == 0) throw new InvalidOperationException("TextGump scroll이 센티널을 넘음"); Visit(seen, line); line = Line(s, line).Next; } return line;
    }
    /// <summary>현재 half-open 경계/표시 조각의 폭과 행 아래 정렬 높이로 선택한다. 위쪽 경계는 제외하고 배치는 자동 갱신하지 않는다.</summary>
    public uint HitTest(uint token, ScreenPoint position)
    {
        ScreenRect b = _collection.Bounds(token); if (position.X < b.Left || position.X >= b.Right || position.Y < b.Top || position.Y >= b.Bottom) return 0;
        var s = Buffer(token).State; uint line = FirstVisible(token, s); int y = unchecked(Line(s, line).Y - Node(token).Scroll), xLocal = unchecked(position.X - b.Left), yLocal = unchecked(position.Y - b.Top); var rows = new HashSet<uint>();
        // 실제 현재 viewport 안의 행만 검사한다. 줄 위치는 높이 누적이다.
        while (line != 0 && BelowViewport(y, Node(token).BaseHeight))
        {
            Visit(rows, line);
            if (yLocal < unchecked(Line(s, line).Height + y))
            {
                if (Line(s, line).Width <= xLocal) return 0; uint fragment = Line(s, line).Head; int width = 0; var seen = new HashSet<uint>();
                // 가상 측정 뒤 현재 next를 읽으며 정확한 위 경계는 선택하지 않는다.
                while (fragment != 0) { Visit(seen, fragment); var size = _style.MeasureFragment(fragment); width = unchecked(width + size.Width); if (xLocal < width && unchecked(Line(s, line).Height - size.Height + y) < yLocal) return fragment; fragment = Fragment(fragment).Next; } return 0;
            }
            int height = Line(s, line).Height; line = Line(s, line).Next; y = unchecked(y + height);
        }
        return 0;
    }
    /// <summary>배치→요청 적용→내부 클립→현재 위치 뒤 기준선을 두 번 순회하며 실제 글자/그림을 출력한다.</summary>
    public void Draw(uint token)
    {
        EnsureLayout(token); ProcessRequest(token); ScreenRect bounds = _collection.Bounds(token); using var guard = new GumpClipScope(_clip, bounds); ScreenPoint position = _collection.Position(token); var s = Buffer(token).State; uint line = FirstVisible(token, s); int y = unchecked(Line(s, line).Y - Node(token).Scroll); var rows = new HashSet<uint>();
        // viewport 끝 동률은 제외하고 저장 y 대신 현재 행 높이를 누적한다.
        while (line != 0 && BelowViewport(y, Node(token).BaseHeight))
        {
            Visit(rows, line); int ascent = 0; uint fragment = Line(s, line).Head; var seen = new HashSet<uint>();
            // 첫 순회는 최대 상승만 구한다. 각 콜백 뒤 현재 next를 읽는다.
            while (fragment != 0) { Visit(seen, fragment); ascent = Math.Max(ascent, _style.Baseline(fragment).Ascent); fragment = Fragment(fragment).Next; }
            int baseline = unchecked(ascent + y), x = 0; fragment = Line(s, line).Head; seen.Clear();
            // 두 번째는 기준선 재측정→크기 저장→실제 출력→저장 폭 전진 순서다.
            while (fragment != 0) { Visit(seen, fragment); var metrics = _style.Baseline(fragment); var size = _style.MeasureFragment(fragment); _style.DrawFragment(fragment, new(unchecked(position.X + x), unchecked(position.Y - metrics.Ascent + baseline))); x = unchecked(x + size.Width); fragment = Fragment(fragment).Next; }
            int height = Line(s, line).Height; line = Line(s, line).Next; y = unchecked(y + height);
        }
    }
}
