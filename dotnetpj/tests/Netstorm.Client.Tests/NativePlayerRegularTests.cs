using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

// 실제 장기 Player 정기의 현재 전역·원문 효과 순서·Template 대여를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>값 사본으로 놓치던 mode/owner 재조회와 실제 Host/Template 연결을 확인한다.</summary>
public sealed class NativePlayerRegularTests
{
    /// <summary>상태 설치만 있는 독립 Native 장기 저장소다. 미선택 큰 효과는 명시 실패한다.</summary>
    private static (RawSurfaceStorage Storage, PlayerWorldState Player, int Sid) Scene()
    {
        var storage = new RawSurfaceStorage(); var player = new PlayerWorldState { BattleOwner = 1, TemplateOwner = 2 };
        storage.BindTransmissionDevices(new() { Broadcast = _ => throw new IOException("미선택 방송"), Connected = _ => throw new IOException("미선택 연결"), Send = (_, _) => throw new IOException("미선택 송신") });
        storage.BindPlayerRegularState(player); int sid = storage.Pool.Allocate(SidAllocation.Client); var bytes = storage.Pool.AllocatedBytes(sid); bytes.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(bytes, 0x50ee08); bytes[10] = 152; bytes[34] = 1; return (storage, player, sid);
    }

    /// <summary>반환 메모리 쓰기와 변경 통지까지 유지되는 검사 수명이다.</summary>
    private sealed class Hold(Action release) : IDisposable
    {
        /// <summary>호출이 끝났을 때 단 한 번 실행할 반납 장치다.</summary>
        private Action? _release = release;
        /// <summary>정상/예외 어느 경로에서도 같은 대여를 한 번만 반납한다.</summary>
        public void Dispose() { Action? release = _release; _release = null; release?.Invoke(); }
    }

    /// <summary>Native 전송/최초 상태/대여 관문을 지키고 두 owner를 초기화하지 않는다.</summary>
    [Fact]
    public void StateInstallationPreservesCurrentOwnersAndRejectsLateOrRepeatedBinding()
    {
        var bare = new RawSurfaceStorage(); Assert.Throws<ArgumentException>(() => bare.BindPlayerRegularState(new()));
        var (storage, player, _) = Scene(); Assert.True(storage.UsesNativePlayerRegularState()); Assert.Equal(1u, player.BattleOwner); Assert.Equal(2u, player.TemplateOwner);
        Assert.Throws<ArgumentException>(() => storage.BindPlayerRegularState(new()));
        var late = new RawSurfaceStorage(); late.BindTransmissionDevices(new() { Broadcast = _ => { }, Connected = _ => false, Send = (_, _) => { } });
        using var use = late.AcquirePrepared(); Assert.Throws<ArgumentException>(() => late.BindPlayerRegularState(new())); Assert.False(late.UsesNativePlayerRegularState());
    }

    /// <summary>비로컬/count0은 로그·Template·Runtime 없이 원본 float 비트를 그대로 반환한다.</summary>
    [Fact]
    public void NaturalGatesReadCurrentLocalOwnerAndKeepPayloadBits()
    {
        var (storage, player, sid) = Scene(); float payload = BitConverter.UInt32BitsToSingle(0x7fc12345);
        Assert.Equal(0x7fc12345u, BitConverter.SingleToUInt32Bits(storage.PlayerRegular(sid, 2, 0, payload)));
        player.BattleOwner = 2; Assert.Equal(7.25f, storage.PlayerRegular(sid, 2, 1, 7.25f));
        player.BattleOwner = 1; storage.OwnerMode.Battle = true;
        Assert.Throws<InvalidOperationException>(() => storage.PlayerRegular(sid, 2, 1, 7.25f));
        BinaryPrimitives.WriteUInt32LittleEndian(storage.Pool.AllocatedBytes(sid), 0x50cc08);
        Assert.Throws<InvalidOperationException>(() => storage.PlayerRegular(sid, 1, 0, 7.25f));
    }

