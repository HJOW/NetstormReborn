// 6차 한국어·11차 일본어: 8비트 글꼴 파이프라인에 한글·일본어 등을 통과시키는 확장 문자 부호화다.
// Windows-1252 바이트 하나가 글리프 하나인 경로에, 1252로 표현할 수 없는 문자를 4바이트 제어문자 시퀀스
// [시작 바이트 0x1f] + [25진수 숫자 3개]로 바꿔 기존 바이트 문자열 경로를 그대로 통과시킨다.
// 2026-11-11 0.350에서 cpppj o/OriginalText.h/.cpp의 확장 부분을 이식했다.
namespace Netstorm.Core.Original;

/// <summary>
/// 표시(글꼴 바이트) 경계용 확장 문자 부호화다(cpppj <c>o/OriginalText.h</c>의 확장 부분).
/// 파일·설정 경계는 확장 시퀀스를 해독하지 않으므로 이 클래스는 화면 표시 경로에서만 쓴다.
/// </summary>
/// <remarks>2026-11-11 0.350에서 이식했다. 11차 일본어 동적 한자 표는 프로세스 전역이며 잠금으로 보호한다.</remarks>
public static class ExtendedText
{
    /// <summary>확장 문자 시퀀스의 시작 바이트다.</summary>
    public const byte ExtendedTextLead = 0x1f;
    /// <summary>시퀀스 길이(시작 바이트 1개 + 숫자 3개)다.</summary>
    public const int ExtendedTextLength = 4;

    /// <summary>원본 0x80~0x9f의 Windows-1252 코드 포인트. 정의되지 않은 바이트는 제어 문자를 보존한다.</summary>
    private static readonly uint[] Windows1252 =
    [
        0x20ac, 0x81, 0x201a, 0x192, 0x201e, 0x2026, 0x2020, 0x2021,
        0x2c6, 0x2030, 0x160, 0x2039, 0x152, 0x8d, 0x17d, 0x8f,
        0x90, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
        0x2dc, 0x2122, 0x161, 0x203a, 0x153, 0x9d, 0x17e, 0x178,
    ];

    /// <summary>숫자 25개의 바이트다. 순서가 곧 값(0..24)이다.</summary>
    private static readonly byte[] ExtendedDigits =
    [
        1, 2, 3, 4, 5, 6, 7, 8, 0x0b, 0x0c, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
        0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1c, 0x1d, 0x1e,
    ];

    /// <summary>확장 대상 코드 포인트 구간이다. 각 구간은 이어 붙인 인덱스 공간의 연속 구간이 된다.</summary>
    /// <param name="First">구간 첫 코드 포인트다.</param><param name="Last">구간 끝 코드 포인트다.</param>
    private readonly record struct ExtendedRange(uint First, uint Last);

    /// <summary>
    /// 확장 대상 구간이다. 앞 네 구간의 인덱스는 6차 한국어와 같아야 하므로 새 구간은 뒤에만 붙인다.
    /// 한글 음절, 호환 자모, CJK 기호, 전각 ASCII, 히라가나·가타카나, 반각 가타카나다.
    /// </summary>
    private static readonly ExtendedRange[] ExtendedRanges =
    [
        new(0xac00, 0xd7a3), new(0x3131, 0x318e), new(0x3000, 0x303f),
        new(0xff01, 0xff5e), new(0x3040, 0x30ff), new(0xff61, 0xff9f),
    ];

    /// <summary>정적 구간의 코드 포인트 수다.</summary>
    private const uint StaticExtendedCount = 11679;
    /// <summary>동적 배정 한자 구간의 처음과 끝이다.</summary>
    private const uint DynamicFirst = 0x4e00;
    /// <summary>동적 배정 한자 구간의 끝이다.</summary>
    private const uint DynamicLast = 0x9fff;
    /// <summary>숫자 3개의 전체 용량(25^3)이다.</summary>
    private const uint ExtendedCapacity = 25 * 25 * 25;

