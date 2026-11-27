using Netstorm.Client.Gump;

// 실제 미션의 구분선을 기준으로 채팅창을 부착한다.
namespace Netstorm.Client.Host;

/// <summary>미션 HUD의 채팅창을 공통 트리에 연결하는 호스트다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>채팅창을 만들고 현재 세로·채팅 구분선이 정한 영역에 부착한다.</summary>
    /// <remarks>원본 UberRootGump와 HudDock의 세로 slave 4·채팅 slave 8 배치를 사용한다. 구분선 이동에 따른 전체 재배치는 R5-4d에서 연결한다.</remarks>
    private void AttachMissionChat()
    {
        var tree = (_mainMenu ?? throw new InvalidOperationException("채팅창의 실제 공통 트리 없음")).Tree;
        var vertical = tree.Node(_hudVertical) ?? throw new InvalidOperationException("채팅창의 세로 구분선 없음");
        var divider = tree.Node(_hudChatDivider) ?? throw new InvalidOperationException("채팅창의 가로 구분선 없음");
        uint token = NativeChatWindows.Create();
        var node = tree.Node(token) as HudChatNode ?? throw new InvalidOperationException("채팅창의 실제 부모 없음");
        float x = vertical.X + vertical.BaseWidth;
        float y = divider.Y + divider.BaseHeight;
        node.BaseWidth = _driver.Screen.Width - x;
        node.Width = node.BaseWidth * node.Scale;
        node.BaseHeight = _driver.Screen.Height - y;
        node.Height = node.BaseHeight * node.Scale;
        node.Flags |= 0x20000;
        (tree.TreeHooks.Attach ?? throw new InvalidOperationException("채팅창의 실제 부착 장치 없음"))(token, x, y, tree.Create.Entries[0], 0);
        var dock = _missionHudDock ?? throw new InvalidOperationException("채팅창의 실제 HUD 도킹 없음");
        // 원본 Root의 세로·채팅 구분선 slave를 등록하고 미션 시작의 채팅 숨김 사건을 적용한다.
        dock.RegisterSlave(_hudVertical, token, 4);
        dock.RegisterSlave(_hudChatDivider, token, 8);
        dock.Handle(0x02000088, 0, 0);
        _menuDirty = true;
    }
}
