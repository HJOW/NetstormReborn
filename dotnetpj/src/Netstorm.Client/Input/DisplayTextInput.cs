// UTF-8/UTF-16 확정 입력과 기존 표시 바이트 문자열의 문자 경계를 연결한다.
// 2026-11-11 0.351에서 cpppj client/DisplayTextInput.h/.cpp를 이식했다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Input;

/// <summary>
/// 원본 큐의 크기·배치를 유지하는 한국어 입력 경계다(cpppj <c>client/DisplayTextInput</c>).
/// 플랫폼·IME·MCP와 TextEntry가 같은 변환 규칙을 공유한다.
/// </summary>
/// <remarks>2026-11-11 0.351에서 이식했다. 표시 바이트 부호화는 <see cref="ExtendedText"/>를 쓴다.</remarks>
public static class DisplayTextInput
{
    /// <summary>비명령 사건의 미사용 상위 비트로 확정 Unicode 한 문자를 식별한다. 실제 가상 키는 0xff 이하라 겹치지 않는다.</summary>
    public const uint CommittedTextInput = 0x01000000U;

    /// <summary>문자 하나의 물리 바이트 폭이다. 정확한 네 바이트만 원자 문자로 취급한다.</summary>
    /// <param name="text">표시 바이트다.</param><param name="at">읽을 위치다.</param>
    /// <returns>4 또는 1이다.</returns>
    private static int Width(ReadOnlySpan<byte> text, int at) =>
        ExtendedText.DecodeExtendedSequence(text, at) is not null ? ExtendedText.ExtendedTextLength : 1;

    /// <summary>문자 태그·명령 비트와 Unicode 범위를 구별한다.</summary>
    /// <param name="code">검사할 사건 코드다.</param>
    /// <returns>확정 입력 사건이면 참이다.</returns>
    public static bool IsCommittedTextInput(uint code) =>
        (code & 0x03e00000U) == CommittedTextInput && (code & 0x001fffffU) <= 0x10ffffU;

    /// <summary>확정 입력 한 사건을 기존 표시 바이트로 바꾼다. 잘못된 태그·미지원 문자는 거부한다.</summary>
    /// <param name="code">확정 입력 사건 코드다.</param>
    /// <returns>표시 바이트다.</returns>
    /// <exception cref="ArgumentException">태그 범위를 벗어나거나 표시 부호화 미지원 문자다.</exception>
    public static byte[] CommittedTextBytes(uint code)
    {
        if (!IsCommittedTextInput(code))
        {
            throw new ArgumentException("확정 입력 태그 범위", nameof(code));
        }
        return ExtendedText.EncodeExtendedCodePoint(code & 0x001fffffU)
            ?? throw new ArgumentException("표시 부호화 미지원 확정 문자", nameof(code));
    }

    /// <summary>
    /// 전체 표시 바이트를 먼저 검증한다. ASCII·1252는 기존 한 바이트 사건, 한글은 태그 한 사건이다.
    /// 제어문자·NUL은 받지 않는다. Enter·Tab·Backspace는 기존 키 동작으로 전달한다.
    /// </summary>
    /// <param name="utf8">확정 본문이다.</param><param name="x">X 좌표.</param><param name="y">Y 좌표.</param>
    /// <returns>원자적 입력 사건 목록이다.</returns>
    /// <exception cref="ArgumentException">제어문자·표현 불가 문자가 있다.</exception>
    public static InputEvent[] MakeCommittedTextEvents(string utf8, int x = 0, int y = 0)
    {
        ArgumentNullException.ThrowIfNull(utf8);
        byte[] bytes = ExtendedText.EncodeDisplayText(utf8);
        var result = new List<InputEvent>(bytes.Length);
        // 전체 검증 뒤에만 사건 목록을 만들므로 실패가 일부 입력을 남기지 않는다
        for (int at = 0; at < bytes.Length;)
        {
            uint? codePoint = ExtendedText.DecodeExtendedSequence(bytes, at);
            if (codePoint is not null)
            {
                result.Add(new InputEvent(CommittedTextInput | codePoint.Value, x, y));
                at += ExtendedText.ExtendedTextLength;
            }
            else
            {
                byte value = bytes[at++];
                if (value < 0x20U || value == 0x7fU)
                {
                    throw new ArgumentException("text 입력의 제어문자는 key 동작으로 전달해야 함", nameof(utf8));
                }
                result.Add(new InputEvent(value, x, y));
            }
        }
        return result.ToArray();
    }

