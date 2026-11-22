using System.Text;
using Netstorm.Assets;

namespace Netstorm.Tests; // 현재 원문 이름과 실제 검색 수명/반납을 확인한다.

/// <summary>FileSpec의 Source 계약과 실제 Windows Unicode 검색이다.</summary>
public sealed class FileSpecQueryTests
{
    /// <summary>검색 입력/결과의 원시 UTF-8 표현이다.</summary>
    private static byte[] B(string text) => Encoding.UTF8.GetBytes(text);
    /// <summary>순수 BYTE 검사의 현재 이름이다.</summary>
    private static string T(FileSpecQuery query) => Encoding.UTF8.GetString(query.Text.Span);
    /// <summary>Source 일반/유효검색/종료/실패 핸들의 재대입과 동일 반납을 재생한다.</summary>
    [Fact]
    public void AssignNextAndInvalidHandleKeepSourceLifetime()
    {
        var closed = new List<nuint>(); int next = 0, find = 0;
        var hooks = new FileSpecQueryHooks { Find = _ => new(++find == 1 ? 0x1234u : nuint.MaxValue, B("one")), Close = handle => closed.Add(handle), Next = _ => { next++; return null; } };
        using var query = new FileSpecQuery(B("ordinary"), hooks); Assert.True(query.Valid); Assert.False(query.Next()); Assert.Equal(".\\ordinary", T(query));
        query.Assign(B("d\\*")); Assert.True(query.Valid); Assert.Equal("d\\one", T(query)); Assert.False(query.Next()); Assert.Equal("d\\one", T(query)); Assert.Equal(1, next);
        query.Assign(B("C:/other")); Assert.True(query.Valid); Assert.False(query.Next()); Assert.Equal("C:\\other", T(query)); Assert.Equal(new nuint[] { 0x1234 }, closed);
        query.Assign(B("missing*")); Assert.False(query.Valid); Assert.False(query.Next()); Assert.Empty(query.Text.ToArray()); Assert.Equal(1, next); query.Assign(B("done")); Assert.Equal(new nuint[] { 0x1234, nuint.MaxValue }, closed);
    }
    /// <summary>Find는 원래 패턴이며 결과 prefix는 원래 backslash만 사용하고 장치는 생성 때 복사한다.</summary>
    [Fact]
    public void OriginalPatternNulAndCopiedHooksStayDistinctFromNormalizedText()
    {
        var seen = new List<string>(); var closed = new List<nuint>();
        var hooks = new FileSpecQueryHooks { Find = name => { seen.Add(Encoding.Latin1.GetString(name.Span)); return new(7, B("chosen\0tail")); }, Close = handle => closed.Add(handle), Next = _ => new ReadOnlyMemory<byte>(B("later\0tail")) };
        using var query = new FileSpecQuery(B("d/*.cfg\0ignored"), hooks); Assert.Equal(new[] { "d/*.cfg" }, seen); Assert.Equal("chosen", T(query));
        hooks.Find = _ => throw new InvalidOperationException("외부 교체 장치"); hooks.Close = _ => throw new InvalidOperationException("외부 교체 반납"); hooks.Next = _ => null;
        Assert.True(query.Next()); Assert.Equal("later", T(query)); query.Assign(B("d\\*.cfg")); Assert.Equal("d\\chosen", T(query)); Assert.Equal(new nuint[] { 7 }, closed); Assert.True(query.Next()); Assert.Equal("d\\later", T(query)); query.Dispose(); query.Dispose(); Assert.Equal(new nuint[] { 7, 7 }, closed);
    }
    /// <summary>일반 이름에서는 미선택 장치를 요구하지 않으며 실제 선택 때 누락을 드러낸다.</summary>
    [Fact]
    public void SelectedMissingSearchAndNextAreRejected()
    {
        using var ordinary = new FileSpecQuery(B("a/b"), new() { Find = _ => throw new Exception() }); Assert.Equal(".\\a\\b", T(ordinary));
        Assert.Throws<InvalidOperationException>(() => ordinary.Assign(B("*")));
        var closed = new List<nuint>(); using var selected = new FileSpecQuery(B("*"), new() { Find = _ => new(9, B("one")), Close = handle => closed.Add(handle) }); Assert.Throws<InvalidOperationException>(() => selected.Next()); selected.Dispose(); Assert.Equal(new nuint[] { 9 }, closed);
        using var failed = new FileSpecQuery(B("*"), new() { Find = _ => new(nuint.MaxValue, ReadOnlyMemory<byte>.Empty), Close = handle => closed.Add(handle) }); Assert.False(failed.Valid); Assert.False(failed.Next()); failed.Dispose(); Assert.Equal(new nuint[] { 9, nuint.MaxValue }, closed);
    }
    /// <summary>실제 W 검색은 Unicode/보충문자 폴더와 가상 CWD·다음 이름·실패 핸들을 처리한다.</summary>
    [Fact]
    public void ActualUnicodeSearchKeepsHostCurrentDirectoryAndRootedRelativePolicy()
    {
        Assert.True(OperatingSystem.IsWindows()); string current = Environment.CurrentDirectory;
        string root = Path.Combine(Path.GetTempPath(), "Netstorm-FileSpec-" + Guid.NewGuid().ToString("N"), "한글😀"), data = Path.Combine(root, "d"); Directory.CreateDirectory(data); File.WriteAllBytes(Path.Combine(data, "하나😀.cfg"), [1]); File.WriteAllBytes(Path.Combine(data, "둘.cfg"), [2]);
        var hooks = FileSpecQuery.DirectoryHooks(root);
        // 일반 상대/루트 디렉터리만 있는 상대/같은 드라이브 상대가 같은 실제 게임 폴더를 검색한다.
        foreach (string pattern in new[] { "d\\*.cfg", "\\d\\*.cfg", Path.GetPathRoot(root)![..2] + "d\\*.cfg" })
        {
            using var query = new FileSpecQuery(B(pattern), hooks); Assert.True(query.Valid); var actual = new HashSet<string>(StringComparer.Ordinal);
            do { string name = T(query); actual.Add(name[(name.LastIndexOf('\\') + 1)..]); } while (query.Next()); Assert.Equal(2, actual.Count); Assert.Contains("하나😀.cfg", actual); Assert.Contains("둘.cfg", actual); string last = T(query); Assert.False(query.Next()); Assert.Equal(last, T(query));
        }
        using (var direct = new FileSpecQuery(B(Path.Combine(data, "*.cfg")))) Assert.True(direct.Valid);
        using (var missing = new FileSpecQuery(B("d\\*.absent"), hooks)) { Assert.False(missing.Valid); Assert.Empty(missing.Text.ToArray()); Assert.False(missing.Next()); }
        Assert.Equal(current, Environment.CurrentDirectory);
    }
}
