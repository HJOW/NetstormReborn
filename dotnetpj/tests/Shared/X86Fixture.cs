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

    /// <summary>부호 있는 64비트 십진수 칸을 읽는다. 원본의 64비트 누적값·시각 틱에 쓴다 (W01 확장, 2026-10-16).</summary>
    public static long Long(string value) => long.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>부호 없는 64비트 십진수 칸을 읽는다. double 비트 패턴처럼 0x8000... 이상인 값도 잃지 않는다 (W01 확장).</summary>
    public static ulong ULong(string value) => ulong.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>정수로 저장한 단정밀도 좌표의 비트를 그대로 복원한다.</summary>
    public static float FloatBits(string value) => BitConverter.UInt32BitsToSingle(UInt(value));

    /// <summary>64비트 십진수로 저장한 배정밀도 값의 비트 패턴을 그대로 복원한다. 원본 시계·시각 필드의 double 이 대상이다 (W01 확장).</summary>
    public static double DoubleBits(string value) => BitConverter.UInt64BitsToDouble(ULong(value));

    /// <summary>쉼표로 나눈 부호 없는 64비트 십진수 목록 칸을 읽는다. 빈 칸과 "-" 는 빈 목록이다 (W01 확장).</summary>
    public static ulong[] ULongs(string cell) => cell.Length == 0 || cell == Empty ? [] : cell.Split(',').Select(ULong).ToArray();

    /// <summary>첫 칸이 <paramref name="label"/> 인 행(한 파일 안의 한 절)만 고른다. 여러 절이 섞인 fixture 에 쓴다 (W01 확장).</summary>
    public static string[][] Section(string[][] rows, string label) => rows.Where(row => row[0] == label).ToArray();

    /// <summary>fixture 를 읽어 첫 칸이 <paramref name="label"/> 인 절만 돌려준다 (W01 확장).</summary>
    public static string[][] Read(string name, string label) => Section(Read(name), label);

    /// <summary>행에 나온 절 이름과 행 수를 첫 등장 순서로 돌려준다. 절 구성이 바뀌었는지 확인하는 데 쓴다 (W01 확장).</summary>
    public static IReadOnlyList<(string Label, int Count)> Sections(string[][] rows) => rows
        .GroupBy(row => row[0]).Select(group => (group.Key, group.Count())).ToArray();

    /// <summary>Adler-32 의 나머지 연산 값과, 나머지를 미뤄도 32비트가 넘치지 않는 최대 묶음 길이.</summary>
    private const uint AdlerPrime = 65521, AdlerBlock = 5552;

    /// <summary>Python zlib 와 같은 Adler-32. 원본 풀 전체 바이트 같은 긴 버퍼를 기대값과 대조할 때 쓴다(묶음마다 한 번만 나머지를 구한다).</summary>
    public static uint Adler32(ReadOnlySpan<byte> bytes)
    {
        uint a = 1;
        uint b = 0;
        // 묶음 단위로 누적한다
        while (bytes.Length > 0)
        {
            int length = (int)Math.Min(AdlerBlock, (uint)bytes.Length);
            // 한 묶음의 바이트
            foreach (byte value in bytes[..length])
            {
                a += value;
                b += a;
            }
            a %= AdlerPrime;
            b %= AdlerPrime;
            bytes = bytes[length..];
        }
        return (b << 16) | a;
    }

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
