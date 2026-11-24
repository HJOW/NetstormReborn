using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x0f Root 로그인의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/o/RawRootLoginReply.cpp 의 ObserveRootLoginReply(40a070)/ReceiveRootLoginReply(4b4250)와
// cpppj/src/client/ClientCampaignTransport.cpp 의 ReceiveClientRootReply/InstallClientMetaGlobalReceivers 를 이식한다.
// 0.953에서 R4-6 전송·수신 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x0f Root 로그인이 선택한 효과의 실제 외부 장치다.
public sealed class ClientRootLoginReplyHooks
{
    /// <summary>같은 실제 로컬 상태 블록 owner다.</summary>
    public LocalMissionStateStorage? State { get; set; }

    /// <summary>00546908의 같은 실제 51×144BYTE 기록이다.</summary>
    public Memory<byte> Zones { get; set; }

    /// <summary>같은 실제 현재 갱신 예약 owner다.</summary>
    public UIntBox? Refresh { get; set; }

    /// <summary>004b4160의 실제 버전 확인이다.</summary>
    public Func<uint, uint, bool>? Version { get; set; }

    /// <summary>현재 마커 삭제/표 정리 부모다.</summary>
    public Action? ClearZones { get; set; }

    /// <summary>phase+44 서버 행 값을 갱신한다.</summary>
    public Action<uint>? Checksum { get; set; }

    /// <summary>현재 서버 목록에 행을 추가한다.</summary>
    public Action<ReadOnlyMemory<byte>>? Server { get; set; }

    /// <summary>현재 서버 목록을 사용자 홈에 저장한다.</summary>
    public Action? SaveServers { get; set; }

    /// <summary>원문 진단의 실제 선택 줄 소비자다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

// 40a070·4b4250 전체이며 메뉴·세션을 요구하지 않는다.
public static class ClientRootLoginReply
{
    /// <summary>원문 0x0f Root 로그인 응답 종류다. 0.953에서 추가됐다.</summary>
    public const uint Type = 0x0f;

    /// <summary>원문 0x0f 수신의 실제 고정 F 주소다. 0.953에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b4250;

    /// <summary>원문 0x0f 관찰기의 실제 진단 파일이다. 0.953에서 추가됐다.</summary>
    private const string PacketFile = "\\Ns\\O\\_p_packets.cpp";

    /// <summary>원문 0x0f 수신의 진단 파일이다. 0.953에서 추가됐다.</summary>
    private const string StateFile = ".\\State.cpp";

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.953에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("Root 로그인 응답의 선택된 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.953에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("Root 로그인 응답의 선택된 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.953에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "Root 로그인 응답의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>원문 지정 폭의 현재 BYTE만 순서대로 쓰고 이웃 필드를 보존한다. 0.953에서 추가됐다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "Root 로그인 응답의 실제 쓰기 범위");
        }

        // 선택된 폭의 낮은 BYTE부터 같은 실제 기록에 쓴다.
        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>원문 char 필드를 부호 확장한다. 0.953에서 추가됐다.</summary>
    private static int SignedByte(ReadOnlySpan<byte> bytes, int offset) => unchecked((sbyte)Read(bytes, offset, 1));

    /// <summary>서버 문자열 개수/길이만 네트워크 바이트 순서를 사용한다. 0.953에서 추가됐다.</summary>
    private static uint NetworkWord(ReadOnlySpan<byte> bytes, int offset) => (Read(bytes, offset, 1) << 8) | Read(bytes, offset + 1, 1);

    /// <summary>제공된 10.71 응답의 버전·선언 길이·개수가 모두 맞을 때만 좁은 3BYTE 호환을 선택한다. 0.953에서 추가됐다.</summary>
    private static bool Compact1071(ReadOnlySpan<byte> packet)
    {
        // 다른 10.71 확장 응답을 좁은 형식으로 추정하지 않는다.
        if (Read(packet, 4, 2) != 10 || Read(packet, 6, 2) != 71 || SignedByte(packet, 10) <= 0 || SignedByte(packet, 11) < 0)
        {
            return false;
        }

        ulong length = (ulong)Read(packet, 12) + 20u + (ulong)Read(packet, 10, 1) * 3u + (ulong)Read(packet, 11, 1) * 5u;
        return length == Read(packet, 2, 2) && length <= (ulong)packet.Length;
    }

    /// <summary>선택된 진단만 요구한다. 정상 복귀 뒤 필드는 같은 현재 물리에서 다시 읽는다. 0.953에서 추가됐다.</summary>
    private static void Assert(Action<string, string, uint>? assertion, string expression, uint line) =>
        (assertion ?? throw new InvalidOperationException("Root 로그인 응답의 실제 진단 미연결"))(expression, PacketFile, line);

    /// <summary>수신 방향은 버퍼를 읽지 않으며 진단 복귀 뒤 현재 필드를 다시 읽는다. 0.953에서 추가됐다.</summary>
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

        if (Read(packet.Span, 12) == 0xffffffffu)
        {
            Assert(assertion, "rootServerLen != (int)-1", 0x3ac);
        }

        if (Read(packet.Span, 10, 1) == 255)
        {
            Assert(assertion, "numChals != (char)-1", 0x3ad);
        }

        if (Read(packet.Span, 11, 1) == 255)
        {
            Assert(assertion, "numFriendsInChal != (char)-1", 0x3ae);
        }

