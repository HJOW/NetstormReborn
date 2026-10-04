using System.Diagnostics;
using Microsoft.Xna.Framework.Audio;
using Netstorm.Assets;
using Netstorm.Core.Audio;

namespace Netstorm.Game;

/// <summary>
/// 원본 소리 파일을 그대로 재생한다 (7단계 오디오). 효과음은 `sound/*.wav` 를 메모리에 올려 겹쳐 틀고,
/// 배경음악은 `music/*.mus` 를 조금씩 읽어 스트리밍한다. 곡 선택은 <see cref="MusicDirector"/>(원본 규칙, docs/exe/music.md)가 정한다.
/// 소리 장치가 없거나 초기화에 실패하면 소리 없이 계속 실행한다.
/// </summary>
internal sealed class AudioPlayer : IDisposable
{
    /// <summary>동시에 재생하는 효과음 상한 (원본 setup.cfg maxSimulSounds = "8")</summary>
    private const int MaxSimultaneousSounds = 8;

    /// <summary>재생 장치가 받는 가장 낮은 표본율. 더 낮은 원본(6,000Hz)은 두 배씩 올린다</summary>
    private const int MinimumDeviceRate = 8000;

    /// <summary>음악 스트리밍 한 번에 넘기는 길이(초)</summary>
    private const double MusicChunkSeconds = 0.25;

    /// <summary>음악 장치에 미리 쌓아 두는 조각 수 (끊김 방지)</summary>
    private const int MusicQueuedChunks = 3;

    /// <summary>원본 데이터 폴더</summary>
    private readonly string _dataDirectory;

    /// <summary>파일 이름 → 효과음 (못 읽은 파일은 null 로 기억해 다시 시도하지 않는다)</summary>
    private readonly Dictionary<string, SoundEffect?> _sounds = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>재생 중인 효과음 (동시 재생 상한 확인용)</summary>
    private readonly List<SoundEffectInstance> _playing = [];

    /// <summary>실시간 시계 (음악 곡 끝 시각은 게임 정지와 무관한 실시간이다)</summary>
    private readonly Stopwatch _clock = Stopwatch.StartNew();

    /// <summary>음악 스트리밍 출력 (곡마다 새로 만든다)</summary>
    private DynamicSoundEffectInstance? _music;
    private SoundEffectInstance? _wind;
    private bool _speakerSwap;
    /// <summary>원본 바람 배경음을 독립적으로 끌 수 있다.</summary>
    public bool WindNoise { get; set; } = true;

    /// <summary>스트리밍 중인 음악 파일</summary>
    private FileStream? _musicStream;

    /// <summary>음악 파일의 남은 data 바이트 수</summary>
    private long _musicRemaining;

    /// <summary>음악 파일 형식</summary>
    private WaveFile? _musicFormat;

    /// <summary>소리 장치를 쓸 수 있는지 (초기화 실패 시 false)</summary>
    public bool Available { get; private set; } = true;

    /// <summary>곡 선택 규칙</summary>
    public MusicDirector Director { get; }

    /// <summary>표시 설정의 소리 항목 (켜기·볼륨)</summary>
    public bool SoundOn { get; set; }

    /// <summary>배경음악 켜기</summary>
    public bool MusicOn { get; set; }

    /// <summary>효과음 볼륨 단계 1~5</summary>
    public int SoundVolume { get; set; }

    /// <summary>음악 볼륨 단계 1~5</summary>
    public int MusicVolume { get; set; }

    /// <summary>UI 자동 검사에서 버튼 요청 중복·누락을 확인하는 누적 횟수. 음소거 중 요청도 포함한다.</summary>
    public long ButtonSoundRequests { get; private set; }

    /// <summary>마지막 효과음 요청 파일. 실제 재생 여부는 LastSoundResult로 구분한다.</summary>
    public string LastSoundCue { get; private set; } = "none";

    /// <summary>마지막 효과음 결과: none·played·disabled·unavailable·limited·missing.</summary>
    public string LastSoundResult { get; private set; } = "none";

    /// <summary>지금 실시간(초)</summary>
    public double Now => _clock.Elapsed.TotalSeconds;

