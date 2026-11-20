using Netstorm.Core.Original;

// 같은 미션의 Host·HP·난수·공통 표시 목록에 건설 표시를 부착한다.
namespace Netstorm.Client.Host;

/// <summary>실제 건설 표시와 외부 제품 장치를 연결하는 세션 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 공통 등록소를 빌리는 실제17번 건설 표시 조회기다.</summary>
    private ConstructionVisualSystem? _worldConstruction;
    /// <summary>현재 미션의 실제 건설 표시 시스템이다. 장치 연결 전 호출은 명시 실패한다.</summary>
    public ConstructionVisualSystem WorldConstruction => _worldConstruction ?? throw new InvalidOperationException("호스트 건설 표시 미연결");

    /// <summary>제품 캡처·색 표·화면 장치를 연소와 같은 Host/목록 및 실제 최대HP에 연결한다.</summary>
    public ConstructionVisualSystem BuildConstructionWorldDevices(ConstructionFrameState frame, ConstructionFramePalette palette,
        ConstructionVisualHooks visual, ConstructionFrameHooks hooks, Action<ConstructionRelease>? release = null)
    {
        ArgumentNullException.ThrowIfNull(frame); ArgumentNullException.ThrowIfNull(palette);
        ArgumentNullException.ThrowIfNull(visual); ArgumentNullException.ThrowIfNull(hooks);
        if (_worldConstruction is not null) throw new InvalidOperationException("호스트 실제 건설 표시 중복 연결");
        RenderProcessSystem rendering = WorldRendering;
        ConstructionFrameHooks actual = hooks.Copy(); actual.MaxHitPoints = Reward.MaxHitPoints;
        var construction = new ConstructionVisualSystem(rendering, frame, palette, Game, visual, actual, release ?? (static _ => { }));
        _worldConstruction = construction; return construction;
    }

    /// <summary>연소/공통 목록과 외부 팔레트보다 먼저 같은Form·Kernel 소유권을 반납한다.</summary>
    public void ReleaseConstructionWorld()
    {
        EnsureRuntimeCanRelease();
        ReleaseConstructionProgressWorld();
        _worldConstruction?.Dispose(); _worldConstruction = null;
        if (_worldRendering is not null) _worldRendering.State.Palette = 0;
    }
}
