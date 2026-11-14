using System.Buffers.Binary;
using Netstorm.Core.Original;

// 일반 현재 Pop 부모가 보호막 접두 뒤 공통 깊이를 정확히 한 번 끝내는지 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>0.513 실제 분배를 적용하기 전 같은 원문 부모 사본으로 연결 순서를 대조한다.</summary>
public sealed class RawForcefieldIntegrationTests
{
    /// <summary>PATCH 가상 진입은 실제VT를 선택하며 억제된 공통 부모는 잘못된TYPE/free를 선행 거부하지 않는다.</summary>
    [Fact]
    public void VirtualUsesCurrentVtableThenExactlyOneCommonDepth()
    {
        var pool = new RawSidPool(24000, false); OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        var book = new RawPostPopState { Suppressed = true, Depth = 1 }; var body = new RawPostPop(pool, types, book);
        Span<byte> raw = pool.RawSlot(50); BinaryPrimitives.WriteUInt32LittleEndian(raw, ForcefieldLifecycle.PatchVtable); raw[10] = 255; raw[11] = 9;
        var prefix = new RawForcefieldPostPop(pool, new ForcefieldPostPopHooks()); body.SetForcefieldPostPop(prefix);
        Assert.True(body.HandlesForcefield(50)); body.PostPopVirtual(50, 0); Assert.Equal(0u, book.Depth);
        Assert.Throws<ArgumentException>(() => prefix.Validate(50)); int calls = 0;
        body.SetForcefieldPostPop(new RawForcefieldPostPop(pool, new ForcefieldPostPopHooks { ReserveRegular = () => { calls++; return false; } }));
        book.Depth++; body.PostPopVirtual(50, 1); Assert.Equal(2, calls); Assert.Equal(0u, book.Depth); Assert.Equal(9, raw[11]);
        Assert.Throws<ArgumentException>(() => body.SetForcefieldPostPop(new RawForcefieldPostPop(new RawSidPool(24000, false), new ForcefieldPostPopHooks())));
    }

    /// <summary>선택한 확보 장치 실패에서는 공통 깊이를 줄이지 않고 현재 raw를 유지한다.</summary>
    [Fact]
    public void MissingSelectedReserveStopsBeforeCommon()
    {
        var pool = new RawSidPool(24000, false); var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        Span<byte> raw = pool.RawSlot(50); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, ForcefieldLifecycle.PatchVtable); raw[10] = 74;
        byte[] before = raw.ToArray(); var book = new RawPostPopState { Suppressed = true, Depth = 1 }; var body = new RawPostPop(pool, types, book);
        body.SetForcefieldPostPop(new RawForcefieldPostPop(pool, new ForcefieldPostPopHooks()));
        Assert.Throws<InvalidOperationException>(() => body.PostPopVirtual(50, 1)); Assert.Equal(1u, book.Depth); Assert.Equal(before, raw.ToArray());
    }
}
