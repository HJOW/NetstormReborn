using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Assets.Tests;

/// <summary>
/// 원본 기계어 fixture(config-x86.tsv: 2440행)로 설정 객체 층의 L1 일치를 검사한다.
/// 근거: cpppj/tests/ConfigOracleTests.cpp. 기대값은 cpppj가 원본 exe를 격리 실행해 얻은 값이며 C# 결과로 바꾸지 않는다.
/// (R3k-10, 2026-10-30)
/// fixture 소비자(Assets) 규칙에 따라 이 검사는 Assets.Tests에 두며 대상 클래스(Netstorm.Core.Original.Config)만 Core에서 빌린다.
/// </summary>
public sealed class X86ConfigObjectTests
{
    /// <summary>Def 줄을 뺀 데이터 행 수.</summary>
    private const int Rows = 2440;

    /// <summary>원본 테스트와 같은 고정 환경 변수. 실제 환경에 의존하지 않는다.</summary>
    private static readonly Dictionary<string, string> OracleEnvironment = new()
    {
        ["nsenv"] = "env{A}val",
        ["nsempty"] = "",
    };

    /// <summary>ASCII 대문자만 소문자로 바꾼다(원본 AsciiLower와 같다). 검사 하네스의 환경 조회에만 쓴다.</summary>
    private static string AsciiLower(string text)
    {
        // ASCII 대문자만 소문자로 바꾼다
        char[] chars = text.ToCharArray();
        for (int i = 0; i < chars.Length; i++)
        {
            if (chars[i] >= 'A' && chars[i] <= 'Z')
            {
                chars[i] = (char)(chars[i] + ('a' - 'A'));
            }
        }
        return new string(chars);
    }

    /// <summary>원본 테스트의 Stack 묶음. 등록 순서가 같은 객체 목록과 입력 구성 여부를 함께 둔다.</summary>
    private sealed class OracleStack : IDisposable
    {
        /// <summary>설정 객체를 조회하는 등록부.</summary>
        public ConfigRegistry Registry { get; } = new();

        /// <summary>현재 테스트가 만든 집합 계열의 객체 목록.</summary>
        public List<Config> Objects { get; } = [];

        /// <summary>다 쓴 객체를 등록부에서 뺀다.</summary>
        public void Dispose()
        {
            // 뒤에서부터 등록을 푼다
            for (int i = Objects.Count - 1; i >= 0; i--)
            {
                Objects[i].Dispose();
            }
        }
    }

    /// <summary>TSV 한 줄을 공백(공백·탭) 단위로 나눈다. istringstream &gt;&gt; 과 같이 빈 칸은 생기지 않는다.</summary>
    private static string[] SplitRow(string line) =>
        line.Split([' ', '\t'], StringSplitOptions.RemoveEmptyEntries);

    /// <summary>fixture 16진 칸을 원본 바이트 문자열로 바꾼다. '-'는 빈 문자열이다.</summary>
    private static string Unhex(string value) => X86Fixture.Text(value);

    /// <summary>행마다 종류대로 같은 입력을 실행해 원본 기대값과 대조한다.</summary>
    [Fact]
    public void ConfigObjectLayerMatchesNative()
    {
        string path = Path.Combine(CppFixtureManifest.X86Directory, "config-x86.tsv");
        string[] lines = File.ReadLines(path)
            .Where(line => line.Length > 0 && !line.StartsWith('#'))
            .ToArray();
        var defined = new Dictionary<string, string>(StringComparer.Ordinal);
        int count = 0;
        var failures = new List<string>();
        // 한 행의 실패가 뒤 행의 검사를 막지 않는다
        foreach (string line in lines)
        {
            string[] row = SplitRow(line);
            try
            {
                if (row[0] == "Def")
                {
                    defined[row[1]] = Unhex(row[2]);
                    continue;
                }
                CheckRow(row, defined);
            }
            catch (Exception error)
            {
                failures.Add($"행 {count + 1}({row[0]}): {error.GetType().Name}: {error.Message}");
            }
            count++;
        }
        Assert.True(failures.Count == 0, $"원본 입력 {count}개 중 {failures.Count}개 불일치\n" + string.Join('\n', failures.Take(8)));
        Assert.Equal(Rows, count);
    }

    /// <summary>종류별로 같은 객체 묶음을 세우고 같은 호출을 실행한다.</summary>
    private static void CheckRow(string[] row, Dictionary<string, string> defined)
    {
        switch (row[0])
        {
            case "Subst":
            case "Get":
                CheckLookup(row, defined);
                break;
            case "Remove":
                CheckRemove(row);
                break;
            case "Set":
                CheckSet(row, defined);
                break;
            case "FindSection":
                CheckFindSection(row);
                break;
            case "Section":
                CheckSection(row);
                break;
            default:
                throw new InvalidDataException($"모르는 설정 oracle 종류 {row[0]}");
        }
    }