    /// <summary>첫 로그가 바꾼 현재 전투 모드를 다시 읽어 예약 대신 원본 대기 로그를 남긴다.</summary>
    [Fact]
    public void FirstEventRereadsBattleAfterActualLogCallback()
    {
        var (storage, _, sid) = Scene(); var logs = new List<string>(); storage.OwnerMode.Battle = true;
        storage.BindGraphTrace((group, text) => { Assert.Equal(29, group); logs.Add(text); if (logs.Count == 1) storage.OwnerMode.Battle = false; });
        Assert.Equal(5f, storage.PlayerRegular(sid, 1, 1, 5));
        Assert.Equal(new[] { "SORE LOSER TEST: ", "NOT STARTING YET.\n" }, logs);
        Assert.True(storage.ReservePlayerRegular()); Assert.Throws<InvalidOperationException>(() => storage.ReservePlayerRegular());
    }

    /// <summary>pin 뒤/State getter 뒤의 현재 Template owner를 각각 읽고 같은 반환 바이트에 bit4를 쓴다.</summary>
    [Fact]
    public void TemplatePinAndNotifyRereadCurrentOwnerAndRetainReturnedMemory()
    {
        var (storage, player, sid) = Scene(); storage.OwnerMode.Battle = true; byte[] bytes = new byte[28]; bool held = false; var trace = new List<string>();
        var runtime = new RawSurfacePlayerBindings
        {
            Pin = () => { held = true; player.TemplateOwner = 3; trace.Add("pin"); return new Hold(() => { held = false; trace.Add("release"); }); },
            State = owner => { Assert.True(held); Assert.Equal(3u, owner); player.TemplateOwner = 4; trace.Add("state"); return bytes; },
            Changed = owner => { Assert.True(held); Assert.Equal(4u, owner); Assert.Equal(4, bytes[0]); trace.Add("notify"); },
        };
        storage.BindPlayerRegularRuntime(runtime); Assert.Equal(0f, storage.PlayerRegular(sid, 2, 1, 60));
        Assert.False(held); Assert.Equal(new[] { "pin", "state", "notify", "release" }, trace);
        storage.ReleasePlayerRegularRuntime(runtime); Assert.Throws<InvalidOperationException>(() => storage.PlayerRegular(sid, 2, 1, 60));
    }

    /// <summary>State 쓰기 뒤 Notify 실패도 앞선 bit와 pin 반납을 보존한다. 새 후단을 옛 대여로 지우지 않는다.</summary>
    [Fact]
    public void NotifyFailurePreservesWrittenStateAndReleasesPin()
    {
        var (storage, _, sid) = Scene(); storage.OwnerMode.Battle = true; byte[] bytes = [0x20]; int releases = 0; var failure = new IOException("Notify 실패");
        var runtime = new RawSurfacePlayerBindings { Pin = () => new Hold(() => releases++), State = _ => bytes, Changed = _ => throw failure };
        storage.BindPlayerRegularRuntime(runtime); Assert.Throws<ArgumentException>(() => storage.BindPlayerRegularRuntime(new()));
        Assert.Same(failure, Assert.Throws<IOException>(() => storage.PlayerRegular(sid, 2, 1, 60))); Assert.Equal(0x24, bytes[0]); Assert.Equal(1, releases);
        storage.ReleasePlayerRegularRuntime(runtime); var next = new RawSurfacePlayerBindings { Pin = () => new Hold(() => releases++), State = _ => bytes, Changed = _ => { } }; storage.BindPlayerRegularRuntime(next);
        Assert.Throws<ArgumentException>(() => storage.ReleasePlayerRegularRuntime(runtime)); Assert.Equal(0f, storage.PlayerRegular(sid, 2, 1, 60)); Assert.Equal(2, releases); storage.ReleasePlayerRegularRuntime(next);
    }

