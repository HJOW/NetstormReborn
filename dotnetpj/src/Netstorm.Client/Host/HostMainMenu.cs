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
        new("Tell", "DEMOVILLE"), new("Menu", "help"), new("AskEditFort", "0"), new("Tell", "Credits"),
        new("Menu", "options"), new("RequestQuitApp", "0")];
    /// <summary>선택된 명령을 현재 상위 메뉴에 전달하는 실제 소비자.</summary>
    private readonly Action<MainMenuAction>? _activate;
    /// <summary>실제 화면과 공통 입력·노드 트리.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>버튼·장식·스크롤이 공유하는 실제 fortGump 타입 번호.</summary>
    public uint BackgroundType { get; }
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
        for (int index = 0; index < Labels.Length; index++)
        {
            byte[] label = Encoding.Latin1.GetBytes(Labels[index] + "\0");
            _buttons[index] = _tree.AddStone(75, 19, label, left + (index % 4) * 79, top + (index / 4) * 23);
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
