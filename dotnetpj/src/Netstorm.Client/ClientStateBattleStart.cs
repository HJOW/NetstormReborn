using Netstorm.Core.Original;
using Netstorm.Net;

// 1a/1b 전투 시작 수신의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/o/RawStateBattleStart.cpp 의 ObserveBattleStartZone(40a1e0)/ReceiveBattleStartZone(4b4df0)/ReceiveBattleStartPeer(4b8b20)와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientStateBattleStartReceivers/ReceiveClientBattleStartPeer 를 이식한다.
// 0.951에서 R4-6 전송·수신 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 1a/1b 전투 시작이 선택한 효과의 실제 외부 장치다.
public sealed class ClientStateBattleStartHooks
{
    /// <summary>같은 실제 로컬 상태 블록 owner다.</summary>
    public LocalMissionStateStorage? State { get; set; }

    /// <summary>같은 실제 예약 수이며 State7 캡처/정리와 공유한다.</summary>
    public UIntBox? ZonePending { get; set; }

    /// <summary>현재 게임 포트를 읽는다.</summary>
    public Func<uint>? GamePort { get; set; }

    /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
    public UIntBox? Subscriber { get; set; }

    /// <summary>현재 요청 DWORD를 읽는다.</summary>
    public Func<uint>? RecordRequested { get; set; }

    /// <summary>같은 실제 현재 검사 뷰다.</summary>
    public RawCheckingFlag? Checking { get; set; }

    /// <summary>현재 서버 여부를 읽는다.</summary>
    public Func<bool>? Server { get; set; }

    /// <summary>원문 진단의 실제 선택 줄 소비자다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>같은 실제 가변 복제 장부 토큰을 반환한다.</summary>
    public Func<Memory<byte>, uint>? Duplicate { get; set; }

    /// <summary>현재 이름으로 같은 실제 기록을 닫고 새로 연다.</summary>
    public Action<string>? OpenRecord { get; set; }

    /// <summary>실제 현재 연결을 순서대로 닫는다.</summary>
    public Action<int>? Disconnect { get; set; }

    /// <summary>1b의 현재 쓰기 뒤 같은 실제 상태 처리기를 동기 실행한다.</summary>
    public Action? Run { get; set; }

    /// <summary>같은 실제 외부 장치 묶음을 복사한다.</summary>
    /// <returns>같은 값을 가리키는 새 묶음이다.</returns>
    public ClientStateBattleStartHooks Clone()
    {
        // 원문 호출 동안만 같은 실제 owner를 빌리며 새 owner를 합성하지 않는다.
        return (ClientStateBattleStartHooks)MemberwiseClone();
    }
}

// 40a1e0·4b4df0·4b8b20 전체이며 메뉴·세션을 요구하지 않는다.
public static class ClientStateBattleStart
{
    /// <summary>원문 1a 전투 시작 구역 종류다. 0.951에서 추가됐다.</summary>
    public const uint ZoneType = 0x1a;

    /// <summary>원문 1b 전투 시작 상대 종류다. 0.951에서 추가됐다.</summary>
    public const uint PeerType = 0x1b;

    /// <summary>원문 1a 수신의 실제 고정 F 주소다. 0.951에서 추가됐다.</summary>
    public const uint ZoneEntry = 0x004b4df0;

    /// <summary>원문 1b 수신의 실제 고정 F 주소다. 0.951에서 추가됐다.</summary>
    public const uint PeerEntry = 0x004b8b20;

    /// <summary>원문 1b 수신의 실제 고정 길이다. 0.951에서 추가됐다.</summary>
    public const ushort PeerLength = 38;

    /// <summary>원문 1a 송신 관찰기 진단 줄 번호다(0x48f). 0.951에서 추가됐다.</summary>
    public const uint ZoneAssertLine = 0x48f;

    /// <summary>원문 1a 수신 진단 파일이다. 0.951에서 추가됐다.</summary>
    private const string ZoneStateFile = ".\\State.cpp";

    /// <summary>원문 1a 수신 진단 줄 번호다(0xdef). 0.951에서 추가됐다.</summary>
    private const uint ZoneStateLine = 0xdef;

    /// <summary>원문 1b 수신 진단 줄 번호다(0xe39). 0.951에서 추가됐다.</summary>
    private const uint PeerStateLine = 0xe39;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.951에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("전투 시작 수신의 선택된 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.951에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("전투 시작 수신의 선택된 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.951에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "전투 시작 수신의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>원문 지정 폭의 현재 BYTE만 순서대로 쓰고 이웃 필드를 보존한다. 0.951에서 추가됐다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "전투 시작 수신의 실제 쓰기 범위");
        }

