using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Netstorm.Client.Host;

// 실제 Windows 할당을 읽으며 기계마다 달라지는 인접값을 상수로 기대하지 않는다.
namespace Netstorm.Client.Tests;

/// <summary>요청색수·실제주소·인접DWORD·반납 경계를 창 없이 검증한다.</summary>
[SupportedOSPlatform("windows")]
public sealed class ParticlePaletteMemoryTests
{
    /// <summary>여러 요청 크기에서 마지막DWORD를 복제하지 않고 실제 인접 메모리를 읽는지 확인한다.</summary>
    [Fact]
    public void SnapshotReadsActualHeapNeighborAndKeepsLogicalCount()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var memory = new ParticlePaletteMemory();
        Assert.Equal(0, memory.Count); Assert.Equal(0, memory.Address); Assert.Throws<InvalidOperationException>(() => memory.Snapshot());
        // 재할당도 같은 소유heap에서 처리하고 snapshot의 추가칸을 색수로 세지 않는다.
        foreach (int count in new[] { 8, 11, 256, 1, 8 })
        {
            uint[] colors = Enumerable.Range(0, count).Select(i => 0xf0ee0100u + (uint)i).ToArray(); memory.Assign(colors);
            Assert.Equal(count, memory.Count); Assert.NotEqual(0, memory.Address);
            uint[] snapshot = memory.Snapshot().ToArray(); Assert.Equal(count + 1, snapshot.Length); Assert.Equal(colors, snapshot[..count]);
            // 실제 관찰주소를 독립적인 읽기로 확인한다. 소유범위 밖에 쓰지 않는다.
            Assert.Equal(unchecked((uint)Marshal.ReadInt32(memory.Address, count * 4)), snapshot[count]);
        }
    }
    /// <summary>빈 입력/과대 요청은 기존주소를 보존하며 반납 후 재생성·조회는 거부한다.</summary>
    [Fact]
    public void InvalidRequestsAndDisposedOwnerDoNotReplaceLiveAllocation()
    {
        if (!OperatingSystem.IsWindows()) return;
        var memory = new ParticlePaletteMemory();
        try
        {
            memory.Assign([1, 2, 3, 4, 5, 6, 7, 8]); nint address = memory.Address;
            Assert.Throws<ArgumentException>(() => memory.Assign([])); Assert.Throws<ArgumentException>(() => memory.Assign(new uint[257]));
            Assert.Equal(address, memory.Address); Assert.Equal(8, memory.Count);
            memory.Dispose(); memory.Dispose(); Assert.Equal(0, memory.Address); Assert.Equal(0, memory.Count);
            Assert.Throws<ObjectDisposedException>(() => memory.Snapshot()); Assert.Throws<ObjectDisposedException>(() => memory.Assign([1]));
        }
        finally { memory.Dispose(); }
    }
}
