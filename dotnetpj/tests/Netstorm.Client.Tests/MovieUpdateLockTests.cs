using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 영화 갱신 잠금 깊이의 가감과 잠금 판정을 검증한다.
// 2026-11-26 1.060에서 SL-7용으로 추가했다.
public sealed class MovieUpdateLockTests
{
    /// <summary>잠금 깊이는 0에서 시작하고 양수에서만 잠긴다.</summary>
    [Fact]
    public void PauseDepthStartsAtZeroUnlocked()
    {
        var game = new ClientGameplayGlobals();
        var local = new ClientLocalStateGlobals(game, new AmbientSoundState());
        Assert.Equal(0, local.MovieUpdatePauseDepth);
        Assert.False(local.IsMovieUpdateLocked);
    }

    /// <summary>잠금·해제는 32비트 감김으로 가감하며 양수에서만 잠긴다.</summary>
    [Fact]
    public void PauseAndResumeMoveDepthWithWraparound()
    {
        var game = new ClientGameplayGlobals();
        var local = new ClientLocalStateGlobals(game, new AmbientSoundState());
        local.PauseMovieUpdates(true);
        local.PauseMovieUpdates(true);
        Assert.Equal(2, local.MovieUpdatePauseDepth);
        Assert.True(local.IsMovieUpdateLocked);
        local.PauseMovieUpdates(false);
        Assert.Equal(1, local.MovieUpdatePauseDepth);
        Assert.True(local.IsMovieUpdateLocked);
        local.PauseMovieUpdates(false);
        Assert.Equal(0, local.MovieUpdatePauseDepth);
        Assert.False(local.IsMovieUpdateLocked);
        // 원문 bit_cast 감김과 같이 0에서 풀면 -1이 되어 잠그지 않는다.
        local.PauseMovieUpdates(false);
        Assert.Equal(-1, local.MovieUpdatePauseDepth);
        Assert.False(local.IsMovieUpdateLocked);
    }
}
