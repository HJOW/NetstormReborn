using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Automation;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Client.Host;

/// <summary>메인 메뉴에서 상위 명령 소비자로 전달할 원본 명령과 인자다.</summary>
/// <param name="Command">원본 명령 이름.</param><param name="Argument">원본 명령 인자.</param>
public readonly record struct MainMenuAction(string Command, string Argument);

/// <summary>실제 자산과 공통 Gump 트리로 메인 메뉴의 여덟 돌 버튼을 표시한다.
/// 근거: cpppj/src/client/UberGump.cpp의 메인 메뉴 배치와 RawButtonControls.cpp.</summary>
public sealed class HostMainMenu : IUiControlLocator
{
    /// <summary>원본 메인 메뉴의 두 행 순서.</summary>
    private static readonly string[] Labels = ["Campaign", "Multiplayer", "Demo", "Help", "Edit", "Credits", "Options", "Quit"];
    /// <summary>같은 버튼 순서로 원본 상위 메뉴에 전달하는 명령과 인자.</summary>
    private static readonly MainMenuAction[] Actions = [new("Tell", "UCampaign"), new("Tell", "NotImplemented"),
        new("Tell", "Demoville"), new("Menu", "help"), new("AskEditFort", "0"), new("Tell", "Credits"),
        new("Menu", "options"), new("RequestQuitApp", "0")];
    /// <summary>선택된 명령을 현재 상위 메뉴에 전달하는 실제 소비자.</summary>
    private readonly Action<MainMenuAction>? _activate;
    /// <summary>실제 화면과 공통 입력·노드 트리.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>버튼·장식·스크롤이 공유하는 실제 fortGump 타입 번호.</summary>
    public uint BackgroundType { get; }
    /// <summary>fortGump 체크 표시의 J 첫 프레임.</summary>
    internal int MarkerFrame { get; }
    /// <summary>HTML 등 파생 창이 메인 메뉴와 공유하는 실제 Gump 트리.</summary>
    public GumpButtonTree Tree => _tree;
    /// <summary>같은 트리와 파일 자산의 HTML 장식 생성·출력 장치.</summary>
    public HostGumpDecoration Decoration { get; }
    /// <summary>메뉴 표시 순서에 대응하는 실제 버튼 토큰.</summary>
    private readonly uint[] _buttons = new uint[8];

