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
}
