using System.Globalization;
using System.Numerics;
using System.Text;
using System.Text.RegularExpressions;

// 개발자 명령의 ASCII 토큰과 선택한 scanf 입력을 원문 순서로 소비한다.
namespace Netstorm.Core.Original;

/// <summary>원문 %255s·%d·%f 분기를 위한 물리 BYTE 입력이다. 미기입 stack 값을 합성하지 않는다.</summary>
public sealed class RawDeveloperCommandInput
{
    /// <summary>첫NUL까지 보존한 Latin1 BYTE 문자열이다.</summary>
    private readonly string _input;
    /// <summary>현재 선택한 sscanf의 다음 입력 위치다.</summary>
    private int _position;
    /// <summary>숫자 실수의 현재 접두부를 판독한다. 나머지 문자는 다음 변환에 남긴다.</summary>
    private static readonly Regex FloatPrefix = new(@"\G[+-]?(?:0x(?:[0-9a-f]+(?:\.[0-9a-f]*)?|\.[0-9a-f]+)(?:p[+-]?[0-9]+)?|(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:e[+-]?[0-9]+)?|inf(?:inity)?|nan(?:\([a-z0-9_]*\))?)", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
    /// <summary>첫NUL을 자르고 첫토큰255BYTE의 ASCII 대문자만 소문자로 바꾼다.</summary>
    public RawDeveloperCommandInput(ReadOnlySpan<byte> input)
    {
        int end = input.IndexOf((byte)0); _input = Encoding.Latin1.GetString(end < 0 ? input : input[..end]);
        string token = Word() ?? throw new ArgumentException("개발자 명령의 미정 첫 토큰");
        Key = string.Create(token.Length, token, static (span, text) =>
        {
            // 실제 명령 이름의 ASCII 범위만 변환하고8비트 문자는 보존한다.
            for (int index = 0; index < text.Length; index++) span[index] = text[index] is >= 'A' and <= 'Z' ? (char)(text[index] + 32) : text[index];
        });
    }
    /// <summary>원문 비교에 사용하는 소문자 첫토큰이다.</summary>
    public string Key { get; }
    /// <summary>CRT C 로캘의 여섯 공백 BYTE만 판정한다.</summary>
    private static bool Space(char c) => c is ' ' or '\t' or '\r' or '\n' or '\v' or '\f';
    /// <summary>현재 변환 앞의 공백을 건너뛴다.</summary>
    private void SkipSpace() { while (_position < _input.Length && Space(_input[_position])) _position++; } // 연속 원문 공백만 소비한다.
    /// <summary>새 scanf의 %*s로 첫 명령 전체를 소비한다.255BYTE 제한은 적용하지 않는다.</summary>
    public void BeginArguments()
    {
        _position = 0; SkipSpace();
        // 명령키 비교 폭과 구별하여 scanf의 생략 토큰 전체를 소비한다.
        while (_position < _input.Length && !Space(_input[_position])) _position++;
    }
    /// <summary>현재 %255s 변환이며 남은 입력이 없으면 null이다.</summary>
    public string? Word()
    {
        SkipSpace(); int begin = _position;
        // 문자열 변환은255BYTE에서 멈추며 뒤 문자를 다음 변환에 남긴다.
        while (_position < _input.Length && _position - begin < 255 && !Space(_input[_position])) _position++;
        return begin == _position ? null : _input[begin.._position];
    }
    /// <summary>현재 %d의 부호/십진 접두부를 signed64로 포화한 뒤 하위DWORD를 쓴다.</summary>
    public bool Integer(out int value)
    {
        SkipSpace(); int begin = _position; value = 0;
        if (_position < _input.Length && _input[_position] is '+' or '-') _position++;
        int digits = _position;
        // 첫 비숫자에서 멈추므로12x의 다음 정수 변환은 실패한다.
        while (_position < _input.Length && _input[_position] is >= '0' and <= '9') _position++;
        if (_position == digits) return false;
        BigInteger number = BigInteger.Parse(_input.AsSpan(begin, _position - begin), NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture);
        long stored = number > long.MaxValue ? long.MaxValue : number < long.MinValue ? long.MinValue : (long)number;
        value = unchecked((int)stored);
        return true;
    }
    /// <summary>첫 인자부터 원문 한 정수 변환을 수행한다.</summary>
    public bool One(out int value) { BeginArguments(); return Integer(out value); }
    /// <summary>첫 변환이 성공한 경우에만 둘째 정수를 읽고 성공 개수를 반환한다.</summary>
    public int Two(out int first, out int second) { BeginArguments(); second = 0; if (!Integer(out first)) return 0; return Integer(out second) ? 2 : 1; }
    /// <summary>미기입 원문 정수값을0으로 대신하지 않고 명시 실패한다.</summary>
    public int Required() => One(out int value) ? value : throw new ArgumentException("개발자 명령의 원문 미정 정수 입력");
    /// <summary>float32의 십진/16진/NaN 입력을 읽으며 미완성 지수는 원문 미기입 입력으로 거부한다.</summary>
    public float RequiredFloat()
    {
        BeginArguments(); SkipSpace(); Match match = FloatPrefix.Match(_input, _position);
        if (!match.Success) throw new ArgumentException("개발자 다리 스캔 주기의 원문 미정 입력");
        string text = match.Value; bool negative = text.StartsWith('-'); string unsigned = text.TrimStart('+', '-').ToLowerInvariant();
        char tail = match.Index + match.Length < _input.Length ? char.ToLowerInvariant(_input[match.Index + match.Length]) : '\0';
        bool hex = unsigned.StartsWith("0x", StringComparison.Ordinal);
        if (tail == (hex ? 'p' : 'e') || (!hex && unsigned == "0" && tail == 'x') || (unsigned == "inf" && tail == 'i') || (unsigned == "nan" && tail == '('))
            throw new ArgumentException("개발자 실수의 원문 미완성 변환");
        if (unsigned.StartsWith("inf", StringComparison.Ordinal)) return negative ? float.NegativeInfinity : float.PositiveInfinity;
        if (unsigned.StartsWith("nan", StringComparison.Ordinal))
        {
            uint bits = unsigned == "nan(ind)" ? 0xffc00000u : unsigned == "nan(snan)" ? 0x7f800001u : 0x7fffffffu;
            return BitConverter.UInt32BitsToSingle(bits | (negative ? 0x80000000u : 0));
        }
        if (hex) return HexFloat(unsigned[2..], negative);
        return float.Parse(text, NumberStyles.Float, CultureInfo.InvariantCulture);
    }
    /// <summary>정확한2진 정수와 지수로16진 입력을 직접 float32에 최근접 짝수 반올림한다.</summary>
    private static float HexFloat(string input, bool negative)
    {
        int mark = input.IndexOf('p'); string significand = mark < 0 ? input : input[..mark];
        BigInteger exponent = mark < 0 ? 0 : BigInteger.Parse(input[(mark + 1)..], CultureInfo.InvariantCulture);
        int dot = significand.IndexOf('.'), fraction = dot < 0 ? 0 : significand.Length - dot - 1;
        BigInteger value = BigInteger.Parse("0" + significand.Replace(".", ""), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture);
        uint sign = negative ? 0x80000000u : 0;
        if (value.IsZero) return BitConverter.UInt32BitsToSingle(sign);
        BigInteger power = exponent - 4L * fraction; long length = value.GetBitLength(); BigInteger highest = power + length - 1;
        if (highest > 127) return BitConverter.UInt32BitsToSingle(sign | 0x7f800000);
        if (highest < -150) return BitConverter.UInt32BitsToSingle(sign);
        if (highest < -126) return BitConverter.UInt32BitsToSingle(sign | (uint)RoundBinary(value, checked((int)(-149 - power))));
        BigInteger rounded = RoundBinary(value, checked((int)(length - 24))); int normalized = (int)highest;
        if (rounded == 0x1000000) { rounded >>= 1; normalized++; }
        uint bits = normalized > 127 ? 0x7f800000 : ((uint)(normalized + 127) << 23) | ((uint)rounded & 0x7fffff);
        return BitConverter.UInt32BitsToSingle(sign | bits);
    }
    /// <summary>우측 절삭의 정확한 절반을 짝수로 처리하고 음수 shift는 정수 좌측 이동으로 유지한다.</summary>
    private static BigInteger RoundBinary(BigInteger value, int shift)
    {
        if (shift <= 0) return value << -shift;
        BigInteger quotient = value >> shift, remainder = value - (quotient << shift), half = BigInteger.One << (shift - 1);
        return remainder > half || (remainder == half && !quotient.IsEven) ? quotient + 1 : quotient;
    }
    /// <summary>원문 float의 double 승격 뒤 %f6자리 출력과 NaN의 부호/ind 표식을 보존한다.</summary>
    public static string FormatFloat(float value)
    {
        uint bits = BitConverter.SingleToUInt32Bits(value);
        if (float.IsNaN(value)) return bits == 0xffc00000 ? "-nan(ind)" : (bits >> 31) != 0 ? "-nan" : "nan";
        if (float.IsPositiveInfinity(value)) return "inf";
        if (float.IsNegativeInfinity(value)) return "-inf";
        return ((double)value).ToString("F6", CultureInfo.InvariantCulture);
    }
}
