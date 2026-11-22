using Netstorm.Client.Display;

namespace Netstorm.Client.Host; // 실제 Client 시작 순서와 장기 색표 소유자를 연결한다.

/// <summary>첫 팔레트와 TYPE 사이의 고정 입력 및 실제 캐시/옵션 수명을 조립한다.</summary>
public sealed class HostStartupColorMaps : IDisposable
{
    /// <summary>같은 설정·파일·월드 dirty를 빌리는 실제 시작 자료다.</summary>
    private readonly HostStartupScene _startup;
    /// <summary>동일 Clock/Player를 최초 한 번 초기화하는 실제 Client 경계다.</summary>
    private readonly Action _initializePrefix;
    /// <summary>동일 장기 TYPE를 준비하고 그 실제 소유자를 돌려준다.</summary>
    private readonly Func<HostAssetTables> _prepareAssets;
    /// <summary>캐시가 설치 디렉터리에 생성되지 않게 하는 사용자 경로다.</summary>
    private readonly UserDataRoot _userData;
    /// <summary>해제 때 다른 설정 소비자를 보존하는 자신의 구독이다.</summary>
    private readonly Action _optionsChanged;
    /// <summary>정리 후 다시 설치하거나 재사용하지 않는 수명 표식이다.</summary>
    private bool _disposed;
    /// <summary>화면·세션 교체에서 재생성하지 않는 실제 입력/9표/소유자 표다.</summary>
    public ClientColorSortInput Input { get; } = new();

    /// <summary>설정 구독만 설치한다. 준비 전 옵션 변경은 Source처럼 OwnerTables에서 무시한다.</summary>
    public HostStartupColorMaps(HostStartupScene startup, Action initializePrefix, Func<HostAssetTables> prepareAssets, UserDataRoot? userData = null)
    {
        _startup = startup ?? throw new ArgumentNullException(nameof(startup));
        _initializePrefix = initializePrefix ?? throw new ArgumentNullException(nameof(initializePrefix));
        _prepareAssets = prepareAssets ?? throw new ArgumentNullException(nameof(prepareAssets));
        _userData = userData ?? UserDataRoot.Resolve();
        _optionsChanged = () => Input.OwnerTables.SetEnabled(unchecked((uint)_startup.Configuration.GetInt("useBanners")), _startup.Cloud.WorldRedrawBox);
        _startup.Configuration.OnChanged += _optionsChanged;
    }

    /// <summary>첫 팔레트→실제 Player→primary→TYPE→캐시/소유자→Screen을 연결한다. 새 화면에는 같은 소유자만 다시 빌린다.</summary>
    public void PrepareScreen(Screen screen)
    {
        ObjectDisposedException.ThrowIf(_disposed, this); ArgumentNullException.ThrowIfNull(screen);
        if (!Input.Captured) { _initializePrefix(); Input.CaptureWithPrimary(screen.LogicalPalette); }
        screen.BindColorSortInput(() => Input.Logical);
        if (!Input.MapsReady)
        {
            HostAssetTables assets = _prepareAssets();
            string cache = _userData.Cache(); UserDataRoot.EnsureDirectory(cache);
            Input.InitializeCache(_startup.Files, Path.Combine(cache, "!color.dat"), screen.Colors().Basic[1],
                () => Input.OwnerTables.Initialize(Input.Primary, assets.LoadedTypes.Count, unchecked((uint)assets.TypeTable.Find("bridge")), _startup.Cloud.WorldRedrawBox),
                () => new WindowsBaseFileDevice());
        }
        screen.BindColorMaps(() => new GumpShadeMaps(Input.Maps[4], Input.Maps[6]));
    }

    /// <summary>fort 전환은 최초 RGB/primary와 같은9배열을 유지하고 현재 red만 사용한다.</summary>
    public void Rebuild(Screen screen)
    {
        ObjectDisposedException.ThrowIf(_disposed, this); ArgumentNullException.ThrowIfNull(screen);
        if (!Input.MapsReady) throw new InvalidOperationException("실제 색표 캐시 준비 전 fort 전환");
        Input.Rebuild(screen.Colors().Basic[1]); screen.ShadeMaps();
    }

    /// <summary>같은 Config에서 자신의 통지만 해제한다. 색표 배열은 같은 소유자에 남긴다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        _startup.Configuration.OnChanged -= _optionsChanged; _disposed = true;
    }
}
