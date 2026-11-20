using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 포획/전투/승리 없이 실제 미션의 건설 시작·HP·완료·공간·음향 수명을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>제품 자산과 같은 Host/Kernel/표시/음향을 쓰는 건설 진행 검사다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>완료 재등록을 검사할 서버 공장을 실제 지면/공간/소유자에 놓는다.</summary>
    private static int ProgressFactory(BurningContext scene)
    {
        HostSession session = scene.Session;
        session.BuildIslandPostPopDevices(scene.Types, session.WorldFrames);
        session.BuildTerrainBinding(scene.Types, session.TerrainDiagnostics, new()
        {
            SetResidenceOwner = session.SetFortObjectOwner,
            Assertion = text => throw new InvalidOperationException($"검사 섬 원본 진단: {text}"),
        });
        session.WireTerrainPostPopGraph(); session.BuildRegionOwnershipDevices(scene.Types);
        TerrainStampHooks stamp = session.BuildTerrainStampHooks(scene.Types);
        session.SetWorldLoadingDepth(1);
        // 실제 발자국 주위의 지면 도장은 원본 등록/점유 지도를 사용한다.
        for (int y = 57; y <= 62; y++)
        {
            // 각 지면은 독립 실제 자산이며 보호 파일은 수정하지 않는다.
            for (int x = 47; x <= 52; x++) { int ground = stamp.Create!(97, 1); stamp.Pop!(ground, x, y, 0x02000061); }
        }
        int parent = session.BuildPlayerWorldHooks().Create!(85, 0);
        session.BuildPlayerWorldHooks().SetOwner!(parent, 1); session.Pool.AllocatedBytes(parent)[40] |= 1;
        session.PopWorldObject(parent, 50, 60, 9); session.SetWorldLoadingDepth(0); RawHp(session, parent, 0);
        scene.Visuals.SetView(0, 0, new(0, 0, 1024, 768)); return parent;
    }

    /// <summary>같은Kernel의현재프로세스에서진행본체가소유한객체만찾는다.</summary>
    private static ConstructionProcess Progress(HostSession session)
    {
        // 현재PID순서로조회하고원본역할TYPE를임의클래스판정으로대체하지않는다.
        for (uint pid = 1; pid <= session.Kernel.Capacity(); pid++)
            if (session.Kernel.Get(pid) is ConstructionProcess process && process.BelongsTo(session.WorldConstructionProgress)) return process;
        throw new InvalidOperationException("검사 건설 진행 프로세스 없음");
    }

    /// <summary>지정/기본10초가실제17→30→47부착·HP·공간재등록·표시/진행음반납으로이어진다.</summary>
    [Theory, SupportedOSPlatform("windows")]
    [InlineData(0f)]
    [InlineData(8f)]
    public void ProductConstructionProgressCompletesRealFactory(float duration)
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = ProgressFactory(scene);
        session.ProcessTime.Value = 2; scene.Visuals.PrepareFrame(2, 1);
        session.WorldConstructionProgress.Start(parent, duration); ConstructionProcess progress = Progress(session);
        Assert.Equal(2, progress.Fields.Start); Assert.Equal(duration == 0 ? 12 : 10, progress.Fields.End);
        Assert.NotEqual(0u, progress.Fields.Sound); Assert.Equal(30, session.Pool.Slot(progress.Form())[10]);
        var lookup = new RawContainedLookup(session.Pool, (_, _) => throw new InvalidOperationException("조회가 삭제 요청"));
        int visual = lookup.Find(parent, 17), sound = lookup.Find(parent, 47); Assert.NotEqual(0, visual); Assert.NotEqual(0, sound);
        int maximum = session.Reward.MaxHitPoints(parent);
        scene.Visuals.PrepareFrame(6, 2); session.AdvanceMissionFrame(4);
        Assert.Equal((int)(4.0 / (duration == 0 ? 10 : duration) * maximum), BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(parent)[26..]));
        double end = progress.Fields.End; scene.Visuals.PrepareFrame(end, 3); session.AdvanceMissionFrame(end - 6);
        Assert.Equal(maximum, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(parent)[26..]));
        Assert.Equal(0, session.Pool.Slot(parent)[40] & 1); Assert.Equal(0, session.Pool.Slot(parent)[11] & 7);
        Assert.Equal(0, lookup.Find(parent, 30)); Assert.Equal(0, lookup.Find(parent, 47));
        session.Kernel.RunFrame(); Assert.Equal(0, lookup.Find(parent, 17)); Assert.Equal(0, session.WorldConstruction.Size);
        Assert.NotEmpty(scene.Visuals.Dirty.Entries);
    }

    /// <summary>시작뒤권한/즉시완료/프로세스역할을같은현재GameAux/세션시각에서읽는다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductConstructionProgressUsesCurrentAuthorityQuickAndRole()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = ProgressFactory(scene);
        session.GameAux.ConstructionProcessTypeBox.Value = 31; Assert.Equal(31u, session.WorldConstructionProgressState.Type());
        session.GameAux.ConstructionProcessTypeBox.Value = 30; session.WorldConstructionProgress.Start(parent, .5f);
        ConstructionProcess progress = Progress(session); Assert.Equal(0u, progress.Fields.Sound);
        session.Gameplay.Authority.Boss = false; scene.Visuals.PrepareFrame(.1, 1); session.AdvanceMissionFrame(.1);
        Assert.True(session.Pool.Slot(parent)[40] % 2 == 1); Assert.InRange(BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(parent)[26..]), 1, session.Reward.MaxHitPoints(parent) - 1);
        session.GameAux.QuickBuildBox.Value = 0x80000000; session.Kernel.RunFrame();
        Assert.Equal(session.Reward.MaxHitPoints(parent), BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(parent)[26..])); Assert.Same(progress, session.PlayerProcesses.FromForm(progress.Form()));
        session.Gameplay.Authority.Boss = true; session.Kernel.RunFrame(); Assert.Equal(0, session.Pool.Slot(parent)[40] & 1);
        session.Kernel.RunFrame(); Assert.Equal(0, session.WorldConstruction.Size);
    }

    /// <summary>같은진행/표시/진행음을실제Form삭제로먼저반납하고외부Kernel객체를보존한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductConstructionProgressReleasePreservesForeignKernel()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = ProgressFactory(scene);
        uint foreign = session.Kernel.Add(new BaseProcess()); session.WorldConstructionProgress.Start(parent, 8);
        int form = Progress(session).Form(); session.ReleaseConstructionWorld();
        Assert.NotNull(session.Kernel.Get(foreign)); Assert.Equal(1, session.Pool.Slot(form)[11] & 1);
        Assert.Throws<InvalidOperationException>(() => session.WorldConstructionProgress); Assert.Equal(0, session.WorldRendering.Size);
        session.Kernel.Remove(foreign);
    }

    /// <summary>현재사제이동TYPE·실제contained삭제·공간해제/재등록으로발자국의아군을옮긴다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductConstructionDisplacementMovesRealPriestAndClearsPath()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session;
        int priest = PlacePriest(session, scene.Types); TerrainStampHooks stamp = session.BuildTerrainStampHooks(scene.Types);
        int ground = stamp.Create!(97, 1); stamp.Pop!(ground, 50, 61, 0x02000061);
        uint pathType = session.PriestWorld.MovementProcessType(158);
        SquidProcess path = session.PlayerProcesses.Attach(new SquidProcess(), pathType, priest, 0x10) ?? throw new InvalidOperationException("검사 경로 부착 실패");
        int form = path.Form();
        Assert.True(session.DisplaceConstructionUnits(50, 60, 97, 1));
        Assert.Equal(50, BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(priest)[14..]));
        Assert.Equal(61, BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(priest)[18..]));
        Assert.Equal(1, session.Pool.Slot(form)[11] & 1); Assert.Null(session.PlayerProcesses.FromForm(form));
    }
}