        // 진단 복귀 뒤 현재 필드를 다시 읽고 하위 WORD만 쓴다.
        uint length = Read(packet.Span, 12) + 20u + unchecked((uint)SignedByte(packet.Span, 10)) * 47u + unchecked((uint)SignedByte(packet.Span, 11)) * 5u;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Span.Slice(2, 2), unchecked((ushort)length));
    }

    /// <summary>버전 거부·서버 목록·존·친구·현재 phase 분기를 원문 순서로 수행한다. 0.953에서 추가됐다.</summary>
    /// <param name="hooks">실제 상태·기록·갱신·버전·정리·값·목록·저장 장치다.</param>
    /// <param name="packet">원문 0x0f 패킷이다.</param>
    public static void Receive(ClientRootLoginReplyHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        LocalMissionStateStorage state = Need(hooks.State);
        Memory<byte> zones = hooks.Zones.Length >= RawMetaZoneCleanup.Bytes ? hooks.Zones : throw new InvalidOperationException("Root 로그인 응답의 실제 기록 물리 부족");
        UIntBox refresh = Need(hooks.Refresh);
        ReadOnlySpan<byte> bytes = packet.Span;

        // 버전 거부이면 next12/+50만 쓰고 목록·존을 건드리지 않는다.
        if (!NeedDelegate(hooks.Version)(Read(bytes, 4, 2), Read(bytes, 6, 2)))
        {
            BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0, 4), 12);
            BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0x50, 4), 0);
            return;
        }

        NeedDelegate(hooks.ClearZones)();
        NeedDelegate(hooks.Checksum)(Read(bytes, 16));
        uint count = NetworkWord(bytes, 20);
        int cursor = 22;
        // 원문은 개수를 읽은 뒤 현재 rootServerLen 범위일 때만 다음 문자열을 소비한다.
        for (uint index = 0; index < count; index++)
        {
            if (unchecked((int)Read(bytes, 12)) <= (long)(cursor - 20))
            {
                break;
            }

            uint length = NetworkWord(bytes, cursor);
            cursor += 2;
            byte[] text = new byte[256];
            uint copied = Math.Min(length, 255u);
            // 최대 255BYTE만 복사하고 미리 0인 마지막 BYTE를 남긴다.
            for (uint copy = 0; copy < copied; copy++)
            {
                text[copy] = (byte)Read(bytes, cursor + (int)copy, 1);
            }

            NeedDelegate(hooks.Server)(text);
            cursor += (int)length;
        }

        NeedDelegate(hooks.SaveServers)();
        bool compact = Compact1071(bytes);
        uint stride = compact ? 3u : 47u;
        uint records = 1;
        // 원문은 기록 1부터 순서대로 쓰고 존 번호를 배열 인덱스로 사용하지 않는다.
        for (int index = 0; index < SignedByte(bytes, 10); index++)
        {
            int source = 20 + (int)Read(bytes, 12) + index * (int)stride;
            int target = (index + 1) * RawMetaZoneCleanup.Stride;
            Put(zones.Span, target, unchecked((uint)SignedByte(bytes, source)));
            Put(zones.Span, target + 4, unchecked((uint)SignedByte(bytes, source + 1)));
            Put(zones.Span, target + 8, unchecked((uint)SignedByte(bytes, source + 2)));
            if (!compact)
            {
                Put(zones.Span, target + 132, Read(bytes, source + 43));
                // 원문 strcpy 순서를 유지하고 실제 패킷/표 밖의 읽기·쓰기만 거부한다.
                for (int copy = 0; ; copy++)
                {
                    uint value = Read(bytes, source + 3 + copy, 1);
                    Put(zones.Span, target + 92 + copy, value, 1);
                    if (value == 0)
                    {
                        break;
                    }
                }
            }

            // 10.71에 없는 이름/+84 값은 앞선 실제 정리 결과를 보존하며 값을 만들지 않는다.
            records++;
        }

        // 47BYTE 형식도 3BYTE 기반 친구 위치를 그대로 읽는다.
        for (int index = 0; index < SignedByte(bytes, 11); index++)
        {
            long position = 20L + (long)Read(bytes, 12) + (long)SignedByte(bytes, 10) * 3L + (long)index * 5L;
            if (position < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "Root 로그인 응답의 실제 패킷 앞 친구 위치");
            }

            int source = (int)position;
            // 빈 0번을 포함해 첫 존 번호 일치만 선택한다.
            for (uint zone = 0; zone < records; zone++)
            {
                int target = (int)zone * RawMetaZoneCleanup.Stride;
                if (Read(zones.Span, target) != unchecked((uint)SignedByte(bytes, source)))
                {
                    continue;
                }

                // 현재 0인 첫 친구 슬롯에만 구독자 DWORD를 넣는다.
                for (uint slot = 0; slot < 16; slot++)
                {
                    if (Read(zones.Span, target + 12 + (int)slot * 4) != 0)
                    {
                        continue;
                    }

                    Put(zones.Span, target + 12 + (int)slot * 4, Read(bytes, source + 1));
                    break;
                }

                break;
            }
        }

        if (Read(state.Phase.Span, 0) == 23)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0, 4), 24);
            BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0x34, 4), unchecked((uint)SignedByte(bytes, 9)));
        }
        else
        {
            refresh.Value++;
        }
    }

    /// <summary>0x0f의 원문 packet 관찰기·버전·가변 길이와 고정 F를 설치한다. 0.953에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 상태·기록·갱신·버전·정리·값·목록·저장·진단 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientRootLoginReplyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        dispatch.ObservePacket(Type, (packet, sending) => Observe(packet, sending, hooks.Assertion));
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, ushort.MaxValue);
        // 발신자/nullable 연결/phase/known을 앞단 필터로 추가하지 않는다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) => Receive(hooks, packet));
    }
}
