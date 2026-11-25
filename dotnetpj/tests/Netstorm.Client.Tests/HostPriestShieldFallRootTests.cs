using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 같은 Host의 사제 삭제·지면 소유자 결선과 보호막·낙하 음향 경계를 검증한다.
// 2026-11-26 1.039에서 WP-4c 사제 보호막/낙하 음향용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>사제 삭제·지면 소유자의 실제 결선과 외부 음향 경계를 같은 세션에서 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>제품 흐름과 같은 순서로 섬·지형·지역·사제·패킷 장치를 연결한 세션을 만든다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface) PriestShieldFallScene()
    {
        var (session, types, surface) = SquidPacketScene();
        session.BuildPriestWorldDevices();
        session.BuildIslandPostPopDevices(types, session.WorldFrames);
        session.BuildTerrainBinding(types, session.TerrainDiagnostics, new IslandRuntimeHooks { SetResidenceOwner = (_, _) => { }, Assertion = _ => { } });
        session.BuildRegionOwnershipDevices(types);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { FormPacket = _ => { } });
        return (session, types, surface);
    }

    /// <summary>사제 Pop의 지면 소유자 갱신이 같은 섬 받침에 도달하고 명시 실패가 없다.</summary>
    [Fact]
    public void PriestClaimGroundUsesIslandSupportOwner()
    {
        var (session, types, _) = PriestShieldFallScene();
        // 운반체 후처리의 지면 갱신 관문(Flags1 0x400)을 실제 사제 TYPE에 둔다.
        types[158].Flags1 |= 0x400;
        int priest = PlacePriest(session, types, 0, 9);
        session.SetWorldLoadingDepth(1);
        session.PopWorldObject(priest, 50, 60, 1);
        session.SetWorldLoadingDepth(0);
        Assert.Equal(0u, session.PostPopDepth);
        Assert.Contains(unchecked((uint)priest), session.SidLists.Priests.Items().ToArray());
        session.ReleasePlayerProcesses();
    }

    /// <summary>섬 받침 미설치 사제 Pop은 지면 소유자를 숨기지 않고 명시 실패한다.</summary>
    [Fact]
    public void PriestClaimGroundWithoutIslandPostPopFailsExplicitly()
    {
        var (session, types, surface) = SquidPacketScene();
        session.BuildPriestWorldDevices();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        types[158].Flags1 |= 0x400;
        session.SetWorldLoadingDepth(1);
        // 지면 갱신은 적재 Pop에서 바로 일어나므로 생성 Pop 자체가 명시 실패한다.
        var failure = Assert.Throws<InvalidOperationException>(() => PlacePriest(session, types, 0, 9));
        Assert.Contains("연결 통지", failure.Message);
        session.SetWorldLoadingDepth(0);
        session.ReleasePlayerProcesses();
    }

    /// <summary>사제 601 삭제가 같은 Host의 전체 가상 삭제 분배를 탄다.</summary>
    [Fact]
    public void PriestDestroyHookUsesFullDispatch()
    {
        var (session, types, surface) = SquidPacketScene();
        session.BuildPriestWorldDevices();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { Log = (_, _) => { } });
        int priest = PlacePriest(session, types, 0, 9);
        RegularHandler? handler = session.PlayerProcesses.Regular();
        Assert.NotNull(handler);
        // 전체 가상 삭제 미설치는 같은 분배 관문에서 드러나고 구 가상 삭제 문구로 돌아가지 않는다.
        var failure = Assert.Throws<InvalidOperationException>(() => handler(priest, 601, 1, 0f));
        Assert.Contains("가상 삭제 분배 미연결", failure.Message);
        Assert.DoesNotContain("실제 몸체 미연결(R6)", failure.Message);
        session.ReleasePlayerProcesses();
    }

    /// <summary>외부 Walker 낙하 훅 미연결은 실제 낙하 본체에서 명시 실패한다.</summary>
    [Fact]
    public void PriestOtherWalkerFallStaysExplicitWithoutExternalHook()
    {
        var (session, types, surface) = SquidPacketScene();
        session.BuildPriestWorldDevices();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        int parent = session.BuildPlayerWorldHooks().Create!(152, 2);
        int calls = 0;
        var observed = new ObservedFallProcess(_ => calls++);
        Assert.Same(observed, session.PlayerProcesses.Attach(observed, 10, parent, 0x50));
        int form = observed.Form();
        // 외부 훅 미연결은 사제 세션 기본값에서 명시 실패하고 R6 외부 몸체로 조용히 넘어가지 않는다.
        var failure = Assert.Throws<InvalidOperationException>(() => session.PriestWorld.FallWalker(form));
        Assert.Contains("다른 Walker 낙하", failure.Message);
        Assert.Equal(0, calls);
        session.ReleasePlayerProcesses();
    }

    /// <summary>실제 Form 가상 통지만 관찰하는 프로세스다.</summary>
    private sealed class ObservedFallProcess(Action<uint> destroyed) : SquidProcess
    {
        /// <summary>실제 가상 소멸 통지의 관측 장치다.</summary>
        private readonly Action<uint> _destroyed = destroyed;

        /// <summary>풀 반납은 실제 Host에 맡기고 원문 가상 통지 시점만 기록한다.</summary>
        /// <param name="flags">삭제 플래그다.</param>
        public override void OnFormDestroy(uint flags) => _destroyed(flags);
    }
}
