using System.Globalization;
using System.Text;

namespace Netstorm.Client.Gump; // 원본 HTML 링크의 연속 NUL 문자열과 실제 T태그 소유 영역이다.

/// <summary>원본+10 쓰기 포인터를 호스트 토큰/실제 BYTE 위치로 나타낸다.</summary>
public sealed class HtmlLinkTextState
{
    /// <summary>원본 포인터 대신 쓰는 문자열 저장소의 시작 토큰.</summary>
    public uint Base { get; set; } = 0x61010000;
    /// <summary>출력 설정이 공급하는 물리 링크 버퍼 한도.</summary>
    public int Capacity { get; set; } = 65536;
    /// <summary>원본+10이0이면 문자열/T태그를 모두 만들지 않는다.</summary>
    public bool Enabled { get; set; } = true;
    /// <summary>NUL을 포함한 실제 소유 BYTE.커서 되돌림은 뒤쪽 BYTE를 지우지 않는다.</summary>
    public List<byte> Bytes { get; } = [];
    /// <summary>파서별 현재 위치.중첩 파서는 BYTE를 공유하고 부모 위치는 복구한다.</summary>
    public int Cursor { get; set; }
}
/// <summary>cpppj/src/client/HtmlLinkText.h/.cpp의 원문0046bfc0 저장/서식/조회.</summary>
public static class HtmlLinkText
{
    /// <summary>열림의 첫 인용 내부에서#을 한 번 생략한다.닫힘/인용 없음은 저장 없는 T0이다.</summary>
    public static string Convert(HtmlLinkTextState links, string raw, bool closing)
    {
        ArgumentNullException.ThrowIfNull(links); ArgumentNullException.ThrowIfNull(raw); if (!links.Enabled) return ""; uint token = 0; int nul = raw.IndexOf('\0'); if (nul >= 0) raw = raw[..nul]; int quote = raw.IndexOf('"'), end = raw.IndexOf('>');
        if (!closing && quote >= 0 && (end < 0 || quote < end))
        {
            int first = quote + 1; if (first < raw.Length && raw[first] == '#') first++; int last = first;
            // 인용 내부의 공백/대소문자/프로토콜을 그대로 보존한다.
            while (last < raw.Length && raw[last] is not '"' and not '>') last++;
            long required = (long)links.Cursor + last - first + 1; if (links.Base == 0 || links.Cursor < 0 || links.Cursor > links.Bytes.Count || required > links.Capacity || (ulong)links.Base + (ulong)links.Cursor + (ulong)(last - first) >= uint.MaxValue) throw new ArgumentException("HTML 링크 실제 저장소/토큰 범위 초과"); token = links.Base + (uint)links.Cursor; if (links.Bytes.Count < required) links.Bytes.AddRange(new byte[checked((int)required - links.Bytes.Count)]);
            // 현재 접두부만 덮어쓰고 이전 링크 영역의 뒤쪽은 보존한다.
            for (int i = first; i < last; i++) links.Bytes[links.Cursor + i - first] = checked((byte)raw[i]); links.Cursor = (int)required; links.Bytes[links.Cursor - 1] = 0;
        }
        return "~[T" + token.ToString(CultureInfo.InvariantCulture) + ']';
    }
    /// <summary>현재 유효 저장소의 토큰부터 첫 NUL까지 원시 대상 문자열을 읽는다.</summary>
    public static string ResolveHtmlLink(HtmlLinkTextState links, uint token)
    {
        ArgumentNullException.ThrowIfNull(links); if (!links.Enabled || token < links.Base || token - links.Base >= (uint)links.Bytes.Count) throw new ArgumentOutOfRangeException(nameof(token), "현재 HTML 링크 토큰 누락"); int first = (int)(token - links.Base), last = links.Bytes.IndexOf(0, first); if (last < 0) throw new ArgumentException("HTML 링크 문자열 NUL 누락"); return Encoding.Latin1.GetString(links.Bytes.GetRange(first, last - first).ToArray());
    }
    /// <summary>같은 실제 소유 저장소를 기본/파생 HTML의 현재 링크 효과에 연결한다.</summary>
    public static void BindHtmlLinkText(HtmlTextParserHooks hooks, HtmlLinkTextState links) { ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(links); hooks.Link = (_, raw, closing) => Convert(links, raw, closing); }
}
