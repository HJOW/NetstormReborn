using System.Text;

namespace Netstorm.Assets;

/// <summary>원본 Htmlgump의 조건 평가와 본문 표시/숨김 규칙을 재현한다. 명령은 실행하지 않는다.</summary>
public static class MissionConditions
{
    /// <summary>원본 00487620이 따옴표를 포함하여 읽는 피연산자 최대 문자 수(0x4e).</summary>
    private const int OperandLimit = 78;

    /// <summary>본문 전체를 치환한 뒤 조건 태그를 제거하고 표시되는 텍스트·HTML·명령만 남긴다.</summary>
    public static string Filter(string body, ConfigStore settings)
    {
        string text = settings.Expand(body);
        var result = new StringBuilder(text.Length);
        bool visible = true;
        int position = 0;
        // 태그 사이의 본문과 각 태그를 왼쪽부터 한 번씩 처리한다.
        while (position < text.Length)
        {
            int start = text.IndexOf('<', position);
            if (start < 0)
            {
                if (visible)
                {
                    result.Append(text.AsSpan(position));
                }
                break;
            }
            if (visible)
            {
                result.Append(text.AsSpan(position, start - position));
            }
            bool open = text.AsSpan(start).StartsWith("<?", StringComparison.Ordinal);
            bool close = text.AsSpan(start).StartsWith("</?", StringComparison.Ordinal);
            int end = text.IndexOf('>', start + 1);
            if (end < 0)
            {
                // 불완전한 조건 태그는 제거하고, 다른 미완성 HTML은 표시 중일 때 그대로 남긴다.
                if (visible && !open && !close)
                {
                    result.Append(text.AsSpan(start));
                }
                break;
            }
            if (close)
            {
                visible = true;
            }
            else if (open)
            {
                // 원본 0046c810은 숨김 중 여는 조건을 무시하며 첫 닫는 조건에서 표시를 재개한다.
                if (visible)
                {
                    visible = Evaluate(text[(start + 2)..end], settings);
                }
            }
            else if (visible)
            {
                result.Append(text.AsSpan(start, end - start + 1));
            }
            position = end + 1;
        }
        return result.ToString();
    }

    /// <summary>태그의 첫 '?' 다음 식을 평가한다. 접두어는 치환된 본문 기준이며 피연산자는 다시 치환한다.</summary>
    public static bool Evaluate(string expression, ConfigStore settings)
    {
        int position = 0;
        bool negate = Consume(expression, ref position, '!');
        int comparison = Consume(expression, ref position, '?') ? 1 : 0;
        if (Consume(expression, ref position, 'g'))
        {
            comparison = Consume(expression, ref position, 'e') ? 3 : 2;
        }
        if (Consume(expression, ref position, 'l'))
        {
            comparison = Consume(expression, ref position, 'e') ? 5 : 4;
        }
        // 원본은 접두어 앞과 뒤의 !를 각각 부정으로 취급하며 두 번 있어도 서로 상쇄하지 않는다.
        if (Consume(expression, ref position, '!'))
        {
            negate = true;
        }
        string left = settings.Expand(ReadOperand(expression, ref position, comparison == 0 ? "=>" : "="));
        bool value;
        if (comparison == 0)
        {
            value = DecimalPrefix(left) != 0;
        }
        else
        {
            string right = settings.Expand(ReadOperand(expression, ref position, ">"));
            value = comparison switch
            {
                1 => string.Equals(left, right, StringComparison.Ordinal),
                2 => DecimalPrefix(left) > DecimalPrefix(right),
                3 => DecimalPrefix(left) >= DecimalPrefix(right),
                4 => DecimalPrefix(left) < DecimalPrefix(right),
                5 => DecimalPrefix(left) <= DecimalPrefix(right),
                _ => false,
            };
        }
        return negate ? !value : value;
    }

    /// <summary>접두어 문자를 공백을 건너뛰지 않고 대소문자 무시로 읽는다.</summary>
    private static bool Consume(string text, ref int position, char expected)
    {
        if (position < text.Length && char.ToLowerInvariant(text[position]) == expected)
        {
            position++;
            return true;
        }
        return false;
    }

    /// <summary>00487620처럼 공백·탭과 따옴표를 처리하고 지정한 구분자까지 피연산자를 읽는다.</summary>
    private static string ReadOperand(string text, ref int position, string delimiters)
    {
        // 피연산자 앞의 공백·탭만 건너뛴다. 부정/비교 접두어를 다시 검사하지 않는다.
        while (position < text.Length && text[position] is ' ' or '\t')
        {
            position++;
        }
        var result = new StringBuilder();
        bool quoted = false;
        int consumed = 0;
        // 따옴표 밖의 구분자 또는 원본 문자 한도까지 읽으며 따옴표 자체는 결과에서 제외한다.
        while (position < text.Length && consumed < OperandLimit)
        {
            char current = text[position];
            if (!quoted && delimiters.Contains(current))
            {
                break;
            }
            if (current == '"')
            {
                quoted = !quoted;
            }
            else
            {
                result.Append(current);
            }
            position++;
            consumed++;
        }
        // 원본은 연속 구분자를 모두 소비한 뒤 다음 피연산자를 읽는다.
        while (position < text.Length && delimiters.Contains(text[position]))
        {
            position++;
        }
        return result.ToString().TrimEnd(' ', '\t');
    }

    /// <summary>원본 MSVC atol처럼 부호 있는 10진 접두어를 32비트 오버플로로 읽고 숫자가 없으면 0을 반환한다.</summary>
    private static int DecimalPrefix(string text)
    {
        int position = 0;
        // C 런타임의 기본 ASCII 공백 문자를 건너뛴다.
        while (position < text.Length && text[position] is ' ' or '\t' or '\r' or '\n' or '\v' or '\f')
        {
            position++;
        }
        bool negative = position < text.Length && text[position] == '-';
        if (position < text.Length && text[position] is '+' or '-')
        {
            position++;
        }
        int value = 0;
        // 소수점·16진 표기·후행 문자 앞까지만 누적하고 정수 넘침은 원본처럼 버린다.
        while (position < text.Length && text[position] is >= '0' and <= '9')
        {
            value = unchecked(value * 10 + text[position] - '0');
            position++;
        }
        return negative ? unchecked(-value) : value;
    }
}
