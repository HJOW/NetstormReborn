using Netstorm.Client.Input;

// 커서와 진단 소유자는 World/Session의 생성과 반납을 넘어 같은 주소를 유지한다.
namespace Netstorm.Client;

/// <summary>Gameplay의 실제 장기 커서/AI 진단 부분이다. 렌더러/가상 효과는 별도 장치다.</summary>
public sealed partial class ClientGameplayGlobals
{
    /// <summary>실제 Gameplay 최초 잠금1이며 독립 raw struct의0과 구분한다.</summary>
    public CursorDropState Cursor { get; } = new() { LockDepth = 1 };
    /// <summary>수명·위치·고정 배열과 회전/격자 옵션의 같은 장기 owner다.</summary>
    public ClientCursorGlobals CursorGlobals { get; } = new();
    /// <summary>메뉴/커서/표면/Session이 현재값을 빌리는 단일 진단 소유자다.</summary>
    public ClientAiDebugGlobals AiDebug { get; } = new();
    /// <summary>기존 일반 World의 fresh 커서만 명시 준비한다. prepared 대여/반납에는 호출하지 않는다.</summary>
    public void ResetCursorForGeneralWorld() { Cursor.ResetForGeneralWorld(); CursorGlobals.ResetForGeneralWorld(); }
}
