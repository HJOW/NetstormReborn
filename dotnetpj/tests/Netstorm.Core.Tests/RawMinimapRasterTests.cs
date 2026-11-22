using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests; // 실제 미니맵 셀과 갱신 전역의 대여를 검증한다.

/// <summary>4722개 독립 원본 관측의 입력과 전체 bitmap을 실제 Pool/Hash/TYPE에서 대조한다.</summary>
public sealed class RawMinimapRasterTests
{
    /// <summary>원문의DWORD 숫자다.</summary>
    private static uint N(string text) => uint.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>명시 폭만 써 이웃 필드의A5를 보존한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value, int width = 4)
    {
        // 원본 little-endian 입력 바이트만 바꾼다.
        for (int i = 0; i < width; i++) raw[at + i] = unchecked((byte)(value >> (i * 8)));
    }
    /// <summary>원본 출력 offset:BYTE를 전체 초기 표면에 복원한다.</summary>
    private static byte[] Observed(string text)
    {
        var result = new byte[0x4000]; Array.Fill(result, (byte)0xa5);
        if (text == "-") return result;
        // 실제 관찰한 쓰기 위치만 변경하며 색 선택 알고리즘을 반복하지 않는다.
        foreach (string item in text.Split(';')) { var pair = item.Split(':'); result[N(pair[0])] = checked((byte)N(pair[1])); }
        return result;
    }
    /// <summary>기존 원문 관측4722행을 빠짐없이 같은 제품 본체로 그린다.</summary>
    [Fact]
    public void Native4722MatchEveryByteOfFullBitmap()
    {
        int count = 0;
        // 앞9입력과 뒤7관측을 분리해 기대 픽셀은 입력으로 쓰지 않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "minimap-cell-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#')) continue;
            var row = line.Split('\t'); Assert.Equal(16, row.Length); var pool = new RawSidPool(24000, false); var hash = new RawSquidHash();
            var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); var state = new MinimapRasterState { Challenge = N(row[3]), SuppressColors = N(row[4]), Frame = N(row[5]) };
            var colors = row[6].Split(':'); Assert.Equal(12, colors.Length); state.Black = N(colors[0]); state.NeutralIsle = N(colors[9]); state.NeutralNoIsland = N(colors[10]); state.ChallengeGreen = N(colors[11]);
            // 일반 Player 색과 다른 미니맵 전용8색을 그대로 제공한다.
            for (int i = 0; i < 8; i++) state.Owners[i] = N(colors[i + 1]);
            if (row[7] != "-")
            {
                // 실제SID/타입/소유자/flags2/좌표/next/state/extra 입력이다.
                foreach (string item in row[7].Split(';'))
                {
                    var fields = item.Split(':'); Assert.Equal(9, fields.Length); int sid = (int)N(fields[0]), kind = (int)N(fields[1]); var raw = pool.RawSlot(sid); raw.Fill(0xa5);
                    Put(raw, 4, N(fields[6]), 2); Put(raw, 10, (uint)kind, 1); Put(raw, 11, N(fields[7]), 1); Put(raw, 14, N(fields[4])); Put(raw, 18, N(fields[5])); Put(raw, 34, N(fields[2]), 1); Put(raw, 40, N(fields[8]), 1); types[kind].Flags2 = N(fields[3]);
                }
            }
            if (row[8] != "-")
            {
                // 같은 실제 hash의level/x/y/head 입력만 바꾼다.
                foreach (string item in row[8].Split(';')) { var fields = item.Split(':'); Assert.Equal(4, fields.Length); hash.Cell((int)N(fields[0]), (int)N(fields[1]), (int)N(fields[2])) = checked((ushort)N(fields[3])); }
            }
            var bitmap = new byte[0x4000]; Array.Fill(bitmap, (byte)0xa5); new RawMinimapRaster(pool, hash, types, state).DrawCell(bitmap, (int)N(row[1]), (int)N(row[2]));
            Assert.True(bitmap.AsSpan().SequenceEqual(Observed(row[10])), row[0]); Assert.Equal("1", row[9]); Assert.Equal("0", row[13]); Assert.True(N(row[14]) > 0); Assert.Equal(64, row[15].Length); count++;
        }
        Assert.Equal(4722, count);
    }
    /// <summary>실제 공유 객체의 색/타입/슬롯 변경을 다음 호출에서 바로 읽는다.</summary>
    [Fact]
    public void ReadsCurrentOwnerPaletteFlagsAndCoordinateFromSameObjects()
    {
        var pool = new RawSidPool(24000, false); var hash = new RawSquidHash(); var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); var state = new MinimapRasterState();
        var raster = new RawMinimapRaster(pool, hash, types, state); var bitmap = new byte[0x4000]; var raw = pool.RawSlot(1); raw.Clear(); raw[10] = 5; raw[34] = 2;
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 10); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 12); hash.Cell(2, 0, 0) = 1; types[5].Flags2 = 4; state.Owners[1] = 19;
        raster.DrawCell(bitmap, 0, 0); Assert.Equal(19, bitmap[6 * 128 + 5]);
        state.Owners[1] = 73; BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 14); raster.DrawCell(bitmap, 0, 0); Assert.Equal(73, bitmap[6 * 128 + 7]);
        types[5].Flags2 = 0; bitmap[6 * 128 + 7] = 99; raster.DrawCell(bitmap, 0, 0); Assert.Equal(99, bitmap[6 * 128 + 7]);
    }
    /// <summary>순환 체인은 성공이나 무한 반복으로 숨기지 않는다.</summary>
    [Fact]
    public void RejectsCyclicCurrentHashChain()
    {
        var pool = new RawSidPool(24000, false); var hash = new RawSquidHash(); var raw = pool.RawSlot(1); raw.Clear(); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], 1); hash.Cell(2, 0, 0) = 1;
        var raster = new RawMinimapRaster(pool, hash, [new()], new()); Assert.Throws<InvalidOperationException>(() => raster.DrawCell(new byte[0x4000], 0, 0));
    }
    /// <summary>실제 판본/빈 TYPE/잘못된 bitmap과 셀은 명시적으로 거부한다.</summary>
    [Fact]
    public void RejectsWrongEditionEmptyTypesAndInvalidBitmapCells()
    {
        var pool = new RawSidPool(24000, false); var hash = new RawSquidHash();
        Assert.Throws<ArgumentException>(() => new RawMinimapRaster(new RawSidPool(24000, false, edition: OriginalEdition.Cd1072), hash, [new()], new()));
        Assert.Throws<ArgumentException>(() => new RawMinimapRaster(pool, hash, [], new())); var raster = new RawMinimapRaster(pool, hash, [new()], new());
        Assert.Throws<ArgumentOutOfRangeException>(() => raster.DrawCell(new byte[3], 0, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => raster.DrawCell(new byte[0x4000], 64, 0));
    }
    /// <summary>공유DWORD는 외부 변경과 큐의 최초 확보/전체 갱신/지우기를 양방향으로 즉시 반영한다.</summary>
    [Fact]
    public void DirtyBorrowsSameFullRefreshWordWithoutCopyingStandaloneValue()
    {
        var word = new UIntBox { Value = 7 }; var state = new MetadisplayDirtyState { FullRefresh = 19, SharedFullRefresh = word }; var pool = new RawSidPool(24000, false); int cells = 0;
        var dirty = new RawMetadisplayDirty(pool, state, new() { Rebuild = () => { }, RedrawCell = (_, _) => cells++ }); Assert.Equal(7U, state.FullRefresh);
        dirty.Refresh(default); Assert.Equal(1U, word.Value); state.DisplayPresent = true; state.DisplayFlags = 1; dirty.Refresh(default); Assert.Equal(4096, cells); Assert.Equal(0U, word.Value);
        word.Value = 3; dirty.Refresh(default); Assert.Equal(8192, cells); Assert.Equal(0U, word.Value);
        state.SharedFullRefresh = null; Assert.Equal(19U, state.FullRefresh);
    }
}
