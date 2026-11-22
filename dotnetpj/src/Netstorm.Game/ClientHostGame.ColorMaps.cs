using Netstorm.Client.Display;

namespace Netstorm.Client.Host; // 실제 첫 화면과 장기 소유자 색표를 같은 Client 수명에 둔다.

/// <summary>현재 게임/장면이 빌리는 단일 색표 조립 경계다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은 첫 RGB·primary·캐시·옵션 구독이며 화면/세션마다 다시 만들지 않는다.</summary>
    private readonly HostStartupColorMaps? _startupColors;
    /// <summary>실제 최초 설치된 색표만 공개하며 자산 없는 호스트에는 임의 표를 만들지 않는다.</summary>
    public ClientColorSortInput ColorSortInput => (_startupColors ?? throw new InvalidOperationException("실제 시작 색표 장치 미연결")).Input;

    /// <summary>최초 고정 입력 뒤 음향/커서와 실제 TYPE를 준비한다. 이후 같은 장기 장치만 빌린다.</summary>
    private HostAssetTables PrepareStartupColorAssets()
    {
        HostStartupScene startup = _startup ?? throw new InvalidOperationException("실제 색표의 시작 자산 미연결");
        _missionAudio ??= new HostMissionAudio(startup, Window.Handle, _options.MuteAudio,
            () => _clientClock.Clock.WallSeconds(_clock.Milliseconds), _stateResources.Music);
        _buttonSound ??= new HostButtonSound(startup); _cursor ??= new HostOriginalCursor(startup);
        PrepareNativeAssetTables(); return AssetTables;
    }

    /// <summary>첫 표시와 재합성이 같은 준비/fort 갱신 경계를 사용한다. 누락 장치를 무시하지 않는다.</summary>
    [System.Diagnostics.CodeAnalysis.MemberNotNull(nameof(_buttonSound), nameof(_cursor))]
    private void DrawStartupScreen()
    {
        HostStartupScene startup = _startup ?? throw new InvalidOperationException("실제 시작 장면 미연결");
        HostStartupColorMaps colors = _startupColors ?? throw new InvalidOperationException("실제 시작 색표 장치 미연결");
        startup.Draw(_driver.Screen, colors.PrepareScreen, colors.Rebuild);
        if (_buttonSound is null || _cursor is null) throw new InvalidOperationException("실제 시작 색표의 음향/커서 준비 미완료");
    }
}
