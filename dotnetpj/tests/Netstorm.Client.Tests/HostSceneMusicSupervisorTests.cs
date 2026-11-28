using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 실제 준비 음향·현재 월드·공통 Scene 상자의 감독 결선을 검사한다.

/// <summary>하드웨어 초기화나 가짜 재생 성공 없이 같은 장기 소유자와 선택된 표시 경계를 검증한다.</summary>
public sealed class HostSceneMusicSupervisorTests
{
    /// <summary>미준비 장치와 실제 현재 Surface 조회를 조립하는 독립 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>읽기 전용 시작 자산과 현재 설정이다.</summary>
        public readonly HostStartupScene Startup = new(GameDataLocator.FindDataDirectory() ?? throw new InvalidOperationException("시작 자산 부재"));
        /// <summary>같은 실제 Player/카메라/월드 소유자다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>State와 감독이 공유하는 장기 음악 상태다.</summary>
        public readonly ClientStateResources State = new();
        /// <summary>실제 CurrentBattle/PlayerKnown를 제공한다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>이 실제 Surface의 희생/대기실 조회다.</summary>
        public readonly SceneMusicWorldQueries World;
        /// <summary>실제 파일/음악 채널을 준비만 한 장기 음향이다.</summary>
        public readonly HostMissionAudio Audio;
        /// <summary>동적 벽시각 조회 횟수다.</summary>
        public int ClockReads;
        /// <summary>선택된 실제 외부 표시 요청을 관찰한다.</summary>
        public int Refreshes;
        /// <summary>팔레트 로더 경계가 받은 확장자 포함 이름이다.</summary>
        public readonly List<string> Palettes = [];

        /// <summary>실제 owner만 결합하고 Scene/장치/곡은 시작하지 않는다.</summary>
        public Scene()
        {
            var local = new ClientLocalStateGlobals(Game, new()); var connections = new ClientConnectionGlobals(local);
            var clock = new ClientClockGlobals(local); Surface = new(Game, clock, new(), () => throw new IOException("미선택 동적 밀리초"));
            State.BindCurrentMusicOwners(Game, Surface, local, connections, Startup.Cloud);
            World = SceneMusicWorldQueries.ForCurrentSurface(Surface, new());
            Audio = new(Startup, () => { ClockReads++; return 100; }, State.Music);
            new uint[] { 1, 7, 11, 19 }.CopyTo(State.Music.Tints, 0);
        }

        /// <summary>실제 음악·월드와 관측하는 외부 표시 경계만 결합한다.</summary>
        public void Bind(Action<string>? load = null) => Audio.BindSceneMusic(Startup.Random, World,
            () => Refreshes++, load ?? Palettes.Add, () => throw new IOException("미선택 번개"));
        /// <summary>자기 장기 파일/음향 소유자를 반납한다. 실제 장치를 연 것으로 합산하지 않는다.</summary>
        public void Dispose() => Audio.Dispose();
    }

    /// <summary>바인딩은 같은 상태/RNG/채널을 유지하고 시각·곡·날씨를 선택하지 않는다.</summary>
    [Fact]
    public void BindingIsQuietAndDuplicateNeverReplacesTheSupervisor()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new Scene(); uint random = scene.Startup.Random.State(); string name = scene.Audio.CurrentMusicName;
        scene.State.Music.SongEnd.Value = 77; scene.Bind(); var supervisor = scene.Audio.SceneSupervisor;
        Assert.Same(scene.State.Music, scene.Audio.Scene); Assert.Equal(random, scene.Startup.Random.State());
        Assert.Equal(0, scene.ClockReads); Assert.Equal(name, scene.Audio.CurrentMusicName); Assert.Equal(77, scene.State.Music.SongEnd.Value);
        Assert.False(scene.Audio.DeviceReady); Assert.Equal(0, scene.Refreshes); Assert.Empty(scene.Palettes);
        Assert.Throws<InvalidOperationException>(() => scene.Bind()); Assert.Same(supervisor, scene.Audio.SceneSupervisor);
    }

    /// <summary>Start 인자는 fallback만 기록하며 실제 현재 Battle0이 메뉴 선택과 RNG 무소비를 결정한다.</summary>
    [Fact]
    public void StartUsesActualCurrentBattleAndTheSameUnpreparedMusicChannel()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new Scene(); scene.Bind(); uint random = scene.Startup.Random.State();
        scene.Audio.StartScene(true);
        Assert.Equal(1u, scene.State.Music.Battle); Assert.False(scene.Surface.Storage.OwnerMode.Battle);
        Assert.Equal(SceneMusic.MenuMusic, scene.Audio.CurrentMusicName); Assert.Equal(280.0, scene.State.Music.SongEnd.Value);
        Assert.Equal(19u, scene.State.Music.Tint); Assert.Equal(2, scene.ClockReads); Assert.Equal(random, scene.Startup.Random.State());
        Assert.False(scene.Audio.DeviceReady); Assert.Empty(scene.Palettes);
    }

    /// <summary>현재 옵션이 정확히1일 때만 같은 파일 후단을 호출하고 갱신된 Tints가 현재 Tint를 되쓰지 않는다.</summary>
    [Fact]
    public void CurrentPaletteOptionAndLoaderKeepWeatherWriteOrder()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new Scene(); scene.Bind(name => { scene.Palettes.Add(name); scene.State.Music.Tints[1] = 99; });
        scene.Audio.StartScene(false); scene.Startup.Configuration.SetInt("ascendancyPalette", 2); scene.Audio.NextSceneMusic();
        Assert.Equal(1, scene.Refreshes); Assert.Empty(scene.Palettes);
        scene.Startup.Configuration.SetInt("ascendancyPalette", 1); scene.Audio.NextSceneMusic();
        Assert.Equal(["rainy.col"], scene.Palettes); Assert.Equal(2, scene.Refreshes);
        Assert.Equal(7u, scene.State.Music.Tint); Assert.Equal(99u, scene.State.Music.Tints[1]);
        Assert.Equal(1u, scene.Startup.Cloud.WorldRedraw); Assert.Equal(1u, scene.State.Music.PaletteDirty);
    }

    /// <summary>장치 닫기는 감독을 유지하고 최종 Dispose 뒤에는 새 감독이나 효과를 만들지 않는다.</summary>
    [Fact]
    public void DeviceCloseKeepsSupervisorAndFinalDisposeRejectsFurtherCalls()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new Scene(); scene.Bind(); var supervisor = scene.Audio.SceneSupervisor;
        scene.Audio.CloseDevice(); Assert.Same(supervisor, scene.Audio.SceneSupervisor); scene.Audio.Dispose();
        Assert.Throws<ObjectDisposedException>(() => scene.Audio.StartScene(false));
        Assert.Throws<ObjectDisposedException>(() => scene.Audio.SceneFrame());
    }
}
