using System.Security.Cryptography;
using System.Text;

namespace Netstorm.Tests;

/// <summary>SHA-256 목록(cpp-fixture-manifest.tsv)의 한 줄. 파일 이름·SHA-256·주석 제외 행 수·출처·사용 프로젝트다.</summary>
/// <param name="File">fixture 파일 이름 (예: sid-x86.tsv).</param>
/// <param name="Sha256">CRLF 를 LF 로 바꾼 바이트의 SHA-256 소문자 16진수.</param>
/// <param name="Rows">주석('#')과 빈 줄을 뺀 행 수. X86Fixture.Read 가 돌려주는 행 수와 같다.</param>
/// <param name="Origin">출처. cpppj 는 읽기 전용 기대값, dotnetpj 는 이 저장소가 직접 소유한 관찰 자료다.</param>
/// <param name="Consumer">이 fixture 를 읽는 테스트 프로젝트 이름 (Core 또는 Assets).</param>
internal sealed record CppFixtureEntry(string File, string Sha256, int Rows, string Origin, string Consumer);

/// <summary>
/// 연결한 원본 기계어 fixture 의 SHA-256 목록을 읽고 파일 내용과 비교하는 도구.
/// cpppj 가 fixture 를 다시 만들면 목록과 달라져 검사가 실패하고, 그때 해당 C# 검사를 재대조하도록 요구한다 (W01 동기화 장치, 2026-10-16).
/// 목록 갱신은 dotnetpj/tools/cpp_sync.py 가 같은 계산으로 한다.
/// </summary>
internal static class CppFixtureManifest
{
    /// <summary>테스트 출력의 X86 폴더에 복사되는 목록 파일 이름.</summary>
    public const string FileName = "cpp-fixture-manifest.tsv";

    /// <summary>목록 파일에서 '반영한 cpppj 커밋'을 적는 주석 머리말.</summary>
    private const string ReflectedKey = "# reflected-commit:";

    /// <summary>fixture 와 목록이 복사되는 테스트 출력 폴더.</summary>
    public static string X86Directory => Path.Combine(AppContext.BaseDirectory, "X86");

    /// <summary>현재 테스트 프로젝트의 사용 프로젝트 이름. 어셈블리 이름 Netstorm.&lt;이름&gt;.Tests 에서 읽는다.</summary>
    public static string Consumer => typeof(CppFixtureManifest).Assembly.GetName().Name!.Split('.')[1];

    /// <summary>목록에 기록된 '반영한 cpppj 커밋'(40자리 16진수). 기록이 없으면 빈 문자열이다.</summary>
    public static string ReflectedCommit => Load().Reflected;

    /// <summary>목록의 모든 항목. 목록 파일이 없으면 검사를 생략하지 않고 예외로 실패한다.</summary>
    public static IReadOnlyList<CppFixtureEntry> Entries => Load().Entries;

    /// <summary>목록 파일을 읽어 (반영 커밋, 항목들)로 나눈다. 열 수가 틀린 행은 예외로 알려 조용히 건너뛰지 않는다.</summary>
    private static (string Reflected, CppFixtureEntry[] Entries) Load()
    {
        string reflected = "";
        var entries = new List<CppFixtureEntry>();
        // 줄마다 머리말 주석, 일반 주석, 데이터 행을 구분한다.
        foreach (string line in File.ReadLines(Path.Combine(X86Directory, FileName)))
        {
            if (line.StartsWith(ReflectedKey, StringComparison.Ordinal)) reflected = line[ReflectedKey.Length..].Trim();
            else if (line.Length > 0 && line[0] != '#')
            {
                string[] cell = line.Split('\t');
                if (cell.Length != 5) throw new InvalidDataException($"{FileName}: 열이 5개가 아닌 행 '{line}'");
                entries.Add(new CppFixtureEntry(cell[0], cell[1], int.Parse(cell[2]), cell[3], cell[4]));
            }
        }
        return (reflected, entries.ToArray());
    }

    /// <summary>CRLF 를 LF 로 바꾼 바이트의 SHA-256 을 소문자 16진수로 돌려준다. Windows 체크아웃의 줄바꿈 차이가 섞이지 않는다.</summary>
    public static string Sha256Lf(byte[] data) => Convert.ToHexStringLower(SHA256.HashData(NormalizeLineEndings(data)));

    /// <summary>주석('#')과 빈 줄을 뺀 행 수를 센다. 줄 구분은 LF 와 CRLF 를 모두 받는다.</summary>
    public static int CountRows(byte[] data)
    {
        int rows = 0;
        // 정규화한 바이트를 줄 단위로 나눠 데이터 행만 센다.
        foreach (string line in Encoding.Latin1.GetString(NormalizeLineEndings(data)).Split('\n'))
        {
            if (line.Length > 0 && line[0] != '#') rows++;
        }
        return rows;
    }

    /// <summary>CRLF(0D 0A) 쌍의 CR 만 제거한다. 단독 CR 은 데이터이므로 건드리지 않는다.</summary>
    private static byte[] NormalizeLineEndings(byte[] data)
    {
        var result = new List<byte>(data.Length);
        // 바이트마다 0D 0A 쌍의 앞쪽 0D 를 건너뛴다.
        for (int i = 0; i < data.Length; i++)
        {
            if (data[i] == 0x0D && i + 1 < data.Length && data[i + 1] == 0x0A) continue;
            result.Add(data[i]);
        }
        return result.ToArray();
    }
}
