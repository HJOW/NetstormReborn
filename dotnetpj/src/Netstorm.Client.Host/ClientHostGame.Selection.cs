using Netstorm.Core.Rules;

// 현재 실제 하드웨어 커서를 선택/강조 정리의 같은 Client owner에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>전체Root 기본 장치 조립에서도 재사용할 실제 선택 정리 입구다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>실제 현재 커서를 선택하며 모양을 합성하거나 초기화 누락을 성공 처리하지 않는다.</summary>
    public void SelectNativeCursor(uint cursor) => (_cursor ?? throw new InvalidOperationException("실제Client커서미설치")).Set(unchecked((GameCursor)(int)cursor));
    /// <summary>같은 장기 선택/Pool/표시와 실제 현재 커서를 사용해 원문 Clear를 호출한다.</summary>
    public void ClearNativeUserSelection() => ClientUserSelectionCleanup.Clear(_gameplay, _surfaceGlobals.Storage, SelectNativeCursor);
}
