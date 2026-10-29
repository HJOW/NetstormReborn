using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 돌 버튼의 실제 라벨/배경 자식/가상 부착/그리기 영역이다.

/// <summary>같은 기본 버튼의 라벨·세 문맥·두 측정값.
/// 근거: cpppj/src/client/StoneButtonGump.h. 측정값은 원본 출력 주소처럼 ref로 공급하는 저장 필드다.</summary>
public class StoneButtonGumpNode : ButtonGumpNode
{
    /// <summary>+c0..ff의64바이트 라벨. 마지막 바이트는 strncpy63이 쓰지 않는다.</summary>
    public byte[] Label { get; } = new byte[64];
    /// <summary>+104/+124/+144의 정상/눌림/비활성 문맥.</summary>
    public GumpTextContext[] Contexts { get; } = new GumpTextContext[3];
    /// <summary>+164 첫 측정 폭.</summary>
    public int TextWidth;
    /// <summary>+168 측정 높이.</summary>
    public int TextHeight;
}

/// <summary>생성 시 현재 색/기본 문맥/파일 팔레트와 배경 타입/프레임 전역이다.</summary>
public sealed class StoneButtonGumpState
{
    /// <summary>command에 signed 확장할 색 BYTE.</summary>
    public sbyte Color { get; set; }
    /// <summary>기본 측정 문맥. 실제 Draw의 세 저장 문맥과 별개다.</summary>
    public uint[] DefaultContext { get; } = new uint[8];
    /// <summary>일시 팔레트가 아닌 마지막 파일의 버튼 색.</summary>
    public ButtonPalette Palette { get; set; }
    /// <summary>할당 뒤 읽는 현재 배경 타입.</summary>
    public uint BackgroundType { get; set; }
    /// <summary>할당 뒤 읽는 현재 배경 프레임.</summary>
    public uint BackgroundFrame { get; set; }
}

/// <summary>같은 현재 라벨/문맥과 실제 두 출력 주소를 원본 측정기에 전달한다.</summary>
public delegate void StoneButtonMeasure(uint token, byte[] label, uint[] context, ref int width, ref int height, uint length);

/// <summary>실제 파생 노드/배경 할당/가상 Push/Attach와 글자 측정 경계다.</summary>
public sealed class StoneButtonGumpHooks
{
    /// <summary>기본 생성/수집과 같은 돌 버튼 객체.</summary>
    public Func<uint, StoneButtonGumpNode?>? Node { get; set; }
    /// <summary>0xb8바이트의 배경 저장소 토큰.0은 원본 할당 실패다.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>배경의 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>배경의 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>원본 바이트 측정. 콜백은 현재 부모 scale/크기도 바꿀 수 있다.</summary>
    public StoneButtonMeasure? Measure { get; set; }
}

/// <summary>기본 버튼/생성/트리/수명의 같은 저장소에 실제 돌 버튼을 조립한다.
/// 근거: cpppj/src/client/StoneButtonGump.cpp, 004254b0·00424f90·00424f00·00424ff0/CD00441310·004415c0·00441520·00441660.</summary>
public sealed class StoneButtonGump
{
    /// <summary>원본 배경 객체의 할당 요청 크기.</summary>
    private const int BackgroundBytes = 0xb8;
    /// <summary>마지막 라벨 바이트를 보존하는 복사 길이.</summary>
    private const int LabelCopyBytes = 63;
    /// <summary>같은 현재 판본/등록 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 색/문맥/배경 전역.</summary>
    private readonly StoneButtonGumpState _state;
    /// <summary>같은 기본 노드 저장소/생성.</summary>
    private readonly GumpCreate _factory;
    /// <summary>현재 기본/돌 버튼 상태 처리기.</summary>
    private readonly ButtonGump _buttons;
    /// <summary>같은 배경 파생 생성기.</summary>
    private readonly GumpBackgroundCreate _backgrounds;
    /// <summary>실제 기본 트리 부착.</summary>
    private readonly GumpTree _tree;
    /// <summary>자식 우선 파괴/기본 scalar 소멸자.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>현재 파생 저장소/가상 경계.</summary>
    private readonly StoneButtonGumpHooks _hooks;
    /// <summary>실제 도달한 경계만 필수로 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("돌 버튼 가상/저장소 경계 필요");

