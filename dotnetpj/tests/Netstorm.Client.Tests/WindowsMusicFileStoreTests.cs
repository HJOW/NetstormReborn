using System.Buffers.Binary;
using System.Collections.Concurrent;
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;

// 실제 Winmm 음악 파일의 부분 헤더·스트리밍 위치·토큰 수명을 창과 음향 장치 없이 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>cpppj MusicFileStore와 같은 실제 RIFF IO를 보호 사본과 독립 입력으로 검증한다.</summary>
[SupportedOSPlatform("windows")]
public sealed class WindowsMusicFileStoreTests
{
    /// <summary>이 검사만 소유하는 새 임시 폴더와 파일을 관리한다.</summary>
    private sealed class Files : IDisposable
    {
        /// <summary>검사별 고유 절대 루트다.</summary>
        public string Root { get; } = Directory.CreateTempSubdirectory("NetstormDnMusic-").FullName;
        /// <summary>자기 폴더 아래에 독립 입력만 기록한다.</summary>
        public string Write(string name, byte[] bytes)
        {
            string path = Path.Combine(Root, name);
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.WriteAllBytes(path, bytes);
            return path;
        }
        /// <summary>검사가 직접 만든 폴더만 재귀 삭제한다.</summary>
        public void Dispose() => Directory.Delete(Root, true);
    }

