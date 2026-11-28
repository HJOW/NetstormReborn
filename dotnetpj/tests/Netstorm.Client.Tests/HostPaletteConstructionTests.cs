using System.Reflection;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 실제 시작 owner를 빌리되 준비 전 identity 실패만 검사한다.

/// <summary>잘못된 인계가 옵션/입력/파일·시각·난수의 초기 효과를 앞당기지 않음을 검사한다.</summary>
public sealed class HostPaletteConstructionTests
{
    /// <summary>실제 자산을 읽기 전용으로 준비한다. 아래 실패 검사 중에는 새 파일을 열지 않는다.</summary>
    private static HostStartupScene Startup() => new(GameDataLocator.FindDataDirectory() ?? throw new InvalidOperationException("실제 시작 자산 부재"));
    /// <summary>호출하지 않은 캐시 경로가 만들어지지 않았는지 관측할 검사 소유 경로다.</summary>
    private static UserDataRoot Root() => new(Path.Combine(Path.GetTempPath(), "Netstorm-PaletteOwner-" + Guid.NewGuid().ToString("N")));
    /// <summary>base Game을 실행하지 않고 private 순수 생성 검증의 성공 결과만 관측한다.</summary>
    private static ClientPaletteGlobals ValidatedOwner(HostFrameDriver driver, HostOptions options, HostStartupScene? startup,
        HostTimeSource clock, ClientPaletteGlobals? palette, bool explicitOwner)
    {
        MethodInfo method = typeof(ClientHostGame).GetMethod("ValidatePaletteConstruction", BindingFlags.Static | BindingFlags.NonPublic)!;
        object construction = method.Invoke(null, [driver, options, startup, clock, palette, explicitOwner])!;
        return (ClientPaletteGlobals)construction.GetType().GetProperty("Palette")!.GetValue(construction)!;
    }

    /// <summary>명시 Game의 null/foreign/필수인자 실패는 base Game보다 먼저 발생하고 실제 시작 자료를 보존한다.</summary>
    [Fact]
    public void GameRejectsOwnersBeforeBaseConstructionAndOtherStartupEffects()
    {
        var startup = Startup(); var palette = new ClientPaletteGlobals(); palette.Loaded.Value = 0x80000001;
        var screen = new Screen(4, 2, palette.Loaded); var input = new InputQueue(); input.Push(0x41, 7, 9);
        var clock = new HostTimeSource(); clock.EnableFixed(new HostFixedTime(25, 1234, 946684800));
        var driver = new HostFrameDriver(screen, input, clock: clock); var options = HostOptions.Parse([]);
        Action? subscribers = startup.Configuration.OnChanged; uint random = startup.Random.State(), dirty = startup.Cloud.WorldRedraw;
        int fileIndex = startup.Files.FileNameOwner.Index; byte[] currentFile = startup.Files.FileNameOwner.CurrentBytes.ToArray();
        int slots = input.FreeSlots(); uint milliseconds = clock.Milliseconds; int unix = clock.UnixSeconds;
        Assert.Throws<ArgumentNullException>(() => new ClientHostGame(driver, options, startup, clock, null!));
        Assert.Throws<ArgumentException>(() => new ClientHostGame(driver, options, startup, clock, new ClientPaletteGlobals()));
        Assert.Throws<ArgumentNullException>(() => new ClientHostGame(null!, options, startup, clock, palette));
        Assert.Throws<ArgumentNullException>(() => new ClientHostGame(driver, null!, startup, clock, palette));
        Assert.Same(subscribers, startup.Configuration.OnChanged); Assert.Equal(random, startup.Random.State()); Assert.Equal(dirty, startup.Cloud.WorldRedraw);
        Assert.Equal(fileIndex, startup.Files.FileNameOwner.Index); Assert.Equal(currentFile, startup.Files.FileNameOwner.CurrentBytes.ToArray());
        Assert.Equal(milliseconds, clock.Milliseconds); Assert.Equal(unix, clock.UnixSeconds); Assert.Equal(0ul, driver.Frame);
        Assert.Equal(slots, input.FreeSlots()); Assert.Equal(new InputEvent(0x41, 7, 9), input.Pop(false));
        Assert.Equal(0x80000001u, screen.PaletteLoaded);
    }

    /// <summary>호환4인자와 제품5인자의 순수 검증은 preloaded 저장소 identity를 보존한다. 성공 Game/창 검사는 아니다.</summary>
    [Fact]
    public void GameValidationAdoptsPreloadedStorageAndRetainsExplicitCanonicalOwner()
    {
        var screen = new Screen(4, 2); screen.PaletteLoadedOwner.Value = 0xf0123456;
        var driver = new HostFrameDriver(screen, new InputQueue()); var clock = new HostTimeSource();
        ClientPaletteGlobals adopted = ValidatedOwner(driver, HostOptions.Parse([]), null, clock, null, false);
        Assert.Same(screen.PaletteLoadedOwner, adopted.Loaded); Assert.Equal(0xf0123456u, screen.PaletteLoaded);
        Assert.Same(adopted, ValidatedOwner(driver, HostOptions.Parse([]), null, clock, adopted, true));
    }

