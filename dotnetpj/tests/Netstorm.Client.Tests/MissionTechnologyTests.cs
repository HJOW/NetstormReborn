using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>기술 목록과 같은 제한 표에 대한 독립 원본 관측을 비교한다.</summary>
public sealed class MissionTechnologyTests
{
    [Fact]
    public void UsesCurrentMissionConfigAndPreservesLastRestriction()
    {
        var files = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase)
        {
            ["d\\setup.cfg"] = Encoding.Latin1.GetBytes("DataDir=\"\\D\"\r\ncurrentLanguage=\"english\"\r\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\r\n"),
            ["\\D\\one.english"] = Encoding.Latin1.GetBytes("missionType=\"Tutorial\"\r\ntechAllowed=\"deny;all;allow;sunArcher\"\r\n")
        };
        byte[]? Read(string path) => files.GetValueOrDefault(path);
        var config = new ConfigInterface(Read); config.Startup(new ConfigInterface.StartupOptions());
        using var script = new Netstorm.Client.Mission.MissionScript(config, "one"); uint[] restrictions = new uint[256]; restrictions[255] = 0x7391;
        MissionTechnology.Apply(script, name => name == "sunArcher" ? 70 : -1, restrictions);
        Assert.Equal(0u, restrictions[70]); Assert.Equal(1u, restrictions[71]); Assert.Equal(0x7391u, restrictions[255]);
        script.Set("techAllowed", "missingType");
        Assert.Throws<ArgumentOutOfRangeException>(() => MissionTechnology.Apply(script, _ => -1, restrictions));
    }

    private static string Decode(string value) => value == "-" ? "" : Encoding.Latin1.GetString(Convert.FromHexString(value));
    private static string Visible(string value) { int n = value.IndexOf('\0'); return n < 0 ? value : value[..n]; }
    private static string Root()
    {
        DirectoryInfo? dir = new(AppContext.BaseDirectory);
        while (dir != null) { if (File.Exists(Path.Combine(dir.FullName, "originals", "Netstorm.exe"))) return dir.FullName; dir = dir.Parent; }
        throw new InvalidOperationException("보호 원본 저장소 경로 없음");
    }

    [Fact]
    public void MatchesNativeTechnologyLists()
    {
        var files = GameFileSystem.Open(Path.Combine(Root(), "originals")); var types = new TypeCatalog(files);
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "mission-technology-x86.tsv");
        string[] rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').ToArray(); Assert.Equal(55, rows.Length);
        foreach (string row in rows)
        {
            string[] columns = row.Split('\t'); string initial = Decode(columns[0]), changed = Decode(columns[1]); uint seed = uint.Parse(columns[2]);
            uint[] table = Enumerable.Range(0, 257).Select(i => ((uint)i * 7) ^ seed).ToArray(); var trace = new List<string>(); int lookups = 0;
            MissionTechnology.Apply(new MissionTechnologyHooks
            {
                Item = index => { trace.Add("G," + index); string current = changed.Length != 0 && lookups != 0 ? changed : initial; lookups++; return ConfigNumbers.ListItem(Visible(current), index); },
                FindType = name => types.Types.FirstOrDefault(type => string.Equals(type.RuntimeName, name, StringComparison.OrdinalIgnoreCase))?.RuntimeIndex ?? -1,
                AllowAll = allowed => { trace.Add("A," + allowed); Array.Fill(table, allowed == 0 ? 1u : 0u, 1, 255); },
                Allow = (type, allowed) => { trace.Add($"T,{unchecked((uint)type)},{allowed}"); table[type + 1] = allowed == 0 ? 1u : 0u; }
            });
            byte[] result = new byte[257 * 4]; for (int i = 0; i < table.Length; i++) BinaryPrimitives.WriteUInt32LittleEndian(result.AsSpan(i * 4), table[i]);
            Assert.Equal(columns[3], Convert.ToHexString(result).ToLowerInvariant()); Assert.Equal(columns[4], string.Join('|', trace));
        }
    }
}
