using System.Buffers.Binary;
using Netstorm.Client.Audio;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 제품과 같은 실제 자료·공통 공간·HP·프로세스를 사용해 사제 결선을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>호스트 사제의 실제 등록·현재 owner/SP 회복·반납·미연결 경계를 검증한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실자산과 시작 때 초기화한 SP·장기 목록을 같은 세션에 연결한다. 공유 표면 지정 시 제품처럼 같은 풀/게임/해시/점유 지도를 빌린다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) Scene(ClientSurfaceGlobals? sharedSurface = null, bool initializeSurfaceAssets = false, Action<List<SquidDisplayShape>>? inspectShapes = null, CampaignTemplateStorage? sharedTemplates = null)
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var random = new GameRandom(1);
        var globals = new ClientSpGlobals(random, new UIntBox { Value = 1 }, new UIntBox(), () => 1760000000);
        globals.Store.Initialize();
        var session = new HostSession(sharedSurface?.Kernel ?? new Kernel(), random, new CrtRandom(), sharedSp: globals.Store,
            sharedGameplay: sharedSurface?.Gameplay, pool: sharedSurface?.Storage.Pool,
            sharedSurface: sharedTemplates is null ? null : sharedSurface, sharedTemplates: sharedTemplates);
        if (sharedSurface is not null) RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, session.BattleOptions);
        session.MissionPlayer.PlayerKnown = 1; session.MissionPlayer.BattleOwner = 1;
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions); RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        var shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(startup.Files.ReadAllBytes("d/_shapes.shp")));
        // 실제Host가빌릴동일SHP목록을검사에서관찰하며별도복사표를만들지않는다.
        inspectShapes?.Invoke(shapes);
        IReadOnlyList<OriginalTypeRecord> types = table.Types;
        if (initializeSurfaceAssets)
        {
            Assert.NotNull(sharedSurface);
            if (sharedTemplates is null) sharedSurface.Storage.InitializeAssetTables(types, session.WorldFrames, shapes);
            else _ = new HostAssetTables(startup, sharedSurface.Storage);
            types = sharedSurface.Storage.Types;
        }
        session.BuildTerrainPopDevices(types, sharedTemplates is null ? shapes : sharedSurface!.Storage.Shapes, hash: sharedSurface?.Storage.Hash, spots: sharedSurface?.Storage.Spots); session.BuildFortObjectDevices(types, session.WorldFrames);
        session.BuildWorldGraphDevices(types);
        return (session, types);
    }

    /// <summary>실제 지형 도장으로 지면을 만들고 같은 공장/소유자/Pop으로 사제를 등록한다.</summary>
    private static int PlacePriest(HostSession session, IReadOnlyList<OriginalTypeRecord> types, uint allocation = 2, uint flags = 0)
    {
        TerrainStampHooks stamp = session.BuildTerrainStampHooks(types);
        int ground = stamp.Create!(97, 1); stamp.Pop!(ground, 50, 60, 0x02000061);
        RawPriestWorldBindings world = session.BuildPriestWorldDevices();
        session.SetWorldLoadingDepth(1);
        int priest = session.BuildPlayerWorldHooks().Create!(158, allocation); world.SetOwner(priest, 1);
        session.PopWorldObject(priest, 50, 60, flags);
        session.SetWorldLoadingDepth(0);
        return priest;
    }

    /// <summary>아직 없는 공간·공장 앞에서는 사제 구성을 시작하지 않는다.</summary>
    [Fact]
    public void PriestWorldRequiresRealWorldDevices()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.BuildPriestWorldDevices());
        Assert.Throws<InvalidOperationException>(() => session.PriestWorld);
    }

    /// <summary>제품과 같은 실제 Fort 로더가 사제 Pop을 건너뛰지 않고 목록·공통 장부·회복 Form에 도달한다.</summary>
    [Fact]
    public void RealFortLoaderRegistersBothPriestsAndTheirRegenerationForms()
    {
        var (session, types) = Scene();
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        session.BuildPriestWorldDevices();
        HostFortObjectLiveTests.BuildLiveTerrainFirst(session, fort.Fort, types);
        session.LoadFortObjects(fort.Fort, 1);
        Assert.Equal(2u, session.SidLists.Priests.Count); Assert.Equal(2u, session.TotalMade.GlobalCounts[158]);
        Assert.Equal(1u, session.TotalMade.LocalCounts[158]); Assert.Equal(2, session.Kernel.Size());
        // 실제 로더가 등록한 두 사제의 공간/원래 소유자와 실제602 부모를 검사한다.
        foreach (uint entry in session.SidLists.Priests.Items())
        {
            int sid = checked((int)entry); ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
            Assert.Equal(158, raw[10]); Assert.Equal(0, raw[11] & RawSidPool.VoidBit);
            Assert.Equal(raw[34], raw[12] & 0x7f); Assert.NotNull(session.PlayerProcesses.FindEvent(sid, 602));
        }
        Assert.Equal(0u, session.PriestState.Owner.LoadingDepth); Assert.Equal(0u, session.PriestState.Carrier.ReadLoadingDepth());
        session.ReleasePlayerProcesses(); Assert.Equal(0, session.Kernel.Size());
    }

    /// <summary>일반 표면 VT를 포함한 실제 Fort도 현재 사제 전용 적재 어댑터에서 사제/보호막만 선택한다.</summary>
    [Fact]
    public void FortWithSurfaceVtablesLoadsThroughInstalledPriestBodies()
    {
        var (session, types) = Scene(); string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); MissionFortLoadResult fort = MissionFortLoad.Load(startup, "bc3acid_bar76");
        session.BuildPriestWorldDevices(); HostFortObjectLiveTests.BuildLiveTerrainFirst(session, fort.Fort, types);
        session.LoadFortObjects(fort.Fort, 1); Assert.True(session.SidLists.Priests.Count > 0); Assert.Equal(0u, session.PostPopDepth);
        // 실제 적재된 각 사제의 회복 Form이 같은 Host에 붙어 있는지 확인한다.
        foreach (uint entry in session.SidLists.Priests.Items())
        { Assert.NotNull(session.PlayerProcesses.FindEvent(checked((int)entry), 602)); }
        session.ReleasePlayerProcesses(); Assert.Equal(0, session.Kernel.Size());
    }

    /// <summary>실제 사제 Pop이 공유 목록·장부·해시와 동일 ProcessHost에 회복 Regular를 등록한다.</summary>
    [Fact]
    public void ActualPopRegistersSharedListAndRegenerationForm()
    {
        var (session, types) = Scene(); int priest = PlacePriest(session, types, 0, 9);
        Assert.Same(session.SidLists.Priests, session.PriestState.Priests.GetPriests());
        Assert.Equal(1u, session.SidLists.Priests.Count); Assert.Equal((uint)priest, session.SidLists.Priests.Items()[0]);
        Assert.Equal(0, session.Pool.Slot(priest)[11] & RawSidPool.VoidBit);
        Assert.NotNull(session.PlayerProcesses.FindEvent(priest, 602)); Assert.Equal(1, session.Kernel.Size());
        Assert.NotEmpty(session.WorldPackets);
        Assert.Equal(1u, session.TotalMade.GlobalCounts[158]); Assert.Equal(1u, session.TotalMade.LocalCounts[158]);
        Assert.Equal(0u, session.PostPopDepth);
        session.UnpopWorldObject(priest, 0); Assert.NotEqual(0, session.Pool.Slot(priest)[11] & RawSidPool.VoidBit);
        session.RepopWorldObject(priest, 0);
        Assert.Equal(1u, session.SidLists.Priests.Count); Assert.Equal(1u, session.TotalMade.GlobalCounts[158]);
        Assert.Equal(1, session.Kernel.Size());
        uint outside = session.Kernel.Add(new BaseProcess());
        session.ReleasePlayerProcesses(); Assert.Equal(1, session.Kernel.Size());
        Assert.NotNull(session.Kernel.Get(outside)); session.Kernel.Remove(outside);
        Assert.Null(session.PriestState.HitPoints.PlayerRecords);
        Assert.Throws<InvalidOperationException>(() => session.PriestWorld);
    }

    /// <summary>실제 Regular가 같은 HP·Player9 허용 기록과 SP 추적 주소/회전을 사용한다.</summary>
    [Fact]
    public void ActualRegenerationUsesSharedPlayerHpAndSpOwners()
    {
        var (session, types) = Scene(); int priest = PlacePriest(session, types);
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x7c), 1);
        int maximum = session.Reward.MaxHitPoints(priest);
        session.PriestWorld.SetHitPoints(priest, maximum - 1);
        int tracking = session.Sp.TrackingOffsetCurrent.Value; uint offset = session.Sp.OffsetCurrent;
        uint random = session.Game.State();
        session.Kernel.RunFrame();
        Assert.Equal(maximum, session.PriestWorld.PriestState.CurrentHitPoints(priest));
        Assert.Equal(tracking + 3, session.Sp.TrackingOffsetCurrent.Value);
        Assert.NotEqual(offset, session.Sp.OffsetCurrent); Assert.NotEqual(random, session.Game.State());
        Assert.Equal(1u, session.PlayerProcesses.FindEvent(priest, 602)!.Count());
        Assert.Same(session.PlayerRecords, session.PriestState.HitPoints.PlayerRecords);
        session.ReleasePlayerProcesses(); Assert.Equal(0, session.Kernel.Size());
    }

    /// <summary>음소거에서도 실제 저체력 보호막·Form47·같은 Host 수명이 생성되고 함께 반납된다.</summary>
    [Fact]
    public void MutedRealAudioStillAttachesShieldSoundProcessAndReleasesIt()
    {
        if (!OperatingSystem.IsWindows()) return;
        var (session, types) = Scene();
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        // HWND 없이 실제 준비 소유자를 만들고 명시 보류로 보호막 소리의 수명만 검사한다.
        using var audio = new HostMissionAudio(new HostStartupScene(data), () => 1);
        audio.PushMute();
        RawPriestWorldEffects effects = audio.BindPriest(session); string? notice = null;
        // 아직 이식하지 않은 UI 출력만 단위 관측기로 받으며 보호막/소리 몸체는 제품과 동일하다.
        effects.Shield.Translate = key => key; effects.Shield.Tell = text => notice = text;
        session.BuildPriestWorldDevices(effects); int priest = PlacePriest(session, types);
        session.PriestWorld.SetHitPoints(priest, session.Reward.MaxHitPoints(priest) / 2 - 1);
        int shield = session.PriestWorld.Forcefield.Find(priest);
        Assert.NotEqual(0, shield); Assert.Equal(167, session.Pool.Slot(shield)[10]);
        Assert.Equal("PriestImmobile", notice); Assert.Equal(4, session.Kernel.Size());
        int soundForm = 0;
        // 실제 풀의 살아 있는47 Form이 같은 보호막을 부모로 삼는지 검사한다.
        for (int sid = RawSidPool.FirstOrdinary; sid < session.Pool.Capacity; sid++)
        {
            ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
            if ((raw[11] & RawSidPool.FreeBit) != 0 || raw[10] != session.GameAux.SoundProcessType) continue;
            Assert.Equal(0, soundForm); soundForm = sid;
            Assert.Equal(shield, RawContainedLink.CommonContainedParent(session.Pool, sid));
        }
        Assert.NotEqual(0, soundForm);
        Assert.IsType<SoundProcess>(session.PlayerProcesses.FromForm(soundForm));
        Assert.NotNull(session.PlayerProcesses.FindEvent(shield, ForcefieldLifecycle.FrameEvent));
        Assert.NotNull(session.PlayerProcesses.FindEvent(shield, ForcefieldLifecycle.CheckEvent));
        audio.SetFrame(9);
        session.Kernel.RunFrame(); Assert.Equal(4, session.Kernel.Size());
        session.ReleasePlayerProcesses(); Assert.Equal(0, session.Kernel.Size());
    }

    /// <summary>현재 boss는 같은 장기 주소를 읽고 아직 없는 피해·해방은 준비 완료로 오인하지 않는다.</summary>
    [Fact]
    public void CurrentAuthorityAndRemainingBoundariesAreExplicit()
    {
        var (session, _) = Scene(); RawPriestWorldBindings world = session.BuildPriestWorldDevices();
        Assert.Same(world, session.BuildPriestWorldDevices());
        Assert.True(session.PriestState.HitPoints.GetAuthority());
        session.Gameplay.Authority.Boss = false; Assert.False(session.PriestState.HitPoints.GetAuthority());
        Assert.False(session.PriestState.Carrier.ReadBoss());
        session.Gameplay.Authority.Server = false; Assert.False(session.Pool.IsServer);
        Assert.Throws<InvalidOperationException>(() => world.ValidateReady());
        session.ReleasePlayerProcesses();
    }
}
