using System.Globalization;
using Netstorm.Core.Original;

// 원본 PE의 기존 관측만 기대값으로 사용해 경로 이미지/시간/보간을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>독립 원문216관측과 생성자32전체 이미지 및 실패 부분 효과를 대조한다.</summary>
public sealed class RawPathRuntimeTests
{
    /// <summary>원본 Prepare/Step이 받은 float 좌표를 같은 순서로 공급한다.</summary>
    private static readonly RawPathPoint[] Points = [new(10, 20), new(13.1f, 24.7f), new(15.2f, 19.8f), new(23.9f, 27.6f)];
    /// <summary>원본 시간표 입력의 네 방향이다.</summary>
    private static readonly int[] Directions = [7, 0, 4, 6];
    /// <summary>Step 연쇄가 읽는 원본 도착 시각이다.</summary>
    private static readonly double[] Times = [0, 1, 2, 3];
    /// <summary>한 소비자에 연결한 실제 원문 행만 읽고 빈 행/주석을 제외한다.</summary>
    private static string[][] Rows(string name) => File.ReadAllLines(Path.Combine(AppContext.BaseDirectory, "X86", name))
        .Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t')).ToArray();
    /// <summary>원문 판본 이름을 두 실제 배치 중 하나로 바꾼다.</summary>
    private static OriginalEdition Edition(string name) => name == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
    /// <summary>문화권과 무관하게 원문 DWORD를 읽는다.</summary>
    private static uint U32(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>원문 QWORD 비트를 double로 해석한다.</summary>
    private static double Double(string value) => BitConverter.UInt64BitsToDouble(ulong.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원문 DWORD 비트를 float로 해석한다.</summary>
    private static float Float(string value) => BitConverter.UInt32BitsToSingle(U32(value));
    /// <summary>전체 미초기화 영역까지 포함한 원본 FNV32 체크섬을 계산한다.</summary>
    private static uint Fnv(ReadOnlySpan<byte> bytes)
    {
        uint value = 2166136261;
        // 원문 이미지의 바이트를 순서대로 모두 소비한다.
        foreach (byte item in bytes) value = unchecked((value ^ item) * 16777619);
        return value;
    }

    /// <summary>두 판본/두 가상 종류/두 heap 입력의 전체 이미지 해시8개를 대조한다.</summary>
    [Fact]
    public void NativeConstructorHashesPreserveWholeImage()
    {
        string[][] rows = Rows("pathruntime-x86.tsv").Where(row => row[0] == "ctor").ToArray(); Assert.Equal(8, rows.Length);
        // 실제 원본이 해시한 같은 전체 객체를 각각 새로 생성한다.
        foreach (string[] row in rows)
        {
            var path = new RawPathRuntime(Edition(row[1]), U32(row[2]) != 0, (byte)U32(row[3]));
            Assert.Equal(U32(row[4]), Fnv(path.Bytes.Span));
        }
    }
    /// <summary>PATCH의8종 cdecl 생성 결과32개를 독립 RLE 전체100080BYTE와 직접 대조한다.</summary>
    [Fact]
    public void NativeConstructorsMatchAllBytesForEightClasses()
    {
        string[][] rows = Rows("path-constructor-x86.tsv"); Assert.Equal(32, rows.Length);
        // 원문 ignored caller 인자까지 다른 각 관측을 재계산 없이 읽는다.
        foreach (string[] row in rows)
        {
            uint type = U32(row[0]); var expected = new List<byte>(0x186f0);
            // 원본 RLE를 해제하며 부족한 바이트를 임의로 채우지 않는다.
            foreach (string run in row[4].Split(','))
            {
                string[] pair = run.Split(':'); Assert.Equal(2, pair.Length); uint count = U32(pair[0]), value = U32(pair[1]);
                Assert.True(value <= 255 && count <= 0x186f0 && (ulong)expected.Count + count <= 0x186f0);
                expected.AddRange(Enumerable.Repeat((byte)value, (int)count));
            }
            Assert.Equal(0x186f0, expected.Count);
            var path = new RawPathRuntime(OriginalEdition.Patch1078, type == 54, (byte)U32(row[2]), type == 39,
                type == 40, type == 42, type == 24, type == 41, type == 38);
            Assert.Equal(expected.ToArray(), path.Bytes.ToArray());
        }
    }
    /// <summary>속도/회전/2초 경계를 포함한48입력의 모든 double을 허용오차 없이 대조한다.</summary>
    [Fact]
    public void NativePrepareMatchesEveryQword()
    {
        string[][] rows = Rows("pathruntime-x86.tsv").Where(row => row[0] == "prepare").ToArray(); Assert.Equal(48, rows.Length);
        // 각 기대 시각은 원본 관측 QWORD이며 제품 공식으로 만들지 않는다.
        foreach (string[] row in rows)
        {
            var path = new RawPathRuntime(Edition(row[1])); path.SetPath(Points, Directions);
            path.Prepare(Double(row[2]), Double(row[3]), Float(row[4]), Float(row[5]), 7); string[] expected = row[6].Split(','); Assert.Equal(4, expected.Length);
            // 원문 네 저장 위치를 모두 검사한다.
            for (int index = 0; index < 4; index++) Assert.Equal(ulong.Parse(expected[index], CultureInfo.InvariantCulture), BitConverter.DoubleToUInt64Bits(path.Time(index)));
        }
    }
    /// <summary>실제 비행 생성자3관측/기존count를 유지하는 Init24관측을 전체 원문 해시와 대조한다.</summary>
    [Fact]
    public void NativeFlightInitializationPreservesExistingCountAndAllBytes()
    {
        string[][] rows = Rows("flypath-x86.tsv").Where(row => row[0] is "ctor" or "init").ToArray(); Assert.Equal(27, rows.Length);
        // Process 프레임63행은 이 Runtime 단계에서 성공으로 계산하지 않는다.
        foreach (string[] row in rows)
        {
            if (row[0] == "ctor")
            {
                var constructor = new RawPathRuntime(OriginalEdition.Patch1078, false, (byte)U32(row[1]), true);
                Assert.Equal(U32(row[2]), Fnv(constructor.Bytes.Span)); continue;
            }
            var path = new RawPathRuntime(OriginalEdition.Patch1078, flying: true); int existing = (int)U32(row[1]);
            path.SetPath(new RawPathPoint[existing], new int[existing]); path.RestoreStep(existing, 0);
            path.InitializeFlight(new(10, 20), new(Float(row[2]), Float(row[3]))); Assert.Equal(U32(row[4]), Fnv(path.Bytes.Span));
        }
    }
    /// <summary>원본과 같은 profile 연쇄160호출의 진입/반환/저장 상태와 발소리 횟수를 대조한다.</summary>
    [Fact]
    public void NativeStepMatchesEveryFloatAndKeepsCallSequence()
    {
        string[][] rows = Rows("pathruntime-x86.tsv").Where(row => row[0] == "step").ToArray(); Assert.Equal(160, rows.Length);
        RawPathRuntime? path = null; var observation = new RawPathObservation(); string? lastEdition = null; int lastProfile = -1;
        // 같은 profile은 새 객체로 되감지 않고 직전 원본 호출 결과를 이어 사용한다.
        foreach (string[] row in rows)
        {
            int profile = int.Parse(row[2], CultureInfo.InvariantCulture);
            if (lastEdition != row[1] || lastProfile != profile)
            {
                path = new(Edition(row[1])); path.SetPath(Points, [profile, (profile + 1) % 8, (profile + 5) % 8, profile], Times);
                observation = new(); lastEdition = row[1]; lastProfile = profile;
            }
            Assert.NotNull(path); Assert.Equal(unchecked((int)U32(row[4])), path.Current); Assert.Equal(U32(row[5]), BitConverter.SingleToUInt32Bits(path.Angle));
            int sounds = 0; RawPathSample sample = path.Step(Double(row[3]), new(10, 20), observation, () => sounds++, 0x12345678); string[] expected = row[6].Split(','); Assert.Equal(7, expected.Length);
            Assert.Equal(U32(expected[0]), BitConverter.SingleToUInt32Bits(sample.Position.X)); Assert.Equal(U32(expected[1]), BitConverter.SingleToUInt32Bits(sample.Position.Y));
            Assert.Equal(U32(expected[2]), BitConverter.SingleToUInt32Bits(sample.Angle)); Assert.Equal(unchecked((int)U32(expected[3])), path.Current);
            Assert.Equal(U32(expected[4]), BitConverter.SingleToUInt32Bits(path.Angle)); Assert.Equal(U32(expected[5]) != 0, sample.Finished); Assert.Equal((int)U32(expected[6]), sounds);
        }
    }
    /// <summary>빈 경로/속도0/상태 이관 실패를 직선 성공으로 대신하지 않고 반대 방향 동률을 보존한다.</summary>
    [Fact]
    public void InvalidInputAndOppositeDirectionKeepSourceContract()
    {
        var path = new RawPathRuntime(OriginalEdition.Patch1078); var observation = new RawPathObservation();
        Assert.Throws<ArgumentException>(() => path.Prepare(0, 0, 1, 1, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => path.Step(0, new(1, 1), observation, null));
        path.SetPath([new(1, 1)], [0]); byte[] before = path.Bytes.ToArray();
        Assert.Throws<ArgumentException>(() => path.Prepare(0, 0, 0, 1, 0)); Assert.Throws<ArgumentException>(() => path.Prepare(0, 0, 1, 0, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => path.RestoreStep(2, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => path.RestoreStep(0, float.NaN)); Assert.Equal(before, path.Bytes.ToArray());
        Assert.Equal(180.0, RawPathRuntime.DirectionDelta(0, 4)); Assert.Equal(45.0, RawPathRuntime.DirectionDelta(7, 0)); Assert.Equal(-45.0, RawPathRuntime.DirectionDelta(0, 7));
    }
    /// <summary>짝수 전환의 실제 발소리가 누락돼도 앞선 current/관찰 저장을 되돌리지 않는다.</summary>
    [Fact]
    public void MissingSoundPreservesAlreadyAdvancedCurrentAndObservation()
    {
        var path = new RawPathRuntime(OriginalEdition.Patch1078); path.SetPath(Points, Directions, Times); path.RestoreStep(1, 7.5f);
        var observation = new RawPathObservation { Flags = 0x10, Stamp = ulong.MaxValue };
        Assert.Throws<InvalidOperationException>(() => path.Step(1, new(5, 6), observation, null));
        Assert.Equal(2, path.Current); Assert.Equal(7.5f, path.Angle); Assert.False(path.Finished); Assert.Equal(0x11u, observation.Flags); Assert.Equal(0ul, observation.Stamp);
    }
}
