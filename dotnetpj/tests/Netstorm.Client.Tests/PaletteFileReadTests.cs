using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 최초 PE92행의 File 여섯 필드·저장 팔레트·장치 효과열만 재생한다.
namespace Netstorm.Client.Tests;

/// <summary>Screen/OS/색 검색으로 확대하지 않고 실제 파일 읽기 부모의 독립 원문 자료를 대조한다.</summary>
public sealed class PaletteFileReadTests
{
    /// <summary>fixture가 명시한 실제 File/키의 불투명 DWORD 토큰이다.</summary>
    private static readonly nuint FileToken = 0x12200000, KeyToken = 0x12300100;

    /// <summary>원문 여섯 필드를 signed 위치의 DWORD 비트까지 보존해 직렬화한다.</summary>
    private static string Fields(RawBaseFileState state) => string.Join(",", new ulong[]
    {
        (ulong)state.File, state.Flags, unchecked((uint)state.Offset), unchecked((uint)state.Length),
        (ulong)state.Key, state.KeyLength,
    }.Select(value => value.ToString(CultureInfo.InvariantCulture)));

    /// <summary>Native92행의 기대값을 그대로 읽으며 C# 결과로 새 기대값을 만들지 않는다.</summary>
    [Fact]
    public void Native92RowsKeepDirectSavedWritesAndFileCleanupPrefixes()
    {
        string[][] rows = X86Fixture.Read("palette-file-read");
        Assert.Equal(92, rows.Length);
        // 각 행은 같은 명시 입력에서 시작하고 이전 관측의 파일/작업 BYTE를 공유하지 않는다.
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            string[] input = row[0].Split(','); Assert.Equal(10, input.Length);
            int length = X86Fixture.Int(input[0]); uint key = X86Fixture.UInt(input[1]), sub = X86Fixture.UInt(input[2]);
            int partial = X86Fixture.Int(input[3]); uint scenario = X86Fixture.UInt(input[7]), failRead = X86Fixture.UInt(input[8]), returnAssert = X86Fixture.UInt(input[9]);
            byte[] stored = new byte[length], keyBytes = "abc"u8.ToArray();
            // 원문이 명시한 입력 파일 생성식이며 기대 saved 바이트를 계산하는 식이 아니다.
            for (int index = 0; index < stored.Length; index++)
                stored[index] = unchecked((byte)(((index * 29 + 7) & 255) ^ (key != 0 ? keyBytes[index % 3] : 0)));
            byte[] saved = Enumerable.Repeat((byte)0xa5, 1024).ToArray(), scratch = new byte[4096];
            string events = "", outcome = "ok"; int position = 0; uint reads = 0;
            var state = new RawBaseFileState { File = FileToken, Flags = 0x65656565, Offset = 0x65656565, Length = 0x65656565, Key = KeyToken, KeyLength = 3 };
            // 현재 진입 순서만 기록하며 callback이 원문 본체의 분기를 대신 실행하지 않는다.
            void Event(string text) => events += (events.Length == 0 ? "" : ";") + text;
            var io = new RawBaseFileIOHooks();
            var hooks = new PaletteFileReadHooks
            {
                Open = (current, name, mode, create) =>
                {
                    Assert.Equal("palette.col\0"u8.ToArray(), name.ToArray()); Assert.Equal(0u, mode); Assert.Equal(1u, create);
                    Assert.Equal((nuint)0, current.Key); Assert.Equal(0u, current.KeyLength); Event("O");
                    current.File = scenario == 2 ? 0 : FileToken; current.Flags = 0x80u | sub;
                    current.Offset = sub != 0 ? 37 : 0; current.Length = sub != 0 ? length : 0;
                    current.Key = key != 0 ? KeyToken : 0; current.KeyLength = key != 0 ? 3u : 0;
                    position = sub != 0 ? 37 : 0;
                    if (scenario == 1) throw new InvalidOperationException("open");
                },
                Assertion = line => { Event("D:" + line); if (returnAssert == 0) throw new InvalidOperationException("assertion"); },
                Apply = () => { Assert.Equal(FileToken, state.File); Event("A"); if (scenario == 4) throw new InvalidOperationException("apply"); },
            };
            io.Tell = file => { Assert.Equal(state.File, file); Event("T"); return position; };
            io.Seek = (file, offset, origin) =>
            {
                Assert.Equal(state.File, file); Event("S:" + offset + ":" + origin);
                position = origin == 2 ? (sub != 0 ? 37 : 0) + length : origin == 1 ? position + offset : offset;
            };
            io.Read = (file, bytes, size, count) =>
            {
                if (file == 0) { Event("R:null:" + size + ":" + count); throw new InvalidOperationException("nullRead"); }
                Assert.Equal(FileToken, file); int start = position - (sub != 0 ? 37 : 0); Assert.True(start >= 0);
                int requested = checked((int)(size * count)), actual = Math.Min(requested, Math.Max(0, length - partial - start));
                Assert.True(bytes.Length >= requested); reads++; Event("R:" + size + ":" + count + ":" + actual);
                if (scenario == 3 && reads == failRead) throw new InvalidOperationException("read");
                if (actual != 0) stored.AsMemory(start, actual).CopyTo(bytes);
                position += actual;
            };
            io.Key = (token, size) => { Assert.Equal(KeyToken, token); Assert.Equal(3u, size); return keyBytes; };
            io.Close = file => { Assert.Equal(FileToken, file); Event("C"); if (scenario == 5) throw new InvalidOperationException("close"); };
            io.FreeKey = token => { Assert.Equal(KeyToken, token); Event("K"); };
            io.Assertion = hooks.Assertion;
            try { PaletteFileRead.Read(state, "palette.col\0"u8.ToArray(), saved, io, hooks, scratch); }
            catch (InvalidOperationException error) { outcome = error.Message; }
            Assert.Equal(row[1], outcome); Assert.Equal(row[2], Fields(state));
            Assert.Equal(row[3], Convert.ToHexStringLower(saved)); Assert.Equal(row[4], events);
            // row5의 화면 전역과 row6의 색 검색 수는 이 reader 검사의 증거로 합산하지 않는다.
        }
    }
}
