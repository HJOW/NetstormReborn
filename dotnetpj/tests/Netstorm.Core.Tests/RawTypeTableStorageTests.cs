using Netstorm.Core.Original;

// TYPE 저장소와 물리 배열/부분 뷰가 같은 슬롯을 가리키는지 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>레코드 값이나 참조 중복으로 물리 주소를 추정하지 않는 계약을 검사한다.</summary>
public sealed class RawTypeTableStorageTests
{
    /// <summary>저장소/배열 뷰의 슬롯 교체가 양방향으로 같은 실제 메모리에 반영된다.</summary>
    [Fact]
    public void PhysicalViewAliasesRecordSlotsInBothDirections()
    {
        var first = new OriginalTypeRecord(); var second = new OriginalTypeRecord(); var storage = new RawTypeTableStorage(); storage.AddRange([first, second]);
        ArraySegment<OriginalTypeRecord> view = storage.PhysicalView; Assert.Equal(2, storage.Count); Assert.Same(first, view[0]);
        var replacement = new OriginalTypeRecord(); storage[1] = replacement; Assert.Same(replacement, view[1]); view[0] = second; Assert.Same(second, storage[0]);
        Assert.Same(view.Array, storage.PhysicalView.Array); Assert.Same(replacement, storage.Memory.Span[1]);
    }
    /// <summary>두 슬롯이 같은 객체 참조를 담아도 HP의 this 번호는 실제 배열 시작 위치로 계산한다.</summary>
    [Fact]
    public void OverlappingViewUsesPhysicalIndexDespiteDuplicateRecordReferences()
    {
        var record = new OriginalTypeRecord(); var storage = new RawTypeTableStorage(); storage.AddRange([record, record]);
        var second = new ArraySegment<OriginalTypeRecord>(storage.PhysicalView.Array!, 1, 1);
        Assert.Equal(1u, RawRiftHitPoints.CurrentRiftTypeNumber(0, second, storage));
        Assert.Equal(1u, RawRiftHitPoints.CurrentRiftTypeNumber(1, storage, storage.PhysicalView));
        var different = new RawTypeTableStorage(); different.AddRange([record, record]);
        Assert.Throws<InvalidOperationException>(() => RawRiftHitPoints.CurrentRiftTypeNumber(0, second, different));
    }
}
