using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

// 서버 주소의 원문 바이트 판독만 수행하며 통신 장치를 열지 않는다.
namespace Netstorm.Client;

/// <summary>원문 지역80BYTE에서 선택한 host와 별도 DWORD 포트다.</summary>
/// <param name="Host">첫 성공 패턴의 콜론 앞 그룹 또는 원래 C문자열이다.</param>
/// <param name="Port">명시 포트가 비영이면 마지막에 이를 적용한 현재 포트다.</param>
public readonly record struct RootServerEndpoint(string Host, uint Port);

/// <summary>00458e70/004b3ed0의 두 고정 패턴과 현재 출력 물리를 그대로 판독한다.</summary>
public static class RootServerRecords
{
    /// <summary>첫 NUL 뒤의 실제 저장소를 C문자열 입력에 포함하지 않는다.</summary>
    private static ReadOnlySpan<byte> Text(ReadOnlySpan<byte> text)
    {
        int end = text.IndexOf((byte)0);
        return end < 0 ? text : text[..end];
    }

    /// <summary>GetMatch의 최대 count 복사와 마지막 NUL만 현재 목적 span에 쓴다.</summary>
    private static void Copy(Span<byte> target, ReadOnlySpan<byte> source)
    {
        if (target.IsEmpty) return;
        int count = Math.Min(target.Length - 1, source.Length);
        source[..count].CopyTo(target); target[count] = 0;
    }

    /// <summary>유효 행 parser의 출력만0으로 만든 뒤 첫 세미콜론과 선택 subscriber의 원문 atol을 해석한다.</summary>
    public static void Read(ReadOnlySpan<byte> record, Span<byte> endpoint, Span<byte> name, IntBox? subscriber)
    {
        endpoint.Clear(); name.Clear();
        if (subscriber is not null) subscriber.Value = 0;
        record = Text(record);
        int first = record.IndexOf((byte)';');
        if (first < 0) return;
        ReadOnlySpan<byte> tail = record[(first + 1)..];
        int second = tail.IndexOf((byte)';');
        Copy(endpoint, record[..first]); Copy(name, second < 0 ? tail : tail[..second]);
        if (subscriber is not null)
        {
            ReadOnlySpan<byte> suffix = second < 0 ? [] : tail[(second + 1)..];
            int newline = suffix.IndexOf((byte)'\n');
            if (newline >= 0) suffix = suffix[..newline];
            Span<byte> temporary = stackalloc byte[80]; temporary.Clear(); Copy(temporary, suffix);
            subscriber.Value = ConfigNumbers.ParseLong(Encoding.Latin1.GetString(Text(temporary)));
        }
    }

    /// <summary>첫 성공한 콜론/숫자 패턴을 선택하고 매치가 없을 때 현재 기본 포트를 조회한 뒤 명시 포트를 적용한다.</summary>
    public static RootServerEndpoint Resolve(ReadOnlySpan<byte> input, uint port, Func<int>? configuredPort)
    {
        ReadOnlySpan<byte> text = Text(input);
        if (text.Length >= 80) throw new ArgumentOutOfRangeException(nameof(input), "원문 root endpoint 지역80BYTE 물리 부족");
        string host = Encoding.Latin1.GetString(text); uint selected = 0; bool matched = false;
        int begin = 0;
        // 실패한 콜론을 가로질러 host를 합치지 않고 왼쪽부터 첫 성공 패턴만 선택한다.
        for (int colon = text.IndexOf((byte)':'); colon >= 0;)
        {
            if (colon + 1 < text.Length && text[colon + 1] is >= (byte)'0' and <= (byte)'9')
            {
                int end = colon + 1;
                // 연속된 십진 숫자 그룹만 탐욕적으로 소비한다.
                while (end < text.Length && text[end] is >= (byte)'0' and <= (byte)'9') end++;
                host = Encoding.Latin1.GetString(text[begin..colon]);
                selected = unchecked((uint)ConfigNumbers.ParseLong(Encoding.Latin1.GetString(text[(colon + 1)..end])));
                matched = true; break;
            }
            begin = colon + 1;
            int relative = text[begin..].IndexOf((byte)':');
            colon = relative < 0 ? -1 : begin + relative;
        }
        if (!matched) selected = unchecked((uint)(configuredPort ?? throw new InvalidOperationException("현재 rootServerBasePort 조회 미연결"))());
        if (port != 0) selected = port;
        return new(host, selected);
    }
}
