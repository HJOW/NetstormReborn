using Netstorm.Client.Audio;
using Netstorm.Core.Original;

// 같은 실제 시계의 중첩 정지와 Client UI 단일 보유 표식을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>CPP ClientMain의 Pause/PushGamePause/PopGamePause 경계를 창 없이 검사한다.</summary>
public sealed class ClientGamePauseTests
{
    /// <summary>같은 장기 LocalState와 실제 GameClock을 초기화한 소유자를 만든다.</summary>
    private static ClientClockGlobals Owner()
    {
        var local = new ClientLocalStateGlobals(new ClientGameplayGlobals(), new());
        var owner = new ClientClockGlobals(local);
        owner.InitializeFullTime(1000);
        return owner;
    }

    /// <summary>같은 UI 값은 시각 공급을 생략하며 실제 게임 시각만 멈추고 벽시각과 프레임 주소는 유지한다.</summary>
    [Fact]
    public void UiPauseSkipsDuplicateTimeReadsAndResumesActualClock()
    {
        var owner = Owner();
        var clock = owner.Clock;
        var frame = owner.Frame;
        owner.CaptureFrame(1100);
        ulong number = frame.Value.Number;
        int reads = 0;
        owner.Pause(false, () => throw new InvalidOperationException("중복 false의 시각 조회 금지"));
        owner.Pause(true, () => { reads++; return 1200; });
        double held = clock.GameSeconds(1200);
        Assert.True(owner.UiPauseHeld);
        Assert.True(clock.IsPaused());
        owner.Pause(true, () => throw new InvalidOperationException("중복 true의 시각 조회 금지"));
        Assert.Equal(1, reads);
        Assert.Equal(held, clock.GameSeconds(1800));
        Assert.Equal(0.8, clock.WallSeconds(1800), 12);
        Assert.Equal(number, frame.Value.Number);
        owner.Pause(false, () => { reads++; return 2000; });
        Assert.Equal(2, reads);
        Assert.False(owner.UiPauseHeld);
        Assert.False(clock.IsPaused());
        Assert.Equal(held + 0.1, clock.GameSeconds(2100), 12);
        Assert.Same(clock, owner.Clock);
        Assert.Same(frame, owner.Frame);
        Assert.Equal(number, frame.Value.Number);
    }

    /// <summary>독립 Push와 UI 정지는 중첩하며 State의 최종 Pop 뒤 다음 UI가 다시 정지를 보유한다.</summary>
    [Fact]
    public void FinalPopReleasesUiHeldFlagAndNextUiCanPauseAgain()
    {
        var owner = Owner();
        owner.Pause(true, () => 1200);
        owner.PushGamePause(1300);
        owner.PopGamePause(1500);
        Assert.True(owner.Clock.IsPaused());
        Assert.True(owner.UiPauseHeld);
        owner.PopGamePause(1800);
        Assert.False(owner.Clock.IsPaused());
        Assert.False(owner.UiPauseHeld);
        owner.Pause(true, () => 2000);
        Assert.True(owner.Clock.IsPaused());
        Assert.True(owner.UiPauseHeld);
        owner.Pause(false, () => 2200);
        Assert.False(owner.Clock.IsPaused());
        Assert.False(owner.UiPauseHeld);
    }

    /// <summary>UI 한 깊이를 먼저 반납해도 독립 스크립트 깊이는 남으며 추가 Pop은 시각을 바꾸지 않는다.</summary>
    [Fact]
    public void UiReleaseKeepsIndependentNestedPauseAndExtraPopIsHarmless()
    {
        var owner = Owner();
        owner.PushGamePause(1100);
        owner.Pause(true, () => 1200);
        owner.Pause(false, () => 1500);
        Assert.True(owner.Clock.IsPaused());
        Assert.False(owner.UiPauseHeld);
        Assert.Equal(0.1, owner.Clock.GameSeconds(1800), 12);
        owner.PopGamePause(2000);
        double before = owner.Clock.GameSeconds(2200);
        owner.PopGamePause(2200);
        Assert.Equal(before, owner.Clock.GameSeconds(2200));
        Assert.False(owner.Clock.IsPaused());
    }

    /// <summary>시각 공급 실패는 실제 정지와 UI 보유를 바꾸지 않으며 이후 정상 요청으로 복구한다.</summary>
    [Fact]
    public void FailedTimeReadPreservesCurrentPauseOwnership()
    {
        var owner = Owner();
        Assert.Throws<IOException>(() => owner.Pause(true, () => throw new IOException("현재 시각 실패")));
        Assert.False(owner.UiPauseHeld);
        Assert.False(owner.Clock.IsPaused());
        owner.Pause(true, () => 1200);
        Assert.Throws<IOException>(() => owner.Pause(false, () => throw new IOException("현재 시각 실패")));
        Assert.True(owner.UiPauseHeld);
        Assert.True(owner.Clock.IsPaused());
        owner.PopGamePause(1800);
        Assert.False(owner.UiPauseHeld);
    }

    /// <summary>실제 음소거 및 영상 갱신 잠금이 별도로 보유된 동안 게임 정지만 풀어도 두 소유권은 유지한다.</summary>
    [Fact]
    public void ClockPauseDoesNotReturnActualAudioMuteOrMovieUpdateLock()
    {
        var owner = Owner();
        var sound = new SoundState { MasterVolume = -400, MusicVolume = -600 };
        // 아직 장치를 초기화하지 않은 실제 음량 계층이며 이 검사가 물리 소리를 재생하면 실패한다.
        var effects = new SoundPlayer(new SoundList(OriginalEdition.Patch1078), sound, new()
        {
            Status = _ => throw new InvalidOperationException("미선택 소리 상태"),
            Play = (_, _) => throw new InvalidOperationException("미선택 재생"),
            SetPosition = (_, _) => throw new InvalidOperationException("미선택 위치"),
            SetVolume = (_, _) => throw new InvalidOperationException("미선택 장치 음량"),
            SetPan = (_, _) => throw new InvalidOperationException("미선택 좌우"),
            Stop = _ => throw new InvalidOperationException("미선택 정지"),
            Duplicate = (uint _, ref uint _) => throw new InvalidOperationException("미선택 복제"),
            Load = _ => throw new InvalidOperationException("미선택 파일 적재"),
        });
        var music = new SoundMusicControl(OriginalEdition.Patch1078, sound, effects);
        music.PushMute();
        owner.LocalState.PauseMovieUpdates(true);
        owner.LocalState.PauseMovieUpdates(true);
        owner.Pause(true, () => 1200);
        owner.PushGamePause(1300);
        owner.PopGamePause(1500);
        owner.PopGamePause(1800);
        Assert.False(owner.Clock.IsPaused());
        Assert.False(owner.UiPauseHeld);
        Assert.Equal(1, sound.VolumeHoldDepth);
        Assert.Equal(SoundConstants.Minimum, sound.MasterVolume);
        Assert.Equal(SoundConstants.Minimum, sound.MusicVolume);
        Assert.Equal(2, owner.LocalState.MovieUpdatePauseDepth);
        Assert.True(owner.LocalState.IsMovieUpdateLocked);
        music.PopMute();
        owner.LocalState.PauseMovieUpdates(false);
        owner.LocalState.PauseMovieUpdates(false);
        Assert.Equal(0, sound.VolumeHoldDepth);
        Assert.Equal(-400, sound.MasterVolume);
        Assert.Equal(-600, sound.MusicVolume);
        Assert.False(owner.LocalState.IsMovieUpdateLocked);
    }
}