    /// <summary>IME의 UTF-16 확정 본문을 UTF-8로 바꾼다. 비정상 서로게이트는 전체 거부한다.</summary>
    /// <param name="text">UTF-16 본문이다.</param>
    /// <returns>UTF-8 바이트다.</returns>
    /// <exception cref="ArgumentException">잘린 서로게이트·Unicode 범위 밖이다.</exception>
    public static byte[] CommittedUtf16ToUtf8(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        var result = new List<byte>(text.Length);
        // UTF-16 서로게이트 쌍을 하나의 scalar로 검증한다
        for (int at = 0; at < text.Length; at++)
        {
            uint codePoint = text[at];
            if (codePoint is >= 0xd800U and <= 0xdbffU)
            {
                if (++at == text.Length || text[at] is < '\udc00' or > '\udfff')
                {
                    throw new ArgumentException("잘린 UTF-16 확정 입력", nameof(text));
                }
                codePoint = 0x10000U + ((codePoint - 0xd800U) << 10) + ((uint)text[at] - 0xdc00U);
            }
            AppendUtf8(result, codePoint);
        }
        return result.ToArray();
    }

    /// <summary>검증된 scalar를 UTF-8 뒤에 기록한다. 서로게이트·Unicode 범위 밖을 거부한다.</summary>
    /// <param name="result">쓸 목록이다.</param><param name="codePoint">쓸 코드 포인트다.</param>
    /// <exception cref="ArgumentException">Unicode 범위를 벗어났다.</exception>
    private static void AppendUtf8(List<byte> result, uint codePoint)
    {
        if (codePoint > 0x10ffffU || (codePoint is >= 0xd800U and <= 0xdfffU))
        {
            throw new ArgumentException("확정 입력 Unicode 범위");
        }
        if (codePoint < 0x80U) result.Add((byte)codePoint);
        else if (codePoint < 0x800U) { result.Add((byte)(0xc0U | (codePoint >> 6))); result.Add((byte)(0x80U | (codePoint & 63U))); }
        else if (codePoint < 0x10000U) { result.Add((byte)(0xe0U | (codePoint >> 12))); result.Add((byte)(0x80U | ((codePoint >> 6) & 63U))); result.Add((byte)(0x80U | (codePoint & 63U))); }
        else { result.Add((byte)(0xf0U | (codePoint >> 18))); result.Add((byte)(0x80U | ((codePoint >> 12) & 63U))); result.Add((byte)(0x80U | ((codePoint >> 6) & 63U))); result.Add((byte)(0x80U | (codePoint & 63U))); }
    }

    /// <summary>표시 바이트를 UTF-16으로 되돌린다. 클립보드 경계에서만 사용한다.</summary>
    /// <param name="text">표시 바이트다.</param>
    /// <returns>UTF-16 본문이다.</returns>
    public static string DisplayTextToUtf16(ReadOnlySpan<byte> text)
    {
        var result = new System.Text.StringBuilder(text.Length);
        // 기존 1252·확장 부호화는 BMP만 표현하므로 UTF-16 한 칸이다
        for (int at = 0; at < text.Length;)
        {
            uint? codePoint = ExtendedText.DecodeExtendedSequence(text, at);
            if (codePoint is not null)
            {
                result.Append((char)codePoint.Value);
                at += ExtendedText.ExtendedTextLength;
                continue;
            }
            string decoded = ExtendedText.DecodeDisplayText(text.Slice(at, 1));
            uint scalar = (uint)char.ConvertToUtf32(decoded, 0);
            // else 분기의 cp 범위가 이어지므로 같은 이름으로 한 칸을 기록한다
            result.Append((char)scalar);
            at++;
        }
        return result.ToString();
    }

    /// <summary>정확히 인식되는 네 바이트 확장 토큰이 하나라도 있는지 읽는다.</summary>
    /// <param name="text">표시 바이트다.</param>
    /// <returns>토큰이 있으면 참이다.</returns>
    public static bool HasExtendedDisplayText(ReadOnlySpan<byte> text)
    {
        // 원문 일반 바이트만 있으면 편집 동작을 그대로 보존한다
        for (int at = 0; at < text.Length; at++)
        {
            if (Width(text, at) > 1)
            {
                return true;
            }
        }
        return false;
    }