    /// <summary>배정 표 접근을 보호한다(UI 외 스레드의 부호화 대비).</summary>
    private static readonly object DynamicLock = new();
    /// <summary>인덱스 순서대로 배정한 한자 코드 포인트다.</summary>
    private static readonly List<uint> DynamicIdeographs = [];

    /// <summary>숫자 바이트의 값(0..24)이다. 숫자가 아니면 -1이다.</summary>
    /// <param name="value">검사할 바이트다.</param>
    /// <returns>숫자 값, 아니면 -1이다.</returns>
    private static int DigitValue(byte value)
    {
        // 25개 중 일치하는 위치가 값이다
        for (int index = 0; index < ExtendedDigits.Length; index++)
        {
            if (ExtendedDigits[index] == value)
            {
                return index;
            }
        }
        return -1;
    }

    /// <summary>바이트가 확장 시퀀스의 숫자 바이트인지 판정한다(글자 폭 표에서 폭 0으로 다루는 데 쓴다).</summary>
    /// <param name="value">검사할 바이트다.</param>
    /// <returns>숫자 바이트이면 참이다.</returns>
    public static bool IsExtendedTextDigit(byte value) => DigitValue(value) >= 0;

    /// <summary>
    /// 코드 포인트가 확장 부호화 대상이면 4바이트 시퀀스를 만든다. 대상이 아니면 null이다.
    /// CJK 통합 한자는 처음 쓰일 때 남은 인덱스를 배정한다(화면 전용 경계라 같은 프로세스 안의 표만 일치하면 된다).
    /// </summary>
    /// <param name="codePoint">부호화할 코드 포인트다.</param>
    /// <returns>4바이트 시퀀스, 대상이 아니면 null이다.</returns>
    public static byte[]? EncodeExtendedCodePoint(uint codePoint)
    {
        uint basis = 0;
        // 구간을 순서대로 훑어 코드 포인트의 인덱스를 구한다
        foreach (ExtendedRange range in ExtendedRanges)
        {
            if (codePoint >= range.First && codePoint <= range.Last)
            {
                uint index = basis + (codePoint - range.First);
                return [ExtendedTextLead, ExtendedDigits[index / 625], ExtendedDigits[(index / 25) % 25], ExtendedDigits[index % 25]];
            }
            basis += range.Last - range.First + 1;
        }
        // 정적 구간 밖의 한자는 처음 쓰일 때 남은 인덱스를 배정한다
        if (codePoint >= DynamicFirst && codePoint <= DynamicLast)
        {
            lock (DynamicLock)
            {
                int found = DynamicIdeographs.IndexOf(codePoint);
                if (found < 0)
                {
                    if (StaticExtendedCount + (uint)DynamicIdeographs.Count >= ExtendedCapacity)
                    {
                        return null;
                    }
                    DynamicIdeographs.Add(codePoint);
                    found = DynamicIdeographs.Count - 1;
                }
                uint index = StaticExtendedCount + (uint)found;
                return [ExtendedTextLead, ExtendedDigits[index / 625], ExtendedDigits[(index / 25) % 25], ExtendedDigits[index % 25]];
            }
        }
        return null;
    }

    /// <summary>
    /// 확장 시퀀스의 선두에서 코드 포인트를 해독한다. 바이트 위치부터 정확한 시퀀스가 아니면 null이다(그 바이트는 일반 문자다).
    /// 배정 전 동적 인덱스는 시퀀스가 아니다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param><param name="position">선두 위치다.</param>
    /// <returns>코드 포인트, 정확한 시퀀스가 아니면 null이다.</returns>
    public static uint? DecodeExtendedSequence(ReadOnlySpan<byte> bytes, int position)
    {
        if (position < 0 || position + ExtendedTextLength > bytes.Length || bytes[position] != ExtendedTextLead)
        {
            return null;
        }
        int high = DigitValue(bytes[position + 1]), middle = DigitValue(bytes[position + 2]), low = DigitValue(bytes[position + 3]);
        if (high < 0 || middle < 0 || low < 0)
        {
            return null;
        }
        uint index = (uint)high * 625 + (uint)middle * 25 + (uint)low;
        // 인덱스가 속한 구간을 찾아 코드 포인트로 되돌린다
        foreach (ExtendedRange range in ExtendedRanges)
        {
            uint size = range.Last - range.First + 1;
            if (index < size)
            {
                return range.First + index;
            }
            index -= size;
        }
        lock (DynamicLock)
        {
            // 배정된 한자 인덱스다
            if (index < (uint)DynamicIdeographs.Count)
            {
                return DynamicIdeographs[(int)index];
            }
        }
        // 구간 밖의 숫자 조합은 시퀀스가 아니다
        return null;
    }

