using Netstorm.Client.Chat;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

// 현재 Client/실제 UI와 같은 설정·관계·로그를 수신 본체에 빌려준다.
namespace Netstorm.Client.Host;

/// <summary>실제 Root가 소유하는 음향/뉴스/설정 저장/진단 효과다. 선택한 분기에서만 요구한다.</summary>
public sealed class HostChatReceiveDevices
{
    /// <summary>현재 장기 WorldAudio의 이름별 전역 소리를 재생한다.</summary>
    public Action<string>? Sound { get; init; }
    /// <summary>뉴스 시각을 기록한 뒤 같은 서버별 설정 저장 부모를 실행한다.</summary>
    public Action? SaveNewsTime { get; init; }
    /// <summary>현재 실제 지도 영역을 사용하는 HTML 뉴스 창을 연다.</summary>
    public Action<string, uint, uint>? News { get; init; }
    /// <summary>같은 Client의 원문 수준별 진단을 쓴다.</summary>
    public Action<uint, string>? Log { get; init; }
}

/// <summary>cpppj/src/client/UberHud.inc의 ReceiveChatMessage·ReadChatGlobal 실제 소비자다.</summary>
public sealed class HostChatReceive
{
    /// <summary>장기 소유자만 캡처하고 값/배열은 사용 위치에서 읽는 수신 장치다.</summary>
    private readonly ChatReceiveHooks _hooks;
    /// <summary>미선택 효과를 선행 요구하지 않으며 도달한 실제 장치 누락은 실패로 드러낸다.</summary>
    private static T Need<T>(T? device) where T : Delegate => device ?? throw new InvalidOperationException("Chat 수신의 실제 Root 장치 미연결");

    /// <summary>같은 현재 Game/Surface/구독자와 실제 창/트리/팔레트/설정을 연결한다. 생성 때 값을 읽거나 창을 만들지 않는다.</summary>
    public HostChatReceive(ClientGameplayGlobals game, ClientSurfaceGlobals surface, ClientSubscriberGlobals subscribers,
        ClientChatWindowGlobals globals, ConfigInterface config, Func<uint, uint> senderOwner, GumpButtonTree tree,
        Screen screen, HostChatWindows windows, Func<Memory<byte>> hiddenPlayers, HostChatReceiveDevices devices)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(subscribers);
        ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(config); ArgumentNullException.ThrowIfNull(senderOwner);
        ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(screen); ArgumentNullException.ThrowIfNull(windows);
        ArgumentNullException.ThrowIfNull(hiddenPlayers); ArgumentNullException.ThrowIfNull(devices);
        if (!ReferenceEquals(surface.Gameplay, game) || !ReferenceEquals(game.Subscribers, subscribers)) throw new ArgumentException("Chat 수신의 같은 실제 Client 소유자 필요");
        // 고정 주소는 원문 슬롯의 현재 저장소를 읽으며 짧은 Session 배열을 보관하지 않는다.
        uint Read(uint address)
        {
            if (address >= 0x595200 && address < 0x595200 + 81 * 4 && (address & 3) == 0) return game.PlayerRecords.Alliances[(address - 0x595200) / 4];
            if (address >= 0x52f81e && address < 0x52f827) return hiddenPlayers().Span[(int)(address - 0x52f81e)];
            return address switch
            {
                0x540c74 => subscribers.MySubscriber.Value, 0x540c70 => game.Player.BattleOwner,
                0x594fb8 => surface.Storage.OwnerMode.Fort ? 1u : 0, 0x594fbc => surface.Storage.OwnerMode.Battle ? 1u : 0, 0x594fc8 => surface.Storage.OwnerMode.Challenge ? 1u : 0,
                0x548740 => globals.Current.Value, 0x548710 => globals.ReceivedMask.Value, 0x548748 => globals.NewsTime.Value,
                0x5ca92c => tree.ChatAlertId.Value, 0x5b5dd8 => screen.Colors().Named[33].Index,
                _ => throw new ArgumentOutOfRangeException(nameof(address), "Chat 수신 실제 전역 주소 범위"),
            };
        }
        // ReadInt의 동일0 반환과 실제 누락을 문자열 조회로 구별한다. 빈 설정도 원문처럼 실패한다.
        int Option(string key)
        {
            int value = 0;
            if (!config.ReadInt(key, ref value)) { string text = ""; if (!config.ReadString(key, ref text)) throw new InvalidOperationException("Chat 수신 실제 옵션 누락:" + key); }
            return value;
        }
        _hooks = new()
        {
            Read = Read, Ignored = globals.Ignored.Contains, Config = Option, Owner = senderOwner,
            Wake = id => tree.PulseRegisteredIfIdle(id), Sound = name => Need(devices.Sound)(name),
            Append = (chat, text, context) => windows.Window.AppendLog(chat, text, context), Show = windows.Show,
            SetNewsTime = stamp => { globals.NewsTime.Value = stamp; Need(devices.SaveNewsTime)(); },
            News = (text, width, height) => Need(devices.News)(text, width, height), Log = (level, text) => Need(devices.Log)(level, text),
        };
    }
    /// <summary>원본 패킷의 같은 부분 물리를 실제 수신 본체에 빌린다. 현재 창0은 빈 버퍼도 건드리지 않는다.</summary>
    public void Receive(Memory<byte> packet) => ChatReceiveBody.DispatchChatPacket(packet.Span, _hooks);
}
