using System.Globalization;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;

namespace Netstorm.Client.Tests;

/// <summary>편집 선택의 원본 State·전역과 외부 호출 순서를 대조한다.</summary>
public sealed class FortSelectionTests
{
    [Fact]
    public void MatchesNativeSelection()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "fort-selection-x86.tsv");
        string[] rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').ToArray(); Assert.Equal(3672, rows.Length);
        foreach (string row in rows)
        {
            string[] col = row.Split('\t'); Assert.Equal(22, col.Length);
            var state = new FortSelectionGlobals { Mode = uint.Parse(col[2]), InBattle = uint.Parse(col[4]), CurrentState = uint.Parse(col[5]), TemplateOwner = uint.Parse(col[6]), Owner = uint.Parse(col[7]) };
            Array.Fill(state.Next, 0xabcdef01u); state.Next[0] = uint.Parse(col[3]);
            byte[][] cores = [Enumerable.Repeat((byte)0xa5, 32).ToArray(), Enumerable.Repeat((byte)0xa5, 32).ToArray()];
            for (int i = 0; i < 2; i++) { cores[i][0] = byte.Parse(col[8 + i]); cores[i][12] = 0x44; cores[i][13] = 0x33; cores[i][14] = 0x22; cores[i][15] = 0x11; }
            string pending = "old", mutation = col[12]; int coreCalls = 0; var trace = new List<string>();
            void Mutate(string at)
            {
                if (mutation == at + "-mode0") state.Mode = 0; else if (mutation == at + "-mode2") state.Mode = 2;
                else if (mutation == at + "-slot1") state.TemplateOwner = 1; else if (mutation == at + "-owner2") state.Owner = 2; else if (mutation == at + "-next99") state.Next[0] = 99;
            }
            var hooks = new FortSelectionHooks
            {
                ResolveString = token => { Assert.Equal(202u, token); return "Selected"; },
                LastEditPath = () => { trace.Add("path:lastEditFort:0:0:0"); Mutate("path"); return "LastEdit"; },
                Pending = name => { trace.Add("pending:" + name); pending = FortSelection.PendingName(name); Mutate("pending"); },
                Refresh = value => { trace.Add("refresh:" + value); Mutate("refresh"); }, Tell = name => { trace.Add("tell:" + name); Mutate("tell"); },
                CurrentFortPath = () => { trace.Add("current-path"); Mutate("path"); return "CurrentPath"; },
                LoadFort = (slot, name) => { trace.Add($"load:{slot}:{name}"); Mutate("load"); },
                Core = slot => { trace.Add("core:" + slot); Mutate("core" + ++coreCalls); return cores[slot]; },
                GameSeconds = () => { trace.Add("time"); return double.Parse(col[10], CultureInfo.InvariantCulture); },
                OwnedIslands = owner => { trace.Add("islands:" + owner); Mutate("islands"); return int.Parse(col[11]); },
                Notify = slot => { trace.Add("notify:" + slot); Mutate("notify"); }
            };
            if (col[1] == "next6") FortSelection.RequestRead(state);
            else if (col[1] == "next1") FortSelection.RequestEdit(state, hooks);
            else Assert.True(FortSelection.Dispatch(new(col[1] == "ask" ? FortSelection.AskEditFort : FortSelection.ReadFort, 101, 202), state, hooks));
            Assert.Equal(uint.Parse(col[13]), state.Mode); Assert.Equal(uint.Parse(col[14]), state.Next[0]); Assert.Equal(uint.Parse(col[15]), state.TemplateOwner); Assert.Equal(uint.Parse(col[16]), state.Owner);
            Assert.Equal(col[17], Convert.ToHexString(cores[0]).ToLowerInvariant()); Assert.Equal(col[18], Convert.ToHexString(cores[1]).ToLowerInvariant());
            Assert.Equal(col[19], pending); Assert.Equal(col[20], string.Join(';', trace)); Assert.Equal("0", col[21]); Assert.All(state.Next[1..], value => Assert.Equal(0xabcdef01u, value));
        }
    }

    [Fact]
    public void RejectsReleaseAndPreservesPartialState()
    {
        var shared = new MissionFortRequestGlobals(); var state = new FortSelectionGlobals(shared.Next); state.Next[0] = 7;
        Assert.Same(shared.Next, state.Next);
        Assert.False(FortSelection.Dispatch(new(FortSelection.ReadFort | InputCode.Release, 0, 202), state, new()));
        Assert.Throws<InvalidOperationException>(() => FortSelection.Dispatch(new(FortSelection.AskEditFort, 0, 0), state, new()));
        Assert.Equal(2u, state.Mode); Assert.Equal(7u, state.Next[0]);
        Assert.Equal("Map", FortSelection.PendingName("Map\0.ignored"));
        Assert.Throws<InvalidOperationException>(() => FortSelection.PendingName("bad.name"));
    }
}
