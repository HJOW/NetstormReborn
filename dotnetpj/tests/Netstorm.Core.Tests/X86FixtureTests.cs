namespace Netstorm.Tests;

/// <summary>
/// X86Fixture 판독 도구의 확장(64비트 정수·double 비트·쉼표 목록·절 필터)이 비트를 잃지 않고 동작하는지 확인한다.
/// 이 도구가 어긋나면 모든 fixture 대조가 같이 어긋나므로 먼저 도구를 검사한다 (W01, 2026-10-16).
/// </summary>
public sealed class X86FixtureTests
{
    /// <summary>64비트 십진수 칸이 부호 있음/없음 경계 값을 잃지 않는지 확인한다.</summary>
    [Fact]
    public void ParsesSixtyFourBitIntegers()
    {
        Assert.Equal(ulong.MaxValue, X86Fixture.ULong("18446744073709551615"));
        Assert.Equal(0x8000000000000000UL, X86Fixture.ULong("9223372036854775808"));
        Assert.Equal(long.MinValue, X86Fixture.Long("-9223372036854775808"));
        Assert.Equal(long.MaxValue, X86Fixture.Long("9223372036854775807"));
    }

    /// <summary>십진수로 쓴 double 비트 패턴이 값·부호·무한대·NaN 까지 그대로 복원되는지 확인한다.</summary>
    [Fact]
    public void RestoresDoubleBitPatterns()
    {
        Assert.Equal(1.0, X86Fixture.DoubleBits("4607182418800017408"));
        Assert.Equal(0.5, X86Fixture.DoubleBits("4602678819172646912"));
        // 음의 0 은 0 과 ==로는 같지만 부호 비트가 남아야 한다.
        double negativeZero = X86Fixture.DoubleBits("9223372036854775808");
        Assert.True(double.IsNegative(negativeZero) && negativeZero == 0.0);
        Assert.True(double.IsPositiveInfinity(X86Fixture.DoubleBits("9218868437227405312")));
        Assert.True(double.IsNaN(X86Fixture.DoubleBits("9221120237041090560")));
        // 되돌려 쓴 비트가 입력과 같다.
        Assert.Equal(4562254508917369340UL, BitConverter.DoubleToUInt64Bits(X86Fixture.DoubleBits("4562254508917369340")));
    }

    /// <summary>쉼표 목록 칸이 64비트 값을 잃지 않고 나뉘며, 빈 칸과 "-" 는 빈 목록인지 확인한다.</summary>
    [Fact]
    public void SplitsCommaSeparatedLists()
    {
        Assert.Equal([1UL, 0UL, ulong.MaxValue], X86Fixture.ULongs("1,0,18446744073709551615"));
        Assert.Empty(X86Fixture.ULongs("-"));
        Assert.Empty(X86Fixture.ULongs(""));
    }

    /// <summary>첫 칸 라벨로 절을 고르고, 절 이름·행 수가 첫 등장 순서로 나오는지 합성 행으로 확인한다.</summary>
    [Fact]
    public void FiltersSectionsInFirstSeenOrder()
    {
        string[][] rows = [["B", "1"], ["A", "2"], ["B", "3"], ["C", "4"], ["A", "5"]];
        Assert.Equal([("B", 2), ("A", 2), ("C", 1)], X86Fixture.Sections(rows));
        Assert.Equal([["A", "2"], ["A", "5"]], X86Fixture.Section(rows, "A"));
        Assert.Empty(X86Fixture.Section(rows, "없는 절"));
    }

    /// <summary>실제 fixture 의 절 구성이 cpppj 의 기록(공통 미션 프레임 328행, 튜토리얼 1 판정 370행)과 같은지 확인한다. 절 이름이 바뀌면 여기서 먼저 드러난다.</summary>
    [Fact]
    public void RealFixturesKeepDocumentedSections()
    {
        Assert.Equal([("Frame", 265), ("Advance", 48), ("Pause", 9), ("Postpone", 6)], X86Fixture.Sections(X86Fixture.Read("missionframe")));
        Assert.Equal([("Total", 104), ("Stage", 266)], X86Fixture.Sections(X86Fixture.Read("tutorial-one")));
        Assert.Equal(265, X86Fixture.Read("missionframe", "Frame").Length);
        // 미션 프레임 행의 double 칸이 비트 패턴 그대로 복원된다 (첫 Frame 행의 현재 시각 열은 10.0 초).
        Assert.Equal(10.0, X86Fixture.DoubleBits(X86Fixture.Read("missionframe", "Frame")[0][2]));
    }
}
