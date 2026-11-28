using Netstorm.Client.Audio;
using Netstorm.Client.Display;

// 실제 Scene상자/Screen명명색/Renderer dirty와 선택된 경계 순서를 함께 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>새 감독/OS/파일을 만들지 않고 실제 소비자 갱신의 순서·실패접두·미선택경계를 검사하는 검사한다.</summary>
public sealed class ClientPaletteConsumersTests
{
    /// <summary>명명색은 실제 파일 팔레트 로더가 구성하며 임시 flash팔레트는 쓰지 않는다.</summary>
    private static Screen Screen()
    {
        var screen = new Screen(32, 24); screen.LoadPalette(new GamePalette(new byte[1024])); return screen;
    }

    /// <summary>메뉴가 없을 때 실제Renderer의전체dirty를세우며Tint/현재결과/구름dirty와현재곡은변경하지않는다.</summary>
    [Fact]
    public void NoMenuRefreshesActualRendererAndOnlyCopiesFourWeatherColors()
    {
        Screen screen = Screen(); var scene = new SceneMusicState { Tint = 0xdeadbeef, ResultState = 9, PaletteDirty = 7 };
        scene.SongEnd.Value = 12.5;
        uint[] expected = [1, 7, 11, 19]; expected.CopyTo(screen.Colors().Weather, 0);
        var renderer = new Renderer(32, 24); renderer.ClearDirty(); Assert.False(renderer.FullRedrawPending);
        ClientPaletteConsumers.Refresh(screen, () => scene, () => false, null, () => null, () => renderer);
        Assert.Equal(expected, scene.Tints); Assert.True(renderer.FullRedrawPending);
        Assert.Equal(0xdeadbeefu, scene.Tint); Assert.Equal(9u, scene.ResultState); Assert.Equal(7u, scene.PaletteDirty); Assert.Equal(12.5, scene.SongEnd.Value);
    }

    /// <summary>메뉴가있는분기는Renderer대체경로를조회하지않고실제같은메뉴의후단이dirty를책임진다.</summary>
    [Fact]
    public void MenuBranchRunsAfterAudioAndSkipsFallbackRendererGetter()
    {
        Screen screen = Screen(); var scene = new SceneMusicState(); var events = new List<string>();
        screen.Colors().Weather[0] = 97;
        ClientPaletteConsumers.Refresh(screen, () => { events.Add("audio"); return scene; }, () => { events.Add("cursor"); return true; }, null,
            () => { events.Add("menu-get"); return () => { events.Add("menu"); Assert.Equal(97u, scene.Tints[0]); }; },
            () => throw new IOException("미선택fallback"));
        Assert.Equal(new[] { "audio", "cursor", "menu-get", "menu" }, events);
    }

    /// <summary>현재화면에software분기가선택되면누락을false성공으로숨기지않고이미갱신된audio접두를보존한다.</summary>
    [Fact]
    public void SelectedSoftwareCursorFailurePreservesAudioPrefixAndSkipsMenu()
    {
        Screen screen = Screen(); screen.SetMode(ScreenMode.FullScreen | ScreenMode.SoftwareMouse | ScreenMode.BackBufferDib);
        var scene = new SceneMusicState(); screen.Colors().Weather[1] = 73;
        Assert.Throws<InvalidOperationException>(() => ClientPaletteConsumers.Refresh(screen, () => scene, () => true, null,
            () => throw new IOException("미선택menu"), () => throw new IOException("미선택renderer")));
        Assert.Equal(73u, scene.Tints[1]);
    }

    /// <summary>Software분기뒤현재메뉴를재조회하고그예외를잡아Renderer로돌리지않는다.</summary>
    [Fact]
    public void SoftwareReentrySelectsCurrentMenuAndPreservesItsFailure()
    {
        Screen screen = Screen(); screen.SetMode(ScreenMode.FullScreen | ScreenMode.SoftwareMouse | ScreenMode.BackBufferDib);
        var events = new List<string>(); var failure = new IOException("현재menu"); Action? menu = null;
        Assert.Same(failure, Assert.Throws<IOException>(() => ClientPaletteConsumers.Refresh(screen, () => null, () => true,
            () => { events.Add("software"); menu = () => { events.Add("menu"); throw failure; }; }, () => menu,
            () => throw new IOException("미선택renderer"))));
        Assert.Equal(new[] { "software", "menu" }, events);
    }
}