    /// <summary>같은 생성/트리/수명 처리기를 빌린다. 파생 노드 조회만 생성 시 필수다.</summary>
    public StoneButtonGump(GumpCreateState create, StoneButtonGumpState state, GumpCreate factory, ButtonGump buttons, GumpBackgroundCreate backgrounds, GumpTree tree, GumpLifetime lifetime, StoneButtonGumpHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(buttons); ArgumentNullException.ThrowIfNull(backgrounds); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(hooks);
        Require(hooks.Node); _create = create; _state = state; _factory = factory; _buttons = buttons; _backgrounds = backgrounds; _tree = tree; _lifetime = lifetime; _hooks = hooks;
    }
    /// <summary>기본 생성/수집과 같은 파생 객체인지 호출 때 검사한다.</summary>
    private StoneButtonGumpNode Node(uint token)
    {
        StoneButtonGumpNode? n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _factory.CollectionHooks().Node!(token))) throw new InvalidOperationException("돌 버튼 토큰/공유 저장소 없음"); return n;
    }
    /// <summary>배경은 다른 파생이므로 실제 기본 생성 저장소에서 읽는다.</summary>
    private GumpCreateNode BaseNode(uint token) => (token != 0 ? _factory.CollectionHooks().Node!(token) as GumpCreateNode : null) ?? throw new InvalidOperationException("돌 버튼 배경/기본 저장소 없음");
    /// <summary>원본 strncpy63처럼 첫NUL 이후0으로 채우며 label63은 쓰지 않는다. 안전 범위를 벗어난 non-NUL 소스는 부분 쓰기 뒤 거부한다.</summary>
    private static void CopyLabel(StoneButtonGumpNode node, byte[]? label)
    {
        if (label is null) { node.Label[0] = 0; return; } bool ended = false;
        // 첫NUL 뒤 소스를 읽지 않으므로 짧은 정상 문자열도 허용한다.
        for (int i = 0; i < LabelCopyBytes; i++)
        {
            if (!ended && i >= label.Length) throw new ArgumentException("돌 버튼 라벨 소스 범위 초과"); byte value = ended ? (byte)0 : label[i]; node.Label[i] = value; if (value == 0) ended = true;
        }
    }
    /// <summary>기본 생성 뒤 세 문맥의 앞4DWORD만 초기화하고 현재 기본 문맥/라벨/배경 자식을 만든다. unused는 읽지 않는다.</summary>
    public uint Construct(uint token, int width, int height, byte[]? label, uint unused = 0, uint[]? context = null)
    {
        StoneButtonGumpNode n = Node(token); _buttons.ConstructBase(token, width, height, 0); n.Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x503898U : 0x501828U; n.Kind = ButtonGumpKind.Stone;
        // extra/그림자 미작성 DWORD를 전체 초기화로 지우지 않는다.
        for (int i = 0; i < n.Contexts.Length; i++) { n.Contexts[i].Flags = n.Contexts[i].Font = n.Contexts[i].Color = n.Contexts[i].Style = 0; }
        n.Command = unchecked((uint)(int)_state.Color); n.Contexts[2].Color = _state.Palette.Black;
        n.Contexts[1].Color = _state.Palette.White; n.Contexts[1].ShadowColor = _state.Palette.Black; n.Contexts[1].Flags |= 2; n.Contexts[1].ShadowX = n.Contexts[1].ShadowY = 1;
        n.Contexts[0].Color = _state.Palette.White; n.Contexts[0].Flags |= 2; n.Contexts[0].ShadowX = n.Contexts[0].ShadowY = 1; n.Contexts[0].ShadowColor = _state.Palette.Black;
        uint[] selected = context ?? _state.DefaultContext; if (selected.Length != 8) throw new ArgumentException("돌 버튼 기본 문맥 크기 오류"); selected.CopyTo(n.TextContext, 0); CopyLabel(n, label);
        n.BaseWidth = width; n.Width = (float)((double)width * n.Scale); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale);
        uint background = Require(_hooks.Allocate)(BackgroundBytes);
        if (background != 0) { uint frame = _state.BackgroundFrame, type = _state.BackgroundType; background = _backgrounds.Construct(background, unchecked(width - 2), unchecked(height - 2), GumpBackgroundFlags.Button, 1, type, frame); }
        n.Background = background; BaseNode(background).SecondaryDepth = 1; Require(_hooks.Attach)(n.Background, 1, 1, token, 0); return token;
    }
    /// <summary>현재 라벨/기본 문맥과 실제 두 출력 필드를 길이-1의 비트로 전달한다.</summary>
    private void Measure(uint token) { StoneButtonGumpNode n = Node(token); Require(_hooks.Measure)(token, n.Label, n.TextContext, ref n.TextWidth, ref n.TextHeight, uint.MaxValue); }
    /// <summary>라벨 변경 뒤 측정만 한다. 자동 크기/배경 부착/재그리기를 추가하지 않는다.</summary>
    public void SetLabel(uint token, byte[] label) { ArgumentNullException.ThrowIfNull(label); CopyLabel(Node(token), label); Measure(token); }
    /// <summary>측정 콜백 뒤 현재 scale로 정수+12/+6의 DWORD 감김과 float 저장 지점을 유지한다.</summary>
    public void FitToText(uint token)
    {
        Measure(token); StoneButtonGumpNode n = Node(token); int width = unchecked(n.TextWidth + 12); n.BaseWidth = width; n.Width = (float)((double)width * n.Scale); int height = unchecked(n.TextHeight + 6); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale);
    }
    /// <summary>배경Push 뒤 현재 포인터/부모 크기로 자식을 먼저 다시 붙이고 기본 부모를 붙인다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        StoneButtonGumpNode n = Node(token); Require(_hooks.Push)(n.Background); GumpCreateNode background = BaseNode(n.Background); double width = (double)n.BaseWidth - 2; background.BaseWidth = (float)width; background.Width = (float)(width * background.Scale);
        GumpCreateNode heightBackground = _create.Collection.Edition == OriginalEdition.Patch1078 ? BaseNode(n.Background) : background; double height = (double)n.BaseHeight - 2; heightBackground.BaseHeight = (float)height; heightBackground.Height = (float)(height * heightBackground.Scale);
        Require(_hooks.Attach)(n.Background, 1, 1, token, 0); _tree.Attach(token, x, y, parent, mode);
    }
    /// <summary>실제 자식 우선 Destroy에 맡기고 별도 배경 free 없이 기본 scalar 소멸자에 위임한다.</summary>
    public uint DeleteScalar(uint token, uint flags) { Node(token); return _lifetime.DeleteScalar(token, flags); }
    /// <summary>현재 측정값과 저장 세 문맥을 원본 출력 계획에 전달한다.</summary>
    public ButtonDrawPlan DrawPlan(uint token) { StoneButtonGumpNode n = Node(token); return GumpVisual.PlanButtonDraw(_buttons.VisualState(token, n.TextWidth, n.TextHeight), n.Contexts, _state.Palette.Edge); }
    /// <summary>선→현재Position→라벨을 출력한다. 배경 자식은 별도 수집 항목이며 측정 기본 문맥과 Draw 문맥을 구별한다.</summary>
    public void Draw(uint token, GumpCollection collection, GumpTextDraw text, GumpPixelTarget target)
    {
        ButtonDrawPlan plan = DrawPlan(token); target.Lines(plan.Lines.ToArray()); StoneButtonGumpNode n = Node(token); GumpTextContext context = plan.Text; text.DrawGump(collection, token, n.Label, plan.TextOffset, n.TextContext, in context);
    }
    /// <summary>현재 돌 버튼의 가상Attach/scalar만 분배하며 다른 파생의 기존 처리기를 보존한다.</summary>
    public static void BindStoneButtonGump(GumpTreeHooks tree, GumpLifetimeHooks lifetime, Func<uint, StoneButtonGump?> resolve)
    {
        ArgumentNullException.ThrowIfNull(resolve); Action<uint, float, float, uint, uint>? attach = tree.Attach; Action<uint, uint>? destroy = lifetime.DeletingDestructor;
        tree.Attach = (t, x, y, p, m) => { if (resolve(t) is { } stone) stone.Attach(t, x, y, p, m); else Require(attach)(t, x, y, p, m); };
        lifetime.DeletingDestructor = (t, f) => { if (resolve(t) is { } stone) stone.DeleteScalar(t, f); else Require(destroy)(t, f); };
    }
    /// <summary>공통 임시 클립 안에서 부모 선/라벨로 위임하고 다른 타입의 가상Draw는 보존한다.</summary>
    public static void BindStoneButtonDraw(GumpDrawHooks hooks, GumpCollection collection, GumpTextDraw text, GumpPixelTarget target, Func<uint, StoneButtonGump?> resolve)
    {
        ArgumentNullException.ThrowIfNull(resolve); Action<uint>? previous = hooks.Draw; hooks.Draw = token => { if (resolve(token) is { } stone) stone.Draw(token, collection, text, target); else Require(previous)(token); };
    }
}
