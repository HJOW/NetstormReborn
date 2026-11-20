using System.Buffers.Binary;

// 원문 메타구역 기록과 선택 마커의 DWORD 순서·부분 효과를 유지한다.
namespace Netstorm.Core.Original;

/// <summary>0042b1b0 부모와00479520 자식의 현재 물리와 선택 장치만 빌린다.</summary>
public static class RawMetaZoneCleanup
{
    /// <summary>원문 index0..50의 실제 기록 개수다.</summary>
    public const uint Count = 51;
    /// <summary>00546908의 기록 하나가 차지하는 실제 바이트 폭이다.</summary>
    public const int Stride = 144;
    /// <summary>원문0x72c DWORD 초기화가 쓰는 실제 표 전체 크기다.</summary>
    public const int Bytes = 7344;

    /// <summary>선택된 원문 위치의 한 DWORD만 검사하며 생략되는 뒤 필드를 미리 요구하지 않는다.</summary>
    private static uint Word(ReadOnlySpan<byte> records, int offset)
    {
        if (offset > records.Length || 4 > records.Length - offset)
            throw new ArgumentOutOfRangeException(nameof(records), "메타구역 마커의 현재 DWORD 물리 부족");
        return BinaryPrimitives.ReadUInt32LittleEndian(records.Slice(offset, 4));
    }

    /// <summary>rep stosd의 각 DWORD 전체 폭을 확인한 뒤 낮은 주소부터 쓰고 완료 prefix를 보존한다.</summary>
    private static void ZeroRecords(Span<byte> records)
    {
        // 원문0x72c회 쓰기이며 인접 canary와 짧은 마지막 DWORD는 변경하지 않는다.
        for (int offset = 0; offset < Bytes; offset += 4)
        {
            if (offset > records.Length || 4 > records.Length - offset)
                throw new ArgumentOutOfRangeException(nameof(records), "메타구역 정리의 실제 DWORD 물리 부족");
            BinaryPrimitives.WriteUInt32LittleEndian(records.Slice(offset, 4), 0);
        }
    }

    /// <summary>signed상태>0·SID비영일 때만 실제 가상 삭제를 호출하고 정상 복귀 뒤 같은 SID DWORD를 지운다.</summary>
    public static void ClearMarker(Memory<byte> records, uint index, Action<uint, uint>? destroy)
    {
        if (index >= Count) throw new ArgumentOutOfRangeException(nameof(index), "메타구역 마커의51기록 범위");
        int offset = (int)index * Stride;
        if (unchecked((int)Word(records.Span, offset)) <= 0) return;
        uint sid = Word(records.Span, offset + 76);
        if (sid == 0) return;
        (destroy ?? throw new InvalidOperationException("메타구역 마커의 실제 가상 삭제 미연결"))(sid, 0);
        BinaryPrimitives.WriteUInt32LittleEndian(records.Span.Slice(offset + 76, 4), 0);
    }

    /// <summary>진입flag비영은 표0→flag0, 진입0은 자식0..50→표0이며 callback이 바꾼 flag를 보존한다.</summary>
    public static void ClearRecords(UIntBox firstReset, Memory<byte> records, Action<uint>? clearMarker)
    {
        ArgumentNullException.ThrowIfNull(firstReset);
        if (firstReset.Value != 0)
        {
            ZeroRecords(records.Span);
            firstReset.Value = 0;
            return;
        }
        // 진입 때 선택한51회 분기는 callback의 현재 flag 변경으로 중간 전환하지 않는다.
        for (uint index = 0; index < Count; index++)
            (clearMarker ?? throw new InvalidOperationException("메타구역 정리의 실제 marker 자식 미연결"))(index);
        ZeroRecords(records.Span);
    }
}
