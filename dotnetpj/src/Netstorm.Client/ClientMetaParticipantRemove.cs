using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x21 메타 참가자 제거의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/NativeMetaParticipantRemove.cpp 의 Observe/Receive/Destroy/Remove 와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaPacketReceivers 중 0x21 등록을 이식한다.
// 0.957에서 R4-6 메타 참가자 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x21 메타 참가자 제거가 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaParticipantRemoveHooks
{
    /// <summary>같은 실제 CS 참가자40 기록 owner다.</summary>
    public ClientChallengeGlobals? Challenge { get; set; }

    /// <summary>같은 실제 메타맵 모드 owner다. 거짓이면 수신 전체를 건너뛴다.</summary>
    public Func<bool>? OwnerModeChallenge { get; set; }

    /// <summary>같은 실제 현재 검사 뷰다.</summary>
    public RawCheckingFlag? Checking { get; set; }

    /// <summary>원문 진단의 실제 선택 줄 소비자다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>수준9의 실제 진단 로그 장치다.</summary>
    public Action<string>? Log { get; set; }

    /// <summary>같은 실제 전투 표시 갱신 장치다.</summary>
    public Action<uint>? Battle { get; set; }

    /// <summary>실제 Chat 채널 망각 장치다.</summary>
    public Action<uint>? Chat { get; set; }

    /// <summary>같은 실제 섬 장비 하차 장치다.</summary>
    public Action<uint, bool>? Leave { get; set; }

    /// <summary>같은 실제 NSInfo 갱신 장치다.</summary>
    public Action? RefreshInfo { get; set; }
}

// 42c2a0 참가자 제거의 수신 관찰·캡처·삭제·버전 경계를 복원한다.
public static class ClientMetaParticipantRemove
{
    /// <summary>원문 0x21 메타 참가자 제거 종류다. 0.957에서 추가됐다.</summary>
    public const uint Type = 0x21;

    /// <summary>원문 0x21 수신의 실제 고정 F 주소다. 0.957에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b46d0;

    /// <summary>원문 0x21 관찰기의 진단 줄 번호다(0x586). 0.957에서 추가됐다.</summary>
    public const uint ObserveAssertLine = 0x586;

    /// <summary>원문 0x21 관찰기의 진단 파일이다. 0.957에서 추가됐다.</summary>
    private const string PacketFile = "\\Ns\\O\\_p_packets.cpp";

    /// <summary>참가자 제거 진단의 원문 파일이다. 0.957에서 추가됐다.</summary>
    private const string ChallengeFile = ".\\State.cpp";

