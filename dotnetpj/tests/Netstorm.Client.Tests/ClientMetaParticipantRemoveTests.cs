// 0x21 메타 참가자 제거의 원문 관찰·삭제·버전 경계를 대조한다.
// cpppj/src/client/NativeMetaParticipantRemove.cpp 의 Observe/Receive/Destroy/Remove와
// ClientChallengeGlobals.cpp 의 ClearArray/UpdateBattleVersionBounds를 대조한다.
// 0.957에서 R4-6 메타 참가자 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x21 메타 참가자 제거의 실제 CS·장치 호출을 기록한다.
public sealed class ClientMetaParticipantRemoveTests
{
    /// <summary>같은 실제 CS와 장치 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>같은 실제 메타맵 모드다.</summary>
        public bool OwnerMode = true;

        /// <summary>같은 실제 현재 검사 뷰다.</summary>
        public readonly RawCheckingFlag Checking = new();

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>실제 로그 호출이다.</summary>
        public readonly List<string> Logs = [];

        /// <summary>실제 전투 갱신이다.</summary>
        public readonly List<uint> Battles = [];

        /// <summary>실제 Chat 망각이다.</summary>
        public readonly List<uint> Chats = [];

        /// <summary>실제 하차 호출이다.</summary>
        public readonly List<(uint Object, bool Downward)> Leaves = [];

        /// <summary>실제 NSInfo 갱신 횟수다.</summary>
        public int Refreshes;

        /// <summary>같은 실제 CS를 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaParticipantRemoveHooks Hooks() => new()
        {
            Challenge = Challenge,
            OwnerModeChallenge = () => OwnerMode,
            Checking = Checking,
            Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
            Log = text => Logs.Add(text),
            Battle = id => Battles.Add(id),
            Chat = subscriber => Chats.Add(subscriber),
            Leave = (id, downward) => Leaves.Add((id, downward)),
            RefreshInfo = () => Refreshes++,
        };

