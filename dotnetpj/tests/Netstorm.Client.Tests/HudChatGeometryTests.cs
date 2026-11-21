using System.Globalization;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 네 실제 자식의 기하3960관측과 callback 뒤 현재 포인터/프레임 요청을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>독립 x87 입력을 같은 노드로 재생하며 전체 창 생성 완료로 계산하지 않는다.</summary>
public sealed class HudChatGeometryTests
{
    /// <summary>원문DWORD 숫자를 문화권 없이 읽는다.</summary>
    private static uint Number(string text) => uint.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>단정도 비트를 원문 입력값으로 복원한다.</summary>
    private static float Float(string text) => BitConverter.UInt32BitsToSingle(Number(text));
    /// <summary>실제 단정도 출력의 모든 비트를 관찰한다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>실제 경계 인자를 순서대로 직렬화한다.</summary>
    private static string Packed(params uint[] values) => string.Join(':', values.Select(value => value.ToString(CultureInfo.InvariantCulture)));
    /// <summary>원문1..5 토큰의 동일 root/네 자식과 명시 효과 관찰이다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 기본 조회와 공유하는 실제 파생 root다.</summary>
        public readonly HudChatNode Root = new();
        /// <summary>Console/로그/helper/배경의 실제 네 저장소다.</summary>
        public readonly GumpCreateNode[] Children = [new(), new(), new(), new()];
        /// <summary>기존 로그 위치의 명시 입력이다.</summary>
        public uint Bottom;
        /// <summary>현재 메타 모드의 명시 입력이다.</summary>
        public uint Challenge;
        /// <summary>실제 공개 경계의 호출 순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>같은 노드와 장치로 실행하는 제품 본체다.</summary>
        public readonly HudChatGeometry Geometry;
        /// <summary>원문 토큰의 같은 실제 객체를 반환한다.</summary>
        public GumpCreateNode? Node(uint token) => token == 1 ? Root : token is >= 2 and <= 5 ? Children[token - 2] : null;
        /// <summary>앞14열 입력만 사용하고 기하 기대값은 계산하지 않는다.</summary>
        public Scene(string[] row)
        {
            Root.Console = 2; Root.Text = 3; Root.Helper = 4; Root.Background = 5; Root.LeftPadding = int.Parse(row[4], CultureInfo.InvariantCulture); Root.HelperVisible = (byte)Number(row[5]); string[] scales = row[7].Split(':'), positions = row[9].Split(':');
            // 각 실제 입력 노드에 원문 표식과 독립 배율만 공급한다.
            for (uint token = 1; token <= 5; token++)
            {
                var node = Node(token)!; node.Flags = token == 1 ? 0xe01u : token == 2 ? Number(row[6]) : 0x601; node.Scale = token == 1 ? 1 : Float(scales[token - 2]); node.BaseWidth = token == 1 ? Float(row[2]) : new float[] { 60, 40, 1, 20 }[token - 2]; node.BaseHeight = token == 1 ? Float(row[3]) : token == 2 ? Float(row[8]) : new float[] { 0, 50, 9, 30 }[token - 2]; node.Width = 123; node.Height = -456;
            }
            Root.X = Float(positions[0]); Root.Y = Float(positions[1]); Children[0].X = Float(positions[2]); Children[0].Y = Float(positions[3]); Bottom = Number(row[13]); Challenge = Number(row[11]);
            Geometry = new(new()
            {
                Chat = token => token == 1 ? Root : null, Node = Node,
                BaseAttach = (token, x, y, parent, mode) => Trace.Add("baseAttach:" + Packed(token, Bits(x), Bits(y), parent, mode)), Push = token => Trace.Add("push:" + token),
                Attach = (token, x, y, parent, mode) => Trace.Add("attach:" + Packed(token, Bits(x), Bits(y), parent, mode)), BottomIndex = token => { Trace.Add("bottomIndex:" + token); return Bottom; },
                RequestBottom = (token, extra) => Trace.Add("bottom:" + Packed(token, extra)), Challenge = () => { Trace.Add("challenge:" + Challenge); return Challenge; },
            });
        }
        /// <summary>실제로 쓴 다섯 노드의 크기/표시만 읽는다.</summary>
        public string State()
        {
            var result = new List<string>();
            // 변경하지 않은 root/Console 높이도 같은 관측에 포함한다.
            for (uint token = 1; token <= 5; token++) { var node = Node(token)!; result.Add(Packed(token, node.Flags, Bits(node.BaseWidth), Bits(node.BaseHeight), Bits(node.Width), Bits(node.Height))); }
            return string.Join(';', result);
        }
    }
    /// <summary>1980입력의 두x87 정밀도에 대한3960개 크기/외부 경계 관측을 대조한다.</summary>
    [Fact]
    public void ReplaysNative3960SizesAndPublicCallbacks()
    {
        int rows = 0;
        // 일반 헤더와 주석을 제외한 독립 원문만 소비한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "hud-chat-geometry-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#') || line.StartsWith("name\t", StringComparison.Ordinal)) continue; string[] row = line.Split('\t'); Assert.Equal(20, row.Length); Assert.Equal("1", row[14]); Assert.Equal("0", row[18]); Assert.True(Number(row[19]) > 0); var scene = new Scene(row);
            if (row[1] == "attach") { string[] positions = row[9].Split(':'); scene.Geometry.Attach(1, Float(positions[4]), Float(positions[5]), 17, 3); }
            else if (row[1] == "helper") scene.Geometry.SetHelperVisible(1, Number(row[10])); else { Assert.Equal("input", row[1]); scene.Geometry.SetInputVisible(Number(row[12]) != 0 ? 1u : 0, Number(row[10])); }
            Assert.Equal(row[15], scene.State()); Assert.Equal(Number(row[16]), scene.Root.HelperVisible);
            // 내부 ScrollingText의 Redraw는 공개 요청 경계 바깥이므로 CPP와 같은 관측만 비교한다.
            string expected = row[17] == "-" ? "" : string.Join(';', row[17].Split(';').Where(item => !item.StartsWith("redraw:", StringComparison.Ordinal))); Assert.Equal(expected, string.Join(';', scene.Trace)); rows++;
        }
        Assert.Equal(3960, rows);
    }
    /// <summary>Console Push 뒤 현재 root가 바뀌면 새 자식만 표시 반전/재부착한다.</summary>
    [Fact]
    public void ToggleRequeriesCurrentRootAfterPush()
    {
        var first = new HudChatNode { Console = 2 }; var second = new HudChatNode { Console = 7 }; var original = new GumpCreateNode(); var replacement = new GumpCreateNode { X = 4, Y = 5 }; uint current = 1; var trace = new List<string>();
        // 현재 root와 자식이 같은 공통 표를 참조한다.
        GumpCreateNode? Node(uint token) => token switch { 1 => first, 6 => second, 2 => original, 7 => replacement, _ => null };
        var geometry = new HudChatGeometry(new() { Chat = token => Node(token) as HudChatNode, Node = Node, Current = () => current, Push = token => { trace.Add("push:" + token); current = 6; }, Attach = (token, x, y, parent, mode) => trace.Add("attach:" + Packed(token, Bits(x), Bits(y), parent, mode)) }); geometry.ToggleInputVisible(); Assert.Equal(0u, original.Flags); Assert.Equal(1u, replacement.Flags); Assert.Equal(new[] { "push:2", "attach:" + Packed(7, Bits(4), Bits(5), 0, 0) }, trace); current = 0; Assert.Equal(0u, geometry.InputVisible()); geometry.ToggleInputVisible(); Assert.Equal(2, trace.Count);
    }
    /// <summary>뗌/다른키/표시중 Enter의 관문과 두장치의 선행 확인을 유지한다.</summary>
    [Fact]
    public void EnterKeyGuardsAndEffectOrder()
    {
        Assert.False(HudChatGeometry.DispatchChatInputKey(new(InputCode.Release | 13, 0, 0), null, null, null)); Assert.False(HudChatGeometry.DispatchChatInputKey(new(12, 0, 0), null, null, null)); Assert.False(HudChatGeometry.DispatchChatInputKey(new(13, 0, 0), () => 1, null, null)); var effects = new List<string>(); Assert.Throws<InvalidOperationException>(() => HudChatGeometry.DispatchChatInputKey(new(13, 0, 0), () => 0, () => effects.Add("show"), null)); Assert.Empty(effects); Assert.True(HudChatGeometry.DispatchChatInputKey(new(InputCode.Shift | 13, 0, 0), () => 0, () => effects.Add("show"), () => effects.Add("open"))); Assert.Equal(new[] { "show", "open" }, effects);
    }
    /// <summary>프레임 요청은 정상 복귀 뒤0이며 예외에서 현재 부분 변경을 보존한다.</summary>
    [Fact]
    public void FrameToggleClearsOnlyAfterSuccessfulCallback()
    {
        var request = new IntBox(); HudChatGeometry.ConsumeChatInputToggle(request, null); request.Value = -1; Assert.Throws<InvalidOperationException>(() => HudChatGeometry.ConsumeChatInputToggle(request, null)); Assert.Equal(-1, request.Value); Assert.Throws<OperationCanceledException>(() => HudChatGeometry.ConsumeChatInputToggle(request, () => { request.Value = 9; throw new OperationCanceledException(); })); Assert.Equal(9, request.Value); HudChatGeometry.ConsumeChatInputToggle(request, () => request.Value = 7); Assert.Equal(0, request.Value);
    }
}