    /// <summary>제거·버전 집계 진단의 원문 파일이다. 0.957에서 추가됐다.</summary>
    private const string ChannelFile = ".\\Ns\\O\\Ch.cpp";

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.957에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 참가자 제거의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.957에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 참가자 제거의 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.957에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 제거의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>같은 현재 기록의 지정 폭만 쓰고 선행 효과를 되돌리지 않는다. 0.957에서 추가됐다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 제거의 실제 쓰기 범위");
        }

        // 도달한 폭만 기록한다.
        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>수신 관찰은 필드를 읽지 않고 송신 진단에서 복귀한 뒤 현재 개수를 다시 읽는다. 0.957에서 추가됐다.</summary>
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

        if (Read(packet.Span, 4, 1) == 0xffu)
        {
            (assertion ?? throw new InvalidOperationException("메타 참가자 제거의 실제 진단 미연결"))("numPlayers != (unsigned char)-1", PacketFile, ObserveAssertLine);
        }

        // 진단 복귀 뒤 현재 개수를 다시 읽는다.
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Span.Slice(2, 2), unchecked((ushort)(Read(packet.Span, 4, 1) + 5u)));
    }

    /// <summary>원문 42b980의 미사용 추가 인자는 수명 정책으로 재해석하지 않는다. 0.957에서 추가됐다.</summary>
    private static void Destroy(Memory<byte> record, ClientChallengeGlobals challenge, ClientMetaParticipantRemoveHooks hooks)
    {
        bool downward = (Read(record.Span, 0x64, 1) & 0xa0u) == 0;
        NeedDelegate(hooks.Leave)(Read(record.Span, 0xb8), downward);
        challenge.ClearArray(record.Slice(0xbc, 16));
        // 원문의 115DWORD 전체를 같은 순서로 0으로 쓰며 이름표 크기도 마지막 기록 초기화에 포함한다.
        for (int at = 0; at < RawChallengeState.ParticipantBytes; at += 4)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(at, 4), 0);
        }
    }

    /// <summary>범위·유효성·좌석·버전·Chat·파괴 순서로 같은 부분 효과를 둔다. 0.957에서 추가됐다.</summary>
    private static void Remove(Memory<byte> cs, int participant, ClientChallengeGlobals challenge, ClientMetaParticipantRemoveHooks hooks)
    {
        if (participant < 1 || participant > 39)
        {
            NeedDelegate(hooks.Assertion)("0 < pId && pId < CL_NUM_CHAL_PLAYERS", ".\\State.cpp", 0x499);
            NeedDelegate(hooks.Log)("Cannot delete illegal player number " + participant.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\n");
            return;
        }

        Memory<byte> record = cs.Slice(RawChallengeState.ParticipantOffset + participant * RawChallengeState.ParticipantBytes, RawChallengeState.ParticipantBytes);
        uint checking = hooks.Checking is null ? 0 : hooks.Checking.Read();
        if (checking != 0 && (Read(record.Span, 0x64, 1) & 1u) == 0)
        {
            NeedDelegate(hooks.Assertion)("p->isValid()", ChannelFile, 0x4a1);
        }

        if ((Read(record.Span, 0x64, 1) & 1u) == 0)
        {
            NeedDelegate(hooks.Log)("Cannot delete invalid player number " + participant.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\n");
            return;
        }

        uint battle = Read(record.Span, 0xa4);
        if (battle != 0)
        {
            Memory<byte> current = cs.Slice(RawChallengeState.BattleOffset + unchecked((int)battle) * RawChallengeState.BattleBytes, RawChallengeState.BattleBytes);
            uint slot = Read(record.Span, 0xa8);
            if (slot >= 8)
            {
                throw new ArgumentOutOfRangeException(nameof(cs), "메타 참가자 제거의 실제 좌석8칸 범위");
            }

            Put(current.Span, 0x38 + unchecked((int)slot) * 4, 0);
            NeedDelegate(hooks.Battle)(Read(current.Span, 4));
            ClientChallengeGlobals.UpdateBattleVersionBounds(cs, unchecked((int)Read(record.Span, 0xa4)), hooks.Assertion);
        }

        NeedDelegate(hooks.Chat)(Read(record.Span, 4));
        Destroy(record, challenge, hooks);
    }

    /// <summary>packet 개수는 각 하위 호출 뒤 재조회하며 0명인 패킷도 마지막 NSInfo를 갱신한다. 0.957에서 추가됐다.</summary>
    /// <param name="hooks">실제 CS·모드·검사·진단·로그·전투·Chat·하차·NSInfo 장치다.</param>
    /// <param name="packet">원문 0x21 패킷이다.</param>
    public static void Receive(ClientMetaParticipantRemoveHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientChallengeGlobals challenge = Need(hooks.Challenge);
        if (!NeedDelegate(hooks.OwnerModeChallenge)())
        {
            return;
        }

        if (challenge.Current == 0)
        {
            NeedDelegate(hooks.Assertion)("CS", ChallengeFile, 0xd04);
        }

        Memory<byte> cs = challenge.Bytes;
        ReadOnlySpan<byte> bytes = packet.Span;
        uint index = 0;
        // 진입 CS와 현재 packet 요소를 구별하고 새 active/known/송신자 관문을 넣지 않는다.
        while (index < Read(bytes, 4, 1))
        {
            Remove(cs, unchecked((int)Read(bytes, 5 + unchecked((int)index), 1)), challenge, hooks);
            index++;
        }

        NeedDelegate(hooks.RefreshInfo)();
    }

    /// <summary>0x21의 원문 packet 관찰기·버전·가변 길이와 고정 F를 설치한다. 0.957에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 CS·모드·검사·진단·로그·전투·Chat·하차·NSInfo 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientMetaParticipantRemoveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        dispatch.ObservePacket(Type, (packet, sending) => Observe(packet, sending, hooks.Assertion));
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, ushort.MaxValue);
        // 저장된 F는 packet type이 바뀌어도 이 본문을 유지한다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) => Receive(hooks, packet));
    }
}
