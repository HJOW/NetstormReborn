using System.Buffers.Binary;
using Netstorm.Client.Player;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x20 메타 참가자 추가의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/NativeMetaParticipantCreate.cpp 의 ObserveNativeMetaParticipantAdd 와
// cpppj/src/client/ClientCampaignTransport.cpp 의 ReceiveClientMetaParticipantAdd/InstallClientMetaPacketReceivers 중 0x20 등록을 이식한다.
// 생성 본체(ConstructNativeMetaParticipant)·좌석·입장은 후속 결선에서 잇는다.
// 0.960에서 R4-6 메타 참가자 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x20 메타 참가자 추가가 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaParticipantAddHooks
{
    /// <summary>같은 실제 CS 참가자40 기록 owner다.</summary>
    public ClientChallengeGlobals? Challenge { get; set; }

    /// <summary>같은 실제 메타맵 모드 owner다. 거짓이면 수신 전체를 건너뛴다.</summary>
    public Func<bool>? OwnerModeChallenge { get; set; }

    /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
    public UIntBox? Subscriber { get; set; }

    /// <summary>같은 실제 메타 예약 owner다.</summary>
    public UIntBox? MetaPending { get; set; }

    /// <summary>같은 실제 메타 갱신 예약 owner다.</summary>
    public UIntBox? MetaRefresh { get; set; }

    /// <summary>같은 실제 Core/Player owner다.</summary>
    public ClientCoreDataGlobals? CoreData { get; set; }

    /// <summary>같은 실제 Player 전역이다.</summary>
    public PlayerWorldState? Player { get; set; }

    /// <summary>같은 실제 메타맵 모드를 읽는다.</summary>
    public Func<uint>? Metamap { get; set; }

    /// <summary>원문 진단의 실제 선택 줄 소비자다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>active 진단 뒤 현재 실제 생성 장치를 결합하는 전체 생성 부모다.</summary>
    public Action<Memory<byte>, ReadOnlyMemory<byte>>? Construct { get; set; }

    /// <summary>캡처한 전투 실제 식별자의 특정 전투 표시 장치다.</summary>
    public Action<uint>? Battle { get; set; }

    /// <summary>캡처된 참가자와 현재 전투의 포구/좌석 계산 장치다.</summary>
    public Func<Memory<byte>, uint, uint, float[]>? Seat { get; set; }

    /// <summary>원문 packet 이름 주소를 같은 Chat 채널에 보내는 장치다.</summary>
    public Action<uint, ReadOnlyMemory<byte>, uint>? Chat { get; set; }

    /// <summary>지정 참가자 ff 갱신 장치다.</summary>
    public Action<uint, uint>? Update { get; set; }

    /// <summary>같은 NSInfo 갱신 장치다.</summary>
    public Action? RefreshInfo { get; set; }
}

// 42c180 참가자 추가 수신의 캡처 기록 생성·좌석·Chat·표시 순서다.
public static class ClientMetaParticipantAdd
{
    /// <summary>원문 0x20 메타 참가자 추가 종류다. 0.960에서 추가됐다.</summary>
    public const uint Type = 0x20;

    /// <summary>원문 0x20 수신의 실제 고정 F 주소다. 0.960에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b4710;

    /// <summary>원문 0x20 관찰기의 진단 줄 번호다(0x534). 0.960에서 추가됐다.</summary>
    public const uint ObserveAssertLine = 0x534;

    /// <summary>원문 0x20 관찰기의 진단 파일이다. 0.960에서 추가됐다.</summary>
    private const string PacketFile = "\\Ns\\O\\_p_packets.cpp";

