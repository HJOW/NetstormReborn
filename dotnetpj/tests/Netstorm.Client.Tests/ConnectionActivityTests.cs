using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 활동표/간극DWORD와 현재callback순서를 실제 등록/직접수신으로 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>64h의typed/Native등록과장기실제활동주소를대조한다.</summary>
public sealed class ConnectionActivityTests
{
    /// <summary>실제 분배기의 필수 외부 장치를 제공한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new() { Now = () => 100, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
    /// <summary>같은22칸표와첫BSS/PE값을공유하고Session대여로초기화하지않는다.</summary>
    [Fact]
    public void ClientActivityOwnsTwoTablesAndPreservesGapFields()
    {
        var local = new ClientLocalStateGlobals(new(), new()); var connections = new ClientConnectionGlobals(local); ClientConnectionActivityGlobals owner = connections.Activity; Assert.Same(owner.Frames, owner.Tables.Frames); Assert.Same(owner.WallSeconds, owner.Tables.WallSeconds); Assert.Equal(6u, owner.AutoRestart.Value); Assert.Equal(0, owner.HeartbeatEnd.Value);
        owner.ActiveStamp.Value = 0x12345678; owner.ActiveZoneWord.Value = 0x87654321; owner.HeartbeatEnd.Value = 99.5; owner.WallSeconds[3] = 41;
        var body = new RawConnectionActivity(owner.Tables, new() { WallSeconds = () => 10.5, FrameNumber = () => 7, Server = () => true, EraseStatus = (kind, player) => { Assert.Equal(2u, kind); Assert.Equal(3u, player); } }); body.Receive(3);
        Assert.Equal(10u, owner.WallSeconds[3]); Assert.Equal(7u, owner.Frames[3]); Assert.Equal(0x12345678u, owner.ActiveStamp.Value); Assert.Equal(0x87654321u, owner.ActiveZoneWord.Value); Assert.Equal(99.5, owner.HeartbeatEnd.Value); Assert.Same(owner, connections.Activity);
    }
    /// <summary>wall→frame캡처→wall쓰기→현재server→frame쓰기→erase와복귀뒤현재값을보존한다.</summary>
    [Fact]
    public void CurrentOrderAndCapturedCallableValuesMatchCppBody()
    {
        var owner = new ClientConnectionActivityGlobals(); owner.Frames[3] = 88; owner.WallSeconds[3] = 77; var trace = new List<string>();
        var hooks = new ConnectionActivityHooks
        {
            WallSeconds = () => { trace.Add("wall"); return 4294967297.75; }, FrameNumber = () => { trace.Add("frame"); Assert.Equal(77u, owner.WallSeconds[3]); return 1234; },
            Server = () => { trace.Add("server"); Assert.Equal(1u, owner.WallSeconds[3]); Assert.Equal(88u, owner.Frames[3]); owner.Frames[3] = 99; return false; },
            EraseStatus = (kind, player) => { trace.Add("erase"); Assert.Equal(1u, kind); Assert.Equal(0u, player); Assert.Equal(1234u, owner.Frames[3]); owner.Frames[3] = 55; owner.WallSeconds[3] = 44; },
        };
        var body = new RawConnectionActivity(owner.Tables, hooks); hooks.Server = () => throw new InvalidOperationException("원래 함수객체값과 구별할 외부 재등록"); body.Receive(3); Assert.Equal(new[] { "wall", "frame", "server", "erase" }, trace); Assert.Equal(55u, owner.Frames[3]); Assert.Equal(44u, owner.WallSeconds[3]);
    }
    /// <summary>현재쓰기뒤의erase실패와실제인덱스거부의서로다른부분효과를검증한다.</summary>
    [Fact]
    public void FailureKeepsBothStoresAndInvalidIndexDoesNotCallClock()
    {
        var owner = new ClientConnectionActivityGlobals(); int clocks = 0; var failure = new InvalidOperationException("실제 안내 제거 실패"); var body = new RawConnectionActivity(owner.Tables, new() { WallSeconds = () => { clocks++; return -1.75; }, FrameNumber = () => 0x12345678, Server = () => true, EraseStatus = (_, _) => throw failure });
        Assert.Same(failure, Assert.Throws<InvalidOperationException>(() => body.Receive(21))); Assert.Equal(uint.MaxValue, owner.WallSeconds[21]); Assert.Equal(0x12345678u, owner.Frames[21]); Assert.Throws<ArgumentOutOfRangeException>(() => body.Receive(-1)); Assert.Throws<ArgumentOutOfRangeException>(() => body.Receive(22)); Assert.Equal(1, clocks);
    }
    /// <summary>typed등록이현재표를쓰고Native수신은같은고정F와metadata를대여한다.</summary>
    [Fact]
    public void RegistrationUsesActualTypedBodyOrSameNativeValue()
    {
        var owner = new ClientConnectionActivityGlobals(); var body = new RawConnectionActivity(owner.Tables, new() { WallSeconds = () => 13, FrameNumber = () => 29, Server = () => false, EraseStatus = (_, _) => { } }); RawPacketDispatch dispatch = Dispatch(); ConnectionActivityRegistration.Register(dispatch, body); Assert.Equal(1, dispatch.Registration(0x64).Version); Assert.Equal(5, dispatch.Registration(0x64).Length); Assert.NotNull(dispatch.Registration(0x64).Observer);
        dispatch.InvokeDirect(0x64, 3, new(), ReadOnlyMemory<byte>.Empty); Assert.Equal(13u, owner.WallSeconds[3]); Assert.Equal(29u, owner.Frames[3]);
        int native = 0; dispatch.RegisterCanonicalDirect(0x64, 0x004b6180, (sender, connection, packet) => { Assert.Null(connection); Assert.True(packet.IsEmpty); native++; body.Receive(unchecked((int)sender)); }); CanonicalPacketReceiver receiver = dispatch.Registration(0x64).CanonicalReceive!;
        using var lease = dispatch.Registrations.BindSession(owner); ConnectionActivityRegistration.Register(dispatch, body); Assert.Same(receiver, dispatch.Registration(0x64).CanonicalReceive); dispatch.InvokeDirect(0x64, 4, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(1, native); Assert.Equal(13u, owner.WallSeconds[4]); dispatch.Version(0x64, 2, 0); Assert.Throws<InvalidOperationException>(() => ConnectionActivityRegistration.Register(dispatch, body)); Assert.Same(receiver, dispatch.Registration(0x64).CanonicalReceive);
    }
}
