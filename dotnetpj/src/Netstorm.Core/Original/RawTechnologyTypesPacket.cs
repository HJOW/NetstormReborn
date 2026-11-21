using System.Buffers.Binary;

// 현재 기술 목록의 순서와 부분 저장을 원문 그대로 유지한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawTechnologyTypesPacket.cpp의 목록 누적·수신·송신 관찰 본체다.</summary>
public static class RawTechnologyTypesPacket
{
    /// <summary>현재 선택한 BYTE만 확인하며 뒤쪽 물리를 미리 읽지 않는다.</summary>
    private static byte Read(ReadOnlySpan<byte> bytes, int index) => (uint)index < (uint)bytes.Length
        ? bytes[index] : throw new ArgumentOutOfRangeException(nameof(bytes), "69h 타입 목록의 선택된 현재 BYTE 물리 부족");

    /// <summary>첫NUL까지 중복을 제외하고 현재256BYTE의 첫빈칸에만 추가한다. 부족 전의 저장은 유지한다.</summary>
    public static void Add(ReadOnlySpan<byte> input, Span<byte> typeUse)
    {
        if (typeUse.Length != 256) throw new ArgumentException("기술 사용 목록의 실제256BYTE 필요", nameof(typeUse));
        // 입력을 복사하거나 별도 길이로 제한하지 않고 다음 BYTE를 순서대로 읽는다.
        for (int source = 0; ; source++)
        {
            byte value = Read(input, source);
            if (value == 0) return;
            int index = 0;
            // 같은 현재 목록의 첫NUL 또는 첫일치에서만 멈춘다.
            while (true)
            {
                byte current = Read(typeUse, index);
                if (current == 0 || current == value) break;
                index++;
            }
            if (Read(typeUse, index) == 0) typeUse[index] = Read(input, source);
        }
    }

    /// <summary>payload+5 합집합 뒤 기존/신규 전체 현재 목록을 다시 표시한다. lenBYTE는 수신 경계가 아니다.</summary>
    public static void Receive(ReadOnlyMemory<byte> packet, Memory<byte> typeUse, Action<uint>? indicate)
    {
        if (packet.Length < 5) throw new ArgumentOutOfRangeException(nameof(packet), "69h payload+5의 실제 물리 부족");
        Add(packet.Span[5..], typeUse.Span);
        // 각 실제 표시 효과 뒤 다음 현재 항목을 다시 선택하고 빈 목록은 장치를 요구하지 않는다.
        for (int index = 0; ; index++)
        {
            byte current = Read(typeUse.Span, index);
            if (current == 0) return;
            (indicate ?? throw new InvalidOperationException("69h의 현재 기술표시 장치 미연결"))(current);
        }
    }

    /// <summary>송신BYTE+4의 진단 뒤 현재BYTE+5를 선언WORD에 쓴다. 수신은 Span 조회도 생략한다.</summary>
    public static void Observe(Memory<byte> packet, uint sending, Action<string, string, uint>? assertion)
    {
        if (sending == 0) return;
        Span<byte> bytes = packet.Span;
        if (Read(bytes, 4) == 0xff)
            (assertion ?? throw new InvalidOperationException("69h의 현재 원문 진단 장치 미연결"))("len != (unsigned char)-1", @"\Ns\O\_p_packets.cpp", 0xa5f);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.Slice(2, 2), (ushort)(Read(bytes, 4) + 5));
    }
}
