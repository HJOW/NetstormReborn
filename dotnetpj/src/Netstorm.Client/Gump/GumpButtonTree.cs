using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump;

/// <summary>실제 호스트가 버튼 트리에 공급하는 현재 입력·시계·출력 효과다.</summary>
public sealed class GumpButtonTreeHooks
{
    /// <summary>현재 벽시계 초를 읽는다.</summary>
    public required Func<double> Now { get; init; }
    /// <summary>현재 버튼의 물리 폴링 상태를 읽는다.</summary>
    public required Func<uint, InputEvent> Poll { get; init; }
    /// <summary>버튼 소리를 재생한다.</summary>
    public required Action Sound { get; init; }
    /// <summary>변경된 실제 노드와 분리 여부를 화면 갱신에 알린다.</summary>
    public required Action<uint, bool> Redraw { get; init; }
    /// <summary>기본 또는 부모 노드의 현재 사건을 처리한다.</summary>
    public required Func<uint, uint, uint, uint, uint> HandleBase { get; init; }
}

/// <summary>실제 돌 버튼·배경·공통 Gump 등록표와 입력/수명을 같은 노드 저장소로 조립한다.
/// 근거: cpppj/src/client/RawButtonControls.cpp의 공통 트리와 StoneButtonGump 연결.</summary>
public sealed class GumpButtonTree
{
    /// <summary>실제 모든 기본·파생 노드의 안정된 소유 저장소.</summary>
    private readonly Dictionary<uint, GumpCreateNode> _nodes = [];
    /// <summary>다음에 할당할 비영 토큰.</summary>
    private uint _nextToken;
    /// <summary>같은 원본 등록표와 화면 크기.</summary>
    public GumpCreateState Create { get; }
    /// <summary>같은 캡처·idle·삭제 표.</summary>
    public GumpLifetimeState LifeState { get; } = new();
    /// <summary>기본 및 파생 공통 생성기.</summary>
    public GumpCreate Factory { get; }
    /// <summary>추가 파생 Gump가 기존 가상 부착 경로에 연결하는 같은 장치.</summary>
    public GumpTreeHooks TreeHooks { get; }
    /// <summary>추가 파생 Gump가 기존 소멸 경로에 연결하는 같은 장치.</summary>
    public GumpLifetimeHooks LifetimeHooks { get; }
    /// <summary>부모 전파와 재귀 입력이 쓰는 같은 사건 장치.</summary>
    public GumpEventHooks EventHooks { get; }
    /// <summary>캡처 입력이 쓰는 같은 가상 사건 장치.</summary>
    public GumpInputHooks InputHooks { get; }
    /// <summary>프레임 idle과 통지가 쓰는 같은 가상 장치.</summary>
    public GumpFrameHooks FrameHooks { get; }
    /// <summary>현재 위치와 수집기.</summary>
    public GumpCollection Collection { get; }
    /// <summary>부모·자식 부착기.</summary>
    public GumpTree Tree { get; }
    /// <summary>지연 삭제와 자식 우선 파괴.</summary>
    public GumpLifetime Lifetime { get; }
    /// <summary>같은 공통 노드의 재귀 사건 전달과 부모 전파.</summary>
    public GumpEvents Events { get; }
    /// <summary>현재 캡처와 사건 전달.</summary>
    public GumpInput Input { get; }
    /// <summary>현재 버튼 상태 처리기.</summary>
    public ButtonGump Buttons { get; }
    /// <summary>현재 돌 버튼 생성·그리기 처리기.</summary>
    public StoneButtonGump Stones { get; }
    /// <summary>같은 입력·수명·노드 트리의 모달 소유자.</summary>
    public GumpModal Modal { get; }
    /// <summary>프레임별 등록 버튼의 idle과 통지.</summary>
    public GumpFrame Frame { get; }
    /// <summary>생성과 출력이 공유하는 실제 배경 자산.</summary>
    private readonly GumpBackgroundAssets _assets;
    /// <summary>공통 출력기가 같은 실제 배경 자산을 사용한다.</summary>
    internal GumpBackgroundAssets BackgroundAssets => _assets;
    /// <summary>라벨 측정과 출력이 공유하는 실제 글꼴.</summary>
    private readonly FontStore _fonts;
    /// <summary>메뉴와 다중 줄 본문이 공유하는 실제 글꼴 저장소.</summary>
    public FontStore Fonts => _fonts;
    /// <summary>실제 버튼 배경 생성기.</summary>
    private readonly GumpBackgroundCreate _backgrounds;
    /// <summary>같은 실제 배경 생성기를 추가 파생 소유자에 빌려준다.</summary>
    public GumpBackgroundCreate Backgrounds => _backgrounds;
    /// <summary>글자와 배경이 공유하는 현재 VFX 색표.</summary>
    private readonly GumpGlyphColorState _colors = new();
    /// <summary>파일 팔레트와 현재 버튼 배경 타입.</summary>
    private readonly StoneButtonGumpState _stoneState;

