// 포함 링크의 표 선택과 WORD 조회를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 포함 링크를 검증하는 테스트 클래스다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.
public sealed class RawContainedLinkTests
{
    /// <summary>생성 자료 151개가 원문 개수·앞 항목과 같다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    [Fact]
    public void TablesHaveNativeCountsAndHeads()
    {
        Assert.Equal(4, ContainedLinkTables.PatchForms.Length);
        Assert.Equal(4, ContainedLinkTables.CdForms.Length);
        Assert.Equal([0x512110u, 0x512098u, 0x512188u, 0x512208u], ContainedLinkTables.PatchForms);
        Assert.Equal([0x505a08u, 0x505990u, 0x505a80u, 0x505b00u], ContainedLinkTables.CdForms);
        Assert.Equal(83, ContainedLinkTables.PatchNotices.Length);
        Assert.Equal(68, ContainedLinkTables.CdNotices.Length);
        Assert.Equal(new ContentNoticeEntry(0x00501dd0u, 0x0041c100u), ContainedLinkTables.PatchNotices[0]);
        Assert.Equal(new ContentNoticeEntry(0x00500030u, 0x00401c20u), ContainedLinkTables.CdNotices[0]);
    }

    /// <summary>네 Form 주소는 배치 참, 허용 목록은 거짓, 모르는 표는 거부한다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    [Fact]
    public void LayoutSelectsFormNoticeOrRejects()
    {
        Assert.True(RawContainedLink.FormLinkLayoutAtTable(OriginalEdition.Patch1078, 0x512098u));
        Assert.False(RawContainedLink.FormLinkLayoutAtTable(OriginalEdition.Patch1078, 0x00501dd0u));
        Assert.True(RawContainedLink.FormLinkLayoutAtTable(OriginalEdition.Cd1072, 0x505990u));
        Assert.False(RawContainedLink.FormLinkLayoutAtTable(OriginalEdition.Cd1072, ContainedLinkTables.CdNotices[0].Table));
        Assert.Throws<InvalidOperationException>(() => RawContainedLink.FormLinkLayoutAtTable(OriginalEdition.Patch1078, 0x12345678u));
        Assert.Throws<InvalidOperationException>(() => RawContainedLink.FormLinkLayoutAtTable(OriginalEdition.Cd1072, 0x12345678u));
    }

    /// <summary>현재 가상 표에 따라 부모·이전 WORD 위치가 갈린다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    [Fact]
    public void ParentPreviousOffsetsFollowLayout()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        PutTable(pool, 5, 0x512098u);
        PutWord(pool, 5, 16, 70);
        PutWord(pool, 5, 14, 71);
        Assert.True(RawContainedLink.FormLinkLayout(pool, 5));
        Assert.Equal(70, RawContainedLink.CommonContainedParent(pool, 5));
        Assert.Equal(71, RawContainedLink.CommonContainedPrevious(pool, 5));
        PutTable(pool, 6, 0x00501dd0u);
        PutWord(pool, 6, 14, 72);
        PutWord(pool, 6, 16, 73);
        Assert.False(RawContainedLink.FormLinkLayout(pool, 6));
        Assert.Equal(72, RawContainedLink.CommonContainedParent(pool, 6));
        Assert.Equal(73, RawContainedLink.CommonContainedPrevious(pool, 6));
        Assert.Equal(73, RawContainedLink.CommonContainedPreviousAtTable(pool, 6, 0x00501dd0u));
        Assert.Throws<ArgumentNullException>(() => RawContainedLink.CommonContainedParent(null!, 5));
    }

    /// <summary>쓰기는 같은 WORD만 바꾸고 이웃 payload를 보존한다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    [Fact]
    public void SettersPreserveNeighborBytes()
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        PutTable(pool, 5, 0x512098u);
        PutWord(pool, 5, 14, 0x1234);
        RawContainedLink.CommonContainedSetParent(pool, 5, 80);
        Assert.Equal(80, RawContainedLink.CommonContainedParent(pool, 5));
        Assert.Equal(0x1234, GetWord(pool, 5, 14));
        RawContainedLink.CommonContainedSetPrevious(pool, 5, 81);
        Assert.Equal(81, RawContainedLink.CommonContainedPrevious(pool, 5));
        Assert.Equal(80, RawContainedLink.CommonContainedParent(pool, 5));
        RawContainedLink.CommonContainedSetPreviousAtTable(pool, 5, 0x512098u, 82);
        Assert.Equal(82, RawContainedLink.CommonContainedPrevious(pool, 5));
        Assert.Throws<ArgumentNullException>(() => RawContainedLink.CommonContainedSetParent(null!, 5, 1));
    }

    /// <summary>가상 표 DWORD를 쓰는 정적 보조 함수다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="table">쓸 표 주소다.</param>
    private static void PutTable(RawSidPool pool, int sid, uint table)
    {
        Span<byte> raw = pool.RawSlot(sid);
        raw[0] = unchecked((byte)table);
        raw[1] = unchecked((byte)(table >> 8));
        raw[2] = unchecked((byte)(table >> 16));
        raw[3] = unchecked((byte)(table >> 24));
    }

    /// <summary>WORD를 쓰는 정적 보조 함수다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutWord(RawSidPool pool, int sid, int offset, ushort value)
    {
        Span<byte> raw = pool.RawSlot(sid);
        raw[offset] = unchecked((byte)value);
        raw[offset + 1] = unchecked((byte)(value >> 8));
    }

    /// <summary>WORD를 읽는 정적 보조 함수다. 2026-11-07 F2d 포함 링크 이식에서 추가됐다.</summary>
    /// <param name="pool">같은 풀이다.</param>
    /// <param name="sid">읽을 번호다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static ushort GetWord(RawSidPool pool, int sid, int offset)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }
}
