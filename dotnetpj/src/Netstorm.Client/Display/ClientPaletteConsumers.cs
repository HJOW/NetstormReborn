using Netstorm.Client.Audio;

// 파일 팔레트의 명명색이 바뀐 뒤 같은 실제 소비자를 원문 순서로 갱신한다.
namespace Netstorm.Client.Display;

/// <summary>CPP Client::RefreshPaletteConsumers의 작은 현재 소유자 경계다. 파일 로딩/9표 재생성은 호출자가 끝낸다.</summary>
public static class ClientPaletteConsumers
{
    /// <summary>audio4색→선택된 softwarecursor→현재menu 또는renderer 순서를 유지한다. 생성/부모없는 경우는 원문 선택 관문만 생략한다.</summary>
    public static void Refresh(Screen screen, Func<SceneMusicState?> currentAudio, Func<bool> cursorPresent,
        Action? rebuildSoftwareCursor, Func<Action?> currentMenuRefresh, Func<Renderer?> currentRenderer)
    {
        ArgumentNullException.ThrowIfNull(screen); ArgumentNullException.ThrowIfNull(currentAudio); ArgumentNullException.ThrowIfNull(cursorPresent);
        ArgumentNullException.ThrowIfNull(currentMenuRefresh); ArgumentNullException.ThrowIfNull(currentRenderer);
        SceneMusicState? scene = currentAudio();
        if (scene is not null) screen.Colors().Weather.CopyTo(scene.Tints, 0);
        if (cursorPresent() && (screen.Flags & ScreenMode.SoftwareMouse) != 0)
            (rebuildSoftwareCursor ?? throw new InvalidOperationException("현재 소프트웨어 커서의 실제 팔레트 재생성 장치 미연결"))();
        Action? menu = currentMenuRefresh();
        if (menu is not null) menu();
        else currentRenderer()?.InvalidateAll();
    }
}
