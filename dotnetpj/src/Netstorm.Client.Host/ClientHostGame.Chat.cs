using Netstorm.Client.Chat;

// 실제 Client가 메뉴/미션/State 사이에서 동일한 구독자와 채팅 기록을 보관한다.
namespace Netstorm.Client.Host;

/// <summary>ClientMain.cpp의 현재 Subscriber/ChatState 접근을 실제 호스트 수명에 연결한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>00540c74/0054265c와 현재 Handle64BYTE를 최초 한 번 구성한다.</summary>
    private readonly ClientSubscriberGlobals _subscriberGlobals = new();
    /// <summary>현재 구독자 getter와 Chat4800BYTE를 소유하며 Session 교체로 다시 만들지 않는다.</summary>
    private readonly ClientChatGlobals _chatGlobals;
    /// <summary>World/Init/전송 소비자가 빌릴 실제 Client 구독자 주소다.</summary>
    public ClientSubscriberGlobals SubscriberGlobals => _subscriberGlobals;
    /// <summary>미션 유무와 관계없이 실제 MySubscriber의 현재 전체 DWORD를 읽는다.</summary>
    public uint CurrentSubscriber => _subscriberGlobals.MySubscriber.Value;
    /// <summary>같은4800BYTE 처리기이며 Root State 정리가 직접 호출한다.</summary>
    public RawChatChannels ChatChannels => _chatGlobals.Channels;
    /// <summary>같은100×48BYTE 기록이다. 조회나 Session 반납에서 초기화하지 않는다.</summary>
    public byte[] ChatRecords => _chatGlobals.Records;
}
