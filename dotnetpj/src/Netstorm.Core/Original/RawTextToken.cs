using System.Text;

namespace Netstorm.Core.Original; // HTML/Config가 공유하는 원본 토큰 소비 영역이다.

/// <summary>따옴표를 제외한 원시 BYTE 문자열과 구분자 뒤의 현재 입력 위치다.</summary>
/// <param name="Text">Latin1으로 보존한 최대78입력 BYTE의 본문.</param><param name="Next">같은 입력의 다음 위치.</param>
public readonly record struct RawTextTokenResult(string Text, int Next);

/// <summary>cpppj/src/o/RawTextToken.h/.cpp의 PATCH00487620/CD0041a3d0 공통 토큰 helper.</summary>
public static class RawTextToken
{
    /// <summary>원본80BYTE 지역 버퍼가 허용하는 입력 BYTE 수.인용부호도 소비한다.</summary>
    private const int InputLimit = 78;
    /// <summary>첫 NUL 앞의 BYTE 문자열만 빌린다.</summary>
    private static string BeforeNul(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>앞 space/tab·인용·입력78BYTE 제한·연속 구분자·뒤 공백 첫 BYTE 보존을 적용한다.</summary>
    public static RawTextTokenResult Read(string source, int offset = 0, string delimiters = " \t\n,")
    {
        ArgumentNullException.ThrowIfNull(source); ArgumentNullException.ThrowIfNull(delimiters); source = BeforeNul(source); delimiters = BeforeNul(delimiters); if ((uint)offset > (uint)source.Length) throw new ArgumentOutOfRangeException(nameof(offset), "원본 토큰 NUL/입력 위치 범위 오류");
        // 앞 space/tab만 건너뛰고 나머지 공백은 토큰의 실제 BYTE로 남긴다.
        while (offset < source.Length && source[offset] is ' ' or '\t') offset++;
        var output = new StringBuilder(); bool quoted = false; int remaining = InputLimit;
        // 인용부호도 입력 한도를 소비하며 제한 중단 때에도 연속 구분자는 건너뛴다.
        while (offset < source.Length)
        {
            char item = source[offset]; if (remaining == 0 || (!quoted && delimiters.Contains(item)))
            {
                // 한도 때문에 멈춘 경우 현재 인용 상태와 무관하게 구분자를 소비한다.
                while (offset < source.Length && delimiters.Contains(source[offset])) offset++;
                break;
            }
            if (item == '"') quoted = !quoted; else output.Append(item); offset++; remaining--;
        }
        // 뒤 공백을 지워도 첫 출력 BYTE는 보존한다.
        while (output.Length > 1 && output[^1] is ' ' or '\t') output.Length--;
        return new(output.ToString(), offset);
    }
}
