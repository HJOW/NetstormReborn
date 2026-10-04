using System.Text;

namespace Netstorm.Assets;

/// <summary>
/// 원본 텍스트 파일(미션 스크립트·번역표·설정) 디코딩.
/// 원본 파일은 Windows-1252 이고, 클론에서 새로 만드는 파일(예: 한국어 번역)은 UTF-8 이므로 둘 다 받아들인다.
/// </summary>
public static class OriginalText
{
    /// <summary>UTF-8 BOM</summary>
    private static ReadOnlySpan<byte> Utf8Bom => [0xEF, 0xBB, 0xBF];

    /// <summary>잘못된 바이트가 있으면 예외를 던지는 엄격한 UTF-8 디코더 (UTF-8 여부 판별용)</summary>
    private static readonly UTF8Encoding StrictUtf8 = new(encoderShouldEmitUTF8Identifier: false, throwOnInvalidBytes: true);

    /// <summary>
    /// Windows-1252 에서 0x80~0x9F 구간의 문자 (나머지 0xA0~0xFF 는 Latin-1 과 같다).
    /// 정의되지 않은 자리(0x81, 0x8D, 0x8F, 0x90, 0x9D)는 Latin-1 제어 문자 그대로 둔다.
    /// </summary>
    private static readonly char[] Cp1252High =
    [
        '€', '\u0081', '‚', 'ƒ', '„', '…', '†', '‡',
        'ˆ', '‰', 'Š', '‹', 'Œ', '\u008D', 'Ž', '\u008F',
        '\u0090', '‘', '’', '“', '”', '•', '–', '—',
        '˜', '™', 'š', '›', 'œ', '\u009D', 'ž', 'Ÿ',
    ];

    /// <summary>Windows-1252 특수 구간의 시작 바이트</summary>
    private const int Cp1252HighStart = 0x80;

    /// <summary>
    /// 바이트를 문자열로 바꾼다. BOM 이 있거나 ASCII 가 아닌 바이트가 올바른 UTF-8 이면 UTF-8,
    /// 그렇지 않으면 Windows-1252 로 해석한다. 줄바꿈은 바꾸지 않는다.
    /// </summary>
    /// <param name="data">파일 내용</param>
    public static string Decode(ReadOnlySpan<byte> data)
    {
        if (data.StartsWith(Utf8Bom))
        {
            return Encoding.UTF8.GetString(data[Utf8Bom.Length..]);
        }
        if (Ascii.IsValid(data))
        {
            return Encoding.ASCII.GetString(data);
        }
        try
        {
            return StrictUtf8.GetString(data);
        }
        catch (DecoderFallbackException)
        {
            return DecodeWindows1252(data);
        }
    }

    /// <summary>Windows-1252 로 해석한다</summary>
    /// <param name="data">원본 바이트</param>
    public static string DecodeWindows1252(ReadOnlySpan<byte> data)
    {
        var chars = new char[data.Length];
        // 0x80~0x9F 만 표로 바꾸고 나머지는 코드 값 그대로 (Latin-1 과 동일)
        for (int i = 0; i < data.Length; i++)
        {
            byte b = data[i];
            chars[i] = b is >= Cp1252HighStart and < Cp1252HighStart + 32 ? Cp1252High[b - Cp1252HighStart] : (char)b;
        }
        return new string(chars);
    }
}