    /// <summary>파일 팔레트를 적용한 논리 화면과 실제 자산으로 버튼 트리를 생성한다.</summary>
    /// <param name="startup">같은 파일 시스템과 Config를 보유한 시작 화면.</param>
    /// <param name="screen">이미 팔레트를 적용한 논리 화면.</param>
    /// <param name="hooks">실제 입력·시계·음향·부모 사건 장치.</param>
    /// <param name="activate">버튼에서 선택한 현재 상위 메뉴 명령 소비자.</param>
    public HostMainMenu(HostStartupScene startup, Screen screen, GumpButtonTreeHooks hooks, Action<MainMenuAction>? activate = null)
    {
        ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(screen); ArgumentNullException.ThrowIfNull(hooks);
        _activate = activate;
        if (screen.PaletteLoaded == 0) throw new InvalidOperationException("메인 메뉴에는 실제 파일 팔레트가 필요하다");
        var types = new TypeCatalog(startup.Files);
        TypeInfo type = types.Find("fortGump") ?? throw new InvalidDataException("fortGump 타입이 없다");
        int frame = type.Definition.Frames.Find('A', TypeFrameTable.DefaultVariant, 1);
        int corners = type.Definition.Frames.Find('I', TypeFrameTable.DefaultVariant, 0);
        MarkerFrame = type.Definition.Frames.Find('J', TypeFrameTable.DefaultVariant, 0);
        if (frame < 0 || corners < 0) throw new InvalidDataException("fortGump A01 또는 I 모서리 프레임이 없다");
        var shapes = new Netstorm.Client.Display.ShapeDatabase(startup.Files.ReadAllBytes("d/_shapes.shp"));
        uint typeNumber = (uint)type.RuntimeIndex; BackgroundType = typeNumber;
        var assets = new GumpBackgroundAssets(shapes, [new(typeNumber, type.LoadIndex, type.Definition.Frames.Codes.Count, type.Flags1)]);
        string face = startup.Configuration.PathSpec("fontFaceName");
        var fonts = new FontStore(startup.Files.TryReadAllBytes, face,
            (_, _, _, _) => throw new InvalidOperationException("원본 비트맵 글꼴 캐시가 없어 GDI 생성 경계가 필요하다"));
        var connected = new GumpButtonTreeHooks { Now = hooks.Now, Poll = hooks.Poll, Sound = hooks.Sound, Redraw = hooks.Redraw,
            HandleBase = (token, code, first, second) => HandleBase(hooks, token, code, first, second) };
        _tree = new GumpButtonTree(screen.Width, screen.Height, assets, typeNumber, (uint)frame, GumpVisual.ButtonColors(screen.Colors()), fonts, connected);
        var cornerAssets = new GumpDecorationAssets(shapes, [new(typeNumber, type.LoadIndex, (uint)corners)]);
        Decoration = new HostGumpDecoration(_tree, assets, cornerAssets, typeNumber, () => hooks.Redraw(_tree.Create.Entries[0], false));
        int left = screen.Width / 2 - 156, top = screen.Height / 2 - 73;
        // 원본 4열 2행의 순서와 폭·행 간격으로 실제 돌 버튼을 만든다.
        // 버튼 높이는 20이다. cpppj 는 컨트롤 영역을 {x, y, x+75, y+kButtonHeight(19)+1}로 만들고(UberGump.cpp 2019행)
        // 돌 버튼을 그 영역의 크기(75×20)로 구성한다(RawButtonControls.cpp 294행). 높이 19로 만들면 아래 테두리가 한 줄 위에 그려져
        // 첫 L4a 비교(0.380)에서 버튼마다 3행씩 달랐다. 그 뒤 20으로 바로잡았다.
        for (int index = 0; index < Labels.Length; index++)
        {
            byte[] label = Encoding.Latin1.GetBytes(Labels[index] + "\0");
            _buttons[index] = _tree.AddStone(75, 20, label, left + (index % 4) * 79, top + (index / 4) * 23);
        }
        // 모달이 열리면 숨길 부품(버튼과 각 배경 자식)을 미리 모아 둔다. 배경은 별도 수집 항목이다.
        // 각 버튼의 실제 배경 자식을 순회해 같은 집합에 넣는다.
        foreach (uint button in _buttons)
        {
            _modalHidden.Add(button);
            if (_tree.Node(button) is StoneButtonGumpNode stone && stone.Background != 0) _modalHidden.Add(stone.Background);
        }
    }

    /// <summary>실제 버튼 serial의 클릭만 상위 메뉴 명령으로 분배하고 나머지 부모 사건은 기존 장치로 넘긴다.</summary>
    private uint HandleBase(GumpButtonTreeHooks hooks, uint token, uint code, uint first, uint second)
    {
        if (code == 0x0200002c)
        {
            // 현재 버튼 노드의 serial을 확인하며 배열 인덱스를 원본 명령 번호로 사용하지 않는다.
            for (int index = 0; index < _buttons.Length; index++)
            {
                if (_tree.Node(_buttons[index])?.Serial != first) continue;
                (_activate ?? throw new InvalidOperationException("메인 메뉴 상위 명령 소비자 미연결"))(Actions[index]);
                return 1;
            }
        }
        return hooks.HandleBase(token, code, first, second);
    }

    /// <summary>실제 큐에서 꺼낸 마우스 사건을 같은 트리의 캡처·재귀 전달로 처리한다.</summary>
    /// <param name="input">코드와 논리 화면 좌표를 담은 입력.</param>
    /// <returns>소비된 Gump 토큰. 0이면 소비하지 않았다.</returns>
    public uint Route(InputEvent input) => _tree.Input.Route(input.Code, unchecked((uint)input.X), unchecked((uint)input.Y));

