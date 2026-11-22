using System.Buffers.Binary;

// RawGunAnimation.cpp의 같은 공통 접두와 수신/송신 비트 폭을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>썬 캐논의 원문 회전·발사·원위치 파생 선택이다.</summary>
public enum GunAnimationKind
{
    /// <summary>표적 방향으로 회전한다.</summary>
    SunRotate,
    /// <summary>현재 표적을 향해 발사한다.</summary>
    SunFire,
    /// <summary>원문 방향표로 원위치에 돌아간다.</summary>
    SunRest,
}
/// <summary>수신 전용 생성자의 네 가지 기본 파생이며 일반 생성과 구별한다.</summary>
public enum GunAnimationReceiveKind
{
    /// <summary>썬 캐논의 수신 기본발사다.</summary>
    SunFire,
    /// <summary>썬 캐논의 수신 기본회전이다.</summary>
    SunRotate,
    /// <summary>썬 아처의 수신 기본발사다.</summary>
    ArcherFire,
    /// <summary>윈드 아처의 수신 기본회전이다.</summary>
    ArcherRotate,
}

/// <summary>0x40 원문 객체의 패딩까지 소유하는 실제 이미지다.</summary>
public sealed class GunAnimationFields
{
    /// <summary>outer/내장 데이터가 함께 읽고 쓰는 원문 바이트다.</summary>
    public byte[] Bytes { get; } = new byte[64];
    /// <summary>미작성 패딩 검사나 일반 기본값에 사용할 최초 바이트 패턴이다.</summary>
    public GunAnimationFields(byte fill = 0) => Array.Fill(Bytes, fill);
    /// <summary>외부 객체 이미지와 소유권을 분리하는 값 복사다.</summary>
    public GunAnimationFields(ReadOnlySpan<byte> bytes) { if (bytes.Length != Bytes.Length) throw new ArgumentException("총 애니메이션64BYTE 필요"); bytes.CopyTo(Bytes); }
    /// <summary>현재 outer의 Form DWORD를 WORD 식별자로 읽는다.</summary>
    public ushort Form() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(4)));
    /// <summary>현재 부모 DWORD의 낮은 WORD를 읽는다.</summary>
    public ushort Parent() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(8)));
    /// <summary>현재 통지 대상 Gun Form의 원문 WORD다.</summary>
    public ushort GunForm() => BinaryPrimitives.ReadUInt16LittleEndian(Bytes.AsSpan(42));
    /// <summary>현재 피해 표적의 원문 WORD다.</summary>
    public ushort Target() => BinaryPrimitives.ReadUInt16LittleEndian(Bytes.AsSpan(44));
    /// <summary>다음 실행 시각의 실제 double이다.</summary>
    public double Time() => BinaryPrimitives.ReadDoubleLittleEndian(Bytes.AsSpan(48));
    /// <summary>원위치가 순차 소비하는 부호 있는 방향 DWORD다.</summary>
    public int RestDirection() => BinaryPrimitives.ReadInt32LittleEndian(Bytes.AsSpan(56));
}

