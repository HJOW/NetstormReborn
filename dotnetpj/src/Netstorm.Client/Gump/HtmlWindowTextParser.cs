using System.Text;

namespace Netstorm.Client.Gump; // 일반 HTML 창의 같은8192BYTE 링크를 두 번 파서/현재 효과에 연결한다.

/// <summary>cpppj/src/client/RawMenuHtml.inc의 ParseHtml/HtmlLink 소유 경계.전체 메뉴 소유자와 구별한다.</summary>
public sealed class HtmlWindowTextParser
{
    /// <summary>현재 같은 실제 부모 저장소.</summary>
    private readonly Func<uint, HtmlWindowGumpNode?> _node;
    /// <summary>같은 현재 파서 전역.</summary>
    private readonly HtmlTextParserGlobals _globals;
    /// <summary>현재 실제 명령/조건/그림/배경 효과.</summary>
    private readonly HtmlTextParserHooks _basic;
    /// <summary>부모 수명의 현재 링크 시작 토큰.</summary>
    private readonly Dictionary<uint, uint> _bases = [];
    /// <summary>호스트 포인터 대신8192BYTE 단위로 할당하는 원문 양수 주소 별칭.</summary>
    private uint _nextBase = 0x63000000;
    /// <summary>현재 같은 저장소/파서 장치를 빌린다.</summary>
    public HtmlWindowTextParser(Func<uint, HtmlWindowGumpNode?> node, HtmlTextParserGlobals globals, HtmlTextParserHooks basic) { ArgumentNullException.ThrowIfNull(node); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(basic); _node = node; _globals = globals; _basic = basic; }
    /// <summary>현재 실제 부모가 없으면 반납되었거나 잘못된 토큰이다.</summary>
    private HtmlWindowGumpNode Node(uint token) => _node(token) ?? throw new InvalidOperationException("HTML 현재 링크 소유 부모 없음");
    /// <summary>같은 상태의 hidden/shown을 유지하고 매 호출 링크cursor만0으로 되돌린다.각 링크 효과 직후 실제 부모에 게시한다.</summary>
    public string Parse(HtmlTextParserState state, uint root, string source, uint capacity)
    {
        var links = new HtmlLinkTextState { Enabled = root != 0, Capacity = 8192 };
        if (root != 0) { if (!_bases.TryGetValue(root, out uint address)) { if (_nextBase > 0x7fffe000) throw new OverflowException("HTML 부모 링크 주소 별칭 소진"); address = _nextBase; _bases.Add(root, address); _nextBase += 8192; } links.Base = address; links.Bytes.AddRange(Node(root).Html); }
        var effects = new HtmlTextParserHooks { Command = _basic.Command, Condition = _basic.Condition, Picture = _basic.Picture, Background = _basic.Background, InsideTag = _basic.InsideTag, BeforeClose = _basic.BeforeClose, AfterTag = _basic.AfterTag };
        effects.Link = (_, raw, closing) => { if (root != 0) { links.Bytes.Clear(); links.Bytes.AddRange(Node(root).Html); } string output = HtmlLinkText.Convert(links, raw, closing); if (root != 0) links.Bytes.ToArray().CopyTo(Node(root).Html, 0); return output; };
        return new HtmlTextParser(state, _globals, effects).Parse(source, unchecked((int)capacity));
    }
    /// <summary>죽음 비트 대신 실제 부모 반납 시점을 검사해 주소부터 첫 NUL까지 읽는다.</summary>
    public string Resolve(uint token) {
        // 현재 부모별 주소 범위와 실제 소유 저장소를 순서대로 조회한다.
        foreach (var (root, address) in _bases) if (token >= address && token - address < 8192) { var n = _node(root); if (n is null) break; int first = (int)(token - address), end = Array.IndexOf(n.Html, (byte)0, first); if (end < 0) throw new ArgumentException("HTML 실제 링크 NUL 범위 초과"); return Encoding.Latin1.GetString(n.Html, first, end - first); } throw new ArgumentOutOfRangeException(nameof(token), "HTML 반납되었거나 없는 링크 주소");
    }
    /// <summary>상위 실제 소유자가 부모를 반납할 때 별칭도 지운다.파서가 임의로 UI 수명을 끝내지 않는다.</summary>
    public void Forget(uint root) => _bases.Remove(root);
}
