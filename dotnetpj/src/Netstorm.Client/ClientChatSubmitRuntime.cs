using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

// 실제 장기 Chat 소유자를 원문 제출→점 명령 분배 부모에 연결한다.
namespace Netstorm.Client;

/// <summary>실제 일반 조합/송신에 NUL 포함 읽기 본문을 넘긴다.</summary>
public delegate uint ClientChatMessageHandler(ReadOnlySpan<byte> input);
/// <summary>실제 창/번역/로그/메뉴/음향 경계다. 미선택 장치는 요구하지 않는다.</summary>
public sealed class ClientChatSubmitDevices
{
    /// <summary>현재 Player/Chat 식별자 조회이며 binding 시 반드시 필요하다.</summary>
    public Func<byte[], bool>? Resolve { get; init; }
    /// <summary>현재 언어 표시 BYTE 문자열이며 binding 시 반드시 필요하다.</summary>
    public Func<string, string>? Translate { get; init; }
    /// <summary>원문 Chat 진단274/157/184 등의 실제 공통 장치다.</summary>
    public Action<uint>? Assertion { get; init; }
    /// <summary>현재 비영 root의 실제 입력줄 표시를 변경한다.</summary>
    public Action<uint, uint>? SetInputVisible { get; init; }
    /// <summary>실제 Client 번쩍임 요청이다.</summary>
    public Action<int>? Flash { get; init; }
    /// <summary>번쩍임 뒤 실제 원문 소리를 재생한다.</summary>
    public Action<string>? Sound { get; init; }
    /// <summary>실제 clear/quit 부모를 callback 식별자로 호출한다.</summary>
    public Func<uint, uint>? MenuCommand { get; init; }
    /// <summary>일반 say의 실제 조합/송신이다. binding 시 현재 장치를 캡처한다.</summary>
    public ClientChatMessageHandler? Message { get; init; }
    /// <summary>실제 Chat/스크롤 출력이며 비영 명령 코드에서만 요구한다.</summary>
    public Action<string>? PrintChat { get; init; }
    /// <summary>현재 종류별 원문 진단 로그를 기록하며 형식 실패에서29를 받는다.</summary>
    public Action<uint, string>? DiagnosticLog { get; init; }
}
/// <summary>ClientChatRuntime.cpp BindClientChatSubmit의 같은 현재 소유자/선택 장치 부모다.</summary>
public sealed class ClientChatSubmitRuntime
{
    /// <summary>실제 manager/수신자 할당과 현재 토큰의 장기 소유자다.</summary>
    private readonly ClientChatManagerGlobals _manager;
    /// <summary>같은 실제 인접 명령표와 장기 검색 상태다.</summary>
    private readonly ClientDotCommandGlobals _commands;
    /// <summary>현재 구독자 DWORD를 조회하는 실제 소유자다.</summary>
    private readonly ClientSubscriberGlobals _subscribers;
    /// <summary>005cab30을 같은 에너지/명령 소비자와 공유하는 장기 게임 전역이다.</summary>
    private readonly ClientGameplayGlobals _game;
    /// <summary>같은4800BYTE 현재 기록이다.</summary>
    private readonly ClientChatGlobals _chat;
    /// <summary>현재 root0의 자연 관문을 제공하는 실제 장기 슬롯이다.</summary>
    private readonly ClientChatWindowGlobals _window;
    /// <summary>실제 UTF8 설정/변경 알림 소유자다.</summary>
    private readonly ConfigInterface _configuration;
    /// <summary>binding 시의 실제 선택 장치 묶음이다.</summary>
    private readonly ClientChatSubmitDevices _devices;
    /// <summary>원문 binding 위치에서 캡처한 일반 메시지 장치다.</summary>
    private readonly ClientChatMessageHandler? _message;
    /// <summary>원문과 같이 필수 Identity/번역만 먼저 검증하고 나머지는 선택 위치까지 보존한다.</summary>
    public ClientChatSubmitRuntime(ClientChatManagerGlobals manager, ClientDotCommandGlobals commands, ClientSubscriberGlobals subscribers,
        ClientGameplayGlobals game, ClientChatGlobals chat, ClientChatWindowGlobals window, ConfigInterface configuration, ClientChatSubmitDevices devices)
    {
        _manager = manager ?? throw new ArgumentNullException(nameof(manager)); _commands = commands ?? throw new ArgumentNullException(nameof(commands));
        _subscribers = subscribers ?? throw new ArgumentNullException(nameof(subscribers)); _game = game ?? throw new ArgumentNullException(nameof(game));
        _chat = chat ?? throw new ArgumentNullException(nameof(chat)); _window = window ?? throw new ArgumentNullException(nameof(window));
        _configuration = configuration ?? throw new ArgumentNullException(nameof(configuration)); _devices = devices ?? throw new ArgumentNullException(nameof(devices));
        if (!ReferenceEquals(game.Subscribers, subscribers)) throw new ArgumentException("Chat 제출의 실제 같은 Subscriber/Game 필요", nameof(game));
        if (devices.Resolve is null || devices.Translate is null) throw new ArgumentException("Chat 제출의 실제 Identity/번역 장치 필요", nameof(devices));
        _message = devices.Message;
    }
    /// <summary>선택된 실제 부모 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Chat 제출 부모의 실제 선택 장치 미연결");
    /// <summary>현재 root0이면 장치를 조회하지 않고 비영 root에서만 실제 입력줄을 바꾼다.</summary>
    private void InputVisible(uint visible)
    {
        uint current = _window.Current.Value; if (current == 0) return; Need(_devices.SetInputVisible)(current, visible);
    }
    /// <summary>같은 NUL 포함 인자와 실제 현재 전역으로 Issue/메뉴/형식/say를 원문 순서로 선택한다.</summary>
    private uint Invoke(uint callback, Span<byte> argument)
    {
        int end = argument.IndexOf((byte)0); if (end < 0) throw new ArgumentOutOfRangeException(nameof(argument), "Chat 명령 인자의 실제 NUL 없음");
        string text = Encoding.Latin1.GetString(argument[..end]);
        uint? issue = RawIssueCommands.InvokeIssueCommand(callback, text, _game.UserCheat, new() { Flash = _devices.Flash, Sound = _devices.Sound });
        if (issue.HasValue) return issue.Value;
        if (callback is 0x42f680 or 0x42e520) return Need(_devices.MenuCommand)(callback);
        if (callback is 0x42ddf0 or 0x42def0) return ClientChatRuntime.ApplyClientChatFormat(_manager, _subscribers, _configuration, callback == 0x42def0, text, _devices.Assertion, _devices.DiagnosticLog);
        if (callback == 0x42f1e0)
        {
            if (_manager.Current.Value == 0) return 0;
            if (argument.IsEmpty) throw new ArgumentOutOfRangeException(nameof(argument), "Chat say의 실제 인자 물리 없음");
            if (argument[0] == 0) { InputVisible(0); return 0; }
            if (argument[0] == ' ') return 0;
            return Need(_message)(argument);
        }
        throw new InvalidOperationException("Chat 제출의 실제 명령 callback 미연결:" + callback);
    }
    /// <summary>비영 코드의 이미 서식화한 오류만 같은 실제 Chat/스크롤에 출력한다.</summary>
    private void Error(ReadOnlySpan<byte> name, uint code) => Need(_devices.PrintChat)("Error in command \"." + Encoding.Latin1.GetString(name) + "\": code == " + code.ToString("x", System.Globalization.CultureInfo.InvariantCulture) + "\n");
    /// <summary>원문 지역60/4097BYTE를 값 초기화 요청 없이 만들고 같은 실제 본문 분배에 연결한다.</summary>
    public void Submit(uint manager, ReadOnlySpan<byte> input)
    {
        var hooks = _manager.MakeSubmitHooks(); hooks.Assertion = _devices.Assertion; hooks.Resolve = _devices.Resolve; hooks.Translate = _devices.Translate;
        hooks.Records = () => _chat.Records; hooks.InputVisible = InputVisible;
        hooks.Dispatch = body => _commands.Dispatch(body, new() { Invoke = Invoke, Error = Error });
        byte[] identity = GC.AllocateUninitializedArray<byte>(60), work = GC.AllocateUninitializedArray<byte>(RawChatSubmit.WorkBytes);
        RawChatSubmit.SubmitChatInput(manager, input, identity, work, hooks);
    }
}