    /// <summary>모든 필수 ctor 인자를 구독/경로 준비/초기 callback 전에 거부한다.</summary>
    [Fact]
    public void ColorMapsConstructorValidationLeavesSubscriptionsAndFilesUnchanged()
    {
        var startup = Startup(); var root = Root(); int prefixes = 0, assets = 0;
        Action prefix = () => prefixes++; Func<HostAssetTables> prepare = () => { assets++; throw new InvalidOperationException("미선택 TYPE"); };
        Action? subscribers = startup.Configuration.OnChanged; int index = startup.Files.FileNameOwner.Index;
        uint random = startup.Random.State(), dirty = startup.Cloud.WorldRedraw;
        Assert.Throws<ArgumentNullException>(() => new HostStartupColorMaps(startup, prefix, prepare, root, null!));
        Assert.Throws<ArgumentNullException>(() => new HostStartupColorMaps(null!, prefix, prepare, root));
        Assert.Throws<ArgumentNullException>(() => new HostStartupColorMaps(startup, null!, prepare, root));
        Assert.Throws<ArgumentNullException>(() => new HostStartupColorMaps(startup, prefix, null!, root));
        Assert.Same(subscribers, startup.Configuration.OnChanged); Assert.Equal(index, startup.Files.FileNameOwner.Index);
        Assert.Equal(random, startup.Random.State()); Assert.Equal(dirty, startup.Cloud.WorldRedraw);
        Assert.Equal(0, prefixes); Assert.Equal(0, assets); Assert.False(Directory.Exists(root.Cache()));
    }

    /// <summary>standalone은 첫 Prepare에서 인계하고 준비 실패 뒤에도 identity를 유지한다. Rebuild는 먼저 인계하지 않는다.</summary>
    [Fact]
    public void LazyAdoptionSurvivesPrefixFailureAndRejectsForeignScreenBeforeEffects()
    {
        var startup = Startup(); var root = Root(); var first = new Screen(4, 2); first.PaletteLoadedOwner.Value = 1;
        var foreign = new Screen(4, 2); foreign.PaletteLoadedOwner.Value = 1;
        var clock = new HostTimeSource(); clock.EnableFixed(new HostFixedTime(25)); int prefixes = 0;
        var failure = new InvalidOperationException("prefix");
        using var owner = new HostStartupColorMaps(startup, () => { prefixes++; throw failure; },
            () => throw new InvalidOperationException("미선택 TYPE"), root);
        Assert.Throws<InvalidOperationException>(() => owner.PaletteGlobals);
        Assert.Throws<InvalidOperationException>(() => owner.Rebuild(first));
        Assert.Throws<ArgumentNullException>(() => owner.PrepareScreen(null!));
        Assert.Throws<InvalidOperationException>(() => owner.PaletteGlobals);
        Assert.Same(failure, Assert.Throws<InvalidOperationException>(() => owner.PrepareScreen(first)));
        Assert.Same(first.PaletteLoadedOwner, owner.PaletteGlobals.Loaded); Assert.False(owner.Input.Captured);
        uint random = startup.Random.State(), now = clock.Milliseconds, dirty = startup.Cloud.WorldRedraw;
        int index = startup.Files.FileNameOwner.Index; Action? subscribers = startup.Configuration.OnChanged;
        Assert.Throws<ArgumentException>(() => owner.PrepareScreen(foreign));
        Assert.Throws<ArgumentException>(() => owner.Rebuild(foreign));
        Assert.Throws<ArgumentNullException>(() => owner.Rebuild(null!));
        Assert.Equal(1, prefixes); Assert.False(owner.Input.Captured); Assert.False(owner.Input.MapsReady);
        Assert.Equal(random, startup.Random.State()); Assert.Equal(now, clock.Milliseconds); Assert.Equal(index, startup.Files.FileNameOwner.Index);
        Assert.Equal(dirty, startup.Cloud.WorldRedraw); Assert.Same(subscribers, startup.Configuration.OnChanged); Assert.False(Directory.Exists(root.Cache()));
        var next = new Screen(8, 4, owner.PaletteGlobals.Loaded);
        Assert.Same(failure, Assert.Throws<InvalidOperationException>(() => owner.PrepareScreen(next))); Assert.Equal(2, prefixes);
        Assert.Same(first.PaletteLoadedOwner, next.PaletteLoadedOwner); Assert.Equal(1u, next.PaletteLoaded);
    }

    /// <summary>실제 Game용 명시 색표 경로는 첫 화면부터 다른 저장소를 거부한다.</summary>
    [Fact]
    public void ExplicitColorMapsNeverAdoptForeignScreen()
    {
        var startup = Startup(); var palette = new ClientPaletteGlobals(); var root = Root(); int prefixes = 0;
        using var owner = new HostStartupColorMaps(startup, () => prefixes++, () => throw new InvalidOperationException("미선택 TYPE"), root, palette);
        Assert.Same(palette, owner.PaletteGlobals);
        Assert.Throws<ArgumentException>(() => owner.PrepareScreen(new Screen(4, 2)));
        Assert.Throws<ArgumentException>(() => owner.Rebuild(new Screen(4, 2)));
        Assert.Equal(0, prefixes); Assert.False(owner.Input.Captured); Assert.False(Directory.Exists(root.Cache()));
    }
}
