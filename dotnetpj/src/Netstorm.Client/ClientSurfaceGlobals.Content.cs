using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client; // 같은 장기 Client의 내용물 수명 장치를 연결한다.

/// <summary>현재 커서·Player와 실제 입력/전송/음향을 내용물 수명에 제공한다.</summary>
public sealed partial class ClientSurfaceGlobals
{
    /// <summary>첫 내용물 본체 전에 같은 주소와 선택 효과를 연결하며 게임 상태는 바꾸지 않는다.</summary>
    public void BindNativeContentDevices(ClientConnectionGlobals connections, InputQueue input, Action cancelCursor, Action<float, float> sound)
    {
        ArgumentNullException.ThrowIfNull(connections); ArgumentNullException.ThrowIfNull(input);
        ArgumentNullException.ThrowIfNull(cancelCursor); ArgumentNullException.ThrowIfNull(sound);
        if (!ReferenceEquals(connections.LocalState.Gameplay, Gameplay)) throw new ArgumentException("내용물의 같은 Client 연결 필요", nameof(connections));
        Storage.BindContentDevices(new RawSurfaceContentDevices
        {
            Cursor = Gameplay.Cursor.PreviewBox,
            LocalOwner = Gameplay.Player.BattleOwnerBox,
            ContentType = Gameplay.ContentFormTypeBox,
            Effects = new ContentLifetimeHooks
            {
                CancelCursor = cancelCursor,
                LocalNotice = code => input.PushCommand(code, 0, 0),
                RemoteNotice = owner => ClientContentIncomeSender.Send(Gameplay, this, connections, owner),
                Sound = sound,
            },
        });
    }
}