    /// <summary>안정된 등록 노드를 찾는다. 없는 토큰은 null이다.</summary>
    /// <param name="token">원본 포인터를 대신하는 토큰.</param>
    public GumpCreateNode? Node(uint token) => _nodes.GetValueOrDefault(token);

    /// <summary>새 실제 파생 노드를 공통 소유 저장소에 할당한다. 생성과 등록은 각 원본 생성기가 수행한다.</summary>
    /// <param name="node">다른 토큰에 아직 할당되지 않은 실제 기본 또는 파생 객체.</param>
    public uint AllocateNode(GumpCreateNode node)
    {
        ArgumentNullException.ThrowIfNull(node);
        if (_nodes.Values.Any(existing => ReferenceEquals(existing, node))) throw new ArgumentException("이미 소유한 Gump 객체", nameof(node));
        if (_nextToken == uint.MaxValue) throw new OverflowException("Gump 토큰 소진");
        uint token = ++_nextToken; _nodes.Add(token, node); return token;
    }

    /// <summary>등록 전 생성 실패의 노드만 실제 저장소에서 반납한다. 등록된 노드는 자식 우선 수명으로 삭제해야 한다.</summary>
    /// <param name="token">공통 저장소가 소유한 미등록 토큰.</param>
    public void ReleaseUnregistered(uint token)
    {
        if (token == 0 || !_nodes.ContainsKey(token)) throw new ArgumentException("없는 Gump 토큰", nameof(token));
        GumpCreateNode node = _nodes[token];
        if (Create.Entries.Contains(token) || node.Vtable != 0 || node.Parent != 0 || node.Child != 0 || node.Next != 0)
            throw new InvalidOperationException("생성되거나 연결된 Gump는 공통 수명으로 삭제해야 한다");
        _nodes.Remove(token);
    }

    /// <summary>현재 등록표에서 serial이 같은 실제 노드를 찾는다. 죽음 표시와 실제 반납을 구분한다.</summary>
    /// <param name="serial">원본 생성기가 부여한 serial. 0은 항상 없음이다.</param>
    public uint Reference(uint serial)
    {
        if (serial == 0) return 0;
        // 등록표 순서대로 현재 노드의 serial을 조회한다.
        foreach (uint token in Create.Entries) if (token != 0 && Node(token)?.Serial == serial) return token;
        return 0;
    }

    /// <summary>같은 배경의 실제 플래그 저장 위치를 버튼에 빌려준다.</summary>
    private ref uint BackgroundFlags(uint token)
    {
        if (Node(token) is not GumpBackgroundNode background) throw new InvalidOperationException("버튼의 실제 배경 노드가 없다");
        return ref background.BackgroundFlags;
    }

    /// <summary>요청 크기별 기본·돌 버튼·배경 파생을 실제 소유하고 토큰을 등록한다.</summary>
    private uint Allocate(int size)
    {
        GumpCreateNode node = size switch
        {
            GumpCreateState.BaseSize => new GumpCreateNode(),
            0x170 => new StoneButtonGumpNode(),
            0xb8 => new GumpBackgroundNode(),
            _ => throw new ArgumentOutOfRangeException(nameof(size), "이 트리의 미연결 Gump 파생 크기")
        };
        return AllocateNode(node);
    }

