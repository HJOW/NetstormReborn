using Netstorm.Core.Original;

// 실제 효과음과 음악의 같은 전역 음량·보류 깊이를 제어한다.
namespace Netstorm.Client.Audio;

/// <summary>cpppj/src/client/SoundMusic.cpp의 공개 Play·Stop·SetVolume·PushMute·PopMute 부분 이식이다.
/// 실제 채널과 파일·버퍼 장치는 호스트가 조립하며 같은 primary의 Update와 명령 잠금을 공유한다.</summary>
public sealed partial class SoundMusicControl
{
    /// <summary>Patch의 마지막 음량 복원 재조회와 CD 분기를 선택하는 실제 판본이다.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>효과음 재생기와 동일한 현재 전역이다. 예약 음량의 사본을 두지 않는다.</summary>
    private readonly SoundState _state;
    /// <summary>현재 물리 소리 표의 버퍼를 실제로 갱신하는 효과음 재생기다.</summary>
    private readonly SoundPlayer _effects;
    /// <summary>선택됐을 때만 요구하는 실제 음악 채널 SetVolume이다. 빈 성공 함수를 만들지 않는다.</summary>
    private readonly Action<int>? _setMusicChannelVolume;
    /// <summary>원문 recursive_mutex처럼 재진입과 중첩 음소거를 허용하는 실제 명령 잠금이다.</summary>
    private readonly object _gate = new();
    /// <summary>CPP SoundMusic::stopping_의 초기false를 이 제어 소유자에 보관한다.
    /// MusicInitialized와 다른 값이며, 아직 미이식인 실제 Runtime의 초기화·종료가 같은 소유자를 변경해야 한다.</summary>
    public bool Stopping { get; set; }

    /// <summary>같은 실제 전역·효과음 재생기만 결합하며 음악 채널의 존재나 준비 상태를 합성하지 않는다.</summary>
    public SoundMusicControl(OriginalEdition edition, SoundState state, SoundPlayer effects, Action<int>? setMusicChannelVolume = null)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(effects);
        if (!effects.UsesState(state)) throw new ArgumentException("음악 제어와 효과음의 동일 실제 전역 필요", nameof(effects));
        _edition = edition;
        _state = state;
        _effects = effects;
        _setMusicChannelVolume = setMusicChannelVolume;
    }

    /// <summary>실제 선택 위치에서만 채널 장치를 요구하고 자연 미준비 반환은 그대로 유지한다.</summary>
    private static T Need<T>(T? device) where T : Delegate => device ?? throw new InvalidOperationException("음악 제어의 선택된 실제 채널 미연결");

    /// <summary>같은 명령 잠금·준비/장치/활성/이름 관문 뒤 실제 열기·버퍼·시작을 실행한다. 열기 실패만 상위 demo 재선택을 허용한다.</summary>
    public bool Play(string? name, uint loop, MusicDirectories directories, Func<string, string?> find,
        MusicPlaybackDevices? devices, Action<string>? fallback = null)
    {
        lock (_gate)
        {
            if (Stopping || !_state.MusicInitialized || !_state.Device || Need(devices?.Active)() || name is null) return false;
            string selected = MusicByteText.CString(name);
            if (!Need(devices?.Open)(selected, directories, find))
            {
                if (MusicByteText.Same(selected, "demo.mus")) return false;
                fallback?.Invoke("demo.mus");
                return Need(devices?.Active)();
            }
            return Need(devices?.EnsureBuffer)() && Need(devices?.Start)(loop);
        }
    }

    /// <summary>같은 실제 음악 준비 상태에서만 현재 채널의 실제 정지 부모를 호출한다.</summary>
    public void Stop(MusicPlaybackDevices? devices)
    {
        lock (_gate)
        {
            if (!_state.MusicInitialized) return;
            Need(devices?.Stop)();
        }
    }

    /// <summary>보류 중에는 예약 음악 음량만 쓰고, 그 밖에는 현재값 저장 뒤 실제 준비 채널을 갱신한다.</summary>
    public void SetVolume(int volume)
    {
        lock (_gate)
        {
            if (_state.VolumeHoldDepth != 0) { _state.PendingMusicVolume = volume; return; }
            _state.MusicVolume = volume;
            if (_state.MusicInitialized && !Stopping)
                (_setMusicChannelVolume ?? throw new InvalidOperationException("음악 음량의 선택된 실제 채널 미연결"))(volume);
        }
    }

    /// <summary>최초 깊이0에서만 두 음량을 예약하고 실제 효과음·음악을 순서대로 낮춘 뒤 DWORD 깊이를 증가시킨다.</summary>
    public void PushMute()
    {
        lock (_gate)
        {
            if (_state.VolumeHoldDepth == 0)
            {
                _state.PendingMasterVolume = _state.MasterVolume;
                _state.PendingMusicVolume = _state.MusicVolume;
                _effects.SetMasterVolume(SoundConstants.Minimum);
                SetVolume(SoundConstants.Minimum);
            }
            _state.VolumeHoldDepth = unchecked(_state.VolumeHoldDepth + 1);
        }
    }

    /// <summary>signed 양수 깊이만 감소시키고 마지막 해제에서 최신 예약 음량을 복원한다. Patch는 효과음 재진입 뒤 깊이를 다시 읽는다.</summary>
    public void PopMute()
    {
        lock (_gate)
        {
            if (_state.VolumeHoldDepth <= 0) return;
            _state.VolumeHoldDepth--;
            if (_state.VolumeHoldDepth == 0)
            {
                _effects.SetMasterVolume(_state.PendingMasterVolume);
                if (_edition != OriginalEdition.Patch1078 || _state.VolumeHoldDepth == 0)
                    SetVolume(_state.PendingMusicVolume);
            }
        }
    }
}
