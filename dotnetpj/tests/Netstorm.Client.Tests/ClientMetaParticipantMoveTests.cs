// 0x22 메타 참가자 이동의 원문 슬롯 효과·이동 후행을 대조한다.
// cpppj/src/client/NativeMetaParticipantMove.cpp 의 ExplodeNativeMetaParticipantMove/ReceiveNativeMetaParticipantMove 와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaPacketReceivers 중 0x22 등록을 대조한다.
// 0.962에서 R4-6 메타 참가자 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 0x22 메타 참가자 이동의 실제 CS·장치 호출을 기록한다.
public sealed class ClientMetaParticipantMoveTests
{
    /// <summary>같은 실제 CS와 장치 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();

        /// <summary>같은 실제 메타맵 모드다.</summary>
        public bool OwnerMode = true;

        /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
        public readonly UIntBox Subscriber = new() { Value = 0x111 };

        /// <summary>같은 실제 known 값이다.</summary>
        public uint Known = 1;

        /// <summary>같은 실제 소유자 번호다.</summary>
        public uint Owner = 0;

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>실제 폭발 효과 전투다.</summary>
        public readonly List<uint> Kicks = [];

        /// <summary>실제 전투 갱신이다.</summary>
        public readonly List<uint> Battles = [];

        /// <summary>실제 쫓겨남 음향 횟수다.</summary>
        public int KickedSounds;

        /// <summary>실제 떨굼 음향 횟수다.</summary>
        public int DropSounds;

        /// <summary>실제 음악 요청 횟수다.</summary>
        public int Musics;

        /// <summary>실제 당시 위치 음향이다.</summary>
        public readonly List<(float X, float Y)> Thunders = [];

        /// <summary>실제 Rig 좌표 호출 횟수다.</summary>
        public int Animates;

        /// <summary>실제 옵션 초기화 횟수다.</summary>
        public int Resets;

        /// <summary>실제 옵션 송신이다.</summary>
        public readonly List<uint> Sends = [];

        /// <summary>실제 Chat 모드다.</summary>
        public readonly List<uint> ChatModes = [];

        /// <summary>실제 Chat 일원 갱신 횟수다.</summary>
        public int ChatMembers;

        /// <summary>실제 NSInfo 갱신 횟수다.</summary>
        public int Refreshes;

        /// <summary>실제 참가자 표시다.</summary>
        public readonly List<(uint Participant, uint Mask)> Updates = [];

        /// <summary>실제 장부 갱신 횟수다.</summary>
        public int Cores;

        /// <summary>실제 이름표 서식이다.</summary>
        public readonly List<uint> Names = [];

        /// <summary>같은 실제 좌표 계산 결과다.</summary>
        public float[] SeatPoint = [10f, 20f];

        /// <summary>같은 실제 CS를 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaParticipantMoveHooks Hooks() => new()
        {
            Challenge = Challenge,
            OwnerModeChallenge = () => OwnerMode,
            Subscriber = Subscriber,
            Known = () => Known,
            Owner = () => Owner,
            Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
            Kick = (battle, slot) => Kicks.Add(battle),
            Battle = id => Battles.Add(id),
            KickedSound = () => KickedSounds++,
            DropSound = () => DropSounds++,
            Music = () => Musics++,
            Thunder = (x, y) => Thunders.Add((x, y)),
            Seat = (_, _, _) => SeatPoint,
            Animate = _ => Animates++,
            ResetOptions = _ => Resets++,
            SendOptions = (battle, _) => Sends.Add(battle),
            ChatMode = mode => ChatModes.Add(mode),
            ChatMembers = () => ChatMembers++,
            RefreshInfo = () => Refreshes++,
            Update = (participant, mask) => Updates.Add((participant, mask)),
            RefreshCore = () => Cores++,
            RebuildName = id => Names.Add(id),
        };

