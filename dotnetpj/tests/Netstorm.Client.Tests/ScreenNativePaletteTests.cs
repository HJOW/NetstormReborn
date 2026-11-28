using System.Buffers.Binary;
using System.Reflection;
using System.Runtime.InteropServices;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// Screen의 실제 같은 저장 물리와 실패 접두를 검사한다. 명시 장치와 Windows CRT 검사를 구별한다.
namespace Netstorm.Client.Tests;

/// <summary>파일 부모가 현재/논리/저장/loaded에 남기는 효과와 기존 값 decoder의 공유 후단을 검사한다.</summary>
public sealed class ScreenNativePaletteTests
{
    /// <summary>새 public 진단 없이 현재 Screen의 실제 저장1024BYTE를 읽기/명시 초기 입력에 빌린다.</summary>
    private static byte[] Saved(Screen screen) => (byte[])(typeof(Screen).GetField("_saved", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(screen)!);

    /// <summary>명시 단위 검사만 사용하는 FILE이다. 실제 OS 파일 성공으로 합산하지 않는다.</summary>
    private sealed class FileDevice : IRawBaseFileDevice
    {
        /// <summary>같은 여섯 필드와 실제 callback 묶음이다.</summary>
        public RawBaseFileState State { get; } = new();
        /// <summary>명시 부분 읽기/현재 위치와 close를 공급한다.</summary>
        public RawBaseFileIOHooks IO { get; }
        /// <summary>직접 목적지 identity를 확인할 현재 Screen 저장 배열이다.</summary>
        public byte[]? ExpectedSaved { get; set; }
        /// <summary>이 번호의 읽기에서 명시 접두를 쓴 뒤 실패한다.0이면 정상이다.</summary>
        public int FailRead { get; set; }
        /// <summary>실패하는 읽기가 실제 목적지에 먼저 작성할 BYTE 수다.</summary>
        public int PrefixBeforeFailure { get; set; }
        /// <summary>완료 후단이 실행된 상태를 close 위치에서 관측할 선택 장치다.</summary>
        public Action? Closing { get; set; }
        /// <summary>실제 도달한 read/close/scope 반납 횟수다.</summary>
        public int Reads { get; private set; }
        /// <summary>현재 File을 닫은 횟수다.</summary>
        public int Closes { get; private set; }
        /// <summary>호스트 scope가 이 장치의 반납을 요청한 횟수다.</summary>
        public int Disposals { get; private set; }

        /// <summary>명시 파일 바이트/위치만 보유하며 현재 Screen의 saved를 직접 목적지로 받는다.</summary>
        public FileDevice(byte[] input)
        {
            int position = 0;
            RawBaseFileIO.ConstructBaseFile(State);
            IO = new()
            {
                Open = (_, _) => 1,
                Tell = _ => position,
                Seek = (_, offset, origin) => position = origin == 2 ? input.Length + offset : origin == 1 ? position + offset : offset,
                Read = (_, destination, size, count) =>
                {
                    if (ExpectedSaved is not null)
                    {
                        Assert.True(MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)destination, out ArraySegment<byte> array));
                        Assert.Same(ExpectedSaved, array.Array);
                    }
                    int requested = checked((int)(size * count)), actual = Math.Min(requested, Math.Max(0, input.Length - position));
                    Reads++; bool fail = Reads == FailRead;
                    if (fail) actual = Math.Min(actual, PrefixBeforeFailure);
                    if (actual != 0) input.AsMemory(position, actual).CopyTo(destination);
                    position += actual;
                    if (fail) throw new InvalidOperationException("read-after-prefix");
                },
                Close = _ => { Closing?.Invoke(); Closes++; },
            };
        }

