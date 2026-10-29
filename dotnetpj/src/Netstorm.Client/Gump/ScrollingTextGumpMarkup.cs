namespace Netstorm.Client.Gump; // 같은 실제 다중 줄 문맥/태그/입력 버퍼를 쓰는 스타일 파서다.

/// <summary>생성/표시와 같은 실제 노드를 조회한다.</summary>
public sealed class ScrollingTextGumpMarkupHooks
{
    /// <summary>현재 다중 줄 파생 노드.</summary>
    public Func<uint, ScrollingTextGumpNode?>? Node { get; set; }
    /// <summary>같은 기본 노드.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
}
/// <summary>cpppj/src/client/ScrollingTextGumpMarkup.cpp의 구간별 추가 통지/재그리기와 현재 문맥/태그/제한 재조회.</summary>
public sealed class ScrollingTextGumpMarkup
{
    /// <summary>같은 실제 버퍼 추가/dirty/재그리기 처리기.</summary>
    private readonly ScrollingTextGump _text;
    /// <summary>같은 짧은 명령/팔레트 처리기.</summary>
    private readonly StyleText _style;
    /// <summary>같은 공통/파생 그림 처리기.</summary>
    private readonly Markup _pictures;
    /// <summary>같은 대괄호/옵션 처리기.</summary>
    private readonly StyleTextMarkup _markup;
    /// <summary>현재 실제 노드 조회.</summary>
    private readonly ScrollingTextGumpMarkupHooks _hooks;
    /// <summary>같은 판본/저장소의 처리기를 빌린다.</summary>
    public ScrollingTextGumpMarkup(ScrollingTextGump text, StyleText style, Markup pictures, StyleTextMarkup markup, ScrollingTextGumpMarkupHooks hooks)
    { ArgumentNullException.ThrowIfNull(text); ArgumentNullException.ThrowIfNull(style); ArgumentNullException.ThrowIfNull(pictures); ArgumentNullException.ThrowIfNull(markup); ArgumentNullException.ThrowIfNull(hooks); if (hooks.Node is null || hooks.Base is null) throw new ArgumentException("다중 줄 스타일 공유 저장소 필요"); _text = text; _style = style; _pictures = pictures; _markup = markup; _hooks = hooks; }
    /// <summary>태그/문맥이 실제 표시/입력과 같은 객체에 있어야 한다.</summary>
    private ScrollingTextGumpNode Node(uint token)
    { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("다중 줄 스타일 공유 노드 없음"); return n; }
    /// <summary>입력을 복사한 뒤 첫NUL까지만 읽는다.T 태그는 호출 간 실제 필드에 남으며 끝 빈 구간도 버퍼 추가를 수행한다.</summary>
    public void Parse(uint token, ReadOnlySpan<byte> text)
    {
        uint[] context = (uint[])Node(token).TextContext.Clone(); int end = text.IndexOf((byte)0); int count = end < 0 ? text.Length : end; byte[] source = new byte[checked(count + 1)]; text[..count].CopyTo(source); int position = 0;
        // 명령 앞 구간을 추가한 뒤 현재 태그/기본 문맥을 각 다음 구간에서 다시 읽는다.
        while (true)
        {
            int relative = source.AsSpan(position, count - position).IndexOf((byte)'~'); var remaining = source.AsSpan(position);
            if (relative < 0) { _text.AppendText(token, remaining, context, -1, Node(token).Metadata); return; } int mark = position + relative;
            if (mark > position) _text.AppendText(token, remaining, context, mark - position, Node(token).Metadata); byte command = source[mark + 1]; int next = mark + 2;
            if (command == '~') _text.AppendText(token, [(byte)'~', 0], context, 1, Node(token).Metadata);
            else if (command == '[')
            {
                int closing = source.AsSpan(next, count - Math.Min(next, count)).IndexOf((byte)']');
                if (closing >= 0)
                {
                    int close = next + closing; _markup.ApplyScrolling(source.AsSpan(next, close - next), context, ref Node(token).Metadata, () => Node(token).Callback != 0,
                        (type, frame, width, height) => _text.AppendPicture(token, _pictures, type, frame, Node(token).Metadata, width, height)); next = close + 1;
                }
            }
            else if (!_style.ApplyShortCommand(command, context, Node(token).TextContext, Node(token).Callback != 0)) next = mark + 1;
            position = Math.Min(next, count);
        }
    }
}
