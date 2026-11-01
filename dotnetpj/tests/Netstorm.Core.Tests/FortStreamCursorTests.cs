using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// Territory·Money·Badges/Technology 커서 클래스(<see cref="FortTerritoryStream"/>, <see cref="FortMoneyCursor"/>, <see cref="FortMetaStream"/>)의
/// 초기값과 <c>Reset</c>·<c>Set</c>을 확인한다. C++ 값 구조체의 <c>stream = {}</c>·<c>stream = {…}</c> 대입과 같은 결과를 내면서 인스턴스는 교체하지 않는다 (0.130 TS0, 2026-11-01).
/// </summary>
public sealed class FortStreamCursorTests
{
    /// <summary>Territory 커서의 초기값은 비어 있고 <c>Set</c>이 모든 필드를 바꾸며 <c>Reset</c>이 되돌린다.</summary>
    [Fact]
    public void TerritoryStream_SetAndResetKeepSameInstance()
    {
        FortTerritoryStream stream = new();
        Assert.True(stream.Bytes.IsEmpty);
        Assert.Equal(0u, stream.Position);
        Assert.Equal(0u, stream.End);

        byte[] bytes = [1, 2, 3];
        FortTerritoryStream same = stream;
        stream.Set(bytes, 2, 3);
        Assert.Same(same, stream);
        Assert.Equal(bytes, stream.Bytes.ToArray());
        Assert.Equal(2u, stream.Position);
        Assert.Equal(3u, stream.End);

        stream.Reset();
        Assert.True(stream.Bytes.IsEmpty);
        Assert.Equal(0u, stream.Position);
        Assert.Equal(0u, stream.End);
    }

    /// <summary>Money 커서는 위치와 끝만 가지며 <c>Set</c>과 <c>Reset</c>이 두 필드를 모두 바꾼다.</summary>
    [Fact]
    public void MoneyCursor_SetAndReset()
    {
        FortMoneyCursor cursor = new();
        Assert.Equal(0u, cursor.Position);
        Assert.Equal(0u, cursor.End);
        cursor.Set(4, 8);
        Assert.Equal(4u, cursor.Position);
        Assert.Equal(8u, cursor.End);
        cursor.Reset();
        Assert.Equal(0u, cursor.Position);
        Assert.Equal(0u, cursor.End);
    }

    /// <summary>Badges·Technology 커서는 data 포인터 존재 여부가 기본 참이고, <c>Reset</c>도 참으로 되돌린다.</summary>
    [Fact]
    public void MetaStream_PresentDefaultsToTrueAndSetOverridesIt()
    {
        FortMetaStream stream = new();
        Assert.True(stream.Present);
        Assert.True(stream.Bytes.IsEmpty);

        byte[] bytes = [5, 6];
        stream.Set(bytes, 1, 2, present: false);
        Assert.False(stream.Present);
        Assert.Equal(bytes, stream.Bytes.ToArray());
        Assert.Equal(1u, stream.Position);
        Assert.Equal(2u, stream.End);

        // 길이 0인 존재하는 섹션과 널 포인터 섹션은 Present로만 구별된다
        stream.Set(ReadOnlyMemory<byte>.Empty, 0, 0, present: true);
        Assert.True(stream.Present);
        stream.Reset();
        Assert.True(stream.Present);
        Assert.True(stream.Bytes.IsEmpty);
        Assert.Equal(0u, stream.Position);
        Assert.Equal(0u, stream.End);
    }
}
