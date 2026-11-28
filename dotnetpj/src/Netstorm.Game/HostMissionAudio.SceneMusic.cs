using Netstorm.Client.Audio;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // 장기 채널/현재 월드와 같은 Scene 감독을 한 번 조립한다.

/// <summary>같은 음향 소유자의 실제 이름·길이·선택·효과음을 장면 감독에 제공한다.</summary>
public sealed partial class HostMissionAudio
{
    /// <summary>장치 재열기나 미션 교체에서 다시 만들지 않는 장기 감독이다.</summary>
    private SceneMusic? _sceneMusic;
    /// <summary>명시 결선 뒤 같은 감독만 공개하며 조회로 생성하거나 시작하지 않는다.</summary>
    public SceneMusic SceneSupervisor
    {
        get { EnsureAudioAlive(); return _sceneMusic ?? throw new InvalidOperationException("실제 Scene 음악 감독 미설치"); }
    }

    /// <summary>같은 RNG/현재 월드/실제 외부 표시만 결합한다. 바인딩은 시계·파일·곡·난수를 읽거나 시작하지 않는다.</summary>
    public void BindSceneMusic(GameRandom random, SceneMusicWorldQueries world, Action refresh,
        Action<string> loadPalette, Action thunder)
    {
        ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(refresh); ArgumentNullException.ThrowIfNull(loadPalette); ArgumentNullException.ThrowIfNull(thunder);
        EnsureAudioAlive();
        if (_sceneMusic is not null) throw new InvalidOperationException("장기 Scene 음악 감독 중복 설치");
        _sceneMusic = new(Scene, random, new()
        {
            WallSeconds = _wallSeconds,
            CurrentName = () => CurrentMusicName,
            Select = SelectMusic,
            Duration = () => CurrentMusicDuration,
            Sacrificing = world.Sacrificing,
            WaitingRoom = world.WaitingRoom,
            Refresh = refresh,
            AscendancyPalette = () => _audioStartup.Configuration.GetInt("ascendancyPalette") == 1,
            LoadPalette = loadPalette,
            ThunderFlash = thunder,
            WeatherSound = PlayGlobal,
        });
    }

    /// <summary>원문 Start의 fallback battle만 기록한다. 실제 CurrentBattle getter와 현재 월드 모드는 바꾸지 않는다.</summary>
    public void StartScene(bool battle) { SceneMusic supervisor = SceneSupervisor; Scene.Battle = battle ? 1u : 0u; supervisor.Start(); }
    /// <summary>주 스레드의 실제 현재 벽시각/모드로 감독 한 프레임을 실행한다.</summary>
    public void SceneFrame() => SceneSupervisor.Frame();
    /// <summary>같은 결과 잠금과 현재 채널을 통해 이름을 요청한다. 빈 곡으로 직접 Stop을 대체하지 않는다.</summary>
    public void RequestSceneMusic(string name) { ArgumentNullException.ThrowIfNull(name); SceneSupervisor.Request(name); }
    /// <summary>같은 원소 순서·실제 희생/날씨/효과음으로 다음 곡을 고른다.</summary>
    public void NextSceneMusic() => SceneSupervisor.Next();
}
