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

    /// <summary>스크롤 시험용 창 상태와 이동 행 관측이다.</summary>
    private sealed class PagingDevice
    {
        public int Dirty, Count, WindowFirst, Begins, Rows, Reused;
        public bool Keep;
        public Func<string, string>? Translate;
        public List<string> Scrolled { get; } = [];
        public List<string> Trace { get; } = [];
        public TellMapMenuHooks Hooks() => new()
        {
            BeginVertical = _ => { Trace.Add("begin"); Begins++; },
            Invalidate = () => Dirty = 1, SelectFileSpec = _ => { }, Refresh = _ => Dirty = 0, Count = () => Count,
            AppendRow = index => { Trace.Add("row:" + index); if (index >= Count) return false; Rows++; return true; },
            WindowFirst = () => WindowFirst, SetWindowFirst = first => WindowFirst = first,
            TakeKeepCache = () => { bool value = Keep; Keep = false; return value; },
            ReuseCache = () => Reused++,
            AppendScroll = (label, delta) => Scrolled.Add(label + "|" + delta),
            Translate = Translate,
        };
    }

    /// <summary>50행 초과 목록은 한 창씩 이전·다음 이동 행과 함께 만든다.</summary>
    [Fact]
    public void PagingLabelsClampAndHalves()
    {
        var device = new PagingDevice { Count = 120 };
        uint columns = 0;
        Assert.True(TellMapMenus.Append("*sololist*", ref columns, device.Hooks()));
        // 첫 창 50행 + 다음 이동행, 두 번째 열에서 분할이다
        Assert.Equal(2u, columns);
        Assert.Equal(50, device.Rows);
        Assert.Equal(["~oNext 50 >  (51-100 of 120)|50"], device.Scrolled);
        // 다음 창으로 넘기면 이전·다음 이동행이 모두 붙는다
        device.WindowFirst = 50; device.Keep = true;
        columns = 0; device.Rows = 0; device.Scrolled.Clear(); device.Trace.Clear();
        Assert.True(TellMapMenus.Append("*sololist*", ref columns, device.Hooks()));
        Assert.Equal(["~o< Previous 50  (1-50 of 120)|-50", "~oNext 50 >  (101-120 of 120)|50"], device.Scrolled);
        Assert.Equal(50, device.Rows);
        // 마지막 창은 꽉 채우고 다음 이동행이 없다
        device.WindowFirst = 200; device.Keep = true;
        columns = 0; device.Rows = 0; device.Scrolled.Clear(); device.Trace.Clear();
        Assert.True(TellMapMenus.Append("*sololist*", ref columns, device.Hooks()));
        Assert.Equal(["~o< Previous 50  (21-70 of 120)|-50"], device.Scrolled);
        Assert.Equal(50, device.Rows);
        // 50행 이하는 스크롤 없이 원본 흐름이다
        device.Count = 50; device.WindowFirst = 0; device.Keep = false;
        columns = 0; device.Rows = 0; device.Scrolled.Clear(); device.Trace.Clear();
        Assert.True(TellMapMenus.Append("*sololist*", ref columns, device.Hooks()));
        Assert.Empty(device.Scrolled);
        Assert.Equal(50, device.Rows);
    }

    /// <summary>이동 행 단어는 번역 장치가 없으면 영어 그대로, 있으면 바꾼다.</summary>
    [Fact]
    public void ScrollWordsTranslateWithFallback()
    {
        var device = new PagingDevice { Count = 120, Translate = key => key == "Previous" ? "이전" : key };
        uint columns = 0;
        Assert.True(TellMapMenus.Append("*fortlist*", ref columns, device.Hooks()));
        Assert.Equal(["~oNext 50 >  (51-100 of 120)|50"], device.Scrolled);
        device.WindowFirst = 50; device.Keep = true; device.Scrolled.Clear();
        columns = 0;
        Assert.True(TellMapMenus.Append("*fortlist*", ref columns, device.Hooks()));
        Assert.Equal(["~o< 이전 50  (1-50 of 120)|-50", "~oNext 50 >  (101-120 of 120)|50"], device.Scrolled);
    }

    /// <summary>재표시이면 무효화 없이 캐시를 쓰고 일회성 표시는 소비한다.</summary>
    [Fact]
    public void KeepCacheSkipsInvalidateAndIsConsumed()
    {
        var device = new PagingDevice { Count = 60, Keep = true };
        uint columns = 0;
        Assert.True(TellMapMenus.Append("*fortlist*", ref columns, device.Hooks()));
        Assert.Equal(0, device.Dirty);
        Assert.Equal(1, device.Reused);
        Assert.False(device.Keep);
        Assert.Equal(50, device.Rows);
    }

    /// <summary>묶음 연결이 스크롤 창 상태와 이동 행 명령을 같은 목록 상태에 둔다.</summary>
    [Fact]
    public void BindScrollWiringSharesWindowState()
    {
        // 5차 무제한 Edit 목록처럼 한도를 올린다
        var state = new SoloMapListState { Limit = int.MaxValue };
        var files = Enumerable.Range(0, 60).Select(i => "m" + i).ToArray();
        var list = new RawSoloMapList(state, new SoloMapListHooks
        {
            MayEditMissions = () => 0, Enumerate = () => files, Kind = _ => 0,
            Construct = () => { }, Load = _ => { }, Present = () => true,
            Subscriber = () => "u", StateByte = () => 0, UserName = () => "u", Reserved = () => 0u,
        });
        var rows = new List<(SoloMapRow Row, uint Command)>();
        var bound = TellMapMenus.Bind(list, state, _ => { }, () => "", () => "", _ => { }, (row, command) => rows.Add((row, command)));
        Assert.Equal(0, bound.WindowFirst!());
        bound.SetWindowFirst!(7);
        Assert.Equal(7, state.WindowFirst);
        state.KeepCache = true;
        Assert.True(bound.TakeKeepCache!());
        Assert.False(state.KeepCache);
        state.Dirty = 1;
        bound.ReuseCache!();
        Assert.Equal(0u, state.Dirty);
        uint columns = 0;
        state.WindowFirst = 0;
        Assert.True(TellMapMenus.Append("*fortlist*", ref columns, bound));
        Assert.Equal(60, state.Count);
        Assert.Equal(51, rows.Count);
        Assert.Equal(50, rows.Count(row => row.Command == 0x020000cau));
        Assert.Equal(0x02000134u, rows[^1].Command);
        Assert.Equal("50", rows[^1].Row.Name);
        Assert.False(rows[^1].Row.Checked);
        Assert.True(rows[^1].Row.Enabled);
    }

    /// <summary>묶음 연결이 이동 행 번역 장치를 그대로 전달한다.</summary>
    /// <remarks>2026-11-11 0.367에서 RawMenuHtml.inc K4 맵 단어 번역 배선과 함께 추가했다.</remarks>
    [Fact]
    public void BindForwardsTranslateDevice()
    {
        var state = new SoloMapListState();
        var list = new RawSoloMapList(state, new SoloMapListHooks { MayEditMissions = () => 0 });
        var bound = TellMapMenus.Bind(list, state, _ => { }, () => "", () => "", _ => { }, (_, _) => { },
            key => key == "Next" ? "다음" : key);
        Assert.NotNull(bound.Translate);
        Assert.Equal("다음", bound.Translate!("Next"));
        Assert.Equal("of", bound.Translate("of"));
        var plain = TellMapMenus.Bind(list, state, _ => { }, () => "", () => "", _ => { }, (_, _) => { });
        Assert.Null(plain.Translate);
    }
}
