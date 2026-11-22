using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 Player 재할당과 전체 Runtime 반납을 장기 AI 지갑/통지에서 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 장기 owner와 선택된 실제 Session 메모리·pin의 계약이다.</summary>
public sealed class ClientAiSurfaceDevicesTests
{
    /// <summary>같은 장기 Template 깊이와 실제 Client 시계/Surface를 첫 Host 전에 준비한다.</summary>
    private static (ClientSurfaceGlobals Surface, CampaignTemplateStorage Templates) Scene()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var templates = new CampaignTemplateStorage();
        var surface = new ClientSurfaceGlobals(game, new(local), new(), static () => 1000, installDevices: owner => owner.Storage.BindSerializationGlobals(templates.SerializationGlobals));
        return (surface, templates);
    }
    /// <summary>같은 실제 물리/Player owner를 대여하고 별도 brain 메모리를 만들지 않는 Host다.</summary>
    private static HostSession Session(ClientSurfaceGlobals surface, CampaignTemplateStorage templates) => new(surface.Kernel, new(17), new(23), sharedSurface: surface, sharedTemplates: templates);
    /// <summary>실제 Player/본체의 한 DWORD만 테스트 입력으로 쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[at..], value);
    /// <summary>실제 본체의 signed 지갑을 읽는다.</summary>
    private static int Wallet(HostSession session, uint token) => BinaryPrimitives.ReadInt32LittleEndian(session.AiBrainMemory.Bytes(token).AsSpan(0x2d8));

    /// <summary>미초기화 메뉴에서도 비활성/비양수 관문은 Session을 요구하지 않고 NULL brain도 자연 반환한다.</summary>
    [Fact]
    public void FirstMenuKeepsNaturalGatesAndDoesNotSynthesizeRuntime()
    {
        var (surface, templates) = Scene(); var game = surface.Gameplay;
        var wallet = surface.CreateAiWallet(() => throw new IOException("빈 메뉴에서 데모 선택 금지"));
        Assert.False(game.PlayerRecords.Initialized); surface.Storage.OwnerMode.Challenge = true;
        wallet.Credit(1, 5); Assert.Equal(0, wallet.Balance(1)); surface.NotifyAiObject(1, int.MaxValue, true);
        surface.Storage.OwnerMode.Challenge = false; wallet.Credit(0, 5); Assert.Equal(0, wallet.Balance(0xffffffff)); surface.NotifyAiObject(0, int.MaxValue, true);
        Assert.Throws<ArgumentOutOfRangeException>(() => wallet.Credit(1, 5)); Assert.Throws<ArgumentOutOfRangeException>(() => surface.NotifyAiObject(1, int.MaxValue, true));
        var session = Session(surface, templates); Assert.Same(game.PlayerRecords, session.PlayerRecordsOwner); Assert.True(game.PlayerRecords.Initialized);
        wallet.Credit(1, 5); Assert.Equal(0, wallet.Balance(1)); surface.NotifyAiObject(1, int.MaxValue, true);
        Put(session.PlayerRecords, 180+0xb0, 0xdeadbeef);
        Assert.Throws<InvalidOperationException>(() => wallet.Credit(1, 5)); Assert.Throws<InvalidOperationException>(() => surface.NotifyAiObject(1, 1, true));
        Put(session.PlayerRecords, 180+0xb0, 0); session.ReleasePlayerRecordViews(); Assert.False(game.Borrowed());
    }

    /// <summary>데모 중 종료 요청은 저장된 같은 brain의 마지막 재조회 뒤 반납하고 다음 Session의 현재 base를 사용한다.</summary>
    [Fact]
    public void CreditPinsActualRuntimeThroughSavedBrainRereadAndReborrow()
    {
        var (surface, templates) = Scene(); var first = Session(surface, templates); uint a = first.AiBrainMemory.Allocate(0xb08), b = first.AiBrainMemory.Allocate(0xb08);
        Put(first.PlayerRecords, 180+0xb0, a); Put(first.AiBrainMemory.Bytes(a), 0x2d8, 7); Put(first.AiBrainMemory.Bytes(b), 0x2d8, 5000);
        var runtime = HostMissionRuntime.Create(first, surface.Storage, []); Func<bool> inspect = () =>
        {
            Assert.True(surface.Gameplay.RuntimeCallActive(first)); Assert.Equal(8, Wallet(first, a)); runtime.Dispose(); Assert.False(runtime.Released);
            Put(first.PlayerRecords, 180+0xb0, b); Put(first.AiBrainMemory.Bytes(a), 0x2d8, 0x80000001); return true;
        };
        var wallet = surface.CreateAiWallet(() => inspect()); wallet.Credit(1, 1);
        Assert.Equal(1190, Wallet(first, a)); Assert.Equal(5000, Wallet(first, b)); Assert.True(runtime.Released); Assert.False(surface.Gameplay.Borrowed());
        Assert.Throws<InvalidOperationException>(() => wallet.Balance(1)); Put(first.PlayerRecords, 180+0xb0, 0); first.ReleasePlayerRecordViews();
        var next = Session(surface, templates); uint token = next.AiBrainMemory.Allocate(0xb08); Put(next.PlayerRecords, 180+0xb0, token); Put(next.AiBrainMemory.Bytes(token), 0x2d8, 77);
        using (var current = HostMissionRuntime.Create(next, surface.Storage, []))
        {
            inspect = () => false; Assert.Equal(77, wallet.Balance(1)); wallet.Credit(1, 5); Assert.Equal(82, Wallet(next, token)); Assert.Equal(1190, Wallet(first, a));
        }
        Put(next.PlayerRecords, 180+0xb0, 0); next.ReleasePlayerRecordViews();
    }

    /// <summary>실제 Runtime 안의 통지 하위 미설치와 데모 예외도 pin을 해제하고 앞선 가산을 보존한다.</summary>
    [Fact]
    public void SelectedFailuresReleaseActualRuntimeAndKeepWalletPrefix()
    {
        var (surface, templates) = Scene(); var session = Session(surface, templates); uint token = session.AiBrainMemory.Allocate(0xb08); Put(session.PlayerRecords, 180+0xb0, token); Put(session.AiBrainMemory.Bytes(token), 0x2d8, 7);
        var runtime = HostMissionRuntime.Create(session, surface.Storage, []);
        Assert.Contains("실제 AI 객체 통지 미설치", Assert.Throws<InvalidOperationException>(() => surface.NotifyAiObject(1, 1, true)).Message); Assert.False(surface.Gameplay.RuntimeCallActive(session));
        var failure = new IOException("실제 현재 데모 실패"); var wallet = surface.CreateAiWallet(() => { runtime.Dispose(); Assert.False(runtime.Released); throw failure; });
        Assert.Same(failure, Assert.Throws<IOException>(() => wallet.Credit(1, 1))); Assert.Equal(8, Wallet(session, token)); Assert.True(runtime.Released); Assert.False(surface.Gameplay.RuntimeCallActive(session));
        Put(session.PlayerRecords, 180+0xb0, 0); session.ReleasePlayerRecordViews();
    }

    /// <summary>실제 자산/Host의 기존 통지 본체로 같은 brain의 타입 카운터를 갱신하며 legacy 부착 bool을 읽지 않는다.</summary>
    [Fact]
    public void ObjectNoticeUsesActualHostBodyAndCurrentPlayerBrain()
    {
        var (surface, templates) = Scene(); var session = Session(surface, templates);
        var types = HostPostPopRuntimeTests.Build(session, surface.Storage, initializeAssetTables: true); _ = session.BuildPlayerWorldHooks();
        session.BuildTerrainBinding(types, session.TerrainDiagnostics, new() { SetResidenceOwner = (_, _) => throw new IOException("통지에서 거주 효과 금지"), Assertion = _ => throw new IOException("통지에서 지형 진단 금지") });
        session.BuildRegionOwnershipDevices(types, () => 1u);
        uint token = session.AiBrainMemory.Allocate(0xb08); Put(session.PlayerRecords, 180+0xb0, token);
        int sid = session.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client); session.Pool.AllocatedBytes(sid).Clear();
        using (var runtime = HostMissionRuntime.Create(session, surface.Storage, []))
        {
            Assert.False(session.TerrainBookkeeping.AiAttached[1]); surface.NotifyAiObject(1, sid, true);
            Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(token).AsSpan(0x6d4))); Assert.False(surface.Gameplay.RuntimeCallActive(session));
            surface.NotifyAiObject(1, sid, false); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(token).AsSpan(0x6d4)));
        }
        Put(session.PlayerRecords, 180+0xb0, 0); session.ReleasePlayerRecordViews();
    }

    /// <summary>같은 Surface에 다른 기록 owner를 명시해도 실제 Player 초기화 전에 거부한다.</summary>
    [Fact]
    public void SharedSurfaceRejectsDifferentPlayerRecordsBeforeInitialization()
    {
        var (surface, templates) = Scene(); var other = new Netstorm.Client.Player.ClientPlayerRecords();
        Assert.Throws<ArgumentException>(() => new HostSession(surface.Kernel, new(17), new(23), sharedSurface: surface, sharedTemplates: templates, sharedPlayerRecords: other));
        Assert.False(surface.Gameplay.PlayerRecords.Initialized); Assert.False(other.Initialized); Assert.True(surface.Gameplay.PlayerRecords.Records.IsEmpty);
    }
}
