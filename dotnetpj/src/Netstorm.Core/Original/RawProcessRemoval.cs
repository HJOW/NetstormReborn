using System.Buffers.Binary;

// 원문 ProcessFinder가 저장한 next를 따라 같은 Host에서 선택한 프로세스만 제거한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawProcessRemoval.cpp의 PATCH4b27a0이다.</summary>
public static class RawProcessRemoval
{
    /// <summary>삭제 전 next를 보존하고 정확한 타입 또는 전체를 flags40으로 제거한다.</summary>
    public static void RemoveAttached(SquidProcessHost host, int parent, uint type)
    {
        RawSidPool pool = host.Pool(); if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("CD 프로세스 일괄 제거 미복원");
        int current = BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(parent).Slice(6, 2)); var visited = new HashSet<int>();
        // 새 머리를 반복 검색하지 않고 효과 뒤 저장한 다음 번호의 현재 raw를 읽는다.
        while (current != 0)
        {
            if (current >= pool.Capacity || !visited.Add(current)) throw new InvalidOperationException("프로세스 제거 종속 체인 손상");
            ReadOnlySpan<byte> raw = pool.Slot(current); int next = BinaryPrimitives.ReadUInt16LittleEndian(raw.Slice(4, 2)); byte candidate = raw[10];
            if (candidate < 70 && candidate >= 10 && candidate < host.ProcessTypeEnd())
            {
                SquidProcess process = host.FromForm(current) ?? throw new InvalidOperationException("실제 제거할 프로세스 부재");
                if (type == 0 || pool.Slot(process.Form())[10] == type) host.Kill(process, 0x40);
            }
            current = next;
        }
    }
}
