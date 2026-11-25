using System.Text;
using Netstorm.Client.Chat;

// 실제 Chat 점 명령 분배 효과를 같은 Root 장치에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>UberGump의 실제 Chat 제출 부모를 미션 진입 경로가 아닌 공통 메뉴 경로에 설치한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은 장기 0.0.0.0 수신 guard다. 원문 공유 zero 배열과 같이 송신보다 먼저 무장한다.</summary>
    private readonly byte[] _chatZeroAddress = new byte[20];
    /// <summary>같은 실제 메뉴 트리/음향/채팅 창에 점 명령 분배 효과를 한 번 연결한다. 전송 backend는 R11에서 잇는다.</summary>
    /// <remarks>2026-11-25 0.995에서 RU-5용으로 추가했다. 근거: cpppj ClientChatRuntime.cpp 174~212행의 BindClientChatSubmit·BindClientChatMessage. 치트 상자·설정·입력줄·메뉴 명령·일반 say·오류 출력·진단은 실제 owner에 닿고 번쩍임과 일반 송신은 후속 단계에서 잇는다.</remarks>
    private void InitializeNativeChatSubmit()
    {
        if (_nativeChatSubmit is not null) throw new InvalidOperationException("Chat 제출 부모 중복 설치");
        var windows = _nativeChatWindows ?? throw new InvalidOperationException("Chat 제출의 실제 채팅 창 없음");
        var identity = _nativeChatIdentity ?? throw new InvalidOperationException("Chat 제출의 실제 Identity 없음");
        ChatPacketHooks packet = _chatManagerGlobals.MakePacketHooks();
        packet.CurrentName = () => Encoding.Latin1.GetBytes(CurrentNativeChatName() + '\0');
        packet.Read = address => address switch
        {
            RawChatPacket.SubscriberAddress => _subscriberGlobals.MySubscriber.Value,
            RawChatPacket.BattleAddress => _surfaceGlobals.Storage.OwnerMode.Battle ? 1u : 0,
            RawChatPacket.ChannelOneAddress => _surfaceGlobals.Storage.OwnerMode.Fort ? 1u : 0,
            RawChatPacket.MetamapAddress => _surfaceGlobals.Storage.OwnerMode.Challenge ? 1u : 0,
            0x544c5cu => 0u,
            0x52edf6u => 0x41u,
            _ => throw new ArgumentOutOfRangeException(nameof(address), "Chat 제출의 실제 패킷 전역 미연결"),
        };
        packet.Send = (channel, body) => throw new InvalidOperationException("Chat 일반 송신은 R11 수송에서 잇는다");
        packet.Log = (level, text) => ErrorLog.Note("Chat 종류" + level.ToString(System.Globalization.CultureInfo.InvariantCulture) + ": " + text);
        packet.ZeroAddress = () => { Array.Clear(_chatZeroAddress); _chatZeroAddress[0] = 2; };
        packet.LocalReceive = body => ReceiveNativePacketChat(body);
        _nativeChatMessage = new ClientChatMessageRuntime(_chatManagerGlobals, _subscriberGlobals, identity, CurrentNativeChatName, packet,
            _chatWindowGlobals, (current, visible) => NativeChatGeometry.SetInputVisible(current, visible));
        BindNativeChatSubmit(new ClientChatSubmitDevices
        {
            Resolve = ResolveNativeChatIdentity,
            Translate = TranslateNativeChatText,
            Assertion = line => ErrorLog.RecordAssertion("Chat 원문 진단", ".\\Chat.cpp", line),
            SetInputVisible = (current, visible) => NativeChatGeometry.SetInputVisible(current, visible),
            Flash = brightness =>
            {
                if (_gameplay.Player.PlayerKnown == 0) return;
                throw new InvalidOperationException("Chat 번쩍임 표시는 R6 월드 표시 연결에서 잇는다");
            },
            Sound = name => (_missionAudio ?? throw new InvalidOperationException("Chat 명령의 실제 장기 음향 없음")).PlayGlobal(name),
            MenuCommand = windows.InvokeCommand,
            Message = _nativeChatMessage.Say,
            PrintChat = windows.Print,
            DiagnosticLog = (level, text) => ErrorLog.Note("Chat 종류" + level.ToString(System.Globalization.CultureInfo.InvariantCulture) + ": " + text),
        });
    }
}
