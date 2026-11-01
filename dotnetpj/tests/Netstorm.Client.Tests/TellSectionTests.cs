using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>원본 Tell 매개변수와 섹션 이력·실제 Config 수명을 검사한다.</summary>
public sealed class TellSectionTests
{
    /// <summary>원본 BYTE 문자열을 fixture의 소문자 hex로 바꾼다.</summary>
    private static string Hex(string text) => Convert.ToHexStringLower(Encoding.Latin1.GetBytes(text));
    /// <summary>현재 배열 전체의 원본 FNV 지문이다.</summary>
    private static string Fingerprint(byte[] bytes)
    {
        uint value = 2166136261;
        // NUL 뒤 미수정 영역까지 포함한다.
        foreach (byte b in bytes) value = unchecked((value ^ b) * 16777619);
        return value.ToString("x8");
    }

    /// <summary>원본 858행의 선택 순서, 부분 쓰기와 전체 상태를 비교한다.</summary>
    [Fact]
    public void SectionMatchesNativeFixture()
    {
        string[][] rows = X86Fixture.Read("tell-section"); Assert.Equal(858, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            string target = X86Fixture.Text(row[1]), prior = X86Fixture.Text(row[2]); uint mask = X86Fixture.UInt(row[3]);
            var state = new TellSectionState(); Array.Fill(state.Current, (byte)'Z');
            Encoding.Latin1.GetBytes(prior, state.Current); state.Current[prior.Length] = 0;
            bool available = (mask & 4) != 0; var trace = new List<string>();
            string? Body(bool present, string name) => present ? ((mask & 32) != 0 ? "" : name) : null;
            string? result = TellSection.Find(state, target, new TellSectionHooks
            {
                Prior = value => { trace.Add("P," + Hex(value)); if ((mask & 64) != 0) { available = !available; Encoding.Latin1.GetBytes("changed", state.Current); state.Current[7] = 0; } },
                Named = (prefix, section) => { trace.Add("N," + Hex(prefix) + "," + Hex(section)); return Body((mask & 3) == 3, "named"); },
                Mission = value => { if (!available) return null; trace.Add("M," + Hex(value)); return Body((mask & 8) != 0, "mission"); },
                Fallback = value => { trace.Add("F," + Hex(value)); return Body((mask & 16) != 0, "fallback"); }
            });
            int end = Array.IndexOf(state.Current, (byte)0);
            string actual = Hex(Encoding.Latin1.GetString(state.Current, 0, end)) + "|" + Fingerprint(state.Current) + "|" +
                (result is null ? "-" : Hex(result)) + "|" + string.Join(';', trace);
            return actual == row[4];
        });
    }

    /// <summary>원본 1070행의 토큰과 순차 치환·Append 효과를 비교한다.</summary>
    [Fact]
    public void ParametersMatchNativeFixture()
    {
        string[][] rows = X86Fixture.Read("tell-parameters"); Assert.Equal(1070, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint profile = X86Fixture.UInt(row[2]); var values = new Dictionary<uint, string>(); var trace = new List<string>();
            string name = TellParameters.Parse(X86Fixture.Text(row[1]), text =>
            {
                trace.Add("S," + Hex(text));
                return profile switch { 1 => "[" + text + "]", 2 when text == "{param.1}" => values.GetValueOrDefault(1u, ""),
                    3 when text == "EMPTY" => "", 3 => "Q\";=\n" + text, 4 => "N\0TAIL", _ => text };
            }, line =>
            {
                trace.Add("A," + Hex(line)); int equal = line.IndexOf('=');
                values[uint.Parse(line[..equal])] = line[(equal + 2)..^1];
            });
            return Hex(name) + "|" + string.Join(';', trace) == row[3];
        });
    }

    /// <summary>등록 객체의 빈 본문도 미션보다 우선하고 미등록 접두는 미션으로 넘긴다.</summary>
    [Fact]
    public void NamedEmptySectionAndMissionPriorityUseActualRegistry()
    {
        int reads = 0, missions = 0; var config = new ConfigInterface(_ => { reads++; return null; });
        config.Configuration.Assign("missionSpec=d/{local.1}.english\n");
        using var file = new Config(config.Registry, "file"); file.Assign("[empty]\n[part]\nNamed\n[END]\n");
        var state = new TellSectionState();
        string? Mission(string name) { missions++; return name == "missing.part" ? "Mission" : null; }
        Assert.Equal("", TellSection.FindInConfig(state, "file.empty", config, Mission));
        Assert.Equal("Named\n", TellSection.FindInConfig(state, "file.part", config, Mission));
        Assert.Equal("Mission", TellSection.FindInConfig(state, "missing.part", config, Mission));
        Assert.Equal(1, missions); Assert.Equal(0, reads);
        Assert.Equal((false, 0u), TellSection.ShowInConfig(state, "missing", 0, config, null, null));
    }

    /// <summary>동일 등록부의 param 치환, 임시 파일 수명과 누락 이름 이력을 검사한다.</summary>
    [Fact]
    public void ActualConfigKeepsParametersDuringShowAndReloadsFallback()
    {
        int reads = 0; var config = new ConfigInterface(path => { Assert.Equal("d/tell.english", path); reads++; return Encoding.ASCII.GetBytes("[SETSIZE]\n{param.1},{param.2}\n[END]\n"); });
        config.Configuration.Assign("missionSpec=d/{local.1}.english\nwidth=800\n");
        using var outer = new Config(config.Registry, "param"); outer.Append("1=outer"); int before = config.Registry.Objects.Count;
        var state = new TellSectionState();
        var result = TellSection.ShowInConfig(state, "SETSIZE({width},{param.1})", 0, config, null,
            (body, suppress, width, height) => { Assert.Equal("800,800\n", config.SubstituteLocal(body)); Assert.Equal(before + 1, config.Registry.Objects.Count); return 42; });
        Assert.Equal((true, 42u), result); Assert.Equal(before, config.Registry.Objects.Count); Assert.Equal("outer", config.SubstituteLocal("{param.1}"));
        Assert.Null(TellSection.FindInConfig(state, "missing", config, null)); Assert.Equal("SETSIZE", config.SubstituteLocal("{global.priorTell}")); Assert.Equal(2, reads);
    }
}
