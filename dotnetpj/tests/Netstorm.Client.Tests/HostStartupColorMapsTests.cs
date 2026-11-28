using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 실제 시작 자료·현재 Player·TYPE·CRT 캐시를 같은 경계로 검사한다.

/// <summary>그래픽 장치 없이 실제 Game과 같은 색표 조립 순서/주소/옵션 수명을 검증한다.</summary>
public sealed class HostStartupColorMapsTests
{
    /// <summary>실제 게임 자산을 읽기 전용으로 여는 시작 자료다.</summary>
    private static HostStartupScene Startup() => new(GameDataLocator.FindDataDirectory() ?? throw new InvalidOperationException("실제 시작 자산 부재"));
    /// <summary>현재 모드가 확정된 실제 논리 화면이다.</summary>
    private static Screen Screen(UIntBox? loaded = null)
    {
        var screen = loaded is null ? new Screen(1024, 768) : new Screen(1024, 768, loaded); Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); return screen;
    }
    /// <summary>다른 검사/사용자 저장물과 섞이지 않는 검사 소유 폴더다.</summary>
    private static UserDataRoot Root() => new(Path.Combine(Path.GetTempPath(), "Netstorm-StartupColors-" + Guid.NewGuid().ToString("N")));

    /// <summary>첫 팔레트 뒤 실제 Clock/Player·TYPE가 캐시보다 먼저 준비되고 화면 교체에서 주소를 보존한다.</summary>
    [Fact]
    public void ActualStartupCapturesBeforeTypesAndFortWithoutRecapture()
    {
        if (!OperatingSystem.IsWindows()) return;
        var startup = Startup(); var screen = Screen(); var game = new ClientGameplayGlobals();
        var clock = new ClientClockGlobals(new ClientLocalStateGlobals(game, new()));
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, game.Authority, game.GameAux); var root = Root();
        int prefixes = 0, types = 0, rebuilds = 0; HostStartupColorMaps? owner = null; HostAssetTables? assets = null; uint[]? first = null;
        using (owner = new HostStartupColorMaps(startup, () =>
        {
            Assert.False(owner!.Input.Captured); Assert.False(game.PlayerRecords.Initialized); Assert.Equal(0u, game.PlayerRecords.Base);
            first = screen.LogicalPalette.ToArray(); clock.InitializeFullTime(1234); ClientPlayerRuntime.Initialize(game); prefixes++;
        }, () =>
        {
            Assert.True(clock.Initialized); Assert.True(game.PlayerRecords.Initialized); Assert.True(owner!.Input.Captured);
            Assert.False(owner.Input.MapsReady); Assert.False(owner.Input.OwnerTables.Ready); Assert.False(Directory.Exists(root.Cache())); types++;
            return assets = new HostAssetTables(startup, storage);
        }, root))
        {
            startup.Draw(screen, owner.PrepareScreen, current => { Assert.True(owner.Input.OwnerTables.Ready); rebuilds++; owner.Rebuild(current); });
            Assert.Equal(1, prefixes); Assert.Equal(1, types); Assert.Equal(1, rebuilds); Assert.Equal(first, owner.Input.Logical.ToArray());
            Assert.Equal(188, assets!.LoadedTypes.Count); Assert.Equal(82, assets.TypeTable.Find("bridge")); Assert.Equal(2304, new FileInfo(Path.Combine(root.Cache(), "!color.dat")).Length);
            var primary = owner.Input.Primary; byte[] primaryBytes = primary.Bytes.ToArray(); var maps = owner.Input.Maps; var tokens = owner.Input.OwnerTables.Tokens;
            byte[] bridge = owner.Input.OwnerTables.Color(82, 1); Assert.Equal(256, bridge.Length);
            GumpShadeMaps shades = screen.ShadeMaps(); Assert.Equal(maps[4], shades.Bright); Assert.Equal(maps[6], shades.Dark); Assert.NotSame(maps[4], shades.Bright);
            uint playerBase = game.PlayerRecords.Base; startup.Draw(screen, owner.PrepareScreen, owner.Rebuild);
            var next = Screen(owner.PaletteGlobals.Loaded); startup.Draw(next, owner.PrepareScreen, owner.Rebuild);
            Assert.Equal(1, prefixes); Assert.Equal(1, types); Assert.Equal(playerBase, game.PlayerRecords.Base);
            Assert.Same(primary, owner.Input.Primary); Assert.Equal(primaryBytes, primary.Bytes.ToArray()); Assert.Equal(first, owner.Input.Logical.ToArray());
            Assert.Same(maps, owner.Input.Maps); Assert.True(tokens.Equals(owner.Input.OwnerTables.Tokens)); Assert.Same(bridge, owner.Input.OwnerTables.Color(82, 1));
            Assert.Same(shades, screen.ShadeMaps()); Assert.Equal(maps[4], next.ShadeMaps().Bright); Assert.Equal(maps[6], next.ShadeMaps().Dark);
        }
    }

    /// <summary>초기 옵션은 무시하고 준비 뒤 같은 Config 쓰기가 현재 owner/dirty를 갱신하며 자기 구독만 정리한다.</summary>
    [Fact]
    public void ConfigChangesUseCurrentTablesAndPreserveOtherSubscribers()
    {
        if (!OperatingSystem.IsWindows()) return;
        var startup = Startup(); var screen = Screen(); var root = Root(); int before = 0, after = 0;
        Action first = () => before++; Action last = () => after++; startup.Configuration.OnChanged += first;
        var owner = new HostStartupColorMaps(startup, () => { }, () => new HostAssetTables(startup, new RawSurfaceStorage()), root);
        startup.Configuration.OnChanged += last;
        startup.Configuration.SetInt("useBanners", 0); Assert.Equal(1u, owner.Input.OwnerTables.Enabled); Assert.False(owner.Input.OwnerTables.Ready);
        screen.LoadPalette(startup.Palette, 1); owner.PrepareScreen(screen); Assert.Equal(1u, owner.Input.OwnerTables.Enabled);
        var tokens = owner.Input.OwnerTables.Tokens; uint dirty = startup.Cloud.WorldRedraw;
        startup.Configuration.SetInt("useBanners", 0);
        Assert.Equal(0u, owner.Input.OwnerTables.Enabled); Assert.Equal(unchecked(dirty + 1), startup.Cloud.WorldRedraw); Assert.NotEqual(0u, tokens.Span[97]); Assert.NotEqual(0u, tokens.Span[94]);
        dirty = startup.Cloud.WorldRedraw; startup.Configuration.SetInt("useBanners", 0); Assert.Equal(dirty, startup.Cloud.WorldRedraw);
        startup.Configuration.SetInt("useBanners", 1); Assert.Equal(1u, owner.Input.OwnerTables.Enabled); Assert.Equal(0u, tokens.Span[97]); Assert.Equal(0u, tokens.Span[94]);
        Assert.True(tokens.Equals(owner.Input.OwnerTables.Tokens)); Assert.Equal(4, before); Assert.Equal(4, after);
        owner.Dispose(); owner.Dispose(); dirty = startup.Cloud.WorldRedraw; startup.Configuration.SetInt("useBanners", 0);
        Assert.Equal(5, before); Assert.Equal(5, after); Assert.Equal(1u, owner.Input.OwnerTables.Enabled); Assert.Equal(dirty, startup.Cloud.WorldRedraw);
        Assert.Throws<ObjectDisposedException>(() => owner.PrepareScreen(screen)); Assert.Throws<ObjectDisposedException>(() => owner.Rebuild(screen));
    }

    /// <summary>기존2304BYTE를 실제 CRT로 읽고 새 화면과 재준비에서 캐시/고정 입력을 보존한다.</summary>
    [Fact]
    public void CacheHitAndNewScreenKeepSameNineRows()
    {
        if (!OperatingSystem.IsWindows()) return;
        var startup = Startup(); var root = Root(); Directory.CreateDirectory(root.Cache());
        byte[] cache = Enumerable.Range(0, 2304).Select(index => unchecked((byte)(index * 7 + 23))).ToArray();
        string path = Path.Combine(root.Cache(), "!color.dat"); File.WriteAllBytes(path, cache); var screen = Screen(); screen.LoadPalette(startup.Palette, 1);
        int prefixes = 0, types = 0;
        using var owner = new HostStartupColorMaps(startup, () => prefixes++, () => { types++; return new HostAssetTables(startup, new RawSurfaceStorage()); }, root);
        owner.PrepareScreen(screen); var maps = owner.Input.Maps; uint[] logical = owner.Input.Logical.ToArray(); var primary = owner.Input.Primary;
        int[] fileOrder = [2, 7, 6, 4, 3, 5, 1, 0, 8];
        // 파일의 각256BYTE가 원문 순서의 같은 출력 행에 들어갔는지 독립 입력과 비교한다.
        for (int index = 0; index < fileOrder.Length; index++) Assert.Equal(cache.AsSpan(index * 256, 256).ToArray(), maps[fileOrder[index]]);
        var next = Screen(owner.PaletteGlobals.Loaded); owner.PrepareScreen(next); owner.PrepareScreen(screen);
        Assert.Equal(1, prefixes); Assert.Equal(1, types); Assert.Equal(logical, owner.Input.Logical.ToArray()); Assert.Same(primary, owner.Input.Primary);
        Assert.Same(maps, owner.Input.Maps); Assert.Equal(cache, File.ReadAllBytes(path)); Assert.Equal(maps[4], next.ShadeMaps().Bright); Assert.Equal(maps[6], next.ShadeMaps().Dark);
    }
}
