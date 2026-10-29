using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 Gump 트리의 Slider 값/부착/손잡이 입력 영역이다.

/// <summary>원본 Slider의 범위/페이지/방향/자식 필드.생성은 ScrollGumpWidgets가 맡는다.</summary>
public class ScrollGumpNode : GumpCreateNode
{
    /// <summary>signed 최소 위치.</summary>
    public int Minimum { get; set; }
    /// <summary>signed 최대 위치.</summary>
    public int Maximum { get; set; }
    /// <summary>현재 원본 위치.</summary>
    public int Position { get; set; }
    /// <summary>원본 페이지 길이.음수도 보존한다.</summary>
    public int Page { get; set; }
    /// <summary>화살표 이동 단위.</summary>
    public int Step { get; set; }
    /// <summary>방향 bit1은 가로다.</summary>
    public uint ScrollFlags { get; set; }
    /// <summary>감소 화살표 토큰.</summary>
    public uint Decrement { get; set; }
    /// <summary>증가 화살표 토큰.</summary>
    public uint Increment { get; set; }
    /// <summary>손잡이 토큰.</summary>
    public uint Thumb { get; set; }
    /// <summary>생성자가0으로 쓰는+b0.</summary>
    public uint ReservedB0 { get; set; }
    /// <summary>생성자가 쓰지 않는+bc.</summary>
    public uint ReservedBC { get; set; }
    /// <summary>생성자가0으로 쓰는 두 보조 DWORD.</summary>
    public uint[] Reserved { get; } = new uint[2];
}
/// <summary>같은 실제 배경의 포함 손잡이 사각형과 이전 입력 좌표.</summary>
public class ScrollGumpThumbNode : GumpBackgroundNode
{
    /// <summary>추가 생성 타입 사본.Draw는 기본 배경 Type을 사용한다.</summary>
    public uint DetailType { get; set; }
    /// <summary>추가 생성 프레임 사본.Draw는 기본 배경 Frame을 사용한다.</summary>
    public uint DetailFrame { get; set; }
    /// <summary>양 끝을 포함하는 원본 선택 사각형.</summary>
    public ScreenRect Handle { get; set; }
    /// <summary>이전 화면 Y.</summary>
    public int PreviousY { get; set; }
    /// <summary>패치에만 있는 이전 화면 X.CD에서는 사용하지 않는다.</summary>
    public int PreviousX { get; set; }
}
/// <summary>실제 문자열 사건 초기화 결과를 공급한다.</summary>
public sealed class ScrollGumpState
{
    /// <summary>버튼과 공유하는 click ID.</summary>
    public uint ClickEvent { get; set; }
    /// <summary>버튼과 공유하는 repeat ID.</summary>
    public uint RepeatEvent { get; set; }
    /// <summary>부모 이동 요청 ID.</summary>
    public uint StepEvent { get; set; }
    /// <summary>위치 변경 통지 ID.</summary>
    public uint ChangeEvent { get; set; }
}
/// <summary>현재 같은 기본/파생 저장소와 실제 가상 부착/재그리기/폴링 경계.</summary>
public sealed class ScrollGumpHooks
{
    /// <summary>현재 Slider 노드.</summary>
    public Func<uint, ScrollGumpNode?>? Node { get; set; }
    /// <summary>현재 손잡이 노드.</summary>
    public Func<uint, ScrollGumpThumbNode?>? ThumbNode { get; set; }
    /// <summary>동일 기본 객체.</summary>
    public Func<uint, GumpCreateNode?>? BaseNode { get; set; }
    /// <summary>현재 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>현재 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>현재 Redraw(false).</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>현재 왼쪽 버튼 상태.반환 좌표는 사용하지 않는다.</summary>
    public Func<uint, InputEvent>? Poll { get; set; }
}
/// <summary>cpppj/src/client/ScrollGump.cpp의 값/범위/부착/선택/드래그/부모 통지를 같은 저장소에 연결한다.</summary>
public sealed class ScrollGump
{
    /// <summary>현재 등록/판본 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 실제 사건 ID.</summary>
    private readonly ScrollGumpState _state;
    /// <summary>같은 실제 기본 트리.</summary>
    private readonly GumpTree _tree;
    /// <summary>같은 현재 좌표/경계.</summary>
    private readonly GumpCollection _collection;
    /// <summary>실제 부모 체인 통지.</summary>
    private readonly GumpEvents _events;
    /// <summary>실제 입력 캡처.</summary>
    private readonly GumpInput _input;
    /// <summary>같은 파생 저장소/가상 경계.</summary>
    private readonly ScrollGumpHooks _hooks;
    /// <summary>도달한 가상 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("ScrollGump 가상 경계 필요");
    /// <summary>안정된 같은 기본/파생 조회를 빌린다.</summary>
    public ScrollGump(GumpCreateState create, ScrollGumpState state, GumpTree tree, GumpCollection collection, GumpEvents events, GumpInput input, ScrollGumpHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(events); ArgumentNullException.ThrowIfNull(input); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.ThumbNode); Require(hooks.BaseNode); _create = create; _state = state; _tree = tree; _collection = collection; _events = events; _input = input; _hooks = hooks;
    }
    /// <summary>값/자식과 기본 등록이 같은 객체인지 검사한다.</summary>
    private ScrollGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.BaseNode!(token))) throw new InvalidOperationException("ScrollGump 공유 노드 없음"); return n; }
    /// <summary>선택 사각형과 실제 배경이 같은 객체인지 검사한다.</summary>
    private ScrollGumpThumbNode Thumb(uint token) { var n = token != 0 ? _hooks.ThumbNode!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.BaseNode!(token))) throw new InvalidOperationException("ScrollGump 손잡이 공유 노드 없음"); return n; }
    /// <summary>화살표/손잡이의 현재 기본 필드.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.BaseNode!(token) : null) ?? throw new InvalidOperationException("ScrollGump 기본 노드 없음");
    /// <summary>signed 페이지 계약과 min/max-page 제한 뒤 저장하고 현재 void에 따라 재부착/재그리기한다.</summary>
    public void SetPosition(uint token, int position)
    {
        var n = Node(token); if (unchecked(n.Maximum - n.Minimum) < n.Page) throw new ArgumentException("ScrollGump page가 범위보다 큼"); if (position < n.Minimum) position = n.Minimum; if (n.Maximum < unchecked(n.Page + position)) position = unchecked(n.Maximum - n.Page); n.Position = position;
        if ((n.Flags & 8) == 0) { Push(token); float x = Node(token).X, y = Node(token).Y; Require(_hooks.Attach)(token, x, y, 0, 0); } else Require(_hooks.Redraw)(token, false);
    }
    /// <summary>범위보다 큰 페이지만 제한하며 음수 하한 제한을 추가하지 않는다.</summary>
    public void SetPage(uint token, int page) { var n = Node(token); int length = unchecked(n.Maximum - n.Minimum); if (length < page) page = length; n.Page = page; SetPosition(token, Node(token).Position); }
    /// <summary>끝점 정렬/동률 max+1 뒤 판본별 페이지 보정과 현재 위치 두 번 적용을 수행한다.</summary>
    public void SetRange(uint token, int first, int last)
    {
        int minimum = first < last ? first : last, maximum = first < last ? last : first; if (first == last) maximum = unchecked(minimum + 1); var n = Node(token);
        if (_create.Collection.Edition == OriginalEdition.Patch1078) { n.Maximum = maximum; n.Minimum = minimum; int length = unchecked(maximum - minimum); if (length < n.Page) n.Page = length; SetPosition(token, Node(token).Position); } else { n.Minimum = minimum; n.Maximum = maximum; SetPage(token, Node(token).Page); } SetPosition(token, Node(token).Position);
    }
    /// <summary>보정 없이 현재 원본 위치를 반환한다.</summary>
    public int Position(uint token) => Node(token).Position;
    /// <summary>같은 현재 트리/파생 Push를 호출한다.</summary>
    private void Push(uint token) => Require(_hooks.Push)(token);
    /// <summary>자식 부착은 현재 부모/mode0이다.</summary>
    private void AttachChild(uint token, float x, float y, uint parent) => Require(_hooks.Attach)(token, x, y, parent, 0);
    /// <summary>지정 축의 기본 크기는 float32로,배율 결과는 좁힘 전 size로 계산한다.</summary>
    private void SetSize(uint token, double size, bool width) { var n = Base(token); if (width) { n.BaseWidth = (float)size; n.Width = (float)(size * n.Scale); } else { n.BaseHeight = (float)size; n.Height = (float)(size * n.Scale); } }
    /// <summary>기본 부착→현재 세 자식 Push/크기/부착→원본 position 분자의 포함 손잡이 사각형 순서다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        Node(token); _tree.Attach(token, x, y, parent, mode); bool horizontal = (Node(token).ScrollFlags & 1) != 0; int thickness = GumpCollection.Ftol(horizontal ? Node(token).BaseHeight : Node(token).BaseWidth); float rounded = thickness; double second = _create.Collection.Edition == OriginalEdition.Cd1072 ? rounded : (double)thickness; float twice = unchecked(thickness + thickness);
        Push(Node(token).Decrement); Push(Node(token).Increment); Push(Node(token).Thumb);
        if (horizontal)
        {
            SetSize(Node(token).Decrement, Node(token).BaseHeight, false); SetSize(Node(token).Decrement, thickness, true); SetSize(Node(token).Increment, Node(token).BaseHeight, false); SetSize(Node(token).Increment, second, true); SetSize(Node(token).Thumb, Node(token).BaseHeight, false); SetSize(Node(token).Thumb, (double)Node(token).BaseWidth - twice, true);
            AttachChild(Node(token).Decrement, 0, 0, token); AttachChild(Node(token).Increment, (float)((double)Node(token).BaseWidth - rounded), 0, token); AttachChild(Node(token).Thumb, rounded, 0, token);
        }
        else
        {
            SetSize(Node(token).Decrement, thickness, false); SetSize(Node(token).Decrement, Node(token).BaseWidth, true); SetSize(Node(token).Increment, second, false); SetSize(Node(token).Increment, Node(token).BaseWidth, true); SetSize(Node(token).Thumb, (double)Node(token).BaseHeight - twice, false); SetSize(Node(token).Thumb, Node(token).BaseWidth, true);
            AttachChild(Node(token).Decrement, 0, 0, token); AttachChild(Node(token).Increment, 0, (float)((double)Node(token).BaseHeight - rounded), token); AttachChild(Node(token).Thumb, 0, rounded, token);
        }
        var n = Node(token); var thumb = Thumb(n.Thumb); double available = (double)(horizontal ? n.BaseWidth : n.BaseHeight) - twice; int range = unchecked(n.Maximum - n.Minimum); int page = GumpCollection.Ftol((double)n.Page * available / range), position = GumpCollection.Ftol((double)n.Position * available / range), cross = unchecked(GumpCollection.Ftol(horizontal ? n.BaseHeight : n.BaseWidth) - 1), low = unchecked(position + 1), high = unchecked(position + page - 1);
        thumb.Handle = horizontal ? new(low, 1, high, cross) : new(1, low, cross, high);
    }
    /// <summary>가로는 무동작이다.세로 range*(delta+handle.top)은 DWORD 곱 뒤 실수 분모로 나눈다.</summary>
    public void DragVertical(uint token, int delta)
    { var n = Node(token); if ((n.ScrollFlags & 1) != 0) return; int numerator = unchecked((n.Maximum - n.Minimum) * (delta + Thumb(n.Thumb).Handle.Top)); double denominator = (double)n.BaseHeight - (double)n.BaseWidth * 2; SetPosition(token, GumpCollection.Ftol(numerator / denominator)); }
    /// <summary>호출 직전 현재 change ID/serial로 값0의 부모 통지를 보낸다.</summary>
    private void NotifyChange(uint token) => _events.Bubble(token, _state.ChangeEvent, Base(token).Serial, 0);
    /// <summary>화살표 click/repeat와 손잡이 step을 실제 부모 체인에 보내며 실제 값이 바뀐 경우만 change를 보낸다.항상0을 반환해 바깥 Bubble도 원래 serial로 상위 전달을 계속한다.</summary>
    public uint Handle(uint token, uint eventCode, uint first, uint second)
    {
        eventCode &= 0x03ffffff;
        if ((eventCode == _state.ClickEvent || eventCode == _state.RepeatEvent) && first != Base(Node(token).Thumb).Serial)
        {
            int previous = Node(token).Position; bool arrow = false;
            if (first == Base(Node(token).Decrement).Serial) { arrow = true; if (_events.Bubble(token, _state.StepEvent, Node(token).Serial, 2) == 0) SetPosition(token, unchecked(Node(token).Position - Node(token).Step)); }
            else if (first == Base(Node(token).Increment).Serial) { arrow = true; if (_events.Bubble(token, _state.StepEvent, Node(token).Serial, 1) == 0) SetPosition(token, unchecked(Node(token).Step + Node(token).Position)); }
            if (arrow && previous != Node(token).Position) NotifyChange(token);
        }
        if (eventCode == _state.StepEvent && first == Base(Node(token).Thumb).Serial) { int previous = Node(token).Position; _events.Bubble(token, _state.StepEvent, Node(token).Serial, second); if (previous != Node(token).Position) NotifyChange(token); } return 0;
    }
    /// <summary>손잡이 포함 사각형은 캡처,앞/뒤 영역은 부모 page 요청4/3이다.CD 가로는 누름 무동작이다.</summary>
    public uint ThumbHandle(uint token, uint eventCode, uint first, uint second)
    {
        if ((eventCode & 0x03ffffff) != InputCode.LeftButton) return 0; var thumb = Thumb(token); bool vertical = !(thumb.BaseWidth > thumb.BaseHeight); if ((eventCode & InputCode.Release) != 0) { if (vertical) thumb.Flags &= ~0x100U; return 1; } if (!vertical && _create.Collection.Edition == OriginalEdition.Cd1072) return 1;
        ScreenRect b = _collection.Bounds(token); int point = unchecked((int)(vertical ? second : first)), local = unchecked(point - (vertical ? b.Top : b.Left)), low = vertical ? Thumb(token).Handle.Top : Thumb(token).Handle.Left, high = vertical ? Thumb(token).Handle.Bottom : Thumb(token).Handle.Right;
        if (local < low) _events.Bubble(token, _state.StepEvent, Thumb(token).Serial, 4); else if (high < local) _events.Bubble(token, _state.StepEvent, Thumb(token).Serial, 3); else { if (vertical) Thumb(token).PreviousY = point; else Thumb(token).PreviousX = point; Thumb(token).Flags |= 0x100; _input.Capture(token); } return 1;
    }
    /// <summary>폴링 release는 프레임 플래그만 해제한다.그 외에는 전달 y delta로 현재 부모 Slider를 갱신한다.</summary>
    public void ThumbBroadcast(uint token, uint eventCode, uint first, uint second)
    {
        if ((Require(_hooks.Poll)(InputCode.LeftButton).Code & InputCode.Release) != 0) { Thumb(token).Flags &= ~0x100U; return; } int y = unchecked((int)second), delta = unchecked(y - Thumb(token).PreviousY);
        if (delta != 0) { Thumb(token).PreviousY = y; DragVertical(Thumb(token).Parent, delta); _events.Bubble(token, _state.ChangeEvent, Base(Thumb(token).Parent).Serial, 0); }
    }
    /// <summary>현재 파생 분류를 매 호출 조회해 Attach/일반 사건/직접 캡처/프레임을 연결하고 이전 분배를 보존한다.</summary>
    public static void BindScrollGump(GumpTreeHooks tree, GumpEventHooks events, GumpInputHooks input, GumpFrameHooks frame, Func<uint, ScrollGump?> slider, Func<uint, ScrollGump?> thumb)
    {
        ArgumentNullException.ThrowIfNull(slider); ArgumentNullException.ThrowIfNull(thumb); var attach = tree.Attach; tree.Attach = (t, x, y, p, m) => { if (slider(t) is { } s) s.Attach(t, x, y, p, m); else Require(attach)(t, x, y, p, m); };
        /// <summary>재귀/부모 통지와 직접 캡처가 같은 원본 Handle을 사용한다.</summary>
        uint Route(Func<uint, uint, uint, uint, uint>? previous, uint t, uint c, uint a, uint b) => slider(t) is { } s ? s.Handle(t, c, a, b) : thumb(t) is { } h ? h.ThumbHandle(t, c, a, b) : Require(previous)(t, c, a, b);
        var e = events.Handle; var i = input.Handle; events.Handle = (t, c, a, b) => Route(e, t, c, a, b); input.Handle = (t, c, a, b) => Route(i, t, c, a, b); var broadcast = frame.Broadcast; frame.Broadcast = (t, c, a, b) => { if (thumb(t) is { } h) h.ThumbBroadcast(t, c, a, b); else Require(broadcast)(t, c, a, b); };
    }
    /// <summary>다중 줄 표시의 네 slider 가상 경계를 같은 실제 값/범위/페이지/위치에 연결한다.</summary>
    public static void BindScrollingTextSlider(ScrollingTextGumpHooks hooks, ScrollGump slider) { hooks.SliderRange = slider.SetRange; hooks.SliderPage = slider.SetPage; hooks.SliderSetPosition = slider.SetPosition; hooks.SliderPosition = slider.Position; }
}
