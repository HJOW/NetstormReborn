// 공개 음악 갱신도 재생·음소거·종료와 같은 실제 명령 잠금으로 직렬화한다.
namespace Netstorm.Client.Audio;

/// <summary>cpppj SoundMusic::Update를 같은 공개 제어 소유자에 연결한다.</summary>
public sealed partial class SoundMusicControl
{
    /// <summary>종료 중/음악 미준비에는 장치를 요구하지 않고 실제 선택된 기본 채널만 한 번 갱신한다.</summary>
    public bool Update(MusicBufferHooks? hooks)
    {
        lock (_gate)
            return !Stopping && _state.MusicInitialized
                && (_primary ?? throw new InvalidOperationException("음악 갱신의 명시된 실제 기본 채널 미연결")).FillBuffer(hooks, _state);
    }
}
