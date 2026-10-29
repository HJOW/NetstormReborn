using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 실제 버퍼/Slider를 소유하는 다중 줄 위젯의 생성·입력·수명 영역이다.

/// <summary>현재 종류 BYTE/Slider 자산/실제 문자열 사건 초기화 전역.</summary>
public sealed class ScrollingTextGumpWidgetState
{
    /// <summary>기본 command에 signed 확장할 종류 BYTE.</summary>
    public sbyte Alignment { get; set; } = 46;
    /// <summary>Slider 생성에 전달할 현재 타입.</summary>
    public uint SliderType { get; set; }
    /// <summary>Slider 생성에 전달할 현재 프레임.</summary>
    public uint SliderFrame { get; set; }
    /// <summary>Slider와 공유하는 change ID.</summary>
    public uint ChangeEvent { get; set; }
    /// <summary>Slider와 공유하는 step ID.</summary>
    public uint StepEvent { get; set; }
    /// <summary>본문 재배치 요청 ID.</summary>
    public uint RefreshEvent { get; set; }
    /// <summary>커서 선택 요청 ID.</summary>
    public uint CursorEvent { get; set; }
    /// <summary>왼쪽 태그 부모 통지 ID.</summary>
    public uint LeftTagEvent { get; set; }
    /// <summary>오른쪽 태그 부모 통지 ID.</summary>
    public uint RightTagEvent { get; set; }
}
/// <summary>같은 실제 노드/Slider/버퍼 할당과 현재 가상 Push/Attach/Redraw/커서 선택.</summary>
public sealed class ScrollingTextGumpWidgetHooks
{
    /// <summary>현재 다중 줄 노드.</summary>
    public Func<uint, ScrollingTextGumpNode?>? Node { get; set; }
    /// <summary>현재 실제 Slider.</summary>
    public Func<uint, ScrollGumpNode?>? Slider { get; set; }
    /// <summary>같은 기본 생성/트리 노드.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>원본0x38 버퍼/0xd8 Slider new.버퍼는 Gump 표에 넣지 않는다.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>현재 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>현재 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>현재 Redraw(false).</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>현재 원본 커서 번호1/15/16 선택.</summary>
    public Action<uint>? Cursor { get; set; }
}
/// <summary>cpppj/src/client/ScrollingTextGumpWidgets.cpp의 실제 생성/부분 필드/부착/태그·커서·캡처/버퍼 소유 수명.</summary>
public sealed class ScrollingTextGumpWidgets
{
    /// <summary>현재 등록/판본 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 종류/자산/사건 전역.</summary>
    private readonly ScrollingTextGumpWidgetState _state;
    /// <summary>같은 기본 생성기.</summary>
    private readonly GumpCreate _factory;
    /// <summary>같은 실제 기본 트리.</summary>
    private readonly GumpTree _tree;
    /// <summary>같은 overlay/자식 우선 수명.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>같은 부모 체인 통지.</summary>
    private readonly GumpEvents _events;
    /// <summary>같은 실제 입력 캡처.</summary>
    private readonly GumpInput _input;
    /// <summary>현재 태그를 읽는 공통 조각 표.</summary>
    private readonly StyleTextState _fragments;
    /// <summary>같은 실제 버퍼 소유 저장소.</summary>
    private readonly StyleTextBufferStore _buffers;
    /// <summary>같은 표시/요청/선택 처리기.</summary>
    private readonly ScrollingTextGump _text;
    /// <summary>같은 실제 Slider 자식 생성기.</summary>
    private readonly ScrollGumpWidgets _sliderWidgets;
    /// <summary>현재 공유 조회/가상 경계.</summary>
    private readonly ScrollingTextGumpWidgetHooks _hooks;
    /// <summary>실제 도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("다중 줄 Widget 가상 경계 필요");
    /// <summary>같은 실제 노드/Slider 저장소와 기존 처리기를 빌린다.</summary>
    public ScrollingTextGumpWidgets(GumpCreateState create, ScrollingTextGumpWidgetState state, GumpCreate factory, GumpTree tree, GumpLifetime lifetime, GumpEvents events, GumpInput input, StyleTextState fragments, StyleTextBufferStore buffers, ScrollingTextGump text, ScrollGumpWidgets sliderWidgets, ScrollingTextGumpWidgetHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(events); ArgumentNullException.ThrowIfNull(input); ArgumentNullException.ThrowIfNull(fragments); ArgumentNullException.ThrowIfNull(buffers); ArgumentNullException.ThrowIfNull(text); ArgumentNullException.ThrowIfNull(sliderWidgets); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Slider); Require(hooks.Base); _create = create; _state = state; _factory = factory; _tree = tree; _lifetime = lifetime; _events = events; _input = input; _fragments = fragments; _buffers = buffers; _text = text; _sliderWidgets = sliderWidgets; _hooks = hooks;
    }
    /// <summary>현재 같은 실제 기본/다중 줄 노드.</summary>
    private ScrollingTextGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("다중 줄 Widget 공유 노드 없음"); return n; }
    /// <summary>현재 같은 실제 기본/Slider 노드.</summary>
    private ScrollGumpNode Slider(uint token) { var n = token != 0 ? _hooks.Slider!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("다중 줄 Widget Slider 없음"); return n; }
    /// <summary>가상 부착/Push에 도달한 실제 기본 노드.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base!(token) : null) ?? throw new InvalidOperationException("다중 줄 Widget 기본 노드 없음");
    /// <summary>현재 가상+8에 false를 전달한다.</summary>
    private void Redraw(uint token) => Require(_hooks.Redraw)(token, false);
    /// <summary>현재 실제 가상 Push 뒤의 필드는 호출자가 다시 읽는다.</summary>
    private void Push(uint token) { Base(token); Require(_hooks.Push)(token); }
    /// <summary>오른쪽 Slider는 y0/mode1로 화면 제한 없이 부착한다.</summary>
    private void AttachSlider(uint token, float x, uint parent) { Base(token); Require(_hooks.Attach)(token, x, 0, parent, 1); }
    /// <summary>전체 signed 값이 비0이면 기본 입력 제외 비트를 지운다.</summary>
    public void SetSelectable(uint token, int value) { if (value != 0) Node(token).Flags &= ~0x200U; else Node(token).Flags |= 0x200; }
    /// <summary>최하위 비트로 Slider를 켜며 필요하면 스크롤도 켠다.이미 부착돼 있으면 현재 가상 재부착을 수행한다.</summary>
    public void SetSlider(uint token, uint value)
    {
        var n = Node(token); n.TextFlags = (n.TextFlags & ~2U) | ((value & 1) << 1); if ((n.TextFlags & 6) == 2) n.TextFlags |= 4;
        if ((Node(token).Flags & 8) == 0) { Push(token); float x = Node(token).X, y = Node(token).Y; Require(_hooks.Attach)(token, x, y, 0, 0); }
    }
    /// <summary>현재 버퍼 noWrap만 쓴다.dirty나 재그리기를 추가하지 않는다.</summary>
    public void SetWrap(uint token, int value) => _buffers.Binding(Node(token).Buffer).State.NoWrap = value == 0 ? 1U : 0U;
    /// <summary>최하위 비트로 스크롤을 끄면 현재 Slider도 원본 SetSlider로 끈다.</summary>
    public void SetScrollable(uint token, uint value) { var n = Node(token); n.TextFlags = (n.TextFlags & ~4U) | ((value & 1) << 2); if ((n.TextFlags & 6) == 2) SetSlider(token, 0); }
    /// <summary>현재 소유 버퍼만 소멸/free 뒤 포인터0을 쓰고 새 포인터/소유 DWORD를 설치한다.같은 소유 토큰도 예외로 만들지 않는다.</summary>
    public void ReplaceBuffer(uint token, uint buffer, uint owns)
    { if (buffer == 0) throw new ArgumentException("다중 줄 Widget 교체 버퍼 null"); uint old = Node(token).Buffer; if (old != 0 && Node(token).OwnsBuffer != 0) { _buffers.Destroy(old); Node(token).Buffer = 0; } Node(token).Buffer = buffer; Node(token).OwnsBuffer = owns; }
    /// <summary>실제 기본 등록/128칸 소유 버퍼/문맥/overlay/숨긴9×100 Slider/오른쪽 부착/맨 아래 요청을 순서대로 만든다.</summary>
    public uint Construct(uint token, int width, int height, uint font, uint color)
    {
        Node(token); _factory.Construct(token, 0, 0, 0, 1); var n = Node(token); n.Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x51454cU : 0x505d80U; n.Reserved.AsSpan().Clear(); n.Command = unchecked((uint)(int)_state.Alignment);
        uint buffer = Require(_hooks.Allocate)(0x38); uint created = buffer != 0 ? _buffers.Construct(buffer, 128) : 0; Node(token).Buffer = created; Node(token).OwnsBuffer = 1; if (Node(token).Buffer == 0) throw new InvalidOperationException("다중 줄 Widget 버퍼 할당 실패");
        float savedWidth = width; n.BaseWidth = savedWidth; n.Width = (float)((double)width * n.Scale); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale); uint[] context = [0, font, color, 0, 0, 0, 1, 1]; context.CopyTo(n.TextContext, 0); n.LayoutWidth = 0; n.Scroll = 0; n.Callback = 0; n.Flags |= 0x400a00; _lifetime.SetOverlay(token, true); n.ScrollRequest = -1; n.BottomPadding = 0;
        uint slider = Require(_hooks.Allocate)(0xd8); if (slider != 0) slider = _sliderWidgets.Construct(slider, 9, 100, _state.SliderType, _state.SliderFrame); Node(token).Slider = slider; Slider(slider).Flags &= ~1U; AttachSlider(Node(token).Slider, (float)((double)savedWidth - Slider(Node(token).Slider).BaseWidth), token);
        _text.RequestBottom(token); Node(token).TextFlags = (Node(token).TextFlags & ~6U) | 8 | 16; Node(token).Metadata = 0; return token;
    }
    /// <summary>기본 부착 뒤 현재 Slider의 높이/표시/오른쪽 위치와 기존 스크롤 요청·폭 dirty를 갱신한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        Node(token); _tree.Attach(token, x, y, parent, mode); Push(Node(token).Slider); var slider = Slider(Node(token).Slider); double height = Node(token).BaseHeight; slider.BaseHeight = (float)height; slider.Height = (float)(height * slider.Scale); if ((Node(token).TextFlags & 6) == 6) slider.Flags |= 1; else slider.Flags &= ~1U;
        AttachSlider(Node(token).Slider, (float)((double)Node(token).BaseWidth - Slider(Node(token).Slider).BaseWidth), token); if (Node(token).ScrollRequest == -1) _text.RequestScroll(token, Node(token).Scroll); float previous = Node(token).LayoutWidth, current = Node(token).BaseWidth;
        bool changed = previous != current && (_create.Collection.Edition == OriginalEdition.Patch1078 || (!float.IsNaN(previous) && !float.IsNaN(current))); if (changed) { Node(token).TextFlags |= 1; Node(token).LayoutWidth = Node(token).BaseWidth; }
    }
    /// <summary>그림/문자열을 구별하지 않고 실제 공통 조각의 현재 태그를 읽는다.</summary>
    private uint Tag(uint token) => token != 0 && _fragments.Fragments.TryGetValue(token, out var n) ? n.Metadata : throw new InvalidOperationException("다중 줄 Widget 태그 조각 없음");
    /// <summary>화면 DWORD를 signed로 전달하며 선택 자체는 자동 배치하지 않는다.</summary>
    private uint Hit(uint token, uint x, uint y) => _text.HitTest(token, new(unchecked((int)x), unchecked((int)y)));
    /// <summary>Slider change/step·refresh 뒤에도 마우스/커서 분류를 계속 수행한다.태그0 왼쪽은 캡처,오른쪽은 조각 존재만 검사한다.</summary>
    public uint Handle(uint token, uint eventCode, uint first, uint second)
    {
        uint code = eventCode & 0x03ffffff, result = 0;
        if (code == _state.ChangeEvent) { if (first != Slider(Node(token).Slider).Serial) throw new ArgumentException("다중 줄 Widget change 발신 Slider 불일치"); _text.RequestScroll(token, Slider(Node(token).Slider).Position); }
        if (code == _state.StepEvent && first == Slider(Node(token).Slider).Serial)
        {
            int? delta = second switch { 1 => 5, 2 => -5, 3 => GumpCollection.Ftol(Node(token).BaseHeight), 4 => GumpCollection.Ftol(-(double)Node(token).BaseHeight), _ => null }; if (delta is { } d) { _text.RequestRelativeScroll(token, d); result = 1; }
        }
        if (code == _state.RefreshEvent) { Node(token).TextFlags |= 1; Redraw(token); }
        if (code != _state.CursorEvent)
        {
            if (code == InputCode.LeftButton)
            {
                uint fragment = (Node(token).TextFlags & 8) != 0 ? Hit(token, first, second) : 0;
                if (fragment == 0 || Tag(fragment) == 0) { if ((eventCode & InputCode.Release) != 0) Node(token).Flags &= ~0x100U; else { Node(token).DragY = unchecked((int)second); Node(token).Flags |= 0x100; _input.Capture(token); } }
                else if ((eventCode & InputCode.Release) == 0) _events.Bubble(token, _state.LeftTagEvent, Node(token).Serial, Tag(fragment)); result = 1;
            }
            if (code == InputCode.RightButton && (Node(token).TextFlags & 16) != 0) { if ((eventCode & InputCode.Release) == 0) { uint fragment = Hit(token, first, second); if (fragment != 0) { _events.Bubble(token, _state.RightTagEvent, Node(token).Serial, Tag(fragment)); return 1; } } result = 0; } return result;
        }
        uint cursor = 1, hit = (Node(token).TextFlags & 8) != 0 ? Hit(token, first, second) : 0; if (hit != 0 && Tag(hit) != 0) cursor = 16; else if ((Node(token).TextFlags & 4) != 0) { var slider = Slider(Node(token).Slider); if (slider.Page < unchecked(slider.Maximum - slider.Minimum)) cursor = 15; } Require(_hooks.Cursor)(cursor); return 1;
    }
    /// <summary>스크롤 bit4일 때만 전달y delta의 반대 상대 요청을 한다.폴링/사건/x는 사용하지 않는다.</summary>
    public void Broadcast(uint token, uint eventCode, uint first, uint second)
    { if ((Node(token).TextFlags & 4) == 0) return; int y = unchecked((int)second), delta = unchecked(y - Node(token).DragY); if (delta != 0) { Node(token).DragY = y; _text.RequestRelativeScroll(token, unchecked(-delta)); } }
    /// <summary>현재 소유 버퍼만 소멸/free하고 현재 포인터0을 쓴 뒤 기본 scalar로 위임한다.빌린 포인터는 보존한다.</summary>
    public uint DeleteScalar(uint token, uint flags)
    { Node(token).Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x51454cU : 0x505d80U; uint buffer = Node(token).Buffer; if (buffer != 0 && Node(token).OwnsBuffer != 0) { _buffers.Destroy(buffer); Node(token).Buffer = 0; } return _lifetime.DeleteScalar(token, flags); }
    /// <summary>현재 다중 줄 분류만 실제 Attach/사건/캡처/프레임/scalar로 연결하고 이전 분배를 보존한다.</summary>
    public static void BindScrollingTextGumpWidgets(GumpTreeHooks tree, GumpEventHooks events, GumpInputHooks input, GumpFrameHooks frame, GumpLifetimeHooks lifetime, Func<uint, ScrollingTextGumpWidgets?> resolve)
    {
        ArgumentNullException.ThrowIfNull(resolve); var attach = tree.Attach; tree.Attach = (t, x, y, p, m) => { if (resolve(t) is { } w) w.Attach(t, x, y, p, m); else Require(attach)(t, x, y, p, m); };
        // 부모/재귀 사건과 직접 캡처에 같은 현재 원본 Handle을 분배한다.
        uint Route(Func<uint, uint, uint, uint, uint>? previous, uint t, uint c, uint a, uint b) => resolve(t) is { } w ? w.Handle(t, c, a, b) : Require(previous)(t, c, a, b);
        var e = events.Handle; var i = input.Handle; events.Handle = (t, c, a, b) => Route(e, t, c, a, b); input.Handle = (t, c, a, b) => Route(i, t, c, a, b); var broadcast = frame.Broadcast; frame.Broadcast = (t, c, a, b) => { if (resolve(t) is { } w) w.Broadcast(t, c, a, b); else Require(broadcast)(t, c, a, b); }; var deleting = lifetime.DeletingDestructor; lifetime.DeletingDestructor = (t, f) => { if (resolve(t) is { } w) w.DeleteScalar(t, f); else Require(deleting)(t, f); };
    }
}
