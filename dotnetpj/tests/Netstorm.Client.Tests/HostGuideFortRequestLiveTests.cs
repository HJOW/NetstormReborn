using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 Guide 요청이 같은 실제 세션의 Subscriber·Template·설정을 사용하는지 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 실제 지형/사제/개발자 장치 장면의 동일 세션을 재사용한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>Fort 재선택 없이 현재 Guide/세션을 보존하고 정상 요청만 교체한다.</summary>
    [Fact]
    public void CurrentGuideRequestUsesActualSubscriberAndPublishesAfterSuccess()
    {
        var (session, _, surface) = DeveloperScene();
        var startup = new HostStartupScene(GameDataLocator.FindDataDirectory() ?? throw new DirectoryNotFoundException("보호 데이터 부재"));
        var fort = new ClientFortGlobals(); "before.fort\0"u8.CopyTo(fort.CurrentBytes());
        session.MissionPlayer.TemplateOwner = 1; session.OwnerMode.Fort = true;
        session.Templates.Install(1, new FortTemplate(), "same");
        // 실제 메모리 Template만 선택해 파일 저장을 요구하지 않고 같은 Subscriber stream을 사용한다.
        BinaryPrimitives.WriteUInt32LittleEndian(session.WorldTemplates.AsSpan(28), 1);
        "Before\0"u8.CopyTo(session.HandleBuffer); session.WorldGlobals.MySubscriber = 17;
        Action<string, byte[]> setConfig = HostSession.BuildSetConfigHook(startup.Configuration);
        session.BindDeveloperCommandDevices(new()
        {
            SetConfig = (key, bytes) => setConfig(key, bytes.ToArray()),
            RefreshTitle = () => "after.fort\0"u8.CopyTo(fort.CurrentBytes()), SubscriberRegistration = new(),
        });
        ClientLocalStateGlobals local = surface.Clock.LocalState;
        var globals = new MissionFortRequestGlobals(local.State, local.ZonePending, "zlocal,*\0"u8.ToArray(), () => 6799);
        uint[] restrictions = new uint[256]; HostTutorialInitialization? init = null;
        var participation = new PlayerParticipation(session.PlayerRecords, session.Alliances, new byte[9], new(), new());
        var guide = new GuideMissionOwner(startup.Configuration, new(), new(), participation, new(), new(), new(), new()
        {
            Created = _ => { }, ReleaseFortRequest = () => init?.ReleaseRequest(),
            SetAllowAll = value => Array.Fill(restrictions, value == 0 ? 1u : 0u, 0, 255),
        });
        init = new(guide, globals, new byte[87], restrictions,
            () => throw new InvalidOperationException("현재 Guide는 Template 전체를 지우지 않는다"),
            _ => throw new InvalidOperationException("현재 Guide는 Fort를 재선택하지 않는다"),
            _ => throw new InvalidOperationException("현재 Guide는 기술을 재초기화하지 않는다"),
            () => false, _ => throw new InvalidOperationException("요청 작성은 별도 지연 로드를 예약하지 않는다"));
        guide.Begin("Tutorial1"); GuideMissionRuntime runtime = guide.Runtime!;
        MissionFortLoadRequest first = init.RequestCurrentGuideFort(session, fort);
        Assert.Same(runtime, guide.Runtime); Assert.Same(first, init.Request);
        Assert.Equal("after.fort", first.Fort); Assert.Equal("Champion", session.Templates.ReadSubscriberName(1));
        Assert.Equal("Champion", startup.Configuration.PathSpec("fort.mySubHandle")); Assert.Equal(1u, session.WorldGlobals.MySubscriber);
        Assert.Equal(1u, local.ZonePending.Value); Assert.Equal(1u, first.Sequence); Assert.Equal(86, first.Zone.Length);
        Assert.Equal("Champion", Encoding.Latin1.GetString(first.Zone.AsSpan(24, 8)));
        Assert.Equal(7u, BinaryPrimitives.ReadUInt32LittleEndian(first.NextSnapshot));
        Assert.Throws<InvalidOperationException>(() => init.RequestCurrentGuideFort(session, fort));
        first.Consumed = true; MissionFortLoadRequest second = init.RequestCurrentGuideFort(session, fort);
        Assert.NotSame(first, second); Assert.Same(second, init.Request); Assert.Equal(2u, second.Sequence);
        second.Consumed = true; byte[] before = local.State.Next.ToArray();
        session.BindDeveloperCommandDevices(new() { SetConfig = (_, _) => throw new IOException("실제 설정 실패") });
        Assert.Throws<IOException>(() => init.RequestCurrentGuideFort(session, fort));
        Assert.Same(second, init.Request); Assert.Equal(2u, local.ZonePending.Value); Assert.Equal(before, local.State.Next.ToArray());
        guide.Release(); session.ReleasePlayerProcesses();
    }
}
