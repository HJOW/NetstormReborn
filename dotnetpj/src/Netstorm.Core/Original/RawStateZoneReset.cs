using System.Buffers.Binary;

// State Zone의 두 직접 쓰기 폭과 부분 효과를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>004bfd60의 DWORD0 다음 WORD0만 같은 물리에 쓴다.</summary>
public static class RawStateZoneReset
{
    /// <summary>각 쓰기 위치에서 범위를 확인하며 뒤 범위가 짧아도 완료한 첫DWORD는 유지한다.</summary>
    public static void Reset(Span<byte> zone)
    {
        if (zone.Length < 4) throw new ArgumentOutOfRangeException(nameof(zone), "State Zone의 첫 DWORD 물리 부족");
        BinaryPrimitives.WriteUInt32LittleEndian(zone, 0);
        if (zone.Length < 6) throw new ArgumentOutOfRangeException(nameof(zone), "State Zone의 마지막 WORD 물리 부족");
        BinaryPrimitives.WriteUInt16LittleEndian(zone.Slice(4, 2), 0);
    }
}
