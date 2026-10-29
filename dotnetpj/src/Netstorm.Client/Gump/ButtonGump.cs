using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 트리/입력/수명의 실제 버튼 상태 처리 영역이다.

/// <summary>원본 가상 값/누름 구현을 선택하는 호스트 분류. 원본 객체에 저장되는 필드는 아니다.</summary>
public enum ButtonGumpKind
{
    /// <summary>기본 버튼.</summary>
    Base,
    /// <summary>배경 자식이 눌림을 표시하는 돌 버튼.</summary>
    Stone,
}

/// <summary>기본0xa0 노드와 같은 객체의 버튼 추가 상태.
/// 근거: cpppj/src/client/ButtonGump.h.</summary>
public class ButtonGumpNode : GumpCreateNode
{
    /// <summary>+a0 토글/부모 통지 DWORD 값.</summary>
    public uint Value { get; set; }
    /// <summary>+a4 현재 눌림 DWORD.</summary>
    public uint Pressed { get; set; }
    /// <summary>+a8 점멸 DWORD.</summary>
    public uint Blink { get; set; }
    /// <summary>+ac 비활성/토글/즉시/pulse/팝업 유지 비트.</summary>
    public uint ButtonFlags { get; set; }
    /// <summary>+b0 첫 반복 또는 점멸의 벽 시계 기한.</summary>
    public double FirstDeadline { get; set; }
    /// <summary>+b8 다음 반복 통지 기한.</summary>
    public double RepeatDeadline { get; set; }
    /// <summary>+100 현재 배경 자식 토큰.</summary>
    public uint Background { get; set; }
    /// <summary>호스트의 실제 가상 상태 분배 정보.</summary>
    public ButtonGumpKind Kind { get; set; }
}

/// <summary>실제 초기화한 문자열 사건 ID와 기본 버튼의 signed 색 전역이다.</summary>
public sealed class ButtonGumpState
{
    /// <summary>serial 대상의 값 변경 사건.</summary>
    public uint ValueEvent { get; set; }
    /// <summary>부모 왼쪽 클릭 통지 사건.</summary>
    public uint ClickEvent { get; set; }
    /// <summary>부모 우클릭 통지 사건.</summary>
    public uint RightEvent { get; set; }
    /// <summary>부모 누름 유지 반복 통지 사건.</summary>
    public uint RepeatEvent { get; set; }
    /// <summary>기본 command에 signed 확장할 색 BYTE.</summary>
    public sbyte BaseColor { get; set; }
}

/// <summary>실제 배경+b4 저장 주소를 ref로 공급한다. C++ null 주소는 조회자가 명시 저장소 예외로 바꾼다.</summary>
public delegate ref uint ButtonBackgroundFlags(uint token);

/// <summary>같은 노드·실제 벽 시계/폴링·가상 상태 함수와 장치 경계다.</summary>
public sealed class ButtonGumpHooks
{
    /// <summary>안정된 실제 버튼 노드.</summary>
    public Func<uint, ButtonGumpNode?>? Node { get; set; }
    /// <summary>버튼/팝업을 포함한 같은 기본 노드.</summary>
    public Func<uint, GumpCreateNode?>? BaseNode { get; set; }
    /// <summary>현재 배경의 실제 플래그 저장 주소.</summary>
    public ButtonBackgroundFlags? BackgroundFlags { get; set; }
    /// <summary>호출마다 다시 읽는 벽 시계 초.</summary>
    public Func<double>? Now { get; set; }
    /// <summary>원본 왼쪽 버튼 폴링 사건.</summary>
    public Func<uint, InputEvent>? Poll { get; set; }
    /// <summary>가상 재그리기 요청. 두 번째 인자는false다.</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>button.wav 실제 재생 경계.</summary>
    public Action? Sound { get; set; }
    /// <summary>그 밖의 기본 도움말 사건 처리.</summary>
    public Func<uint, uint, uint, uint, uint>? BaseEvent { get; set; }
    /// <summary>선택적 파생 값 가상 함수.</summary>
    public Action<uint, uint>? Value { get; set; }
    /// <summary>선택적 파생 누름 가상 함수.</summary>
    public Action<uint, uint>? Pressed { get; set; }
    /// <summary>선택적 파생 pulse 가상 함수. 전체DWORD 인자를 유지한다.</summary>
    public Action<uint, uint>? Pulse { get; set; }
    /// <summary>선택적 파생 점멸 가상 함수.</summary>
    public Action<uint, uint>? Blink { get; set; }
}

