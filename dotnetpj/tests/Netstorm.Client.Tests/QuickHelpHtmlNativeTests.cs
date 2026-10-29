using System.Globalization;
using System.Text;
using System.Text.Json;
using Netstorm.Assets;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 제품 작성 전 동결된 도움말83행/중첩3행과 실제 보호 본문을 대조한다.

/// <summary>실제 기본/파생 HTML과 명시 TYPE/번역/서식/pane/stream 입력을 결합한다.</summary>
internal sealed class QuickHelpHtmlScene
{
    /// <summary>현재 파서 상태.원문 생성자의 전체 초기화를 검사한다.</summary>
    internal HtmlTextParserState State { get; } = new() { Shown = 99, HiddenDepth = 99, InTag = 99, TextAllowed = 99, AfterHeading = 99, InputOffset = 37, Output = "prior" };
    /// <summary>같은 기본 HTML 전역.</summary>
    internal HtmlTextParserGlobals Globals { get; } = new();
    /// <summary>현재 중첩 정보 모드.</summary>
    internal QuickHelpHtmlState Mode { get; } = new();
    /// <summary>실제 공유 링크 BYTE/현재 부모 쓰기 위치.</summary>
    internal HtmlLinkTextState Links { get; } = new() { Base = 0x13200000 };
    /// <summary>현재 TYPE의 실제 입력 필드.</summary>
    internal QuickHelpTypeInfo Type { get; } = new();
    /// <summary>같은 정보 pane의 실제 기본 필드.</summary>
    internal GumpCreateNode Pane { get; } = new();
    /// <summary>명시 타입 입력 프로필.</summary>
    internal uint Profile { get; }
    /// <summary>현재 설명 getter의 입력.</summary>
    internal string Description { get; set; } = "Unit Label";
    /// <summary>실제 pane 외부 효과의 순서.</summary>
    internal List<string> Trace { get; } = [];
    /// <summary>실제 기록한 원문 BYTE 문자열.</summary>
    internal StringBuilder Stream { get; } = new();
    /// <summary>동적 분기/표시 결과를 공급하지 않고 원문 실행에 주었던 TYPE 필드만 설치한다.</summary>
    internal QuickHelpHtmlScene(uint profile, bool linked, uint flags)
    {
        Profile = profile; Links.Enabled = linked; Pane.Flags = flags; Type.Theme = (int)(profile % 6); Type.Flags1 = profile == 7 ? 0U : 0x10; Type.Flags2 = profile == 3 ? 0x100U : 0; Type.Classification = profile == 4 ? "" : "Class Label"; Type.Mana = profile is 2 or 5 ? "w.r.t" : "";
        // 단정도 NaN/음수/0은 원문 관찰과 같은 명시 입력이다.
        float Number(float value) => profile == 8 ? BitConverter.UInt32BitsToSingle(0x7fc00000) : profile == 5 ? -value : profile is 3 or 4 ? 0 : value;
        Type.Range = Number(23.75f); Type.Damage = Number(3.25f); Type.Cost = Number(17.75f);
    }
    /// <summary>원시 문자열만16진수로 직렬화한다.</summary>
    internal static string Hex(string text) => Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();
    /// <summary>native에 공급한 동일 고정%s/%d 서식 장치.번역/TYPE/분기를 계산하지 않는다.</summary>
    internal static string Print(string pattern, IReadOnlyList<PriestMenuArgument> arguments)
    {
        var result = new StringBuilder(); int argument = 0;
        // 원문 고정 변환 슬롯과 그 인자 종류만 처리한다.
        for (int i = 0; i < pattern.Length; i++)
        {
            if (pattern[i] != '%') { result.Append(pattern[i]); continue; } Assert.True(i + 1 < pattern.Length); char code = pattern[++i]; if (code == '%') { result.Append('%'); continue; } Assert.True(argument < arguments.Count); var value = arguments[argument++]; Assert.Contains(code, new[] { 's', 'd' }); Assert.Equal(code == 's', value.IsText); result.Append(code == 's' ? value.Text : value.Number.ToString(CultureInfo.InvariantCulture));
        }
        Assert.Equal(arguments.Count, argument); return result.ToString();
    }
    /// <summary>같은 현재 TYPE와 원문 pane/파일 효과를 공급한다.</summary>
    internal QuickHelpHtmlHooks Hooks() => new()
    {
        Type = () => Type, Description = () => Description, HitPoints = () => Profile == 6 ? 0 : Profile == 2 ? 25 : 101, Translate = text => text, Substitute = text => text, Config = text => text, Print = Print, Pane = () => Pane,
        PaneText = text => Trace.Add("Pane," + Hex(text)), PaneScroll = value => Trace.Add("Scroll," + value), PanePush = () => Trace.Add("Push"), PaneAttach = () => Trace.Add("PaneAttach"), Stream = text => Stream.Append(text)
    };
    /// <summary>실제 파서/링크/그림 helper의 출력과 현재 상태만 원본 관측 형식으로 직렬화한다.</summary>
    internal string Parse(string source, string topic, bool streaming)
    {
        var basic = new HtmlTextParserHooks(); HtmlTextParser.BindHtmlPictureText(basic); HtmlLinkText.BindHtmlLinkText(basic, Links); var html = new QuickHelpHtml(State, Globals, Mode, Profile != 0 ? 1U : 0, topic, streaming, new(), Hooks(), basic, Links); string output = html.Parse(source); string fields = string.Join(',', new uint[] { (uint)State.InputOffset, State.Shown, State.HiddenDepth, State.InTag, State.TextAllowed, State.AfterHeading, Globals.SuppressParagraph, Mode.InfoMode }); return Hex(output) + '|' + fields + '|' + Convert.ToHexString(Links.Bytes.Take(Links.Cursor).ToArray()).ToLowerInvariant() + '|' + Hex(Stream.ToString()) + '|' + string.Join(';', Trace);
    }
}
/// <summary>최초 독립 원본 결과를 비교에만 쓰며 관리 코드로 기대값을 만들지 않는다.</summary>
public sealed class QuickHelpHtmlNativeTests
{
    /// <summary>같은 읽기 전용 VFS의 보호 도움말 BYTE를 읽는다.</summary>
    internal static string Help() => Encoding.Latin1.GetString(GameFileSystem.Open(Path.Combine(BackgroundGifAssetsNativeTests.Root(), "originals")).ReadAllBytes("d/help.english"));
    /// <summary>native의 명시 JSON 입력에서 원시 DWORD를 읽는다.</summary>
    private static uint Number(JsonElement input, int index) => unchecked((uint)input[index].GetInt32());
    /// <summary>native의 명시 JSON 입력 문자열을 그대로 읽는다.</summary>
    private static string Text(JsonElement input, int index) => input[index].GetString()!;
    /// <summary>전체83행의 출력/필드·링크 원문·스트림/pane 순서를 대조한다.</summary>
    [Fact]
    public void All83NativeHelpTopicManaInfoStreamAndLinksMatch()
    {
        int count = 0;
        // 앞2열만 실행 입력으로 쓰고 마지막 독립 관찰은 비교에만 쓴다.
        foreach (string[] row in X86Fixture.Read("quick-help-html"))
        {
            Assert.Equal(3, row.Length); using var json = JsonDocument.Parse(row[1]); var input = json.RootElement; string result;
            if (row[0] == "M") result = QuickHelpHtmlScene.Hex(QuickHelpHtml.ManaPictureText(Text(input, 0), new(), input[1].GetInt32()));
            else if (row[0] == "B") { var state = new HtmlTextParserState { InputOffset = 1, TextAllowed = Number(input, 1) }; QuickHelpHtml.QuickHelpBeforeClose(state, Text(input, 0) + '/'); result = state.TextAllowed.ToString(CultureInfo.InvariantCulture); }
            else if (row[0] == "F") { var scene = new QuickHelpHtmlScene(Number(input, 1), Number(input, 3) != 0, Number(input, 4)); result = scene.Parse(Help(), Text(input, 0), Number(input, 2) != 0); }
            else { Assert.Equal("D", row[0]); var scene = new QuickHelpHtmlScene(Number(input, 2), Number(input, 4) != 0, Number(input, 5)); result = scene.Parse(Text(input, 0), Text(input, 1), Number(input, 3) != 0); }
            Assert.True(row[2] == result, $"QuickHelp 원본행{count} {row[0]} {row[1]}\n기대:{row[2]}\n실제:{result}"); count++;
        }
        Assert.Equal(83, count);
    }
    /// <summary>중첩 링크가 같은 BYTE를 쓰되 부모 cursor는 복구하는 원본3행을 대조한다.</summary>
    [Fact]
    public void All3NativeNestedInfoAndParentLinkCursorObservationsMatch()
    {
        int count = 0;
        // 앞 열의 설명 getter만 입력으로 바꾸고 두 파서는 같은 실제 저장소를 사용한다.
        foreach (string[] row in X86Fixture.Read("quick-help-html-nested"))
        {
            Assert.Equal(2, row.Length); var scene = new QuickHelpHtmlScene(1, true, 8) { Description = X86Fixture.Text(row[0]) }; Assert.Equal(row[1], scene.Parse("<a name=\"unitType\"><info><a href=\"#outer\">Out</a></a>\nSTOP", "unitType", true)); Assert.Equal("outer", HtmlLinkText.ResolveHtmlLink(scene.Links, scene.Links.Base)); count++;
        }
        Assert.Equal(3, count);
    }
}
