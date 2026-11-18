// 공유 주변음의 기한·종류·분기 진행을 실제 기계어 기대값으로 대조한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 주변음을 검사하는 클래스다. 2026-11-19 0.633에서 R4-4 정기 처리용으로 추가했다.
// 근거: cpppj o/RawAmbientSound.cpp의 StepAmbientSound·ResetAmbientSoundDeadline다.
public sealed class RawAmbientSoundTests
{
    /// <summary>기한이 미래이면 시계만 조회하고 난수·상태를 보존한다. 2026-11-19 0.633에서 추가했다.</summary>
    [Fact]
    public void StepSkipsWhenDeadlineInFuture()
    {
        var state = new AmbientSoundState { Deadline = 100.0, Kind = 7, Forced = 3 };
        var random = new GameRandom(1u);
        var calls = new List<string>();
        RawAmbientSound.Step(state, random, new AmbientSoundHooks
        {
            WallSeconds = () => 50.0,
            Sound = (name, _, _, _, _, _) => calls.Add(name),
            CurrentDialog = () => { calls.Add("dialog"); return 0u; },
            Flash = _ => calls.Add("flash"),
        });
        Assert.Equal(100.0, state.Deadline);
        Assert.Equal(7, state.Kind);
        Assert.Equal(1u, random.State());
        Assert.Empty(calls);
    }

    /// <summary>천둥 종류는 난수 1회·소리·번쩍임을 수행하고 120초 뒤로 재예약한다. 2026-11-19 0.633에서 추가했다.</summary>
    [Fact]
    public void StepPlaysThunderAndRearms()
    {
        var state = new AmbientSoundState();
        var random = new GameRandom(1u);
        var calls = new List<string>();
        RawAmbientSound.Step(state, random, new AmbientSoundHooks
        {
            WallSeconds = () => 50.0,
            Sound = (name, _, _, _, _, _) => calls.Add(name),
            CurrentDialog = () => 0u,
            Flash = _ => calls.Add("flash"),
        });
        Assert.Equal(170.0, state.Deadline);
        Assert.Equal(0, state.Kind);
        Assert.Equal(0x10006u, random.State());
        Assert.Equal(["thunderQuietDistant.wav", "flash"], calls);
    }

    /// <summary>독수리 종류는 좌우 난수를 더 소비한다. 2026-11-19 0.633에서 추가했다.</summary>
    [Fact]
    public void StepPlaysEagleWithPan()
    {
        var state = new AmbientSoundState();
        var random = new GameRandom(1000u);
        string sound = string.Empty;
        int pan = 0;
        RawAmbientSound.Step(state, random, new AmbientSoundHooks
        {
            WallSeconds = () => 50.0,
            Sound = (name, _, _, value, _, _) => { sound = name; pan = value; },
            CurrentDialog = () => 9u,
            Flash = _ => Assert.Fail("천둥이 아니므로 번쩍이면 안 된다"),
        });
        Assert.Equal(170.0, state.Deadline);
        Assert.Equal(1, state.Kind);
        Assert.Equal(["eagleScream.wav"], new[] { sound });
        Assert.Equal(-3997, pan);
    }

    /// <summary>고래 종류는 좌우 난수를 더 소비한다. 2026-11-19 0.633에서 추가했다.</summary>
    [Fact]
    public void StepPlaysWhaleWithPan()
    {
        var state = new AmbientSoundState();
        var random = new GameRandom(0u);
        string sound = string.Empty;
        int pan = 0;
        RawAmbientSound.Step(state, random, new AmbientSoundHooks
        {
            WallSeconds = () => 50.0,
            Sound = (name, _, _, value, _, _) => { sound = name; pan = value; },
            CurrentDialog = () => 9u,
            Flash = _ => Assert.Fail("천둥이 아니므로 번쩍이면 안 된다"),
        });
        Assert.Equal(2, state.Kind);
        Assert.Equal(["whale.wav"], new[] { sound });
        Assert.Equal(-5162, pan);
    }

    /// <summary>강제 양수이면 기존 종류를 유지하고 종류 난수를 건너뛴다. 2026-11-19 0.633에서 추가했다.</summary>
    [Fact]
    public void StepKeepsKindWhenForced()
    {
        var state = new AmbientSoundState { Forced = 1, Kind = 2 };
        var random = new GameRandom(0u);
        string sound = string.Empty;
        RawAmbientSound.Step(state, random, new AmbientSoundHooks
        {
            WallSeconds = () => 50.0,
            Sound = (name, _, _, _, _, _) => sound = name,
            CurrentDialog = () => 9u,
            Flash = _ => Assert.Fail("천둥이 아니므로 번쩍이면 안 된다"),
        });
        Assert.Equal(2, state.Kind);
        Assert.Equal("whale.wav", sound);
    }

    /// <summary>재예약은 현재 벽시각+10을 기한에 둔다. 2026-11-19 0.633에서 추가했다.</summary>
    [Fact]
    public void ResetDeadlineAddsTenSeconds()
    {
        var state = new AmbientSoundState();
        RawAmbientSound.ResetDeadline(state, () => 50.0);
        Assert.Equal(60.0, state.Deadline);
    }

    /// <summary>없는 상태·난수·장치는 실제 호출 전에 거부한다. 2026-11-19 0.633에서 추가했다.</summary>
    [Fact]
    public void StepRejectsMissingDevices()
    {
        var state = new AmbientSoundState();
        var random = new GameRandom(1u);
        var hooks = new AmbientSoundHooks
        {
            WallSeconds = () => 0.0,
            Sound = (_, _, _, _, _, _) => { },
            CurrentDialog = () => 0u,
            Flash = _ => { },
        };
        Assert.Throws<ArgumentNullException>(() => RawAmbientSound.Step(null!, random, hooks));
        Assert.Throws<ArgumentNullException>(() => RawAmbientSound.Step(state, null!, hooks));
        Assert.Throws<ArgumentNullException>(() => RawAmbientSound.Step(state, random, null!));
        Assert.Throws<ArgumentNullException>(() => RawAmbientSound.ResetDeadline(null!, () => 0.0));
        Assert.Throws<ArgumentNullException>(() => RawAmbientSound.ResetDeadline(state, null!));
        Assert.Throws<InvalidOperationException>(() => RawAmbientSound.Step(state, random, new AmbientSoundHooks()));
    }
}
