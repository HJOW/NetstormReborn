using Netstorm.Client.Display;

namespace Netstorm.Client.Host; // 실제 첫 화면과 장기 소유자 색표를 같은 Client 수명에 둔다.

/// <summary>현재 게임/장면이 빌리는 단일 색표 조립 경계다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은 첫 RGB·primary·캐시·옵션 구독이며 화면/세션마다 다시 만들지 않는다.</summary>
    private readonly HostStartupColorMaps? _startupColors;
    /// <summary>실제 최초 설치된 색표만 공개하며 자산 없는 호스트에는 임의 표를 만들지 않는다.</summary>
    public ClientColorSortInput ColorSortInput => (_startupColors ?? throw new InvalidOperationException("실제 시작 색표 장치 미연결")).Input;

    /// <summary>첫 UI 전에 만들어 이후 같은 실제 생산 장치가 빌리는 문맥이다.</summary>
    private ProductionControlContext? _nativeProductionContext;
    /// <summary>실제 장기 Player/옵션/물리/SP/색표만 공개하며 미준비 문맥을 대신 만들지 않는다.</summary>
    public ProductionControlContext NativeProductionContext => _nativeProductionContext ?? throw new InvalidOperationException("실제 생산 문맥 미설치");
    /// <summary>현재 Root/생산 공장이 같은 Source 동기화 부모를 호출하는 실제 진입점이다.</summary>
    public void SynchronizeNativeProductionContents() => ClientProductionSync.SynchronizeProductionControlContents(NativeProductionContext);
    /// <summary>준비된 같은 최초 색표와 실제 Client owner를 한 번 조립하며 Session loaded를 열지 않는다.</summary>
    private void InitializeNativeProductionContext()
    {
        if (_nativeProductionContext is not null) throw new InvalidOperationException("실제 생산 문맥 중복 설치");
        if (!ColorSortInput.OwnerTables.Ready) throw new InvalidOperationException("실제 생산 문맥의 최초 색표 미준비");
        _nativeProductionContext = new() { Storage = _surfaceGlobals.Storage, Gameplay = _gameplay, Players = _gameplay.PlayerRecords,
            Options = _gameplay.BattleOptions, Sp = _spGlobals, OwnerColorTables = ColorSortInput.OwnerTables };
    }

    /// <summary>최초 고정 입력 뒤 음향/커서와 실제 TYPE를 준비한다. 이후 같은 장기 장치만 빌린다.</summary>
    private HostAssetTables PrepareStartupColorAssets()
    {
        HostStartupScene startup = _startup ?? throw new InvalidOperationException("실제 색표의 시작 자산 미연결");
        // 기존 음향의 잘못된 Scene은 HWND/로그/색표 자산을 선택하기 전에 거부한다.
        if (_missionAudio is { } currentAudio) _stateResources.RequireMusicOwner(currentAudio.Scene);
        if (_missionAudio is null)
        {
            // 최종 DesktopGL 창이 생성된 UI 스레드에서 같은 SDL 창의 실제 Windows HWND를 빌린다.
            nint soundWindow = OperatingSystem.IsWindows() ? WindowsSdlWindow.NativeHandle(Window.Handle) : Window.Handle;
            if (OperatingSystem.IsWindows())
                ErrorLog.Note($"미션 음향 창 준비: SDL=0x{Window.Handle.ToInt64():X}, HWND=0x{soundWindow.ToInt64():X}");
            var audio = new HostMissionAudio(startup, soundWindow, _options.MuteAudio,
                () => _clientClock.Clock.WallSeconds(_clock.Milliseconds), _stateResources.Music);
            // 새 장치는 같은 Scene 정체성을 확인한 뒤에만 Game의 장기 별칭으로 공개한다.
            try { _stateResources.RequireMusicOwner(audio.Scene); }
            catch (Exception failure)
            {
                try { audio.Dispose(); }
                catch (Exception cleanup)
                {
                    try { failure.Data["SceneOwnerAudioDisposeException"] = cleanup; }
                    catch { /* 부수 진단 실패가 최초 정체성 오류를 덮지 않게 한다. */ }
                }
                throw;
            }
            _missionAudio = audio;
            BindNativeSceneMusic();
        }
        _buttonSound ??= new HostButtonSound(startup); _cursor ??= new HostOriginalCursor(startup);
        PrepareNativeAssetTables(); return AssetTables;
    }

    /// <summary>첫 표시와 재합성이 같은 준비/fort 갱신 경계를 사용한다. 누락 장치를 무시하지 않는다.</summary>
    [System.Diagnostics.CodeAnalysis.MemberNotNull(nameof(_buttonSound), nameof(_cursor))]
    private void DrawStartupScreen()
    {
        HostStartupScene startup = _startup ?? throw new InvalidOperationException("실제 시작 장면 미연결");
        HostStartupColorMaps colors = _startupColors ?? throw new InvalidOperationException("실제 시작 색표 장치 미연결");
        startup.Draw(_driver.Screen, colors.PrepareScreen, screen =>
        {
            colors.Rebuild(screen);
            RefreshPaletteConsumers();
        }, LoadNativeStartupPalette);
        if (_buttonSound is null || _cursor is null) throw new InvalidOperationException("실제 시작 색표의 음향/커서 준비 미완료");
    }

    /// <summary>완성 경로와 원래 clear를 같은 현재 Screen/FILE 장치에 전달한다. 진단 복귀도 중앙 정책을 따른다.</summary>
    private void LoadNativeStartupPalette(Screen screen, string path, uint clear)
    {
        HostStartupScene startup = _startup ?? throw new InvalidOperationException("실제 팔레트의 시작 파일 장치 미연결");
        _paletteGlobals.RequireScreen(screen);
        screen.LoadNativePaletteFile(startup.Files, path, clear, () => new WindowsBaseFileDevice(),
            line => ErrorLog.RecordAssertion("팔레트 파일 길이 진단", @".\Screen.cpp", line));
    }
}
