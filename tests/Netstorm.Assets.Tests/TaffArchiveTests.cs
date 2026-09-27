using System.Text;

namespace Netstorm.Assets.Tests;

/// <summary>TAFF 아카이브·XOR 인코딩 테스트 (docs/formats/taff.md 의 확인 결과와 일치해야 한다)</summary>
public sealed class TaffArchiveTests
{
    /// <summary>원본 아카이브의 엔트리 수</summary>
    private const int ExpectedEntryCount = 246;

    /// <summary>XOR 을 두 번 적용하면 원래 데이터로 돌아온다</summary>
    [Fact]
    public void XorCipher_RoundTrip()
    {
        byte[] original = Encoding.ASCII.GetBytes("typename Altar; NetStorm XOR test 0123456789");
        byte[] data = (byte[])original.Clone();
        XorCipher.Apply(data);
        Assert.NotEqual(original, data);
        XorCipher.Apply(data);
        Assert.Equal(original, data);
    }

    /// <summary>경로 정규화: 대소문자·구분자·앞 구분자 무시</summary>
    [Theory]
    [InlineData(@"\d\altar.type")]
    [InlineData("d/altar.type")]
    [InlineData(@"D\ALTAR.TYPE")]
    [InlineData("/d/Altar.Type")]
    public void NormalizePath_IgnoresCaseAndSeparators(string path)
    {
        Assert.Equal("d/altar.type", TaffArchive.NormalizePath(path));
    }

    /// <summary>원본 아카이브: 엔트리 수, 복호화 결과, 이름 검색</summary>
    [Fact]
    public void OriginalArchive_ParsesAndDecrypts()
    {
        TaffArchive archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        Assert.Equal("TAFF v0.2", archive.Version);
        Assert.Equal(ExpectedEntryCount, archive.Entries.Count);

        Assert.True(archive.TryFind("D/ALTAR.TYPE", out TaffEntry altar));
        string text = Encoding.Latin1.GetString(archive.Read(altar));
        Assert.StartsWith("typename Altar", text);
    }

    /// <summary>원본 아카이브: 모든 .type 엔트리가 "typename" 으로 시작하는 텍스트로 복호화된다</summary>
    [Fact]
    public void OriginalArchive_AllTypeFilesDecryptToText()
    {
        TaffArchive archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        var types = archive.Entries.Where(e => e.Name.EndsWith(".type", StringComparison.OrdinalIgnoreCase)).ToList();
        Assert.Equal(119, types.Count);
        // .type 엔트리마다 복호화 결과를 검사
        foreach (TaffEntry entry in types)
        {
            string text = Encoding.Latin1.GetString(archive.Read(entry)).TrimStart();
            Assert.True(text.StartsWith("typename", StringComparison.Ordinal) || text.StartsWith("//", StringComparison.Ordinal),
                $"{entry.Name} 복호화 결과가 텍스트가 아님");
        }
    }
}
