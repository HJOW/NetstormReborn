using System.Buffers.Binary;

// 원문 WORD 저장을 먼저 수행하고 현재 다리TYPE에만 실제 표면 통지를 전달한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawSquidWord.h의004ac220/CD004abb00 공통 setter다.</summary>
public static class RawSquidWord
{
    /// <summary>PATCH의 free/예약 슬롯도 물리 범위 안이면WORD를 먼저 쓴다. CD는 기존 할당 슬롯 계약을 유지한다.</summary>
    public static void Set(RawSidPool pool, int sid, ushort word, Func<uint> bridgeType, Action<int>? notify)
    {
        ReadOnlySpan<byte> physical = pool.Slot(sid);
        Span<byte> raw = pool.Edition == OriginalEdition.Patch1078 ? pool.RawSlot(sid) : pool.AllocatedBytes(sid);
        BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(12, 2), word);
        if (physical[10] == bridgeType()) (notify ?? throw new InvalidOperationException("공통 WORD setter의 선택된 표면 통지 미연결"))(sid);
    }
}
