using System.Globalization;
using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 상태/고리/장치순서를 독립 원문 전체와 대조한다.

/// <summary>전체1364관측과 선택된 검색·파일 실패의 수명이다.</summary>
public sealed class RawBaseFileOpenPolicyTests
{
    /// <summary>원문의 FILE/기존키/새할당 토큰이다.</summary>
    private const uint FileToken = 0x12200000, OldKey = 0x12300000, NewKey = 0x12304000;
    /// <summary>원문 열기 예외를 공급하는 명시 장치다.</summary>
    private sealed class OpenFailure : Exception { }
    /// <summary>원시 십진 DWORD다.</summary>
    private static uint U(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>NUL/상위BYTE를 보존하는 원문 hex다.</summary>
    private static string Hex(ReadOnlySpan<byte> value) => Convert.ToHexStringLower(value);
    /// <summary>실제6필드의 원시 DWORD 비트다.</summary>
    private static string Fields(RawBaseFileState state) => $"{state.File},{state.Flags},{unchecked((uint)state.Offset)},{unchecked((uint)state.Length)},{state.Key},{state.KeyLength}";
    /// <summary>최초 Native 출력과 실제 호출/필드/256BYTE·할당을 전수 비교한다.</summary>
    [Fact]
    public void WholeNative1364ModesSourcesFieldsAndRing()
    {
        var rows = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "basefile-open-policy-x86.tsv")).Where(line => line.Length != 0 && line[0] != '#').Select(line => line.Split('\t')).ToArray(); Assert.Equal(1364, rows.Length);
        // 같은 입력 mask/외부 효과만 제공하고 분기는 실제 제품 본체가 결정한다.
        foreach (var row in rows)
        {
            byte[] input = (row[0] == "-" ? Array.Empty<byte>() : Convert.FromHexString(row[0])).Concat(new byte[] { 0 }).ToArray(); int nul = Array.IndexOf(input, (byte)0); string original = Encoding.Latin1.GetString(input, 0, nul); bool device = original.Contains(':') || original.StartsWith("\\\\", StringComparison.Ordinal);
            uint mode = U(row[1]), create = U(row[2]), mask = U(row[3]), archiveOpen = U(row[4]), encrypted = U(row[5]), initial = U(row[6]), createAlt = U(row[7]), allocationFail = U(row[8]);
            var state = new RawBaseFileState { File = 0x1001, Flags = 0x82, Offset = 7, Length = 9, Key = OldKey, KeyLength = 3 }; var names = new RawFileName();
            for (int call = 0; call < initial; call++) names.Format([]); // 원문과 같은 초기 고리 위치다.
            var events = new List<string>(); byte[] key = [];
            var io = new RawBaseFileIOHooks
            {
                Open = (name, fileMode) =>
                {
                    string text = Encoding.Latin1.GetString(name.Span); events.Add($"O:{Hex(name.Span)}:{fileMode}"); bool archive = text is "primary.tarc" or "secondary.tarc"; uint site = device && text == original ? 0u : text.StartsWith("C:\\base", StringComparison.Ordinal) ? 1u : 3u;
                    bool success = archive ? archiveOpen != 0 : (mask & (1u << (int)site)) != 0 || (createAlt != 0 && fileMode == "w+b" && mode == 1 && create != 0); return success ? FileToken : 0;
                },
                Seek = (token, offset, origin) => events.Add($"S:{token}:{unchecked((uint)offset)}:{origin}"), FreeKey = token => events.Add($"F:{token}"),
                AllocateKey = size => { events.Add($"N:{size}"); if (allocationFail != 0) throw new OutOfMemoryException(); key = Enumerable.Repeat((byte)205, checked((int)size)).ToArray(); return NewKey; },
                MutableKey = (token, size) => { Assert.Equal((nuint)NewKey, token); Assert.Equal((uint)key.Length, size); return key; }
            };
            var hooks = new BaseFileOpenPolicyHooks
            {
                PrimaryRoot = () => "C:\\base"u8.ToArray(), SecondaryRoot = () => "D:\\disc"u8.ToArray(),
                Archive = (name, secondary) => { events.Add($"A:{(secondary ? 1 : 0)}:{Hex(name.Span)}"); return (mask & (1u << (secondary ? 4 : 2))) == 0 ? null : new BaseFileArchiveOpen(secondary ? "secondary.tarc"u8.ToArray() : "primary.tarc"u8.ToArray(), secondary ? 0x234 : 0x123, secondary ? 0x456 : 0x345, encrypted); },
                UnableToOpen = name => { events.Add($"E:{Hex(name.Span)}"); throw new OpenFailure(); }
            };
            string outcome = "ok"; try { RawBaseFileOpenPolicy.OpenBaseFileByPolicy(state, input, mode, create, names, io, hooks); } catch (OpenFailure) { outcome = "unable"; } catch (OutOfMemoryException) { outcome = "allocation"; }
            Assert.Equal(row[9], outcome); Assert.Equal(row[10], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(row[11], Fields(state)); Assert.Equal((int)U(row[12]), names.Index); Assert.Equal(row[13], Hex(names.CurrentBytes.Span)); Assert.Equal(row[14], key.Length == 0 ? "-" : Hex(key));
        }
    }
    /// <summary>실제 열기 전체 수명은 현재 검색 핸들을 root 예외 뒤에도 반납한다.</summary>
    [Fact]
    public void SearchHandleClosesAfterSelectedRootFailure()
    {
        var names = new RawFileName(); var closed = new List<nuint>(); var state = new RawBaseFileState(); var hooks = new BaseFileOpenPolicyHooks { Search = new() { Find = _ => new(0x1234, "chosen"u8.ToArray()), Close = handle => closed.Add(handle) }, PrimaryRoot = () => throw new IOException("명시 root 공급 예외") };
        Assert.Throws<IOException>(() => RawBaseFileOpenPolicy.OpenBaseFileByPolicy(state, "d\\*\0"u8.ToArray(), 0, 0, names, new(), hooks)); Assert.Equal(new nuint[] { 0x1234 }, closed); Assert.Equal(1, names.Index); Assert.Equal((nuint)0, state.File);
    }
    /// <summary>선택된 TAFF 열기 실패는 보조 파일로 숨기지 않고 원문 예외의 비복귀도 요구한다.</summary>
    [Fact]
    public void SelectedArchiveOpenFailureAndReturningUnableKeepPrefix()
    {
        int secondary = 0, keys = 0; var names = new RawFileName(); var state = new RawBaseFileState(); var hooks = new BaseFileOpenPolicyHooks { PrimaryRoot = () => "C:\\base"u8.ToArray(), SecondaryRoot = () => { secondary++; return "D:\\disc"u8.ToArray(); }, Archive = (_, _) => new("primary.tarc"u8.ToArray(), 20, 30, 1), UnableToOpen = _ => throw new OpenFailure() };
        var io = new RawBaseFileIOHooks { Open = (_, _) => 0, AllocateKey = _ => { keys++; return 1; } }; Assert.Throws<OpenFailure>(() => RawBaseFileOpenPolicy.OpenBaseFileByPolicy(state, "d/a\0"u8.ToArray(), 0, 0, names, io, hooks)); Assert.Equal(0, secondary); Assert.Equal(0, keys); Assert.Equal(2, names.Index); Assert.Equal((nuint)0, state.File);
        hooks.UnableToOpen = _ => { }; Assert.Throws<InvalidOperationException>(() => RawBaseFileOpenPolicy.OpenBaseFileByPolicy(state, "C:\\absent\0"u8.ToArray(), 2, 0, names, io, hooks)); Assert.Equal(0, secondary);
    }
    /// <summary>명시 OS 매핑의 실패는 선행 고리를 남기고 다음 archive는 현재 NUL 이름을 다시 읽는다.</summary>
    [Fact]
    public void SelectedDiskMappingKeepsFormattedPrefixAndCurrentName()
    {
        byte[] input = new byte[32]; "d\\first\0"u8.CopyTo(input); var names = new RawFileName(); var state = new RawBaseFileState(); int maps = 0, archives = 0;
        var hooks = new BaseFileOpenPolicyHooks
        {
            PrimaryRoot = () => "C:\\base"u8.ToArray(),
            DiskPath = (root, name) => { maps++; Assert.Equal("C:\\base"u8.ToArray(), root.ToArray()); Assert.Equal("d\\first"u8.ToArray(), name.ToArray()); Assert.Equal(2, names.Index); "d\\later\0"u8.CopyTo(input); return null; },
            Archive = (name, secondary) => { archives++; Assert.False(secondary); Assert.Equal("d\\later"u8.ToArray(), name.ToArray()); return new("data.tarc"u8.ToArray(), 3, 8, 0); }
        };
        var io = new RawBaseFileIOHooks { Open = (name, mode) => { Assert.Equal("data.tarc"u8.ToArray(), name.ToArray()); Assert.Equal("rb", mode); return FileToken; }, Seek = (file, offset, origin) => { Assert.Equal((nuint)FileToken, file); Assert.Equal(3, offset); Assert.Equal(0, origin); } };
        RawBaseFileOpenPolicy.OpenBaseFileByPolicy(state, input, 0, 0, names, io, hooks); Assert.Equal(1, maps); Assert.Equal(1, archives); Assert.Equal("C:\\base\\d\\first", Encoding.Latin1.GetString(names.CurrentBytes.Span[..names.CurrentBytes.Span.IndexOf((byte)0)])); Assert.Equal(3, state.Offset); Assert.Equal(8, state.Length);
        hooks.DiskPath = (root, name) => { Assert.True(root.IsEmpty); Assert.Equal("C:\\original"u8.ToArray(), name.ToArray()); return new ReadOnlyMemory<byte>("mapped"u8.ToArray()); }; io.Open = (name, mode) => { Assert.Equal("mapped"u8.ToArray(), name.ToArray()); Assert.Equal("w+b", mode); return FileToken; };
        RawBaseFileOpenPolicy.OpenBaseFileByPolicy(state, "C:\\original\0"u8.ToArray(), 2, 0, names, io, hooks); Assert.Equal(3, names.Index); Assert.Equal(0, state.Offset); Assert.Equal(0, state.Length);
    }
}
