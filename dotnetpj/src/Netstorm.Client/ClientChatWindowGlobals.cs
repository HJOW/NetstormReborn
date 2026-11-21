using Netstorm.Core.Original;

// Chat UI 수명의 현재 root 주소를 부분 이식하며 공유 로그/완성 목록은 후속에서 연결한다.
namespace Netstorm.Client;

/// <summary>ClientChatWindowGlobals의00548740을 먼저 소유하는 장기 부분 소유자다.</summary>
public sealed partial class ClientChatWindowGlobals
{
    /// <summary>표시 root의 BSS0이며 현재0에서는 입력줄 장치를 조회하지 않는다.</summary>
    public UIntBox Current { get; } = new();
}
