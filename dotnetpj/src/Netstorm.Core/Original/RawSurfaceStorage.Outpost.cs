// 실제 Outpost의 두 목록 제거와 현재 영역 갱신 뒤 실제 Damageable 부모를 재사용한다.
// cpppj/src/o/RawOutpostLifecycle.cpp 의 PreDestroyNative 장기 본문을 이식한다.
// 0.582에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Outpost 우선)
namespace Netstorm.Core.Original;

/// <summary>World 없이 같은 장기 Pool/목록으로 Outpost 삭제를 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>실제 0x00455840은 raw 자산·두 번째 목록·부모를 앞에서 검사하지 않고 선택된 현재 물리에 순서대로 도달한다. 0.582에서 추가됐다.</summary>
    /// <param name="sid">삭제할 Outpost 번호다.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트다.</param>
    public void OutpostPreDestroy(int sid, uint flags)
    {
        if ((Pool.Slot(sid)[40] & 9u) == 0)
        {
            RemoveOutpostNative(SidLists.Outposts, sid);
            RemoveOutpostNative(SidLists.Residences, sid);
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            float y = ReadOutpostCoordinate(raw, 18);
            float x = ReadOutpostCoordinate(raw, 14);
            Action<float, float>? regionChanged = _regionChanged;
            if (regionChanged is null)
            {
                throw new InvalidOperationException("장기 Outpost의 실제 영역 갱신 미결합");
            }

            regionChanged(x, y);
        }

        DamageablePre().PreDestroy(sid, flags);
    }

    /// <summary>비정렬 raw 좌표를 읽어 NaN/부호 있는 0의 비트도 그대로 효과에 넘긴다. 0.582에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">좌표 바이트 위치다.</param>
    private static float ReadOutpostCoordinate(ReadOnlySpan<byte> raw, int offset)
    {
        uint bits = 0;
        // 원문 little endian DWORD의 낮은 바이트부터 조립한다.
        for (int index = 0; index < 4; index++)
        {
            bits |= (uint)raw[offset + index] << (8 * index);
        }

        return BitConverter.UInt32BitsToSingle(bits);
    }

    /// <summary>활성 중복 전부를 제거하되 남은 항목의 순서와 비활성 꼬리를 보존한다. 0.582에서 추가됐다.</summary>
    /// <param name="list">압축할 목록이다.</param>
    /// <param name="sid">뺄 번호다.</param>
    private static void RemoveOutpostNative(RawPostPopList list, int sid)
    {
        uint removed = 0;
        uint index = 0;
        // count<=0이면 배열 포인터도 읽지 않는다.
        while (unchecked((int)index) < unchecked((int)list.Count))
        {
            uint[] storage = list.Entries;
            if (index >= (uint)storage.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), "Outpost 삭제 목록의 현재 DWORD 물리 부족");
            }

            uint value = storage[index];
            if (value == unchecked((uint)sid))
            {
                removed++;
            }
            else
            {
                uint destination = index - removed;
                if (destination >= (uint)storage.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(sid), "Outpost 삭제 압축의 현재 DWORD 물리 부족");
                }

                storage[destination] = value;
            }

            index++;
        }

        list.Count = unchecked(list.Count - removed);
    }
}
