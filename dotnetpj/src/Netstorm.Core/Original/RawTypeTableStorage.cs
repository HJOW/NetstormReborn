using System.Collections;

// 장기 TYPE 표가 실제 배열과 겹치는 부분 뷰를 같은 물리로 소유한다.
namespace Netstorm.Core.Original;

/// <summary>Native vector의 초기 구성과 이후 고정 물리 대여를 구별하는 TYPE 저장소다.</summary>
public sealed class RawTypeTableStorage : IReadOnlyList<OriginalTypeRecord>
{
    /// <summary>현재 TYPE 레코드 슬롯을 직접 소유하는 실제 배열이다.</summary>
    private OriginalTypeRecord[] _records = [];
    /// <summary>현재 실제 레코드 개수다.</summary>
    public int Count => _records.Length;
    /// <summary>복사 없이 같은 현재 배열 전체를 빌리는 물리 뷰다.</summary>
    public ArraySegment<OriginalTypeRecord> PhysicalView => new(_records);
    /// <summary>옵션/전역 장치가 같은 현재 배열을 빌리는 읽기 전용 메모리다.</summary>
    public ReadOnlyMemory<OriginalTypeRecord> Memory => _records;
    /// <summary>현재 물리 슬롯의 레코드 참조를 직접 읽거나 교체한다.</summary>
    public OriginalTypeRecord this[int index] { get => _records[index]; set => _records[index] = value; }
    /// <summary>장치가 뷰를 빌리기 전 초기 구성에만 사용한다. 확장은 새 배열을 만들므로 이전 뷰를 새 주소로 가장하지 않는다.</summary>
    public void AddRange(IEnumerable<OriginalTypeRecord> records)
    {
        ArgumentNullException.ThrowIfNull(records); OriginalTypeRecord[] added = records.ToArray();
        if (added.Length == 0) return;
        var combined = new OriginalTypeRecord[checked(_records.Length + added.Length)];
        Array.Copy(_records, combined, _records.Length); Array.Copy(added, 0, combined, _records.Length, added.Length); _records = combined;
    }
    /// <summary>호출 당시의 같은 실제 배열에서 레코드를 순회한다.</summary>
    public IEnumerator<OriginalTypeRecord> GetEnumerator() => ((IEnumerable<OriginalTypeRecord>)_records).GetEnumerator();
    /// <summary>비제네릭 호출도 같은 배열 순회자를 사용한다.</summary>
    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
}