    /// <summary>화면에 그릴 글꼴 바이트로 읽는다. 정확한 확장 시퀀스를 한 문자로 되돌린다.</summary>
    /// <param name="bytes">글꼴 바이트다.</param>
    /// <returns>표시 문자열(UTF-16)이다.</returns>
    public static string DecodeDisplayText(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf)
        {
            return System.Text.Encoding.UTF8.GetString(bytes[3..]);
        }
        var result = new System.Text.StringBuilder(bytes.Length);
        // 각 코드 포인트를 UTF-16으로 기록한다. 정확한 확장 시퀀스는 한 코드 포인트로 읽는다.
        for (int position = 0; position < bytes.Length; position++)
        {
            byte value = bytes[position];
            uint codePoint = value is >= 0x80 and <= 0x9f ? Windows1252[value - 0x80] : value;
            uint? extended = value == ExtendedTextLead ? DecodeExtendedSequence(bytes, position) : null;
            if (extended is not null)
            {
                codePoint = extended.Value;
                position += ExtendedTextLength - 1;
            }
            result.Append(char.ConvertFromUtf32((int)codePoint));
        }
        return result.ToString();
    }

    /// <summary>
    /// 화면에 그릴 글꼴 바이트로 만든다. 확장 대상 문자는 4바이트 확장 시퀀스가 된다.
    /// 그 밖의 표현할 수 없는 문자·잘못된 서로게이트는 거부한다.
    /// </summary>
    /// <param name="text">표시 문자열(UTF-16)이다.</param>
    /// <returns>글꼴 바이트다.</returns>
    /// <exception cref="ArgumentException">표현할 수 없는 문자나 잘못된 서로게이트다.</exception>
    public static byte[] EncodeDisplayText(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        var result = new List<byte>(text.Length);
        // UTF-16을 코드 포인트 단위로 읽는다
        for (int index = 0; index < text.Length;)
        {
            uint codePoint;
            char lead = text[index];
            if (char.IsHighSurrogate(lead))
            {
                if (index + 1 >= text.Length || !char.IsLowSurrogate(text[index + 1]))
                {
                    throw new ArgumentException("잘못된 서로게이트 문자다", nameof(text));
                }
                codePoint = (uint)char.ConvertToUtf32(lead, text[index + 1]);
                index += 2;
            }
            else if (char.IsLowSurrogate(lead))
            {
                throw new ArgumentException("잘못된 서로게이트 문자다", nameof(text));
            }
            else
            {
                codePoint = lead;
                index++;
            }
            if (codePoint < 0x80 || (codePoint >= 0xa0 && codePoint <= 0xff))
            {
                result.Add((byte)codePoint);
            }
            else if (EncodeExtendedCodePoint(codePoint) is { } extended)
            {
                // 표시 경계에서만 확장 대상이 확장 시퀀스다
                result.AddRange(extended);
            }
            else
            {
                int found = codePoint <= 0xffff ? Array.IndexOf(Windows1252, codePoint) : -1;
                if (found < 0)
                {
                    throw new ArgumentException("글꼴 바이트로 표현할 수 없는 문자다", nameof(text));
                }
                result.Add((byte)(0x80 + found));
            }
        }
        return result.ToArray();
    }
}
