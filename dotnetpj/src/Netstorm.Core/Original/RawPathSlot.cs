using System.Buffers.Binary;

// 경로 프레임들이 같은 raw 슬롯/물리 지도 좌표를 읽고 쓴다.
namespace Netstorm.Core.Original;

/// <summary>객체와 별도 보관해 가상 효과가 자신을 삭제한 뒤에도 읽는 수명 감시다.</summary>
public sealed class PathLifeWatch
{
    /// <summary>실제 커널 소멸 시 false로 바뀌는 같은 관찰값이다.</summary>
    public bool Alive { get; set; } = true;
}

/// <summary>RawPathProcess/파생 Frame의 비정렬 필드 접근을 같은 폭으로 공유한다.</summary>
internal static class RawPathSlot
{
    /// <summary>현재 선택한 DWORD만 읽는다.</summary>
    internal static uint Read(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4));
    /// <summary>현재 부모의 실제 두 float 좌표다.</summary>
    internal static RawPathPoint Position(RawSidPool pool, int parent)
    {
        ReadOnlySpan<byte> raw = pool.Slot(parent); return new(BitConverter.UInt32BitsToSingle(Read(raw, 14)), BitConverter.UInt32BitsToSingle(Read(raw, 18)));
    }
    /// <summary>PATCH DWORD/CD BYTE의 현재 프레임을 읽는다.</summary>
    internal static int Frame(RawSidPool pool, int parent) => pool.Edition == OriginalEdition.Patch1078 ? unchecked((int)Read(pool.Slot(parent), 36)) : pool.Slot(parent)[34];
    /// <summary>같은 실제 프레임 필드만 쓰며 공간 효과는 호출자 순서에 맡긴다.</summary>
    internal static void SetFrame(RawSidPool pool, int parent, int frame)
    {
        Span<byte> raw = pool.AllocatedBytes(parent);
        if (pool.Edition == OriginalEdition.Cd1072) raw[34] = unchecked((byte)frame); else BinaryPrimitives.WriteInt32LittleEndian(raw.Slice(36, 4), frame);
    }
    /// <summary>원문 double 덧셈과0쪽 절삭으로 물리 지도 칸을 선택한다.</summary>
    internal static int Cell(float value, bool biased)
    {
        double adjusted = (double)value + (biased ? 0.9999f : 0.0);
        if (!double.IsFinite(adjusted) || adjusted < 0 || adjusted >= 256) throw new ArgumentOutOfRangeException(nameof(value), "경로 지도 좌표");
        return (int)adjusted;
    }
}
