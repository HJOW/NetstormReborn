using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Tests;

// 연소/Renderer가 공유하는 원문변경표 전체와 현재Count0/플래그 수명을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>원본133변경표시퀀스와 원문100칸/첫flags 잔여의 현재계약이다.</summary>
public sealed class DirtyRegionsOracleTests
{
    /// <summary>원문 입력의 signed좌표/flags비트만 같은5필드로 읽는다.</summary>
    private static DirtyRegion[] Entries(string text) => text == "-" ? [] : text.Split(';').Select(record =>
    {
        long[] v = record.Split(',').Select(value => long.Parse(value, CultureInfo.InvariantCulture)).ToArray();
        return new DirtyRegion(new((int)v[0], (int)v[1], (int)v[2], (int)v[3]), unchecked((uint)v[4]));
    }).ToArray();
    /// <summary>기존 두판본 원본의 등록/병합/무시/넘침 전체표를133입력에 대조한다.</summary>
    [Fact]
    public void DirtySequencesMatchAllOriginalTablesAndFullRedrawFlags()
    {
        string[][] rows = X86Fixture.Read("renderer", "Dirty"); Assert.Equal(133, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var regions = new DirtyRegions(X86Fixture.Int(row[1]), X86Fixture.Int(row[2]));
            // 기대출력과 구별한 원문등록 시퀀스만 실제Add에 전달한다.
            foreach (DirtyRegion input in Entries(row[3])) regions.Add(input.Rect, input.Flags);
            Assert.Equal(row[4] != "0", regions.FullRedraw); Assert.Equal(Entries(row[5]), regions.Entries); return true;
        });
    }
    /// <summary>불투명 덮임은원래항목을무시표식으로남기고 Clear뒤 첫물리flags/카메라목록owner를 유지한다.</summary>
    [Fact]
    public void OpaqueDiscardAndClearKeepPhysicalFirstFlagsAndCurrentListOwner()
    {
        var regions = new DirtyRegions(100, 100); List<DirtyRegion> list = regions.CameraEntries;
        regions.Add(new(0, 0, 10, 10)); regions.Add(new(50, 50, 60, 60), 4); regions.Add(new(-5, -5, 65, 65), 4);
        Assert.Equal(new uint[] { 1, 5, 4 }, regions.Entries.Select(e => e.Flags)); regions.Add(new(1, 1, 2, 2)); Assert.Equal(3, regions.Entries.Count);
        regions.InvalidateAll(); Assert.True(regions.FullRedraw); regions.Clear(); Assert.False(regions.FullRedraw); Assert.Empty(list); Assert.Same(list, regions.CameraEntries); Assert.Equal(1u, regions.FirstFlags);
        regions.Add(new(-5, -5, 10, 10), 5); Assert.Equal(new ScreenRect(-5, -5, 10, 10), regions.Entries[0].Rect); Assert.Equal(4u, regions.FirstFlags);
    }
    /// <summary>100칸한계는병합보다먼저 선택하고 화면밖요청은넘침도발생시키지않는다.</summary>
    [Fact]
    public void CapacityIsCheckedBeforeMergingAndOutsideRequestsDoNotOverflow()
    {
        var regions = new DirtyRegions(10000, 100);
        // 병합하지 않는100개영역으로 원문 표를 채운다.
        for (int i = 0; i < 100; i++) regions.Add(new(i * 20, 0, i * 20 + 2, 2));
        Assert.Equal(100, regions.Entries.Count); Assert.False(regions.FullRedraw);
        regions.Add(new(10001, 0, 10002, 1)); Assert.False(regions.FullRedraw); regions.Add(new(0, 0, 2, 2)); Assert.True(regions.FullRedraw); Assert.Equal(100, regions.Entries.Count);
    }
}
