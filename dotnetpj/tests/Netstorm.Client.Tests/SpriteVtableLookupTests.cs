using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 SID 슬롯의 객체 가상표를 읽는 표시 분배 경계를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 슬롯과 10.78 판본에서만 실제 가상 함수 주소를 선택한다.</summary>
public sealed class SpriteVtableLookupTests
{
    /// <summary>같은 슬롯의 가상표가 바뀌면 프레임과 추가 그림 함수도 다시 조회한다.</summary>
    [Fact]
    public void ReadsCurrentVtableAndRejectsUnknownAddress()
    {
        var pool = new RawSidPool(24000, server: false);
        int sid = pool.Allocate(SidAllocation.Client);
        BinaryPrimitives.WriteUInt32LittleEndian(pool.AllocatedBytes(sid), 0x005034c8);
        Assert.Equal(new SpriteVtableEntry(0x005034c8, 0x004222f0, 0x004223c0, 0x00498000), SpriteVtableLookup.Read(pool, sid));
        BinaryPrimitives.WriteUInt32LittleEndian(pool.AllocatedBytes(sid), 0x00502370);
        Assert.Equal(new SpriteVtableEntry(0x00502370, 0x004acfe0, 0x0041e3d0, 0x00498000), SpriteVtableLookup.Read(pool, sid));
        BinaryPrimitives.WriteUInt32LittleEndian(pool.AllocatedBytes(sid), 0x12345678);
        Assert.Contains("12345678", Assert.Throws<InvalidOperationException>(() => SpriteVtableLookup.Read(pool, sid)).Message);
    }

    /// <summary>10.72 풀을 10.78 주소 표로 조회하지 않는다.</summary>
    [Fact]
    public void RejectsDifferentEdition()
    {
        var pool = new RawSidPool(24000, server: false, edition: OriginalEdition.Cd1072);
        int sid = pool.Allocate(SidAllocation.Client);
        Assert.Throws<ArgumentException>(() => SpriteVtableLookup.Read(pool, sid));
    }
}
