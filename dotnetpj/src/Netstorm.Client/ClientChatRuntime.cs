using System.Text;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

// 같은 실제 명령표/할당/Config/Subscriber로 공통 Chat 시작을 실행한다.
namespace Netstorm.Client;

/// <summary>ClientChatRuntime.cpp의 Issue/Chat 초기화 부모를 실제 장기 소유자에 연결한다.</summary>
public static class ClientChatRuntime
{
    /// <summary>원문 좁은 명령 이름에 첫NUL을 붙여 같은 실제 표에 등록한다.</summary>
    private static void Register(ClientDotCommandGlobals commands, string name, uint callback, uint flag) => commands.Register(Encoding.Latin1.GetBytes(name + '\0'), callback, flag);
    /// <summary>map mode의 Chat 세 등록보다 앞서 원문 Issue 네 등록을 실행한다.</summary>
    public static void InitializeClientIssueCommands(ClientDotCommandGlobals commands) => RawIssueCommands.RegisterIssueCommands((name, callback, flag) => Register(commands, name, callback, flag));
    /// <summary>원문 ReadString의 빈 지역/현재 구독자 키와 실제 UTF8 표시바이트 경계를 사용한다.</summary>
    public static void InitializeClientChatManager(ClientChatManagerGlobals manager, ClientDotCommandGlobals commands, ClientSubscriberGlobals subscribers, UIntBox coreActive, ConfigInterface configuration, Action<uint> assertion)
    {
        var hooks = manager.MakePrefixHooks(); hooks.Assertion = assertion;
        hooks.RegisterCommand = (name, callback, flag) => Register(commands, name, callback, flag);
        hooks.ReadConfig = key => { string value = ""; configuration.ReadString(key, ref value); return Encoding.Latin1.GetString(ExtendedText.EncodeDisplayText(value)); };
        RawChatManagerPrefix.InitializeChatManager(new(manager.Current, subscribers.MySubscriber, coreActive), hooks);
    }
}
