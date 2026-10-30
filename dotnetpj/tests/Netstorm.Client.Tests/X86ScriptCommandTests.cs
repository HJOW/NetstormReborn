using System.Text;
using Netstorm.Client.Input;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture(script-command-x86.tsv: 262행)로 스크립트 명령·Config·문자열 고리의 L1 일치를 검사한다.
/// 근거: cpppj/tests/ScriptCommandTests.cpp. 기대값은 cpppj가 원본 exe를 격리 실행해 얻은 값이며 C# 결과로 바꾸지 않는다. (R3k-9, 2026-10-30)
/// </summary>
public sealed class X86ScriptCommandTests
{
    /// <summary>fixture 데이터 행 수(머릿행 제외).</summary>
    private const int Rows = 262;

    /// <summary>ASCII 대문자만 소문자로 내린다(원본 AsciiLower와 같다). 검사 하네스의 표 조회에만 쓴다.</summary>
    private static string AsciiLower(string text)
    {
        // A-Z만 소문자로 바꾼다
        var output = new StringBuilder(text.Length);
        foreach (char c in text)
        {
            output.Append(c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c);
        }
        return output.ToString();
    }

    /// <summary>원본 토큰·설정·명령·고리 재생이 모든 행에서 같은 결과·큐·진단을 낸다.</summary>
    [Fact]
    public void ScriptCommandsMatchNative()
    {
        string[][] rows = X86Fixture.Read("script-command");
        int count = 0;
        var continuous = new ScriptStringRing();
        var tokens = new List<uint>();
        var stored = new List<string>();
        // 행마다 연산 종류대로 같은 입력으로 실행해 기대와 대조한다
        foreach (string[] row in rows)
        {
            if (row[0] == "operation")
            {
                continue;
            }
            Assert.Equal(9, row.Length);
            string source = X86Fixture.Text(row[1]);
            switch (row[0])
            {
                case "Token":
                    CheckToken(source, row);
                    break;
                case "Config":
                    CheckConfig(source, row);
                    break;
                case "Command":
                    CheckCommand(source, row);
                    break;
                case "Ring":
                    CheckRing(source, row, continuous, tokens, stored);
                    break;
                default:
                    Assert.Fail($"알 수 없는 스크립트 연산 {row[0]}");
                    break;
            }
            count++;
        }
        Assert.Equal(Rows, count);
    }

    /// <summary>같은 구분자로 자른 본문과 다음 위치가 같다.</summary>
    private static void CheckToken(string source, string[] row)
    {
        string delimiter = X86Fixture.Text(row[2]);
        RawTextTokenResult token = delimiter == "DEFAULT"
            ? RawTextToken.Read(source)
            : RawTextToken.Read(source, 0, delimiter);
        Assert.Equal(X86Fixture.Text(row[4]), token.Text);
        Assert.Equal(X86Fixture.Int(row[5]), token.Next);
    }

    /// <summary>같은 "키=값" 입력이 같은 쓰기·없음 알림·반환을 낸다.</summary>
    private static void CheckConfig(string source, string[] row)
    {
        var assignments = new List<(string Key, string Value)>();
        int missing = 0;
        int actual = ScriptCommand.ApplyScriptConfig(
            source,
            (key, value) =>
            {
                assignments.Add((key, value));
                return row[3] == "1";
            },
            () => missing++);
        Assert.Equal(X86Fixture.Int(row[4]), actual);
        bool found = false;
        // 설정 쓰기·키/값 쌍·없음 알림을 실제 Config처럼 확인한다
        foreach (string ev in row[8].Split(';'))
        {
            if (!ev.StartsWith("S,", StringComparison.Ordinal))
            {
                continue;
            }
            string[] values = ev.Split(',');
            Assert.Equal(3, values.Length);
            Assert.Single(assignments);
            Assert.Equal(X86Fixture.Text(values[1]), assignments[0].Key);
            Assert.Equal(X86Fixture.Text(values[2]), assignments[0].Value);
            found = true;
        }
        Assert.Equal(found ? 1 : 0, assignments.Count);
        Assert.Equal(found ? 0 : 1, missing);
    }

    /// <summary>같은 "명령,인자" 입력이 같은 큐 사건·고리 위치·진단을 낸다.</summary>
    private static void CheckCommand(string source, string[] row)
    {
        var queue = new InputQueue();
        var strings = new ScriptStringRing();
        var diagnostics = new List<int>();
        var hooks = new ScriptCommandHooks
        {
            Lookup = name =>
            {
                Assert.Equal(0, strings.Index);
                string lower = AsciiLower(name);
                return lower == "config" ? (row[3] == "1" ? 0u : ScriptCommand.ScriptConfig)
                    : lower == "warning" ? ScriptCommand.ScriptWarning : 0xffffffffu;
            },
            Unknown = _ => diagnostics.Add(strings.Index),
        };
        ScriptCommand.QueueScriptCommand(source, queue, strings, hooks);
        Assert.Equal(X86Fixture.Int(row[7]), strings.Index);
        if (row[6] == "-")
        {
            Assert.True(queue.Empty);
        }
        else
        {
            Assert.False(queue.Empty);
            InputEvent actual = queue.Pop(false);
            int comma = row[6].IndexOf(',');
            Assert.Equal(X86Fixture.UInt(row[6][..comma]), actual.Code);
            Assert.Equal(actual.X, actual.Y);
            string expected = row[6][(comma + 1)..];
            if (expected[0] == 's')
            {
                Assert.Equal(X86Fixture.Text(expected[1..]), strings.Resolve(unchecked((uint)actual.X)));
            }
            else
            {
                Assert.Equal(X86Fixture.UInt(expected[1..]), unchecked((uint)actual.X));
            }
            Assert.True(queue.Empty);
        }
        // 'L' 글자 수가 진단 호출 수다
        int logCount = 0;
        foreach (char c in row[8])
        {
            if (c == 'L')
            {
                logCount++;
            }
        }
        Assert.Equal(logCount, diagnostics.Count);
        if (diagnostics.Count != 0)
        {
            Assert.Equal(row[3] == "1" ? 0 : strings.Index, diagnostics[0]);
        }
    }

    /// <summary>같은 문자열 저장이 같은 해석·위치를 내고 80칸 밖 옛 토큰만 무효화한다.</summary>
    private static void CheckRing(string source, string[] row, ScriptStringRing continuous, List<uint> tokens, List<string> stored)
    {
        uint token = continuous.Store(source);
        tokens.Add(token);
        stored.Add(source);
        Assert.Equal(X86Fixture.Text(row[4]), continuous.Resolve(token));
        Assert.Equal(X86Fixture.Int(row[7]), continuous.Index);
        // 80칸을 넘긴 오래된 토큰만 무효화된다
        for (int i = 0; i < tokens.Count; i++)
        {
            bool rejected = false;
            try
            {
                Assert.Equal(stored[i], continuous.Resolve(tokens[i]));
            }
            catch (ArgumentOutOfRangeException)
            {
                rejected = true;
            }
            Assert.Equal(tokens.Count - i > 80, rejected);
        }
    }
}
