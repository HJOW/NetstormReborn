using System.Text;
using System.Text.Json;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>독립 x86 관측과 초기화의 현재 필드 및 효과 순서를 비교한다.</summary>
public sealed class TutorialInitTests
{
    private static string Hex(string text) => Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();

    [Fact]
    public void MatchesNativeInitialization()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "tutorial-init-x86.tsv");
        var rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').ToArray();
        Assert.Equal(152, rows.Length);
        foreach (string row in rows)
        {
            string[] columns = row.Split('\t');
            using JsonDocument doc = JsonDocument.Parse(columns[0]);
            JsonElement[] input = doc.RootElement.EnumerateArray().ToArray();
            string Text(int i) => input[i].ValueKind == JsonValueKind.String ? input[i].GetString()! : input[i].GetRawText();
            uint Number(int i) => uint.Parse(Text(i));
            var state = new TutorialInitState { Name = Text(0), Demo = Number(5), Flags = Number(6) };
            Array.Fill(state.Scratch, (byte)0xa5);
            var globals = new TutorialInitGlobals { TutorialNumber = 0x22222222, MissionNumber = 0x11111111, TutorialSeen = Number(7), LoginSubscriber = Number(8), InitialSubscriber = Number(9) };
            uint variant = Number(10); var trace = new List<string>();
            byte[] options = Enumerable.Range(0, 87).Select(i => unchecked((byte)(i * 11 + 17))).ToArray();
            TutorialInit.Initialize(state, globals, new TutorialInitHooks
            {
                ClearTemplates = () => { trace.Add("C"); if (variant == 1) state.Name = "AFTER_CLEAR"; },
                Get = (key, fallback) => { string value = Text(key switch { "tutorialNumber" => 1, "missionNumber" => 2, "demo" => 3, _ => 4 }); if (value.Length == 0 && fallback != null) value = fallback; trace.Add($"G,{key},{Hex(fallback ?? "")},{Hex(value)}"); return value; },
                ResetOptions = () => { trace.Add("O"); if (variant == 2) { state.Name = "AFTER_OPTIONS"; globals.InitialSubscriber = 7; } RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, options); },
                Set = (key, value) => { trace.Add($"S,{Hex(key)},{Hex(value)}"); if (variant == 3) { state.Name = "AFTER_SET"; globals.InitialSubscriber = 0x76543210; } },
                RequestFort = value => { trace.Add("L," + Hex(value)); if (variant == 4) state.Flags = 0xfffffffe; },
                Header = () => { trace.Add("H"); if (variant == 4) state.Flags ^= 0x102; }
            });
            Assert.Equal(columns[1], $"{state.Flags},{state.Demo},{state.NamePrefix},{globals.TutorialNumber},{globals.MissionNumber},{globals.TutorialSeen},{globals.LoginSubscriber}");
            Assert.Equal(columns[2], Convert.ToHexString(state.Scratch).ToLowerInvariant());
            Assert.Equal(columns[3], Hex(state.Name));
            Assert.Equal(columns[4], Convert.ToHexString(options).ToLowerInvariant());
            Assert.Equal(columns[5], string.Join(';', trace));
        }
    }
}