    /// <summary>현재 메뉴에서 일치하는 첫 버튼의 실제 Gump 경계 중심을 찾는다. 이름은 원본처럼 대소문자를 구분한다.</summary>
    /// <param name="label">찾을 버튼 라벨.</param>
    public ScreenPoint? ControlPoint(string label)
    {
        // 원본 제어 영역 순서로 첫 일치 항목을 선택한다.
        for (int index = 0; index < Labels.Length; index++)
        {
            if (!string.Equals(Labels[index], label, StringComparison.Ordinal)) continue;
            ScreenRect bounds = _tree.Collection.Bounds(_buttons[index]);
            return new(unchecked(bounds.Left + bounds.Right) / 2, unchecked(bounds.Top + bounds.Bottom) / 2);
        }
        return null;
    }

    /// <summary>현재 메뉴에서 일치하는 첫 버튼의 실제 Gump 경계를 찾는다. 팝업 앵커 계산용이다.</summary>
    /// <param name="label">찾을 버튼 라벨.</param>
    /// <remarks>2026-11-12 0.383에서 cpppj `UberGump.cpp` 1212~1215행의 앵커 버튼 조회를 반영하려고 추가했다.
    /// 원문은 앵커 노드의 `baseWidth*0.5+x`·`y`에 팝업을 붙인다(`RawMenuControls.cpp` 1047~1048행).</remarks>
    public ScreenRect? ControlBounds(string label)
    {
        // 원본 제어 영역 순서로 첫 일치 항목을 선택한다.
        for (int index = 0; index < Labels.Length; index++)
        {
            if (!string.Equals(Labels[index], label, StringComparison.Ordinal)) continue;
            return _tree.Collection.Bounds(_buttons[index]);
        }
        return null;
    }

    /// <summary>모달 대화창이 열리면 함께 그리지 않는 실제 메뉴 부품 토큰이다. 돌 버튼 여덟 개와 각 배경 자식이다.</summary>
    private readonly HashSet<uint> _modalHidden = [];

    /// <summary>모달 대화창이 열리면 도와 달리 메인 메뉴 돌 버튼과 그 배경은 그리지 않는다.</summary>
    /// <param name="token">확인할 실제 Gump 토큰.</param>
    /// <returns>여덟 버튼이나 그 배경 자식 중 하나이면 참이다.</returns>
    /// <remarks>2026-11-12 0.382에서 L4a menu-credits 비교로 추가했다. cpppj는 Tell이 열리면 Dialog 단계로 바뀌어
    /// 메인 메뉴 버튼을 그리지 않고 Tell 창만 그린다(<c>UberGump.cpp</c> 2029행의 MainMenu 단계 조건과 2065행의 Dialog 단계 분기).
    /// 배경 자식은 별도 수집 항목이라 함께 건너뛰어야 한다. 팝업 메뉴(Help·Options)는 모달이 아니라 버튼을 계속 그린다.
    /// <see cref="HostGumpDrawing"/>이 이 판별로 건너뛴다.</remarks>
    public bool HideWithModal(uint token) => _modalHidden.Contains(token);

    /// <summary>현재 화면에 배경과 글리프를 같은 실제 팔레트·8비트 표면으로 그린다.</summary>
    /// <param name="screen">시작 타이틀을 그린 논리 화면.</param>
    public void Draw(Screen screen)
    {
        byte[] pixels = screen.Lock();
        try
        {
            var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, screen.Width, screen.Height));
            var target = new GumpPixelTarget(pixels, screen.Pitch, screen.Height, clip);
            // 각 메뉴 버튼의 배경과 부모를 같은 클립·표면에 출력한다.
            foreach (uint token in _buttons) _tree.DrawStone(token, target, clip, screen.ShadeMaps(), screen.Colors());
        }
        finally { screen.Unlock(); }
        screen.Update(new(0, 0, screen.Width, screen.Height));
    }
}
