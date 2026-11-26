// 미션 중 메인 메뉴 배경 숨김을 검증한다.
// 2026-11-26 1.078에서 R5-4c1용으로 추가했다.
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;

namespace Netstorm.Client.Tests;

/// <summary>미션 활성화 때 메인 메뉴 돌 버튼·배경을 그리지 않고 입력도 소비하지 않는다.</summary>
public sealed class HostMissionMenuHideTests
{
    /// <summary>창·음향 없이 원본 자산과 같은 메뉴·그리기 장치를 구성한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>동일 실제 팔레트와 8비트 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>수신 옵션과 HTML 생성기가 공유하는 실제 시작 설정이다.</summary>
        public readonly HostStartupScene Startup;
        /// <summary>같은 실제 메뉴와 공통 노드 트리다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>비교할 기존 HTML 닫기 소유자다.</summary>
        public readonly HostHtmlWindows Html;
        /// <summary>같은 실제 공유 조각과 스크롤 장치다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>같은 실제 Console과 편집 버퍼다.</summary>
        public readonly HostTextEntry Entry;
        /// <summary>실제 controlled 스택이다.</summary>
        public readonly HostHtmlMenus Menus;

        /// <summary>기존 장치를 조립하고 시작 타이틀을 그린다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); Startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); Startup.Draw(Screen);
            Menu = new(Startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            Html = new(Menu, new InputQueue(), () => 1, _ => { }); Html.BindParser(Startup, _ => { }); Text = new(Menu, Html, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Html); Entry = new(Menu, Html, Text, Screen, () => 1, (_, _) => { }); Menus = new(Menu, Html, Text, Startup, Screen, (_, _) => { }); Menus.BindInput(new InputQueue(), _ => { }, (_, _) => { });
        }

        /// <summary>기존 실제 HTML과 편집 저장소를 정리한다.</summary>
        public void Dispose() { Html.CloseAll(); Entry.Dispose(); }
    }

    /// <summary>미션 중 그리기는 메뉴 버튼 중심을 시작 배경 그대로 둔다.</summary>
    [Fact]
    public void MissionDrawingKeepsMenuButtonBackground()
    {
        using var scene = new Scene();
        ScreenPoint? center = scene.Menu.ControlPoint("Campaign"); Assert.NotNull(center);
        byte[] background = scene.Screen.Pixels.ToArray();
        int index = center.Value.Y * scene.Screen.Pitch + center.Value.X;
        Assert.True((uint)index < (uint)background.Length);
        var menuDrawing = new HostGumpDrawing(scene.Menu, scene.Text, scene.Menus, scene.Screen, scene.Entry) { IsMissionActive = () => false };
        menuDrawing.Draw();
        byte[] withMenu = scene.Screen.Pixels.ToArray();
        Assert.NotEqual(background[index], withMenu[index]);
        scene.Startup.Draw(scene.Screen);
        var missionDrawing = new HostGumpDrawing(scene.Menu, scene.Text, scene.Menus, scene.Screen, scene.Entry) { IsMissionActive = () => true };
        missionDrawing.Draw();
        byte[] withoutMenu = scene.Screen.Pixels.ToArray();
        Assert.Equal(background[index], withoutMenu[index]);
    }

    /// <summary>미션 중 입력은 메뉴 버튼을 소비하지 않고 메뉴 상태에서는 소비한다.</summary>
    [Fact]
    public void MissionRouteIgnoresMenuButton()
    {
        using var scene = new Scene();
        ScreenPoint? center = scene.Menu.ControlPoint("Campaign"); Assert.NotNull(center);
        var press = new InputEvent(InputCode.LeftButton, center.Value.X, center.Value.Y);
        scene.Menu.IsMissionActive = () => false;
        Assert.NotEqual(0u, scene.Menu.Route(press));
        scene.Menu.IsMissionActive = () => true;
        Assert.Equal(0u, scene.Menu.Route(press));
    }
}
