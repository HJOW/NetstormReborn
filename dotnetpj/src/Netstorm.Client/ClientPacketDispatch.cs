using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client;

// 장기 기본 표·41h·로그인·설정·버전·기술·변수·Native gameplay 묶음을 같은 분배기에 설치한다. 전송 설치는 R4-6, 실제 송수신 펌프는 R11에서 잇는다.
/// <summary>공통 패킷 분배기의 실제 소유자 조립이다. 장기 기본 표·41h·로그인·설정·버전·기술·변수·Native gameplay 묶음을 같은 분배기에 설치한다.</summary>
public static class ClientPacketDispatch
{
    /// <summary>같은 장기 Client 장치만 읽으며 실제 수신 UI/진단/활동 장치를 묶는다.</summary>
    /// <param name="dispatch">공통 분배기의 실제 시계/기록/종료 장치다.</param>
    /// <param name="game">같은 실제 Player/권한/현재 세션 소유자다.</param>
    /// <param name="connections">같은 실제 연결 Registry/Source 장부다.</param>
    /// <param name="surface">같은 실제 표면/시계/커널이다.</param>
    /// <param name="chat">창이 없어도 존재하는 실제 마스크/현재 UI 슬롯이다.</param>
    /// <param name="receive">실제 Chat UI 수신 입구다. 41h가 도착하면 호출된다.</param>
    /// <param name="assertion">원문 진단의 실제 선택 줄 소비자다.</param>
    /// <param name="activity">장기 연결 활성의 실제 벽시계/프레임/서버 조회다.</param>
    /// <returns>장기 기본 표·41h·로그인·설정·버전·기술·변수·Native gameplay 묶음이 설치된 실제 분배기다.</returns>
    /// <remarks>2026-11-23 0.897에서 R4-6 전송·수신 묶음용으로 추가하고 0.898에서 설정·버전·기술·변수 묶음을, 0.899에서 Native gameplay 6종 묶음을, 0.934에서 장기 기본 18행을 이었다. 근거: R4-6 남음의 "공통 41h가 Root.ReceiveNativePacketChat을 부르는 전송 설치"와 "NativeLoginPacketReceivers의 실제 MonoGame Root 설치 호출"과 InstallClientPacketDefaults의 기본 18행이다. 호스트는 10.78만 다루므로 판본을 고정한다. 종료/닫기/반납은 R11 수송에서 잇는다.</remarks>
    public static RawPacketDispatch Build(
        PacketDispatchHooks dispatch,
        ClientGameplayGlobals game,
        ClientConnectionGlobals connections,
        ClientSurfaceGlobals surface,
        ClientChatWindowGlobals chat,
        Action<Memory<byte>> receive,
        Action<string, string, uint> assertion,
        ConnectionActivityHooks activity)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(connections);
        ArgumentNullException.ThrowIfNull(surface);
        ArgumentNullException.ThrowIfNull(chat);
        ArgumentNullException.ThrowIfNull(receive);
        ArgumentNullException.ThrowIfNull(assertion);
        ArgumentNullException.ThrowIfNull(activity);
        var packet = new RawPacketDispatch(OriginalEdition.Patch1078, dispatch);
        ClientPacketDefaultObservers.Install18(packet, surface.Storage);
        ClientChatPacketReceiver.Install(game, surface, connections, chat, packet, receive);
        NativeLoginPacketReceivers.Install(game, connections, packet, assertion, activity);
        ClientMissionPacketDefaults.InstallMissionSendObservers(packet, surface.Storage);
        ClientMissionPacketDefaults.InstallSettings(packet, game);
        ClientMissionPacketDefaults.InstallVersionReply(packet, game, connections, surface);
        ClientTechnologyPacketDefaults.Install(packet, game, surface.Storage);
        ClientVariablePacketObservers.Install(packet, surface.Storage);
        NativeGameplayPacketReceivers.InstallWindAviary(game, packet);
        NativeGameplayPacketReceivers.InstallSunAviary(game, packet);
        NativeGameplayPacketReceivers.InstallRainAviary(game, packet);
        NativeGameplayPacketReceivers.InstallGun(game, packet);
        NativeGameplayPacketReceivers.InstallFlyer(game, packet);
        NativeGameplayPacketReceivers.InstallArcherAnimation(game, packet);
        return packet;
    }
}
