// 2번 연결 종료의 원문 고정 F 설치와 전역·효과 결선을 대조한다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientConnectionEndedReceiver(004b6660)와
// Netstorm.Net/ConnectionEnded.cs 의 ReceiveConnectionEnded 본체 공유를 대조한다.
// 0.953에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 2번 연결 종료의 실제 게임·연결·활성·기록·효과 호출을 기록한다.
public sealed class ClientConnectionEndedReceiverTests
{
    /// <summary>같은 실제 소유자와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 Player/권한 소유자다.</summary>
        public readonly ClientGameplayGlobals Game = new();

        /// <summary>같은 실제 로컬 상태 owner다.</summary>
        public readonly ClientLocalStateGlobals Local;

        /// <summary>같은 실제 연결 Registry/장부다.</summary>
        public readonly ClientConnectionGlobals Connections;

        /// <summary>같은 실제 22칸 활성 owner다.</summary>
        public readonly ClientConnectionActivityGlobals Activity = new();

        /// <summary>실제 Player9 바이트다.</summary>
        public readonly byte[] Players = new byte[1620];

        /// <summary>실제 장애 저장 횟수다.</summary>
        public int Saves;

        /// <summary>실제 방송 소유자다.</summary>
        public readonly List<uint> Broadcasts = [];

        /// <summary>같은 실제 소유자 묶음이다.</summary>
        public Scene()
        {
            Game.Authority.Server = false;
            Local = new(Game, new AmbientSoundState());
            Connections = new(Local);
        }

        /// <summary>현재 phase를 쓴다.</summary>
        public void Phase(uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Local.State.Phase.Span.Slice(0, 4), value);

        /// <summary>현재 next를 쓴다.</summary>
        public void Next(uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Local.State.Next.Span.Slice(0, 4), value);

        /// <summary>같은 장기 분배기에 실제 고정 F를 설치한다.</summary>
        public RawPacketDispatch Install(RawPacketDispatch dispatch)
        {
            ClientConnectionEndedReceiver.Install(dispatch, Game, Connections, Activity, () => Players, () => Saves++, Broadcasts.Add);
            return dispatch;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치와 소유자 불일치는 일반 성공으로 꾸미지 않는다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesAndOwnerMismatchFailExplicitly()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        Assert.Throws<ArgumentNullException>(() => ClientConnectionEndedReceiver.Install(null!, scene.Game, scene.Connections, scene.Activity, () => scene.Players, () => { }, _ => { }));
        Assert.Throws<ArgumentNullException>(() => ClientConnectionEndedReceiver.Install(dispatch, null!, scene.Connections, scene.Activity, () => scene.Players, () => { }, _ => { }));
        Assert.Throws<ArgumentNullException>(() => ClientConnectionEndedReceiver.Install(dispatch, scene.Game, null!, scene.Activity, () => scene.Players, () => { }, _ => { }));
        Assert.Throws<ArgumentNullException>(() => ClientConnectionEndedReceiver.Install(dispatch, scene.Game, scene.Connections, null!, () => scene.Players, () => { }, _ => { }));
        Assert.Throws<ArgumentNullException>(() => ClientConnectionEndedReceiver.Install(dispatch, scene.Game, scene.Connections, scene.Activity, null!, () => { }, _ => { }));
        Assert.Throws<ArgumentNullException>(() => ClientConnectionEndedReceiver.Install(dispatch, scene.Game, scene.Connections, scene.Activity, () => scene.Players, null!, _ => { }));
        Assert.Throws<ArgumentNullException>(() => ClientConnectionEndedReceiver.Install(dispatch, scene.Game, scene.Connections, scene.Activity, () => scene.Players, () => { }, null!));
        var other = new ClientGameplayGlobals();
        var foreign = new ClientConnectionGlobals(new ClientLocalStateGlobals(other, new AmbientSoundState()));
        Assert.Throws<ArgumentException>(() => ClientConnectionEndedReceiver.Install(dispatch, scene.Game, foreign, scene.Activity, () => scene.Players, () => { }, _ => { }));
    }

    /// <summary>같은 4BYTE 통지의 observer/version/length 표는 바꾸지 않고 고정 F만 설치한다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsCanonicalEntryOnly()
    {
        var scene = new Scene();
        var dispatch = scene.Install(Dispatch());
        Assert.Equal(0x004b6660u, dispatch.SnapshotReceiver(2)!.NativeEntry);
        Assert.Null(dispatch.Registration(2).Observer);
        Assert.Null(dispatch.Registration(2).PacketObserver);
    }

    /// <summary>서버가 아니면 9번 장애를 저장하고 같은 연결을 닫는다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void NonServerNineSavesFailure()
    {
        var scene = new Scene();
        scene.Game.Authority.Server = false;
        var dispatch = scene.Install(Dispatch());
        dispatch.InvokeDirect(2, 9, null, new byte[4]);
        Assert.Equal(1, scene.Saves);
        Assert.Empty(scene.Broadcasts);
    }

    /// <summary>서버이면 1~8번 소유자의 flags를 세우고 방송한다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void ServerOwnerBroadcastsAndFlags()
    {
        var scene = new Scene();
        scene.Game.Authority.Server = true;
        scene.Activity.Frames[5] = 7;
        var dispatch = scene.Install(Dispatch());
        dispatch.InvokeDirect(2, 5, null, new byte[4]);
        Assert.Equal([5u], scene.Broadcasts);
        Assert.Equal(0x100u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Players.AsSpan(5 * 180 + 0x50, 4)));
        Assert.Equal(0u, scene.Activity.Frames[5]);
    }

    /// <summary>NULL 연결은 0 토큰으로 비교하며 10번·23상은 next를 20으로 둔다. 0.953에서 추가됐다.</summary>
    [Fact]
    public void NullConnectionKeepsSyncAndSetsNext20()
    {
        var scene = new Scene();
        scene.Game.Authority.Server = false;
        scene.Local.ClockSynchronization.Connection.Value = 5;
        scene.Phase(23);
        scene.Next(21);
        var dispatch = scene.Install(Dispatch());
        dispatch.InvokeDirect(2, 10, null, new byte[4]);
        Assert.Equal(5u, scene.Local.ClockSynchronization.Connection.Value);
        Assert.Equal(20u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Next.Span.Slice(0, 4)));
    }
}
