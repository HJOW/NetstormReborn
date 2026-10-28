namespace Netstorm.Tests;

/// <summary>
/// 포트 맵(dotnetpj/PORT_MAP.tsv)의 형식과 경로 무결성을 검사한다.
/// 각 행은 cpppj 소스, 대응 C# 파일, 이식 기준 커밋, 상태를 가진다. 이 검사는 파일이 없어지거나 오타가 생긴 행만 잡으며,
/// cpppj 소스가 기준 커밋 이후 바뀌었는지는 `python dotnetpj/tools/cpp_sync.py --ports` 가 git 으로 보고한다.
/// (2026-10-28 R0-3 에서 추가: fixture 가 아니라 cpppj 소스 변경도 추적하기 위한 장치)
/// </summary>
public sealed class CppPortMapTests
{
    /// <summary>허용하는 상태 이름. L1 은 원본 fixture 대조, session 은 세션 연결, port 는 새 경로 직역 이식, retire 는 대체 예정이다.</summary>
    private static readonly string[] AllowedStatus = ["L1", "session", "port", "retire"];

    /// <summary>테스트 출력 폴더에서 위로 올라가며 dotnetpj/PORT_MAP.tsv 가 있는 저장소 루트를 찾는다. 못 찾으면 예외로 실패한다.</summary>
    private static string FindRoot()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        // 폴더를 한 단계씩 올라가며 포트 맵 파일을 찾는다.
        while (directory != null)
        {
            if (File.Exists(Path.Combine(directory.FullName, "dotnetpj", "PORT_MAP.tsv"))) return directory.FullName;
            directory = directory.Parent;
        }
        throw new DirectoryNotFoundException("dotnetpj/PORT_MAP.tsv 가 있는 저장소 루트를 찾지 못했다.");
    }

    /// <summary>포트 맵의 데이터 행(주석·빈 줄 제외)을 열 4개로 나눠 돌려준다. 열 수가 틀린 행은 예외로 알려 조용히 건너뛰지 않는다.</summary>
    private static List<string[]> Rows()
    {
        var rows = new List<string[]>();
        // 줄마다 주석과 데이터 행을 구분한다.
        foreach (string line in File.ReadLines(Path.Combine(FindRoot(), "dotnetpj", "PORT_MAP.tsv")))
        {
            if (line.Length == 0 || line[0] == '#') continue;
            string[] cell = line.Split('\t');
            if (cell.Length != 4) throw new InvalidDataException($"PORT_MAP.tsv: 열이 4개가 아닌 행 '{line}'");
            rows.Add(cell);
        }
        return rows;
    }

    /// <summary>포트 맵에 행이 하나 이상 있고 같은 (cpppj, C#) 쌍이 두 번 나오지 않는다.</summary>
    [Fact]
    public void PortMapHasUniquePairs()
    {
        var rows = Rows();
        Assert.NotEmpty(rows);
        var seen = new HashSet<string>(StringComparer.Ordinal);
        // 행마다 (cpppj, C#) 쌍이 처음 나온 것인지 확인한다.
        foreach (string[] cell in rows) Assert.True(seen.Add(cell[0] + "|" + cell[1]), $"중복 행: {cell[0]} -> {cell[1]}");
    }

    /// <summary>모든 행의 cpppj 소스와 C# 파일이 실제로 있고, 기준 커밋이 40자리 16진수이며, 상태가 허용 목록 안에 있다.</summary>
    [Fact]
    public void PortMapRowsPointToExistingFiles()
    {
        string root = FindRoot();
        // 행마다 두 경로, 기준 커밋, 상태를 검사한다.
        foreach (string[] cell in Rows())
        {
            Assert.True(File.Exists(Path.Combine(root, cell[0].Replace('/', Path.DirectorySeparatorChar))), $"cpppj 파일 없음: {cell[0]}");
            Assert.True(File.Exists(Path.Combine(root, "dotnetpj", "src", cell[1].Replace('/', Path.DirectorySeparatorChar))), $"C# 파일 없음: {cell[1]}");
            Assert.Matches("^[0-9a-f]{40}$", cell[2]);
            Assert.Contains(cell[3], AllowedStatus);
        }
    }
}
