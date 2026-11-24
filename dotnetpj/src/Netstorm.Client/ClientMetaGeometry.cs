using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x26 메타 geometry의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/NativeMetaGeometry.cpp 의 ObserveNativeMetaGeometry(40a3e0)/ReceiveNativeMetaGeometry(4b45a0)와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaGlobalReceivers 를 이식한다.
// 0.954에서 R4-6 전송·수신 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x26 메타 geometry가 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaGeometryHooks
{
    /// <summary>같은 실제 최초 배치 좌표 owner다.</summary>
    public ChallengeGeometry? Geometry { get; set; }

    /// <summary>005467d4의 같은 현재 모드 owner다.</summary>
    public UIntBox? Mode { get; set; }

    /// <summary>실제 화면 논리 크기와 메타 크기로 현재 한계를 둔다.</summary>
    public Action<float, float>? Limits { get; set; }

    /// <summary>원문 진단의 실제 선택 줄 소비자다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

// 40a3e0·4b45a0 전체이며 CS0/메타 모드/송신자/World를 앞에서 요구하지 않는다.
public static class ClientMetaGeometry
{
    /// <summary>원문 0x26 메타 geometry 종류다. 0.954에서 추가됐다.</summary>
    public const uint Type = 0x26;

    /// <summary>원문 0x26 수신의 실제 고정 F 주소다. 0.954에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b45a0;

    /// <summary>원문 0x26 관찰기의 실제 진단 파일이다. 0.954에서 추가됐다.</summary>
    private const string PacketFile = "\\Ns\\O\\_p_packets.cpp";

    /// <summary>원문 x87의 BYTE 높이에 더하는 PE float 상수 비트다(9.727272987365723). 0.954에서 추가됐다.</summary>
    private const uint HeightBits = 0x411ba2e9u;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.954에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 geometry의 선택된 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.954에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 geometry의 선택된 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.954에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 geometry의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>선택된 진단만 요구한다. 정상 복귀 뒤 다음 필드와 최종 길이를 현재 패킷에서 다시 읽는다. 0.954에서 추가됐다.</summary>
    private static void Assert(Action<string, string, uint>? assertion, string expression, uint line) =>
        (assertion ?? throw new InvalidOperationException("메타 geometry의 실제 진단 미연결"))(expression, PacketFile, line);

    /// <summary>송신일 때 두 미작성 길이 진단을 하고 현재 mod/contact 길이로 WORD 길이를 쓴다. 0.954에서 추가됐다.</summary>
    /// <param name="packet">같은 현재 버퍼의 실제 물리다.</param>
    /// <param name="sending">원문 송신 방향 여부다.</param>
    /// <param name="assertion">실제 진단 장치다.</param>
    public static void Observe(Memory<byte> packet, uint sending, Action<string, string, uint>? assertion)
    {
        // 수신 방향은 버퍼를 읽지 않는다.
        if (sending == 0)
        {
            return;
        }

        if (Read(packet.Span, 4, 2) == 0xffffu)
        {
            Assert(assertion, "modLen != (unsigned short)-1", 0x5eb);
        }

        if (Read(packet.Span, 6, 1) == 0xffu)
        {
            Assert(assertion, "contactLen != (unsigned char)-1", 0x5ec);
        }

        // 각 진단에서 복귀한 뒤 다음 필드와 최종 길이를 현재 패킷에서 다시 읽는다.
        uint length = Read(packet.Span, 6, 1) + Read(packet.Span, 4, 2) + 9u;
        if (packet.Length < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "메타 geometry의 길이 WORD 물리 부족");
        }

        BinaryPrimitives.WriteUInt16LittleEndian(packet.Span.Slice(2, 2), unchecked((ushort)length));
    }

    /// <summary>화면 한계→mode 0→조건부 contact 원점/두 반경→mode 1 순서를 수행한다. 0.954에서 추가됐다.</summary>
    /// <param name="hooks">실제 좌표·모드·한계 장치다.</param>
    /// <param name="packet">원문 0x26 패킷이다.</param>
    public static void Receive(ClientMetaGeometryHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ChallengeGeometry geometry = Need(hooks.Geometry);
        UIntBox mode = Need(hooks.Mode);
        ReadOnlySpan<byte> bytes = packet.Span;
        float width = Read(bytes, 7, 1);
        float height = (float)((double)Read(bytes, 8, 1) + (double)BitConverter.UInt32BitsToSingle(HeightBits));
        // 원문 x87의 BYTE 높이+PE float는 DWORD 스택에 저장된 뒤 같은 한계에 전달한다.
        NeedDelegate(hooks.Limits)(width, height);
        mode.Value = 0;
        uint count = Read(bytes, 6, 1) >> 2;
        if (count <= 2)
        {
            return;
        }

        int at = 9 + (int)Read(bytes, 4, 2);
        geometry.OriginX = BitConverter.UInt32BitsToSingle(Read(bytes, at));
        geometry.OriginY = BitConverter.UInt32BitsToSingle(Read(bytes, at + 4));
        at += 8;
        // 첫 세 DWORD일 때 둘째 반경의 오래된 값을 보존하고 넷 이상이어도 추가 contact는 읽지 않는다.
        for (uint index = 0; index < 2 && index < count - 2u; index++, at += 4)
        {
            float value = BitConverter.UInt32BitsToSingle(Read(bytes, at));
            if (index == 0)
            {
                geometry.FirstRadius = value;
            }
            else
            {
                geometry.SecondRadius = value;
            }
        }

        mode.Value = 1;
    }

    /// <summary>0x26의 원문 packet 관찰기·버전·가변 길이와 고정 F를 설치한다. 0.954에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 좌표·모드·한계·진단 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientMetaGeometryHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        dispatch.ObservePacket(Type, (packet, sending) => Observe(packet, sending, hooks.Assertion));
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, ushort.MaxValue);
        // CS0/메타 모드/송신자/known/World 선행 관문이 없는 원문 F다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) => Receive(hooks, packet));
    }
}
