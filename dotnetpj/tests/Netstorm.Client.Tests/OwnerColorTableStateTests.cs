using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.InventoryRawSupport;

namespace Netstorm.Client.Tests; // 실제 현재 색표의 전체 원문 효과와 장기 참조를 검사한다.

/// <summary>원본2066입력과 실제 primary/토큰/색표 수명 경계를 확인한다.</summary>
public sealed class OwnerColorTableStateTests
{
    /// <summary>독립 관측이 제공한 할당 장치의 opaque 주소다.</summary>
    private static uint Token(uint type) => 0x100000 + type * 2304;
    /// <summary>원시 DWORD 배열을 원문 순서로 기록한다.</summary>
    private static string Numbers(IEnumerable<uint> values) => string.Join(',', values.Select(value => value.ToString(CultureInfo.InvariantCulture)));
    /// <summary>원문2066행의 모든 현재 포인터와 순서 있는 생성/반납 효과를 비교한다.</summary>
    [Fact]
    public void Native2066CurrentPointersAndInitialRegistration()
    {
        int count = 0;
        // 원문9입력 열만 공급하고 뒤6관측 열은 실행 뒤에만 비교한다.
        foreach (var row in Rows("banner-option").Where(row => row[0] != "name"))
        {
            Assert.Equal(15, row.Length); var ready = new UIntBox { Value = U(row[2]) }; var enabled = new UIntBox { Value = U(row[3]) }; var dirty = new UIntBox { Value = U(row[4]) }; uint[] pointers = new uint[188]; pointers[111] = Token(111); var types = new OwnerColorTableTypes(); uint[] inputTypes = Inputs(row[6]); Assert.Equal(5, inputTypes.Length); List<string> trace = [];
            // 원문 현재 TYPE 전역과 존재 비트만 복원한다.
            for (int index = 0; index < 5; index++) { types.Banner[index] = inputTypes[index]; if ((U(row[5]) & (1u << index)) != 0) pointers[types.Banner[index]] = Token(types.Banner[index]); }
            var globals = new OwnerColorTableGlobals { Ready = ready, Enabled = enabled, Redraw = dirty, Tables = pointers }; var hooks = new OwnerColorTableHooks { Standard = type => { trace.Add("standard:" + type); pointers[type] = Token(type); }, Special = (type, offset, sources) => { trace.Add("special:" + type + ':' + BitConverter.DoubleToUInt64Bits(offset) + ':' + Numbers(sources)); pointers[type] = Token(type); }, Release = pointer => trace.Add("free:" + pointer) };
            // 연속 요청 사이 현재 표/설정을 재생성하지 않는다.
            foreach (uint request in Inputs(row[7])) { if (row[1] == "init") OwnerColorTableOperations.InitializeOwnerColorTables(globals, U(row[8]), types, hooks); else OwnerColorTableOperations.SetBannerOption(globals, request, types, hooks); }
            Assert.Equal(U(row[9]), ready.Value); Assert.Equal(U(row[10]), enabled.Value); Assert.Equal(U(row[11]), dirty.Value); Assert.Equal(row[12], Numbers(pointers)); Assert.Equal(row[13], trace.Count == 0 ? "-" : string.Join(';', trace)); Assert.True(U(row[14]) > 0); count++;
        }
        Assert.Equal(2066, count);
    }
    /// <summary>실제 ColorSort 저장 명도를 준비하는 명시 회색 입력이다.</summary>
    private static RawColorSortPrimary Primary()
    {
        var primary = new RawColorSortPrimary(new byte[RawColorSortPrimary.TotalBytes]); byte[] rgb = new byte[1024];
        // 검사 명시 RGB 장치만 준비하며 기대 색표를 작성하지 않는다.
        for (int index = 0; index < 256; index++) rgb[index * 4] = rgb[index * 4 + 1] = rgb[index * 4 + 2] = (byte)index;
        primary.Construct(rgb); return primary;
    }
    /// <summary>배너 전환 뒤 이전 토큰 뷰/설정 주소가 현재 표를 계속 읽는다.</summary>
    [Fact]
    public void CurrentTokenViewAndColorsSurviveToggleAndRemainShared()
    {
        var owner = new OwnerColorTableState(); var dirty = new UIntBox(); var enabled = owner.EnabledBox; owner.SetEnabled(0, dirty); Assert.Equal(1u, owner.Enabled); Assert.False(owner.Ready); Assert.Equal(0u, dirty.Value); Assert.Throws<InvalidOperationException>(() => owner.Tokens);
        var unready = new RawColorSortPrimary(new byte[RawColorSortPrimary.TotalBytes]); Assert.Throws<ArgumentException>(() => owner.Initialize(unready, 188, 82, dirty)); Assert.False(owner.Ready);
        owner.Initialize(Primary(), 188, 82, dirty); var memory = owner.Tokens; Assert.True(owner.Ready); Assert.Equal(1u, dirty.Value); Assert.Equal(1u, enabled.Value); Assert.Same(enabled, owner.EnabledBox);
        Assert.NotEqual(0u, memory.Span[82]); Assert.NotEqual(0u, memory.Span[111]); Assert.NotEqual(0u, memory.Span[141]); Assert.NotEqual(0u, memory.Span[137]); Assert.Equal(0u, memory.Span[94]); Assert.Equal(0u, memory.Span[97]); Assert.Equal(0u, memory.Span[85]); Assert.Equal(0u, memory.Span[71]); Assert.Equal(0u, memory.Span[128]);
        owner.SetEnabled(0, dirty); Assert.Equal(2u, dirty.Value); Assert.Equal(0u, enabled.Value); Assert.NotEqual(0u, memory.Span[94]); Assert.NotEqual(0u, memory.Span[97]); byte[] color = owner.Color(97, 2); uint oldToken = memory.Span[97] + 512; Assert.Same(color, owner.ColorToken(oldToken));
        owner.SetEnabled(0, dirty); Assert.Equal(2u, dirty.Value); Assert.Same(color, owner.Color(97, 2)); Assert.Equal(memory, owner.Tokens);
        owner.SetEnabled(1, dirty); Assert.Equal(3u, dirty.Value); Assert.Equal(1u, enabled.Value); Assert.Equal(0u, memory.Span[94]); Assert.Equal(0u, memory.Span[97]); Assert.Equal(memory, owner.Tokens); Assert.Throws<ArgumentOutOfRangeException>(() => owner.ColorToken(oldToken)); Assert.Throws<ArgumentOutOfRangeException>(() => owner.ColorToken(Token(82) + 1));
        owner.SetEnabled(0, dirty); Assert.NotSame(color, owner.Color(97, 2)); Assert.Same(owner.Color(97, 2), owner.ColorToken(oldToken)); Assert.Throws<ArgumentException>(() => owner.Initialize(Primary(), 188, 82, dirty)); Assert.Equal(memory, owner.Tokens);
        var skipped = new OwnerColorTableState(); skipped.Initialize(Primary(), 188, 82, new(), uint.MaxValue); Assert.Equal(0u, skipped.Tokens.Span[82]); Assert.Equal(0u, skipped.Tokens.Span[155]); Assert.NotEqual(0u, skipped.Tokens.Span[158]);
    }
    /// <summary>선택 위치의 장치 요구와 store/free 순서·효과 뒤 현재 TYPE/포인터를 확인한다.</summary>
    [Fact]
    public void LazyGatesCurrentTablesAndPartialFailurePreserveOrderedEffects()
    {
        var globals = new OwnerColorTableGlobals { Ready = new(), Enabled = new() { Value = 1 }, Redraw = new() { Value = uint.MaxValue }, Tables = new uint[188] }; var types = new OwnerColorTableTypes(); var missing = new OwnerColorTableHooks(); OwnerColorTableOperations.SetBannerOption(globals, 0, types, missing); Assert.Equal(uint.MaxValue, globals.Redraw.Value);
        globals.Ready.Value = 1; OwnerColorTableOperations.SetBannerOption(globals, 1, types, missing); Assert.Equal(uint.MaxValue, globals.Redraw.Value); globals.Enabled.Value = 0; globals.Tables[85] = 111; globals.Tables[71] = 222; List<uint> seen = [];
        var hooks = new OwnerColorTableHooks { Release = token => { Assert.Equal(1u, globals.Enabled.Value); Assert.Equal(0u, globals.Redraw.Value); seen.Add(token); if (token == 111) { Assert.Equal(111u, globals.Tables[85]); types.Banner[1] = 128; globals.Tables[128] = 333; } } }; OwnerColorTableOperations.SetBannerOption(globals, 1, types, hooks); Assert.Equal(new uint[] { 111, 333 }, seen); Assert.Equal(0u, globals.Tables[85]); Assert.Equal(222u, globals.Tables[71]); Assert.Equal(0u, globals.Tables[128]);
        globals.Enabled.Value = 0; globals.Tables[85] = 444; hooks.Release = token => { Assert.Equal(444u, token); throw new InvalidOperationException("명시 반납 중단"); }; Assert.Throws<InvalidOperationException>(() => OwnerColorTableOperations.SetBannerOption(globals, 2, types, hooks)); Assert.Equal(2u, globals.Enabled.Value); Assert.Equal(1u, globals.Redraw.Value); Assert.Equal(444u, globals.Tables[85]); OwnerColorTableOperations.SetBannerOption(globals, 2, types, missing); Assert.Equal(444u, globals.Tables[85]);
    }
}
