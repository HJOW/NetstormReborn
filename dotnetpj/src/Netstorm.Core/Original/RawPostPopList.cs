// 현재 Player 목록과 독립 입력의 저장소/용량/개수를 같은 소비 위치에서 선택한다.
namespace Netstorm.Core.Original;

/// <summary>같은 실제 현재 목록의 getter/setter와 소유자 정체성이다.</summary>
public sealed class PostPopListCurrentBinding
{
    /// <summary>현재 실제 배열을 복사 없이 빌린다.</summary>
    public Func<uint[]>? Storage { get; set; }
    /// <summary>현재 Player의 용량DWORD를 읽는다.</summary>
    public Func<uint>? Capacity { get; set; }
    /// <summary>현재 Player의 사용 개수DWORD를 읽는다.</summary>
    public Func<uint>? ReadCount { get; set; }
    /// <summary>같은 현재 사용 개수DWORD만 쓴다.</summary>
    public Action<uint>? WriteCount { get; set; }
    /// <summary>원본 주소값이 아닌 실제 관리 저장소의 정체성이다.</summary>
    public object? SourceOwner { get; set; }
    /// <summary>현재 저장소의 Player 행이다.</summary>
    public uint SourceSlot { get; set; }
    /// <summary>함수 객체/정체성을 값 복사하고 실제 저장소 수명은 빌린다.</summary>
    internal PostPopListCurrentBinding Copy() => new() { Storage = Storage, Capacity = Capacity, ReadCount = ReadCount, WriteCount = WriteCount, SourceOwner = SourceOwner, SourceSlot = SourceSlot };
}

/// <summary>원본 SquidPostPopList의 현재/독립 목록과 stale 꼬리 보존 계약이다.</summary>
public sealed class RawPostPopList
{
    /// <summary>독립 입력만 소유하는 기존 배열이다.</summary>
    private uint[] _entries = [];
    /// <summary>독립 입력만 소유하는 기존 개수다.</summary>
    private uint _count;
    /// <summary>명시 현재 getter이며 독립 입력은 빈 장치를 유지한다.</summary>
    public PostPopListCurrentBinding Current { get; set; } = new();
    /// <summary>현재 실제 배열 또는 독립 배열을 빌린다. 대입은 독립 저장소만 바꾼다.</summary>
    public uint[] Entries { get => Current.Storage?.Invoke() ?? _entries; set => _entries = value; }
    /// <summary>현재 실제 개수 또는 독립 개수를 각 소비 위치에서 읽고 쓴다.</summary>
    public uint Count { get => Current.ReadCount?.Invoke() ?? _count; set { if (Current.WriteCount is { } write) write(value); else _count = value; } }
    /// <summary>배열 포인터를 읽지 않고 현재 용량DWORD 또는 독립 배열 길이를 읽는다.</summary>
    public uint Capacity => Current.Capacity?.Invoke() ?? (uint)_entries.Length;
    /// <summary>같은 실제 owner/행의 현재 연결인지 물리 조회 없이 비교하며 null은 허용하지 않는다.</summary>
    public bool MatchesCurrentSource(object? owner, uint slot) => owner is not null && Current.Storage is not null && ReferenceEquals(Current.SourceOwner, owner) && Current.SourceSlot == slot;
    /// <summary>독립 값이 비어 있을 때만 완전한 현재 연결을 값 복사한다.</summary>
    public void BindCurrent(PostPopListCurrentBinding binding)
    {
        ArgumentNullException.ThrowIfNull(binding);
        if (Current.Storage is not null || _entries.Length != 0 || _count != 0 || binding.Storage is null || binding.Capacity is null || binding.ReadCount is null || binding.WriteCount is null)
            throw new InvalidOperationException("postPop 현재 목록 연결의 소유/장치 오류");
        Current = binding.Copy();
    }
    /// <summary>현재 개수→용량→배열 순서로 활성 항목을 빌리고 꼬리는 유지한다.</summary>
    public ReadOnlySpan<uint> Items()
    {
        uint used = Count; if (used > Capacity) throw new InvalidOperationException("postPop 목록 count 범위 오류");
        return Entries.AsSpan(0, checked((int)used));
    }
    /// <summary>활성 항목에서 현재 번호를 조회한다.</summary>
    public bool Contains(int sid)
    {
        // 원문 활성 보기만 순회한다.
        foreach (uint entry in Items()) if (entry == (uint)sid) return true;
        return false;
    }
    /// <summary>용량이 없으면 배열을 읽지 않고, 저장 뒤 최초 개수+1을 쓴다.</summary>
    public void Append(int sid)
    {
        uint used = Count; if (used >= Capacity) return;
        Entries[used] = unchecked((uint)sid); Count = unchecked(used + 1);
    }
    /// <summary>현재 사용 범위의 중복을 안정 압축하고 비활성 꼬리를 보존한다.</summary>
    public void RemoveAll(uint value)
    {
        _ = Items(); uint removed = 0;
        // 각 조건/저장에서 현재 개수와 배열을 다시 읽는다.
        for (uint index = 0; index < Count; index++)
        {
            uint entry = Entries[index]; if (entry == value) removed++; else Entries[index - removed] = entry;
        }
        Count = unchecked(Count - removed);
    }
}
