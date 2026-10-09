using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 커서 표(<see cref="GameCursors"/>) 테스트. 번호·그룹·그림 번호의 대응과 동봉 파일 18개의 존재·형식을 검사한다
/// (LEFT_JOBS.dotnetpj.md 5-4).
/// </summary>
public sealed class GameCursorTests
{
    /// <summary>원본 10.78 표 005423a8 의 번호 1~18 그룹 리소스 (cpppj/src/client/Cursor.h 와 같은 값).</summary>
    private static readonly int[] OriginalGroups =
        [113, 110, 111, 108, 107, 109, 115, 116, 131, 117, 130, 129, 132, 133, 134, 136, 141, 148];

    /// <summary>열거형 값이 원본 커서 번호 1~18 이고 그룹 번호가 원본 표 순서와 같다.</summary>
    [Fact]
    public void Numbers_FollowTheOriginalTable()
    {
        Assert.Equal(GameCursors.Count, GameCursors.All.Count);
        Assert.Equal(GameCursors.Count, Enum.GetValues<GameCursor>().Length);
        // 번호 1부터 순서대로 그룹 번호를 확인한다.
        for (int number = 1; number <= GameCursors.Count; number++)
        {
            GameCursor cursor = GameCursors.All[number - 1];
            Assert.Equal(number, (int)cursor);
            Assert.True(Enum.IsDefined(cursor));
            Assert.Equal(OriginalGroups[number - 1], GameCursors.GroupResource(cursor));
        }
    }

    /// <summary>이미 쓰임이 확인된 다섯 모양의 그룹·그림 번호가 바뀌지 않았다 (TEST02 픽셀 대조).</summary>
    /// <param name="cursor">커서 모양</param>
    /// <param name="group">그룹 리소스 번호</param>
    /// <param name="image">그림 리소스 번호</param>
    [Theory]
    [InlineData(GameCursor.Arrow, 113, 8)]
    [InlineData(GameCursor.Forbidden, 109, 5)]
    [InlineData(GameCursor.Move, 148, 20)]
    [InlineData(GameCursor.Temple, 111, 7)]
    [InlineData(GameCursor.Place, 110, 6)]
    public void ConfirmedStates_KeepTheirResources(GameCursor cursor, int group, int image)
    {
        Assert.Equal(group, GameCursors.GroupResource(cursor));
        Assert.Equal(image, GameCursors.ImageResource(cursor));
    }

    /// <summary>18개 모양 모두 서로 다른 동봉 파일을 가리키고, 그 파일은 32×32 단색 커서로 읽힌다.</summary>
    [Fact]
    public void EveryCursor_HasABundledBitmap()
    {
        GameFileSystem files = OriginalData.RequireResources().Files;
        var seen = new HashSet<int>();
        // 번호순으로 동봉 파일을 읽는다.
        foreach (GameCursor cursor in GameCursors.All)
        {
            int image = GameCursors.ImageResource(cursor);
            Assert.True(seen.Add(image), $"그림 번호가 겹칩니다: {image}");
            byte[]? bytes = files.TryReadAllBytes($"cursors/RT_CURSOR_{image}.bin");
            Assert.True(bytes != null, $"동봉 커서 파일이 없습니다: {image}");
            CursorBitmap bitmap = CursorBitmap.Parse(bytes);
            Assert.Equal((32, 32), (bitmap.Width, bitmap.Height));
        }
        Assert.Equal(Enumerable.Range(3, GameCursors.Count), seen.Order());
    }

    /// <summary>번호 0(미설정)과 범위 밖 번호는 거부한다.</summary>
    [Fact]
    public void InvalidNumbers_AreRejected()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => GameCursors.ImageResource(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => GameCursors.GroupResource((GameCursor)19));
    }
}
