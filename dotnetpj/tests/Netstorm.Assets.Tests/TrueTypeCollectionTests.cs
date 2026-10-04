using System.Buffers.Binary;

namespace Netstorm.Assets.Tests;

/// <summary>TTC 분리 테스트 (저장소의 D2Coding 글꼴 사용)</summary>
public sealed class TrueTypeCollectionTests
{
    /// <summary>D2Coding TTC 파일 이름 (AGENTS.md 지정 한국어 글꼴)</summary>
    private const string D2CodingFile = "D2Coding-Ver1.3.2-20180524-all.ttc";

    /// <summary>TrueType 글꼴 파일의 sfnt 버전 값 (0x00010000)</summary>
    private const uint TrueTypeVersion = 0x00010000;

    /// <summary>D2Coding 은 face 4개를 담은 TTC 다</summary>
    [Fact]
    public void D2Coding_IsCollectionWithFourFaces()
    {
        byte[] data = File.ReadAllBytes(OriginalData.RepositoryFont(D2CodingFile));
        Assert.True(TrueTypeCollection.IsCollection(data));
        Assert.Equal(4, TrueTypeCollection.FaceCount(data));
    }

    /// <summary>분리한 face 는 TrueType 헤더로 시작하고, 모든 테이블이 파일 범위 안에 있다</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    public void ExtractFace_ProducesValidTrueType(int face)
    {
        byte[] ttf = TrueTypeCollection.ExtractFace(File.ReadAllBytes(OriginalData.RepositoryFont(D2CodingFile)), face);
        Assert.Equal(TrueTypeVersion, BinaryPrimitives.ReadUInt32BigEndian(ttf));
        int numTables = BinaryPrimitives.ReadUInt16BigEndian(ttf.AsSpan(4));
        Assert.True(numTables > 0);
        // 테이블 레코드마다 오프셋+길이가 파일 크기 안에 있는지 검사
        for (int i = 0; i < numTables; i++)
        {
            int record = 12 + i * 16;
            uint offset = BinaryPrimitives.ReadUInt32BigEndian(ttf.AsSpan(record + 8));
            uint length = BinaryPrimitives.ReadUInt32BigEndian(ttf.AsSpan(record + 12));
            Assert.True(offset + length <= (uint)ttf.Length);
        }
    }
}
