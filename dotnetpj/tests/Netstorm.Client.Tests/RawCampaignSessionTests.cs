using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 캠페인 세션 껍데기의 생성·선택·월드 위임을 검증하는 테스트 클래스다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.
public sealed class RawCampaignSessionTests
{
    /// <summary>188개 타입 목록을 만든다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();

    /// <summary>188개 프레임 목록을 만든다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private static List<RiftTypeFrames> Frames188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new RiftTypeFrames([])).ToList();

    /// <summary>188개 셰이프 목록을 만든다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private static List<SquidDisplayShape> Shapes188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();

    /// <summary>호스트·표시 조회까지 되는 전체 저장소를 만든다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    private static RawSurfaceStorage FullStorage(List<OriginalTypeRecord> types, List<RiftTypeFrames> frames, List<SquidDisplayShape> shapes) =>
        new(OriginalEdition.Patch1078, types, frames, shapes);

    /// <summary>빈·크기 불일치·이름 누락 입력을 거부하는지 검증한다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void CtorGuardsInputs()
    {
        // 원본 세션 factory의 필수 경계를 검증한다.
        var types = Types188();
        var frames = Frames188();
        var shapes = Shapes188();
        Assert.Throws<ArgumentNullException>(() => new RawCampaignSession(OriginalEdition.Patch1078, null!, frames, shapes, "TEST01", "1-1"));
        Assert.Throws<ArgumentException>(() => new RawCampaignSession(OriginalEdition.Patch1078, [], frames, shapes, "TEST01", "1-1"));
        Assert.Throws<ArgumentException>(() => new RawCampaignSession(OriginalEdition.Patch1078, types, Frames188().Take(10).ToList(), shapes, "TEST01", "1-1"));
        Assert.Throws<ArgumentException>(() => new RawCampaignSession(OriginalEdition.Patch1078, types, frames, Shapes188().Take(10).ToList(), "TEST01", "1-1"));
        Assert.Throws<ArgumentNullException>(() => new RawCampaignSession(OriginalEdition.Patch1078, types, frames, shapes, null!, "1-1"));
        Assert.Throws<ArgumentException>(() => new RawCampaignSession(OriginalEdition.Patch1078, types, frames, shapes, "", "1-1"));
        Assert.Throws<ArgumentNullException>(() => new RawCampaignSession(OriginalEdition.Patch1078, types, frames, shapes, "TEST01", null!));
    }

    /// <summary>받은 요새·미션·저장소·월드를 그대로 돌려주는지 검증한다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectorsReturnConstructionValues()
    {
        // 세션이 보관하는 이름·저장소·월드 정체성을 검증한다.
        var types = Types188();
        var frames = Frames188();
        var shapes = Shapes188();
        var storage = FullStorage(types, frames, shapes);
        var session = new RawCampaignSession(OriginalEdition.Patch1078, types, frames, shapes, "TEST01", "1-1", storage);
        Assert.Equal("TEST01", session.FortName());
        Assert.Equal("1-1", session.MissionName());
        Assert.Same(storage, session.Storage());
        Assert.NotNull(session.World());
        Assert.False(session.IsLoaded());
    }

    /// <summary>월드 보기가 세션 월드에 연결되는지 검증한다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void CoreDelegatesToWorld()
    {
        // 조립된 월드의 Core 보기가 세션을 통해 같은 참조로 보이는지 검증한다.
        var types = Types188();
        var frames = Frames188();
        var shapes = Shapes188();
        var session = new RawCampaignSession(OriginalEdition.Patch1078, types, frames, shapes, "TEST01", "1-1", FullStorage(types, frames, shapes));
        RawWorldCore core = session.Core();
        Assert.NotNull(core.Surfaces);
        Assert.NotNull(core.BridgeEvents);
        Assert.NotNull(core.Dependencies);
        Assert.NotNull(core.Forms);
    }