    /// <summary>알 수 없는 사건은 같은 원문 식/파일/줄 진단 뒤 payload를 보존하며 Template를 읽지 않는다.</summary>
    [Fact]
    public void UnknownEventUsesActualCurrentAssertionWithoutTemplate()
    {
        var (storage, _, sid) = Scene(); int calls = 0;
        storage.BindDisplayDiagnostics(new(), (expression, file, line) => { Assert.Equal("0", expression); Assert.Equal(@"\Ns\O\Player.cpp", file); Assert.Equal(0x41du, line); calls++; });
        Assert.Equal(3.5f, storage.PlayerRegular(sid, 9, 1, 3.5f)); Assert.Equal(1, calls);
        Assert.Throws<InvalidOperationException>(() => storage.ConstructPlayerRegular(sid, 2, 60));
    }

    /// <summary>제품과 같은 공유 Surface/Host/현재 Template에 정기2 예약과 State 쓰기를 실제로 분배한다.</summary>
    [Fact]
    public void ActualSessionUsesLongPlayerBodyAndSameHostReservation()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var connections = new ClientConnectionGlobals(local); var templates = new CampaignTemplateStorage();
        var logs = new List<string>();
        var surface = new ClientSurfaceGlobals(game, new(local), new(), () => 1000, installDevices: owner =>
        {
            owner.Storage.BindSerializationGlobals(templates.SerializationGlobals); owner.BindNativeTransmission(connections, () => throw new IOException("미선택 선택 정리"));
            owner.Storage.BindGraphTrace((_, text) => logs.Add(text));
            owner.Storage.BindDisplayDevices(new() { View = () => new(), Suppressed = () => true, Invalidate = (_, _) => throw new IOException("미선택 dirty") });
        });
        var session = new HostSession(surface.Kernel, new(17), new(23), sharedSurface: surface, sharedTemplates: templates);
        var types = HostPostPopRuntimeTests.Build(session, surface.Storage, true); _ = session.BuildPlayerWorldHooks();
        session.BuildTerrainBinding(types, session.TerrainDiagnostics, new() { SetResidenceOwner = (_, _) => throw new IOException("미선택 거주 구역"), Assertion = _ => throw new IOException("미선택 지형 진단") });
        session.BuildRegionOwnershipDevices(types, () => 1u);
        var fort = new FortTemplate(); SetSection(fort, "State", new byte[28]); templates.Install(1, fort, "player-regular");
        BinaryPrimitives.WriteUInt32LittleEndian(templates.Headers.Span.Slice(28, 4), 1);
        session.MissionPlayer.BattleOwner = 1; session.MissionPlayer.TemplateOwner = 1; session.OwnerMode.Battle = true;
        int sid = session.Pool.Allocate(SidAllocation.Client); var bytes = session.Pool.AllocatedBytes(sid); bytes.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(bytes, 0x50ee08); bytes[10] = 152; bytes[34] = 1;
        session.ProcessTime.Value = 27.25;
        logs.Clear();
        using (var runtime = HostMissionRuntime.Create(session, surface.Storage))
        {
            RegularHandler handler = session.PlayerProcesses.Regular()!; Assert.Equal(0f, handler(sid, 1, 1, 5));
            RegularProcess regular = Assert.IsType<RegularProcess>(session.PlayerProcesses.FindEvent(sid, 2)); Assert.Equal(60f, regular.Payload()); Assert.Equal(27.25, regular.Time()); Assert.Equal(0u, regular.Count());
            Assert.Equal(new[] { "SORE LOSER TEST: ", "STARTING.\n" }, logs);
            Assert.Equal(0f, handler(sid, 2, 1, 60)); Assert.Equal(4, templates.State(1).Bytes.Span[0]); Assert.Equal(new uint[] { 1 }, session.PlayerTemplateMarks);
            Assert.False(game.RuntimeCallActive(session));
        }
        session.ReleasePlayerRecordViews(); Assert.Throws<InvalidOperationException>(() => surface.Storage.PlayerRegular(sid, 2, 1, 60)); templates.ClearAll();
    }
}