    /// <summary>바이트 오프셋을 이전 문자 경계로 보정한다. 잘못된 선두는 원래 한 바이트로 취급한다.</summary>
    /// <param name="text">표시 바이트다.</param><param name="position">보정할 위치다.</param>
    /// <returns>이전 문자 경계다.</returns>
    public static int DisplayTextFloor(ReadOnlySpan<byte> text, int position)
    {
        position = Math.Min(position, text.Length);
        int at = 0;
        // 문자의 일부를 선택 시작으로 만들지 않는다
        while (at < position)
        {
            int next = at + Width(text, at);
            if (next > position)
            {
                return at;
            }
            at = next;
        }
        return at;
    }

    /// <summary>바이트 오프셋을 다음 문자 경계로 보정한다.</summary>
    /// <param name="text">표시 바이트다.</param><param name="position">보정할 위치다.</param>
    /// <returns>다음 문자 경계다.</returns>
    public static int DisplayTextCeil(ReadOnlySpan<byte> text, int position)
    {
        position = Math.Min(position, text.Length);
        int floor = DisplayTextFloor(text, position);
        return floor == position ? floor : floor + Width(text, floor);
    }

    /// <summary>현재 위치의 이전 문자 경계다. 한글 네 바이트를 한 번에 건너뛴다.</summary>
    /// <param name="text">표시 바이트다.</param><param name="position">현재 위치다.</param>
    /// <returns>이전 문자 경계다.</returns>
    public static int PreviousDisplayText(ReadOnlySpan<byte> text, int position)
    {
        int at = DisplayTextFloor(text, position);
        return at != 0 ? DisplayTextFloor(text, at - 1) : 0;
    }

    /// <summary>현재 위치의 다음 문자 경계다. 한글 네 바이트를 한 번에 건너뛴다.</summary>
    /// <param name="text">표시 바이트다.</param><param name="position">현재 위치다.</param>
    /// <returns>다음 문자 경계다.</returns>
    public static int NextDisplayText(ReadOnlySpan<byte> text, int position)
    {
        int at = DisplayTextFloor(text, position);
        return at < text.Length ? at + Width(text, at) : text.Length;
    }

    /// <summary>LF 기준 실제 줄의 읽기 전용 뷰다. 원문 바이트 줄번호를 바꾸지 않는다.</summary>
    /// <param name="text">표시 바이트다.</param><param name="line">줄 번호다.</param>
    /// <returns>해당 줄 구간이다.</returns>
    public static ReadOnlySpan<byte> DisplayTextLine(ReadOnlySpan<byte> text, int line)
    {
        if (line < 0)
        {
            return [];
        }
        // LF 줄번호는 원문과 같은 바이트 본문에서 읽는다
        for (int at = 0; at < line; at++)
        {
            int end = text.IndexOf((byte)'\n');
            if (end < 0)
            {
                return [];
            }
            text = text[(end + 1)..];
        }
        int stop = text.IndexOf((byte)'\n');
        return stop < 0 ? text : text[..stop];
    }

    /// <summary>바이트 상한까지 완전한 토큰만 복사한다. 제외문자는 확장 토큰 내부에 적용하지 않는다.</summary>
    /// <param name="text">읽을 본문이다.</param><param name="excluded">제외할 선두 바이트들이다.</param><param name="maximum">바이트 상한이다.</param>
    /// <returns>걸러진 본문이다.</returns>
    public static byte[] FilterDisplayText(ReadOnlySpan<byte> text, ReadOnlySpan<byte> excluded, int maximum)
    {
        int nul = text.IndexOf((byte)0);
        if (nul >= 0)
        {
            text = text[..nul];
        }
        var result = new List<byte>(Math.Min(maximum, text.Length));
        // 확장 숫자의 0x08·공백 판정을 금지하고 완전한 문자만 복사한다
        for (int at = 0; at < text.Length;)
        {
            int count = Width(text, at);
            // 숫자 전용처럼 선두가 제외되면 확장 문자 전체를 거부한다
            if (excluded.IndexOf(text[at]) >= 0)
            {
                at += count;
                continue;
            }
            if (count > maximum - result.Count)
            {
                break;
            }
            result.AddRange(text.Slice(at, count).ToArray());
            at += count;
        }
        return result.ToArray();
    }
}