/// <summary>실제 생성·입력·부모 전파·통지·idle와 돌 버튼의 배경 상태를 연결한다.
/// 근거: cpppj/src/client/ButtonGump.cpp, 004249a0·00424cd0·00424dd0/CD00440ce0·00441050·00441170.</summary>
public sealed class ButtonGump
{
    /// <summary>비활성 버튼 비트.</summary>
    private const uint Disabled = 1;
    /// <summary>토글 버튼 비트.</summary>
    private const uint Toggle = 2;
    /// <summary>누름 즉시 통지 비트.</summary>
    private const uint Instant = 4;
    /// <summary>idle 점멸 유지 비트.</summary>
    private const uint PulseFlag = 0x10;
    /// <summary>입력 때 팝업을 닫지 않는 비트.</summary>
    private const uint KeepPopups = 0x20;
    /// <summary>등록 표의 프레임 통지 비트.</summary>
    private const uint NotifyFlag = 0x100;
    /// <summary>루트 직계 팝업 비트.</summary>
    private const uint Popup = 0x4000;
    /// <summary>삭제 대기 비트.</summary>
    private const uint Dead = 0x10000;
    /// <summary>사건 종류의 하위 비트.</summary>
    private const uint EventMask = 0x03ffffff;
    /// <summary>첫 반복 기한의 원본double 초.</summary>
    private const double FirstDelay = 0.4;
    /// <summary>후속 반복 기한의 원본double 초.</summary>
    private const double RepeatDelay = 0.05;
    /// <summary>점멸 기한의 원본double 초.</summary>
    private const double BlinkDelay = 1.0;
    /// <summary>같은 실제 등록/루트 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 사건 ID/색 전역.</summary>
    private readonly ButtonGumpState _state;
    /// <summary>기본 생성/등록 처리기.</summary>
    private readonly GumpCreate _factory;
    /// <summary>같은 현재 위치/로컬 좌표 처리기.</summary>
    private readonly GumpCollection _collection;
    /// <summary>실제 부모 사건 전파.</summary>
    private readonly GumpEvents _events;
    /// <summary>실제 캡처 상태 처리기.</summary>
    private readonly GumpInput _input;
    /// <summary>실제 idle/삭제 요청 처리기.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>노드/가상 상태/장치 경계.</summary>
    private readonly ButtonGumpHooks _hooks;

