using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;

// 실제 원문 패킷 전체와 Native 현재 재조회/부분 효과를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>RawPathData의 독립 작성 관측과 Source 수신 계약을 구별해 검사한다.</summary>
public sealed partial class RawPathDataTests
{
    /// <summary>검사 입력의 WORD를 원문 위치에 쓴다.</summary>
    private static void Word(byte[] bytes, int offset, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(offset, 2), value);
    /// <summary>검사 입력의 DWORD를 원문 위치에 쓴다.</summary>
    private static void Dword(byte[] bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset, 4), value);
    /// <summary>현재 패킷 WORD를 읽는다.</summary>
    private static ushort ReadWord(byte[] bytes, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(bytes.AsSpan(offset, 2));
    /// <summary>현재 패킷 DWORD를 읽는다.</summary>
    private static uint ReadDword(byte[] bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset, 4));
    /// <summary>현재 원본 배치의 typed/Native 작성기를 같은 입력으로 준비한다.</summary>
    private static RawPathRuntime WriterInput(OriginalEdition edition, uint profile, ulong firstTime)
    {
        (RawPathPoint[] points, int[] directions) = Inputs(profile); var times = new double[points.Length]; times[0] = BitConverter.UInt64BitsToDouble(firstTime);
        var runtime = new RawPathRuntime(edition, (profile & 1) != 0); runtime.SetPath(points, directions, times); runtime.SetAttachment((ushort)(32700 + profile), 5);
        runtime.SetCommandFields(0x12340000 + profile * 65537, 0x87654321 + profile, 0xa5 + profile, default); return runtime;
    }
    /// <summary>원문 관찰기가 받은12프로필의 입력만 제공하며 기대 패킷은 계산하지 않는다.</summary>
    private static (RawPathPoint[] Points, int[] Directions) Inputs(uint profile) => profile switch
    {
        0 => ([new(10, 20)], [0]),
        1 => ([new(10, 20), new(10, 20), new(10, 20)], [0, 7, 2]),
        2 => ([new(10, 20), new(11, 20)], [0, 0]),
        3 => ([new(10, 20), new(41, 20)], [0, 0]),
        4 => ([new(10, 20), new(42, 20)], [0, 0]),
        5 => ([new(10, 20), new(73, 20)], [0, 0]),
        6 => ([new(10, 20), new(100, 110)], [0, 1]),
        7 => ([new(10, 20), new(10, 40), new(80, 40), new(20, 100)], [0, 2, 4, 6]),
        8 => ([new(10.9f, 20.1f), new(10.01f, 20.9f), new(11.9f, 20.1f)], [0, 1, 8]),
        9 => ([new(-1.9f, -2.1f), new(-32.9f, 20.9f), new(255.9f, 260.1f)], [0, 7, 15]),
        10 => ([new(255.9f, 256.9f), new(1.9f, -1.9f)], [0, -1]),
        11 => ([new(10, 20), new(10, 80), new(30, 80)], [0, 3, 0]),
        _ => throw new ArgumentOutOfRangeException(nameof(profile)),
    };
    /// <summary>두 PE의768개 typed 버퍼와 PATCH384개 Native 버퍼를 원문 전체2048BYTE로 비교한다.</summary>
    [Fact]
    public void NativeWriterFixtureMatchesWholeBuffersAndBothWriterContracts()
    {
        string[] lines = File.ReadAllLines(Path.Combine(AppContext.BaseDirectory, "X86", "pathdata-x86.tsv")); int typedCount = 0, nativeCount = 0;
        // 기존 원문 기대값을 직접 읽으며 길이 뒤 stale buffer까지 대조한다.
        foreach (string line in lines)
        {
            if (line.Length == 0 || line.StartsWith('#')) continue; string[] row = line.Split('\t'); Assert.Equal(8, row.Length);
            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            uint profile = uint.Parse(row[1], CultureInfo.InvariantCulture), genus = uint.Parse(row[2], CultureInfo.InvariantCulture), variant = uint.Parse(row[3], CultureInfo.InvariantCulture);
            var runtime = WriterInput(edition, profile, ulong.Parse(row[5], CultureInfo.InvariantCulture)); byte[] before = runtime.Bytes.ToArray();
            var header = new RawPathDataHeader { PacketId = (byte)(0x30 + variant), PacketFlags = variant != 0 ? (byte)0x81 : (byte)0xfe, PacketLength = variant != 0 ? 0u : 0x8123u };
            byte[] packet = Enumerable.Repeat(variant != 0 ? (byte)0xa5 : (byte)0x5a, 2048).ToArray(); byte[] expected = Convert.FromHexString(row[7]);
            int length = int.Parse(row[6], CultureInfo.InvariantCulture); var codec = new RawPathData(edition);
            Assert.Equal(length, codec.Write(runtime, (ushort)(32700 + profile), 5, genus, header, packet)); Assert.Equal(expected, packet); Assert.Equal(before, runtime.Bytes.ToArray()); typedCount++;
            if (edition != OriginalEdition.Patch1078) continue; Array.Fill(packet, variant != 0 ? (byte)0xa5 : (byte)0x5a); var trace = new List<string>();
            Assert.Equal(length, codec.WriteCurrent(runtime, (ushort)(32700 + profile), 5, header, packet, new() { ParentGenus = _ => genus, Trace = trace.Add }));
            Assert.Equal(expected, packet); Assert.Equal(before, runtime.Bytes.ToArray()); nativeCount++;
        }
        Assert.Equal(768, typedCount); Assert.Equal(384, nativeCount);
    }
    /// <summary>빈 count 진단6회가 정상 복귀하면 기존 실제 물리를 읽고0 SID도 작성한다.</summary>
    [Fact]
    public void CurrentEmptyCountContinuesAfterDiagnosticsWithoutTypedGuards()
    {
        var runtime = new RawPathRuntime(OriginalEdition.Patch1078, heapFill: 0xa5); var codec = new RawPathData(OriginalEdition.Patch1078); var lines = new List<uint>(); var trace = new List<string>(); byte[] packet = Enumerable.Repeat((byte)0xa5, 48).ToArray();
        Assert.Equal(24, codec.WriteCurrent(runtime, 0, 0, new(), packet, new()
        {
            Assertion = (expression, file, line) => { Assert.Equal("i < nVert", expression); Assert.Equal(@"c:\ns\o\path.h", file); lines.Add(line); },
            ParentGenus = parent => { Assert.Equal(0, parent); return 0; }, Trace = trace.Add,
        }));
        Assert.Equal(new uint[] { 0x1f, 0x1f, 0x24, 0x1f, 0x1f, 0x29 }, lines); Assert.Equal(new[] { "[0] (0,0) , -1515870811\n" }, trace);
        Assert.Equal(4, ReadWord(packet, 2)); Assert.Equal(0, ReadWord(packet, 4)); Assert.Equal(0, ReadWord(packet, 6)); Assert.Equal(0, ReadWord(packet, 13)); Assert.Equal(0xa5, packet[8]); Assert.Equal(0xa5, packet[24]);
        Assert.Throws<ArgumentOutOfRangeException>(() => codec.Write(runtime, 0, 0, 0, new(), packet));
    }
    /// <summary>선행 진단이 바꾼 실제 이미지와 packet+6을 현재 부모 선택 위치에서 읽는다.</summary>
    [Fact]
    public void CurrentDiagnosticCanReplaceFieldsBeforeCurrentParentLookup()
    {
        var runtime = new RawPathRuntime(OriginalEdition.Patch1078); byte[] packet = new byte[48]; int diagnoses = 0;
        int length = new RawPathData(OriginalEdition.Patch1078).WriteCurrent(runtime, 0, 0, new() { PacketFlags = 0xff, PacketLength = 0xffff0000 }, packet, new()
        {
            Assertion = (_, _, _) => { diagnoses++; runtime.SetPath([new(7, 9)], [2]); Word(packet, 6, 11); },
            ParentGenus = parent => { Assert.Equal(11, parent); Assert.Equal(0, ReadWord(packet, 13)); return 0x100000; },
        });
        Assert.Equal(24, length); Assert.Equal(1, diagnoses); Assert.Equal(0x7f, packet[0]); Assert.Equal(0, ReadWord(packet, 2)); Assert.Equal(7, packet[15]); Assert.Equal(9, packet[16]);
    }
    /// <summary>run 로그가 바꾼 현재WORD로 payload 주소/증가/다음 로그/반환을 정한다.</summary>
    [Fact]
    public void CurrentRunUsesWordChangedByTraceAndPreservesStalePayload()
    {
        var runtime = new RawPathRuntime(OriginalEdition.Patch1078); runtime.SetPath([new(10, 20), new(42, 20)], [0, 0]); byte[] packet = Enumerable.Repeat((byte)0xa5, 64).ToArray(); var trace = new List<string>(); bool changed = false;
        int length = new RawPathData(OriginalEdition.Patch1078).WriteCurrent(runtime, 7, 5, new(), packet, new()
        {
            ParentGenus = _ => 0, Trace = text => { trace.Add(text); if (!changed && text.StartsWith("n[", StringComparison.Ordinal)) { Word(packet, 13, 5); changed = true; } },
        });
        Assert.Equal(31, length); Assert.Equal(7, ReadWord(packet, 13)); Assert.Equal(0xa5, packet[24]); Assert.Equal(0xf8, packet[29]); Assert.Equal(8, packet[30]); Assert.Contains("n[6] 0 | 1\n", trace); Assert.Equal(4, ReadWord(packet, 2));
    }
    /// <summary>음수signedWORD의 payload가 현재 길이WORD와 겹쳐도 BYTE 저장 뒤 현재WORD를 증가한다.</summary>
    [Fact]
    public void CurrentNegativeWordCanOverwriteItsOwnLowByteBeforeIncrement()
    {
        var runtime = new RawPathRuntime(OriginalEdition.Patch1078); runtime.SetPath([new(10, 20), new(11, 20)], [0, 0]); byte[] packet = Enumerable.Repeat((byte)0xa5, 64).ToArray();
        int length = new RawPathData(OriginalEdition.Patch1078).WriteCurrent(runtime, 7, 5, new(), packet, new()
        {
            ParentGenus = _ => 0, Trace = text => { if (text.StartsWith("n[", StringComparison.Ordinal)) Word(packet, 13, unchecked((ushort)-11)); },
        });
        Assert.Equal(-223, length); Assert.Equal(0xff09, ReadWord(packet, 13)); Assert.Equal(0xa5, packet[24]); Assert.Equal(4, ReadWord(packet, 2));
    }
    /// <summary>Native signed64 절삭/거리 DWORD 감김을 typed signed32 정책으로 바꾸지 않는다.</summary>
    [Fact]
    public void CurrentMaskedFtolAndSignedDistanceKeepOriginalWrap()
    {
        var runtime = new RawPathRuntime(OriginalEdition.Patch1078); runtime.SetPath([new(float.NaN, float.PositiveInfinity), new(2147483648f, 0)], [0, 0], [double.PositiveInfinity, 0]); byte[] packet = new byte[48]; var trace = new List<string>();
        var codec = new RawPathData(OriginalEdition.Patch1078); Assert.Equal(24, codec.WriteCurrent(runtime, 7, 5, new(), packet, new() { ParentGenus = _ => 0, Trace = trace.Add }));
        Assert.Equal(0, packet[15]); Assert.Equal(0, packet[16]); Assert.Equal(0u, ReadDword(packet, 17)); Assert.Equal(0, ReadWord(packet, 13)); Assert.Contains("[1] (-2147483648,0) , 0\n", trace);
        Assert.Throws<ArgumentOutOfRangeException>(() => codec.Write(runtime, 7, 5, 0, new(), packet));
    }
    /// <summary>작은 Native 버퍼는 선택된 쓰기에서 실패하고 이미 쓴6BYTE를 보존한다.</summary>
    [Fact]
    public void CurrentShortBufferKeepsPrefixBeforeParentWriteFailure()
    {
        var runtime = new RawPathRuntime(OriginalEdition.Patch1078); byte[] packet = Enumerable.Repeat((byte)0xa5, 6).ToArray(); var codec = new RawPathData(OriginalEdition.Patch1078);
        Assert.Throws<ArgumentOutOfRangeException>(() => codec.WriteCurrent(runtime, 0x1234, 5, new(), packet, new())); Assert.Equal(Convert.FromHexString("013004003412"), packet);
        Array.Fill(packet, (byte)0xa5); Assert.Throws<ArgumentException>(() => codec.Write(runtime, 7, 5, 0, new(), packet)); Assert.All(packet, value => Assert.Equal(0xa5, value));
    }
    /// <summary>수신 관찰은 물리를 읽지 않고 송신 sentinel은 진단 복귀 뒤 현재signedWORD를 사용한다.</summary>
    [Fact]
    public void ObserverSelectsSendingAndReReadsWordAfterDiagnostic()
    {
        RawPathData.Observe(Span<byte>.Empty, 0, null); byte[] packet = Enumerable.Repeat((byte)0xa5, 24).ToArray(); Word(packet, 13, ushort.MaxValue); int calls = 0;
        RawPathData.Observe(packet, 1, (expression, file, line) => { Assert.Equal("nodeCount != (short)-1", expression); Assert.Equal(@"\Ns\O\_p_packets.cpp", file); Assert.Equal(0x716u, line); calls++; Word(packet, 13, 0x7fff); });
        Assert.Equal(1, calls); Assert.Equal(0x8017, ReadWord(packet, 2)); Word(packet, 13, unchecked((ushort)-24)); RawPathData.Observe(packet, 1, null); Assert.Equal(0, ReadWord(packet, 2)); Assert.Equal(0xa5, packet[4]);
    }
    /// <summary>관찰의 선택된 부족/sentinel 실패에서 이전 선언 길이를 보존한다.</summary>
    [Fact]
    public void ObserverMissingSelectedPhysicalOrDiagnosticKeepsEarlierLength()
    {
        byte[] shortPacket = new byte[14]; Word(shortPacket, 2, 0x1234); Assert.Throws<ArgumentOutOfRangeException>(() => RawPathData.Observe(shortPacket, 1, null)); Assert.Equal(0x1234, ReadWord(shortPacket, 2));
        byte[] packet = new byte[24]; Word(packet, 2, 0x4321); Word(packet, 13, ushort.MaxValue); Assert.Throws<InvalidOperationException>(() => RawPathData.Observe(packet, 1, null)); Assert.Equal(0x4321, ReadWord(packet, 2));
    }
}
