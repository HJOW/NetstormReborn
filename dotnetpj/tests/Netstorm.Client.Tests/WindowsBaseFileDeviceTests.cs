using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 실제 OS FILE과 같은 캐시/등록 snapshot의 구분을 검사한다.

/// <summary>Source 실제 디스크/TAFF 소비와 같은 ColorMap reader/writer 수명이다.</summary>
public sealed class WindowsBaseFileDeviceTests
{
    /// <summary>보호 원본 밖의 이번 검사만 소유하는 고유 Unicode 폴더다.</summary>
    private static string Work()
    {
        string root = Path.Combine(Path.GetTempPath(), "Netstorm-BaseFile-" + Guid.NewGuid().ToString("N"), "한글😀"); Directory.CreateDirectory(root); return root;
    }
    /// <summary>현재 원시 UTF-8 NUL 이름이다.</summary>
    private static byte[] Name(string value) => Encoding.UTF8.GetBytes(value + '\0');
    /// <summary>입력 평문과 독립 TAFF 헤더/인덱스/레코드/키 시작을 구성한다.</summary>
    private static byte[] Archive(uint flags = 2, bool duplicate = false)
    {
        int count = duplicate ? 2 : 1; byte[] raw = new byte[0x100 + count * 4]; "TAFF v0.2\x1a"u8.CopyTo(raw); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x14), count); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x18), flags); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x1c), 0x2c); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x20), 0x40); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x28), 0x100); int record = 0x40;
        // 각 엔트리는 별도의 키 위치0에서 시작하고 등록 원문 순서를 유지한다.
        for (int index = 0; index < count; index++)
        {
            byte[] name = Encoding.ASCII.GetBytes(index == 0 ? "\\d\\a\0" : "D/A\0"); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x2c + index * 4), record - 0x40); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(record), index * 4); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(record + 4), 4); name.CopyTo(raw, record + 8); record += 8 + name.Length;
            byte[] data = index == 0 ? "ABCD"u8.ToArray() : "WXYZ"u8.ToArray(); if ((flags & 2) != 0) XorCipher.Apply(data); data.CopyTo(raw, 0x100 + index * 4);
        }
        BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x24), record - 0x40); return raw;
    }
    /// <summary>동일 환경·동일 FileNameOwner를 사용하는 원문 열기 입구다.</summary>
    private static void Open(GameFileSystem files, WindowsBaseFileDevice file, string name, uint mode = 0, uint create = 0) => RawBaseFileEnvironment.OpenByPolicy(files, file.State, Name(name), mode, create, file.IO);
    /// <summary>Source 실제 빈/Unicode 디스크와 long32 위치·부분 접두/EOF 및 r+b 꼬리 보존이다.</summary>
    [Fact]
    public void ActualCrtUnicodePartialReadPositionAndUpdateKeepTail()
    {
        string root = Work(), path = Path.Combine(root, "파일😀.bin"); var files = new GameFileSystem(root, []); byte[] input = Enumerable.Range(0, 8201).Select(index => unchecked((byte)(index * 17 + 3))).ToArray();
        using (var writer = new WindowsBaseFileDevice()) { Open(files, writer, path, 2); RawBaseFileIO.WriteBaseFile(writer.State, input, -1, writer.IO); Assert.Equal(8201, RawBaseFileIO.BaseFileLength(writer.State, writer.IO)); }
        Assert.Equal(input, File.ReadAllBytes(path));
        using (var update = new WindowsBaseFileDevice()) { Open(files, update, path, 1, 1); RawBaseFileIO.WriteBaseFile(update.State, new byte[] { 99 }, -1, update.IO); }
        input[0] = 99; Assert.Equal(input, File.ReadAllBytes(path));
        using (var reader = new WindowsBaseFileDevice())
        {
            Open(files, reader, path); byte[] destination = Enumerable.Repeat((byte)0xcc, 12).ToArray(); RawBaseFileIO.ReadBaseFile(reader.State, destination, 8196, reader.IO); Assert.Equal(input[8196..], destination[..5]); Assert.All(destination[5..], value => Assert.Equal(0xcc, value)); Assert.Equal(8201, reader.IO.Tell!(reader.State.File)); Assert.Equal(0x10u, RawBaseFileIO.BaseFileEof(reader.State, reader.IO));
        }
        string empty = Path.Combine(root, "빈.bin"); File.WriteAllBytes(empty, []); using var blank = new WindowsBaseFileDevice(); Open(files, blank, empty); Assert.Equal(0, RawBaseFileIO.BaseFileLength(blank.State, blank.IO)); RawBaseFileIO.WriteBaseFile(blank.State, ReadOnlyMemory<byte>.Empty, -1, blank.IO); Assert.Empty(File.ReadAllBytes(empty));
    }
    /// <summary>등록 metadata는 현재 파일 존재/내용과 독립이며 RawRead는 현재 payload, snapshot은 이전 payload다.</summary>
    [Fact]
    public void RegisteredMetadataUsesCurrentArchiveFileAndKeepsSnapshot()
    {
        string root = Work(), path = Path.Combine(root, "MiXeD😀.tarc"); byte[] raw = Archive(); File.WriteAllBytes(path, raw); var files = GameFileSystem.Open(root); var metadata = Assert.IsType<GameArchiveOpenMetadata>(files.FindArchiveOpen("d/a"u8.ToArray())); Assert.Equal(Path.GetFullPath(path), metadata.FileName); Assert.Equal(0x100, metadata.Offset); Assert.Equal(4, metadata.Length); Assert.Equal(2u, metadata.Encrypted); Assert.Equal(0, files.FileNameOwner.Index);
        raw[0x100] = (byte)('Z' ^ 'm'); File.WriteAllBytes(path, raw);
        var reader = new WindowsBaseFileDevice(); using (reader) { Open(files, reader, "d/a"); byte[] bytes = new byte[4]; RawBaseFileIO.ReadBaseFile(reader.State, bytes, -1, reader.IO, new byte[4]); Assert.Equal("ZBCD"u8.ToArray(), bytes); Assert.Equal(1u, reader.State.Flags); Assert.Equal(13u, reader.State.KeyLength); Assert.Equal(2, files.FileNameOwner.Index); }
        Assert.Equal((nuint)0, reader.State.File); Assert.Equal((nuint)0, reader.State.Key); Assert.Equal(13u, reader.State.KeyLength); Assert.Equal("ABCD"u8.ToArray(), files.ReadAllBytes("d/a"));
        File.Move(path, Path.Combine(root, "moved.tarc")); Assert.Equal(metadata, files.FindArchiveOpen("d/a"u8.ToArray())); using var missing = new WindowsBaseFileDevice(); Assert.Throws<ColorCacheUnableToOpenException>(() => Open(files, missing, "d/a")); Assert.Equal((nuint)0, missing.State.File); Assert.Equal((nuint)0, missing.State.Key);
    }
    /// <summary>현재 raw metadata는 첫 중복·선택 목록·등록 flags를 사용하며 익명 메모리를 실제 파일로 꾸미지 않는다.</summary>
    [Fact]
    public void MetadataFirstDuplicateAndSelectedListKeepRegistration()
    {
        string root = Work(), primary = Path.Combine(root, "primary"), secondary = Path.Combine(root, "secondary"); Directory.CreateDirectory(primary); Directory.CreateDirectory(secondary); string first = Path.Combine(primary, "first.tarc"), second = Path.Combine(secondary, "second.tarc"); File.WriteAllBytes(first, Archive(2, true)); File.WriteAllBytes(second, Archive(0)); var files = GameFileSystem.Open(primary, secondary);
        var a = Assert.IsType<GameArchiveOpenMetadata>(files.FindArchiveOpen("./D/a"u8.ToArray())); var b = Assert.IsType<GameArchiveOpenMetadata>(files.FindArchiveOpen("\\d\\a"u8.ToArray(), true)); Assert.Equal(0x100, a.Offset); Assert.Equal(2u, a.Encrypted); Assert.Equal(Path.GetFullPath(first), a.FileName); Assert.Equal(Path.GetFullPath(second), b.FileName); Assert.Equal(0u, b.Encrypted); Assert.Null(files.FindArchiveOpen("d/missing"u8.ToArray())); Assert.Equal(0, files.FileNameOwner.Index);
        using (var plain = new WindowsBaseFileDevice()) { Open(new GameFileSystem(secondary, [TaffArchive.Open(second)]), plain, "d/a"); byte[] data = new byte[4]; RawBaseFileIO.ReadBaseFile(plain.State, data, -1, plain.IO); Assert.Equal("ABCD"u8.ToArray(), data); Assert.Equal((nuint)0, plain.State.Key); }
        byte[] original = Archive(); var snapshot = new TaffArchive(original); original[0x18] = 0; Assert.Equal(2u, snapshot.Flags); var anonymous = new GameFileSystem(root, [snapshot]); Assert.Throws<InvalidOperationException>(() => anonymous.FindArchiveOpen("d/a"u8.ToArray()));
    }
    /// <summary>암호 읽기의 짧은 CRT 공급은 이전scratch 잔여 전체를 요청 길이대로 XOR한다.</summary>
    [Fact]
    public void ActualShortEncryptedReadPreservesRequestedScratchTail()
    {
        string root = Work(), path = Path.Combine(root, "data.tarc"); File.WriteAllBytes(path, Archive()); var files = GameFileSystem.Open(root); using var reader = new WindowsBaseFileDevice(); Open(files, reader, "d/a"); byte[] destination = new byte[9], scratch = Enumerable.Repeat((byte)0xa5, 9).ToArray(); RawBaseFileIO.ReadBaseFile(reader.State, destination, -1, reader.IO, scratch); Assert.Equal(new byte[] { 65, 66, 67, 68, 0xc2, 0xcd, 0xc4, 0xd6, 0xc3 }, destination); Assert.Equal(destination, scratch); Assert.Equal(0x104, reader.IO.Tell!(reader.State.File));
    }
    /// <summary>실제 상대 writer는 보조 디스크로 가며 직접 장치 실패와 이미 찾은archive 실패는 우회하지 않는다.</summary>
    [Fact]
    public void ActualRelativeWriterAndDirectOrArchiveFailuresKeepSourceOrder()
    {
        string root = Work(), primary = Path.Combine(root, "primary"), secondary = Path.Combine(root, "secondary"); Directory.CreateDirectory(primary); Directory.CreateDirectory(Path.Combine(secondary, "d")); var files = new GameFileSystem(primary, [], secondary, 0);
        using (var writer = new WindowsBaseFileDevice()) { Open(files, writer, "d/new.bin", 2); RawBaseFileIO.WriteBaseFile(writer.State, new byte[] { 3, 1, 4 }, -1, writer.IO); } Assert.Equal(new byte[] { 3, 1, 4 }, File.ReadAllBytes(Path.Combine(secondary, "d/new.bin"))); Assert.Equal(3, files.FileNameOwner.Index);
        using (var direct = new WindowsBaseFileDevice()) { Assert.Throws<ColorCacheUnableToOpenException>(() => Open(files, direct, Path.Combine(primary, "missing", "new.bin"), 2)); Assert.Equal(4, files.FileNameOwner.Index); } Assert.False(File.Exists(Path.Combine(secondary, "missing", "new.bin")));
        string archive = Path.Combine(primary, "data.tarc"); File.WriteAllBytes(archive, Archive()); var registered = GameFileSystem.Open(primary, secondary); File.WriteAllBytes(Path.Combine(secondary, "d", "a"), "S"u8.ToArray()); File.Move(archive, Path.Combine(primary, "moved.tarc")); using var absent = new WindowsBaseFileDevice(); Assert.Throws<ColorCacheUnableToOpenException>(() => Open(registered, absent, "d/a")); Assert.Equal(2, registered.FileNameOwner.Index); Assert.Equal("S"u8.ToArray(), File.ReadAllBytes(Path.Combine(secondary, "d", "a")));
    }
    /// <summary>독립 고정 입력의53비트 원문 기대만 실제 캐시 전체 순서의 기준으로 쓴다.</summary>
    private static (uint[] Logical, uint Red, byte[][] Maps) NativeMaps()
    {
        string[] row = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "colorsort-fixed-input-x86.tsv")).First(line => line.Length != 0 && line[0] != '#' && line.StartsWith("639\t", StringComparison.Ordinal)).Split('\t'); return (MemoryMarshal.Cast<byte, uint>(Convert.FromHexString(row[3])).ToArray(), uint.Parse(row[2], System.Globalization.CultureInfo.InvariantCulture), row[6].Split(':').Select(value => Convert.FromHexString(value)).ToArray());
    }
    /// <summary>실제 두 FILE의 정리를 관측하며 같은 owner 표를 캐시 준비 전에 생성한다.</summary>
    private static List<WindowsBaseFileDevice> Cache(ClientColorSortInput input, GameFileSystem files, string path, uint red)
    {
        var devices = new List<WindowsBaseFileDevice>(); input.InitializeCache(files, path, red, () => { Assert.False(input.MapsReady); input.OwnerTables.Initialize(input.Primary, 188, 82, new UIntBox()); }, () => { var device = new WindowsBaseFileDevice(); devices.Add(device); return device; }); return devices;
    }
    /// <summary>실제 최초 누락은 Native9표를 평문으로 생성하고 다음 hit은 현재 빨강 재계산 없이 같은파일을 읽는다.</summary>
    [Fact]
    public void ActualColorCacheMissAndHitMatchWholeNativeNineMaps()
    {
        string root = Work(), cache = Path.Combine(root, "Cache"); Directory.CreateDirectory(cache); string path = Path.Combine(cache, "!color.dat"); var files = new GameFileSystem(root, []); var native = NativeMaps(); var input = new ClientColorSortInput(); input.CaptureWithPrimary(native.Logical, Enumerable.Repeat((byte)0xa7, 0x3800).ToArray()); var first = Cache(input, files, path, native.Red); int[] order = [2, 7, 6, 4, 3, 5, 1, 0, 8]; byte[] expected = order.SelectMany(index => native.Maps[index]).ToArray(); Assert.Equal(expected, File.ReadAllBytes(path)); Assert.True(input.OwnerTables.Ready); Assert.Equal(2, first.Count); Assert.NotSame(first[0], first[1]); Assert.All(first, file => { Assert.Equal((nuint)0, file.State.File); Assert.Equal((nuint)0, file.State.Key); });
        var next = new ClientColorSortInput(); next.CaptureWithPrimary(native.Logical, new byte[0x3800]); var second = Cache(next, files, path, (uint)(native.Maps[1][17] ^ 255)); Assert.Equal(expected, File.ReadAllBytes(path)); Assert.All(second, file => Assert.Equal((nuint)0, file.State.File));
        for (int index = 0; index < 9; index++) Assert.Equal(native.Maps[index], next.Maps[index]); // 실제 파일이 현재 다른 빨강보다 우선한다.
        Assert.True(next.OwnerTables.Ready); Assert.Equal(2048, next.OwnerHighlight.Length);
    }
    /// <summary>실제 짧은 hit도 접두만 쓰고 재생성/파일 변경을 하지 않는다.</summary>
    [Fact]
    public void ActualShortColorCacheKeepsUnreadZeroRows()
    {
        string root = Work(), path = Path.Combine(root, "!color.dat"); byte[] stored = Enumerable.Range(0, 513).Select(index => unchecked((byte)(index * 7 + 19))).ToArray(); File.WriteAllBytes(path, stored); var input = new ClientColorSortInput(); input.CaptureWithPrimary(new uint[256], new byte[0x3800]); Cache(input, new(root, []), path, 0x123); Assert.Equal(stored, File.ReadAllBytes(path)); Assert.True(input.MapsReady); int[] order = [2, 7, 6, 4, 3, 5, 1, 0, 8]; byte[] actual = order.SelectMany(index => input.Maps[index]).ToArray(); Assert.Equal(stored, actual[..513]); Assert.All(actual[513..], value => Assert.Equal(0, value));
    }
    /// <summary>실제 writer 열기 실패도 선행 소유자/맵 준비를 남기고 두 FILE을 scope에서 정리한다.</summary>
    [Fact]
    public void ActualCacheWriteOpenFailureKeepsBuiltMapsAndClosesBothOwners()
    {
        string root = Work(), path = Path.Combine(root, "absent", "!color.dat"); var input = new ClientColorSortInput(); input.CaptureWithPrimary(new uint[256], new byte[0x3800]); var devices = new List<WindowsBaseFileDevice>(); Assert.Throws<ColorCacheUnableToOpenException>(() => input.InitializeCache(new(root, []), path, 7, () => input.OwnerTables.Initialize(input.Primary, 188, 82, new UIntBox()), () => { var file = new WindowsBaseFileDevice(); devices.Add(file); return file; })); Assert.True(input.OwnerTables.Ready); Assert.True(input.MapsReady); Assert.Equal(2, devices.Count); Assert.All(devices, file => { Assert.Equal((nuint)0, file.State.File); Assert.Equal((nuint)0, file.State.Key); }); Assert.False(File.Exists(path));
    }
}
