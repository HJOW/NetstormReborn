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

    /// <summary>미적재 건설 시작을 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 세션 건설 시작 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void StartConstructionRejectsWhenNotLoaded()
    {
        // 적재 전 건설 시작은 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.StartConstruction(5, 0f));
        Assert.Contains("미적재", error.Message);
    }

    /// <summary>적재 뒤 건설 시작을 본체 미연결로 거부하는지 검증한다. 2026-11-10 F3 세션 건설 시작 Stub 이식에서 추가됐고 2026-11-10 F3 건설 시작 본체에서 묶기 전 관문으로 유지했다.</summary>
    [Fact]
    public void StartConstructionRejectsWhenUnconnected()
    {
        // 실제 빌더 효과를 묶기 전에는 Fort 본체 단위에서 잇는다(결정 6).
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.False(session.HasConstructionStart());
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.StartConstruction(5, 10f));
        Assert.Contains("미연결", error.Message);
    }

    /// <summary>묶은 실제 빌더 효과에 부모·기간을 그대로 넘기는지 검증한다. 2026-11-10 F3 건설 시작 본체에서 추가됐다.</summary>
    [Fact]
    public void StartConstructionForwardsToBoundDevice()
    {
        // 원본 세션 설치의 건설 몸체 연결만 둔다. 준비 시각 대입은 빌더 몸체 쪽에서 잇는다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        var calls = new List<(int Parent, float Duration)>();
        session.BindConstructionStart((parent, duration) => calls.Add((parent, duration)));
        Assert.True(session.HasConstructionStart());
        session.StartConstruction(5, 10f);
        session.StartConstruction(7, 0f);
        Assert.Equal([(5, 10f), (7, 0f)], calls);
    }

    /// <summary>원본 SID 폭을 벗어난 부모 번호를 거부하는지 검증한다. 2026-11-10 F3 건설 시작 본체에서 추가됐다.</summary>
    [Fact]
    public void StartConstructionRejectsParentOutOfSidRange()
    {
        // 원본 Sid의 uint16 폭을 그대로 둔다. 적재 관문보다 먼저 동작한다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        session.BindConstructionStart((_, _) => { });
        Assert.Throws<ArgumentOutOfRangeException>(() => session.StartConstruction(-1, 10f));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.StartConstruction(65536, 10f));
    }

    /// <summary>빈 빌더 묶기를 거부하는지 검증한다. 2026-11-10 F3 건설 시작 본체에서 추가됐다.</summary>
    [Fact]
    public void BindConstructionStartRejectsNull()
    {
        // 묶지 않은 세션은 미연결로 멈춘다.
        var session = LoadableSession();
        Assert.Throws<ArgumentNullException>(() => session.BindConstructionStart(null!));
        Assert.False(session.HasConstructionStart());
    }

    /// <summary>미적재 위치 명령을 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 세션 위치 명령 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void LocationCommandsRejectWhenNotLoaded()
    {
        // 적재 전 생성·삭제 명령은 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        Assert.Throws<ArgumentNullException>(() => session.CreateAt(null!));
        Assert.Throws<ArgumentNullException>(() => session.DestroyAt(null!));
        InvalidOperationException createError = Assert.Throws<InvalidOperationException>(() => session.CreateAt("70,10,10"));
        Assert.Contains("미적재", createError.Message);
        InvalidOperationException destroyError = Assert.Throws<InvalidOperationException>(() => session.DestroyAt("70,10,10"));
        Assert.Contains("미적재", destroyError.Message);
    }

    /// <summary>적재 뒤 위치 명령을 본체 미연결로 거부하는지 검증한다. 2026-11-10 F3 세션 위치 명령 Stub 이식에서 추가됐고 2026-11-10 F3 위치 명령 본체에서 묶기 전 관문으로 유지했다.</summary>
    [Fact]
    public void LocationCommandsRejectWhenUnconnected()
    {
        // 실제 파서·생성·삭제 본체는 Fort 본체·규칙 단위에서 잇는다(결정 6).
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.False(session.HasCreateAt());
        Assert.False(session.HasDestroyAt());
        InvalidOperationException createError = Assert.Throws<InvalidOperationException>(() => session.CreateAt("70,10,10"));
        Assert.Contains("미연결", createError.Message);
        InvalidOperationException destroyError = Assert.Throws<InvalidOperationException>(() => session.DestroyAt("70,10,10"));
        Assert.Contains("미연결", destroyError.Message);
    }

    /// <summary>묶은 생성·삭제 효과에 위치 문자열을 그대로 넘기는지 검증한다. 2026-11-10 F3 위치 명령 본체에서 추가됐다.</summary>
    [Fact]
    public void LocationCommandsForwardToBoundDevices()
    {
        // 원본 세션 설치의 생성·삭제 몸체 연결만 둔다. 파서·지면·Pop 본체는 Fort 본체·규칙 단위에서 잇는다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        var created = new List<string>();
        var destroyed = new List<string>();
        session.BindCreateAt(location => created.Add(location));
        session.BindDestroyAt(location => destroyed.Add(location));
        Assert.True(session.HasCreateAt());
        Assert.True(session.HasDestroyAt());
        session.CreateAt("70,10,10");
        session.DestroyAt("70,10,10");
        Assert.Equal(["70,10,10"], created);
        Assert.Equal(["70,10,10"], destroyed);
    }

    /// <summary>빈 위치 명령 묶기를 거부하는지 검증한다. 2026-11-10 F3 위치 명령 본체에서 추가됐다.</summary>
    [Fact]
    public void BindLocationCommandsRejectNull()
    {
        // 묶지 않은 세션은 미연결로 멈춘다.
        var session = LoadableSession();
        Assert.Throws<ArgumentNullException>(() => session.BindCreateAt(null!));
        Assert.Throws<ArgumentNullException>(() => session.BindDestroyAt(null!));
        Assert.False(session.HasCreateAt());
        Assert.False(session.HasDestroyAt());
    }

    /// <summary>미적재 소유자·전투 조회를 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 세션 소유자 조회 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void OwnerQueriesRejectWhenNotLoaded()
    {
        // 적재 전 소유자·전투 조회는 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        Assert.Contains("미적재", Assert.Throws<InvalidOperationException>(() => session.LocalOwner()).Message);
        Assert.Contains("미적재", Assert.Throws<InvalidOperationException>(() => session.TemplateOwner()).Message);
        Assert.Contains("미적재", Assert.Throws<InvalidOperationException>(() => session.BattleActive()).Message);
    }

    /// <summary>적재 뒤 소유자·전투 조회가 세션 Player·자산 상태를 읽는지 검증한다. 2026-11-10 F3 소유자 본체에서 추가됐다.</summary>
    [Fact]
    public void OwnerQueriesReadSessionState()
    {
        // 설치 본체가 갱신하기 전의 기본값은 battleOwner·templateOwner 1, 전투 끔이다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.Equal(1u, session.LocalOwner());
        Assert.Equal(1u, session.TemplateOwner());
        Assert.Equal(0u, session.BattleActive());
        // 설치 본체의 갱신이 조회에 그대로 보인다.
        session.PlayerState.BattleOwner = 3u;
        session.PlayerState.TemplateOwner = 5u;
        session.InBattle = true;
        Assert.Equal(3u, session.LocalOwner());
        Assert.Equal(5u, session.TemplateOwner());
        Assert.Equal(1u, session.BattleActive());
    }

    /// <summary>원본 mode0이 아닌 지형 갱신을 거부하는지 검증한다. 2026-11-10 F3 세션 지형 갱신 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void ReloadTheamRejectsBadMode()
    {
        // 원본 mode 검사는 적재 관문보다 먼저 동작한다.
        var session = LoadableSession();
        Assert.Throws<ArgumentException>(() => session.ReloadTheamAt(0u, 0u, 1u));
    }

    /// <summary>미적재 지형 갱신을 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 세션 지형 갱신 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void ReloadTheamRejectsWhenNotLoaded()
    {
        // 적재 전 지형 갱신은 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.ReloadTheamAt(0u, 0u));
        Assert.Contains("미적재", error.Message);
    }

    /// <summary>적재 뒤 지형 갱신을 본체 미연결로 거부하는지 검증한다. 2026-11-10 F3 세션 지형 갱신 Stub 이식에서 추가됐고 2026-11-10 F3 지형 갱신 본체에서 묶기 전 관문으로 유지했다.</summary>
    [Fact]
    public void ReloadTheamRejectsWhenUnconnected()
    {
        // 실제 ownership 투표·지형 갱신 본체는 Fort 본체 단위에서 잇는다(결정 6).
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.False(session.HasReloadTheam());
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.ReloadTheamAt(0u, 0u));
        Assert.Contains("미연결", error.Message);
    }

    /// <summary>묶은 ownership 효과에 float 비트 재해석 좌표를 그대로 넘기는지 검증한다. 2026-11-10 F3 지형 갱신 본체에서 추가됐다.</summary>
    [Fact]
    public void ReloadTheamForwardsBitsAsFloat()
    {
        // 원본처럼 uint 비트를 float으로 재해석한다. 묶기 전 관문은 그대로 둔다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        var calls = new List<(float X, float Y)>();
        session.BindReloadTheam((x, y) => calls.Add((x, y)));
        Assert.True(session.HasReloadTheam());
        session.ReloadTheamAt(0x3F800000u, 0x40000000u);
        Assert.Equal([(1f, 2f)], calls);
    }

    /// <summary>빈 지형 갱신 묶기를 거부하는지 검증한다. 2026-11-10 F3 지형 갱신 본체에서 추가됐다.</summary>
    [Fact]
    public void BindReloadTheamRejectsNull()
    {
        // 묶지 않은 세션은 미연결로 멈춘다.
        var session = LoadableSession();
        Assert.Throws<ArgumentNullException>(() => session.BindReloadTheam(null!));
        Assert.False(session.HasReloadTheam());
    }

    /// <summary>미적재 무작위 배치를 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 세션 무작위 배치 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void AddRandomObjectsRejectsWhenNotLoaded()
    {
        // 적재 전 무작위 배치는 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.AddRandomObjects(70u, 3u));
        Assert.Contains("미적재", error.Message);
    }

    /// <summary>적재 뒤 무작위 배치를 본체 미연결로 거부하는지 검증한다. 2026-11-10 F3 세션 무작위 배치 Stub 이식에서 추가됐고 2026-11-10 F3 나머지 본체에서 묶기 전 관문으로 유지했다.</summary>
    [Fact]
    public void AddRandomObjectsRejectsWhenUnconnected()
    {
        // 실제 범위·난수·완전 배치 본체는 Fort 본체 단위에서 잇는다(결정 6).
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.False(session.HasAddRandomObjects());
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.AddRandomObjects(70u, 3u));
        Assert.Contains("미연결", error.Message);
    }

    /// <summary>미적재 재장전 조회를 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 세션 게임메뉴 재장전 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void ReloadQueriesRejectWhenNotLoaded()
    {
        // 적재 전 재장전 개수·항목 조회는 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        Assert.Contains("미적재", Assert.Throws<InvalidOperationException>(() => session.GameMenuReloadCount(false)).Message);
        Assert.Contains("미적재", Assert.Throws<InvalidOperationException>(() => session.GameMenuReloadSid(false, 0u)).Message);
    }

    /// <summary>적재 뒤 재장전 조회가 세션 목록을 읽는지 검증한다. 2026-11-10 F3 재장전 본체에서 추가됐다.</summary>
    [Fact]
    public void ReloadQueriesReadSessionLists()
    {
        // Fort 적재 본체가 채우기 전의 두 목록은 비어 있다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.Equal(0, session.GameMenuReloadCount(false));
        Assert.Equal(0, session.GameMenuReloadCount(true));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.GameMenuReloadSid(false, 0u));
        // Fort 적재 본체의 채움이 조회에 그대로 보인다.
        session.Outposts.Entries = [11u, 23u];
        session.Outposts.Count = 2u;
        session.Temples.Entries = [37u];
        session.Temples.Count = 1u;
        Assert.Equal(2, session.GameMenuReloadCount(false));
        Assert.Equal(1, session.GameMenuReloadCount(true));
        Assert.Equal(11u, session.GameMenuReloadSid(false, 0u));
        Assert.Equal(23u, session.GameMenuReloadSid(false, 1u));
        Assert.Equal(37u, session.GameMenuReloadSid(true, 0u));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.GameMenuReloadSid(false, 2u));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.GameMenuReloadSid(true, 1u));
    }

    /// <summary>미적재 생산 동기화를 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 세션 생산 동기화 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void ProductionSyncRejectsWhenNotLoaded()
    {
        // 적재 전 생산 동기화는 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.SynchronizeProductionContents());
        Assert.Contains("미적재", error.Message);
    }

    /// <summary>적재 뒤 생산 동기화를 본체 미연결로 거부하는지 검증한다. 2026-11-10 F3 세션 생산 동기화 Stub 이식에서 추가됐고 2026-11-10 F3 나머지 본체에서 묶기 전 관문으로 유지했다.</summary>
    [Fact]
    public void ProductionSyncRejectsWhenUnconnected()
    {
        // 실제 Deck 동기화 본체는 Fort 본체 단위에서 잇는다(결정 6).
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.False(session.HasSynchronizeProductionContents());
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.SynchronizeProductionContents());
        Assert.Contains("미연결", error.Message);
    }

    /// <summary>알려지지 않은 별칭 주소를 거부하는지 검증한다. 2026-11-10 F3 세션 메타메뉴 별칭 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void MetaMenuTypeRejectsBadAddress()
    {
        // 원본 별칭 주소가 아니면 세션 상태와 무관하게 거부한다.
        var session = LoadableSession();
        Assert.Throws<ArgumentOutOfRangeException>(() => session.MetaMenuType(0x12345678u));
    }

    /// <summary>미적재 별칭 조회를 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 세션 메타메뉴 별칭 Stub 이식에서 추가됐다.</summary>
    [Fact]
    public void MetaMenuTypeRejectsWhenNotLoaded()
    {
        // 알려진 주소라도 적재 전에는 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.MetaMenuType(0x5412d0u));
        Assert.Contains("미적재", error.Message);
    }

    /// <summary>적재 뒤 별칭 조회를 본체 미연결로 거부하는지 검증한다. 2026-11-10 F3 세션 메타메뉴 별칭 Stub 이식에서 추가됐고 2026-11-10 F3 나머지 본체에서 묶기 전 관문으로 유지했다.</summary>
    [Fact]
    public void MetaMenuTypeRejectsWhenUnconnected()
    {
        // 실제 사제·장식·지형 TYPE 조회 본체는 Fort 본체 단위에서 잇는다(결정 6).
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.False(session.HasMetaMenuType());
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.MetaMenuType(0x5412d0u));
        Assert.Contains("미연결", error.Message);
    }

    /// <summary>묶은 배치·생산·별칭 효과에 인자를 그대로 넘기는지 검증한다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    [Fact]
    public void RemainingBodiesForwardToBoundDevices()
    {
        // 원본 세션 설치의 배치·생산·조회 몸체 연결만 둔다. 범위·Deck·TYPE 본체는 Fort 본체 단위에서 잇는다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        var placed = new List<(uint Type, uint Quantity)>();
        int synced = 0;
        session.BindAddRandomObjects((type, quantity) => placed.Add((type, quantity)));
        session.BindSynchronizeProductionContents(() => synced++);
        session.BindMetaMenuType(address => address ^ 0xFFu);
        Assert.True(session.HasAddRandomObjects());
        Assert.True(session.HasSynchronizeProductionContents());
        Assert.True(session.HasMetaMenuType());
        session.AddRandomObjects(70u, 3u);
        session.SynchronizeProductionContents();
        Assert.Equal([(70u, 3u)], placed);
        Assert.Equal(1, synced);
        Assert.Equal(0x5412d0u ^ 0xFFu, session.MetaMenuType(0x5412d0u));
    }

    /// <summary>빈 나머지 묶기를 거부하는지 검증한다. 2026-11-10 F3 나머지 본체에서 추가됐다.</summary>
    [Fact]
    public void BindRemainingRejectsNull()
    {
        // 묶지 않은 세션은 미연결로 멈춘다.
        var session = LoadableSession();
        Assert.Throws<ArgumentNullException>(() => session.BindAddRandomObjects(null!));
        Assert.Throws<ArgumentNullException>(() => session.BindSynchronizeProductionContents(null!));
        Assert.Throws<ArgumentNullException>(() => session.BindMetaMenuType(null!));
        Assert.False(session.HasAddRandomObjects());
        Assert.False(session.HasSynchronizeProductionContents());
        Assert.False(session.HasMetaMenuType());
    }

    /// <summary>보고서가 월드와 같은 첫 줄을 보이는지 검증한다. 2026-11-10 F3 세션 보고서 위임 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportDelegatesToWorld()
    {
        // 세션 보고서의 월드 위임만 둔다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        string sessionReport = session.Report();
        string worldReport = session.World().Report();
        Assert.Equal(worldReport.Split('\n')[0], sessionReport.Split('\n')[0]);
        Assert.Contains("raw_world", sessionReport);
    }

    /// <summary>미적재 보고서도 월드와 같은 줄을 보이는지 검증한다. 2026-11-10 F3 세션 보고서 위임 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportBeforeLoadMatchesWorld()
    {
        // 적재 전 빈 월드의 보고서 위임을 검증한다.
        var session = LoadableSession();
        Assert.Equal(session.World().Report(), session.Report());
    }

    /// <summary>새 세션의 섬 개수가 0인지 검증한다. 2026-11-10 F3 세션 섬 개수 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void IslandCountStartsAtZero()
    {
        // 현재 IslandList의 count DWORD 직접 읽기를 검증한다.
        var session = LoadableSession();
        Assert.Equal(0u, session.IslandCount());
    }

    /// <summary>씨앗 적재 뒤에도 섬 개수가 그대로인지 검증한다. 2026-11-10 F3 세션 섬 개수 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void IslandCountUnchangedBySeeds()
    {
        // 표면 씨앗 적재는 섬 목록을 바꾸지 않는다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.Equal(0u, session.IslandCount());
    }

    /// <summary>새 세션의 소유자별 섬 개수가 모두 0인지 검증한다. 2026-11-10 F3 세션 소유자별 섬 개수 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void OwnedIslandCountStartsAtZero()
    {
        // 원문대로 소유자 0도 비교하며 빈 목록에서는 모두 0이다.
        var session = LoadableSession();
        Assert.Equal(0u, session.OwnedIslandCount(0u));
        Assert.Equal(0u, session.OwnedIslandCount(1u));
        Assert.Equal(0u, session.OwnedIslandCount(8u));
    }

    /// <summary>씨앗 적재 뒤에도 소유자별 섬 개수가 그대로인지 검증한다. 2026-11-10 F3 세션 소유자별 섬 개수 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void OwnedIslandCountUnchangedBySeeds()
    {
        // 표면 씨앗 적재는 섬 목록을 바꾸지 않는다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.Equal(0u, session.OwnedIslandCount(0u));
        Assert.Equal(0u, session.OwnedIslandCount(7u));
    }

    /// <summary>새 세션의 AI 설정이 원본 기본값인지 검증한다. 2026-11-10 F3 AI 설정 소유자 본체에서 추가됐다.</summary>
    [Fact]
    public void AiSettingsStartWithOriginalDefaults()
    {
        // 원본 세션 Impl의 missionAiSettings 기본값과 같다.
        var session = LoadableSession();
        Assert.Equal(3, session.AiSettings.StuffRefreshRate);
        Assert.Equal(2500, session.AiSettings.StartingSp);
    }

    /// <summary>세션 AI 설정 갱신이 같은 참조에 보이는지 검증한다. 2026-11-10 F3 AI 설정 소유자 본체에서 추가됐다.</summary>
    [Fact]
    public void AiSettingsShareSameReference()
    {
        // 설치 본체의 갱신이 AI 생성·시작에 그대로 보인다.
        var session = LoadableSession();
        MissionAiSettingsState first = session.AiSettings;
        MissionAiSettingsState second = session.AiSettings;
        Assert.Same(first, second);
        first.StuffRefreshRate = 7;
        first.StartingSp = 1000;
        Assert.Equal(7, second.StuffRefreshRate);
        Assert.Equal(1000, second.StartingSp);
    }

    /// <summary>미적재 AI 생성을 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 AI 생성 본체에서 추가됐다.</summary>
    [Fact]
    public void CreateAiRejectsWhenNotLoaded()
    {
        // 적재 전 AI 생성은 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        session.BindAiCreate((owner, settings) => 1u);
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.CreateAi(1u, new byte[600]));
        Assert.Contains("미적재", error.Message);
    }

    /// <summary>원본 1..8 범위를 벗어난 소유자를 거부하는지 검증한다. 2026-11-10 F3 AI 생성 본체에서 추가됐다.</summary>
    [Fact]
    public void CreateAiRejectsBadOwner()
    {
        // 원본 세션의 owner 관문을 그대로 둔다. 적재 관문보다 먼저 동작한다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        session.BindAiCreate((owner, settings) => 1u);
        Assert.Throws<ArgumentOutOfRangeException>(() => session.CreateAi(0u, new byte[600]));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.CreateAi(9u, new byte[600]));
    }

    /// <summary>적재 뒤 AI 생성을 본체 미연결로 거부하는지 검증한다. 2026-11-10 F3 AI 생성 본체에서 추가됐다.</summary>
    [Fact]
    public void CreateAiRejectsWhenUnconnected()
    {
        // 실제 AI 생성 본체는 후속 단위에서 잇는다(결정 6).
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.False(session.HasAiCreate());
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.CreateAi(1u, new byte[600]));
        Assert.Contains("미연결", error.Message);
    }

    /// <summary>묶은 실제 AI 효과에 소유자·설정을 그대로 넘기는지 검증한다. 2026-11-10 F3 AI 생성 본체에서 추가됐다.</summary>
    [Fact]
    public void CreateAiForwardsToBoundDevice()
    {
        // 원본 세션 설치의 AI 몸체 연결만 둔다. 600바이트 검사는 AI 몸체 쪽에서 잇는다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        var calls = new List<(uint Owner, byte[] Settings)>();
        byte[] settings = new byte[600];
        session.BindAiCreate((owner, given) => { calls.Add((owner, given)); return 77u; });
        Assert.True(session.HasAiCreate());
        Assert.Equal(77u, session.CreateAi(3u, settings));
        Assert.Single(calls);
        Assert.Equal(3u, calls[0].Owner);
        Assert.Same(settings, calls[0].Settings);
    }

    /// <summary>빈 AI 생성 묶기를 거부하는지 검증한다. 2026-11-10 F3 AI 생성 본체에서 추가됐다.</summary>
    [Fact]
    public void BindAiCreateRejectsNull()
    {
        // 묶지 않은 세션은 미연결로 멈춘다.
        var session = LoadableSession();
        Assert.Throws<ArgumentNullException>(() => session.BindAiCreate(null!));
        Assert.False(session.HasAiCreate());
    }

    /// <summary>미적재 AI 준비를 적재 관문에서 거부하는지 검증한다. 2026-11-10 F3 AI 준비 본체에서 추가됐다.</summary>
    [Fact]
    public void PrepareAiRejectsWhenNotLoaded()
    {
        // 적재 전 AI 준비는 같은 세션 관문에서 멈춘다.
        var session = LoadableSession();
        session.BindAiPrepare(owner => true);
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.PrepareAi(1u));
        Assert.Contains("미적재", error.Message);
    }

    /// <summary>원본 1..8 범위를 벗어난 소유자를 거부하는지 검증한다. 2026-11-10 F3 AI 준비 본체에서 추가됐다.</summary>
    [Fact]
    public void PrepareAiRejectsBadOwner()
    {
        // 원본 세션의 owner 관문을 그대로 둔다. 적재 관문보다 먼저 동작한다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        session.BindAiPrepare(owner => true);
        Assert.Throws<ArgumentOutOfRangeException>(() => session.PrepareAi(0u));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.PrepareAi(9u));
    }

    /// <summary>적재 뒤 AI 준비를 본체 미연결로 거부하는지 검증한다. 2026-11-10 F3 AI 준비 본체에서 추가됐다.</summary>
    [Fact]
    public void PrepareAiRejectsWhenUnconnected()
    {
        // 실제 AI 준비 본체는 후속 단위에서 잇는다(결정 6).
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        Assert.False(session.HasAiPrepare());
        InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => session.PrepareAi(1u));
        Assert.Contains("미연결", error.Message);
    }

    /// <summary>묶은 실제 AI 효과의 준비 결과를 그대로 돌려주는지 검증한다. 2026-11-10 F3 AI 준비 본체에서 추가됐다.</summary>
    [Fact]
    public void PrepareAiForwardsToBoundDevice()
    {
        // 원본 세션 설치의 AI 준비 몸체 연결만 둔다. brain 없음 거짓 경로도 위임한다.
        var session = LoadableSession();
        session.LoadSeeds(LoadableSeeds());
        var calls = new List<uint>();
        session.BindAiPrepare(owner => { calls.Add(owner); return owner == 2u; });
        Assert.True(session.HasAiPrepare());
        Assert.True(session.PrepareAi(2u));
        Assert.False(session.PrepareAi(3u));
        Assert.Equal([2u, 3u], calls);
    }

    /// <summary>빈 AI 준비 묶기를 거부하는지 검증한다. 2026-11-10 F3 AI 준비 본체에서 추가됐다.</summary>
    [Fact]
    public void BindAiPrepareRejectsNull()
    {
        // 묶지 않은 세션은 미연결로 멈춘다.
        var session = LoadableSession();
        Assert.Throws<ArgumentNullException>(() => session.BindAiPrepare(null!));
        Assert.False(session.HasAiPrepare());
    }
}
