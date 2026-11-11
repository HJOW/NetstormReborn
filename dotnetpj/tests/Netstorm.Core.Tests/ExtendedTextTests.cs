using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 확장 문자 부호화(정적 구간·동적 한자·표시 경계)의 계약을 검사한다
/// (cpppj <c>o/OriginalText.h/.cpp</c>의 확장 계약을 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.350에서 6차·11차 표시 부호화 재대조와 함께 추가했다. 동적 한자 표는 프로세스 전역이라 인덱스 값이 아니라 왕복 성질만 단정한다.</remarks>
public sealed class ExtendedTextTests
{
    /// <summary>정적 구간의 첫·끝 인덱스가 cpppj와 같은 바이트다.</summary>
    /// <param name="codePoint">부호화할 코드 포인트다.</param><param name="first">기대 첫 숫자 바이트다.</param><param name="second">기대 둘째 숫자 바이트다.</param><param name="third">기대 셋째 숫자 바이트다.</param>
    [Theory]
    [InlineData(0xac00u, 0x01, 0x01, 0x01)]
    [InlineData(0x3131u, 0x15, 0x19, 0x1c)]
    [InlineData(0xff9fu, 0x16, 0x15, 0x04)]
    [InlineData(0x3042u, 0x16, 0x08, 0x02)]
    public void StaticRangeIndexesMatchCpp(uint codePoint, byte first, byte second, byte third)
    {
        byte[]? encoded = ExtendedText.EncodeExtendedCodePoint(codePoint);
        Assert.NotNull(encoded);
        Assert.Equal([ExtendedText.ExtendedTextLead, first, second, third], encoded);
        Assert.Equal(codePoint, ExtendedText.DecodeExtendedSequence(encoded, 0));
    }

    /// <summary>확장 대상이 아니면 null이다.</summary>
    [Fact]
    public void NonTargetCodePointEncodesNull()
    {
        Assert.Null(ExtendedText.EncodeExtendedCodePoint(0x41u));
        Assert.Null(ExtendedText.EncodeExtendedCodePoint(0x20acu));
    }

    /// <summary>틀린 선두·짧은 꼬리·숫자 아님은 시퀀스가 아니다.</summary>
    [Fact]
    public void BrokenSequencesDecodeNull()
    {
        Assert.Null(ExtendedText.DecodeExtendedSequence([0x41, 0x01, 0x01, 0x01], 0));
        Assert.Null(ExtendedText.DecodeExtendedSequence([0x1f, 0x01, 0x01], 0));
        Assert.Null(ExtendedText.DecodeExtendedSequence([0x1f, 0x01, 0x02, 0x20], 0));
        Assert.Null(ExtendedText.DecodeExtendedSequence([0x1f, 0x1e, 0x1e, 0x1e], 0));
        // 위치 인자는 시퀀스의 시작이어야 한다
        Assert.Equal(0xac00u, ExtendedText.DecodeExtendedSequence([(byte)'x', 0x1f, 0x01, 0x01, 0x01], 1));
        Assert.Null(ExtendedText.DecodeExtendedSequence([(byte)'x', 0x1f, 0x01, 0x01, 0x01], 0));
        // 정확하지 않은 시퀀스는 일반 제어문자로 보존된다
        Assert.Equal("\u001f\u0001\u0002", ExtendedText.DecodeDisplayText([0x1f, 0x01, 0x02]));
    }

    /// <summary>한글·1252 혼합 문장의 바이트 배치가 cpppj와 같다.</summary>
    [Fact]
    public void MixedKoreanTextMatchesCppByteLayout()
    {
        string text = "Netstorm 한게임 café €ㄱ~r！";
        byte[] bytes = ExtendedText.EncodeDisplayText(text);
        // 영문 9, 한글 3자, " caf"와 é, 공백, €, ㄱ, "~r", 전각 느낌표다
        Assert.Equal(9 + 3 * 4 + 5 + 1 + 1 + 4 + 2 + 4, bytes.Length);
        Assert.Equal(ExtendedText.ExtendedTextLead, bytes[9]);
        for (int index = 0; index < 9; index++)
        {
            Assert.Equal((byte)text[index], bytes[index]);
        }
        Assert.Equal(text, ExtendedText.DecodeDisplayText(bytes));
        Assert.Equal("", ExtendedText.DecodeDisplayText([]));
    }

    /// <summary>숫자 바이트 판정은 글자 폭 표와 같은 집합이다.</summary>
    [Fact]
    public void DigitSetMatchesCpp()
    {
        Assert.True(ExtendedText.IsExtendedTextDigit(0x01));
        Assert.True(ExtendedText.IsExtendedTextDigit(0x1e));
        Assert.False(ExtendedText.IsExtendedTextDigit(0x20));
        Assert.False(ExtendedText.IsExtendedTextDigit(0x09));
        Assert.False(ExtendedText.IsExtendedTextDigit(0x1f));
    }

    /// <summary>한자는 처음 쓰일 때 배정되어 같은 프로세스 안에서 왕복한다.</summary>
    [Fact]
    public void DynamicIdeographRoundTripsWithinProcess()
    {
        byte[]? first = ExtendedText.EncodeExtendedCodePoint(0x6f22u);
        Assert.NotNull(first);
        Assert.Equal(ExtendedText.ExtendedTextLead, first[0]);
        Assert.Equal(0x6f22u, ExtendedText.DecodeExtendedSequence(first, 0));
        // 다시 부호화하면 같은 바이트다
        Assert.Equal(first, ExtendedText.EncodeExtendedCodePoint(0x6f22u));
    }

    /// <summary>표시 경계는 ASCII·1252·확장 시퀀스를 한 번에 읽고 쓴다.</summary>
    [Fact]
    public void DisplayBoundaryRoundTrips()
    {
        byte[] encoded = ExtendedText.EncodeDisplayText("A가€");
        Assert.Equal((byte)'A', encoded[0]);
        Assert.Equal([ExtendedText.ExtendedTextLead, (byte)0x01, (byte)0x01, (byte)0x01], encoded[1..5]);
        Assert.Equal(0x80, encoded[5]);
        Assert.Equal("A가€", ExtendedText.DecodeDisplayText(encoded));
    }

    /// <summary>BOM은 이미 변환된 텍스트로 취급한다.</summary>
    [Fact]
    public void DisplayDecodeStripsBom()
    {
        Assert.Equal("가", ExtendedText.DecodeDisplayText([0xef, 0xbb, 0xbf, 0xea, 0xb0, 0x80]));
    }

    /// <summary>표현할 수 없는 문자와 잘못된 서로게이트는 거부한다.</summary>
    [Fact]
    public void DisplayEncodeRejectsUnrepresentable()
    {
        Assert.Throws<ArgumentException>(() => ExtendedText.EncodeDisplayText("😀"));
        Assert.Throws<ArgumentException>(() => ExtendedText.EncodeDisplayText("㐀"));
        Assert.Throws<ArgumentException>(() => ExtendedText.EncodeDisplayText("\ud800"));
    }
}