    /// <summary>원본 데이터 폴더와 설정으로 만든다</summary>
    /// <param name="dataDirectory">원본 데이터 폴더 (sound/·music/ 이 있는 곳)</param>
    /// <param name="soundOn">효과음 켜기</param>
    /// <param name="musicOn">음악 켜기</param>
    /// <param name="soundVolume">효과음 볼륨 단계</param>
    /// <param name="musicVolume">음악 볼륨 단계</param>
    public AudioPlayer(string dataDirectory, bool soundOn, bool musicOn, int soundVolume, int musicVolume)
    {
        _dataDirectory = dataDirectory;
        SoundOn = soundOn;
        MusicOn = musicOn;
        SoundVolume = soundVolume;
        MusicVolume = musicVolume;
        Director = new MusicDirector(TrackDuration);
    }

    /// <summary>메뉴(전투 밖) 음악을 시작한다</summary>
    public void StartMenu() => Director.StartMenu(Now);

    /// <summary>전투 음악 순환을 시작한다 (첫 곡 난수는 시뮬레이션과 무관하므로 공용 난수를 쓴다)</summary>
    public void StartBattle() => Director.StartBattle(Now, Random.Shared.Next(MusicDirector.StartRandomRange));

    /// <summary>매 프레임: 곡 교체 확인, 쌓인 재생 요청 처리, 음악 스트리밍, 끝난 효과음 정리</summary>
    /// <param name="mySacrificeInProgress">내 제단의 희생 의식이 진행 중인지</param>
    public void Update(bool mySacrificeInProgress)
    {
        Director.Update(Now, mySacrificeInProgress);
        // 감독이 낸 요청을 차례로 실행한다
        foreach (AudioCue cue in Director.TakeCues())
        {
            if (cue.Kind == AudioCueKind.Music)
            {
                StartMusic(cue.Name);
            }
            else
            {
                PlaySound(cue.Name);
            }
        }
        FeedMusic();
        UpdateWind();
        _playing.RemoveAll(instance =>
        {
            if (instance.State != SoundState.Stopped)
            {
                return false;
            }
            instance.Dispose();
            return true;
        });
    }

    /// <summary>효과음 하나를 튼다. 꺼져 있거나 동시 재생 상한이면 무시한다.</summary>
    /// <param name="name">원본 sound/ 안의 파일 이름 (예: "bridgeFall.WAV")</param>
    public void PlaySound(string name)
    {
        // 소리 장치와 설정 상태도 기록해 요청만 있었던 경우를 실제 재생과 구분한다.
        LastSoundCue = name;
        if (name.Equals(OriginalUiSkin.ButtonSound, StringComparison.OrdinalIgnoreCase)) ButtonSoundRequests++;
        LastSoundResult = !SoundOn ? "disabled" : !Available ? "unavailable" : "limited";
        if (!Available || !SoundOn || _playing.Count >= MaxSimultaneousSounds)
        {
            return;
        }
        SoundEffect? effect = LoadSound(name);
        if (effect == null)
        {
            LastSoundResult = Available ? "missing" : "unavailable";
            return;
        }
        try
        {
            SoundEffectInstance instance = effect.CreateInstance();
            instance.Volume = SoundVolume / (float)Netstorm.Core.Display.DisplaySettings.MaximumVolume;
            instance.Play();
            _playing.Add(instance);
            LastSoundResult = "played";
        }
        catch (Exception error) when (error is NoAudioHardwareException or InstancePlayLimitException or InvalidOperationException)
        {
            Available = false;
            LastSoundResult = "unavailable";
        }
    }

