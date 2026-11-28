// 실제 시작/State 부모가 같은 현재 옵션 하위 몸체를 빌리는 입구다.
namespace Netstorm.Client.Host;

/// <summary>다섯 값 해석과 실제 소유자를 연결한다. 전체435220/표시·언어·음향 재해석과 구별한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은 실제 Config/Client 값을 빌린다. 자산 없는 독립 화면은 미설치다.</summary>
    private readonly ClientValueOptions? _nativeValueOptions;
    /// <summary>State/Session 반납 뒤에도 같은 저장 억제 DWORD를 반환한다.</summary>
    public Netstorm.Core.Original.UIntBox ReadOnlyFortOption => _stateResources.ReadOnlyForts;
    /// <summary>복원된 다섯 하위 해석과 채팅 입력 전환 값을 원문 상대 순서로 읽는다. 전체 옵션 부모를 대신하지 않는다.</summary>
    public void InterpretNativeValueOptions()
    {
        var values = _nativeValueOptions ?? throw new InvalidOperationException("값 옵션의 실제 시작 설정 미설치");
        values.InterpretCursor(); values.InterpretMainMenu();
        _chatWindowGlobals.ReadOptions((_startup ?? throw new InvalidOperationException("채팅 값 옵션의 실제 설정 미설치")).Configuration);
        values.InterpretFortFiles(); values.InterpretScriptMenu(); values.InterpretNetwork();
    }
}
