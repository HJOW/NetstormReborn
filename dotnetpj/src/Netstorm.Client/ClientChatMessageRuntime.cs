using Netstorm.Client.Chat;

// 실제 같은 Chat manager/Subscriber/Identity를 일반 조합과 패킷에 연결한다.
namespace Netstorm.Client;

/// <summary>ClientChatRuntime.cpp의 일반 본문 getter와 BindClientChatMessage 부모다.</summary>
public sealed class ClientChatMessageRuntime
{
    /// <summary>현재 manager와 Subscriber를 복사하지 않는 원문 두 주소다.</summary>
    private readonly ChatMessageGlobals _globals;
    /// <summary>binding 때 구성한 같은 실제 이름/메모리/패킷 장치다.</summary>
    private readonly ChatMessageHooks _message;
    /// <summary>이름/표면의 실제 Identity는 준비된 같은 facade를 사용하고 선택 장치는 선행 실행하지 않는다.</summary>
    public ClientChatMessageRuntime(ClientChatManagerGlobals manager, ClientSubscriberGlobals subscribers, ClientChatIdentityRuntime identity,
        Func<string> currentName, ChatPacketHooks packet, ClientChatWindowGlobals? window = null, Action<uint, uint>? inputVisible = null)
    {
        ArgumentNullException.ThrowIfNull(manager); ArgumentNullException.ThrowIfNull(subscribers); ArgumentNullException.ThrowIfNull(identity); ArgumentNullException.ThrowIfNull(currentName); ArgumentNullException.ThrowIfNull(packet);
        _globals = new(manager.Current, subscribers.MySubscriber);
        var captured = new ChatPacketHooks
        {
            Memory = packet.Memory, CurrentName = packet.CurrentName, Read = packet.Read, AddRecipient = packet.AddRecipient,
            Send = packet.Send, Log = packet.Log, ZeroAddress = packet.ZeroAddress, LocalReceive = packet.LocalReceive,
        };
        _message = new()
        {
            Memory = manager.Bytes, CurrentName = currentName, NameForSubscriber = (output, subscriber) => identity.CopyName(subscriber, output),
            Deliver = (output, recipients, body) => ChatPacketBody.SendChatPacket(output, recipients, body, captured),
            InputVisible = visible =>
            {
                uint current = (window ?? throw new InvalidOperationException("일반 Chat의 실제 현재 창 슬롯 미연결")).Current.Value;
                if (current != 0) (inputVisible ?? throw new InvalidOperationException("일반 Chat의 현재 입력줄 장치 미연결"))(current, visible);
            },
        };
    }
    /// <summary>원문256BYTE 지역의 미작성 꼬리를 초기화하지 않고 같은 say 부모를 실행한다.</summary>
    public uint Say(ReadOnlySpan<byte> input) => ChatMessageBody.SayChatMessage(input, GC.AllocateUninitializedArray<byte>(RawChatMessage.NameBytes), _globals, _message);
}
