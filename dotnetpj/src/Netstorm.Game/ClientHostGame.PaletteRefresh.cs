using Netstorm.Client.Display;
using Netstorm.Client.Gump;

// 실제 장기 메뉴/월드/색표의 소비자 갱신을 파일 로더 및 fort 전환의 공통 후단에 둔다.
namespace Netstorm.Client.Host;

/// <summary>현재 Scene 팔레트 소비자를 연결한다. 감독 Start/Frame을 만들거나 실행하지 않는다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>Scene의 확장자 포함 이름을 DataDir에 붙여 실제 FILE로 읽은 뒤 현재 소비자만 갱신한다. 고정9표는 재생성하지 않는다.</summary>
    private void LoadScenePalette(string name)
    {
        ArgumentNullException.ThrowIfNull(name);
        HostStartupScene startup = _startup ?? throw new InvalidOperationException("실제 날씨 팔레트의 시작 장치 미연결");
        LoadNativeStartupPalette(_driver.Screen, startup.Configuration.PathSpec("DataDir") + "\\" + name, 0);
        RefreshPaletteConsumers();
    }

    /// <summary>CPP와 같은 audio/현재커서/현재menu 분기다. 단색SDL커서에는 팔레트가 필요하지 않으며 선택된 software만 미복원 경계다.</summary>
    private void RefreshPaletteConsumers() => ClientPaletteConsumers.Refresh(_driver.Screen, () => _missionAudio?.Scene,
        () => _cursor is not null, null, () => _mainMenu is null ? null : RefreshCurrentMenuPalette, () => _nativeRenderer);

    /// <summary>이미 있는 동일 실제 UI 객체의 색/현재 월드 모드만 갱신한다. 없는 메뉴/HTML을 새로 만들지 않는다.</summary>
    private void RefreshCurrentMenuPalette()
    {
        HostMainMenu menu = _mainMenu ?? throw new InvalidOperationException("현재 팔레트 메뉴 부모 없음");
        Screen screen = _driver.Screen;
        // 같은 World의 모드와 현재 명명색을 갱신한다. 미션이 없으면 기존 함수의 자연 조기반환이다.
        RefreshWorldTileModes();
        menu.Tree.SetPalette(GumpVisual.ButtonColors(screen.Colors()));
        _htmlText?.SetPalette(screen.Colors());
        _htmlMenus?.SetPalette(screen.Colors());
        _textEntry?.SetPalette(screen.Colors());
        _missionProduction?.SetPalette(screen.LogicalPalette, ColorSortInput.Logical.Span, ColorSortInput.Maps);
        _nativeMinimap?.SetPalette(screen.Colors());
        _menuDirty = true;
        (_nativeRenderer ?? throw new InvalidOperationException("현재 메뉴의 실제 Renderer 없음")).InvalidateAll();
    }
}
