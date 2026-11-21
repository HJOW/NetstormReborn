using System.Globalization;
using Netstorm.Core.Original;

// 실제 CPP가 저장한102float/3정수와 CS 기록 초기화를 대조한다.
namespace Netstorm.Tests;

/// <summary>CS 최초 배치와 이미 초기화된 장기 좌표의 보존을 확인한다.</summary>
public sealed class RawChallengeStateTests
{
    /// <summary>저장된 float의 비트를 보정 없이 가져온다.</summary>
    private static long Bits(float value) => BitConverter.SingleToUInt32Bits(value);

    /// <summary>CPP probe와 같은 필드/순서의 현재 관측이다.</summary>
    private static Dictionary<string, long> Values(ChallengeGeometry geometry)
    {
        var values = new Dictionary<string, long>
        {
            ["initialized"] = geometry.Initialized, ["width"] = geometry.Width, ["height"] = geometry.Height,
            ["firstRadius"] = Bits(geometry.FirstRadius), ["secondRadius"] = Bits(geometry.SecondRadius),
            ["originX"] = Bits(geometry.OriginX), ["originY"] = Bits(geometry.OriginY),
        };
        // 모든 실제 ring/grid 성분을 빠짐없이 대조한다.
        foreach ((string name, float[,] points) in new[] { ("ring", geometry.Ring), ("grid", geometry.Grid) })
        {
            // 각 좌표의 두 저장 float를 별도 키로 남긴다.
            for (int index = 0; index < points.GetLength(0); index++)
            {
                values[$"{name}{index}x"] = Bits(points[index, 0]);
                values[$"{name}{index}y"] = Bits(points[index, 1]);
            }
        }
        return values;
    }

    /// <summary>독립 PE 자료와 구별한 실제 Windows CPP Source 관찰105행을 전부 비교한다.</summary>
    [Fact]
    public void ConstructMatchesAll105CppSourceValuesAndClearsWholeRecord()
    {
        var expected = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "challenge-geometry-cpp.tsv"))
            .Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'))
            .ToDictionary(row => row[0], row => long.Parse(row[1], CultureInfo.InvariantCulture));
        var geometry = new ChallengeGeometry(); byte[] bytes = Enumerable.Repeat((byte)0xcc, RawChallengeState.Bytes).ToArray();
        RawChallengeState.Construct(bytes, geometry);
        Assert.Equal(105, expected.Count); Assert.Equal(expected.OrderBy(pair => pair.Key), Values(geometry).OrderBy(pair => pair.Key));
        Assert.All(bytes, value => Assert.Equal(0, value));
    }

    /// <summary>최초 grid0은 보존하고 initialized 이후 재생성은 전체 배치/크기를 건드리지 않는다.</summary>
    [Fact]
    public void GridZeroAndAlreadyInitializedGeometrySurviveRecordReset()
    {
        var geometry = new ChallengeGeometry(); geometry.Grid[0, 0] = 1.25f; geometry.Grid[0, 1] = -3.5f;
        byte[] bytes = new byte[RawChallengeState.Bytes]; RawChallengeState.Construct(bytes, geometry);
        Assert.Equal(1.25f, geometry.Grid[0, 0]); Assert.Equal(-3.5f, geometry.Grid[0, 1]);
        geometry.Width = -17; geometry.Ring[8, 1] = BitConverter.UInt32BitsToSingle(0x7fc00123);
        var before = Values(geometry); bytes.AsSpan().Fill(0xd3); RawChallengeState.Construct(bytes, geometry);
        Assert.Equal(before.OrderBy(pair => pair.Key), Values(geometry).OrderBy(pair => pair.Key)); Assert.All(bytes, value => Assert.Equal(0, value));
    }
}
