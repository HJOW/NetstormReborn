using Microsoft.Xna.Framework.Audio;
using Netstorm.Assets;

namespace Netstorm.Client.Host;

/// <summary>메뉴의 이름별 WAV를 실제 파일과 MonoGame 장치에 잇는다.
/// 원본 공통 SoundPlayer의 위치·음악 처리는 별도다.</summary>
internal sealed class HostButtonSound : IDisposable
{
    /// <summary>실제 설정과 파일을 보유한 시작 Client.</summary>
    private readonly HostStartupScene _startup;
    /// <summary>아직 재생 중인 인스턴스와 장치 수명.</summary>
    private readonly List<SoundEffectInstance> _playing = [];
    /// <summary>파일별 로드 결과. null은 해당 이름의 파일이 없음을 뜻한다.</summary>
    private readonly Dictionary<string, SoundEffect?> _effects = new(StringComparer.OrdinalIgnoreCase);
    /// <summary>장치 오류 이후 재생을 생략하는 상태.</summary>
    private bool _unavailable;

    /// <summary>같은 실제 설정과 파일 장치를 빌린다.</summary>
    public HostButtonSound(HostStartupScene startup) => _startup = startup;

    /// <summary>효과음 설정과 동시 재생 상한을 확인한 뒤 button.wav를 재생한다.</summary>
    public void Play() => PlayByName("button.wav");

    /// <summary>설정 변경을 현재 재생 중인 효과음 인스턴스에도 적용한다.</summary>
    public void ApplyOptions()
    {
        bool enabled = _startup.Configuration.GetInt("sound") != 0;
        float volume = Volume();
        foreach (SoundEffectInstance instance in _playing)
        {
            if (!enabled) instance.Stop();
            else instance.Volume = volume;
        }
    }

    /// <summary>원본 음량 단계의 감쇠를 장치 진폭으로 변환한다.</summary>
    private float Volume()
    {
        int attenuation = _startup.Configuration.GetInt("soundVolume") switch { 1 => -4000, 2 => -2000, 3 => -1000, 4 => -500, _ => 0 };
        return (float)Math.Pow(10, attenuation / 2000.0);
    }

    /// <summary>메뉴가 지정한 WAV를 같은 음량·동시 재생 제한으로 재생한다.</summary>
    public void PlayByName(string name)
    {
        _playing.RemoveAll(instance =>
        {
            if (instance.State != SoundState.Stopped) return false;
            instance.Dispose();
            return true;
        });
        if (_unavailable || _startup.Configuration.GetInt("sound") == 0 ||
            _playing.Count >= _startup.Configuration.GetInt("maxSimulSounds")) return;
        try
        {
            if (!_effects.TryGetValue(name, out SoundEffect? effect))
            {
                byte[]? bytes = _startup.Files.TryReadAllBytes("sound/" + name);
                if (bytes is null) { _effects[name] = null; return; }
                using var stream = new MemoryStream(bytes, writable: false);
                byte[] pcm = WaveFile.ReadPcm16(stream, out WaveFile wave);
                int rate = wave.SampleRate;
                // MonoGame 장치의 최소 표본율에 맞춘다.
                while (rate < 8000) rate *= 2;
                if (rate != wave.SampleRate) pcm = WaveFile.ResamplePcm16(pcm, wave.Channels, wave.SampleRate, rate);
                effect = new SoundEffect(pcm, rate, wave.Channels == 2 ? AudioChannels.Stereo : AudioChannels.Mono);
                _effects[name] = effect;
            }
            if (effect is null) return;
            // ClientAudio::VolumeFromStep의 1/100 dB를 선형 진폭으로 변환한다.
            SoundEffectInstance instance = effect.CreateInstance();
            try
            {
                instance.Volume = Volume();
                instance.Play();
                _playing.Add(instance);
            }
            catch { instance.Dispose(); throw; }
        }
        catch (Exception error) when (error is IOException or InvalidDataException or ArgumentException or
            NoAudioHardwareException or InstancePlayLimitException or InvalidOperationException)
        {
            // 장치 또는 파일 오류는 게임을 중단하지 않는다.
            _unavailable = true;
        }
    }

    /// <summary>재생 인스턴스와 캐시된 음향 자원을 모두 반납한다.</summary>
    public void Dispose()
    {
        foreach (SoundEffectInstance instance in _playing) instance.Dispose();
        _playing.Clear();
        // 캐시한 각 파일의 장치 자원을 모두 반납한다.
        foreach (SoundEffect? effect in _effects.Values) effect?.Dispose();
        _effects.Clear();
    }
}
