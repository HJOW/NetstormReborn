using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 실제 최초 자산과 기존 UI 장치를 함께 사용하여 스타일 그림 결선을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>새 그림 어댑터의 실제 자산 선택/본문/controlled 메뉴/출력을 검증한다.</summary>
public sealed class HostTextPictureTests
{
    /// <summary>창/소리/저장 없이 같은 실제 팔레트/트리/자산/조각을 구성한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 설치/설정 소유자다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>같은 실제 논리 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>같은 실제 트리/글꼴/기본 버튼이다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>실제 부분 자식/창의 닫기 소유자다.</summary>
        public readonly HostHtmlWindows Windows;
        /// <summary>같은 실제 파서/조각/버퍼/스크롤이다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>실제 controlled 메뉴다.</summary>
        public readonly HostHtmlMenus Menus;
        /// <summary>같은 실제 공통 수집/그리기다.</summary>
        public readonly HostGumpDrawing Drawing;
        /// <summary>한 번 적재한 최초 TYPE/정의/SHP다.</summary>
        public readonly HostAssetTables Assets;
        /// <summary>실제 자산 어댑터다.</summary>
        public readonly MarkupGameAssets Pictures;
        /// <summary>헤더/그리기가 같은 SHP 물리를 사용한다.</summary>
        public readonly Netstorm.Client.Display.ShapeDatabase Shapes;
        /// <summary>다음 실제 출력이 조회할 현재 색표다.</summary>
        public byte[] Colors = Enumerable.Range(0, 256).Select(index => (byte)index).ToArray();
        /// <summary>현재 색표 조회 시점을 관측한다.</summary>
        public int ColorReads;
        /// <summary>실제 자산 장치를 새 UI의 같은 수명에 연결한다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); Startup = new(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); Startup.Draw(Screen);
            Menu = new(Startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 }); Windows = new(Menu, new InputQueue(), () => 1, _ => { }); Text = new(Menu, Windows, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Windows); Menus = new(Menu, Windows, Text, Startup, Screen, (_, _) => { }); Drawing = new(Menu, Text, Menus, Screen);
            Assets = new(Startup, new(OriginalEdition.Patch1078)); Shapes = new(Assets.ShapeImage); Pictures = new(Assets.TypeTable, Assets.Definitions, Shapes); Text.BindPictures(Pictures, () => { ColorReads++; return Colors; }, Startup.Configuration);
        }
        /// <summary>실제 제한 Chat 아이콘 조건을 만족하는 일반 불투명 프레임을 찾는다.</summary>
        public (int Type, int Frame) Icon()
        {
            // 로딩 순서/논리 프레임 순서의 첫 실제 작은 그림을 사용한다.
            for (int index = 0; index < Assets.Definitions.Count; index++)
                for (int frame = 0; frame < Assets.Definitions[index].Clusters.Count; frame++)
                {
                    var metric = Shapes.SquidMetrics(index, frame); if (metric.Height is < 8 or > 16 || metric.Width <= 0 || Shapes.Blocks()[index].Frames[frame].IsSpecial()) continue;
                    if (Shapes.Decode(index, frame).Opacity.Any(value => value != 0)) return (70 + index, frame);
                }
            throw new InvalidOperationException("실제 작은 아이콘 없음");
        }
        /// <summary>현재 출력의 논리 픽셀 사본만 읽는다.</summary>
        public byte[] Pixels() { byte[] bytes = Screen.Lock(); try { return bytes.ToArray(); } finally { Screen.Unlock(); } }
        /// <summary>다음 실제 출력 전 동일7번 색으로 표면을 채운다.</summary>
        public void Clear() { byte[] bytes = Screen.Lock(); try { bytes.AsSpan().Fill(7); } finally { Screen.Unlock(); } }
        /// <summary>실제 자식 우선 공통 수명으로 닫는다.</summary>
        public void Dispose() => Windows.CloseAll();
    }
    /// <summary>별도 Assets 해석기의 실제116개 프레임 수/헤더와 대조하고 무자산/경계/값 반환을 확인한다.</summary>
    [Fact]
    public void LoadedAssetViewsAgreeWithIndependentAssetReaders()
    {
        using var scene = new Scene(); var catalog = new TypeCatalog(scene.Startup.Files); var shapes = new Netstorm.Assets.ShapeDatabase(scene.Assets.ShapeImage); Assert.Equal(188, scene.Pictures.TypeCount); Assert.Equal(116, catalog.Types.Count);
        // 독립 자산 파서와 모든 논리 프레임의 signed 실제 높이를 대조한다.
        foreach (var type in catalog.Types)
        {
            int number = type.RuntimeIndex; var view = scene.Pictures.Type(number); Assert.Equal(type.Definition.Frames.Codes.Count, view.FrameCount); Assert.Equal(number, scene.Pictures.FindType(type.RuntimeName)); Assert.NotNull(scene.Pictures.Pictures.Type((uint)number));
            // 각 논리 프레임은 같은 물리 블록의 별도 헤더 해석과 일치해야 한다.
            for (int frame = 0; frame < view.FrameCount; frame++) Assert.Equal(shapes.SquidMetrics(type.LoadIndex, frame).DisplayHeight, scene.Pictures.FrameHeight(number, frame));
            view.First[0] = int.MinValue; Assert.NotEqual(int.MinValue, scene.Pictures.Type(number).First[0]);
        }
        Assert.Null(scene.Pictures.Pictures.Type(69)); Assert.Null(scene.Pictures.Pictures.Type(186)); Assert.Equal(0, scene.Pictures.Type(186).FrameCount); Assert.Null(scene.Pictures.FrameHeight(69, 0)); Assert.Null(scene.Pictures.FrameHeight(188, 0)); Assert.Null(scene.Pictures.FrameHeight(70, -1)); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Pictures.Type(-1)); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Pictures.Type(188)); Assert.Equal(-1, scene.Pictures.FindType("NoSuchAsset"));
    }
    /// <summary>제한 그림을 실제 본문에 삽입/측정/출력하고 색표 실패 후에도 현재 문맥을 다시 쓸 수 있다.</summary>
    [Fact]
    public void ActualInlinePictureDrawsAndRecoversFromColorFailure()
    {
        using var scene = new Scene(); var (type, frame) = scene.Icon(); var tree = scene.Menu.Tree; uint token = tree.AllocateNode(new ScrollingTextGumpNode()); scene.Windows.TrackChild(token); scene.Text.Widgets.Construct(token, 400, 100, 5, scene.Screen.Colors().Basic[7]); var node = Assert.IsType<ScrollingTextGumpNode>(tree.Node(token)); node.Callback = 1; tree.TreeHooks.Attach!(token, 100, 100, tree.Create.Entries[0], 0);
        scene.Text.Parser.Parse(token, Encoding.ASCII.GetBytes($"~[I{type}.{frame}]\n\0")); Assert.Contains(scene.Text.Fragments.Fragments.Values, fragment => fragment.Kind == StyleTextFragmentKind.External); scene.Clear(); scene.Text.Draw(token); Assert.Contains(scene.Pixels(), value => value != 7); Assert.Equal(1, scene.ColorReads); scene.Colors = new byte[3]; Assert.Throws<InvalidOperationException>(() => scene.Text.Draw(token)); scene.Colors = Enumerable.Range(0, 256).Select(index => (byte)index).ToArray(); scene.Clear(); scene.Text.Draw(token); Assert.Contains(scene.Pixels(), value => value != 7); Assert.Equal(3, scene.ColorReads); scene.Windows.CloseAll(); Assert.Null(tree.Node(token)); Assert.Empty(scene.Text.Fragments.Fragments);
    }
    /// <summary>같은 실제 controlled 메뉴에서도 아이콘이 실자산으로 측정/그려지고 payload와 수명은 유지된다.</summary>
    [Fact]
    public void ControlledMenuUsesSameActualInlinePictureAssets()
    {
        using var scene = new Scene(); var (type, frame) = scene.Icon(); scene.Menus.Widgets.BeginVertical(0); scene.Menus.Controls.AddText(Encoding.ASCII.GetBytes($"~[I{type}.{frame}] icon\0"), 0x02000115, 0x71000000); uint token = scene.Menus.Menu.Current(); Assert.NotEqual(0u, token); var node = Assert.IsType<MenuGumpNode>(scene.Menu.Tree.Node(token)); Assert.True(node.BaseWidth > 0); Assert.Contains(scene.Text.Fragments.Fragments.Values, fragment => fragment.Kind == StyleTextFragmentKind.External); scene.Menus.Menu.AttachCurrent(300, 300, scene.Menu.Tree.Create.Entries[0]); scene.Drawing.Draw(); Assert.Equal(1, scene.ColorReads); scene.Windows.CloseAll(); Assert.Null(scene.Menu.Tree.Node(token)); Assert.Empty(scene.Text.Fragments.Fragments); Assert.Throws<InvalidOperationException>(() => scene.Text.BindPictures(scene.Pictures, () => scene.Colors, scene.Startup.Configuration));
    }
}