/// <summary>공통 codec은 다른 파생의 실제 span도 그대로 받는다.</summary>
public sealed partial class RawGunAnimation
{
    /// <summary>네 수신 생성자가 지정하는 필드만 쓰며 좌표/표적/패딩은 보존한다.</summary>
    public static void InitializeReceived(Span<byte> outer, GunAnimationReceiveKind kind)
    {
        bool archer = kind == GunAnimationReceiveKind.ArcherFire; if (outer.Length < (archer ? 88 : 56)) throw new ArgumentException("애니메이션 수신 객체 폭 부족");
        Put(outer, 8, 0); Put(outer, 4, 0); Put(outer, 32, kind is GunAnimationReceiveKind.SunRotate or GunAnimationReceiveKind.ArcherRotate ? 0x50b0c0u : 0x50b0c4u); Put(outer, 48, 0); Put(outer, 52, 0); outer[40] = 0xff;
        if (archer) { Put(outer, 64, 0); Put(outer, 76, 0); Put(outer, 80, 0); Put(outer, 0, 0x50b1c0); Put(outer, 32, 0x50b1bc); Put(outer, 68, 0x43340000); Put(outer, 72, 0x43340000); return; }
        if (kind == GunAnimationReceiveKind.SunFire) { Put(outer, 0, 0x50b318); Put(outer, 32, 0x50b310); return; }
        Put(outer, 0, kind == GunAnimationReceiveKind.SunRotate ? 0x50b260u : 0x50b450u); Put(outer, 32, kind == GunAnimationReceiveKind.SunRotate ? 0x50b258u : 0x50b448u);
    }
    /// <summary>부모DWORD와 두WORD/8BYTE를 순서대로 읽고 뒤 실패에도 앞선 쓰기를 보존한다.</summary>
    public static uint ReadProcess(Span<byte> outer, ReadOnlySpan<byte> packet)
    {
        if (packet.Length < 9) throw new ArgumentException("애니메이션 수신9BYTE 머리 부족"); byte dirty = packet[8]; int offset = 9; uint parent = 0;
        if ((dirty & 1) != 0) { Require(packet.Length, offset, 4); parent = Read(packet, offset); offset += 4; }
        // 미선택 필드는 빈 객체에서도 저장하지 않으며 각 선택 직전에만 폭을 검사한다.
        foreach ((int bit, int target, int width) in DataFields)
        {
            if ((dirty & bit) == 0) continue; Require(packet.Length, offset, width); if (target > outer.Length || width > outer.Length - target) throw new InvalidOperationException("애니메이션 현재PID 선택필드 미연결"); packet.Slice(offset, width).CopyTo(outer[target..]); offset += width;
        }
        return parent;
    }
    /// <summary>보호 PE와 CD의 실제 가상 writer를 가진 VT만 두 헤더로 분배한다.</summary>
    public static bool UsesFirePacket(ReadOnlySpan<byte> outer) => Read(outer, 0) switch
    {
        0x50b0d0 or 0x50b120 or 0x50b170 or 0x50b1c0 or 0x50b318 or 0x50b370 or 0x50b3f0 or 0x505480 or 0x5054d0 => true,
        0x50b210 or 0x50b260 or 0x50b2c8 or 0x50b450 or 0x505520 or 0x505660 or 0x505570 => false,
        _ => throw new InvalidOperationException("애니메이션 현재outer 가상writer 미연결"),
    };
    /// <summary>공통 접두의 실제 dirty/비영 부모만 기록하고 선언 길이와 미작성 tail을 보존한다.</summary>
    public static ushort WriteProcess(Span<byte> outer, Span<byte> packet, bool includeParent, bool force, byte message, byte protocol = 1)
    {
        if (outer.Length < 56) throw new ArgumentException("애니메이션 writer 실제접두 부족"); uint parent = includeParent || force ? Read(outer, 8) : 0; byte dirty = force ? (byte)0xff : outer[40]; int length = 9 + (parent != 0 ? 4 : 0) + ((dirty & 2) != 0 ? 2 : 0) + ((dirty & 4) != 0 ? 2 : 0) + ((dirty & 8) != 0 ? 8 : 0);
        if (packet.Length < length) throw new ArgumentException("애니메이션 writer 버퍼 부족"); if (force) outer[40] = 0xff; packet[0] = protocol; packet[1] = message; Put(packet, 4, Read(outer, 4)); int offset = 9;
        if (parent != 0) { outer[40] |= 1; Put(packet, offset, parent); offset += 4; }
        // 실제 선택 필드의 원문 비트를 계산 없이 전송하며 끝에서 dirty를 소비한다.
        foreach ((int bit, int source, int width) in DataFields) { if ((outer[40] & bit) == 0) continue; outer.Slice(source, width).CopyTo(packet[offset..]); offset += width; }
        BinaryPrimitives.WriteUInt16LittleEndian(packet[2..], (ushort)offset); packet[8] = outer[40]; outer[40] = 0; return (ushort)offset;
    }
    /// <summary>두WORD와시각의 실제선택bit/offset/폭이다.</summary>
    private static readonly (int Bit, int Offset, int Width)[] DataFields = [(2, 42, 2), (4, 44, 2), (8, 48, 8)];
    /// <summary>선택한 packet 범위만 검사한다.</summary>
    private static void Require(int length, int offset, int width) { if (offset > length || width > length - offset) throw new ArgumentException("애니메이션 선택packet 부족"); }
    /// <summary>원문DWORD를 little endian으로 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    /// <summary>주변패딩을보존하며 원문DWORD를쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
}
