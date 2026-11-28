using Netstorm.Core.Original;

// 같은 설정의 원문 기본값·음량 점프표와 최초/재해석의 서로 다른 호출 순서를 보존한다.
namespace Netstorm.Client.Audio;

/// <summary>cpppj ClientAudioOptions의 값 사본이다. 장치 준비·음소거 깊이·현재 곡은 이 값에 저장하지 않는다.</summary>
public sealed record ClientAudioOptions
{
    /// <summary>효과음 사용 옵션이며 최초값은1이다. 음악 장치 준비와 별개다.</summary>
    public bool Sound { get; init; } = true;
    /// <summary>현재 음악 사용 옵션이며 최초값은1이다.</summary>
    public bool Music { get; init; } = true;
    /// <summary>원문 최초 음질3이다. 범위 밖 실제 장치 보정은 Initialize가 수행한다.</summary>
    public int Quality { get; init; } = 3;
    /// <summary>효과음 음량의 원문 최초DWORD5다.</summary>
    public int SoundVolumeStep { get; init; } = 5;
    /// <summary>음악 음량의 원문 최초DWORD5다.</summary>
    public int MusicVolumeStep { get; init; } = 5;
    /// <summary>동시 효과음 한도의 최초8이다.0은 무제한이며 임의로 정규화하지 않는다.</summary>
    public int MaxSimulSounds { get; init; } = 8;
    /// <summary>원문 좌우 교환 옵션의 최초0이다.</summary>
    public bool SwapSpeakers { get; init; }

    /// <summary>원문 ReadClientAudioOptions의 키 순서로 명시된 값만 읽는다. 누락 키는 각 실행 파일 초기값을 유지한다.</summary>
    public static ClientAudioOptions Read(ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(configuration);
        int sound = 1, music = 1, quality = 3, soundVolume = 5, musicVolume = 5, maxSounds = 8, swap = 0;
        configuration.ReadInt("sound", ref sound);
        configuration.ReadInt("music", ref music);
        configuration.ReadInt("soundQuality", ref quality);
        configuration.ReadInt("soundVolume", ref soundVolume);
        configuration.ReadInt("musicVolume", ref musicVolume);
        configuration.ReadInt("maxSimulSounds", ref maxSounds);
        configuration.ReadInt("swapLeftRightSpeakers", ref swap);
        return new()
        {
            Sound = sound != 0, Music = music != 0, Quality = quality,
            SoundVolumeStep = soundVolume, MusicVolumeStep = musicVolume, MaxSimulSounds = maxSounds, SwapSpeakers = swap != 0,
        };
    }

    /// <summary>원문4359d0/4359e0의 unsigned(step-1) 점프표다.1~4만 감쇠를 반환하고0·5·음수·범위 밖은 모두0이다.</summary>
    public static int VolumeFromStep(int step) => unchecked((uint)step - 1u) switch
    {
        0 => -4000,
        1 => -2000,
        2 => -1000,
        3 => -500,
        _ => 0,
    };
}

/// <summary>음향 옵션의 선택된 실제 효과 입구다. State 전체 InterpretOptions와 서로 다른 경계다.</summary>
public sealed class ClientAudioInterpretHooks
{
    /// <summary>현재 옵션 전역을 장치 변경보다 먼저 적용한다.</summary>
    public Action<ClientAudioOptions>? Globals { get; init; }
    /// <summary>같은 실제 장치/Runtime을 현재 음질로 연다. 이미 열림과 실패의 반환 정책은 소유자가 맡는다.</summary>
    public Action<int>? Open { get; init; }
    /// <summary>같은 실제 Runtime을 합류하고 장치를 닫되 파일 저장소를 폐기하지 않는다.</summary>
    public Action? Close { get; init; }
    /// <summary>공통 음소거의 같은 예약 상태를 사용하는 실제 효과음 음량 부모다.</summary>
    public Action<int>? EffectVolume { get; init; }
    /// <summary>공통 음소거의 같은 예약 상태와 기본 채널을 사용하는 실제 음악 음량 부모다.</summary>
    public Action<int>? MusicVolume { get; init; }
    /// <summary>장치 자연 반환 뒤 같은 현재 이름과 음악 옵션을 실제 선택기에 전달한다.</summary>
    public Action<bool>? SelectMusic { get; init; }
}

/// <summary>cpppj ApplyValues/ReinterpretClientAudioOptions의 순수 호출 순서다. 장치 성공이나 현재 전역을 대신 구현하지 않는다.</summary>
public static class ClientAudioOptionPolicy
{
    /// <summary>선택된 함수 위치에서만 누락을 거부하여 앞선 실제 효과와 부분 쓰기를 보존한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("음향 옵션의 선택된 실제 장치 미연결");

    /// <summary>최초 초기화는 전역→효과음 음량→음악 음량만 적용한다. 그 뒤 sound와 무관한 OpenDevice는 실제 소유자가 호출한다.</summary>
    public static void ApplyInitialValues(ClientAudioOptions options, ClientAudioInterpretHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(options); ArgumentNullException.ThrowIfNull(hooks);
        Need(hooks.Globals)(options);
        Need(hooks.EffectVolume)(ClientAudioOptions.VolumeFromStep(options.SoundVolumeStep));
        Need(hooks.MusicVolume)(ClientAudioOptions.VolumeFromStep(options.MusicVolumeStep));
    }

    /// <summary>전역→sound별 열기/닫기→두 음량→현재 곡 순서다. 실제 열기의 자연 실패도 뒤 음량/곡 적용을 생략하지 않는다.</summary>
    public static void Reinterpret(ClientAudioOptions options, ClientAudioInterpretHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(options); ArgumentNullException.ThrowIfNull(hooks);
        Need(hooks.Globals)(options);
        if (options.Sound) Need(hooks.Open)(options.Quality);
        else Need(hooks.Close)();
        Need(hooks.EffectVolume)(ClientAudioOptions.VolumeFromStep(options.SoundVolumeStep));
        Need(hooks.MusicVolume)(ClientAudioOptions.VolumeFromStep(options.MusicVolumeStep));
        Need(hooks.SelectMusic)(options.Music);
    }
}
