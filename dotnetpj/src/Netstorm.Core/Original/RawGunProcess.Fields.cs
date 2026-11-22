using System.Buffers.Binary;

// 원문 new0x38의 padding과 outer/inner 필드를 같은56BYTE로 보존한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawGunProcess.h의 실제 이미지와 폭을 고정한 조회다.</summary>
public sealed class GunProcessFields
{
    /// <summary>가상 표/padding/dirty/시간/표적/단계/group/inner Form·Parent를 가진 실제56BYTE다.</summary>
    public byte[] Bytes { get; }
    /// <summary>입력 heap 패턴만 준비하며 실제 원문 초기화는 별도 Initialize가 수행한다.</summary>
    public GunProcessFields(byte fill = 0) { Bytes = new byte[56]; Bytes.AsSpan().Fill(fill); }
    /// <summary>원문 값 구조체처럼 같은56BYTE를 복사한다.</summary>
    public GunProcessFields(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length != 56) throw new ArgumentException("GunProcess의56BYTE 구조체 계약");
        Bytes = bytes.ToArray();
    }
    /// <summary>+8의 현재 변경 mask다.</summary>
    public byte Dirty() => Bytes[8];
    /// <summary>+10의 실제64비트 다음 시각이다.</summary>
    public double Time() => BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(Bytes.AsSpan(16)));
    /// <summary>+18의 현재 표적 WORD다.</summary>
    public ushort Target() => BinaryPrimitives.ReadUInt16LittleEndian(Bytes.AsSpan(24));
    /// <summary>+1c의 현재 상태 DWORD다.</summary>
    public uint Stage() => BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(28));
    /// <summary>+20의 signed group이다.</summary>
    public int Group() => BinaryPrimitives.ReadInt32LittleEndian(Bytes.AsSpan(32));
    /// <summary>+2c의 Form DWORD에서 실제 SID WORD를 읽는다.</summary>
    public ushort Form() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(44)));
    /// <summary>+30의 부모 DWORD에서 실제 SID WORD를 읽는다.</summary>
    public ushort Parent() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(48)));
}
