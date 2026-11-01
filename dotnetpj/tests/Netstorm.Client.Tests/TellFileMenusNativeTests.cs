using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests;

/// <summary>실제 Config 등록·파일 목록·cur 수명과 문자열 고리를 원본 관측과 대조한다.</summary>
public sealed class TellFileMenusNativeTests
{
    /// <summary>원본 바이트 표현을 소문자 16진수로 기록한다.</summary>
    private static string Hex(string value) => Convert.ToHexString(Encoding.Latin1.GetBytes(value)).ToLowerInvariant();
    /// <summary>경로의 마지막 구분자와 첫 점 사이 이름을 원본처럼 얻는다.</summary>
    private static string Name(string path) { int at = path.LastIndexOfAny(['/', '\\', ':']); string name = path[(at + 1)..]; int dot = name.IndexOf('.'); return dot < 0 ? name : name[..dot]; }

    /// <summary>@/#의 모든 원본 행에서 적재 순서, 중복, 정렬, 항목과 현재 cur을 검증한다.</summary>
    [Fact]
    public void AllNativeFileMenusUseActualConfigAndRing()
    {
        string[][] rows = X86Fixture.Read("tell-file-menus"); Assert.Equal(384, rows.Length);
        // 각 관측은 새 등록부와 원본이 제공한 파일 열거 순서를 사용한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length); string[] paths = row[1].Length == 0 ? [] : row[1].Split(',').Select(X86Fixture.Text).ToArray();
            int profile = X86Fixture.Int(row[2]); var trace = new List<string>(); ConfigInterface? config = null;
            config = new ConfigInterface(path =>
            {
                string name = Name(path); Config loaded = config!.Registry.FindByName(name)!;
                trace.Add("L," + Hex(name) + "," + Hex(path) + "," + loaded.GetRaw("index"));
                string title = profile == 1 && name.Equals("a", StringComparison.OrdinalIgnoreCase) ? "" :
                    profile == 1 && name.Equals("b", StringComparison.OrdinalIgnoreCase) ? "--divider" : "Title " + name;
                return Encoding.Latin1.GetBytes("title=\"" + title + "\"\n");
            });
            config.Configuration.Assign("ready=1\nmissionSpec={local.1}\n");
            using var existing = X86Fixture.Int(row[3]) != 0 ? new Config(config.Registry, "b") : null;
            existing?.Assign("title=Existing\nindex=99\n"); var strings = new ScriptStringRing();
            using var menus = new TellFileMenus(config, pattern => { Assert.Equal("input", pattern); return paths; });
            menus.Append("input", 0x02000095, X86Fixture.Text(row[4]), X86Fixture.Int(row[5]) != 0, strings, item =>
            {
                string current = config.SubstituteLocal("{cur.file}");
                trace.Add(item.Separator ? "S," + Hex(current) : "R," + Hex(item.Label) + "," + item.Command + "," + Hex(strings.Resolve(item.Parameter)) + "," + Hex(current));
            });
            Assert.Null(config.Registry.FindByName("cur")); Assert.Equal(row[6], string.Join(';', trace));
        }
    }

    /// <summary>% 목록의 두 번 치환, 오류 시 부분 출력과 다음 행 복구를 원본 전체 관측과 대조한다.</summary>
    [Theory]
    [InlineData("tell-inline-list", 288, false)]
    [InlineData("tell-inline-errors", 672, true)]
    public void AllNativeInlineMenusPreserveQueriesAndErrors(string fixture, int count, bool errors)
    {
        string[][] rows = X86Fixture.Read(fixture); Assert.Equal(count, rows.Length);
        // 각 관측에서 독립된 파일·설정·오류 시점을 사용한다.
        foreach (string[] row in rows)
        {
            string[] paths = row[1].Length == 0 ? [] : row[1].Split(',').Select(X86Fixture.Text).ToArray();
            int profile = X86Fixture.Int(row[2]), queries = 0, throwAt = errors ? X86Fixture.Int(row[4]) : 0;
            var trace = new List<string>(); ConfigInterface? config = null;
            config = new ConfigInterface(path =>
            {
                if (path.Equals("probe", StringComparison.OrdinalIgnoreCase))
                {
                    queries++; string current = Hex(config!.SubstituteLocal("{cur.file}")); trace.Add("Q," + current + "," + queries);
                    if (queries == throwAt) { trace.Add("E," + current + "," + queries); throw new InvalidOperationException("검사용 파일 조회 오류"); }
                    return Encoding.Latin1.GetBytes("value=" + queries + "\n");
                }
                string name = Name(path); Config loaded = config!.Registry.FindByName(name)!;
                trace.Add("L," + Hex(name) + "," + Hex(path) + "," + loaded.GetRaw("index"));
                string title = profile == 1 && name.Equals("a", StringComparison.OrdinalIgnoreCase) ? "" :
                    profile == 1 && name.Equals("b", StringComparison.OrdinalIgnoreCase) ? "--divider" : "Title " + name;
                return Encoding.Latin1.GetBytes("title=\"" + title + "\"\n");
            });
            config.Configuration.Assign("ready=1\nmissionSpec={local.1}\n");
            using var existing = X86Fixture.Int(row[3]) != 0 ? new Config(config.Registry, "b") : null;
            existing?.Assign("title=Existing\nindex=99\n");
            using var menus = new TellFileMenus(config, _ => paths);
            string html = menus.Inline("input", "TeLl", errors ? "{@probe.value}" : X86Fixture.Text(row[4]));
            Assert.Null(config.Registry.FindByName("cur")); Assert.Equal(row[5], string.Join(';', trace) + "|" + Hex(html));
        }
    }
}
