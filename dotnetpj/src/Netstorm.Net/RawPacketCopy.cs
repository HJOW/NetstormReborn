using System.Buffers.Binary;

// 원본 observer 뒤의 현재 길이와 앞방향 DWORD/BYTE 복제를 같은 소유 메모리에 적용한다.
namespace Netstorm.Net;

/// <summary>원문NULL 반환과 길이0의 실제 할당을 구별하는 복제 결과다.</summary>
public readonly record struct RawPacketCopyResult
{
    /// <summary>반환 주소가 실제로 있는지이며 길이0과 독립이다.</summary>
    public bool Present { get; init; }
    /// <summary>원문 길이만큼의 같은 실제 목적지다.외부 목적지는 별도 복제하지 않는다.</summary>
    public Memory<byte> Bytes { get; init; }
    /// <summary>자동 할당 분기만 소유하는 실제 배열이다.</summary>
    public byte[]? Allocation { get; init; }
}

/// <summary>cpppj/src/o/RawPacketCopy.cpp의4dece0 전체 복제 본체다.</summary>
public static class RawPacketCopy
{
    /// <summary>실제로 읽는 위치에서만 물리 범위를 검사하며 앞선 효과는 되돌리지 않는다.</summary>
    private static uint Read(Memory<byte> bytes, int at, int width) => width switch
    {
        1 => bytes.Span[at], 2 => BinaryPrimitives.ReadUInt16LittleEndian(bytes.Span[at..]),
        4 => BinaryPrimitives.ReadUInt32LittleEndian(bytes.Span[at..]), _ => throw new ArgumentOutOfRangeException(nameof(width)),
    };
    /// <summary>같은DWORD의 읽기 완료 뒤 해당 실제 목적지에만 쓴다.</summary>
    private static void Write(Memory<byte> bytes, int at, uint value, int width)
    {
        if (width == 4) BinaryPrimitives.WriteUInt32LittleEndian(bytes.Span[at..], value);
        else bytes.Span[at] = unchecked((byte)value);
    }
    /// <summary>NULL 입력→관찰1→현재WORD→선택할당/용량거부→앞방향 복사다.같은 배열의 중첩도 원문 순서로 처리한다.</summary>
    public static RawPacketCopyResult Duplicate(Memory<byte>? packet, int capacity, Memory<byte>? destination, PacketRegistrationTable table)
    {
        if (packet is not { } source) return default;
        ArgumentNullException.ThrowIfNull(table);
        PacketRegistration entry = table.Registration(Read(source, 1, 1)).Snapshot();
        using IDisposable? pin = entry.Pin(); entry.NotifyObserver(source, 1);
        uint length = Read(source, 2, 2); byte[]? allocation = null; bool present;
        Memory<byte> target;
        if (destination is null && capacity == 0)
        {
            allocation = new byte[length]; target = allocation; present = true;
        }
        else
        {
            if (capacity < (int)length) return default;
            present = destination.HasValue; target = destination.GetValueOrDefault();
        }
        int offset = 0;
        // 원본MOVSD처럼 같은DWORD 전체를 먼저 읽고 낮은 주소부터 덮어쓴다.
        for (uint word = 0; word < (length >> 2); word++, offset += 4) Write(target, offset, Read(source, offset, 4), 4);
        // 마지막0~3바이트도 같은 앞방향이며 memmove의 중첩 보정을 넣지 않는다.
        for (uint value = 0; value < (length & 3); value++, offset++) Write(target, offset, Read(source, offset, 1), 1);
        return new() { Present = present, Bytes = target.Slice(0, (int)length), Allocation = allocation };
    }
    /// <summary>같은 분배기의 실제 현재표만 빌리며 수신기/시계/로그 효과를 더하지 않는다.</summary>
    public static RawPacketCopyResult Duplicate(Memory<byte>? packet, int capacity, Memory<byte>? destination, RawPacketDispatch dispatch)
        => Duplicate(packet, capacity, destination, dispatch.Registrations);
}

/// <summary>Client 수명의 실제 전체 복제본과 원문 포인터 대용DWORD를 소유한다.</summary>
public sealed class RawPacketCopies
{
    /// <summary>현재 반환되지 않은 실제 전체 바이트 배열이다.</summary>
    private readonly Dictionary<uint, byte[]> _copies = [];
    /// <summary>NULL0과 구별하고 반납해도 재사용하지 않는 다음DWORD다.</summary>
    private uint _nextToken = 1;
    /// <summary>실제 완성된 배열만 장부에 공개하며 등록 실패로 앞선 observer 효과를 되돌리지 않는다.</summary>
    private uint Hold(byte[] allocation)
    {
        ArgumentNullException.ThrowIfNull(allocation);
        if (_nextToken == 0) throw new OverflowException("소유 패킷 복제의 DWORD token 소진");
        uint token = _nextToken; _copies.Add(token, allocation); _nextToken = unchecked(token + 1); return token;
    }
    /// <summary>실제 공유표의 관찰1/현재길이/복제 성공만 장부에 등록하고NULL은0을 반환한다.</summary>
    public uint Duplicate(Memory<byte>? packet, PacketRegistrationTable table)
    {
        RawPacketCopyResult copy = RawPacketCopy.Duplicate(packet, 0, null, table);
        return copy.Present ? Hold(copy.Allocation ?? throw new InvalidOperationException("소유 복제의 실제 배열 누락")) : 0;
    }
    /// <summary>같은 분배기의 실제표를 사용하는 소유 복제 입구다.</summary>
    public uint Duplicate(Memory<byte>? packet, RawPacketDispatch dispatch) => Duplicate(packet, dispatch.Registrations);
    /// <summary>이미 완성된 같은 실제 배열을 인계하며 새 복제나 observer를 더하지 않는다.</summary>
    public uint Adopt(byte[] allocation) => Hold(allocation);
    /// <summary>현재 token의 같은 전체 배열을 호출자가 잠시 빌린다.반납 뒤 기존 대여 배열 자체는 유지된다.</summary>
    public byte[] Acquire(uint token) => token != 0 && _copies.TryGetValue(token, out byte[]? allocation) ? allocation : throw new ArgumentOutOfRangeException(nameof(token), "소유 패킷 token 물리 누락");
    /// <summary>NULL은 무효과이며 실제 현재 token만 한 번 반납한다.</summary>
    public void Release(uint token)
    {
        if (token != 0 && !_copies.Remove(token)) throw new ArgumentOutOfRangeException(nameof(token), "소유 패킷 token 해제 누락/중복");
    }
}