    /// <summary>효과음 파일을 16bit PCM 으로 읽어 캐시한다 (6,000Hz 같은 낮은 표본율은 올린다)</summary>
    private SoundEffect? LoadSound(string name)
    {
        if (_sounds.TryGetValue(name, out SoundEffect? cached))
        {
            return cached;
        }
        SoundEffect? effect = null;
        string? path = GameDataLocator.FindFile(_dataDirectory, "sound/" + name);
        if (path != null)
        {
            try
            {
                using FileStream stream = File.OpenRead(path);
                byte[] pcm = WaveFile.ReadPcm16(stream, out WaveFile wave);
                int rate = wave.SampleRate;
                // 장치가 받는 표본율이 될 때까지 두 배로 올린다
                while (rate < MinimumDeviceRate)
                {
                    rate *= 2;
                }
                if (rate != wave.SampleRate)
                {
                    pcm = WaveFile.ResamplePcm16(pcm, wave.Channels, wave.SampleRate, rate);
                }
                if (_speakerSwap && wave.Channels == 2) PcmChannels.SwapStereo16(pcm);
                effect = new SoundEffect(pcm, rate, wave.Channels == 2 ? AudioChannels.Stereo : AudioChannels.Mono);
            }
            catch (Exception error) when (error is IOException or InvalidDataException or ArgumentException)
            {
                effect = null;
            }
            catch (NoAudioHardwareException)
            {
                Available = false;
            }
        }
        _sounds[name] = effect;
        return effect;
    }

    /// <summary>곡 길이(초). 파일이 없으면 0 (감독은 180초 뒤 다시 확인한다)</summary>
    private double TrackDuration(string name)
    {
        string? path = GameDataLocator.FindFile(_dataDirectory, "music/" + name);
        try
        {
            return path == null ? 0 : WaveFile.ReadHeader(path).DurationSeconds;
        }
        catch (Exception error) when (error is IOException or InvalidDataException)
        {
            return 0;
        }
    }

    /// <summary>지금 곡을 멈추고 새 곡의 스트리밍을 시작한다</summary>
    private void StartMusic(string name)
    {
        StopMusic();
        if (!Available || !MusicOn)
        {
            return;
        }
        string? path = GameDataLocator.FindFile(_dataDirectory, "music/" + name);
        if (path == null)
        {
            return;
        }
        try
        {
            _musicStream = File.OpenRead(path);
            _musicFormat = WaveFile.ReadHeader(_musicStream);
            _musicStream.Position = _musicFormat.DataOffset;
            _musicRemaining = _musicFormat.DataLength;
            _music = new DynamicSoundEffectInstance(_musicFormat.SampleRate,
                _musicFormat.Channels == 2 ? AudioChannels.Stereo : AudioChannels.Mono)
            {
                Volume = MusicVolume / (float)Netstorm.Core.Display.DisplaySettings.MaximumVolume,
            };
            FeedMusic();
            _music.Play();
        }
        catch (Exception error) when (error is IOException or InvalidDataException or ArgumentException)
        {
            StopMusic();
        }
        catch (NoAudioHardwareException)
        {
            StopMusic();
            Available = false;
        }
    }

    /// <summary>음악 장치 대기열이 비지 않도록 파일에서 다음 조각을 읽어 넘긴다</summary>
    private void FeedMusic()
    {
        if (_music == null || _musicStream == null || _musicFormat == null)
        {
            return;
        }
        int chunk = (int)(_musicFormat.SampleRate * MusicChunkSeconds) * _musicFormat.BlockAlign;
        // 대기열이 찰 때까지 조각을 넘긴다. 파일이 끝나면 더 넘기지 않는다 (곡 교체는 감독이 정한다)
        while (_music.PendingBufferCount < MusicQueuedChunks && _musicRemaining > 0)
        {
            int length = (int)Math.Min(chunk, _musicRemaining);
            var data = new byte[length];
            int read = _musicStream.Read(data, 0, length);
            if (read <= 0)
            {
                _musicRemaining = 0;
                break;
            }
            _musicRemaining -= read;
            byte[] pcm = _musicFormat.ToPcm16(data.AsSpan(0, read - read % _musicFormat.BlockAlign));
            if (_speakerSwap && _musicFormat.Channels == 2) PcmChannels.SwapStereo16(pcm);
            if (pcm.Length > 0)
            {
                _music.SubmitBuffer(pcm);
            }
        }
    }

    /// <summary>음악을 멈추고 파일을 닫는다</summary>
    private void StopMusic()
    {
        _music?.Stop();
        _music?.Dispose();
        _music = null;
        _musicStream?.Dispose();
        _musicStream = null;
        _musicFormat = null;
        _musicRemaining = 0;
    }

