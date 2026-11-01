using Microsoft.Xna.Framework.Audio;
using Netstorm.Assets;

namespace Netstorm.Client.Host;

/// <summary>메인 메뉴 버튼 소리를 실제 WAV와 MonoGame 장치에 잇는다.
/// 원본의 공통 SoundPlayer 전체가 아니라 버튼 경계만 제공한다.</summary>
internal sealed class HostButtonSound : IDisposable
{
    private readonly HostStartupScene _startup;
    private readonly List<SoundEffectInstance> _playing = [];
    private SoundEffect? _effect;
    private bool _attempted;
    private bool _unavailable;

    public HostButtonSound(HostStartupScene startup) => _startup = startup;

    /// <summary>효과음 설정과 동시 재생 상한을 확인한 뒤 button.wav를 재생한다.</summary>
    public void Play()
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
            if (!_attempted)
            {
                _attempted = true;
                byte[]? bytes = _startup.Files.TryReadAllBytes("sound/button.wav");
                if (bytes is null) return;
                using var stream = new MemoryStream(bytes, writable: false);
                byte[] pcm = WaveFile.ReadPcm16(stream, out WaveFile wave);
                int rate = wave.SampleRate;
                // MonoGame 장치의 최소 표본율에 맞춘다.
                while (rate < 8000) rate *= 2;
                if (rate != wave.SampleRate) pcm = WaveFile.ResamplePcm16(pcm, wave.Channels, wave.SampleRate, rate);
                _effect = new SoundEffect(pcm, rate, wave.Channels == 2 ? AudioChannels.Stereo : AudioChannels.Mono);
            }
            if (_effect is null) return;
            // ClientAudio::VolumeFromStep의 1/100 dB를 선형 진폭으로 변환한다.
            int attenuation = _startup.Configuration.GetInt("soundVolume") switch
            {
                1 => -4000, 2 => -2000, 3 => -1000, 4 => -500, _ => 0
            };
            SoundEffectInstance instance = _effect.CreateInstance();
            try
            {
                instance.Volume = (float)Math.Pow(10, attenuation / 2000.0);
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

    public void Dispose()
    {
        foreach (SoundEffectInstance instance in _playing) instance.Dispose();
        _playing.Clear();
        _effect?.Dispose();
        _effect = null;
    }
}
