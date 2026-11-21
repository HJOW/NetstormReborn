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
    /// <summary>실제 현재 Chat root의 장기 슬롯이며 나머지 창/공유 로그는 후속이다.</summary>
    private readonly ClientChatWindowGlobals _chatWindowGlobals = new();
    /// <summary>같은 실제 root 슬롯을 UI 공장/표시가 빌린다.</summary>
    public ClientChatWindowGlobals ChatWindowGlobals => _chatWindowGlobals;
    /// <summary>실제 UI/Identity 장치를 제공한 뒤만 생성되는 제출 부모다.</summary>
    private ClientChatSubmitRuntime? _nativeChatSubmit;
    /// <summary>실제 UI의 Identity/번역/선택 장치와 같은 Root 소유자로 제출 부모를 설치한다.</summary>
    public void BindNativeChatSubmit(ClientChatSubmitDevices devices)
    {
        var startup = _startup ?? throw new InvalidOperationException("Chat 제출의 실제 시작 설정 미설치");
        _nativeChatSubmit = new(_chatManagerGlobals, _dotCommandGlobals, _subscriberGlobals, _gameplay, _chatGlobals, _chatWindowGlobals, startup.Configuration, devices);
    }
    /// <summary>명시 manager/가변 원문 BYTE 입력을 설치한 실제 제출 부모로 전달한다.</summary>
    public void SubmitNativeChat(uint manager, ReadOnlySpan<byte> input) => (_nativeChatSubmit ?? throw new InvalidOperationException("실제 Chat 제출 장치 미설치")).Submit(manager, input);
    /// <summary>실제 현재 Player/표면/Chat과 동기 이름 getter를 사용하는 대상 조회 부모다.</summary>
    private ClientChatIdentityRuntime? _nativeChatIdentity;
    /// <summary>대상 검사 binding에서 받은 같은 현재 표시 BYTE 번역이다.</summary>
    private Func<string, string>? _nativeChatRecipientTranslate;
    /// <summary>현재 이름/번역과 실제 Root의 같은 Identity 저장소를 연결한다. CS 장치는 메타 분기에서만 필요하다.</summary>
    public void BindNativeChatRecipients(Func<string> currentName, Func<string, string> translate, Func<byte[]>? metaRecords = null)
    {
        ArgumentNullException.ThrowIfNull(translate);
        var identity = new ClientChatIdentityRuntime(_gameplay, _surfaceGlobals, _subscriberGlobals, _chatGlobals, currentName, metaRecords);
        _nativeChatIdentity = identity; _nativeChatRecipientTranslate = translate;
    }
    /// <summary>설치한 같은 현재 Identity에60BYTE 지역을 전달한다.</summary>
    public bool ResolveNativeChatIdentity(byte[] record) => (_nativeChatIdentity ?? throw new InvalidOperationException("실제 Chat Identity 미설치")).Resolve(record);
    /// <summary>현재 manager와 실제 대상/번역을 원문 표시 전용 검사에 연결한다.</summary>
    public bool CheckNativeChatRecipients(string? text)
    {
        var identity = _nativeChatIdentity ?? throw new InvalidOperationException("실제 Chat 대상 조회 미설치");
        var translate = _nativeChatRecipientTranslate ?? throw new InvalidOperationException("실제 Chat 대상 번역 미설치");
        return Netstorm.Client.Chat.ChatRecipients.CheckChatRecipients(_chatManagerGlobals.Current.Value, text, GC.AllocateUninitializedArray<byte>(60), new() { Resolve = identity.Resolve, Translate = translate });
    }
    /// <summary>실제 Root 대상 조회를 먼저 설치하고 같은 장치/Identity로 제출 부모를 연결한다.</summary>
    public void BindNativeChatSubmit(ClientChatSubmitDevices devices, Func<string> currentName, Func<string, string> translate, Func<byte[]>? metaRecords = null)
    {
        ArgumentNullException.ThrowIfNull(devices); BindNativeChatRecipients(currentName, translate, metaRecords);
        BindNativeChatSubmit(new ClientChatSubmitDevices
        {
            Resolve = ResolveNativeChatIdentity, Translate = translate, Assertion = devices.Assertion, SetInputVisible = devices.SetInputVisible,
            Flash = devices.Flash, Sound = devices.Sound, MenuCommand = devices.MenuCommand, Message = devices.Message, PrintChat = devices.PrintChat, DiagnosticLog = devices.DiagnosticLog,
        });
    }
    /// <summary>같은 Root의 일반 메시지 부모이며 UI 설치 이후만 사용한다.</summary>
    private ClientChatMessageRuntime? _nativeChatMessage;
    /// <summary>현재 이름/대상·일반 조합/명시 패킷 장치·제출을 같은 Root 소유자로 한 번에 연결한다.</summary>
    public void BindNativeChatFlow(ClientChatSubmitDevices devices, Func<string> currentName, Func<string, string> translate, ChatPacketHooks packet, Func<byte[]>? metaRecords = null)
    {
        ArgumentNullException.ThrowIfNull(devices); ArgumentNullException.ThrowIfNull(packet); BindNativeChatRecipients(currentName, translate, metaRecords);
        var message = new ClientChatMessageRuntime(_chatManagerGlobals, _subscriberGlobals, _nativeChatIdentity!, currentName, packet, _chatWindowGlobals, devices.SetInputVisible);
        BindNativeChatSubmit(new ClientChatSubmitDevices
        {
            Resolve = ResolveNativeChatIdentity, Translate = translate, Assertion = devices.Assertion, SetInputVisible = devices.SetInputVisible,
            Flash = devices.Flash, Sound = devices.Sound, MenuCommand = devices.MenuCommand, Message = message.Say, PrintChat = devices.PrintChat, DiagnosticLog = devices.DiagnosticLog,
        });
        _nativeChatMessage = message;
    }
    /// <summary>설치된 같은 일반 say/패킷 부모를 UI가 직접 부를 수 있다.</summary>
    public uint SayNativeChatMessage(ReadOnlySpan<byte> input) => (_nativeChatMessage ?? throw new InvalidOperationException("실제 일반 Chat 메시지 미설치")).Say(input);
    /// <summary>World/Init/전송 소비자가 빌릴 실제 Client 구독자 주소다.</summary>
    public ClientSubscriberGlobals SubscriberGlobals => _subscriberGlobals;
    /// <summary>미션 유무와 관계없이 실제 MySubscriber의 현재 전체 DWORD를 읽는다.</summary>
    public uint CurrentSubscriber => _subscriberGlobals.MySubscriber.Value;
    /// <summary>같은4800BYTE 처리기이며 Root State 정리가 직접 호출한다.</summary>
    public RawChatChannels ChatChannels => _chatGlobals.Channels;
    /// <summary>같은100×48BYTE 기록이다. 조회나 Session 반납에서 초기화하지 않는다.</summary>
    public byte[] ChatRecords => _chatGlobals.Records;
}