    /// <summary>실제 화면과 자산·글꼴 및 호스트 장치를 사용해 루트와 공통 처리기를 만든다.</summary>
    /// <param name="width">논리 화면 폭.</param><param name="height">논리 화면 높이.</param>
    /// <param name="assets">실제 버튼 배경 자산.</param><param name="backgroundType">배경의 실제 타입 번호.</param>
    /// <param name="backgroundFrame">실제 A01 프레임 번호.</param><param name="palette">실제 버튼 색.</param>
    /// <param name="fonts">같은 원본 글꼴 저장소.</param><param name="hooks">실제 호스트 입력·출력 장치.</param>
    public GumpButtonTree(int width, int height, GumpBackgroundAssets assets, uint backgroundType, uint backgroundFrame,
        ButtonPalette palette, FontStore fonts, GumpButtonTreeHooks hooks)
    {
        if (width <= 0 || height <= 0) throw new ArgumentOutOfRangeException(nameof(width));
        ArgumentNullException.ThrowIfNull(assets); ArgumentNullException.ThrowIfNull(fonts); ArgumentNullException.ThrowIfNull(hooks);
        _assets = assets; _fonts = fonts;
        Create = new() { ScreenWidth = width, ScreenHeight = height };
        Create.Collection.Edition = OriginalEdition.Patch1078;
        _stoneState = new() { Color = 49, Palette = palette, BackgroundType = backgroundType, BackgroundFrame = backgroundFrame };
        _stoneState.DefaultContext[6] = _stoneState.DefaultContext[7] = 1;
        Factory = new(Create, new() { Node = Node, Allocate = Allocate }); Factory.InitializeRoot();
        Collection = new(Create.Collection, Factory.CollectionHooks());
        var treeHooks = TreeHooks = new GumpTreeHooks { Node = Node, Redraw = hooks.Redraw,
            Push = token => Tree!.Push(token), Attach = (token, x, y, parent, mode) => Tree!.Attach(token, x, y, parent, mode),
            Kill = token => Lifetime!.Kill(token) };
        var lifeHooks = LifetimeHooks = new GumpLifetimeHooks { Node = Node, Push = treeHooks.Push,
            DeletingDestructor = (token, flags) => Lifetime!.DeleteScalar(token, flags),
            Release = token => { if (!_nodes.Remove(token)) throw new InvalidOperationException("Gump 저장소 반납 불일치"); } };
        StoneButtonGump.BindStoneButtonGump(treeHooks, lifeHooks, token => Node(token) is StoneButtonGumpNode ? Stones : null);
        Tree = new(Create, new() { MapView = new(0, 0, width, height) }, treeHooks);
        Lifetime = new(Create, LifeState, lifeHooks);
        var eventsHooks = EventHooks = new GumpEventHooks { Node = Node, Handle = hooks.HandleBase };
        var inputHooks = InputHooks = new GumpInputHooks { Node = Node, Handle = hooks.HandleBase };
        var frameHooks = FrameHooks = new GumpFrameHooks { Node = Node, Redraw = hooks.Redraw, Broadcast = (_, _, _, _) => { } };
        ButtonGump.BindButtonGump(eventsHooks, inputHooks, frameHooks, token => Node(token) is StoneButtonGumpNode ? Buttons : null);
        var events = Events = new GumpEvents(Create, LifeState, new(), Collection, eventsHooks);
        Input = new(Create, LifeState, new() { OverlayEvent = 0x0200003b }, Collection, events, inputHooks);
        Frame = new(Create, LifeState, frameHooks);
        Modal = new(Node, LifeState, Input, Lifetime, Tree, () => hooks.Redraw(Create.Entries[0], false));
        Buttons = new(Create, new() { ValueEvent = 0x02000070, ClickEvent = 0x0200002c, RightEvent = 0x02000012,
            RepeatEvent = 0x0200002f, BaseColor = 56 }, Factory, Collection, events, Input, Lifetime,
            new() { Node = token => Node(token) as ButtonGumpNode, BaseNode = Node, BackgroundFlags = BackgroundFlags,
                Redraw = hooks.Redraw, Now = hooks.Now, Poll = hooks.Poll, Sound = hooks.Sound, BaseEvent = hooks.HandleBase });
        var backgroundHooks = new GumpBackgroundCreateHooks { Node = token => Node(token) as GumpBackgroundNode };
        GumpBackgroundAssets.BindGumpBackgroundAssets(backgroundHooks, new(), assets);
        _backgrounds = new(Create, Factory, () => 35, backgroundHooks);
        var stoneHooks = new StoneButtonGumpHooks { Node = token => Node(token) as StoneButtonGumpNode,
            Allocate = Allocate, Push = treeHooks.Push, Attach = treeHooks.Attach };
        GumpText.BindStoneButtonText(stoneHooks, new GumpText(fonts));
        Stones = new(Create, _stoneState, Factory, Buttons, _backgrounds, Tree, Lifetime, stoneHooks);
    }

