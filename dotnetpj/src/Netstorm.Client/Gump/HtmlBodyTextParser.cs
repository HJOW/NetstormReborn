using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 부모의 본문/링크 BYTE와 현재 HTML 부수 효과를 연결한다.

/// <summary>cpppj/src/client/RawMenuControls.cpp의 RenderEmbeddedTell 중 실제 파서 게시 경계만 옮긴다.</summary>
public static class HtmlBodyTextParser
{
    /// <summary>같은 실제 부모 영역의 접두부를 효과 전후/실패 때에도 게시하고 링크 원시 BYTE를 공유한다.</summary>
    public static void Parse(string source, Memory<byte> body, Memory<byte> linkBytes, HtmlLinkTextState links, HtmlTextParserGlobals globals, HtmlTextParserHooks basic, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        if (body.Length == 0 || linkBytes.Length == 0) throw new ArgumentException("HTML 부모 실제 출력 영역 필요"); var state = new HtmlTextParserState(); links.Cursor = 0; links.Capacity = linkBytes.Length; links.Bytes.Clear(); links.Bytes.AddRange(linkBytes.ToArray());
        // 실제 효과가 같은 현재 접두부를 관찰하도록 종료 NUL까지 게시한다.
        void Publish() { int count = Math.Min(state.Output.Length, body.Length - 1);
            // 원문 BYTE를 같은 실제 부모 접두부에만 쓰며 뒤쪽은 보존한다.
            for (int i = 0; i < count; i++) body.Span[i] = checked((byte)state.Output[i]); body.Span[count] = 0; }
        var effects = new HtmlTextParserHooks { InsideTag = basic.InsideTag, BeforeClose = basic.BeforeClose, AfterTag = basic.AfterTag };
        if (basic.Command is { } command) effects.Command = (s, raw) => { Publish(); command(s, raw); Publish(); }; if (basic.Condition is { } condition) effects.Condition = (s, raw, close) => { Publish(); condition(s, raw, close); Publish(); }; if (basic.Picture is { } picture) effects.Picture = (s, raw) => { Publish(); return picture(s, raw); }; if (basic.Background is { } background) effects.Background = (s, raw) => { Publish(); background(s, raw); Publish(); };
        effects.Link = (_, raw, close) => { Publish(); links.Bytes.Clear(); links.Bytes.AddRange(linkBytes.ToArray()); string result = HtmlLinkText.Convert(links, raw, close); links.Bytes.ToArray().CopyTo(linkBytes); return result; };
        try { new HtmlTextParser(state, globals, effects, edition).Parse(source, body.Length); } catch { Publish(); throw; } Publish();
    }
}
