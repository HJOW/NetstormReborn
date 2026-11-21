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
    /// <summary>현재 manager와 미초기화 할당의 실제 장기 소유자다. 공통 시작 초기화는 별도 호출자다.</summary>
    private readonly ClientChatManagerGlobals _chatManagerGlobals = new();
    /// <summary>같은 manager current/할당을 빌리며 조회 자체는 초기화하지 않는다.</summary>
    public ClientChatManagerGlobals ChatManagerGlobals => _chatManagerGlobals;
    /// <summary>원문 say/count1 정적표를 같은 Client 수명에 한 번 구성한다.</summary>
    private readonly ClientDotCommandGlobals _dotCommandGlobals = new();
    /// <summary>같은 실제 이름/함수/표시/count와 분배 검색 상태다.</summary>
    public ClientDotCommandGlobals DotCommandGlobals => _dotCommandGlobals;
    /// <summary>콘텐츠 재연결에서 성공한 Issue/Chat 초기화를 반복하지 않는 수명 표식이다.</summary>
    private bool _nativeChatInitialized;
    /// <summary>첫 메뉴 전에 원문 Issue 네 등록→같은 실제 Chat manager 초기화를 실행한다.</summary>
    private void InitializeNativeChat()
    {
        if (_nativeChatInitialized) return;
        var startup = _startup ?? throw new InvalidOperationException("Chat 공통 시작의 실제 설정 미설치");
        ClientChatRuntime.InitializeClientIssueCommands(_dotCommandGlobals);
        ClientChatRuntime.InitializeClientChatManager(_chatManagerGlobals, _dotCommandGlobals, _subscriberGlobals, _gameplay.CoreData.ActiveBox, startup.Configuration,
            line => ErrorLog.RecordAssertion("Chat 원문 진단", ".\\Chat.cpp", line));
        _nativeChatInitialized = true;
    }
    /// <summary>World/Init/전송 소비자가 빌릴 실제 Client 구독자 주소다.</summary>
    public ClientSubscriberGlobals SubscriberGlobals => _subscriberGlobals;
    /// <summary>미션 유무와 관계없이 실제 MySubscriber의 현재 전체 DWORD를 읽는다.</summary>
    public uint CurrentSubscriber => _subscriberGlobals.MySubscriber.Value;
    /// <summary>같은4800BYTE 처리기이며 Root State 정리가 직접 호출한다.</summary>
    public RawChatChannels ChatChannels => _chatGlobals.Channels;
    /// <summary>같은100×48BYTE 기록이다. 조회나 Session 반납에서 초기화하지 않는다.</summary>
    public byte[] ChatRecords => _chatGlobals.Records;
}
