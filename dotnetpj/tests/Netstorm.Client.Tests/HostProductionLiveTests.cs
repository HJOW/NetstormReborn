using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 창 없이 실제 생성/정렬/부모 통지·공유 SP/Deck·연구 운반과 반납을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>제품과 같은 실제 월드에 생산/연구 장치를 설치해 관측한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실제 장면의 외부 설정/UI/입력 경계와 요청 관측만 공급한다.</summary>
    private static CampaignProductionDevices ProductionDevices(Action<uint, ReadOnlyMemory<byte>>? send = null) => new()
    {
        ConfigInt = key => { Assert.Equal("getTeleportTechFree", key); return 0; },
        Gump = _ => throw new InvalidOperationException("검사에 없는 Gump 조회"), Handle = (_, _, _, _) => throw new InvalidOperationException("검사에 없는 Gump 처리"),
        Command = (_, _, _) => throw new InvalidOperationException("검사에 없는 UI 명령"), Log = (_, _) => { }, Send = send,
        ResearchHelp = (key, mode) => { Assert.Equal("researchHelp", key); Assert.Equal(1u, mode); },
    };
    /// <summary>실제 예측 내용물→로컬 큐→서버 수신→예측 삭제/서버 생성과 Deck/dirty 갱신을 한 경로로 실행한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductResearchPredictionAndServerReceiveUseSameContentsDeckAndDirty()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = UpgradeFactory(scene);
        Assert.Same(session.SidLists.Factories, session.TerrainBookkeeping.Factories);
        Assert.Same(session.SidLists.Providers, session.TerrainBookkeeping.Providers);
        Assert.Contains((uint)parent, session.TerrainBookkeeping.Factories.Items().ToArray());
        Assert.Equal(1u, session.MissionPlayer.TemplateOwner);
        session.Gameplay.Authority.Boss = true; using var wire = new LocalMissionTransport(); wire.Open("127.0.0.1", 6799); wire.AssignConnection(-1, 1);
        session.BuildProductionWorldDevices(ProductionDevices((channel, packet) => Assert.True(wire.Send((int)channel, packet.Span))));
        wire.RegisterDispatch(packet => { session.WorldResearch.Receive(1, packet.ToArray()); return true; });
        uint before = session.ProductionDirty; var finder = new RawCarrierPreDestroy(session.Pool, new(), new()
        {
            ValidateDamageable = (_, _) => throw new InvalidOperationException("조회 검사에서 삭제 준비"),
            DamageablePre = (_, _) => throw new InvalidOperationException("조회 검사에서 파괴"), DependentFound = () => throw new InvalidOperationException("조회 검사에서 파괴 후처리"),
        });
        Assert.Equal(1u, session.WorldResearch.Input(parent, 0x03000080, (71u << 16) | (uint)parent));
        int predicted = finder.FindContained(parent, 16, 71, 0); Assert.InRange(predicted, 5, 14999);
        Assert.Equal(before + 1, session.ProductionDirty); Assert.True(session.WorldProduction.Contains(71)); Assert.Equal(1, wire.QueuedFrames());
        Assert.Equal(1, wire.Poll()); int server = finder.FindContained(parent, 16, 71, 0);
        Assert.NotEqual(predicted, server); Assert.True(server >= 15000); Assert.Equal(1, session.Pool.Slot(predicted)[11] & 1);
        Assert.Contains(session.WorldPackets, packet => packet.Length > 6 && BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(4)) == server);
        Assert.Equal(before + 3, session.ProductionDirty); Assert.True(session.WorldProduction.Contains(71)); Assert.Equal(1, session.WorldProduction.AppendFactoryProducts(parent, 0));
        Assert.Same(session.Pool, session.WorldResearch.Pool); Assert.Same(session.WorldProductionDeck, session.WorldProduction.Deck);
        Assert.Same(session.TotalMade.LocalCounts, session.WorldProductionTotals.LocalCounts); Assert.Same(session.Gameplay.ProductionRestrictions, session.WorldProductionTotals.Restrictions);
        session.WorldProductionTotals.Allow(71, 0); session.WorldProduction.RefreshCurrent(); Assert.False(session.WorldProduction.Allowed(71)); Assert.True(session.WorldProduction.Contains(71));
        uint token = session.WorldProductionDeck.Get(1); Assert.NotEqual(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x68)));
        session.ReleasePlayerProcesses(); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x68)));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.Gameplay.ProductionDeckStorage.Bytes(token)); Assert.Throws<InvalidOperationException>(() => session.WorldResearch);
    }
    /// <summary>로딩 중 부착은 dirty/Deck 갱신을 억제하고 로딩 뒤 명시 재계산은 같은 실제 내용을 읽는다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductLoadingDepthSuppressesResearchRefreshUntilActualRecalculation()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = UpgradeFactory(scene);
        session.BuildProductionWorldDevices(ProductionDevices()); uint before = session.ProductionDirty;
        session.SetWorldLoadingDepth(1); session.WorldResearch.Learn(parent, 71, 16);
        Assert.Equal(before, session.ProductionDirty); Assert.False(session.WorldProduction.Contains(71));
        session.SetWorldLoadingDepth(0); session.WorldProduction.RefreshCurrent(); Assert.True(session.WorldProduction.Contains(71));
        Assert.Throws<InvalidOperationException>(() => session.BuildProductionWorldDevices(ProductionDevices()));
    }
    /// <summary>실제 기술 부모의 GumpForm을 현재 등록 표/사건으로 통지하고 동일 SP 접두·장부/역할을 공유한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductTechnologyUsesPlayerSpPrefixAndCurrentGumpNotification()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session;
        int parent = session.BuildPlayerWorldHooks().Create!(152, 2), gump = session.BuildPlayerWorldHooks().Create!(3, 2);
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x9c), (uint)parent);
        var forms = new RawSquidForms(session.Pool); forms.Attach(gump, parent, 0x110); BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(gump)[18..], 7);
        var handled = new List<(uint Token, uint Event, uint Form, uint Reason)>(); CampaignProductionDevices devices = ProductionDevices();
        devices.Gump = id => { Assert.Equal(7u, id); return 77; }; devices.Handle = (token, ev, form, reason) => handled.Add((token, ev, form, reason));
        session.BuildProductionWorldDevices(devices); session.Sp.Set(1, BitConverter.SingleToUInt32Bits(7000.5f));
        session.Gameplay.ContentMoneyChecksBox.Value = 1; session.Gameplay.PatchDiagnostic.Value = 99; session.Gameplay.PatchSample.Value = 23;
        session.WorldProduction.IndicateTechnology(71); var notification = Assert.Single(handled);
        Assert.Equal(77u, notification.Token); Assert.Equal(session.Gameplay.ContentEventBox.Value, notification.Event); Assert.Equal(1u, notification.Reason);
        Assert.Equal(71, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot((int)notification.Form)[18..]));
        Assert.Equal(0u, session.Gameplay.ContentMoneyChecksBox.Value); Assert.Equal(23u, session.Gameplay.PatchDiagnostic.Value);
        session.WorldProduction.IndicateTechnology(71); Assert.Single(handled);
        session.WorldProductionTotals.Allow(71, 0); Assert.Equal(1u, session.Gameplay.ProductionRestrictions[71]);
    }
}