        /// <summary>실제 adapter와 같은 scope 반납이며 이미 닫힌 File은 다시 close하지 않는다.</summary>
        public void Dispose() { Disposals++; RawBaseFileIO.DestructBaseFile(State, IO); }
    }

    /// <summary>clear 후단의 wait 위치에서 실패하며 앞선 픽셀/loaded 쓰기의 보존을 확인한다.</summary>
    private sealed class FailingOutput : IScreenOutput
    {
        /// <summary>wait가 실패하므로 정상 실행에서는 present에 도달하지 않는다.</summary>
        public void Present(Screen screen, ScreenRect rect) => Assert.Fail("wait 실패 뒤 present에 도달했습니다");
        /// <summary>실제 선택된 후단 위치를 명시 오류로 중단한다.</summary>
        public void WaitVerticalBlank(Screen screen) => throw new InvalidOperationException("wait");
    }

    /// <summary>같은 Client DWORD를 wait 위치에서 바꾸며 반환/예외 뒤 되쓰기가 없는지 관측한다.</summary>
    private sealed class MutatingOutput(UIntBox loaded, bool fail) : IScreenOutput
    {
        /// <summary>이 검사의 표시 장치는 실제 윈도우 출력을 선택하지 않는다.</summary>
        public void Present(Screen screen, ScreenRect rect) { }
        /// <summary>실제 clear 후단 위치의 변경값을 유지한다.</summary>
        public void WaitVerticalBlank(Screen screen)
        {
            Assert.Equal(1u, loaded.Value); loaded.Value = 0xfedcba98u;
            if (fail) throw new InvalidOperationException("mutating-wait");
        }
    }

    /// <summary>현재 파일 읽기 실패는 preloaded DWORD를 건드리지 않고 saved 부분 쓰기/파일 정리만 남긴다.</summary>
    [Fact]
    public void NativeReadFailureKeepsExistingClientLoadedDword()
    {
        var palette = new ClientPaletteGlobals(); palette.Loaded.Value = 0x80000001;
        var screen = new Screen(4, 2, palette.Loaded); var raw = new ColorSortStartupGlobals(palette.Loaded);
        var file = new FileDevice(new byte[1024]) { ExpectedSaved = Saved(screen), FailRead = 1, PrefixBeforeFailure = 3 };
        Assert.Throws<InvalidOperationException>(() => screen.LoadNativePaletteFile(EmptyFiles(), "palette.col", 0, () => file));
        Assert.Equal(0x80000001u, raw.PaletteLoaded); Assert.Equal(0x80000001u, screen.PaletteLoaded); Assert.Equal(1, file.Closes);
    }

    /// <summary>실제 Screen clear callback이 공유값을 바꾸면 정상 복귀/예외 모두 그 DWORD를 보존한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void NativeClearPreservesClientMutationAfterWait(bool fail)
    {
        var palette = new ClientPaletteGlobals(); var screen = new Screen(4, 2, palette.Loaded);
        screen.SetMode(ScreenMode.BackBufferDib); screen.Output = new MutatingOutput(palette.Loaded, fail);
        var raw = new ColorSortStartupGlobals(palette.Loaded); var file = new FileDevice(new byte[1024]) { ExpectedSaved = Saved(screen) };
        if (fail) Assert.Equal("mutating-wait", Assert.Throws<InvalidOperationException>(() => screen.LoadNativePaletteFile(EmptyFiles(), "palette.col", 1, () => file)).Message);
        else screen.LoadNativePaletteFile(EmptyFiles(), "palette.col", 1, () => file);
        Assert.Equal(0xfedcba98u, raw.PaletteLoaded); Assert.Equal(0xfedcba98u, screen.PaletteLoaded); Assert.Equal(1, file.Closes);
    }

    /// <summary>현재 Screen과 명시 파일 장치가 공유할 빈 등록 환경이다.</summary>
    private static GameFileSystem EmptyFiles() => new(Path.GetTempPath(), []);

    /// <summary>BGRX 한 번의 직접 읽기가 예외 전에 작성한 접두는 남고 적용 전 화면/loaded는 유지된다.</summary>
    [Fact]
    public void BulkReadFailureKeepsDirectSavedPrefixWithoutApplyingScreen()
    {
        var screen = new Screen(4, 2); screen.SetMode(ScreenMode.BackBufferDib);
        byte[] saved = Saved(screen); Array.Fill(saved, (byte)0xa5);
        ScreenColor[] before = screen.Palette.ToArray(); uint[] logical = screen.LogicalPalette.ToArray(); var colors = screen.Colors();
        byte[] input = Enumerable.Range(0, 1024).Select(value => unchecked((byte)value)).ToArray();
        var file = new FileDevice(input) { ExpectedSaved = saved, FailRead = 1, PrefixBeforeFailure = 5 };
        var failure = Assert.Throws<InvalidOperationException>(() => screen.LoadNativePaletteFile(EmptyFiles(), "palette.col", 0, () => file));
        Assert.Equal("read-after-prefix", failure.Message); Assert.Equal(input[..5], saved[..5]);
        Assert.All(saved[5..], value => Assert.Equal((byte)0xa5, value));
        Assert.Equal(before, screen.Palette.ToArray()); Assert.Equal(logical, screen.LogicalPalette.ToArray());
        Assert.Same(colors, screen.Colors()); Assert.Equal(0u, screen.PaletteLoaded);
        Assert.Equal(1, file.Reads); Assert.Equal(1, file.Closes); Assert.Equal(1, file.Disposals); Assert.Equal((nuint)0, file.State.File);
    }

    /// <summary>RGB 세 번째1BYTE가 실패해도 각 채널의 선행 직접 쓰기와 X 예약BYTE를 보존한다.</summary>
    [Fact]
    public void RgbReadFailureDoesNotBatchOrRollbackOneByteWrites()
    {
        var screen = new Screen(4, 2); byte[] saved = Saved(screen); Array.Fill(saved, (byte)0xa5);
        byte[] input = new byte[776]; input[8] = 11; input[9] = 22; input[10] = 33;
        var file = new FileDevice(input) { ExpectedSaved = saved, FailRead = 3, PrefixBeforeFailure = 1 };
        Assert.Throws<InvalidOperationException>(() => screen.LoadNativePaletteFile(EmptyFiles(), "palette.col", 0, () => file));
        Assert.Equal(new byte[] { 33, 22, 11, 0xa5 }, saved[..4]);
        Assert.All(saved[4..], value => Assert.Equal((byte)0xa5, value));
        Assert.Equal(3, file.Reads); Assert.Equal(1, file.Closes); Assert.Equal(0u, screen.PaletteLoaded);
    }

    /// <summary>clear의 loaded1/픽셀 지우기 뒤 wait 실패는 현재/논리 적용 전에 File을 반납하며 앞선 효과를 유지한다.</summary>
    [Fact]
    public void ClearFailureKeepsLoadedAndPixelsButNotYetAppliedPalette()
    {
        var screen = new Screen(4, 2); screen.SetMode(ScreenMode.BackBufferDib); screen.SetClip(0, 0, 4, 2);
        screen.Clear(new(0, 0, 4, 2), 9); screen.Output = new FailingOutput();
        ScreenColor[] before = screen.Palette.ToArray(); uint[] logical = screen.LogicalPalette.ToArray();
        byte[] input = Enumerable.Repeat((byte)71, 1024).ToArray(); var file = new FileDevice(input) { ExpectedSaved = Saved(screen) };
        var failure = Assert.Throws<InvalidOperationException>(() => screen.LoadNativePaletteFile(EmptyFiles(), "palette.col", 1, () => file));
        Assert.Equal("wait", failure.Message); Assert.Equal(input, Saved(screen)); Assert.Equal(1u, screen.PaletteLoaded);
        Assert.Equal(before, screen.Palette.ToArray()); Assert.Equal(logical, screen.LogicalPalette.ToArray());
        Assert.All(screen.Pixels.ToArray(), value => Assert.Equal((byte)0, value));
        Assert.Throws<InvalidOperationException>(screen.Unlock); Assert.Equal(1, file.Closes);
    }

    /// <summary>색표/현재 팔레트까지 쓴 뒤 명암 getter가 실패하면 그 효과와 File 반납을 보존한다.</summary>
    [Fact]
    public void ShadeMapFailureKeepsEarlierAppliedPaletteAndNamedColors()
    {
        var screen = new Screen(4, 2); var file = new FileDevice(Enumerable.Repeat((byte)29, 1024).ToArray()) { ExpectedSaved = Saved(screen) };
        bool fail = false; var previous = screen.Colors();
        screen.BindColorMaps(() =>
        {
            if (fail) { Assert.NotEqual((nuint)0, file.State.File); throw new InvalidOperationException("maps"); }
            return new GumpShadeMaps();
        });
        fail = true;
        var failure = Assert.Throws<InvalidOperationException>(() => screen.LoadNativePaletteFile(EmptyFiles(), "palette.col", 0, () => file));
        Assert.Equal("maps", failure.Message); Assert.Equal(1u, screen.PaletteLoaded);
        Assert.Equal(new ScreenColor(29, 29, 29, 29), screen.Palette[17]); Assert.NotSame(previous, screen.Colors());
        Assert.Equal(0u, screen.LogicalPalette[0]); Assert.Equal(0xffffffu, screen.LogicalPalette[255]); Assert.Equal(1, file.Closes);
    }

    /// <summary>정상 두 형식의 같은 저장 뷰·예약BYTE·기존 화면 모드 전환을 보존한다.</summary>
    [Theory]
    [InlineData(776)]
    [InlineData(1024)]
    public void BothFormatsApplyBeforeCloseAndKeepOneSavedBackingArray(int length)
    {
        var screen = new Screen(4, 2); byte[] saved = Saved(screen); Array.Fill(saved, (byte)0xa5);
        Assert.Equal(4, MemoryMarshal.AsBytes(new ScreenColor[1].AsSpan()).Length);
        byte[] input = Enumerable.Repeat((byte)37, length).ToArray();
        var file = new FileDevice(input) { ExpectedSaved = saved };
        file.Closing = () => { Assert.Equal(1u, screen.PaletteLoaded); Assert.Equal((byte)37, screen.Palette[17].Red); Assert.Equal(4, screen.Colors().Weather.Length); };
        screen.LoadNativePaletteFile(EmptyFiles(), "palette.col", 0, () => file);
        Assert.Equal(length == 776 ? 768 : 1, file.Reads); Assert.Equal(1, file.Closes);
        Assert.Equal(new ScreenColor(37, 37, 37, length == 776 ? (byte)0xa5 : (byte)37), screen.Palette[17]);
        Assert.Equal(default, screen.Palette[0]); Assert.Equal(new ScreenColor(255, 255, 255, 0), screen.Palette[255]);
        screen.SetMode(ScreenMode.BackBufferDib); Assert.Same(saved, Saved(screen));
        Assert.Equal((byte)37, screen.Palette[17].Red);
    }

    /// <summary>이 검사만 소유하는 고유 임시 경로이며 보호 원본에는 작성하지 않는다.</summary>
    private sealed class Files : IDisposable
    {
        /// <summary>실제 CRT Unicode 경로 검사용 자기 폴더다.</summary>
        public string Root { get; } = Directory.CreateTempSubdirectory("Netstorm-Palette-한글-").FullName;
        /// <summary>이 scope가 만든 자기 폴더만 종료 때 반납한다.</summary>
        public void Dispose() => Directory.Delete(Root, true);
    }

    /// <summary>독립 입력의 TAFF 한 엔트리를 만들고 등록 snapshot과 backing 파일의 차이를 검사한다.</summary>
    private static byte[] Archive(byte value)
    {
        byte[] raw = new byte[0x100 + 1024], name = "d/live.col\0"u8.ToArray(), payload = Enumerable.Repeat(value, 1024).ToArray();
        "TAFF v0.2\x1a"u8.CopyTo(raw); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x14), 1);
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x18), 2); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x1c), 0x2c);
        BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x20), 0x40); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x24), 8 + name.Length);
        BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x28), 0x100); BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x40), 0);
        BinaryPrimitives.WriteInt32LittleEndian(raw.AsSpan(0x44), 1024); name.CopyTo(raw, 0x48);
        XorCipher.Apply(payload); payload.CopyTo(raw, 0x100); return raw;
    }

    /// <summary>같은 현재 VFS metadata가 바뀐 실제 archive FILE을 읽고 이전 snapshot은 유지한다. 창/게임은 생성하지 않는다.</summary>
    [Fact]
    public void ActualWindowsFileUsesCurrentArchiveWhileValueSnapshotStaysOld()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var work = new Files(); string path = Path.Combine(work.Root, "현재😀.tarc");
        File.WriteAllBytes(path, Archive(17)); var files = GameFileSystem.Open(work.Root); byte[] snapshot = files.ReadAllBytes("d/live.col");
        File.WriteAllBytes(path, Archive(29)); byte[] saved = new byte[1024]; WindowsBaseFileDevice? device = null;
        PaletteFileRead.ReadNative(files, "d/live.col", saved, () =>
        {
            Assert.NotNull(device); Assert.NotEqual((nuint)0, device.State.File); Assert.NotEqual((nuint)0, device.State.Key);
            Assert.All(saved, value => Assert.Equal((byte)29, value));
        }, () => device = new WindowsBaseFileDevice());
        Assert.NotNull(device); Assert.Equal((nuint)0, device.State.File); Assert.Equal((nuint)0, device.State.Key); Assert.Equal(13u, device.State.KeyLength);
        Assert.All(snapshot, value => Assert.Equal((byte)17, value)); Assert.Equal(snapshot, files.ReadAllBytes("d/live.col"));
        using var exclusive = File.Open(path, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
        Assert.Equal(0x500, exclusive.Length);
    }

    /// <summary>실제 loose RGB는 헤더 내용 대신 길이로 선택하며 실패 뒤 같은 VFS 재읽기도 가능하다.</summary>
    [Fact]
    public void ActualWindowsLooseRgbAndRequiredFailuresKeepSavedBytes()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var work = new Files(); var files = new GameFileSystem(work.Root, []); byte[] saved = Enumerable.Repeat((byte)0xa5, 1024).ToArray();
        string path = Path.Combine(work.Root, "색표.col"); bool applied = false;
        Assert.Throws<ColorCacheUnableToOpenException>(() => PaletteFileRead.ReadNative(files, path, saved, () => applied = true, () => new WindowsBaseFileDevice()));
        File.WriteAllBytes(path, new byte[775]);
        Assert.Throws<InvalidOperationException>(() => PaletteFileRead.ReadNative(files, path, saved, () => applied = true, () => new WindowsBaseFileDevice()));
        Assert.False(applied); Assert.All(saved, value => Assert.Equal((byte)0xa5, value));
        byte[] rgb = new byte[776];
        // 독립 실제 파일 입력의 RGB를 각 BYTE로 작성하며 예상 저장 배열은 직접 읽지 않는다.
        for (int index = 0; index < 256; index++) { rgb[8 + index * 3] = unchecked((byte)index); rgb[9 + index * 3] = unchecked((byte)(index + 1)); rgb[10 + index * 3] = unchecked((byte)(index + 2)); }
        File.WriteAllBytes(path, rgb); PaletteFileRead.ReadNative(files, path, saved, () => applied = true, () => new WindowsBaseFileDevice());
        Assert.True(applied);
        // RGB의 예약BYTE가 현재 저장 배열의 입력값 그대로 남는지 전체256칸에서 확인한다.
        for (int index = 0; index < 256; index++)
        {
            Assert.Equal(unchecked((byte)(index + 2)), saved[index * 4]); Assert.Equal(unchecked((byte)(index + 1)), saved[index * 4 + 1]);
            Assert.Equal(unchecked((byte)index), saved[index * 4 + 2]); Assert.Equal((byte)0xa5, saved[index * 4 + 3]);
        }
    }
}
