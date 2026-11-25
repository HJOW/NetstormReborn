using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// Native WorldRuntime 장치 일원화: 제품 순서의 공유 소유자 행렬을 고정한다.
// 2026-11-26 1.059에서 SL-6용으로 추가했다. 주소 세대는 R4-8·M1에서 잇는다.
public sealed class HostSharedDevicesRootTests
{
    /// <summary>제품 StartMission과 같은 공유 장기 소유자를 묶는다.</summary>
    /// <returns>게임·로컬·시계·커널·표면·공유 저장소다.</returns>
    private static (ClientGameplayGlobals Game, ClientLocalStateGlobals Local, ClientClockGlobals Clock, Kernel Kernel, ClientSurfaceGlobals Surface, RawSpStore Sp, UIntBox CrtBox, CampaignTemplateStorage Templates) Scene()
    {
        var game = new ClientGameplayGlobals();
        var local = new ClientLocalStateGlobals(game, new AmbientSoundState());
        var clock = new ClientClockGlobals(local);
        var kernel = new Kernel();
        var surface = new ClientSurfaceGlobals(game, clock, kernel, () => 0u);
        var gameRandom = new GameRandom(71u);
        var crtBox = new UIntBox { Value = 1u };
        var modeBox = new UIntBox { Value = 0u };
        var seed = new RawGameRandomSeed(gameRandom, crtBox, modeBox, () => 946684800);
        var shared = new RawSpStore(gameRandom, () => seed.Reseed());
        shared.Initialize();
        var templates = new CampaignTemplateStorage();
        // 제품과 같은 순서로 표면 저장소에 템플릿 직렬화 전역을 먼저 연결한다.
        surface.Storage.BindSerializationGlobals(templates.SerializationGlobals);
        return (game, local, clock, kernel, surface, shared, crtBox, templates);
    }

    /// <summary>같은 장기 소유자를 빌린 세션은 장치를 복제하지 않는다.</summary>
    [Fact]
    public void ProductOrderSharesSingleWorldRuntimeDevices()
    {
        (ClientGameplayGlobals game, ClientLocalStateGlobals local, ClientClockGlobals clock, Kernel kernel, ClientSurfaceGlobals surface, RawSpStore shared, UIntBox crtBox, CampaignTemplateStorage templates) = Scene();
        var session = new HostSession(kernel, new GameRandom(73u), new CrtRandom(),
            sharedSp: shared, sharedCrtBox: crtBox, sharedGameplay: game, pool: surface.Storage.Pool,
            sharedTemplates: templates, sharedProcessTime: clock.GameTime, sharedSurface: surface);
        Assert.Same(surface.Storage.Pool, session.Pool);
        Assert.Same(shared, session.Sp);
        Assert.Same(game, session.Gameplay);
        Assert.Same(game, surface.Gameplay);
        Assert.Same(templates, session.Templates);
        Assert.Same(clock.GameTime, session.ProcessTime);
        Assert.Same(local, session.LocalState);
        Assert.Same(local, session.Connections.LocalState);
        Assert.Same(kernel, session.Kernel);
        Assert.Same(game.PlayerRecords, surface.Gameplay.PlayerRecords);
        // Root 등록은 CreatePrepared에서만 일어나며 조립만으로 빌리지 않는다.
        Assert.False(game.Borrowed());
        Assert.Throws<InvalidOperationException>(game.RequireSession);
    }

    /// <summary>소유자가 엇갈리면 조립 단계에서 거부한다.</summary>
    [Fact]
    public void MismatchedOwnersFailAtAssembly()
    {
        (ClientGameplayGlobals game, ClientLocalStateGlobals _, ClientClockGlobals _, Kernel kernel, ClientSurfaceGlobals surface, RawSpStore shared, UIntBox crtBox, CampaignTemplateStorage _) = Scene();
        var other = new ClientGameplayGlobals();
        Assert.Throws<ArgumentException>(() => new HostSession(kernel, new GameRandom(73u), new CrtRandom(),
            sharedSp: shared, sharedCrtBox: crtBox, sharedGameplay: other, pool: surface.Storage.Pool, sharedSurface: surface));
    }
}