    /// <summary>원문 0x20 수신의 진단 파일이다. 0.960에서 추가됐다.</summary>
    private const string StateFile = ".\\State.cpp";

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.960에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 참가자 추가의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.960에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 참가자 추가의 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.960에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 추가의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>수신 관찰은 필드를 읽지 않고 송신 진단에서 복귀한 뒤 현재 BYTE를 다시 읽는다. 0.960에서 추가됐다.</summary>
    /// <param name="packet">같은 현재 버퍼의 실제 물리다.</param>
    /// <param name="sending">원문 송신 방향 여부다.</param>
    /// <param name="assertion">실제 진단 장치다.</param>
    public static void Observe(Memory<byte> packet, uint sending, Action<string, string, uint>? assertion)
    {
        // 수신 방향은 실제 패킷 필드를 읽기 전에 복귀한다.
        if (sending == 0)
        {
            return;
        }

        if (Read(packet.Span, 0x12, 1) == 0xffu)
        {
            (assertion ?? throw new InvalidOperationException("메타 참가자 추가의 실제 진단 미연결"))("nameLen != (unsigned char)-1", PacketFile, ObserveAssertLine);
        }

        // 진단 복귀 뒤 현재 BYTE를 다시 읽어 선언 WORD를 쓴다.
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Span.Slice(2, 2), unchecked((ushort)(Read(packet.Span, 0x12, 1) + 0x35u)));
    }

    /// <summary>같은 현재 전투8자리 구독자/Core 장부를 갱신한다. 0.960에서 추가됐다.</summary>
    /// <param name="challenge">같은 실제 CS owner다.</param>
    /// <param name="battleOwner">현재 전투 소유자 번호다.</param>
    /// <param name="missionPost">현재 미션 후처리 모드다.</param>
    /// <param name="playerKnown">현재 Player known 값이다.</param>
    /// <param name="core">같은 실제 Core/Player owner다.</param>
    public static void RefreshCore(ClientChallengeGlobals challenge, uint battleOwner, bool missionPost, uint playerKnown, ClientCoreDataGlobals core)
    {
        ArgumentNullException.ThrowIfNull(challenge);
        ArgumentNullException.ThrowIfNull(core);
        if (missionPost || playerKnown == 0)
        {
            return;
        }

        Memory<byte> own = challenge.Participant(battleOwner);
        uint battle = BinaryPrimitives.ReadUInt32LittleEndian(own.Span.Slice(0xa4, 4));
        if (battle == 0)
        {
            return;
        }

        // 원문의 8묶음×5기록을 같은 인덱스 순서로 읽는다.
        for (uint index = 0; index < RawChallengeState.ParticipantCount; index++)
        {
            Memory<byte> peer = challenge.Participant(index);
            if ((peer.Span[0x64] & 1u) == 0 || BinaryPrimitives.ReadUInt32LittleEndian(peer.Span.Slice(0xa4, 4)) != battle)
            {
                continue;
            }

            byte[] coreBytes = peer.Span.Slice(0x54, 10).ToArray();
            core.StoreMetadata(BinaryPrimitives.ReadUInt32LittleEndian(peer.Span.Slice(0xa8, 4)) + 1u, BinaryPrimitives.ReadUInt32LittleEndian(peer.Span.Slice(4, 4)), coreBytes);
        }
    }

    /// <summary>캡처 기록 생성→같은 CS 좌석/버전→좌표→Chat→지정 표시→NSInfo 순서다. 0.960에서 추가됐다.</summary>
    /// <param name="hooks">실제 CS·모드·구독자·예약·Core·Player·진단·생성·전투·좌석·Chat·표시·NSInfo 장치다.</param>
    /// <param name="packet">원문 0x20 패킷이다.</param>
    public static void Receive(ClientMetaParticipantAddHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientChallengeGlobals challenge = Need(hooks.Challenge);
        if (!NeedDelegate(hooks.OwnerModeChallenge)())
        {
            return;
        }

        if (challenge.Current == 0)
        {
            NeedDelegate(hooks.Assertion)("CS", StateFile, 0xd0d);
        }

        ReadOnlySpan<byte> bytes = packet.Span;
        uint subscriber = Need(hooks.Subscriber).Value;
        if (Read(bytes, 5, 1) == subscriber)
        {
            Need(hooks.CoreData).Select(Need(hooks.Player), NeedDelegate(hooks.Metamap)(), Read(bytes, 4, 1), 1);
            Need(hooks.MetaPending).Value++;
            Need(hooks.MetaRefresh).Value++;
        }

        Memory<byte> cs = challenge.Bytes;
        uint id = Read(bytes, 4, 1);
        if (id >= RawChallengeState.ParticipantCount)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "메타 참가자 추가의 실제 참가자40 범위");
        }

        Memory<byte> record = cs.Slice(RawChallengeState.ParticipantOffset + unchecked((int)id) * RawChallengeState.ParticipantBytes, RawChallengeState.ParticipantBytes);
        if ((record.Span[0x64] & 1u) != 0)
        {
            NeedDelegate(hooks.Assertion)("!p->isValid()", StateFile, 0x5b4);
        }

        NeedDelegate(hooks.Construct)(record, packet);
        uint battle = BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0xa4, 4));
        if (battle != 0)
        {
            uint seat = BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0xa8, 4));
            uint participant = Read(bytes, 4, 1);
            Memory<byte> current = cs.Slice(RawChallengeState.BattleOffset + unchecked((int)battle) * RawChallengeState.BattleBytes, RawChallengeState.BattleBytes);
            if (seat >= 8)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "메타 참가자 추가의 실제 좌석8칸 범위");
            }

            if (BinaryPrimitives.ReadUInt32LittleEndian(current.Span.Slice(0x38 + unchecked((int)seat) * 4, 4)) != 0)
            {
                NeedDelegate(hooks.Assertion)("!playerIds[slot]", StateFile, 0x1c8);
            }

            BinaryPrimitives.WriteUInt32LittleEndian(current.Span.Slice(0x38 + unchecked((int)seat) * 4, 4), participant);
            NeedDelegate(hooks.Battle)(BinaryPrimitives.ReadUInt32LittleEndian(current.Span.Slice(4, 4)));
            ClientChallengeGlobals.UpdateBattleVersionBounds(cs, unchecked((int)BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0xa4, 4))), hooks.Assertion);
        }

        uint currentBattle = BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0xa4, 4));
        if (currentBattle != 0)
        {
            float[] position = NeedDelegate(hooks.Seat)(record, currentBattle, BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0xa8, 4)));
            if (position.Length < 2)
            {
                throw new ArgumentOutOfRangeException(nameof(hooks), "메타 참가자 추가의 실제 좌석 좌표 부족");
            }

            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x9c, 4), BitConverter.SingleToUInt32Bits(position[0]));
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0xa0, 4), BitConverter.SingleToUInt32Bits(position[1]));
        }

        if (bytes.Length < 0x35)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "메타 참가자 Chat 이름의 실제 packet 물리 부족");
        }

        ReadOnlyMemory<byte> name = packet.Slice(0x35);
        NeedDelegate(hooks.Chat)(Read(bytes, 5, 1), name, 0x20);
        NeedDelegate(hooks.Update)(Read(bytes, 4, 1), 0xffu);
        NeedDelegate(hooks.RefreshInfo)();
    }

    /// <summary>0x20의 원문 packet 관찰기·버전·가변 길이와 고정 F를 설치한다. 0.960에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 CS·모드·구독자·예약·Core·Player·진단·생성·전투·좌석·Chat·표시·NSInfo 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientMetaParticipantAddHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        dispatch.ObservePacket(Type, (packet, sending) => Observe(packet, sending, hooks.Assertion));
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, ushort.MaxValue);
        // 모드 자연 반환 앞에는 Source/Session/packet을 요구하지 않는다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) => Receive(hooks, packet));
    }
}
