using Netstorm.Client.Chat;
using Netstorm.Core.Original;

// ChatState의 정적4800BYTE와 현재 subscriber 조회를 Client 수명에 보관한다.
namespace Netstorm.Client;

/// <summary>ClientMain.cpp ChatState의 PATCH/CD 경계를 유지한 장기 소유자다.</summary>
public sealed class ClientChatGlobals
{
    /// <summary>00558ed0의100×48BYTE이며 최초 BSS0을 한 번만 만든다.</summary>
    public byte[] Records { get; } = new byte[RawChatChannels.Bytes];
    /// <summary>PATCH만 구성한 실제 동일 기록 처리기다.</summary>
    private readonly RawChatChannels? _channels;
    /// <summary>기록을 다시 초기화하지 않고 PATCH의 실제 현재 getter만 빌린다.</summary>
    public ClientChatGlobals(OriginalEdition edition, Func<uint> currentSubscriber)
    {
        ArgumentNullException.ThrowIfNull(currentSubscriber); if (edition == OriginalEdition.Patch1078) _channels = new(edition, Records, currentSubscriber);
    }
    /// <summary>CD를 PATCH로 대체하지 않고 실제 선택 시 미복원을 명시한다.</summary>
    public RawChatChannels Channels => _channels ?? throw new InvalidOperationException("CDChat채널본체미복원");
}