    /// <summary>원본 보호 사본 음악9개의 헤더와 처음·중간·끝 표본을 실제 디스크의 독립 읽기와 대조한다.</summary>
    [Fact]
    public void AllProtectedMusicAssetsMatchHeaderAndRandomAccessSamples()
    {
        if (!OperatingSystem.IsWindows()) return;
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        string[] paths = Directory.GetFiles(Path.Combine(data, "music"), "*.mus").Order().ToArray();
        Assert.Equal(9, paths.Length);
        using var store = new WindowsMusicFileStore();
        uint previous = 0;
        // 각 실제 자산은 전체 적재 없이 헤더와 세 위치의256바이트만 읽는다.
        foreach (string path in paths)
        {
            using var source = File.OpenRead(path);
            WaveFile expected = WaveFile.ReadHeader(source);
            var header = new MusicFileHeader();
            Assert.True(store.Open(path, header), path);
            Assert.True(header.File > previous);
            previous = header.File;
            Assert.Equal((uint)expected.DataOffset, header.DataOffset);
            Assert.Equal((uint)expected.DataLength, header.Length);
            Assert.Equal((ushort)1, BinaryPrimitives.ReadUInt16LittleEndian(header.Format));
            Assert.Equal((ushort)expected.Channels, BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(2)));
            Assert.Equal((uint)expected.SampleRate, BinaryPrimitives.ReadUInt32LittleEndian(header.Format.AsSpan(4)));
            Assert.Equal((ushort)expected.BitsPerSample, BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(14)));
            Assert.Equal((int)header.DataOffset, store.Position(header.File));
            // 같은 파일의 현재 위치 이동과 표본을 서로 독립인 FileStream으로 검증한다.
            foreach (uint relative in new uint[] { 0, header.Length / 2, header.Length - 256 })
            {
                uint absolute = header.DataOffset + relative;
                Assert.Equal((int)absolute, store.Seek(header.File, absolute));
                byte[] actual = new byte[256];
                Assert.Equal(actual.Length, store.Read(header.File, actual));
                source.Position = absolute;
                byte[] samples = new byte[256];
                source.ReadExactly(samples);
                Assert.Equal(samples, actual);
                Assert.Equal((int)absolute + actual.Length, store.Position(header.File));
            }
            store.Close(header.File);
            Assert.Equal(-1, store.Position(header.File));
        }
    }

    /// <summary>실패 단계까지의 헤더 출력만 남고 실패 토큰이 실제 열린 파일을 소유하지 않는지 검증한다.</summary>
    [Fact]
    public void FailedOpenPreservesPartialOutputAndClosesActualHandle()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var files = new Files();
        using var store = new WindowsMusicFileStore();
        var header = HeaderWithCanaries();
        Assert.False(store.Open(Path.Combine(files.Root, "missing.mus"), header));
        Assert.Equal(0u, header.File);
        AssertCanaryScalars(header);
        Assert.All(header.Format, value => Assert.Equal((byte)0xa5, value));
        string invalid = files.Write("invalid.mus", new byte[48]);
        Assert.False(store.Open(invalid, header));
        uint first = header.File;
        Assert.NotEqual(0u, first);
        Assert.Equal(-1, store.Position(first));
        AssertCanaryScalars(header);
        Assert.All(header.Format, value => Assert.Equal((byte)0xa5, value));
        AssertClosedOnDisk(invalid);

        byte[] complete = Wave();
        string truncated = files.Write("short-format.mus", complete[..28]);
        Assert.False(store.Open(truncated, header));
        Assert.True(header.File > first);
        Assert.Equal(-1, store.Position(header.File));
        AssertCanaryScalars(header);
        // Winmm은 실제 잔여8BYTE로 fmt 크기를 줄이므로 원문 최소16 관문에서 읽기 전에 거부한다.
        Assert.All(header.Format, value => Assert.Equal((byte)0xa5, value));
        AssertClosedOnDisk(truncated);

        string clamped = files.Write("clamped-format.mus", Wave(formatLength: 18)[..36]);
        Assert.False(store.Open(clamped, header));
        Assert.True(header.File > first);
        Assert.Equal(-1, store.Position(header.File));
        AssertCanaryScalars(header);
        // 선언18/실제16은 같은 Winmm이16으로 줄인 형식만 읽고 보정한 뒤 data 부재에서 실패한다.
        Assert.Equal(complete[20..36], header.Format[..16]);
        Assert.Equal((ushort)0, BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(16)));
        AssertClosedOnDisk(clamped);

        string missingData = files.Write("missing-data.mus", Wave(includeData: false));
        Assert.False(store.Open(missingData, header));
        Assert.True(header.File > first);
        Assert.Equal(-1, store.Position(header.File));
        AssertCanaryScalars(header);
        Assert.Equal((ushort)1, BinaryPrimitives.ReadUInt16LittleEndian(header.Format));
        Assert.Equal((ushort)16, BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(14)));
        Assert.Equal((ushort)0, BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(16)));
        AssertClosedOnDisk(missingData);
        store.Close(first);
        store.Close(header.File);
    }

    /// <summary>짧은 비PCM 보고의 실제 순서와 보고 예외 때 부분 출력·실제 핸들 정리를 확인한다.</summary>
    [Fact]
    public void ReportingExceptionKeepsReadPrefixAndClosesFailureToken()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var files = new Files();
        string path = files.Write("reported.mus", Wave(tag: 2));
        var header = HeaderWithCanaries();
        var expected = new IOException("검사 진단 예외");
        int reports = 0;
        using var store = new WindowsMusicFileStore((text, line) =>
        {
            Assert.Equal("wfFormat.wFormatTag == WAVE_FORMAT_PCM", text);
            Assert.Equal(0xeb, line);
            Assert.NotEqual(0u, header.File);
            reports++;
            throw expected;
        });
        Assert.Same(expected, Assert.Throws<IOException>(() => store.Open(path, header)));
        Assert.Equal(1, reports);
        Assert.Equal(-1, store.Position(header.File));
        AssertCanaryScalars(header);
        Assert.Equal((ushort)2, BinaryPrimitives.ReadUInt16LittleEndian(header.Format));
        Assert.Equal((ushort)0xa5a5, BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(16)));
        AssertClosedOnDisk(path);
    }

    /// <summary>진단이 반환하면 지역 형식을 보정하고 추가 코덱·0나눗셈·빈data는 실제 실패로 닫는다.</summary>
    [Fact]
    public void ReturningReportAndUnsafeFormatsFollowNativeOpenBoundary()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var files = new Files();
        int reports = 0;
        var header = HeaderWithCanaries();
        using var store = new WindowsMusicFileStore((_, _) => { reports++; header.Format.AsSpan().Fill(0x7e); });
        Assert.True(store.Open(files.Write("short-nonpcm.mus", Wave(tag: 2)), header));
        Assert.Equal(1, reports);
        Assert.Equal((ushort)2, BinaryPrimitives.ReadUInt16LittleEndian(header.Format));
        Assert.Equal((ushort)16, BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(14)));
        Assert.Equal((ushort)0, BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(16)));
        store.Close(header.File);
        byte[] codec = Wave(formatLength: 18, tag: 2);
        BinaryPrimitives.WriteUInt16LittleEndian(codec.AsSpan(36), 22);
        byte[] zeroChannels = Wave();
        BinaryPrimitives.WriteUInt16LittleEndian(zeroChannels.AsSpan(22), 0);
        // 거부한 형식은 실패 토큰만 남기고 실제 파일 잠금을 모두 반납한다.
        foreach ((string name, byte[] bytes) in new[] { ("codec.mus", codec), ("zero-channel.mus", zeroChannels), ("empty-data.mus", Wave(sampleLength: 0)) })
        {
            string path = files.Write(name, bytes);
            var failed = HeaderWithCanaries();
            Assert.False(store.Open(path, failed));
            Assert.NotEqual(0u, failed.File);
            Assert.Equal(-1, store.Position(failed.File));
            AssertCanaryScalars(failed);
            AssertClosedOnDisk(path);
        }
    }

    /// <summary>만료 토큰·0 재닫기·signed seek 범위·실제 짧은 읽기의 쓰지 않은 꼬리를 확인한다.</summary>
    [Fact]
    public void StaleCloseAndShortReadCannotDamageNewFile()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var files = new Files();
        string path = files.Write("stream.mus", Wave());
        using var store = new WindowsMusicFileStore();
        var old = new MusicFileHeader();
        Assert.True(store.Open(path, old));
        store.Close(old.File);
        var current = new MusicFileHeader();
        Assert.True(store.Open(path, current));
        Assert.True(current.File > old.File);
        store.Close(old.File);
        store.Close(0);
        Assert.Equal((int)current.DataOffset, store.Position(current.File));
        Assert.Equal(-1, store.Seek(current.File, uint.MaxValue));
        Assert.Equal((int)current.DataOffset, store.Position(current.File));
        Assert.Equal(-1, store.Read(old.File, Span<byte>.Empty));
        Assert.Equal(0, store.Read(current.File, Span<byte>.Empty));
        Assert.Equal((int)(current.DataOffset + current.Length - 2), store.Seek(current.File, current.DataOffset + current.Length - 2));
        byte[] tail = [0xa5, 0xa5, 0xa5, 0xa5, 0xa5];
        Assert.Equal(2, store.Read(current.File, tail));
        Assert.Equal(new byte[] { Sample(62), Sample(63), 0xa5, 0xa5, 0xa5 }, tail);
        Assert.Equal(0, store.Read(current.File, tail));
        store.Dispose();
        store.Dispose();
        Assert.Equal(-1, store.Position(current.File));
        Assert.Equal(-1, store.Read(current.File, tail));
        AssertClosedOnDisk(path);
        Assert.Throws<ObjectDisposedException>(() => store.Open(path, new MusicFileHeader()));
    }

    /// <summary>파일 hook은 실제 검색·유니코드 경로를 사용하고 기존 장치 함수의 사본을 보존한다.</summary>
    [Fact]
    public void HooksKeepDeviceCallbacksAndUseOnePrimaryResolverCall()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var files = new Files();
        string path = files.Write("backup/음악.mus", Wave());
        using var store = new WindowsMusicFileStore();
        var visited = new List<(string Pattern, bool Alternate)>();
        var disk = WindowsSoundFiles.MakeDiskResolver(files.Root);
        string? fallback = null;
        MusicOpenHooks open = store.OpenHooks((pattern, alternate) => { visited.Add((pattern, alternate)); return disk(pattern, alternate); }, name => fallback = name);
        string? found = open.Find!("backup\\음악.mus");
        Assert.Equal(Path.GetFullPath(path), found);
        Assert.Equal(new[] { ("backup\\음악.mus", false) }, visited);
        Assert.Null(fallback);
        open.Fallback!("demo.mus");
        Assert.Equal("demo.mus", fallback);
        var header = new MusicFileHeader();
        Assert.True(open.Open!(found!, header));
        var original = new MusicChannelHooks { SetVolume = (_, _) => 37, Release = _ => 39, Seek = (_, _) => 41 };
        MusicChannelHooks bound = store.FileHooks(original);
        Assert.NotSame(original, bound);
        Assert.Same(original.SetVolume, bound.SetVolume);
        Assert.Same(original.Release, bound.Release);
        Assert.Equal(41, original.Seek!(header.File, 0));
        Assert.Equal((int)header.DataOffset, bound.Seek!(header.File, header.DataOffset));
        byte[] sample = new byte[4];
        Assert.Equal(4, bound.Read!(header.File, sample));
        Assert.Equal(new byte[] { Sample(0), Sample(1), Sample(2), Sample(3) }, sample);
        bound.Close!(header.File);
        Assert.Equal(-1, store.Position(header.File));
    }

    /// <summary>실제 Winmm 핸들을 여러 호출자가 열고 읽고 닫아도 토큰을 재사용하거나 섞지 않는다.</summary>
    [Fact]
    public void ConcurrentFileOwnersHaveDistinctTokensAndRealSamples()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var files = new Files();
        string path = files.Write("parallel.mus", Wave());
        using var store = new WindowsMusicFileStore();
        var tokens = new ConcurrentDictionary<uint, byte>();
        Parallel.For(0, 32, _ =>
        {
            var header = new MusicFileHeader();
            Assert.True(store.Open(path, header));
            Assert.True(tokens.TryAdd(header.File, 0));
            byte[] samples = new byte[4];
            Assert.Equal(4, store.Read(header.File, samples));
            Assert.Equal(new byte[] { Sample(0), Sample(1), Sample(2), Sample(3) }, samples);
            store.Close(header.File);
            Assert.Equal(-1, store.Position(header.File));
        });
        Assert.Equal(32, tokens.Count);
        AssertClosedOnDisk(path);
    }

    /// <summary>정상 data에 도달하기 전에는 바뀌면 안 되는 scalar와 형식 꼬리를 준비한다.</summary>
    private static MusicFileHeader HeaderWithCanaries()
    {
        var header = new MusicFileHeader { File = 999, Length = 47, DataOffset = 49 };
        header.Format.AsSpan().Fill(0xa5);
        return header;
    }

    /// <summary>헤더 실패가 기존 길이·시작 위치를 보존했는지 확인한다.</summary>
    private static void AssertCanaryScalars(MusicFileHeader header)
    {
        Assert.Equal(47u, header.Length);
        Assert.Equal(49u, header.DataOffset);
    }

    /// <summary>표만 지운 가짜 성공이 아니라 실제 Winmm 핸들도 닫혔는지 배타적 파일 열기로 확인한다.</summary>
    private static void AssertClosedOnDisk(string path)
    {
        using var exclusive = new FileStream(path, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
        Assert.True(exclusive.CanRead);
    }

    /// <summary>위치별로 달라서 섞인 파일 위치를 식별할 수 있는 독립 표본이다.</summary>
    private static byte Sample(int index) => unchecked((byte)(11 + index * 7));

    /// <summary>원본 음악과 같은 PCM 헤더를 만들되 실패 경계를 위한 fmt/data 변형도 허용한다.</summary>
    private static byte[] Wave(int formatLength = 16, ushort tag = 1, int sampleLength = 64, bool includeData = true)
    {
        byte[] bytes = new byte[20 + formatLength + (includeData ? 8 + sampleLength : 0)];
        "RIFF"u8.CopyTo(bytes);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(4), (uint)bytes.Length - 8);
        "WAVEfmt "u8.CopyTo(bytes.AsSpan(8));
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(16), (uint)formatLength);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(20), tag);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(22), 2);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(24), 22050);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(28), 88200);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(32), 4);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(34), 16);
        if (includeData)
        {
            int chunk = 20 + formatLength;
            "data"u8.CopyTo(bytes.AsSpan(chunk));
            BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(chunk + 4), (uint)sampleLength);
            // 요청한 각 위치에 독립 표본을 넣어 실제 seek/read 결과를 대조한다.
            for (int index = 0; index < sampleLength; index++) bytes[chunk + 8 + index] = Sample(index);
        }
        return bytes;
    }
}