    /// <summary>현재 루트에 실제 돌 버튼과 배경을 만들고 라벨 측정 후 지정 좌표에 부착한다.</summary>
    /// <param name="width">버튼 폭.</param><param name="height">버튼 높이.</param><param name="label">NUL 종료된 원본 글자 바이트.</param>
    /// <param name="x">루트 기준 X.</param><param name="y">루트 기준 Y.</param>
    public uint AddStone(int width, int height, byte[] label, int x, int y)
    {
        ArgumentNullException.ThrowIfNull(label);
        uint token = Allocate(0x170);
        Stones.Construct(token, width, height, label);
        Stones.SetLabel(token, label);
        Tree.Move(token, x, y, Create.Entries[0]);
        return token;
    }

    /// <summary>같은 실제 표면과 파일 색표에 버튼 배경을 먼저 그리고 부모 선·원본 글리프를 그린다.</summary>
    /// <param name="token">등록된 돌 버튼 토큰.</param><param name="target">호출자가 잠근 실제 8비트 표면.</param>
    /// <param name="clip">표면과 공유하는 현재 Gump 클립.</param>
    /// <param name="maps">파일 팔레트의 명암표.</param><param name="palette">파일 팔레트의 색 번호 표.</param>
    public void DrawStone(uint token, GumpPixelTarget target, GumpDrawState clip, GumpShadeMaps maps, PaletteColorTable palette)
    {
        if (Node(token) is not StoneButtonGumpNode stone) throw new ArgumentException("돌 버튼 토큰 필요", nameof(token));
        var text = new GumpTextDraw(_fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(target, _colors));
        var backgroundState = new GumpBackgroundDrawState(); GumpBackgroundDraw.SetGumpBackgroundColors(backgroundState, palette);
        var hooks = new GumpBackgroundDrawHooks { Node = t => Node(t) as GumpBackgroundNode };
        GumpBackgroundAssets.BindGumpBackgroundAssets(new(), hooks, _assets);
        var pixels = new GumpBackgroundPixelTarget(target, _assets, maps, _colors.VfxColors);
        var background = new GumpBackgroundDraw(Collection, clip, backgroundState, pixels.Hooks(clip, hooks));
        var dispatch = new GumpDrawHooks();
        StoneButtonGump.BindStoneButtonDraw(dispatch, Collection, text, target, t => t == token ? Stones : null);
        GumpBackgroundDraw.BindGumpBackgroundDraw(dispatch, background, t => t == stone.Background);
        var draw = new GumpDraw(Collection, clip, dispatch);
        draw.Draw(stone.Background); draw.Draw(token);
    }

    /// <summary>현재 버튼과 배경을 원본 자식 우선 수명으로 삭제 요청한 뒤 정리한다.</summary>
    /// <param name="token">등록된 돌 버튼 토큰.</param>
    public void Remove(uint token) { if (Node(token) is not StoneButtonGumpNode) throw new ArgumentException("돌 버튼 토큰 필요", nameof(token)); Lifetime.Kill(token); Lifetime.Cleanup(); }
}
