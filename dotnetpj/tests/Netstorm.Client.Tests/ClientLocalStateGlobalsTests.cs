using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 공유 owner와 물리 별칭을 기존 소비자에서 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>State/Zone/주변음/저장모드/연결의 같은 물리와 두 HostSession 수명을 확인한다.</summary>
public sealed class ClientLocalStateGlobalsTests
{
    /// <summary>같은 실제 Gameplay/주변음을 새 Client 전역에 명시 연결한다.</summary>
    private static ClientLocalStateGlobals Local() => new(new(), new());
    /// <summary>현재 실제 블록의 지정DWORD를 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int at) => BinaryPrimitives.ReadUInt32LittleEndian(bytes[at..]);

    /// <summary>새 Client에서만 최초 원문 값과 서로 다른 같은720바이트를 구성한다.</summary>
    [Fact]
    public void InitialValuesAndActualMenuWindowStateAliasArePreserved()
    {
        ClientLocalStateGlobals local = Local(); Assert.Equal(1u, local.FirstMenuPending.Value); Assert.Equal(1u, local.TellTips.Value); Assert.Equal(1u, local.TipNumber.Value); Assert.Equal(1u, local.ScriptBreak.Value); Assert.Equal(1u, local.MetaFirstReset.Value);
        Assert.Equal(uint.MaxValue, local.RestoreOwner.Value); Assert.Equal(uint.MaxValue, local.RestoreSubscriber.Value); Assert.Equal(uint.MaxValue, local.CachedPeerIndex.Value); Assert.Equal(uint.MaxValue, local.CachedPeerValue.Value); Assert.Equal(uint.MaxValue, local.CachedPeerFieldA4.Value); Assert.Equal(uint.MaxValue, local.CachedPeerFieldA8.Value);
        Assert.Equal(0x0200002bu, local.JoinTestBattleCommand.Value); Assert.Equal(0x02000013u, local.ViewMetaCommand.Value); Assert.Equal(0x0200003du, local.HudModeCommand.Value); Assert.Equal(0x02000022u, local.AddIslandCommand.Value); Assert.Equal(127, local.InvalidTerritory.Value);
        Memory<byte>[] blocks = [local.State.Old, local.State.Phase, local.State.Immediate, local.State.Current, local.State.Next];
        // 최초 BSS 전체를 확인하고 각 블록에 서로 다른 인접 입력을 쓴다.
        for (int index = 0; index < blocks.Length; index++) { Assert.Equal(144, blocks[index].Length); Assert.DoesNotContain(blocks[index].ToArray(), value => value != 0); blocks[index].Span[8] = (byte)(index + 1); }
        // 서로 다른 다섯 블록의 실제 주소가 중첩되지 않아야 한다.
        for (int index = 0; index < blocks.Length; index++) Assert.Equal((byte)(index + 1), blocks[index].Span[8]);
        local.MenuWindow.Value = 0xabcdef01; Assert.Equal(0xabcdef01u, Read(local.State.Immediate.Span, 132)); BinaryPrimitives.WriteUInt32LittleEndian(local.State.Immediate.Span[132..], 0x87654321); Assert.Equal(0x87654321u, local.MenuWindow.Value);
        var body = new RawLocalMissionState(OriginalEdition.Patch1078, local.State, new() { MenuWindow = local.MenuWindow }, new()); body.Request(8); Assert.Equal(8u, Read(local.State.Immediate.Span, 0));
    }

    /// <summary>Zone6/8과BattleBYTE/DWORD가 같은 물리이며 인접상위바이트를 보존한다.</summary>
    [Fact]
    public void ZoneAndBattleByteViewsPreserveNeighbourBytes()
    {
        ClientLocalStateGlobals local = Local(); Assert.Equal(8, local.ZoneStorage.Length); Assert.Equal(6, local.Zone.Length); local.ZoneStorage.Span.Fill(0xaa); local.Zone.Span.Clear(); Assert.Equal(new byte[] { 0, 0, 0, 0, 0, 0, 0xaa, 0xaa }, local.ZoneStorage.ToArray());
        local.BattleTerritory.Value = 0xa1b2c3d4; Assert.Equal((byte)0xd4, local.BattleByte); local.BattleByte = 0x77; Assert.Equal(0xa1b2c377u, local.BattleTerritory.Value);
    }

