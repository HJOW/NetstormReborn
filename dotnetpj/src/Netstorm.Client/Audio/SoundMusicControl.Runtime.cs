using Netstorm.Core.Original;

// 실제 기본 채널의 명시 소유자와 기존 음악 명령 잠금을 Runtime에 빌린다.
namespace Netstorm.Client.Audio;

/// <summary>기존 음량 전용 생성자는 보존하고 실제 Runtime용 기본 채널 결합을 추가한다.</summary>
public sealed partial class SoundMusicControl
{
    /// <summary>실제 채널 factory에서만 기록한다. 기존 callback 전용 제어는 Runtime을 소유하지 않는다.</summary>
    private readonly MusicChannel? _primary;

    /// <summary>기존 전역·효과음 검사를 재사용하고 음량 callback과 채널 identity를 같은 대상으로 묶는다.</summary>
    private SoundMusicControl(OriginalEdition edition, SoundState state, SoundPlayer effects, MusicChannel primary, bool bindPrimary)
        : this(edition, state, effects, primary.SetVolume)
    {
        _primary = primary;
    }

    /// <summary>실제 기본 채널을 명시해 Runtime·Update·음량이 같은 채널과 기존 명령 잠금을 사용하게 한다.</summary>
    public static SoundMusicControl ForPrimary(OriginalEdition edition, SoundState state, SoundPlayer effects, MusicChannel primary)
    {
        ArgumentNullException.ThrowIfNull(primary);
        return new SoundMusicControl(edition, state, effects, primary, true);
    }

    /// <summary>실제 기본 채널을 조회한다. callback 전용 제어를 실제 Runtime으로 간주하지 않는다.</summary>
    internal MusicChannel Primary => _primary ?? throw new InvalidOperationException("음악 제어의 명시 기본 채널 미연결");
    /// <summary>Play·Stop·Update·음량·Runtime이 함께 쓰는 동일 재진입 잠금이다.</summary>
    internal object CommandGate => _gate;
    /// <summary>원문 초기화 assert의 판본별 줄 번호를 선택한다.</summary>
    internal OriginalEdition Edition => _edition;
    /// <summary>다른 전역이나 callback만 같은 채널을 Runtime의 실제 소유자로 오인하지 않게 검사한다.</summary>
    internal bool UsesRuntime(SoundState state, MusicChannel primary) => ReferenceEquals(_state, state) && ReferenceEquals(_primary, primary);
}
