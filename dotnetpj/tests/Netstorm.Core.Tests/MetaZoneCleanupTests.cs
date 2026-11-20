using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;

// 최초 독립 x86 관측의 모든 바이트·flag·선택 호출을 같은 원문 본체와 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>메타구역 부모38입력과 마커86입력·짧은 물리의 부분 효과를 검증한다.</summary>
public sealed class MetaZoneCleanupTests
{
    /// <summary>명시 외부 장치의 중단만 구분하며 제품 물리 예외와 혼동하지 않는다.</summary>
    private sealed class Stop : Exception { }
    /// <summary>원문10진 DWORD를 현재 OS 문화권과 독립적으로 읽는다.</summary>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>원문 초기 패턴을 실제 표와 인접16BYTE에만 입력한다. 기대 출력은 fixture에서 읽는다.</summary>
    private static byte[] Pattern(uint seed)
    {
        byte[] bytes = new byte[RawMetaZoneCleanup.Bytes + 16];
        // 초기 원시 입력의 모든 바이트를 원문 식의 하위8비트로 만든다.
        for (int index = 0; index < bytes.Length; index++) bytes[index] = unchecked((byte)(seed + index * 17));
        return bytes;
    }
    /// <summary>명시 입력 또는 callback이 현재 실제 DWORD 한 칸만 바꾸도록 한다.</summary>
    private static void Put(byte[] bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset, 4), value);
    /// <summary>사본 없는 원문 출력 전체와 canary를 같은 소문자 hex로 대조한다.</summary>
    private static string Hex(byte[] bytes) => Convert.ToHexStringLower(bytes);
    /// <summary>링크된 최초 fixture의 실제 모든 데이터 행을 읽으며 첫 행/마지막 칼럼을 버리지 않는다.</summary>
    private static IEnumerable<string[]> Rows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name))
        .Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));

    /// <summary>두 제어어의 정상28·명시중단10 전체 물리와 현재flag·자식 호출/변경 순서를 대조한다.</summary>
    [Fact]
    public void IndependentParent38MatchesEveryByteFlagAndChildEffect()
    {
        int rows = 0, normal = 0, stopped = 0;
        // 원문38관측을 빠짐없이 실행하고 기대를 제품 코드에서 다시 만들지 않는다.
        foreach (string[] row in Rows("meta-zone-cleanup-x86.tsv"))
        {
            Assert.Equal(10, row.Length); string[] input = row[1].Split(','); Assert.Equal(4, input.Length);
            byte[] bytes = Pattern(Number(input[0])); var flag = new UIntBox { Value = Number(input[1]) };
            string[] mutation = input[2].Split(':'); uint mutateAt = Number(mutation[0]), stopAt = Number(input[3]), calls = 0;
            var indices = new List<uint>(); var trace = new List<string>(); string status = "normal";
            try
            {
                RawMetaZoneCleanup.ClearRecords(flag, bytes, index =>
                {
                    indices.Add(index); trace.Add($"C:{index}"); calls++;
                    if (calls == stopAt) throw new Stop();
                    if (calls != mutateAt) return;
                    trace.Add("M:" + mutation[1]);
                    switch (mutation[1])
                    {
                        case "flag1": flag.Value = 1; break;
                        case "flagMax": flag.Value = uint.MaxValue; break;
                        case "touch": Put(bytes, (int)index * RawMetaZoneCleanup.Stride + 76, 0x89abcdef); break;
                        default: throw new InvalidOperationException("미해석 원문 callback 입력");
                    }
                });
            }
            catch (Stop) { status = "stop"; }
            Assert.Equal(row[2], status); Assert.Equal(Number(row[3]), flag.Value); Assert.Equal(row[4], Hex(bytes));
            Assert.Equal(row[5], indices.Count == 0 ? "-" : string.Join(',', indices));
            Assert.Equal(row[6], trace.Count == 0 ? "-" : string.Join(';', trace));
            Assert.Equal(status == "normal" ? "1836" : "0", row[7]); Assert.True(Number(row[9]) > 0);
            rows++; if (status == "normal") normal++; else stopped++;
        }
        Assert.Equal(38, rows); Assert.Equal(28, normal); Assert.Equal(10, stopped);
    }

    /// <summary>signed첫 관문·전체 DWORD SID·삭제 복귀 후 쓰기와 명시중단을 독립86관측에 대조한다.</summary>
    [Fact]
    public void IndependentMarker86MatchesEveryByteAndVirtualCall()
    {
        int rows = 0, stopped = 0;
        // 두 x87 환경의 전체 입력과 물리7360BYTE를 각각 대조한다.
        foreach (string[] row in Rows("meta-zone-marker-x86.tsv"))
        {
            Assert.Equal(11, row.Length); byte[] bytes = Pattern(Number(row[0])); uint index = Number(row[1]);
            int offset = (int)index * RawMetaZoneCleanup.Stride; Put(bytes, offset, Number(row[2])); Put(bytes, offset + 76, Number(row[3]));
            string status = "normal", calls = "-";
            try
            {
                RawMetaZoneCleanup.ClearMarker(bytes, index, (sid, flags) =>
                {
                    calls = $"{unchecked(0x12000000u + sid * 50u)}:{flags}";
                    if (row[4] == "stop") throw new Stop();
                    if (row[4] == "sid") Put(bytes, offset + 76, 0x76543210);
                    if (row[4] == "status") Put(bytes, offset, 0x89abcdef);
                });
            }
            catch (Stop) { status = "stop"; stopped++; }
            Assert.Equal(row[6], status); Assert.Equal(row[7], Hex(bytes)); Assert.Equal(row[8], calls); Assert.True(Number(row[10]) > 0); rows++;
        }
        Assert.Equal(86, rows); Assert.Equal(2, stopped);
    }

    /// <summary>첫/중간/마지막 DWORD의 짧은 물리에서는 완료한 store만 남기고 flag0을 앞당기지 않는다.</summary>
    [Fact]
    public void ShortPhysicalPreservesExactDwordPrefixAndChildTiming()
    {
        // 기존 CPP 경계 입력을 같은 실제 물리에서 실행한다.
        foreach (int length in new[] { 0, 1, 3, 4, 5, 7, 8, 142, 143, 144, 7339, 7340, 7343, 7344, 7348 })
        {
            byte[] bytes = Enumerable.Repeat((byte)0xa5, RawMetaZoneCleanup.Bytes + 16).ToArray(); var flag = new UIntBox { Value = 1 };
            Exception? error = Record.Exception(() => RawMetaZoneCleanup.ClearRecords(flag, bytes.AsMemory(0, length), null));
            if (length < RawMetaZoneCleanup.Bytes) Assert.IsType<ArgumentOutOfRangeException>(error); else Assert.Null(error);
            Assert.Equal(length < RawMetaZoneCleanup.Bytes ? 1u : 0u, flag.Value); int written = Math.Min(length / 4, RawMetaZoneCleanup.Bytes / 4) * 4;
            Assert.All(bytes[..written], value => Assert.Equal(0, value)); Assert.All(bytes[written..], value => Assert.Equal(0xa5, value));
        }
        byte[] shortBytes = Enumerable.Repeat((byte)0xa5, RawMetaZoneCleanup.Bytes).ToArray(); var current = new UIntBox(); uint calls = 0;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawMetaZoneCleanup.ClearRecords(current, shortBytes.AsMemory(0, 8), index =>
        { Assert.Equal(calls, index); calls++; if (index == 50) current.Value = 0x12345678; }));
        Assert.Equal(51u, calls); Assert.Equal(0x12345678u, current.Value); Assert.Equal(new byte[8], shortBytes[..8]); Assert.Equal(0xa5, shortBytes[8]);
    }

    /// <summary>상태비양수/SID0은 뒤 물리·삭제 장치를 요구하지 않으며 선택 실패는 SID를 보존한다.</summary>
    [Fact]
    public void MarkerDemandsOnlySelectedFieldsAndPreservesFailedDelete()
    {
        byte[] bytes = Enumerable.Repeat((byte)0xa5, 100).ToArray(); Put(bytes, 0, uint.MaxValue); Put(bytes, 76, 0x10000);
        RawMetaZoneCleanup.ClearMarker(bytes.AsMemory(0, 4), 0, null); Assert.Equal(1, bytes[78]);
        Put(bytes, 0, 1); Put(bytes, 76, 0); RawMetaZoneCleanup.ClearMarker(bytes, 0, null);
        Put(bytes, 76, 0x10000); byte[] before = bytes.ToArray();
        Assert.Throws<InvalidOperationException>(() => RawMetaZoneCleanup.ClearMarker(bytes, 0, null)); Assert.Equal(before, bytes);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawMetaZoneCleanup.ClearMarker(bytes.AsMemory(0, 79), 0, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawMetaZoneCleanup.ClearMarker(bytes, 51, null)); Assert.Equal(before, bytes);
        uint observed = 0; RawMetaZoneCleanup.ClearMarker(bytes, 0, (sid, flags) => { observed = sid; Assert.Equal(0u, flags); Put(bytes, 76, 7); });
        Assert.Equal(0x10000u, observed); Assert.Equal(new byte[4], bytes[76..80]); Assert.Equal(before[80], bytes[80]);
    }

    /// <summary>최초 비영flag만 자식을 생략하고 이후 자식 실패의 현재flag/부분 쓰기는 그대로 남긴다.</summary>
    [Fact]
    public void FirstResetSkipsChildrenAndFailedCallbackKeepsActualEffects()
    {
        byte[] bytes = Enumerable.Repeat((byte)0xa5, RawMetaZoneCleanup.Bytes + 16).ToArray(); var flag = new UIntBox { Value = uint.MaxValue };
        RawMetaZoneCleanup.ClearRecords(flag, bytes, null); Assert.Equal(0u, flag.Value);
        Assert.All(bytes[..RawMetaZoneCleanup.Bytes], value => Assert.Equal(0, value)); Assert.Equal(0xa5, bytes[^1]);
        bytes[0] = 0x7a; Assert.Throws<InvalidOperationException>(() => RawMetaZoneCleanup.ClearRecords(flag, bytes, null));
        Assert.Equal(0x7a, bytes[0]);
        Assert.Throws<Stop>(() => RawMetaZoneCleanup.ClearRecords(flag, bytes, index => { Assert.Equal(0u, index); flag.Value = 1; bytes[1] = 0x6b; throw new Stop(); }));
        Assert.Equal(1u, flag.Value); Assert.Equal(0x7a, bytes[0]); Assert.Equal(0x6b, bytes[1]);
    }
}
