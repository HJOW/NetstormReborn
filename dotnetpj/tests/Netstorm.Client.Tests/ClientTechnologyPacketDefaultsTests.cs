using System.Buffers;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 69h의 실제 장기 목록/고정 F와 현재 Runtime 선택 관문을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 Session을 보관하지 않는 기술 수신 기본 등록의 경계를 확인한다.</summary>
public sealed class ClientTechnologyPacketDefaultsTests
{
    /// <summary>수신 방향의 Span 접근 여부를 감지한다.</summary>
    private sealed class Unreadable : MemoryManager<byte>
    {
        /// <summary>메모리 조회를 지연하는 핸들이다.</summary>
        public Memory<byte> Buffer => CreateMemory(8);
        /// <summary>실제 Span을 요구하면 실패한다.</summary>
        public override Span<byte> GetSpan() => throw new IOException("미선택 Span 접근");
        /// <summary>이 검사에서는 포인터를 고정하지 않는다.</summary>
        public override MemoryHandle Pin(int elementIndex = 0) => throw new NotSupportedException();
        /// <summary>고정한 포인터가 없다.</summary>
        public override void Unpin() => throw new NotSupportedException();
        /// <summary>외부 소유 자원이 없다.</summary>
        protected override void Dispose(bool disposing) { }
    }
    /// <summary>연결/소켓 없이 실제 기본 표를 구성한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>정확한 고정 F/metadata와 저장된 callable은 빈 목록에서 Session/Player/연결을 조회하지 않는다.</summary>
    [Fact]
    public void EmptyFixedFunctionAndIncomingObserverNeedNoWorldOrPacketSpan()
    {
        var game = new ClientGameplayGlobals(); var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, game.Authority, game.GameAux); var dispatch = Dispatch();
        Assert.Equal(256, game.GameAux.TypeUse.Length); Assert.All(game.GameAux.TypeUse, value => Assert.Equal(0, value));
        ClientTechnologyPacketDefaults.Install(dispatch, game, storage); var row = dispatch.Registration(0x69); var saved = dispatch.SnapshotReceiver(0x69)!;
        Assert.Equal(0x004b49e0u, saved.NativeEntry); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(65535, row.Length); Assert.NotNull(row.PacketObserver); Assert.Null(row.Observer);
        using var memory = new Unreadable(); row.NotifyObserver(memory.Buffer, 0);
        dispatch.InvokeDirect(0x69, uint.MaxValue, null, new byte[6]);
        dispatch.RegisterCanonicalDirect(0x69, 0x9999, (_, _, _) => throw new IOException("새 표의 다른 F")); saved.Invoke(0, null, new byte[6]);
        Assert.Equal(0u, game.PlayerRecords.Base);
        Assert.Throws<ArgumentException>(() => ClientTechnologyPacketDefaults.Install(dispatch, game, new RawSurfaceStorage()));
    }

    /// <summary>빈 payload라도 기존 목록은 표시 대상이며 Session 부족 전에 합집합 저장을 유지한다.</summary>
    [Fact]
    public void UnionPersistsBeforeCurrentRuntimeSelection()
    {
        var game = new ClientGameplayGlobals(); var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, game.Authority, game.GameAux); var dispatch = Dispatch();
        ClientTechnologyPacketDefaults.Install(dispatch, game, storage);
        Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x69, 0, null, new byte[] { 0, 0, 0, 0, 0, 71, 72, 0 }));
        Assert.Equal(new byte[] { 71, 72, 0 }, game.GameAux.TypeUse[..3]);
        Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x69, 0, null, new byte[6]));
        Assert.Equal(0u, game.PlayerRecords.Base);
    }

    /// <summary>현재 실제 owner를 pin한 뒤 생산 미설치가 실패해도 호출 대여를 반납한다.</summary>
    [Fact]
    public void SelectedActualRuntimePinUnwindsWhenProductionIsMissing()
    {
        var game = new ClientGameplayGlobals(); var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, game.Authority, game.GameAux);
        var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: game, pool: storage.Pool);
        try
        {
            using var owner = HostMissionRuntime.Create(session, storage);
            var dispatch = Dispatch(); ClientTechnologyPacketDefaults.Install(dispatch, game, storage);
            var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x69, 0, null, new byte[] { 0, 0, 0, 0, 0, 71, 0 }));
            Assert.Contains("생산", error.Message); Assert.False(game.RuntimeCallActive(session)); Assert.False(owner.Released);
            Assert.Equal(71, game.GameAux.TypeUse[0]);
        }
        finally { session.ReleasePlayerProcesses(); }
    }
}
