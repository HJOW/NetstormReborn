using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 Client 표에 남는 다섯 원문 길이 관찰기를 연결한다.
namespace Netstorm.Client;

/// <summary>ClientCampaignTransport.cpp가 선택하는3f/30/7a/5f/7d의 순수 송신 관찰기다.</summary>
public static class ClientVariablePacketObservers
{
    /// <summary>실제 기본 설치 순서다. 수신 F는 이 부모에서 교체하지 않는다.</summary>
    private static readonly uint[] Types = [0x3f, 0x30, 0x7a, 0x5f, 0x7d];

    /// <summary>필요한 현재 필드의 최소 물리 크기를 확인한다.</summary>
    private static void NeedBytes(ReadOnlySpan<byte> bytes, int count)
    {
        if (bytes.Length < count) throw new ArgumentOutOfRangeException(nameof(bytes), "가변 관찰기 현재 필드 물리 부족");
    }

    /// <summary>선택한 원문 진단만 요구한다. 정상 복귀 뒤 필드를 다시 읽는다.</summary>
    private static void Assert(Action<string, string, uint>? assertion, string expression, uint line) =>
        (assertion ?? throw new InvalidOperationException("가변 관찰기 실제 진단 미연결"))(expression, @"\Ns\O\_p_packets.cpp", line);

    /// <summary>수신은 Span도 읽지 않는다. 송신은 현재 필드와 원문 지정 폭으로 선언WORD만 쓴다.</summary>
    public static void Observe(uint type, Memory<byte> packet, uint sending, Action<string, string, uint>? assertion = null)
    {
        if (sending == 0) return;
        Span<byte> bytes = packet.Span;
        uint length;
        switch (type)
        {
            case 0x3f:
                NeedBytes(bytes, 27);
                if (bytes[26] == 255) Assert(assertion, "sidCount != (unsigned char)-1", 0x7c9);
                length = (uint)bytes[26] * 2 + 27;
                break;
            case 0x30:
                NeedBytes(bytes, 15);
                if (BinaryPrimitives.ReadInt16LittleEndian(bytes.Slice(13, 2)) == -1) Assert(assertion, "nodeCount != (short)-1", 0x716);
                length = unchecked((uint)(BinaryPrimitives.ReadInt16LittleEndian(bytes.Slice(13, 2)) + 24));
                break;
            case 0x7a:
                NeedBytes(bytes, 9);
                if (bytes[8] == 255) Assert(assertion, "dataLength != (unsigned char)-1", 0xb83);
                length = (uint)bytes[8] + 9;
                break;
            case 0x5f:
                NeedBytes(bytes, 10);
                if (BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(6, 4)) == uint.MaxValue) Assert(assertion, "count != (int)-1", 0x96b);
                length = ((uint)BinaryPrimitives.ReadUInt16LittleEndian(bytes.Slice(6, 2)) + 5) * 2;
                break;
            case 0x7d:
                NeedBytes(bytes, 18);
                ushort count = BinaryPrimitives.ReadUInt16LittleEndian(bytes.Slice(16, 2));
                if (count == ushort.MaxValue) throw new InvalidOperationException("원본 상태 타이머 대상 수 진단bda");
                length = ((uint)count + 11) * 2;
                break;
            default:
                throw new ArgumentOutOfRangeException(nameof(type), "미등록 가변 관찰기 종류");
        }
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.Slice(2, 2), unchecked((ushort)length));
    }

    /// <summary>다섯 observer/version1/policy0/ffff만 설치하고 기존 수신 F와 다른 표를 유지한다.</summary>
    public static void Install(RawPacketDispatch dispatch, RawSurfaceStorage storage)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(storage);
        // 실제 Source의 기본 순서로 각 관찰기만 교체한다.
        foreach (uint type in Types)
        {
            dispatch.ObservePacket(type, (packet, sending) => Observe(type, packet, sending, storage.AssertDisplay));
            dispatch.Version(type, 1, 0);
            dispatch.RequireLength(type, 0xffff);
        }
    }
}
