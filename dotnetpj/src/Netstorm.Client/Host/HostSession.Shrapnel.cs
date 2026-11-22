using Netstorm.Core.Original;

// 같은 월드의 공장·Pop·감시 쌍·전체 가상 삭제로 실제 파편을 조립한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession/RawCampaignParticles의 파편16을 같은 Host 수명에 연결한다.</summary>
public sealed partial class HostSession
{
    /// <summary>다른 효과와 같은 등록 목록/Kernel이 소유하는 현재 파편 시스템이다.</summary>
    private ShrapnelVisualSystem? _worldShrapnel;
    /// <summary>장치 설치/반납 전후를 구별하는 실제 파편 조회기다.</summary>
    public ShrapnelVisualSystem WorldShrapnel => _worldShrapnel ?? throw new InvalidOperationException("호스트 실제 파편 미연결");

    /// <summary>외부 캡처/표시만 빌리고 생성·공간·감시·반납은 같은 실제 월드 몸체로 고정한다.</summary>
    public ShrapnelVisualSystem BuildShrapnelWorldDevices(FieryTailFrameState frame, ShrapnelSystemHooks devices)
    {
        ArgumentNullException.ThrowIfNull(frame); ArgumentNullException.ThrowIfNull(devices);
        if (_worldShrapnel is not null) throw new InvalidOperationException("호스트 실제 파편 중복 설치");
        PlayerWorldHooks world = BindLivePlayerWorld(); RenderProcessSystem rendering = WorldRendering;
        RawDependFormHost dependencies = _worldDependencies ?? BuildDependencyWorldDevices();
        ShrapnelLifetimeHooks lifetime = devices.Lifetime.Copy();
        lifetime.Create = world.Create; lifetime.Pop = PopWorldObject;
        lifetime.Depend = (form, dummy) => { _ = dependencies.Create(form, dummy); };
        var actual = new ShrapnelSystemHooks { Lifetime = lifetime, Frame = devices.Frame.Copy(), Release = devices.Release, DestroyUnpaired = sid => DestroyWorldObject(sid, 0) };
        var system = new ShrapnelVisualSystem(rendering, Game, Crt, frame, actual); _worldShrapnel = system; return system;
    }

    /// <summary>같은 프레임에서 준비한 시각/카메라를 사용하고 실제 확보 실패만 원본 null로 바꾼다.</summary>
    public ShrapnelVisualProcess? AddWorldShrapnel(int source, ShrapnelKind kind)
    {
        ShrapnelVisualSystem system = WorldShrapnel;
        try { return system.Add(source, kind); }
        catch (OutOfMemoryException) { return null; }
    }
    /// <summary>공통 표시 목록과 빌린 장치가 남아 있을 때 같은 파편 Form을 먼저 반납한다.</summary>
    public void ReleaseShrapnelWorld()
    {
        EnsureRuntimeCanRelease(); _worldShrapnel?.Dispose(); _worldShrapnel = null;
    }
}
