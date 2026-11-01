using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using System.Text;

namespace Netstorm.Client.Tests;

/// <summary>Tell 맵 목록의 독립 원본 관측과 실제 캐시 조립을 검증한다.</summary>
public sealed class TellMapMenusTests
{
    private sealed class Device
    {
        public uint Menus, Dirty;
        public string Key = "", Mutation = "";
        public int Count, Begins, Rows;
        public List<string> Trace { get; } = [];
        public TellMapMenuHooks Hooks() => new()
        {
            BeginVertical = flags => { Trace.Add("begin:" + flags); if (Begins++ == 0 && Mutation == "begin-menus") Menus = 5; },
            Invalidate = () => Dirty = 1, SelectFileSpec = solo => Key = solo ? "SOLOSpec" : "fortSpec",
            Refresh = mode => { Trace.Add($"refresh:{mode}:{Key}:{Dirty}"); Dirty = 0; }, Count = () => Count,
            AppendRow = index => { Trace.Add("row:" + index); if (index >= Count) return false; if (Rows++ == 0) { if (Mutation == "row-count") Count = 32; if (Mutation == "row-state") { Key = "changed!"; Dirty = 7; } } return true; }
        };
    }

    [Fact]
    public void MatchesNativeMenuColumnsAndCurrentState()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "tell-map-menus-x86.tsv");
        string[] rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').ToArray(); Assert.True(rows.Length >= 80);
        foreach (string row in rows)
        {
            string[] col = row.Split('\t'); Assert.Equal(13, col.Length);
            var device = new Device { Menus = uint.Parse(col[2]), Dirty = uint.Parse(col[3]), Key = col[4], Count = int.Parse(col[5]), Mutation = col[6] };
            bool handled = TellMapMenus.Append(col[1], ref device.Menus, device.Hooks());
            Assert.Equal(col[7] == "1", handled); Assert.Equal(uint.Parse(col[8]), device.Menus); Assert.Equal(uint.Parse(col[9]), device.Dirty);
            Assert.Equal(col[10], device.Key); Assert.Equal(col[11], string.Join(';', device.Trace)); Assert.Equal("0", col[12]);
        }
    }

    [Fact]
    public void ConnectsCurrentCacheAndReadFortCommand()
    {
        var state = new SoloMapListState { Count = 2 }; state.Names[0] = "one"; state.Names[1] = "two";
        var list = new RawSoloMapList(state, new SoloMapListHooks { MayEditMissions = () => 0 });
        // 이 조립 검사는 Refresh를 별도로 공급해 외부 파일 수명 대신 현재 캐시와 행 전달만 검사한다.
        string key = "fortSpec"; var rows = new List<(SoloMapRow Row, uint Command)>();
        var bound = TellMapMenus.Bind(list, state, solo => key = solo ? "SOLOSpec" : "fortSpec", () => "two", () => "", _ => { }, (row, command) => rows.Add((row, command)));
        Assert.NotNull(bound.AppendRow); Assert.True(bound.AppendRow!(1)); Assert.Single(rows); Assert.Equal(("two", 0x020000cau), (rows[0].Row.Name, rows[0].Command));
        Assert.True(rows[0].Row.Checked); Assert.False(bound.AppendRow(2)); Assert.Equal("fortSpec", key);
    }

    [Fact]
    public void MetadataUsesExistingColumnAndSplitsCurrentMapRows()
    {
        var node = new HtmlWindowGumpNode(); int begins = 0, appended = 0; string key = "fortSpec";
        var metadata = new HtmlWindowMetadataHooks
        {
            Node = _ => node, ClearButtons = _ => { }, BeginMenu = _ => begins++,
            Commands = new() { Lookup = _ => 0x02000089 }, StoreText = _ => 101
        };
        HtmlWindowMetadata.BindHtmlWindowMetadataMaps(metadata, new TellMapMenuHooks
        {
            BeginVertical = _ => begins++, Invalidate = () => { }, SelectFileSpec = solo => key = solo ? "SOLOSpec" : "fortSpec",
            Refresh = _ => { }, Count = () => 16, AppendRow = index => { if (index >= 16) return false; appended++; return true; }
        });
        byte[] buffer = new byte[8192]; Encoding.Latin1.GetBytes("$Menu=*SOLOLIST*,DoNothing,0", buffer);
        Assert.Equal(1u, new HtmlWindowMetadata(metadata).Process(1, buffer));
        Assert.Equal(2, begins); Assert.Equal(2u, node.MenuColumns); Assert.Equal(16, appended); Assert.Equal("fortSpec", key);
    }
}
