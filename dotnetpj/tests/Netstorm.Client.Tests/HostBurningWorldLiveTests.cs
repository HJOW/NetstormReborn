using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 게임창 없이 제품과 같은 실제자산/장치와 동일Host의 연소·회복 수명을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 실제 사제 장면에 제품 연소/캡처/팔레트 장치를 결합한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>프로세스→외부팔레트→소리 순서로 반납하는 실제 자료 장면이다.</summary>
    private sealed class BurningContext : IDisposable
    {
        /// <summary>실제 공유 월드/HP/난수/프로세스다.</summary>
        public HostSession Session { get; }
        /// <summary>원본파일은 변경하지 않고 현재TYPE 객체만 검사 입력으로 빌린다.</summary>
        public IReadOnlyList<OriginalTypeRecord> Types { get; }
        /// <summary>창이 없는 실제8비트 논리 표면이다.</summary>
        public Screen Screen { get; } = new(1024, 768);
        /// <summary>실제 소리몸체를 음소거관문으로 연결한다.</summary>
        public HostMissionAudio Audio { get; }
        /// <summary>제품과 같은SHP/현재raw/실제팔레트 장치다.</summary>
        public HostMissionBurning Visuals { get; }
        /// <summary>실제 입력 프레임 선택에 사용하는 별도 자산 판독기다.</summary>
        public Netstorm.Assets.ShapeDatabase SourceShapes { get; }
        /// <summary>실제 클러스터 개수/기본프레임 메타데이터다.</summary>
        public IReadOnlyList<RiftTypeDefinition> Definitions { get; }
        /// <summary>아직 없는UI의 텍스트 요청만 관측한다.</summary>
        public List<string> Messages { get; } = [];
        /// <summary>사제효과를 먼저 값복사한 뒤에도 후행연소 owner를 찾게 구성한다.</summary>
        [SupportedOSPlatform("windows")]
        public BurningContext(Action<int>? overrideRegen = null, ClientSurfaceGlobals? sharedSurface = null)
        {
            (Session, Types) = Scene(sharedSurface); string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
            // 실제 준비 소유자와 공통 음량 보류로 장면을 구성하며 이 검사는 창/장치를 열지 않는다.
            var startup = new HostStartupScene(data); Audio = new(startup, static () => 0); Audio.PushMute();
            RawPriestWorldEffects effects = Audio.BindPriest(Session);
            effects.Shield.Translate = key => key; effects.Shield.Tell = Messages.Add; effects.Regen.Damageable = overrideRegen;
            Session.BuildPriestWorldDevices(effects);
            Definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
            byte[] shp = startup.Files.ReadAllBytes("d/_shapes.shp");
            SourceShapes = new(shp); var shapes = TerrainDisplayShapes.Build(Definitions, SourceShapes);
            Visuals = new(Session, startup, Screen, Types, Definitions, shapes, shp, Audio);
            Screen.SetClip(0, 0, Screen.Width, Screen.Height);
        }
        /// <summary>이 장면이 소유하는 실제 Host/외부장치만 반납한다.</summary>
        public void Dispose() { Session.ReleasePlayerProcesses(); Visuals.Dispose(); Audio.Dispose(); }
    }

    /// <summary>원본과 같은 현재HP의 입력 DWORD를 지정하며 포획/전투를 실행하지 않는다.</summary>
    private static void RawHp(HostSession session, int sid, int hp) => BinaryPrimitives.WriteInt32LittleEndian(session.Pool.AllocatedBytes(sid)[26..], hp);

    /// <summary>실제 TYPE의 저HP가 같은 부모체인의14 Form을 만들고 절반 경계에서 실제 삭제한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductLowHpUsesSameRenderingHostAndActualFormLifetime()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session;
        int type = Enumerable.Range(70, scene.Types.Count - 70).First(i => i != 135 && scene.Types[i].DamageEffect != 0 && (scene.Types[i].Flags1 & 0x10) != 0 && scene.Types[i].MaxHitPoints > 0);
        int victim = session.BuildPlayerWorldHooks().Create!((uint)type, 2);
        int maximum = session.Reward.TypeHitPoints((uint)type); RawHp(session, victim, maximum / 2 - 1);
        int form = session.WorldDamageableBurning.Refresh(victim);
        Assert.NotEqual(0, form); Assert.Equal(14, session.Pool.Slot(form)[10]); Assert.Equal(1, session.WorldBurning.Size);
        Assert.Same(session.PlayerProcesses, session.WorldRendering.Host); Assert.Same(session.Pool, session.WorldRendering.Pool);
        var lookup = new RawContainedLookup(session.Pool, (_, _) => throw new InvalidOperationException("조회가 삭제를 요청함"));
        Assert.Equal(form, lookup.Find(victim, 14)); Assert.Equal(form, session.WorldDamageableBurning.Refresh(victim));
        Assert.Equal(1, session.Kernel.Size()); RawHp(session, victim, maximum / 2);
        Assert.Equal(0, session.WorldDamageableBurning.Refresh(victim)); Assert.Equal(0, session.WorldBurning.Size);
        Assert.Equal(0, lookup.Find(victim, 14)); Assert.Equal(0, session.Kernel.Size()); Assert.Equal(0, session.WorldRendering.Size);
    }

    /// <summary>실제SHP/첫팔레트·캡처와 Kernel 단계/픽셀·반납을 연결하고 없는제품뷰는 명시실패한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductCaptureAndKernelNeedRealViewThenDrawActualPixels()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session;
        int victim = session.BuildPlayerWorldHooks().Create!(70, 2); Span<byte> raw = session.Pool.AllocatedBytes(victim);
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 20); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 20);
        BurningVisualProcess process = session.WorldBurning.Add(victim, 0); Assert.Equal(1, session.Kernel.Size());
        Assert.Contains("viewport", Assert.Throws<InvalidOperationException>(() => session.Kernel.RunFrame()).Message);
        scene.Visuals.SetView(0, 0, new(0, 0, 1024, 768)); scene.Visuals.PrepareFrame(0, 1); session.Kernel.RunFrame();
        Assert.Equal(1u, process.Fields.Stage); Assert.True(process.Fields.Image.Width > 0); Assert.Contains(process.Fields.Image.Pixels, pixel => pixel != 255);
        scene.Visuals.PrepareFrame(.1, 2); session.Kernel.RunFrame(); Assert.Equal(2u, process.Fields.Stage);
        scene.Visuals.PrepareFrame(.2, 3); session.Kernel.RunFrame(); Assert.True(process.Fields.Total > 0);
        // 표시 재진입의 첫 프레임은 시각차0이므로 다음 실제시각에서 이동입자가 생성된다.
        scene.Visuals.PrepareFrame(.4, 4); session.Kernel.RunFrame();
        Assert.Contains(process.Fields.Particles, particle => !particle.Source && particle.Life > 0);
        session.WorldRendering.Draw(); Assert.Contains(scene.Screen.Pixels.ToArray(), pixel => pixel != 0); Assert.NotEmpty(scene.Visuals.Dirty.Entries);
        uint foreign = session.Kernel.Add(new BaseProcess()); session.ReleasePlayerProcesses();
        Assert.Equal(1, session.Kernel.Size()); Assert.NotNull(session.Kernel.Get(foreign)); session.Kernel.Remove(foreign);
        Assert.Throws<InvalidOperationException>(() => session.WorldBurning); Assert.Throws<InvalidOperationException>(() => session.WorldRendering);
    }

    /// <summary>현재 사제TYPE의nonzero damageEffect 입력으로 사제효과 사본이 후행 실제연소를 조회하는지 검증한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void RegenerationCopiedBeforeBurningUsesCurrentRealOwnerAndRemovesAtHalf()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session;
        int priest = PlacePriest(session, scene.Types, 0, 9); scene.Types[158].DamageEffect = 1;
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x7c), 1);
        int maximum = session.Reward.MaxHitPoints(priest); RawHp(session, priest, 0);
        session.PriestWorld.UniqueRegular(priest, 602, 0, 0);
        Assert.InRange(session.PriestWorld.PriestState.CurrentHitPoints(priest), 1, maximum / 2 - 1); Assert.Equal(1, session.WorldBurning.Size);
        RawHp(session, priest, maximum / 2 - 1); session.PriestWorld.UniqueRegular(priest, 602, 1, 0);
        Assert.True(session.PriestWorld.PriestState.CurrentHitPoints(priest) >= maximum / 2); Assert.Equal(0, session.WorldBurning.Size);
    }

    /// <summary>명시적으로 준 실제상위 효과는 기본결선으로 덮어쓰지 않는다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ExplicitRegenerationEffectRemainsSelected()
    {
        if (!OperatingSystem.IsWindows()) return;
        var calls = new List<int>(); using var scene = new BurningContext(calls.Add); HostSession session = scene.Session;
        int priest = PlacePriest(session, scene.Types, 0, 9); scene.Types[158].DamageEffect = 1; RawHp(session, priest, 0);
        session.PriestWorld.UniqueRegular(priest, 602, 0, 0);
        Assert.Equal(new[] { priest }, calls); Assert.Equal(0, session.WorldBurning.Size);
    }

    /// <summary>실제 소유자 일반색표·사제의 빈+a4를 사용하며 TYPE만 같은 다른VT는 거부한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ActualPriestAdditionalCaptureUsesCurrentOwnerAndExactVtable()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session;
        int priest = PlacePriest(session, scene.Types, 0, 9); session.BridgeState.OwnerColors[1] = 3;
        // 실제SHP의 독립Decoder에서 보이는 프레임을 입력으로 선택한다. 초기0프레임은 빈그림일 수 있다.
        int frame = Enumerable.Range(0, scene.Definitions[158 - 70].Clusters.Count)
            .First(number => scene.SourceShapes.Decode(scene.SourceShapes.Blocks[158 - 70].Frames[number]).Opaque.Any(opaque => opaque));
        BinaryPrimitives.WriteInt32LittleEndian(session.Pool.AllocatedBytes(priest)[36..], frame);
        ConstructionVisualImage image = scene.Visuals.Capture.Capture(priest);
        Assert.True(image.Width > 0); Assert.Contains(image.Pixels, pixel => pixel != 255);
        session.Pool.AllocatedBytes(priest)[40] |= 2;
        image = scene.Visuals.Capture.Capture(priest);
        Assert.Equal(scene.Definitions[158 - 70].SpecialFrames.BaseFrame != -1, image.Pixels.Any(pixel => pixel != 255));
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(priest));
        BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(priest), table ^ 1);
        Assert.Contains("주그림", Assert.Throws<InvalidOperationException>(() => scene.Visuals.Capture.Capture(priest)).Message);
        BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(priest), table);
    }

    /// <summary>현재 전체checking·실제선택물리·장기최근TYPE/실패비트와번호부모의게이트를 확인한다.</summary>
    [Fact]
    public void CurrentSidDiagnosticsUseLiveManagedPoolAndLongLivedOwner()
    {
        var (session, _) = Scene(); uint checking = 0; session.AiChecking.Bind(() => checking);
        session.Gameplay.SidRobustDiagnostics.LastRobust = 99; session.Gameplay.SidRobustDiagnostics.Type = 77;
        Assert.True(session.CurrentWorldSidRobust(65534, true)); Assert.Equal(99u, session.Gameplay.SidRobustDiagnostics.LastRobust);
        int sid = session.BuildPlayerWorldHooks().Create!(70, 2); checking = 0x80000000;
        Assert.True(session.CurrentWorldSidRobust(sid, true)); Assert.Equal(0u, session.Gameplay.SidRobustDiagnostics.LastRobust); Assert.Equal(70u, session.Gameplay.SidRobustDiagnostics.Type);
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(sid)); BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(sid), 0);
        Assert.Contains("resolve( sid ).isRobust()", Assert.Throws<InvalidOperationException>(() => session.CurrentWorldSidRobust(sid, true)).Message);
        Assert.Equal(2u, session.Gameplay.SidRobustDiagnostics.LastRobust); Assert.Equal(70u, session.Gameplay.SidRobustDiagnostics.Type);
        BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(sid), table); session.ReleasePlayerProcesses();
        var next = new HostSession(new Kernel(), new GameRandom(), new CrtRandom(), sharedGameplay: session.Gameplay);
        Assert.Same(session.Gameplay.SidRobustDiagnostics, next.Gameplay.SidRobustDiagnostics); Assert.Equal(2u, next.Gameplay.SidRobustDiagnostics.LastRobust);
    }
}
