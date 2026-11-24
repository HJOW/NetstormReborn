using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Tests;

// RU-3d 버전 차이 Tell의 실제 메뉴 연결 계약이다. 0.990에서 추가됐다.
public sealed class MenuVersionTellTests
{
    /// <summary>창이나 음향을 열지 않고 원본 자산과 같은 메뉴 Tell 장치를 구성한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>동일 실제 팔레트와 8비트 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>수신 옵션과 HTML 생성기가 공유하는 실제 시작 설정이다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>같은 HTML과 controlled 메뉴가 사용하는 실제 사건 큐다.</summary>
        public readonly InputQueue Queue = new();
        /// <summary>같은 실제 메뉴와 공통 노드 트리다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>비교할 기존 HTML 닫기 소유자다.</summary>
        public readonly HostHtmlWindows Html;
        /// <summary>같은 실제 공유 조각과 스크롤 장치다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>실제 controlled 스택이다.</summary>
        public readonly HostHtmlMenus Menus;
        /// <summary>같은 실제 원문 문자열 고리다.</summary>
        public readonly ScriptStringRing Strings = new();
        /// <summary>같은 실제 Tell 생성기다.</summary>
        public readonly HostHtmlFactory Factory;
        /// <summary>같은 실제 메뉴 입력 분배기다.</summary>
        public readonly HostMenuInput MenuInput;

        /// <summary>기존 실제 메뉴와 문자와 controlled 메뉴를 조립한다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); Startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); Startup.Draw(Screen);
            Menu = new(Startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            Html = new(Menu, Queue, () => 1, _ => { }); Html.BindParser(Startup, _ => { }); Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html); Menus = new(Menu, Html, Text, Startup, Screen, (_, _) => { }); Menus.BindInput(Queue, _ => { }, (_, _) => { });
            Factory = new(Menu, Html, Menus, Startup, Screen, Queue, Strings, () => 1.0, _ => 0u, _ => { });
            MenuInput = new(Menu, Html, Factory, Queue, Strings, () => { }, _ => { });
        }

        /// <summary>최종 owner 반납 뒤 기존 실제 HTML 저장소도 정리한다.</summary>
        public void Dispose() { Html.CloseAll(); }
    }

    /// <summary>빈 창 닫기는 항상 성공한다.</summary>
    [Fact]
    public void TryShowTellBlankClosesAndReturnsTrue()
    {
        using var scene = new Scene();
        Assert.True(scene.MenuInput.TryShowTell("Blank"));
    }

    /// <summary>없는 Tell은 거짓만 돌려주고 예외로 꾸미지 않는다.</summary>
    [Fact]
    public void TryShowTellMissingReturnsFalseWithoutThrow()
    {
        using var scene = new Scene();
        Assert.False(scene.MenuInput.TryShowTell("NoSuchTellSectionXYZ123"));
    }

    /// <summary>버전 차이 Tell은 실제 tell 자료에서 찾아 HTML 창을 연다.</summary>
    [Fact]
    public void TryShowTellVersionDifferenceResolves()
    {
        using var scene = new Scene();
        Assert.True(scene.MenuInput.TryShowTell("MetaVerDiff(9.0)"));
    }

    /// <summary>있는 Tell은 실제 HTML 창을 연다.</summary>
    [Fact]
    public void TryShowTellExistingOpensWindow()
    {
        using var scene = new Scene();
        Assert.True(scene.MenuInput.TryShowTell("Credits"));
    }
}
