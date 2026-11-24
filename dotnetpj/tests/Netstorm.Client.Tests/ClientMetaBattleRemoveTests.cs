// 0x24 메타 전투 삭제의 원문 부분 효과·버전·참가자 검사를 대조한다.
// cpppj/src/client/NativeMetaBattleRemove.cpp 의 ReceiveNativeMetaBattleRemove를 대조한다.
// 0.958에서 R4-6 메타 참가자 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x24 메타 전투 삭제의 실제 CS·하차·진단 호출을 기록한다.
public sealed class ClientMetaBattleRemoveTests
{
    /// <summary>같은 실제 CS와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>실제 로그 호출이다.</summary>
        public readonly List<string> Logs = [];

        /// <summary>실제 하차 호출이다.</summary>
        public readonly List<uint> Leaves = [];

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>같은 실제 CS를 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaBattleRemoveHooks Hooks() => new()
        {
            Challenge = Challenge,
            Log = text => Logs.Add(text),
            Leave = id => Leaves.Add(id),
            Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
        };

        /// <summary>실제 전투 기록을 쓴다.</summary>
        public void Battle(uint index, byte valid, uint objectId, uint headerFirst, uint headerSecond)
        {
            Memory<byte> record = Challenge.Battle(index);
            record.Span[0] = valid;
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x20, 4), objectId);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x24, 4), headerFirst);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x28, 4), headerSecond);
        }

        /// <summary>원문 0x24 5BYTE 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(byte id)
        {
            byte[] packet = new byte[5];
            packet[1] = 0x24;
            packet[4] = id;
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleRemove.Receive(null!, new byte[5]));
        var bare = new ClientMetaBattleRemoveHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaBattleRemove.Receive(bare, new byte[5]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleRemove.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaBattleRemove.Install(Dispatch(), null!));
    }

    /// <summary>원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaBattleRemove.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x24);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(5, row.Length);
        Assert.NotNull(row.Observer);
        Assert.Null(row.PacketObserver);
        Assert.Equal(0x004b4600u, dispatch.SnapshotReceiver(0x24)!.NativeEntry);
    }

    /// <summary>없으면 진단 뒤 반환한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void ReceiveLogsMissingBattle()
    {
        var scene = new Scene();
        ClientMetaBattleRemove.Receive(scene.Hooks(), Scene.Packet(2));
        Assert.Single(scene.Logs);
        Assert.Empty(scene.Leaves);
        Assert.Empty(scene.Assertions);
    }

    /// <summary>삭제는 하차·반납·초기화·버전·참가자 검사 순서로 둔다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void ReceiveDestroysBattleAndChecksParticipants()
    {
        var scene = new Scene();
        uint first = scene.Challenge.AllocateArray(8);
        uint second = scene.Challenge.AllocateArray(4);
        scene.Battle(2, valid: 1, objectId: 0x777, headerFirst: first, headerSecond: second);
        scene.Challenge.Participant(5).Span[0xa4] = 2;
        ClientMetaBattleRemove.Receive(scene.Hooks(), Scene.Packet(2));
        Assert.Equal([0x777u], scene.Leaves);
        Assert.Throws<InvalidOperationException>(() => scene.Challenge.ArrayBytes(first));
        Assert.Throws<InvalidOperationException>(() => scene.Challenge.ArrayBytes(second));
        Memory<byte> record = scene.Challenge.Battle(2);
        Assert.Equal(0xffffffffu, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0x18, 4)));
        Assert.Equal(0x100u, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0x1c, 4)));
        byte[] body = record.ToArray();
        body.AsSpan(0x18, 4).Fill(0);
        body.AsSpan(0x1c, 4).Fill(0);
        Assert.True(body.AsSpan().ToArray().All(value => value == 0));
        Assert.Single(scene.Assertions);
    }

    /// <summary>잘못된 전투 번호는 기록 범위에서 거부한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void ReceiveRejectsBadId()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaBattleRemove.Receive(scene.Hooks(), Scene.Packet(10)));
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        scene.Battle(2, valid: 1, objectId: 0x777, headerFirst: 0, headerSecond: 0);
        var dispatch = Dispatch();
        ClientMetaBattleRemove.Install(dispatch, scene.Hooks());
        dispatch.InvokeDirect(0x24, 9, null, Scene.Packet(2));
        Assert.Equal([0x777u], scene.Leaves);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0, 4)));
    }

    /// <summary>짧은 패킷은 원문 필드 폭에서 거부한다. 0.958에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaBattleRemove.Receive(scene.Hooks(), new byte[3]));
    }
}