        /// <summary>실제 참가자 기록을 쓴다.</summary>
        public void Participant(uint index, uint subscriber, uint battle, uint slot, byte flags, uint objectId)
        {
            Memory<byte> record = Challenge.Participant(index);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(4, 4), subscriber);
            record.Span[0x64] = flags;
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa4, 4), battle);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa8, 4), slot);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xb8, 4), objectId);
        }

        /// <summary>실제 전투 기록을 쓴다.</summary>
        public void Battle(uint index, byte valid, uint slotValue, uint slot = 3, uint id = 0x202)
        {
            Memory<byte> record = Challenge.Battle(index);
            record.Span[0] = valid;
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x38 + unchecked((int)slot) * 4, 4), slotValue);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(4, 4), id);
        }

        /// <summary>원문 0x21 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(params byte[] ids)
        {
            byte[] packet = new byte[5 + ids.Length];
            packet[1] = 0x21;
            packet[4] = (byte)ids.Length;
            ids.CopyTo(packet, 5);
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantRemove.Receive(null!, new byte[8]));
        var bare = new ClientMetaParticipantRemoveHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaParticipantRemove.Receive(bare, new byte[8]));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantRemove.Install(null!, scene.Hooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantRemove.Install(Dispatch(), null!));
    }

    /// <summary>원문 packet 관찰기·버전·가변 길이와 고정 F를 설치한다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaParticipantRemove.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x21);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(ushort.MaxValue, row.Length);
        Assert.NotNull(row.PacketObserver);
        Assert.Null(row.Observer);
        Assert.Equal(0x004b46d0u, dispatch.SnapshotReceiver(0x21)!.NativeEntry);
    }

    /// <summary>수신 방향 관찰기는 버퍼를 읽지 않는다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void ObserveReceivingIgnoresBuffer()
    {
        var scene = new Scene();
        byte[] packet = Scene.Packet(1);
        ClientMetaParticipantRemove.Observe(packet, 0, scene.Hooks().Assertion);
        Assert.Empty(scene.Assertions);
    }

    /// <summary>송신 관찰기는 현재 개수를 다시 읽어 선언 WORD를 쓴다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void ObserveSendingAssertsAndWritesLength()
    {
        var scene = new Scene();
        byte[] packet = Scene.Packet(1, 2, 3);
        ClientMetaParticipantRemove.Observe(packet, 1, scene.Hooks().Assertion);
        Assert.Empty(scene.Assertions);
        Assert.Equal((ushort)(3 + 5), BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        byte[] full = [0, 0x21, 0, 0, 0xff];
        ClientMetaParticipantRemove.Observe(full, 1, scene.Hooks().Assertion);
        Assert.Single(scene.Assertions);
    }

    /// <summary>메타맵 모드가 아니면 수신 전체를 건너뛴다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void ReceiveIgnoresNonChallengeMode()
    {
        var scene = new Scene();
        scene.OwnerMode = false;
        ClientMetaParticipantRemove.Receive(scene.Hooks(), Scene.Packet(1));
        Assert.Empty(scene.Logs);
        Assert.Equal(0, scene.Refreshes);
    }

    /// <summary>CS가 없으면 진단 뒤 명시 실패한다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void ReceiveAssertsWithoutCurrent()
    {
        var scene = new Scene();
        var hooks = scene.Hooks();
        hooks.Challenge = new ClientChallengeGlobals();
        Assert.Throws<InvalidOperationException>(() => ClientMetaParticipantRemove.Receive(hooks, Scene.Packet()));
        Assert.Single(scene.Assertions);
    }

    /// <summary>삭제는 좌석·Chat·파괴 뒤 버전을 다시 집계한다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void RemoveDeletesParticipantAndClearsBattleSlot()
    {
        var scene = new Scene();
        scene.Participant(1, 0x111, battle: 2, slot: 3, flags: 1, objectId: 0x901);
        scene.Battle(2, valid: 1, slotValue: 1);
        ClientMetaParticipantRemove.Receive(scene.Hooks(), Scene.Packet(1));
        Assert.Equal([0x111u], scene.Chats);
        Assert.Equal([(0x901u, true)], scene.Leaves);
        Assert.Equal([0x202u], scene.Battles);
        Assert.Equal(1, scene.Refreshes);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0x44, 4)));
        Assert.Equal(0x100u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0x1c, 4)));
        Assert.Equal(0xffffffffu, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0x18, 4)));
        Assert.True(scene.Challenge.Participant(1).Span.ToArray().All(value => value == 0));
    }

    /// <summary>범위 밖 번호는 진단·로그 뒤 다음 요소로 간다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void RemoveRejectsIllegalId()
    {
        var scene = new Scene();
        ClientMetaParticipantRemove.Receive(scene.Hooks(), Scene.Packet(0));
        Assert.Single(scene.Assertions);
        Assert.Single(scene.Logs);
        Assert.Empty(scene.Chats);
        Assert.Equal(1, scene.Refreshes);
    }

    /// <summary>비활성 기록은 로그 뒤 파괴하지 않는다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void RemoveSkipsInactiveWithLog()
    {
        var scene = new Scene();
        scene.Participant(1, 0x111, battle: 0, slot: 0, flags: 0, objectId: 0x901);
        ClientMetaParticipantRemove.Receive(scene.Hooks(), Scene.Packet(1));
        Assert.Single(scene.Logs);
        Assert.Empty(scene.Chats);
        Assert.Empty(scene.Leaves);
        Assert.Equal(1, scene.Refreshes);
    }

    /// <summary>8 이상 좌석은 명시 거부한다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void SlotOutOfRangeThrows()
    {
        var scene = new Scene();
        scene.Participant(1, 0x111, battle: 2, slot: 8, flags: 1, objectId: 0x901);
        scene.Battle(2, valid: 1, slotValue: 1, slot: 3);
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantRemove.Receive(scene.Hooks(), Scene.Packet(1)));
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiverRunsSameBody()
    {
        var scene = new Scene();
        scene.Participant(2, 0x222, battle: 0, slot: 0, flags: 1, objectId: 0x902);
        var dispatch = Dispatch();
        ClientMetaParticipantRemove.Install(dispatch, scene.Hooks());
        dispatch.InvokeDirect(0x21, 9, null, Scene.Packet(2));
        Assert.Equal([0x222u], scene.Chats);
        Assert.Equal(1, scene.Refreshes);
    }

    /// <summary>짧은 패킷은 원문 필드 폭에서 거부한다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantRemove.Receive(scene.Hooks(), new byte[4]));
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantRemove.Observe(new byte[4], 1, (_, _, _) => { }));
    }

    /// <summary>가변 배열 free는 두 토큰을 반납하고 네 DWORD를 순서대로 지운다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void ClearArrayFreesTokensAndZeroes()
    {
        var challenge = new ClientChallengeGlobals();
        uint first = challenge.AllocateArray(8);
        uint second = challenge.AllocateArray(4);
        byte[] header = new byte[16];
        BinaryPrimitives.WriteUInt32LittleEndian(header.AsSpan(0, 4), first);
        BinaryPrimitives.WriteUInt32LittleEndian(header.AsSpan(4, 4), second);
        header[8] = 0xaa;
        header[12] = 0xbb;
        challenge.ClearArray(header);
        Assert.Throws<InvalidOperationException>(() => challenge.ArrayBytes(first));
        Assert.Throws<InvalidOperationException>(() => challenge.ArrayBytes(second));
        Assert.True(header.AsSpan().ToArray().All(value => value == 0));
    }

    /// <summary>미등록 토큰 free는 명시 실패한다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void ClearArrayRejectsUnknownToken()
    {
        var challenge = new ClientChallengeGlobals();
        byte[] header = new byte[16];
        BinaryPrimitives.WriteUInt32LittleEndian(header.AsSpan(0, 4), 0x9999);
        Assert.Throws<InvalidOperationException>(() => challenge.ClearArray(header));
        Assert.Throws<ArgumentOutOfRangeException>(() => challenge.ClearArray(new byte[15]));
    }

    /// <summary>버전 집계는 여덟 슬롯의 상한·하한을 다시 쓴다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void UpdateBattleVersionBoundsComputesMinMax()
    {
        var challenge = new ClientChallengeGlobals();
        challenge.Prepare();
        Memory<byte> battle = challenge.Battle(2);
        battle.Span[0] = 1;
        BinaryPrimitives.WriteUInt32LittleEndian(battle.Span.Slice(0x38, 4), 5);
        BinaryPrimitives.WriteUInt32LittleEndian(battle.Span.Slice(0x3c, 4), 6);
        BinaryPrimitives.WriteUInt16LittleEndian(challenge.Participant(5).Span.Slice(0xb4, 2), 100);
        BinaryPrimitives.WriteUInt16LittleEndian(challenge.Participant(5).Span.Slice(0xb2, 2), 50);
        BinaryPrimitives.WriteUInt16LittleEndian(challenge.Participant(6).Span.Slice(0xb4, 2), 200);
        BinaryPrimitives.WriteUInt16LittleEndian(challenge.Participant(6).Span.Slice(0xb2, 2), 10);
        var assertions = new List<(string, string, uint)>();
        ClientChallengeGlobals.UpdateBattleVersionBounds(challenge.Bytes, 2, (expression, file, line) => assertions.Add((expression, file, line)));
        Assert.Empty(assertions);
        Assert.Equal(10u, BinaryPrimitives.ReadUInt32LittleEndian(battle.Span.Slice(0x1c, 4)));
        Assert.Equal(200u, BinaryPrimitives.ReadUInt32LittleEndian(battle.Span.Slice(0x18, 4)));
    }

    /// <summary>잘못된 전투 번호는 진단 뒤 명시 거부한다. 0.957에서 추가됐다.</summary>
    [Fact]
    public void UpdateBattleVersionBoundsRejectsBadBattle()
    {
        var challenge = new ClientChallengeGlobals();
        challenge.Prepare();
        var assertions = new List<(string, string, uint)>();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientChallengeGlobals.UpdateBattleVersionBounds(challenge.Bytes, 10, (expression, file, line) => assertions.Add((expression, file, line))));
        Assert.Single(assertions);
    }
}
