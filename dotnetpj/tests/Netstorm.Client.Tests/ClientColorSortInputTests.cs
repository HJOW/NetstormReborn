using System.Runtime.InteropServices;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 최초 입력과 실제 primary/9표의 수명을 검사한다.

/// <summary>기존 고정6관측과 두 화면/파일의 실제 메모리를 대조한다.</summary>
public sealed class ClientColorSortInputTests
{
    /// <summary>독립 원문의 53비트 전체9표와 64비트 명도 차이를 그대로 유지한다.</summary>
    [Fact]
    public void WholeNativeSixFixedInputsAndStableArrays()
    {
        var rows = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "colorsort-fixed-input-x86.tsv")).Where(line => line.Length != 0 && line[0] != '#').Select(line => line.Split('\t')).ToArray(); Assert.Equal(6, rows.Length);
        var input = new ClientColorSortInput(); byte[][]? maps = null; ReadOnlyMemory<uint> same = default; uint[]? first = null; int extended = 0;
        // 각 행의 현재 화면 입력은 최초 캡처와 공유하지 않는다.
        foreach (var row in rows)
        {
            uint[] source = MemoryMarshal.Cast<byte, uint>(Convert.FromHexString(row[3])).ToArray(); uint[] current = MemoryMarshal.Cast<byte, uint>(Convert.FromHexString(row[4])).ToArray();
            if (!input.Captured) { input.Capture(source); same = input.Logical; first = source.ToArray(); }
            Assert.True(same.Equals(input.Logical)); Assert.Equal(first, input.Logical.ToArray()); byte[][] actual = input.Rebuild(uint.Parse(row[2], System.Globalization.CultureInfo.InvariantCulture)); string[] expected = row[6].Split(':'); Assert.Equal(9, expected.Length);
            // Native64에서 별도 차이가 있는 명도 표만 구분하고 나머지는 전체 바이트를 비교한다.
            for (int index = 0; index < 9; index++)
            {
                if (row[0] == "639" || index != 8) Assert.Equal(Convert.FromHexString(expected[index]), actual[index]); else { Assert.NotEqual(Convert.FromHexString(expected[index]), actual[index]); extended++; }
                if (maps is not null) Assert.Same(maps[index], actual[index]);
            }
            maps = actual; Array.Fill(current, 0x12345678u); Assert.Equal(first, input.Logical.ToArray()); Assert.True(same.Equals(input.Logical));
        }
        Assert.Equal(3, extended);
    }
    /// <summary>미캡처/미준비를 암묵 생성하지 않고 명시 캡처만 같은 입력 배열을 변경한다.</summary>
    [Fact]
    public void ExplicitCaptureAndFailedPrimaryPublication()
    {
        var input = new ClientColorSortInput(); var owner = input.OwnerTables; Assert.False(input.Captured); Assert.False(input.MapsReady); Assert.Throws<InvalidOperationException>(() => input.Logical); Assert.Throws<InvalidOperationException>(() => input.Primary);
        uint[] first = Enumerable.Range(0, 256).Select(index => (uint)index * 0x010101).ToArray(); input.Capture(first); var same = input.Logical; Array.Fill(first, 0xffffffu); Assert.Equal(0x111111u, input.Logical.Span[17]); Assert.Throws<InvalidOperationException>(() => input.Maps);
        byte[][] maps = input.Rebuild(0x123); Assert.Equal(0x23, maps[1][17]); input.Capture(first); Assert.True(same.Equals(input.Logical)); Assert.Equal(0xffffffu, input.Logical.Span[17]); Assert.Same(owner, input.OwnerTables);
        Assert.Throws<ArgumentException>(() => input.CaptureWithPrimary(first, new byte[3])); Assert.False(input.Captured); Assert.False(input.MapsReady); Assert.Throws<InvalidOperationException>(() => input.Primary); Assert.Same(owner, input.OwnerTables);
        Assert.Equal(first, ClientColorSortInput.LogicalInput([], first).ToArray()); Assert.Throws<ArgumentException>(() => ClientColorSortInput.LogicalInput(new uint[255], first).ToArray());
    }
    /// <summary>두 화면의 현재 RGB 변경은 최초 primary/padding과 고정 명암 입력을 바꾸지 않는다.</summary>
    [Theory]
    [InlineData(1024)]
    [InlineData(1280)]
    public void SamePrimaryAcrossScreenAndPaletteChanges(int width)
    {
        uint[] first = Enumerable.Range(0, 256).Select(index => 0xa5000000u | (uint)index * 0x010101).ToArray(); var input = new ClientColorSortInput(); input.CaptureWithPrimary(first, Enumerable.Repeat((byte)0xa7, 0x3800).ToArray()); var primary = input.Primary; byte[] bytes = primary.Bytes.ToArray();
        var screen = new Screen(width, 720); var sameShade = screen.ShadeMaps(); screen.BindColorSortInput(() => input.Logical); byte[][] maps = input.Rebuild(11); screen.BindColorMaps(() => new GumpShadeMaps(input.Maps[4], input.Maps[6])); Assert.Equal(maps[4], screen.ShadeMaps().Bright); Assert.Equal(maps[6], screen.ShadeMaps().Dark); Assert.NotSame(maps[4], screen.ShadeMaps().Bright); Assert.NotSame(maps[6], screen.ShadeMaps().Dark); Assert.Same(sameShade, screen.ShadeMaps());
        // 실제 현재 화면은 변경하지만 최초 raw와 같은9표 배열은 유지한다.
        foreach (byte red in new byte[] { 71, 193 })
        {
            screen.SetPalette(17, 1, [new ScreenColor(red, 33, 129, 0)], false); Assert.NotEqual(first[17], screen.LogicalPalette[17]); Assert.Same(primary, input.Primary); Assert.Equal(bytes, input.Primary.Bytes); Assert.Equal(first, input.Logical.ToArray()); Assert.True(primary.Bytes.AsSpan(17 * 56 + 52, 4).ToArray().All(value => value == 0xa7));
            screen.LoadPalette(new GamePalette(new byte[0x400])); Assert.Equal(maps[4], screen.ShadeMaps().Bright); Assert.Same(sameShade, screen.ShadeMaps()); Assert.Equal(first, input.Logical.ToArray());
        }
        var replacement = new GumpShadeMaps(); screen.BindColorMaps(() => replacement); Assert.Same(sameShade, screen.ShadeMaps()); Assert.NotSame(replacement, screen.ShadeMaps()); replacement.Bright[17] = 0x99; Assert.Equal(0x99, screen.ShadeMaps().Bright[17]); replacement = new GumpShadeMaps(); Assert.Equal(0, screen.ShadeMaps().Bright[17]); Assert.Same(sameShade, screen.ShadeMaps());
        var independent = new Screen(width, 720); independent.BindColorSortInput(() => input.Logical); independent.LoadPalette(new GamePalette(new byte[0x400])); Assert.Equal(maps[4], independent.ShadeMaps().Bright); Assert.Equal(maps[6], independent.ShadeMaps().Dark);
    }
    /// <summary>실제 파일 순서와 짧은 읽기, 정확 열기 실패만 재생성을 선택한다.</summary>
    [Fact]
    public void CacheUsesSameNineArraysAndExactFileOrder()
    {
        var input = new ClientColorSortInput(); input.CaptureWithPrimary(new uint[256], new byte[0x3800]); var owner = input.OwnerTables; int reads = 0, writes = 0, initialized = 0; byte[] reader = new byte[24], writer = new byte[24]; var captured = new List<Memory<byte>>();
        var hooks = new ColorMapCacheHooks { Path = () => "!color.dat\0"u8.ToArray(), Owners = () => { initialized++; Assert.Same(owner, input.OwnerTables); Assert.False(input.MapsReady); }, OpenRead = (file, _, mode, create) => { Assert.True(file.Equals(reader.AsMemory())); Assert.Equal(0u, mode); Assert.Equal(1u, create); }, Read = (file, target, offset) => { Assert.True(file.Equals(reader.AsMemory())); Assert.Equal(uint.MaxValue, offset); captured.Add(target); target.Span[0] = (byte)++reads; return 1; }, Close = file => Assert.True(file.Equals(reader.AsMemory())), OpenWrite = (_, _, _, _) => writes++ };
        input.InitializeCache(hooks, reader, writer, 0x123); Assert.Equal(1, initialized); Assert.Equal(9, reads); Assert.Equal(0, writes); int[] order = [2, 7, 6, 4, 3, 5, 1, 0, 8];
        for (int index = 0; index < 9; index++) { Assert.True(captured[index].Equals(input.Maps[order[index]].AsMemory())); Assert.Equal(index + 1, input.Maps[order[index]][0]); Assert.Equal(0, input.Maps[order[index]][1]); } // 같은 출력 배열의 부분 읽기다.
        Assert.Equal(2048, input.OwnerHighlight.Length); Assert.Equal(2048, input.OwnerSolid.Length); Assert.Throws<InvalidOperationException>(() => input.InitializeCache(hooks, reader, writer, 0));
    }
    /// <summary>캐시 미발견은 같은9표를 계산하고 파일 순서로 쓰며 선택 쓰기 실패도 그대로 전파한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void RegenerationUsesCurrentOwnerAndPreservesSelectedWriteFailure(bool failWrite)
    {
        var input = new ClientColorSortInput(); input.CaptureWithPrimary(Enumerable.Range(0, 256).Select(index => (uint)index * 0x010101).ToArray(), new byte[0x3800]); var primary = input.Primary; byte[] reader = new byte[24], writer = new byte[24]; var targets = new List<ReadOnlyMemory<byte>>(); int owners = 0, closes = 0;
        var hooks = new ColorMapCacheHooks
        {
            Path = () => "!color.dat\0"u8.ToArray(), Owners = () => { owners++; Assert.False(input.MapsReady); Assert.Same(primary, input.Primary); },
            OpenRead = (_, _, _, _) => throw new ColorCacheUnableToOpenException(), OpenWrite = (file, _, mode, create) => { Assert.Equal(1u, mode); Assert.Equal(1u, create); Assert.True(input.MapsReady); Assert.True(file.Equals(writer.AsMemory())); },
            Write = (file, table, offset) => { Assert.True(file.Equals(writer.AsMemory())); Assert.Equal(uint.MaxValue, offset); targets.Add(table); if (failWrite) throw new IOException("명시 쓰기 중단"); return 1; },
            Close = file => { Assert.True(file.Equals(writer.AsMemory())); closes++; }
        };
        if (failWrite) Assert.Throws<IOException>(() => input.InitializeCache(hooks, reader, writer, 0x123)); else input.InitializeCache(hooks, reader, writer, 0x123);
        Assert.Equal(1, owners); Assert.Equal(1, closes); Assert.True(input.MapsReady); Assert.Same(primary, input.Primary); Assert.Equal(0x23, input.Maps[1][17]); int[] order = [2, 7, 6, 4, 3, 5, 1, 0, 8]; Assert.Equal(failWrite ? 1 : 9, targets.Count);
        for (int index = 0; index < targets.Count; index++) Assert.True(targets[index].Equals((ReadOnlyMemory<byte>)input.Maps[order[index]])); // 같은 현재 표를 복사 없이 쓴다.
    }
}
