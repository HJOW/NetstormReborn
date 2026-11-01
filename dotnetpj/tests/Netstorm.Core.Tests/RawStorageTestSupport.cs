using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// DataManager·Template 계층 검사(<c>X86RawDataManager*</c>, <c>X86RawTemplate*</c>)가 함께 쓰는 입력 복원·관측 도우미다.
/// cpppj <c>tests/RawSceneSupport.h</c>의 <c>Hex</c>·<c>Get</c>·<c>Put</c>·<c>Number</c>와 각 fixture 검사의 16진 복원 함수에 대응한다.
/// 원본 기대값을 계산하지 않고 fixture 입력/기대 바이트를 그대로 읽기만 한다 (0.128, 2026-11-01).
/// </summary>
internal static class RawStorageTestSupport
{
    /// <summary>슬롯 전체를 소문자 16진수로 만든다(빈 입력은 빈 문자열). 실행기와 같은 표기다.</summary>
    /// <param name="bytes">관측할 바이트.</param>
    public static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexStringLower(bytes);

    /// <summary>fixture 기대값 칸의 표기: 빈 관측은 단일 '-'로 적는다.</summary>
    /// <param name="bytes">관측할 바이트.</param>
    public static string DashHex(ReadOnlySpan<byte> bytes) => bytes.IsEmpty ? "-" : Hex(bytes);

    /// <summary>
    /// 최초 원문의 16진 바이트를 복원한다. 일반 두 글자 16진과 반복 표기 <c>~값x개수~</c>(예: <c>~a7x16~</c>)를 원래 순서대로 펼친다.
    /// 입력/기대 영역을 읽을 뿐 제품의 결과를 계산하지 않는다.
    /// </summary>
    /// <param name="text">fixture 칸의 원문.</param>
    public static byte[] Decode(string text)
    {
        List<byte> bytes = [];
        int at = 0;
        // 일반 16진 두 글자 또는 ~값x개수~ 반복 표기를 처음부터 끝까지 읽는다
        while (at < text.Length)
        {
            if (text[at] != '~')
            {
                bytes.Add(byte.Parse(text.AsSpan(at, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture));
                at += 2;
                continue;
            }
            int end = text.IndexOf('~', at + 1);
            int separator = text.IndexOf('x', at + 1);
            if (end < 0 || separator < 0 || separator >= end)
            {
                throw new InvalidDataException("원문의 반복 표기 누락");
            }
            byte value = byte.Parse(text.AsSpan(at + 1, separator - at - 1), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
            int count = int.Parse(text.AsSpan(separator + 1, end - separator - 1), CultureInfo.InvariantCulture);
            // 같은 바이트를 개수만큼 이어 붙인다
            for (int index = 0; index < count; index++)
            {
                bytes.Add(value);
            }
            at = end + 1;
        }
        return [.. bytes];
    }

    /// <summary>fixture 숫자 칸을 DWORD로 읽는다.</summary>
    /// <param name="text">십진수 칸.</param>
    public static uint Number(string text) => uint.Parse(text, CultureInfo.InvariantCulture);

    /// <summary>비정렬 원본 DWORD를 읽는다.</summary>
    /// <param name="raw">읽을 바이트.</param>
    /// <param name="offset">DWORD 위치.</param>
    public static uint Get(ReadOnlySpan<byte> raw, int offset) => RawStorageBytes.Read(raw, offset);

    /// <summary>비정렬 원본 DWORD를 쓰되 주변 바이트는 보존한다.</summary>
    /// <param name="raw">쓸 바이트.</param>
    /// <param name="offset">DWORD 위치.</param>
    /// <param name="value">쓸 값.</param>
    public static void Put(Span<byte> raw, int offset, uint value) => RawStorageBytes.Put(raw, offset, value);

    /// <summary>원본 바이트를 1:1 문자로 바꾼 문자열의 바이트다(이름 관측용).</summary>
    /// <param name="text">관측할 이름.</param>
    public static byte[] Latin1(string text) => Encoding.Latin1.GetBytes(text);

    /// <summary>첫 NUL 이전을 문자열로 읽는다. NUL이 없으면 거부한다.</summary>
    /// <param name="bytes">읽을 바이트.</param>
    public static string TextAt(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        Assert.True(end >= 0, "원문 이름 NUL 누락");
        return Encoding.Latin1.GetString(bytes[..end]);
    }

    /// <summary>동작이 예외로 끝나는지 확인한다. 호스트 예외 경계를 값 0의 정상 반환으로 바꾸지 않는다.</summary>
    /// <param name="action">실행할 동작.</param>
    public static bool Throws(Action action)
    {
        try
        {
            action();
        }
        catch (Exception)
        {
            return true;
        }
        return false;
    }

    /// <summary>fixture 한 행의 입력과 어긋남 내용을 모아 실패 메시지를 만든다. 첫 어긋남에서 멈추지 않고 모든 행의 요약을 보고한다.</summary>
    /// <param name="problems">행별 어긋남 설명.</param>
    /// <param name="label">검사 이름.</param>
    public static void NoProblems(List<string> problems, string label)
    {
        Assert.True(problems.Count == 0, $"{label}: {problems.Count}행 불일치\n" + string.Join('\n', problems.Take(8)));
    }
}