    /// <summary>Fort 적재 미연결을 명시 오류로 멈추는지 검증한다. 2026-11-10 F3 세션 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadRejectsAsUnconnected()
    {
        // 원본 Load의 Fort 적재 본체는 후속 단위에서 잇는다(결정 6).
        var types = Types188();
        var frames = Frames188();
        var shapes = Shapes188();
        var session = new RawCampaignSession(OriginalEdition.Patch1078, types, frames, shapes, "TEST01", "1-1", FullStorage(types, frames, shapes));
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.Load());
        Assert.Contains("미연결", error.Message);
        Assert.False(session.IsLoaded());
    }

    /// <summary>적재 가능한 프레임 목록을 만든다. 지면 94·무인도 157·다리 82에 코드 1개를 둔다. 2026-11-10 F3 Fort 씨앗 적재 이식에서 추가됐다.</summary>
    private static List<RiftTypeFrames> LoadableFrames()
    {
        // 월드 적재 검증의 프레임 범위 관문을 통과한다.
        var frames = Frames188();
        foreach (int type in new int[] { 82, 94, 157 })
        {
            frames[type] = new RiftTypeFrames([new RiftFrameCode((byte)'A', (byte)'P', 0, 0)]);
        }

        return frames;
    }

    /// <summary>적재 가능한 셰이프 목록을 만든다. 지면 94·무인도 157·다리 82에 그림 1개를 둔다. 2026-11-10 F3 Fort 씨앗 적재 이식에서 추가됐다.</summary>
    private static List<SquidDisplayShape> LoadableShapes()
    {
        // 월드 표시 수집의 셰이프 표 관문을 통과한다.
        var shapes = Shapes188();
        foreach (int type in new int[] { 82, 94, 157 })
        {
            shapes[type].FrameCount = 1;
            shapes[type].Loaded = true;
            shapes[type].Frames.Add(new SquidDisplayFrame { Width = 10, Height = 12, HotspotX = 2, HotspotY = 3, CellWidth = 8f, CellHeight = 6f });
        }

        return shapes;
    }

    /// <summary>적재 가능한 세션을 만든다. 2026-11-10 F3 Fort 씨앗 적재 이식에서 추가됐다.</summary>
    private static RawCampaignSession LoadableSession()
    {
        // 저장소 링크의 지면 94·무인도 157·다리 82와 같은 씨앗 종류를 쓴다.
        var types = Types188();
        var frames = LoadableFrames();
        var shapes = LoadableShapes();
        return new RawCampaignSession(OriginalEdition.Patch1078, types, frames, shapes, "TEST01", "1-1", FullStorage(types, frames, shapes));
    }

    /// <summary>세 종류 씨앗 목록을 만든다. 2026-11-10 F3 Fort 씨앗 적재 이식에서 추가됐다.</summary>
    private static List<SurfaceSeed> LoadableSeeds() =>
    [
        new SurfaceSeed(82u, 5u, 0, 20f, 20f),
        new SurfaceSeed(157u, 3u, 0, 10f, 10f),
        new SurfaceSeed(94u, 7u, 0, 30f, 30f),
    ];

    /// <summary>씨앗 적재가 월드에 놓고 세션을 적재됨으로 표시하는지 검증한다. 2026-11-10 F3 Fort 씨앗 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadSeedsPlacesSeedsAndMarksLoaded()
    {
        // Fort 순서의 지면·무인도·다리 씨앗 경로만 둔다.
        var session = LoadableSession();
        Assert.False(session.IsLoaded());
        session.LoadSeeds(LoadableSeeds());
        Assert.True(session.IsLoaded());
        List<SurfaceSnapshot> objects = session.World().Objects();
        Assert.Equal(3, objects.Count);
        SurfaceSnapshot bridge = Assert.Single(objects, snapshot => snapshot.Object.Type == 82u);
        Assert.Equal(5u, bridge.Object.Owner);
        SurfaceSnapshot noIsland = Assert.Single(objects, snapshot => snapshot.Object.Type == 157u);
        Assert.Equal(3u, noIsland.Object.Owner);
        SurfaceSnapshot terrain = Assert.Single(objects, snapshot => snapshot.Object.Type == 94u);
        Assert.Equal(7u, terrain.Object.Owner);
    }

    /// <summary>미지원 씨앗 타입을 거부하고 미적재로 남는지 검증한다. 2026-11-10 F3 Fort 씨앗 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadSeedsRejectsBadType()
    {
        // 월드 적재 검증의 지원 타입 관문을 그대로 빌린다.
        var session = LoadableSession();
        Assert.Throws<ArgumentException>(() => session.LoadSeeds([new SurfaceSeed(999u, 0u, 0, 10f, 10f)]));
        Assert.False(session.IsLoaded());
    }

    /// <summary>중복 씨앗 적재를 거부하는지 검증한다. 2026-11-10 F3 Fort 씨앗 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadSeedsRejectsDuplicate()
    {
        // 세션·월드 두 중복 관문 중 먼저 걸리는 쪽에서 멈춘다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.Throws<InvalidOperationException>(() => session.LoadSeeds(LoadableSeeds()));
        Assert.True(session.IsLoaded());
    }

    /// <summary>미적재 세션의 준비·실행을 거부하는지 검증한다. 2026-11-10 F3 세션 프레임 위임 이식에서 추가됐다.</summary>
    [Fact]
    public void FrameRejectsWhenNotLoaded()
    {
        // 적재 전 프레임 준비·실행은 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        Assert.Throws<InvalidOperationException>(() => session.PrepareFrame(new FrameTime { Game = 100.0 }, false));
        Assert.Throws<InvalidOperationException>(() => session.RunFrame(new FrameTime { Game = 100.0 }, false));
        Assert.Equal(0.0, session.CurrentTime().Game);
    }

    /// <summary>준비된 시각이 월드에 그대로 보이는지 검증한다. 2026-11-10 F3 세션 프레임 위임 이식에서 추가됐다.</summary>
    [Fact]
    public void PrepareFrameForwardsTime()
    {
        // 세션 준비의 월드 시각 대입만 둔다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        session.PrepareFrame(new FrameTime { Game = 100.0 }, false);
        Assert.Equal(100.0, session.CurrentTime().Game);
        Assert.Equal(100.0, session.World().CurrentTime().Game);
    }

    /// <summary>정지 실행이 시각만 전진시키는지 검증한다. 2026-11-10 F3 세션 프레임 위임 이식에서 추가됐다.</summary>
    [Fact]
    public void RunFramePausedAdvancesTimeOnly()
    {
        // 다리 스캔·월드 준비는 돌리고 커널은 건드리지 않는다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        session.RunFrame(new FrameTime { Game = 100.0, Delta = 1.0 }, true);
        Assert.Equal(100.0, session.CurrentTime().Game);
        session.RunFrame(new FrameTime { Game = 110.0, Delta = 10.0 }, false);
        Assert.Equal(110.0, session.CurrentTime().Game);
    }
}