        // 선택된 폭의 낮은 BYTE부터 같은 현재 버퍼에 쓴다.
        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>sending이 0이면 버퍼도 읽지 않으며 진단 복귀 뒤 현재 BYTE를 다시 읽는다. 0.951에서 추가됐다.</summary>
    /// <param name="packet">같은 현재 버퍼의 실제 물리다.</param>
    /// <param name="sending">원문 송신 방향 여부다.</param>
    /// <param name="hooks">실제 진단 장치다.</param>
    public static void ObserveZone(Memory<byte> packet, uint sending, ClientStateBattleStartHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        // 수신 방향은 실제 패킷 필드를 읽기 전에 복귀한다.
        if (sending == 0)
        {
            return;
        }

        // 원문은 진단 복귀 뒤 그 현재 BYTE를 다시 읽는다.
        if (Read(packet.Span, 14, 1) == 0xff)
        {
            NeedDelegate(hooks.Assertion)("numClients != (unsigned char)-1", "\\Ns\\O\\_p_packets.cpp", ZoneAssertLine);
        }

        // 현재 BYTE를 다시 읽어 같은 선언 WORD를 쓴다.
        uint count = Read(packet.Span, 14, 1);
        Put(packet.Span, 2, count * 71u + 15u, 2);
    }

    /// <summary>진단→next zlocal 쓰기→7/포트→복제/예약→선택적 기록→10/11 정리 순서다. 0.951에서 추가됐다.</summary>
    /// <param name="hooks">실제 상태·예약·포트·구독자·요청·검사·진단·복제·기록·연결 장치다.</param>
    /// <param name="packet">원문 1a 패킷이다.</param>
    public static void ReceiveZone(ClientStateBattleStartHooks hooks, Memory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        LocalMissionStateStorage state = Need(hooks.State);
        UIntBox pending = Need(hooks.ZonePending);
        uint gamePort = NeedDelegate(hooks.GamePort)();
        UIntBox subscriber = Need(hooks.Subscriber);
        uint requested = NeedDelegate(hooks.RecordRequested)();
        // 검사가 비영일 때만 현재 phase를 조회한다.
        uint checking = hooks.Checking is null ? 0 : hooks.Checking.Read();
        if (checking != 0)
        {
            uint phase = Read(state.Phase.Span, 0);
            if (phase != 29 && phase != 33)
            {
                NeedDelegate(hooks.Assertion)("currState.state == S_META_CHAL_CONNECTED || S_META_CHAL_CONNECTED_MONITOR", ZoneStateFile, ZoneStateLine);
            }
        }

        // PE00513870의 zlocal,*와 끝 1BYTE만 쓰며 상위 3BYTE는 남긴다.
        System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0x54, 4), 0x636f6c7au);
        System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0x58, 4), 0x2a2c6c61u);
        uint tail = System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(state.Next.Span.Slice(0x5c, 4));
        System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0x5c, 4), tail & 0xffffff00u);
        System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0, 4), 7);
        System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0x7c, 4), gamePort);
        uint token = NeedDelegate(hooks.Duplicate)(packet);
        System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0x80, 4), token);
        pending.Value++;

        // 복제/observer 뒤의 현재 packet+10과 자기 구독자로 같은 파일을 연다.
        if (requested != 0)
        {
            uint address = Read(packet.Span, 10);
            string name = string.Format(System.Globalization.CultureInfo.InvariantCulture, "{0:X8}.{1:X}.c.dat", address, subscriber.Value);
            NeedDelegate(hooks.OpenRecord)(name);
        }

        // 수신 중인 노드의 지연 반납/깊이는 같은 실제 연결 owner가 처리한다.
        NeedDelegate(hooks.Disconnect)(10);
        NeedDelegate(hooks.Disconnect)(11);
    }

    /// <summary>서버/예약 조기 반환→진단/기록→현재 주소/포트/7→직접 Run 순서다. 0.951에서 추가됐다.</summary>
    /// <param name="hooks">실제 상태·예약·포트·구독자·요청·검사·서버·진단·기록·실행 장치다.</param>
    /// <param name="packet">원문 1b 패킷이다.</param>
    public static void ReceivePeer(ClientStateBattleStartHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        LocalMissionStateStorage state = Need(hooks.State);
        UIntBox pending = Need(hooks.ZonePending);
        uint gamePort = NeedDelegate(hooks.GamePort)();
        UIntBox subscriber = Need(hooks.Subscriber);
        uint requested = NeedDelegate(hooks.RecordRequested)();
        bool server = NeedDelegate(hooks.Server)();
        // 원문 두 전역 관문은 검사·파일/패킷 접근보다 먼저다.
        if (server || pending.Value != 0)
        {
            return;
        }

        uint checking = hooks.Checking is null ? 0 : hooks.Checking.Read();
        if (checking != 0)
        {
            uint phase = Read(state.Phase.Span, 0);
            if (phase != 29 && phase != 33)
            {
                NeedDelegate(hooks.Assertion)("currState.state == S_META_CHAL_CONNECTED || S_META_CHAL_CONNECTED_MONITOR", ZoneStateFile, PeerStateLine);
            }
        }

        // 1a의 +10과 구별되는 실제 +4이며 열기가 복귀한 뒤 주소를 읽는다.
        if (requested != 0)
        {
            uint address = Read(packet.Span, 4);
            string name = string.Format(System.Globalization.CultureInfo.InvariantCulture, "{0:X8}.{1:X}.c.dat", address, subscriber.Value);
            NeedDelegate(hooks.OpenRecord)(name);
        }

        // NUL도 같은 현재 블록에 한 BYTE씩 복사하며 사전 문자열 검사/전체 복사를 하지 않는다.
        int offset = 0;
        while (true)
        {
            uint value = Read(packet.Span, 8 + offset, 1);
            Put(state.Current.Span, 0x54 + offset, value, 1);
            offset++;
            if (value == 0)
            {
                break;
            }
        }

        System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Current.Span.Slice(0x7c, 4), gamePort);
        System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(state.Current.Span.Slice(0, 4), 7);
        // next 요청/명령 큐 없이 같은 실제 상태 처리기를 수신 중 직접 실행한다.
        NeedDelegate(hooks.Run)();
    }

    /// <summary>1a의 원문 observer·버전·가변 길이와 1b의 원문 빈 observer·버전·고정 길이를 설치한다. 0.951에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 상태·예약·포트·구독자·요청·검사·서버·진단·복제·기록·연결·실행 장치다.</param>
    /// <param name="selectSource">실제 배송 connection의 Source 선택 장치다. 비어 있으면 명시 실패한다.</param>
    /// <param name="dispatchBattle">실제 UI State 부모의 배송 분배 장치다. source와 함께 호출된다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientStateBattleStartHooks hooks, Func<PacketConnection?, NativeBattleSource> selectSource, Action<NativeBattleSource, bool> dispatchBattle)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(selectSource);
        ArgumentNullException.ThrowIfNull(dispatchBattle);

        dispatch.ObservePacket(ZoneType, (packet, sending) => ObserveZone(packet, sending, hooks));
        dispatch.Version(ZoneType, 1, 0);
        dispatch.RequireLength(ZoneType, ushort.MaxValue);
        dispatch.RegisterCanonicalDirect(ZoneType, ZoneEntry, (_, connection, packet) =>
        {
            // 진단/next 주소·7·포트 이후 원문의 observer/현재 복제를 수행한다.
            NativeBattleSource? selected = null;
            ClientStateBattleStartHooks zoneHooks = hooks.Clone();
            Func<Memory<byte>, uint> rawDuplicate = hooks.Duplicate ?? throw new InvalidOperationException("전투 시작 수신의 선택된 실제 장치 미연결");
            zoneHooks.Duplicate = packet =>
            {
                selected = selectSource(connection) ?? throw new InvalidOperationException("전투 시작의 실제 Source 미연결");
                return rawDuplicate(packet);
            };
            byte[] copy = packet.ToArray();
            ReceiveZone(zoneHooks, copy);
            if (selected is null)
            {
                throw new InvalidOperationException("전투 시작의 실제 Source 미선택");
            }

            // 원래 next7만 실제 UI State 부모에 인계하며 즉시 Run/새 명령을 만들지 않는다.
            dispatchBattle(selected, false);
        });
        dispatch.Observe(PeerType, static _ => { });
        dispatch.Version(PeerType, 1, 0);
        dispatch.RequireLength(PeerType, PeerLength);
        dispatch.RegisterCanonicalDirect(PeerType, PeerEntry, (_, connection, packet) =>
        {
            // 현재 실제 node의 Source 선택은 같은 본문의 run 위치까지 지연한다.
            ClientStateBattleStartHooks peerHooks = hooks.Clone();
            peerHooks.Run = () =>
            {
                NativeBattleSource source = selectSource(connection) ?? throw new InvalidOperationException("전투 시작의 실제 Source 미연결");
                dispatchBattle(source, true);
            };
            // server/pending 자연 반환 앞에 Source를 요구하지 않는다.
            ReceivePeer(peerHooks, packet);
        });
    }
}
