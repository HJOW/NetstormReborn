using System.Collections;

// Native프레임벡터의초기구성과이후Session이빌리는동일고정배열을구분한다.
namespace Netstorm.Core.Original;

/// <summary>TYPE별Frame객체를같은물리배열로소유한다. AddRange는초기구성에서만호출하고대여중확장하지않는다.</summary>
public sealed class RawFrameTableStorage : IReadOnlyList<RiftTypeFrames>
{
    /// <summary>현재실제프레임객체배열이다.</summary>
    private RiftTypeFrames[] _records = [];
    /// <summary>현재실제슬롯수다.</summary>
    public int Count => _records.Length;
    /// <summary>사본없이같은현재객체슬롯을조회/교체한다.</summary>
    public RiftTypeFrames this[int index] { get => _records[index]; set => _records[index] = value; }
    /// <summary>Session의기존배열API가동일물리를빌린다. 초기화후확장은이뷰를무효화하므로금지한다.</summary>
    public RiftTypeFrames[] PhysicalArray => _records;
    /// <summary>원문vector의초기구성처럼새배열에기존참조와추가참조를순서대로넣는다.</summary>
    public void AddRange(IEnumerable<RiftTypeFrames> records)
    {
        ArgumentNullException.ThrowIfNull(records); RiftTypeFrames[] added = records.ToArray(); if (added.Length == 0) return;
        var combined = new RiftTypeFrames[checked(_records.Length + added.Length)]; Array.Copy(_records, combined, _records.Length); Array.Copy(added, 0, combined, _records.Length, added.Length); _records = combined;
    }
    /// <summary>한개추가도같은초기구성경계를사용한다.</summary>
    public void Add(RiftTypeFrames frame) => AddRange([frame]);
    /// <summary>호출시점의실제배열을순서대로순회한다.</summary>
    public IEnumerator<RiftTypeFrames> GetEnumerator() => ((IEnumerable<RiftTypeFrames>)_records).GetEnumerator();
    /// <summary>비제네릭소비자도같은배열을순회한다.</summary>
    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
}
