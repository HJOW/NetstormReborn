using System.Globalization;
using System.Text;

namespace Netstorm.Tests;

/// <summary>C++와 C#이 같은 원본 기계어 기대값을 읽도록 하는 TSV 판독기.</summary>
internal static class X86Fixture
{
    /// <summary>빈 버퍼를 나타내는 기대값 표기.</summary>
    private const string Empty = "-";

    /// <summary>주석을 제외한 행을 읽는다. 자료가 없으면 검사를 생략하지 않고 실패한다.</summary>
    public static string[][] Read(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name + "-x86.tsv"))
        .Where(line => line.Length > 0 && !line.StartsWith('#')).Select(line => line.Split('\t')).ToArray();

    /// <summary>16진수 칸을 손실 없는 바이트 배열로 바꾼다.</summary>
    public static byte[] Bytes(string value) => value == Empty ? [] : Convert.FromHexString(value);

    /// <summary>TSV의 원본 바이트를 일대일 문자로 바꾼다. 기대값의 비ASCII 바이트도 보존한다.</summary>
    public static string Text(string value) => Encoding.Latin1.GetString(Bytes(value));

    /// <summary>부호 있는 십진수 칸을 읽는다.</summary>
    public static int Int(string value) => int.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>부호 없는 십진수 칸을 읽는다.</summary>
    public static uint UInt(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>정수로 저장한 단정밀도 좌표의 비트를 그대로 복원한다.</summary>
    public static float FloatBits(string value) => BitConverter.UInt32BitsToSingle(UInt(value));

    /// <summary>모든 입력을 끝까지 대조해 실패 개수와 대표 입력을 함께 보고한다.</summary>
    public static void CheckRows(string[][] rows, Func<string[], bool> matches)
    {
        var failures = new List<string>();
        // 한 실패 때문에 뒤의 입력 검사를 건너뛰지 않는다.
        for (int i = 0; i < rows.Length; i++)
        {
            try
            {
                if (!matches(rows[i])) failures.Add($"행 {i + 1}: {string.Join(' ', rows[i].Select(cell => cell.Length > 80 ? cell[..80] + "…" : cell))}");
            }
            catch (Exception error)
            {
                failures.Add($"행 {i + 1}: {error.GetType().Name}: {error.Message}");
            }
        }
        Xunit.Assert.True(failures.Count == 0, $"원본 입력 {rows.Length}개 중 {failures.Count}개 불일치\n" + string.Join('\n', failures.Take(8)));
    }
}
