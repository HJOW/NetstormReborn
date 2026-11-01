using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Host;

/// <summary>실제 자산과 공통 Gump 트리로 메인 메뉴의 여덟 돌 버튼을 표시한다.
/// 근거: cpppj/src/client/UberGump.cpp의 메인 메뉴 배치와 RawButtonControls.cpp.</summary>
public sealed class HostMainMenu
{
    /// <summary>원본 메인 메뉴의 두 행 순서.</summary>
    private static readonly string[] Labels = ["Campaign", "Multiplayer", "Demo", "Help", "Edit", "Credits", "Options", "Quit"];
    /// <summary>실제 화면과 공통 입력·노드 트리.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>메뉴 표시 순서에 대응하는 실제 버튼 토큰.</summary>
    private readonly uint[] _buttons = new uint[8];

    /// <summary>파일 팔레트를 적용한 논리 화면과 실제 자산으로 버튼 트리를 생성한다.</summary>
    /// <param name="startup">같은 파일 시스템과 Config를 보유한 시작 화면.</param>
    /// <param name="screen">이미 팔레트를 적용한 논리 화면.</param>
    /// <param name="hooks">실제 입력·시계·음향·부모 사건 장치.</param>
    public HostMainMenu(HostStartupScene startup, Screen screen, GumpButtonTreeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(screen);
        if (screen.PaletteLoaded == 0) throw new InvalidOperationException("메인 메뉴에는 실제 파일 팔레트가 필요하다");
        var types = new TypeCatalog(startup.Files);
        TypeInfo type = types.Find("fortGump") ?? throw new InvalidDataException("fortGump 타입이 없다");
        int frame = type.Definition.Frames.Find('A', TypeFrameTable.DefaultVariant, 1);
        if (frame < 0) throw new InvalidDataException("fortGump A01 프레임이 없다");
        var shapes = new Netstorm.Client.Display.ShapeDatabase(startup.Files.ReadAllBytes("d/_shapes.shp"));
        uint typeNumber = (uint)type.RuntimeIndex;
        var assets = new GumpBackgroundAssets(shapes, [new(typeNumber, type.LoadIndex, type.Definition.Frames.Codes.Count, type.Flags1)]);
        string face = startup.Configuration.PathSpec("fontFaceName");
        var fonts = new FontStore(startup.Files.TryReadAllBytes, face,
            (_, _, _, _) => throw new InvalidOperationException("원본 비트맵 글꼴 캐시가 없어 GDI 생성 경계가 필요하다"));
        _tree = new GumpButtonTree(screen.Width, screen.Height, assets, typeNumber, (uint)frame, GumpVisual.ButtonColors(screen.Colors()), fonts, hooks);
        int left = screen.Width / 2 - 156, top = screen.Height / 2 - 73;
        // 원본 4열 2행의 순서와 폭·행 간격으로 실제 돌 버튼을 만든다.
        for (int index = 0; index < Labels.Length; index++)
        {
            byte[] label = Encoding.Latin1.GetBytes(Labels[index] + "\0");
            _buttons[index] = _tree.AddStone(75, 19, label, left + (index % 4) * 79, top + (index / 4) * 23);
        }
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
