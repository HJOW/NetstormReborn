using System.Text;

namespace Netstorm.Tests;

/// <summary>
/// 연결한 원본 기계어 fixture 가 목록에 기록된 SHA-256·행 수와 같은지 검사한다 (W01 동기화 장치, 2026-10-16).
/// 실패하면 cpppj 가 fixture 를 다시 만들었거나 연결/등록이 어긋난 것이므로 해당 C# 검사를 재대조해야 한다.
/// 두 테스트 프로젝트가 같은 소스를 링크해 각자 자기 사용 프로젝트(Core/Assets)의 항목만 확인한다.
/// </summary>
public sealed class CppFixtureManifestTests
{
    /// <summary>갱신 방법 안내. 모든 실패 메시지 끝에 붙인다.</summary>
    private const string Guide = "\n해당 fixture 를 쓰는 C# 검사를 재대조한 뒤 `python dotnetpj/tools/cpp_sync.py --update` 로 목록을 갱신한다. 기대값을 C# 결과로 덮어쓰지 않는다.";

    /// <summary>목록 자체의 형식: 반영 커밋, 열 값, 이름 중복 여부를 확인한다.</summary>
    [Fact]
    public void ManifestIsWellFormed()
    {
        Assert.Matches("^[0-9a-f]{40}$", CppFixtureManifest.ReflectedCommit);
        Assert.NotEmpty(CppFixtureManifest.Entries);
        // 항목마다 열 값의 형식을 확인한다.
        foreach (CppFixtureEntry entry in CppFixtureManifest.Entries)
        {
            Assert.EndsWith(".tsv", entry.File);
            Assert.Matches("^[0-9a-f]{64}$", entry.Sha256);
            Assert.True(entry.Rows > 0, $"{entry.File}: 행 수가 0 이하");
            Assert.Contains(entry.Origin, new[] { "cpppj", "dotnetpj" });
            Assert.Contains(entry.Consumer, new[] { "Core", "Assets" });
        }
        Assert.Equal(CppFixtureManifest.Entries.Count, CppFixtureManifest.Entries.Select(entry => entry.File).Distinct().Count());
    }

    /// <summary>이 프로젝트가 쓰는 fixture 가 모두 출력 폴더에 있고, 목록의 SHA-256·행 수와 같은지 확인한다. 모든 불일치를 한꺼번에 보고한다.</summary>
    [Fact]
    public void LinkedFixturesMatchRecordedHashAndRows()
    {
        var problems = new List<string>();
        // 이 프로젝트의 항목마다 실제 파일을 읽어 비교한다.
        foreach (CppFixtureEntry entry in CppFixtureManifest.Entries.Where(entry => entry.Consumer == CppFixtureManifest.Consumer))
        {
            string path = Path.Combine(CppFixtureManifest.X86Directory, entry.File);
            if (!File.Exists(path))
            {
                problems.Add($"{entry.File}: 출력 폴더에 없다 (csproj 연결이 빠졌다)");
                continue;
            }
            byte[] data = File.ReadAllBytes(path);
            string sha = CppFixtureManifest.Sha256Lf(data);
            int rows = CppFixtureManifest.CountRows(data);
            if (sha != entry.Sha256 || rows != entry.Rows)
                problems.Add($"{entry.File}: 목록 {entry.Rows}행 {entry.Sha256[..12]} / 실제 {rows}행 {sha[..12]}");
        }
        Assert.True(problems.Count == 0, $"연결 fixture {problems.Count}개가 목록과 다르다 (반영 커밋 {CppFixtureManifest.ReflectedCommit[..8]}):\n" + string.Join('\n', problems) + Guide);
    }

    /// <summary>출력 폴더의 fixture 가 모두 목록에 등록돼 있는지 확인한다. csproj 에 연결만 하고 SHA-256 을 기록하지 않는 일을 막는다.</summary>
    [Fact]
    public void EveryLinkedFixtureIsRegistered()
    {
        var registered = CppFixtureManifest.Entries.Where(entry => entry.Consumer == CppFixtureManifest.Consumer).Select(entry => entry.File).ToHashSet();
        // 출력 폴더의 *.tsv 마다 목록 항목이 있는지 확인한다. 목록 파일 자신은 제외한다.
        string[] missing = Directory.GetFiles(CppFixtureManifest.X86Directory, "*.tsv")
            .Select(path => Path.GetFileName(path)).Where(name => name != CppFixtureManifest.FileName && !registered.Contains(name)).Order().ToArray();
        Assert.True(missing.Length == 0, "목록에 없는 fixture: " + string.Join(", ", missing) + "\n`python dotnetpj/tools/cpp_sync.py --update --add <이름>:" + CppFixtureManifest.Consumer + "` 로 등록한다.");
    }

    /// <summary>CRLF 와 LF 로 저장한 같은 내용은 같은 SHA-256·행 수가 나오고, 내용 변경이나 단독 CR 은 달라지는지 확인한다.</summary>
    [Fact]
    public void HashIgnoresCrlfButNotContent()
    {
        byte[] lf = Encoding.UTF8.GetBytes("# 주석\nA\t1\n\nB\t2\n");
        byte[] crlf = Encoding.UTF8.GetBytes("# 주석\r\nA\t1\r\n\r\nB\t2\r\n");
        Assert.Equal(CppFixtureManifest.Sha256Lf(lf), CppFixtureManifest.Sha256Lf(crlf));
        Assert.Equal(2, CppFixtureManifest.CountRows(lf));
        Assert.Equal(2, CppFixtureManifest.CountRows(crlf));
        // 내용이 한 글자라도 다르면 해시가 달라진다.
        Assert.NotEqual(CppFixtureManifest.Sha256Lf(lf), CppFixtureManifest.Sha256Lf(Encoding.UTF8.GetBytes("# 주석\nA\t1\n\nB\t3\n")));
        // 단독 CR 은 줄바꿈이 아니라 데이터이므로 LF 나 삭제와 구분된다.
        byte[] loneCr = Encoding.UTF8.GetBytes("A\rB\n");
        Assert.NotEqual(CppFixtureManifest.Sha256Lf(loneCr), CppFixtureManifest.Sha256Lf(Encoding.UTF8.GetBytes("A\nB\n")));
        Assert.NotEqual(CppFixtureManifest.Sha256Lf(loneCr), CppFixtureManifest.Sha256Lf(Encoding.UTF8.GetBytes("AB\n")));
    }
}