    /// <summary>바인딩은 기존 저장모드/주변음값을 지우지 않고 실제 양방향 쓰기를 공유한다.</summary>
    [Fact]
    public void ExistingSavedGameAndAmbientDeadlineAreBorrowedWithoutReset()
    {
        var gameplay = new ClientGameplayGlobals(); gameplay.SavedGameBox.Value = true; var ambient = new AmbientSoundState { Deadline = 77, Kind = 2, Forced = 3 }; var local = new ClientLocalStateGlobals(gameplay, ambient);
        Assert.Same(gameplay.SavedGameBox, local.SavedGame); Assert.True(local.SavedGame.Value); Assert.Equal(77, local.ActivityEnd.Value); RawAmbientSound.ResetDeadline(ambient, () => 100.5); Assert.Equal(110.5, local.ActivityEnd.Value); local.ActivityEnd.Value = 231.25; Assert.Equal(231.25, ambient.Deadline); Assert.Equal(2, ambient.Kind); Assert.Equal(3, ambient.Forced); local.SavedGame.Value = false; Assert.False(gameplay.SavedGameBox.Value);
    }

    /// <summary>두 실제 HostSession이 같은장기owner를 빌리며 생성때값을 초기화하지 않는다.</summary>
    [Fact]
    public void TwoActualHostSessionsKeepOneLocalOwnerAndFlareDeadline()
    {
        ClientLocalStateGlobals local = Local(); var connections = new ClientConnectionGlobals(local); local.SourceType.Value = 0x76543210; local.GameTicks.Value = 0x12345678; local.LoginEnd.Value = 137.25; local.State.Current.Span.Fill(0xa5);
        var first = new HostSession(new Kernel(), new GameRandom(31), new CrtRandom(), sharedGameplay: local.Gameplay, sharedLocal: local, sharedConnections: connections);
        var second = new HostSession(new Kernel(), new GameRandom(37), new CrtRandom(), sharedGameplay: local.Gameplay, sharedLocal: local, sharedConnections: connections);
        Assert.Same(local, first.LocalState); Assert.Same(local, second.LocalState); Assert.Same(connections, first.Connections); Assert.Same(connections, second.Connections); Assert.Same(local.LoginEnd, first.WorldLoginEnd); Assert.Same(first.WorldLoginEnd, second.WorldLoginEnd); second.WorldLoginEnd.Value = 245.5; Assert.Equal(245.5, local.LoginEnd.Value); Assert.Equal(0x76543210u, local.SourceType.Value); Assert.Equal(0x12345678u, local.GameTicks.Value); Assert.All(local.State.Current.ToArray(), value => Assert.Equal((byte)0xa5, value));
    }

    /// <summary>같은 동기화DWORD를 실제 내부Client9 종료가 비우고Source최종정리도같은장부를쓴다.</summary>
    [Fact]
    public void ActualConnectionCloseSharesSynchronizationAndShutdownKeepsLocalStorage()
    {
        ClientLocalStateGlobals local = Local(); var connections = new ClientConnectionGlobals(local); byte[] players = new byte[1620]; uint now = 100;
        var source = new CompositeMissionTransport(CompositeTransportRole.InternalMission, players, () => true, new() { Now = () => ++now, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } }, connections.Registry, local);
        connections.PacketSources.Publish(source); source.InternalTransport.RegisterLogin((_, _, _) => throw new InvalidOperationException("이 검사에 없는 로그인")); source.Serve(6799); source.Connect(new byte[20], 6799); source.Poll(); source.Assign(-1, 1);
        uint token = connections.Registry.TokenFor(source.InternalTransport.Client); local.ClockSynchronization.Connection.Value = token; Assert.Equal(token, connections.Registry.Cleanup().Sync.Synchronization); source.Close(9); Assert.Equal(0u, local.ClockSynchronization.Connection.Value);
        local.GameTicks.Value = 0x87654321; source.Dispose(); connections.ShutdownDevices(); Assert.True(source.Released); Assert.Equal(0x87654321u, local.GameTicks.Value);
    }
    /// <summary>한 실제 owner에서 의존성을 유도하고 다른Client 물리 조합은 Pool 생성 전에 거부한다.</summary>
    [Fact]
    public void SharedDependenciesInferOneOwnerAndRejectMixedClients()
    {
        ClientLocalStateGlobals local = Local(), other = Local(); var connections = new ClientConnectionGlobals(local);
        var byLocal = new HostSession(new Kernel(), new GameRandom(41), new CrtRandom(), sharedLocal: local); Assert.Same(local.Gameplay, byLocal.Gameplay); Assert.Same(local, byLocal.Connections.LocalState);
        var byConnections = new HostSession(new Kernel(), new GameRandom(43), new CrtRandom(), sharedConnections: connections); Assert.Same(local, byConnections.LocalState); Assert.Same(local.Gameplay, byConnections.Gameplay);
        Assert.Throws<ArgumentException>(() => new HostSession(new Kernel(), new GameRandom(47), new CrtRandom(), sharedGameplay: other.Gameplay, sharedLocal: local));
        Assert.Throws<ArgumentException>(() => new HostSession(new Kernel(), new GameRandom(53), new CrtRandom(), sharedLocal: other, sharedConnections: connections));
    }

}
