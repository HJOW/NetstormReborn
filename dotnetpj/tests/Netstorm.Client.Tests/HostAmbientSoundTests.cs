using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 미션 주변음의 실제 재생 전달과 원문 종류별 요청을 검증한다.
// 2026-11-26 1.056에서 SL-3c 주변음용으로 추가했다.
public sealed class HostAmbientSoundTests
{
    /// <summary>음소거 미션 음향을 실제 자산으로 만든다.</summary>
    /// <returns>시작 장면과 미션 음향이다.</returns>
    private static (HostStartupScene Startup, HostMissionAudio Audio) MutedScene()
    {
        if (!OperatingSystem.IsWindows()) Assert.Fail("주변음 검사는 Windows 전용이다");
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        return (startup, new HostMissionAudio(startup, 0, true, () => 1));
    }

    /// <summary>음소거에서는 주변음 요청이 장치 없이 0으로 끝난다.</summary>
    [Fact]
    public void MutedAmbientPlaybackReturnsZeroWithoutDevice()
    {
        (_, HostMissionAudio audio) = MutedScene();
        Assert.Equal(0u, audio.PlayAmbient("thunderQuietDistant.wav", 0u, 0, 0, 0u, 0));
        Assert.Equal(0u, audio.PlayAmbient("eagleScream.wav", 0u, 0, 123, 0u, 0));
        Assert.Equal(0u, audio.PlayAmbient("whale.wav", 0u, 0, -456, 0u, 0));
    }

    /// <summary>제품 결선 모양의 갈고리가 종류별 주변음을 미션 음향에 전달한다.</summary>
    [Fact]
    public void AmbientStepReachesMissionAudioLikeProductWiring()
    {
        (_, HostMissionAudio audio) = MutedScene();
        var sounds = new List<string>();
        var flashes = new List<int>();
        var state = new AmbientSoundState { Deadline = 0, Forced = 1, Kind = 0 };
        // 제품과 같은 모양으로 미션 음향이 있을 때만 실제 재생기에 전달한다.
        void Sound(string name, uint loop, int volume, int pan, uint priority, int limit)
        {
            sounds.Add(name);
            audio.PlayAmbient(name, loop, volume, pan, priority, limit);
        }
        var random = new GameRandom(17);
        RawAmbientSound.Step(state, random, new AmbientSoundHooks
        {
            WallSeconds = () => 1000,
            Sound = Sound,
            CurrentDialog = () => 0u,
            Flash = value => flashes.Add(value),
        });
        Assert.Equal(["thunderQuietDistant.wav"], sounds);
        Assert.Equal([0], flashes);
        // 독수리 종류는 번쩍임 없이 좌우를 넣어 재생한다.
        sounds.Clear(); state.Kind = 1;
        RawAmbientSound.Step(state, random, new AmbientSoundHooks
        {
            WallSeconds = () => 2000,
            Sound = Sound,
            CurrentDialog = () => 0u,
            Flash = value => flashes.Add(value),
        });
        Assert.Equal(["eagleScream.wav"], sounds);
        Assert.Equal([0], flashes);
    }
}