    /// <summary>개수·이름·버퍼·환경 묶음을 읽어 같은 순서로 객체를 만든다.</summary>
    private static OracleStack ReadStack(string[] row, int start, Dictionary<string, string> defined)
    {
        var stack = new OracleStack();
        stack.Registry.Environment = name =>
        {
            // ASCII 소문자로 견주어 고정 값을 찾는다
            string lowered = AsciiLower(name);
            return OracleEnvironment.TryGetValue(lowered, out string? value) ? value : null;
        };
        int count = X86Fixture.Int(row[start]);
        // 객체는 등록 순서대로 만든다
        for (int i = 0; i < count; i++)
        {
            string name = row[start + 1 + i * 3];
            string text = row[start + 2 + i * 3];
            bool environment = row[start + 3 + i * 3] != "0";
            string decodedName = name == "~" ? "" : Unhex(name);
            Config config = name == "~" ? new Config(stack.Registry) : new Config(stack.Registry, decodedName);
            if (text != "~")
            {
                config.Assign(text[0] == '@' ? defined[text.Substring(1)] : Unhex(text));
            }
            config.EnvironmentEnabled = environment;
            stack.Objects.Add(config);
        }
        return stack;
    }

    /// <summary>치환·조회 행을 같은 객체·입력으로 실행한다.</summary>
    private static void CheckLookup(string[] row, Dictionary<string, string> defined)
    {
        using OracleStack stack = ReadStack(row, 2, defined);
        int self = X86Fixture.Int(row[1]);
        if (row[0] == "Subst")
        {
            string actual = stack.Objects[self].Substitute(Unhex(row[^2]));
            Assert.Equal(Unhex(row[^1]), actual);
        }
        else
        {
            int end = 3 + X86Fixture.Int(row[2]) * 3;
            string? value = stack.Objects[self].Get(Unhex(row[end]));
            Assert.Equal(X86Fixture.Int(row[end + 1]) != 0, value != null);
            Assert.Equal(Unhex(row[end + 2]), value ?? "");
        }
    }

    /// <summary>키 삭제 행을 같은 버퍼로 실행한다.</summary>
    private static void CheckRemove(string[] row)
    {
        var registry = new ConfigRegistry();
        using var config = new Config(registry);
        config.Assign(Unhex(row[1]));
        Assert.Equal(X86Fixture.Int(row[3]) != 0, config.RemoveKey(Unhex(row[2])));
        Assert.Equal(Unhex(row[4]), config.Text);
    }

    /// <summary>값 쓰기 행을 같은 묶음으로 실행한다.</summary>
    private static void CheckSet(string[] row, Dictionary<string, string> defined)
    {
        using OracleStack stack = ReadStack(row, 2, defined);
        int self = X86Fixture.Int(row[1]);
        int fields = 3 + X86Fixture.Int(row[2]) * 3;
        Assert.Equal(X86Fixture.Int(row[fields + 2]) != 0,
            stack.Objects[self].Set(Unhex(row[fields]), Unhex(row[fields + 1])));
        // 변경 표시는 고른 타깃 객체에만 켜진다
        int target = X86Fixture.Int(row[fields + 3]);
        for (int i = 0; i < stack.Objects.Count; i++)
        {
            Assert.Equal(i == target, stack.Objects[i].Changed);
        }
        // 버퍼 뒤에는 원본 "%s = \"%s\"" 줄과 CRLF가 붙는다
        string appended = Unhex(row[fields + 4]) + " = \"" + Unhex(row[fields + 5]) + "\"\r\n";
        string head = row[fields + 6] == "~" ? "" : Unhex(row[fields + 6]);
        Assert.Equal(head + appended, stack.Objects[target].Text);
    }

    /// <summary>섹션 머리 찾기 행을 같은 버퍼로 실행한다.</summary>
    private static void CheckFindSection(string[] row)
    {
        string text = Unhex(row[1]);
        string header = Unhex(row[2]);
        bool wantName = X86Fixture.Int(row[3]) != 0;
        string? matchedName = null;
        int? found = wantName
            ? Config.FindSection(text, 0, header, out matchedName)
            : Config.FindSection(text, 0, header);
        Assert.Equal(X86Fixture.Int(row[4]), found ?? -1);
        if (found != null && wantName)
        {
            Assert.Equal(Unhex(row[5]), matchedName);
        }
    }

    /// <summary>섹션 본문 행을 같은 버퍼로 실행한다. 위치는 같은 머리 찾기로 구한다.</summary>
    private static void CheckSection(string[] row)
    {
        var registry = new ConfigRegistry();
        using var config = new Config(registry);
        string buffer = Unhex(row[1]);
        string name = Unhex(row[2]);
        config.Assign(buffer);
        string? body = config.Section(name);
        string header = name.Length != 0 && name[0] == '[' ? name : "[" + name + "]";
        int? start = Config.FindSection(buffer, 0, header);
        if (body == null)
        {
            Assert.Null(start);
            Assert.Equal(-1, X86Fixture.Int(row[3]));
            Assert.Equal(0, X86Fixture.Int(row[4]));
        }
        else
        {
            Assert.Equal(X86Fixture.Int(row[3]), start);
            Assert.Equal(X86Fixture.Int(row[4]), body.Length);
        }
    }
}