        /// <summary>실제 참가자 기록을 쓴다.</summary>
        public void Participant(uint index, uint subscriber, uint battle, uint slot, uint flags = 0)
        {
            Memory<byte> record = Challenge.Participant(index);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(4, 4), subscriber);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x64, 4), flags);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x9c, 4), BitConverter.SingleToUInt32Bits(1f));
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa0, 4), BitConverter.SingleToUInt32Bits(2f));
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa4, 4), battle);
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa8, 4), slot);
        }

        /// <summary>실제 전투 기록을 쓴다.</summary>
        public void Battle(uint index, byte valid, uint id = 0x202)
        {
            Memory<byte> record = Challenge.Battle(index);
            record.Span[0] = valid;
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(4, 4), id);
        }

        /// <summary>원문 0x22 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(byte id, byte flagsLow, byte newBattle, byte slot, byte flagsHigh = 0)
        {
            byte[] packet = new byte[17];
            packet[1] = 0x22;
            packet[4] = id;
            packet[5] = flagsLow;
            packet[6] = flagsHigh;
            // 지역 좌표는 0으로 두어 Seat 후행과 구별한다.
            packet[0xf] = newBattle;
            packet[0x10] = slot;
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantMove.Receive(null!, new byte[17], 0, 0));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantMove.Explode(null!, 0, 0, new RawCameraViewState(), new ClientMetaBattleSlotHooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantMove.Install(null!, new ClientMetaParticipantMoveHooks()));
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantMove.Install(Dispatch(), null!));
        var bare = new ClientMetaParticipantMoveHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaParticipantMove.Receive(bare, new byte[17], 0, 0));
    }

    /// <summary>원문 빈 관찰기·버전·고정 길이와 고정 F를 설치한다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserverVersionLengthAndCanonicalEntry()
    {
        var scene = new Scene();
        var dispatch = Dispatch();
        ClientMetaParticipantMove.Install(dispatch, scene.Hooks());
        var row = dispatch.Registration(0x22);
        Assert.Equal(1, row.Version);
        Assert.Equal(0, row.Mismatch);
        Assert.Equal(17, row.Length);
        Assert.NotNull(row.Observer);
        Assert.Null(row.PacketObserver);
        Assert.Equal(0x004b4820u, dispatch.SnapshotReceiver(0x22)!.NativeEntry);
    }

    /// <summary>메타맵 모드가 아니면 수신 전체를 건너뛴다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ReceiveIgnoresNonChallengeMode()
    {
        var scene = new Scene();
        scene.OwnerMode = false;
        ClientMetaParticipantMove.Receive(scene.Hooks(), Scene.Packet(1, 0, 2, 3), 0x111, 1);
        Assert.Empty(scene.Battles);
        Assert.Equal(0, scene.Cores);
    }

    /// <summary>CS가 없으면 진단 뒤 명시 실패한다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ReceiveAssertsWithoutCurrent()
    {
        var scene = new Scene();
        var hooks = scene.Hooks();
        hooks.Challenge = new ClientChallengeGlobals();
        Assert.Throws<InvalidOperationException>(() => ClientMetaParticipantMove.Receive(hooks, Scene.Packet(0, 0, 0, 0), 0, 0));
        Assert.Single(scene.Assertions);
    }

    /// <summary>무효 전투의 슬롯 효과는 진단 뒤 복귀한다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ExplodeAssertsInvalidBattleAndReturns()
    {
        var scene = new Scene();
        scene.Battle(2, valid: 0);
        var snapshot = new RawCameraViewState();
        snapshot.BaseCellFloatBits[0] = BitConverter.SingleToUInt32Bits(8f);
        snapshot.BaseCellFloatBits[1] = BitConverter.SingleToUInt32Bits(11f);
        var calls = new List<string>();
        int grounds = 0;
        var hooks = new ClientMetaBattleSlotHooks
        {
            Raw = _ => new byte[50],
            FirstFrame = _ => 0,
            Muzzle = (_, _) => [0f, 0f],
            Ground = (_, _) => grounds++,
            Sound = (_, _) => { },
            Assertion = (expression, file, line) => calls.Add(expression),
        };
        ClientMetaParticipantMove.Explode(scene.Challenge, 2, 0, snapshot, hooks);
        Assert.Single(calls);
        Assert.Equal(0, grounds);
    }

    /// <summary>유효 전투의 슬롯 효과는 실제 포구·스냅샷 보정 뒤 지면/음향을 호출한다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ExplodeComputesSeatOffsetWithSnapshot()
    {
        var scene = new Scene();
        scene.Battle(2, valid: 1);
        Memory<byte> battle = scene.Challenge.Battle(2);
        BinaryPrimitives.WriteUInt32LittleEndian(battle.Span.Slice(0x20, 4), 7);
        var snapshot = new RawCameraViewState();
        snapshot.BaseCellFloatBits[0] = BitConverter.SingleToUInt32Bits(8f);
        snapshot.BaseCellFloatBits[1] = BitConverter.SingleToUInt32Bits(11f);
        byte[] raw = new byte[50];
        raw[10] = 9;
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0xe, 4), BitConverter.SingleToUInt32Bits(100f));
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x12, 4), BitConverter.SingleToUInt32Bits(200f));
        var points = new List<(float X, float Y)>();
        var hooks = new ClientMetaBattleSlotHooks
        {
            Raw = _ => raw,
            FirstFrame = type => { Assert.Equal(9u, type); return 100u; },
            Muzzle = (type, frame) => [16f, 22f],
            Ground = (x, y) => points.Add((x, y)),
            Sound = (x, y) => points.Add((x, y)),
            Assertion = (_, _, _) => { },
        };
        // 좌석0의 고정 순서 4를 더한 프레임을 확인한다.
        uint seen = 0;
        hooks.Muzzle = (type, frame) => { seen = (uint)frame; return [16f, 22f]; };
        ClientMetaParticipantMove.Explode(scene.Challenge, 2, 0, snapshot, hooks);
        Assert.Equal(104u, seen);
        Assert.Equal(2, points.Count);
        Assert.Equal(102f, points[0].X);
        Assert.Equal(202f, points[0].Y);
    }

    /// <summary>전투 이동은 옛 좌석을 비우고 새 좌석에 참가자를 쓴다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ReceiveMovesParticipantBetweenBattles()
    {
        var scene = new Scene();
        scene.Owner = 0;
        scene.Participant(0, 0x111, battle: 2, slot: 3);
        scene.Participant(1, 0x222, battle: 0, slot: 0);
        scene.Battle(2, valid: 1, id: 0x202);
        scene.Battle(3, valid: 1, id: 0x203);
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0x38 + 3 * 4, 4), 0);
        ClientMetaParticipantMove.Receive(scene.Hooks(), Scene.Packet(0, 0, 3, 5), 0x111, 1);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0x38 + 3 * 4, 4)));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Battle(3).Span.Slice(0x38 + 5 * 4, 4)));
        Assert.Contains(0x202u, scene.Battles);
        Assert.Contains(0x203u, scene.Battles);
        Assert.Equal(0u, (uint)(scene.Challenge.Participant(0).Span[5] & 0x08));
        Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Participant(0).Span.Slice(0xa4, 4)));
        Assert.Equal(5u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Participant(0).Span.Slice(0xa8, 4)));
        Assert.Equal([(0u, 0xffu)], scene.Updates);
        Assert.Equal([0u], scene.Names);
        Assert.Equal(1, scene.Cores);
    }

    /// <summary>packet5 bit8을 중간 위치에서 지우고 Rig 뒤에도 좌표를 다시 쓴다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ReceiveClearsPacketFlagAndRewritesSeatPosition()
    {
        var scene = new Scene();
        scene.Owner = 0;
        scene.Participant(0, 0x111, battle: 0, slot: 0);
        scene.Battle(3, valid: 1, id: 0x203);
        scene.SeatPoint = [30f, 40f];
        byte[] packet = Scene.Packet(0, 8, 3, 5);
        ClientMetaParticipantMove.Receive(scene.Hooks(), packet, 0x111, 0);
        Assert.Equal(0, packet[5] & 8);
        Assert.Equal(BitConverter.SingleToUInt32Bits(30f), BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Participant(0).Span.Slice(0x9c, 4)));
        Assert.Equal(BitConverter.SingleToUInt32Bits(40f), BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Participant(0).Span.Slice(0xa0, 4)));
        Assert.Equal(1, scene.Animates);
    }

    /// <summary>쫓겨남 표식은 효과·음향·진단 순서로 후행한다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ReceiveKicksOldSlotWithSoundAndAssertion()
    {
        var scene = new Scene();
        scene.Owner = 1;
        scene.Participant(0, 0x111, battle: 0, slot: 3);
        scene.Battle(2, valid: 1);
        scene.Battle(3, valid: 1);
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Challenge.Battle(2).Span.Slice(0x38 + 3 * 4, 4), 0);
        byte[] packet = Scene.Packet(0, 8, 3, 5);
        ClientMetaParticipantMove.Receive(scene.Hooks(), packet, 0x111, 0);
        Assert.Equal([0u], scene.Kicks);
        Assert.Equal(1, scene.KickedSounds);
        Assert.Single(scene.Assertions);
    }

    /// <summary>입장은 포그·음악 후행과 자기 Chat 모드 전환을 수행한다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ReceiveEntersNewBattleWithThunderMusicAndChat()
    {
        var scene = new Scene();
        scene.Owner = 0;
        scene.Participant(0, 0x111, battle: 0, slot: 0);
        scene.Battle(3, valid: 1, id: 0x203);
        ClientMetaParticipantMove.Receive(scene.Hooks(), Scene.Packet(0, 0, 3, 5), 0x999, 1);
        Assert.Single(scene.Thunders);
        Assert.Equal(1, scene.Musics);
        Assert.Equal([0u, 1u], scene.ChatModes);
        Assert.Equal(1, scene.Refreshes);
    }

    /// <summary>차이 bit1은 같은 전투 소유자에게 떨굼 음향을 낸다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ReceiveDropsPieceSoundOnFlagDifference()
    {
        var scene = new Scene();
        scene.Owner = 0;
        scene.Participant(0, 0x111, battle: 3, slot: 5, flags: 0);
        scene.Battle(3, valid: 1);
        byte[] packet = Scene.Packet(0, 2, 3, 5);
        ClientMetaParticipantMove.Receive(scene.Hooks(), packet, 0x111, 0);
        Assert.Equal(1, scene.DropSounds);
        Assert.Equal(2u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Challenge.Participant(0).Span.Slice(0x64, 4)));
    }

    /// <summary>차이 bit2는 같은 전투 87BYTE를 초기화하고 송신한다. 0.962에서 추가됐다.</summary>
    [Fact]
    public void ReceiveResetsAndSendsOptionsOnFlagDifference()
    {
        var scene = new Scene();
        scene.Owner = 0;
        scene.Participant(0, 0x111, battle: 3, slot: 5, flags: 0);
        scene.Battle(3, valid: 1);
        byte[] packet = Scene.Packet(0, 4, 3, 5);
        ClientMetaParticipantMove.Receive(scene.Hooks(), packet, 0x111, 0);
        Assert.Equal(1, scene.Resets);
        Assert.Equal([3u], scene.Sends);
    }
}
