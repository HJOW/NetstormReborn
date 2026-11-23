using Netstorm.Core.Original;

// 실제 타입 표 생성과 Clone이 기본 프레임/두 현재 DWORD를 보존하는지 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 ApplyDefinition 접두의 두 판본과 현재 정의 대입을 검사한다.</summary>
public sealed class NativeLoadedAnimationTypeTests
{
    /// <summary>실제 타입 표에 숫자 AST와 클러스터/특수프레임을 함께 공급한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)] [InlineData(OriginalEdition.Cd1072)]
    public void LoadedRecordsAndClonePreserveExactAnimationFields(OriginalEdition edition)
    {
        var input = new RiftTypeTableTests.TestTypes(edition); int at = input.Sources.FindIndex(source => string.Equals(source.AssetName, "sunArcher", StringComparison.OrdinalIgnoreCase)); Assert.True(at >= 0); string name = input.Sources[at].AssetName; RiftTypeDefinition definition = RiftTypeDefinition.Parse("typename sunArcher { orientations=15.9999999; } P00::\"a.gif\"#0; P01:default:\"a.gif\"#1;"); input.Sources[at] = new(name, definition); OriginalTypeRecord record = input.Table(edition).Types[71]; Assert.Equal(1, record.DefaultFrame); Assert.Equal(16, record.Directions); Assert.Equal(2, record.FrameCount); OriginalTypeRecord copy = record.Clone(); record.DefaultFrame = 0; record.Directions = -3; record.FrameCount = 17; Assert.Equal(1, copy.DefaultFrame); Assert.Equal(16, copy.Directions); Assert.Equal(2, copy.FrameCount);
        definition.Properties.Clear(); definition.Clusters.Add(new()); input.Sources[at] = new(name, definition); record = input.Table(edition).Types[71]; Assert.Equal(0, record.Directions); Assert.Equal(3, record.FrameCount); Assert.Equal(1, record.DefaultFrame);
    }
}