    /// <summary>같은 생성/좌표/입력/수명 저장소를 빌리며 버튼/기본 노드 조회만 필수다.</summary>
    public ButtonGump(GumpCreateState create, ButtonGumpState state, GumpCreate factory, GumpCollection collection, GumpEvents events, GumpInput input, GumpLifetime lifetime, ButtonGumpHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(events); ArgumentNullException.ThrowIfNull(input); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null || hooks.BaseNode is null) throw new ArgumentException("버튼/기본 Gump 저장소 필요"); _create = create; _state = state; _factory = factory; _collection = collection; _events = events; _input = input; _lifetime = lifetime; _hooks = hooks;
    }
    /// <summary>실제 도달한 경계만 필수로 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("버튼 가상/장치 경계 필요");
    /// <summary>기본/파생이 같은 객체인 토큰만 반환한다.</summary>
    private ButtonGumpNode Node(uint token)
    {
        ButtonGumpNode? node = token != 0 ? _hooks.Node!(token) : null;
        if (node is null || !ReferenceEquals(node, _hooks.BaseNode!(token))) throw new InvalidOperationException("버튼 Gump 토큰/공유 저장소 없음"); return node;
    }
    /// <summary>버튼 이외 팝업도 같은 기본 저장소로 읽는다.</summary>
    private GumpCreateNode BaseNode(uint token) => (token != 0 ? _hooks.BaseNode!(token) : null) ?? throw new InvalidOperationException("버튼 기본 Gump 토큰 없음");
    /// <summary>원본의 각 예약마다 실제 시계를 다시 조회한다.</summary>
    private double Now() => Require(_hooks.Now)();
    /// <summary>실제 Draw와 구분되는 가상 redraw(false).</summary>
    private void Redraw(uint token) => Require(_hooks.Redraw)(token, false);

    /// <summary>004253e0의 기본 생성 뒤 버튼 필드를 초기화한다. 미작성 파생 필드는 보존한다.</summary>
    public uint ConstructBase(uint token, int width, int height, uint flags)
    {
        ButtonGumpNode node = Node(token); _factory.Construct(token, 0, 0, 0, 1); node.Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x503848U : 0x5017d8U;
        node.FirstDeadline = node.RepeatDeadline = 0; node.Command = unchecked((uint)(int)_state.BaseColor); node.Flags |= 0xa00; node.ButtonFlags = flags; node.Value = node.Pressed = node.Blink = 0; node.Kind = ButtonGumpKind.Base;
        node.BaseWidth = width; node.Width = (float)((double)width * node.Scale); node.BaseHeight = height; node.Height = (float)((double)height * node.Scale); return token;
    }
    /// <summary>현재 배경 참조를 parent redraw 뒤 읽고 두 상태OR의 변화 때만 배경 눌림/재그리기를 갱신한다.</summary>
    private void WriteState(uint token, uint value, bool pressed)
    {
        ButtonGumpNode node = Node(token); bool old = node.Value != 0 || node.Pressed != 0; ButtonGumpKind kind = node.Kind;
        if (pressed) node.Pressed = value; else node.Value = value; Redraw(token); bool current = node.Value != 0 || node.Pressed != 0;
        if (kind == ButtonGumpKind.Stone && old != current)
        {
            ref uint flags = ref Require(_hooks.BackgroundFlags)(node.Background); flags = current ? flags | GumpBackgroundFlags.Pressed : flags & ~GumpBackgroundFlags.Pressed; Redraw(node.Background);
        }
    }
    /// <summary>같은 값도 원본처럼 부모 redraw를 요청한다.</summary>
    public void SetValue(uint token, uint value) => WriteState(token, value, false);
    /// <summary>누름 값의 전체DWORD를 보존한다.</summary>
    public void SetPressed(uint token, uint value) => WriteState(token, value, true);
    /// <summary>비활성 비트만 바꾸고 캡처/idle는 해제하지 않는다.</summary>
    public void SetEnabled(uint token, bool enabled) { ButtonGumpNode n = Node(token); n.ButtonFlags = enabled ? n.ButtonFlags & ~Disabled : n.ButtonFlags | Disabled; Redraw(token); }
    /// <summary>점멸 지정은 redraw를 추가하지 않는다.</summary>
    public void SetBlink(uint token, uint value) => Node(token).Blink = value;
    /// <summary>pulse 비트를 먼저 바꾸고 실제 idle 표/비트를 연결한다.</summary>
    public void SetPulse(uint token, bool enabled) { ButtonGumpNode n = Node(token); n.ButtonFlags = enabled ? n.ButtonFlags | PulseFlag : n.ButtonFlags & ~PulseFlag; _lifetime.SetIdle(token, enabled); }
    /// <summary>로컬 float 영역의 오른쪽/아래 제외 비교다. NaN/음수는 제외하고 -0은 원점이다.</summary>
    public bool Contains(uint token, (float X, float Y) local) { ButtonGumpNode n = Node(token); return local.X >= 0 && local.X < n.Width && local.Y >= 0 && local.Y < n.Height; }
    /// <summary>선택적 파생 값 가상 함수 또는 실제 기본 setter.</summary>
    private void Value(uint token, uint value) { if (_hooks.Value is { } call) call(token, value); else SetValue(token, value); }
    /// <summary>선택적 파생 누름 가상 함수 또는 실제 기본 setter.</summary>
    private void Pressed(uint token, uint value) { if (_hooks.Pressed is { } call) call(token, value); else SetPressed(token, value); }
    /// <summary>전체DWORD pulse 가상 인자를 기본 구현에서만bool로 좁힌다.</summary>
    private void Pulse(uint token, uint value) { if (_hooks.Pulse is { } call) call(token, value); else SetPulse(token, value != 0); }
    /// <summary>점멸 가상 함수에 redraw를 추가하지 않는다.</summary>
    private void Blink(uint token, uint value) { if (_hooks.Blink is { } call) call(token, value); else SetBlink(token, value); }
    /// <summary>현재serial과 값을 실제 부모 체인에 통지하며 반환값은 버린다.</summary>
    private void Notify(uint token, uint eventCode, uint value) => _events.Bubble(token, eventCode, Node(token).Serial, value);
    /// <summary>각 기한의 벽 시계를 별도 호출한다.</summary>
    private void ResetDeadlines(uint token) { Node(token).FirstDeadline = Now() + FirstDelay; Node(token).RepeatDeadline = Now() + RepeatDelay; }
    /// <summary>현재 루트 직계 팝업의 next를Kill 전에 저장한다. 실제 해제는 후속 Cleanup이 맡는다.</summary>
    private void ClosePopups()
    {
        uint token = BaseNode(_create.Entries[0]).Child; var seen = new HashSet<uint>();
        // visible/void를 추가 검사하지 않고 저장한 다음 형제로 진행한다.
        while (token != 0)
        {
            if (seen.Count >= GumpCreateState.RegistryCapacity || !seen.Add(token)) throw new InvalidOperationException("버튼 팝업 형제 순환/한도 초과"); GumpCreateNode n = BaseNode(token); uint next = n.Next;
            if ((n.Flags & Popup) != 0 && (n.Flags & Dead) == 0) _lifetime.Kill(token); token = next;
        }
    }
    /// <summary>현재 버튼/사건으로 일반·토글·즉시·우클릭·값 명령을 처리한다. 값 명령 뒤에도 사건 분기를 계속한다.</summary>
    public uint Handle(uint token, uint eventCode, uint first, uint second)
    {
        ButtonGumpNode n = Node(token); if ((n.ButtonFlags & Disabled) != 0) return 0; uint code = eventCode & EventMask; bool release = (eventCode & InputCode.Release) != 0;
        if (code == _state.ValueEvent && first == n.Serial) { Value(token, second); Redraw(token); Notify(token, _state.ClickEvent, n.Value); }
        if (n.AlternateTextPointer == 0 && code == InputCode.RightButton && !release) { if ((n.ButtonFlags & KeepPopups) == 0) ClosePopups(); Notify(token, _state.RightEvent, n.Value); return 1; }
        if (code == InputCode.LeftButton && !release)
        {
            if ((n.ButtonFlags & KeepPopups) == 0) ClosePopups();
            if ((n.ButtonFlags & Instant) == 0) { ResetDeadlines(token); _input.Capture(token); n.Flags |= NotifyFlag; _lifetime.SetIdle(token, true); Pressed(token, 1); }
            else if ((n.ButtonFlags & Toggle) != 0) { Value(token, n.Value == 0 ? 1U : 0); Notify(token, _state.ClickEvent, n.Value); }
            else if (n.Value == 0) { Value(token, 1); Notify(token, _state.ClickEvent, n.Value); }
            Require(_hooks.Sound)(); Redraw(token); return 1;
        }
        if (code == InputCode.LeftButton && release && (n.ButtonFlags & Instant) == 0)
        {
            var local = _collection.ScreenToLocal(token, new(unchecked((int)first), unchecked((int)second)));
            if (!Contains(token, local)) { n.Flags &= ~NotifyFlag; _lifetime.SetIdle(token, false); Pressed(token, 0); return 1; }
            if ((n.ButtonFlags & Toggle) != 0) { Value(token, n.Value == 0 ? 1U : 0); Notify(token, _state.ClickEvent, n.Value); }
            else if (n.Pressed != 0) Notify(token, _state.ClickEvent, n.Value);
            n.Flags &= ~NotifyFlag; _lifetime.SetIdle(token, false); Pressed(token, 0); Redraw(token); return 1;
        }
        return Require(_hooks.BaseEvent)(token, eventCode, first, second);
    }
    /// <summary>현재 폴링 release와 전달 좌표를 별개로 읽으며 비활성 여부를 추가 필터하지 않는다.</summary>
    public void Broadcast(uint token, uint eventCode, uint first, uint second)
    {
        ButtonGumpNode n = Node(token); if ((n.ButtonFlags & Instant) != 0) throw new InvalidOperationException("즉시 버튼은 원본 통지 대상이 아님");
        if ((Require(_hooks.Poll)(InputCode.LeftButton).Code & InputCode.Release) != 0) n.Flags &= ~NotifyFlag;
        var local = _collection.ScreenToLocal(token, new(unchecked((int)first), unchecked((int)second))); bool inside = Contains(token, local);
        uint value = (n.ButtonFlags & Toggle) != 0 ? inside ? (n.Value == 0 ? 1U : 0) : n.Value : inside ? 1U : 0;
        if (value != n.Pressed) { Pressed(token, value); ResetDeadlines(token); Redraw(token); }
    }
    /// <summary>만료한 프레임마다 최대 한 번 통지한다. pulse 해제 뒤 현재 플래그/누름/기한을 다시 읽는다.</summary>
    public void Idle(uint token)
    {
        ButtonGumpNode n = Node(token);
        if ((n.ButtonFlags & PulseFlag) != 0)
        {
            if (n.Value != 0 || n.Pressed != 0) Pulse(token, 0);
            else { double now = Now(); if (n.FirstDeadline <= now) { Blink(token, n.Blink == 0 ? 1U : 0); n.FirstDeadline = Now() + BlinkDelay; } }
        }
        if ((n.ButtonFlags & Instant) == 0 && n.Pressed != 0)
        {
            double firstNow = Now(); if (n.FirstDeadline <= firstNow) { double repeatNow = Now(); if (n.RepeatDeadline <= repeatNow) { Notify(token, _state.RepeatEvent, 1); n.RepeatDeadline = Now() + RepeatDelay; } }
        }
    }
    /// <summary>현재 경계 조회 뒤 value/pressed의 signed DWORD 비트를 출력 계획에 전달한다.</summary>
    public ButtonVisualState VisualState(uint token, int textWidth, int textHeight)
    {
        ScreenRect rect = _collection.Bounds(token); ButtonGumpNode n = Node(token); return new(rect, textWidth, textHeight, unchecked((int)n.Value), unchecked((int)n.Pressed), n.ButtonFlags);
    }
    /// <summary>현재 버튼 토큰만 실제 입력 몸체로 보내고 기존 층별 처리기를 보존한다.</summary>
    private static Func<uint, uint, uint, uint, uint> BindHandle(Func<uint, uint, uint, uint, uint>? previous, Func<uint, ButtonGump?> resolve) => (t, e, a, b) => resolve(t) is { } button ? button.Handle(t, e, a, b) : Require(previous)(t, e, a, b);
    /// <summary>선택/캡처/idle/통지의 기존 가상 분배를 감싼다. resolve는 호출 때 현재 처리기를 읽어 순환 의존을 연결한다.</summary>
    public static void BindButtonGump(GumpEventHooks events, GumpInputHooks input, GumpFrameHooks frame, Func<uint, ButtonGump?> resolve)
    {
        ArgumentNullException.ThrowIfNull(resolve); events.Handle = BindHandle(events.Handle, resolve); input.Handle = BindHandle(input.Handle, resolve); Action<uint>? idle = frame.Idle; Action<uint, uint, uint, uint>? broadcast = frame.Broadcast;
        frame.Idle = token => { if (resolve(token) is { } b) b.Idle(token); else Require(idle)(token); };
        frame.Broadcast = (t, e, a, b) => { if (resolve(t) is { } button) button.Broadcast(t, e, a, b); else Require(broadcast)(t, e, a, b); };
    }
}
