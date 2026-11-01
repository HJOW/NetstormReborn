using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>독립 원본의 맵 목록 캐시·행·하위 호출 순서를 비교한다.</summary>
public sealed class RawSoloMapListTests
{
    private sealed record FileInput(string Name, string Subscriber, byte Kind, byte State, bool Present);
    private static string Decode(string text) => text == "_" ? "" : Encoding.Latin1.GetString(Convert.FromHexString(text));
    private static string Encode(string text) => text.Length == 0 ? "_" : Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();

    [Fact]
    public void MatchesOriginalCacheAndRows()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "solo-map-list-x86.tsv");
        string[] rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').ToArray(); Assert.Equal(216, rows.Length);
        foreach (string row in rows)
        {
            string[] col = row.Split('\t'); Assert.Equal(18, col.Length);
            var state = new SoloMapListState { Dirty = uint.Parse(col[3]), Count = int.Parse(col[4]) };
            string[] names = col[5].Split(','), flags = col[6].Split(',');
            for (int i = 0; i < 50; i++) { state.Names[i] = names[i] == "-" ? null : Decode(names[i]); state.Flags[i] = uint.Parse(flags[i]); }
            var files = new List<FileInput>();
            if (col[7] != "-") foreach (string item in col[7].Split(';')) { string[] f = item.Split('/'); files.Add(new(Decode(f[0]), Decode(f[4]), byte.Parse(f[1]), byte.Parse(f[3]), f[2] == "1")); }
            var trace = new List<string>(); FileInput? active = null;
            FileInput Find(string name) => files.First(file => file.Name == name);
            var list = new RawSoloMapList(state, new SoloMapListHooks
            {
                MayEditMissions = () => { trace.Add("O"); return int.Parse(col[2]); },
                Enumerate = () => { trace.Add("E"); return files.Select(file => file.Name).ToArray(); },
                Kind = name => { trace.Add("K:" + name); return Find(name).Kind; },
                Construct = () => { trace.Add("C"); active = null; }, Load = name => { trace.Add("L:" + name); active = Find(name); },
                Present = () => { trace.Add("P"); return active!.Present; }, Subscriber = () => { trace.Add("S"); return active!.Subscriber; },
                StateByte = () => { trace.Add("B"); return active!.State; }, UserName = () => { trace.Add("U"); return Decode(col[8]); },
                Reserved = () => { trace.Add("R"); return 0; }, Translate = text => { trace.Add("T"); return text; }
            });
            Assert.Equal(int.Parse(col[11]), list.Refresh(int.Parse(col[1]))); Assert.Equal(uint.Parse(col[12]), state.Dirty);
            string[] expectedNames = col[13].Split(','), expectedFlags = col[14].Split(',');
            for (int i = 0; i < 50; i++) { Assert.Equal(expectedNames[i] == "-" ? null : Decode(expectedNames[i]), state.Names[i]); Assert.Equal(uint.Parse(expectedFlags[i]), state.Flags[i]); }
            var observed = new List<string>();
            foreach (string value in col[15].Split(','))
            {
                SoloMapRow? result = list.Row(int.Parse(value), Decode(col[9]), Decode(col[10]));
                observed.Add(result == null ? "n" : $"{(result.Checked ? 1 : 0)},{(result.Enabled ? 1 : 0)},{Encode(result.Label)},{Encode(result.Name)}");
            }
            Assert.Equal(col[16], string.Join(';', observed)); Assert.Equal(col[17], string.Join(',', trace));
        }
    }

    [Fact]
    public void ReadsCurrentStateAfterCallbacks()
    {
        var state = new SoloMapListState { Count = 1 }; state.Names[0] = "Old"; state.Flags[0] = 2;
        var list = new RawSoloMapList(state, new SoloMapListHooks
        {
            MayEditMissions = () => { state.Dirty = 1; return 0; },
            Translate = _ => { state.Names[0] = "New"; state.Flags[0] = 8; return "%s (~rCORRUPT~.)"; }
        });
        Assert.Equal(1, list.Refresh(2)); Assert.Equal(1u, state.Dirty);
        Assert.Equal(new SoloMapRow("New (~rCORRUPT~.)", "New", true, true), list.Row(0, "old", ""));
        Assert.Null(list.Row(1, "", "")); Assert.Throws<ArgumentOutOfRangeException>(() => list.Row(-1, "", ""));
    }
}
