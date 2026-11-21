using Netstorm.Client.Gump;

// 같은 실제 공통 트리/로그와 장기 Chat 슬롯을 원문 기하/프레임 요청에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawMenuChat/ClientMain의 현재 UI 기하 장치와 프레임말 요청 소비다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>현재 공통 UI 저장소를 빌리는 실제 기하 부모다.게임 상태 사본은 만들지 않는다.</summary>
    private HudChatGeometry? _nativeChatGeometry;
    /// <summary>전체 ChatWindow 공장이 같은 실제 자식 부착/표시 장치를 빌린다.</summary>
    public HudChatGeometry NativeChatGeometry => _nativeChatGeometry ?? throw new InvalidOperationException("실제 Chat 기하 미설치");
    /// <summary>같은 현재 공통 트리/ScrollingText와 Root 모드/현재 창 getter를 연결한다.</summary>
    private void InitializeNativeChatGeometry()
    {
        var tree = (_mainMenu ?? throw new InvalidOperationException("Chat 기하의 실제 공통 트리 미설치")).Tree;
        var text = _htmlText ?? throw new InvalidOperationException("Chat 기하의 실제 공유 로그 장치 미설치");
        _nativeChatGeometry = new(new()
        {
            Chat = token => tree.Node(token) as HudChatNode, Node = tree.Node, BaseAttach = tree.Tree.Attach, Push = tree.Tree.Push,
            Attach = (token, x, y, parent, mode) => (tree.TreeHooks.Attach ?? throw new InvalidOperationException("Chat 기하의 실제 가상 Attach 미설치"))(token, x, y, parent, mode),
            BottomIndex = token => text.Text.AtBottom(token) ? 1u : 0, RequestBottom = (token, extra) => text.Text.RequestBottom(token, unchecked((int)extra)),
            Challenge = () => _surfaceGlobals.Storage.OwnerMode.Challenge ? 1u : 0, Current = () => _chatWindowGlobals.Current.Value,
        });
    }
    /// <summary>현재 그리기 뒤 같은 실제 inputLineOff를 소비하며 하위 실패에서는 요청을 유지한다.</summary>
    private void ConsumeNativeChatInputToggle() => HudChatGeometry.ConsumeChatInputToggle(_chatWindowGlobals.InputLineOff, () => NativeChatGeometry.ToggleInputVisible());
}
