using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 전용 창/HTML 부모의 원문 전체 노드/캐시/표식/효과98행을 대조한다.

/// <summary>원본에 공급한 같은 GUI/파일/Config 입력만 제공한다.전용 창/HTML은 실제 제품이다.</summary>
internal sealed class QuickHelpWindowFixtureScene
{
    /// <summary>원문 격리 풀에서 용도별로 지정한 실제 토큰.</summary>
    internal const uint Window = 0x12040000, Decoration = 0x12041000, Inner = 0x12042000, PictureBg = 0x12043000, BodyBg = 0x12044000, BodyToken = 0x12045000, Learn = 0x1204a000, Back = 0x1204b000, Cancel = 0x1204c000, Info = 0x1204d000, Picture = 0x1204e000, Scroll = 0x1204f000, Table = 0x12051000, Source = 0x13000000;
    /// <summary>원문 raw 비교 순서의12개 노드.</summary>
    internal static readonly uint[] Tokens = [Window, Decoration, Inner, PictureBg, BodyBg, BodyToken, Learn, Back, Cancel, Info, Picture, Scroll];
    /// <summary>같은 현재 전용 파생.</summary>
    internal QuickHelpContentNode Node { get; } = new();
    /// <summary>같은 현재 HTML 부모.</summary>
    internal HtmlBodyGumpNode BodyNode { get; } = new();
    /// <summary>나머지 안정된 현재 기본 자식.</summary>
    private readonly Dictionary<uint, GumpCreateNode> _bases = [];
    /// <summary>현재 전용 캐시/표시/생성 전역.</summary>
    internal QuickHelpWindowState State { get; } = new();
    /// <summary>같은 전역 popup.</summary>
    internal QuickHelpPopupState Popup { get; } = new();
    /// <summary>같은 실제 HTML 부모.</summary>
    internal HtmlBodyGump Html { get; }
    /// <summary>실제 전용 창 본체.</summary>
    internal QuickHelpWindow Help { get; }
    /// <summary>현재 실제 외부 장치 입력.</summary>
    internal QuickHelpWindowHooks Hooks { get; }
    /// <summary>원문 생성 크기별 할당 순번.</summary>
    private readonly Dictionary<int, int> _allocations = [];
    /// <summary>명시 기본 생성 배율.</summary>
    private float _scale = 1;
    /// <summary>명시 스크립트 입력.</summary>
    private bool _script;
    /// <summary>명시 치환 빈 결과 입력.</summary>
    private bool _empty;
    /// <summary>명시 파일 실패 입력.</summary>
    private bool _failure;
    /// <summary>현재 정보 버퍼 완료 줄 수 입력.</summary>
    internal int LineCount { get; set; } = 1;
    /// <summary>현재 TYPE 도움말 프레임 입력.</summary>
    internal int Frame { get; set; } = 3;
    /// <summary>같은 실제 HTML 전역.</summary>
    private readonly HtmlTextParserGlobals _globals = new();
    /// <summary>명시 파일 원문.</summary>
    internal string HelpText { get; set; } = "<a name=\"unitType\"><info><p>BODY</a>\nEND";
    /// <summary>현재 관측된 외부 효과 순서.</summary>
    internal List<string> Trace { get; } = [];
    /// <summary>질감 생성 중 전역 변경을 공급하는 명시 경계.</summary>
    internal Action<uint>? BackgroundCreated { get; set; }
    /// <summary>원시 문자열만16진수로 관측한다.</summary>
    private static string Hex(string value) => Convert.ToHexStringLower(Encoding.Latin1.GetBytes(value));
    /// <summary>실제 float32 비트를 읽는다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>부모/자식 모두 동일한 현재 기본 주소다.</summary>
    internal GumpCreateNode Base(uint token) { if (token == Window) return Node; if (token == BodyToken) return BodyNode; if (!_bases.TryGetValue(token, out var result)) _bases.Add(token, result = new()); return result; }
    /// <summary>현재 기본 필드만 초기화하는 명시 GUI 생성 장치.파생 필드는 보존한다.</summary>
    internal void Fill(uint token, float width, float height, uint flags) { var node = Base(token); node.ClearConstructedFields(); node.Form = node.Untouched = 0; node.Vtable = Table; node.Flags = flags | 8; node.Serial = (uint)(100 + Array.IndexOf(Tokens, token)); node.Scale = _scale; node.BaseWidth = width; node.BaseHeight = height; node.Width = width * _scale; node.Height = height * _scale; }
    /// <summary>실제 기본 생성자의 원문 호출 인자를 기록한다.</summary>
    private void ConstructBase(uint token, uint command, float width, float height, uint flags) { Trace.Add($"Base,{token},{command},{Bits(width)},{Bits(height)},{flags}"); Fill(token, width, height, flags); }
    /// <summary>원문 크기에 맞는 별도 실제 토큰만 공급한다.</summary>
    private uint Allocate(int bytes)
    {
        int index = _allocations.GetValueOrDefault(bytes); _allocations[bytes] = index + 1; uint token = bytes switch { 0xc4 => Decoration, 0xb8 => new[] { Inner, PictureBg, BodyBg }[index], 0x40a4 => BodyToken, 0x170 => new[] { Learn, Back, Cancel }[index], 0xd8 => new[] { Info, Scroll }[index], 0xb4 => Picture, _ => throw new ArgumentException("원문 Help 할당 크기") }; Trace.Add($"New,{bytes},{token}"); if (token == BodyToken) { Array.Fill(BodyNode.Body, (byte)0xa5); Array.Fill(BodyNode.Links, (byte)0xa5); } return token;
    }
    /// <summary>실제 다중 줄 생성에 공급한 원문 인자/현재 줄 수만 기록한다.</summary>
    private void ConstructText(uint token, int width, int height, uint font, uint color) { Trace.Add($"Text,{token},{width},{height},{font},{color}"); Fill(token, 0, 0, 1); LineCount = 1; }
    /// <summary>명시 지연 삭제의 실제 필드 효과.</summary>
    internal void Kill(uint token) { Trace.Add("Kill," + token); Base(token).Flags |= 0x10000; }
    /// <summary>명시 가상 Push의 실제 필드 효과.</summary>
    internal void Push(uint token) { Trace.Add("Push," + token); Base(token).Flags |= 8; }
    /// <summary>공통 트리 원문은 별도 대조 범위이며 현재 부착 필드/효과만 공급한다.</summary>
    internal void BaseAttach(uint token, float x, float y, uint parent, uint mode) { Trace.Add($"Attach,{token},{Bits(x)},{Bits(y)},{parent},{mode}"); var node = Base(token); node.X = x; node.Y = y; node.Flags &= ~8U; if (parent != 0) node.Parent = parent; }
    /// <summary>실제 전용 창/HTML 부모의 파생 부착은 제품 몸체를 실행한다.</summary>
    internal void Attach(uint token, float x, float y, uint parent, uint mode) { if (token == Window) Help.Attach(token, x, y, parent, mode); else if (token == BodyToken && BodyNode.Vtable == 0x50bd38) Html.Attach(token, x, y, parent, mode); else BaseAttach(token, x, y, parent, mode); }
    /// <summary>최초 원문과 동일한 기본 GUI/Config/파일·현재 파생 처리를 조립한다.</summary>
    internal QuickHelpWindowFixtureScene()
    {
        // 실제 생성자가 덮기 전의 독립 기본 필드만 준비한다.
        foreach (uint token in Tokens) { Fill(token, 450, 350, 1); Base(token).Flags = 1; }
        Node.Vtable = 0x514ec4; Node.Command = 19; Node.Decoration = Decoration; Node.Information = Info; Node.Body = BodyToken; Node.CancelButton = Cancel; Node.LearnButton = Learn; Node.BackButton = Back; Node.InnerBackground = Inner; Node.PictureBackground = PictureBg; Node.BodyBackground = BodyBg; State.HelpData = HelpText; State.ScriptName = "mission"; State.Color = 47; State.NoPictureA = 2; State.NoPictureB = 3; Popup.Mark(0x20000a0, "before");
        Html = new(new() { ScreenWidth = 1024, ScreenHeight = 768, Color = 47 }, new() { Node = token => token == BodyToken ? BodyNode : null, Base = Base, ConstructBase = ConstructBase, Allocate = Allocate, ConstructText = ConstructText, Kill = Kill, Push = Push, Attach = Attach, BaseAttach = BaseAttach, Styled = (token, text) => Trace.Add($"Styled,{token},{Hex(text)}"), Scroll = (token, value) => Trace.Add($"Scroll,{token},{value}"), Slider = (token, value) => Trace.Add($"Slider,{token},{value}"), Scrollable = (token, value) => Trace.Add($"Scrollable,{token},{value}") });
        Hooks = new() { Node = token => token == Window ? Node : null, Base = Base, Html = token => token == BodyToken ? BodyNode : null, ConstructBase = ConstructBase, Allocate = Allocate, ConstructDecoration = (token, w, h) => { Trace.Add($"Decoration,{token},{Bits(w)},{Bits(h)}"); Fill(token, w, h, 1); Base(token).AlignFlags = 0x20; Base(token).SecondaryDepth = 0x123; }, ConstructBackground = (token, w, h, background, flags, type, frame) => { Trace.Add($"Background,{token},{Bits(w)},{Bits(h)},{background},{flags},{type},{frame}"); Fill(token, w, h, flags); BackgroundCreated?.Invoke(token); }, ConstructHtml = token => Html.Construct(token), ConstructButton = (token, w, h, label, unused, context) => { Trace.Add($"Stone,{token},{w},{h},{Hex(label)},{unused},{context}"); Fill(token, 60, 20, 0); }, FitButton = token => Trace.Add("Fit," + token), ConstructInformation = ConstructText, ConstructPicture = (token, type, frame, mode) => { Trace.Add($"Picture,{token},{type},{frame},{mode}"); Fill(token, 99, 88, 1); }, Translate = text => text, Config = key => { Trace.Add("Config," + Hex(key)); return "d/help.english"; }, Load = name => { Trace.Add($"Load,{Hex(name)},0,2"); return _failure ? null : HelpText; }, LoadError = name => Trace.Add("LoadError,9," + Hex(name)), Substitute = text => { Trace.Add($"Substitute,{Hex(text)},8192"); return _empty ? "" : text; }, NearestColor = (r, g, b) => { Trace.Add($"Color,{r},{g},{b}"); return 35; }, Scripted = () => { Trace.Add("Script"); return _script; }, Kill = Kill, Push = Push, ClearBody = Html.Clear, Attach = Attach, BaseAttach = BaseAttach, Lines = token => { Assert.Equal(Info, token); return LineCount; }, Render = Render, HelpFrame = _ => Frame, BodyText = Html.SetText }; Help = new(State, Popup, Hooks);
    }
    /// <summary>기존 실제 전용 HTML/링크와 현재 body 배열을 그대로 사용한다.</summary>
    private void Render(uint token, uint type, string topic, uint information, uint stream)
    {
        Assert.Equal((BodyToken, Info, 0U), (token, information, stream)); var state = new HtmlTextParserState(); var links = new HtmlLinkTextState { Base = BodyToken + 0x20a4, Capacity = 8192 }; var basic = new HtmlTextParserHooks(); HtmlTextParser.BindHtmlPictureText(basic); HtmlLinkText.BindHtmlLinkText(basic, links); var data = new QuickHelpTypeInfo { Theme = 3 }; var hooks = new QuickHelpHtmlHooks { Type = () => data, Description = () => "Unit Label", HitPoints = () => 0, Translate = text => text, Substitute = text => text, Config = text => text, Print = QuickHelpHtmlScene.Print, Pane = () => Base(Info), PaneText = text => Trace.Add($"Styled,{Info},{Hex(text)}"), PaneScroll = value => Trace.Add($"Scroll,{Info},{value}"), PanePush = () => Push(Info), PaneAttach = () => { Trace.Add("Reattach," + Info); Base(Info).Flags &= ~8U; } }; var parser = new QuickHelpHtml(state, _globals, State.HtmlMode, type, topic, false, new(), hooks, basic, links); Memory<byte> target = default, linkTarget = default; Html.Parse(token, new() { HasOutput = () => false, Bind = (body, rawLinks) => { target = body; linkTarget = rawLinks; }, Run = () => { string result = parser.Parse(State.HelpData!); Assert.True(result.Length < target.Length && links.Cursor < linkTarget.Length); Encoding.Latin1.GetBytes(result).AsSpan().CopyTo(target.Span); target.Span[result.Length] = 0; links.Bytes.ToArray().AsSpan().CopyTo(linkTarget.Span); } });
    }
    /// <summary>같은 현재 전체0xa0 기본/0xd0 파생·전역·표식·trace를 관측한다.</summary>
    internal string Output()
    {
        var parts = new List<string>();
        // 기대값을 생성하지 않고 실제 현재 노드를 원래 순서로 직렬화한다.
        foreach (uint token in Tokens)
        {
            string raw = GumpRawSupport.Encode(Base(token)); if (token == Window) { uint[] values = [Node.Picture, Node.Decoration, Node.Information, Node.Body, Node.CancelButton, Node.LearnButton, Node.BackButton, Node.Stream, Node.InnerBackground, Node.PictureBackground, Node.BodyBackground, Node.LearnType]; byte[] tail = new byte[48];
                // 원문+a0..cc의 물리 순서로 현재 DWORD만 기록한다.
                for (int i = 0; i < values.Length; i++) BinaryPrimitives.WriteUInt32LittleEndian(tail.AsSpan(i * 4), values[i]); raw += Convert.ToHexStringLower(tail); } parts.Add(token + ":" + raw);
        }
        return string.Join(';', parts) + $"|{State.Id},{(State.HelpData is null ? 0U : Source)},{Popup.Command}|{Hex(Popup.Argument())}|{string.Join(';', Trace)}";
    }
    /// <summary>native의 생성/배치/본문 명시 입력만 준비하고 분기 결과는 실제 제품에 맡긴다.</summary>
    internal void Run(string kind, JsonElement input, bool paths)
    {
        // 입력 정수만 읽으며 원본 기대 관측은 읽지 않는다.
        uint U(int index) => input[index].GetUInt32();
        if (paths) { HelpText = "<a name=\"introHelp\">INTRO</a>\n<a name=\"challengeHelp\">CHALLENGE</a>\n<a name=\"pyrosphereHelp\">BATTLE</a>\n<a name=\"unitType\">UNIT</a>\n"; State.HelpData = HelpText; }
        if (kind == "C") { if (paths) _failure = true; else { _script = U(2) != 0; _scale = BitConverter.UInt32BitsToSingle(U(3)); } if (U(1) == 0) State.HelpData = null; Help.Construct(Window, U(0)); }
        else if (kind == "L")
        {
            uint buttons = U(2), profile = U(3); Node.Picture = U(0) != 0 ? Picture : 0; Node.Information = U(1) != 0 ? Info : 0; Node.CancelButton = (buttons & 1) != 0 ? Cancel : 0; Node.BackButton = (buttons & 2) != 0 ? Back : 0; Node.BaseWidth = new[] { 450f, 641f, 320f }[profile]; Node.BaseHeight = new[] { 350f, 481f, 240f }[profile];
            // 각 자식의 독립 배율/버튼 치수는 원문 입력 그대로다.
            for (int i = 1; i < Tokens.Length; i++) { var node = Base(Tokens[i]); node.Scale = new[] { 1f, 1.25f, .75f }[profile]; node.BaseWidth = 30.75f + (i - 1) * 7; node.BaseHeight = 20.75f + i - 1; } LineCount = profile == 2 ? 0 : 1; Help.Attach(Window, 13, 21, 0, 0);
        }
        else if (kind == "H") { BodyNode.Vtable = 0x50bd38; BodyNode.Text = U(0) != 0 ? Scroll : 0; BodyNode.Flags = U(1) != 0 ? 9U : 1; BodyNode.X = 17; BodyNode.Y = 19; _allocations[0xd8] = 1; Html.SetText(BodyToken, "~Nstyled"); }
        else if (kind == "R") { State.Battle = U(0) != 0; State.Challenge = U(1) != 0; var request = new QuickHelpRequest(Node, new() { Lookup = name => { Trace.Add("Lookup," + Hex(name)); return 0; }, Print = (_, name) => name, History = name => Trace.Add("History," + Hex(name)), Show = (type, name) => Help.Show(Window, type, name), Push = () => Push(Window), Attach = (x, y, p, mode) => Help.Attach(Window, x, y, p, mode) }); request.Open(0, ""); }
        else { Assert.Equal("S", kind); Frame = input[2].GetInt32(); Node.Picture = U(3) != 0 ? Picture : 0; Node.Information = U(3) != 0 ? Info : 0; BodyNode.Vtable = 0x50bd38; BodyNode.Text = U(3) != 0 ? Scroll : 0; if (paths) { State.Battle = U(4) != 0; State.Challenge = U(5) != 0; _empty = U(6) != 0; } Help.Show(Window, U(0), input[1].GetString()!); }
    }
}
/// <summary>최초 전체84행/추가14경로의 전체 바이트/효과를 실제 제품으로 대조한다.</summary>
public sealed class QuickHelpWindowNativeTests
{
    /// <summary>앞2열만 실행 입력으로 쓰고 뒤 원본 관측은 비교에만 쓴다.</summary>
    [Theory]
    [InlineData("quick-help-window", 84, false)]
    [InlineData("quick-help-window-paths", 14, true)]
    public void All98NativeWindowFieldsCachePopupAndEffectsMatch(string fixture, int expected, bool paths)
    {
        int count = 0;
        // 독립 장면에서 전체 실제 부모/자식 상태만 읽는다.
        foreach (string[] row in X86Fixture.Read(fixture)) { Assert.Equal(3, row.Length); using var input = JsonDocument.Parse(row[1]); var scene = new QuickHelpWindowFixtureScene(); scene.Run(row[0], input.RootElement, paths); Assert.True(row[2] == scene.Output(), $"Help 창 원본행{fixture}/{count} {row[0]} {row[1]}\n기대:{row[2]}\n실제:{scene.Output()}"); count++; } Assert.Equal(expected, count);
    }
}
