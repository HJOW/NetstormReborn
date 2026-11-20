using System.Buffers.Binary;

// 객체 선택 비트의 전/후 표시 통지와 내용물 표시 입구를 같은 실제 물리에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>CPP의83VT 기록과 같은 선택 통지 식별자를 조회한다.</summary>
public static partial class OriginalSelectionVtables
{
    /// <summary>현재 기록과 일치하는 원문+7c만 반환하며 미지원 VT는0이다.</summary>
    public static uint Notice(uint table)
    {
        // 원문 선언 순서로 같은 전체 DWORD를 대조한다.
        foreach (Entry entry in Entries) if (entry.Table == table) return entry.Notice;
        return 0;
    }
}

/// <summary>41c0b0/44b2b0의 현재 선택 bit2와 표시 순서를 처리한다.</summary>
public sealed class RawSquidSelection
{
    /// <summary>반납/죽음 필터를 추가하지 않는 같은 실제 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>파생+7c가 bit쓰기 전에 요구하는 현재 실제+88이다.</summary>
    private readonly Action<int>? _refresh;
    /// <summary>판본만 검증하며 실제 효과는 선택된 호출에서 요구한다.</summary>
    public RawSquidSelection(RawSidPool pool, Action<int>? refresh)
    {
        ArgumentNullException.ThrowIfNull(pool); if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("객체선택10.78풀필요", nameof(pool));
        _pool = pool; _refresh = refresh;
    }
    /// <summary>실제83VT의+7c 식별자를 반환한다.</summary>
    public static uint SelectionEntry(uint table) => OriginalSelectionVtables.Notice(table);
    /// <summary>사정거리 종류이며 이동 가능 여부와 구분한다.</summary>
    public static bool UsesRange(OriginalTypeRecord type)
    {
        ArgumentNullException.ThrowIfNull(type); return type.Group is 0 or 1 or 6 or 3 || (type.Flags2 & 0x100) != 0;
    }
    /// <summary>현재 파생 표시 뒤 extra를 다시 읽어 value bit0만 선택 bit2로 옮긴다.</summary>
    public void Mark(int sid, uint value)
    {
        if (sid <= 0 || sid >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(sid));
        uint entry = SelectionEntry(BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(sid)));
        if (entry == 0) throw new InvalidOperationException("객체선택가상7c VT미연결");
        if (entry == 0x44b2b0) (_refresh ?? throw new InvalidOperationException("객체선택실제표시미연결"))(sid);
        else if (entry != 0x41c0b0) throw new InvalidOperationException("객체선택파생몸체미연결");
        ref byte extra = ref _pool.RawSlot(sid)[40]; extra = (byte)((extra & ~4u) | ((value & 1u) << 2));
    }
    /// <summary>로딩/extra 관문과 Form 진단 뒤 현재 TYPE를 재조회해491의 정확한0.2f를 전달한다.</summary>
    public static void StartContentPreview(RawSidPool pool, int parent, uint loadingDepth, Action<int, uint, float>? regular, Action? assertion)
    {
        ArgumentNullException.ThrowIfNull(pool); if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("선택내용물표시10.78풀필요", nameof(pool));
        if (loadingDepth != 0) return; if ((pool.Slot(parent)[40] & 9) != 0) return;
        if (pool.Slot(parent)[10] < 70) { (assertion ?? throw new InvalidOperationException("내용물표시Form진단미연결"))(); if (pool.Slot(parent)[10] < 70) return; }
        (regular ?? throw new InvalidOperationException("내용물표시실제Regular미연결"))(parent, 0x491, BitConverter.UInt32BitsToSingle(0x3e4ccccd));
    }
}