    /// <summary>개발용 상태 문구: 장치 사용 가능 여부, 켜기·볼륨, 지금 곡과 경과 시간</summary>
    public string Describe()
    {
        string device = Available ? "사용 가능" : "장치 없음(소리 끔)";
        string track = Director.Current == null ? "없음" : $"{Director.Current} {Now - Director.CurrentStartedAt:0.0}초";
        string streaming = _music?.State == SoundState.Playing ? "재생 중" : "정지";
        return $"소리: {device} · 효과음 {(SoundOn ? "켬" : "끔")} {SoundVolume}/5 · 음악 {(MusicOn ? "켬" : "끔")} {MusicVolume}/5 · 곡 {track} ({streaming})";
    }

    /// <summary>음악 켜기를 바꾼다. 켜면 지금 곡을 처음부터 다시 튼다.</summary>
    /// <param name="on">켜기</param>
    public void SetMusicOn(bool on)
    {
        MusicOn = on;
        if (!on)
        {
            StopMusic();
        }
        else if (Director.Current != null)
        {
            StartMusic(Director.Current);
        }
    }

    /// <summary>옵션 음량을 현재 재생 중인 효과음·음악에도 즉시 반영한다.</summary>
    public void ApplyVolumes()
    {
        if (_music != null) _music.Volume = MusicVolume / (float)Netstorm.Core.Display.DisplaySettings.MaximumVolume;
        // 이미 울리고 있는 효과음도 음량 변경·끄기를 반영한다.
        foreach (SoundEffectInstance instance in _playing)
        {
            instance.Volume = SoundOn ? SoundVolume / (float)Netstorm.Core.Display.DisplaySettings.MaximumVolume : 0f;
        }
        UpdateWind();
    }

    /// <summary>켜짐·소리 켜짐·음량 설정에 따라 원본 바람 효과음을 반복하거나 멈춘다.</summary>
    private void UpdateWind()
    {
        if (!Available || !SoundOn || !WindNoise)
        {
            _wind?.Stop(); _wind?.Dispose(); _wind = null; return;
        }
        try
        {
            if (_wind == null && LoadSound("distantWindQuiet-3000.wav") is { } sound)
            {
                _wind = sound.CreateInstance(); _wind.IsLooped = true;
                _wind.Volume = SoundVolume / (float)Netstorm.Core.Display.DisplaySettings.MaximumVolume;
                _wind.Play();
            }
            if (_wind != null) _wind.Volume = SoundVolume / (float)Netstorm.Core.Display.DisplaySettings.MaximumVolume;
        }
        catch (Exception error) when (error is NoAudioHardwareException or InstancePlayLimitException or InvalidOperationException)
        { Available = false; }
    }

    /// <summary>좌우 교환 상태를 즉시 반영하고 이전 채널 순서로 만든 소리 캐시를 비운다.</summary>
    public void SetSpeakerSwap(bool swap)
    {
        if (_speakerSwap == swap) return;
        _speakerSwap = swap;
        _wind?.Stop(); _wind?.Dispose(); _wind = null;
        // 이전 PCM으로 만든 재생 인스턴스를 먼저 종료한 뒤 원본 캐시를 해제한다.
        foreach (SoundEffectInstance instance in _playing) { instance.Stop(); instance.Dispose(); }
        _playing.Clear();
        // 다음 재생에서 교환된 채널로 효과음을 다시 읽는다.
        foreach (SoundEffect? effect in _sounds.Values) effect?.Dispose();
        _sounds.Clear();
        // 음악은 곡을 재시작하지 않고 다음 스트리밍 조각부터 채널 순서를 바꾼다.
        UpdateWind();
    }

    /// <summary>재생 중인 소리와 캐시를 모두 해제한다</summary>
    public void Dispose()
    {
        _wind?.Stop(); _wind?.Dispose(); _wind = null;
        StopMusic();
        // 재생 중인 효과음을 멈추고 해제한다
        foreach (SoundEffectInstance instance in _playing)
        {
            instance.Dispose();
        }
        _playing.Clear();
        // 캐시한 효과음을 해제한다
        foreach (SoundEffect? effect in _sounds.Values)
        {
            effect?.Dispose();
        }
        _sounds.Clear();
    }
}
