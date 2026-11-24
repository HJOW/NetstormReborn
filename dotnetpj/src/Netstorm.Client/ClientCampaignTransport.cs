using Netstorm.Client.Player;
using Netstorm.Core.Display;
using Netstorm.Net;

// 같은 Client/Session의 물리를 검증하고 소켓을 열지 않는 실제 coordinator를 만든다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 MakeClientCampaignTransport(004b... 전체)를 이식한다.
// 0.967에서 R4-6 전송·수신(수송 생성·재결합·Publish) 묶음용으로 추가됐다.
namespace Netstorm.Client;

// Player/서버 권한/시계의 실제 소유권만 묶고 현재 World는 약하게 참조한다.
public sealed record ClientTransportGlobals
{
    /// <summary>같은 실제 Player9 기록 owner다.</summary>
    public ClientPlayerRecords Players { get; init; }

    /// <summary>같은 실제 Player/권한/현재 세션 owner다.</summary>
    public ClientGameplayGlobals Game { get; init; }

    /// <summary>원점/정지 상태를 복제하지 않는 실제 Client 시계다.</summary>
    public OriginalGameClock Clock { get; init; }

    /// <summary>같은 세 소유권을 함께 보유한다. 명목 토큰으로 대신하지 않는다. 0.967에서 추가됐다.</summary>
    public ClientTransportGlobals(ClientPlayerRecords players, ClientGameplayGlobals game, OriginalGameClock clock)
    {
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(clock);
        Players = players;
        Game = game;
        Clock = clock;
    }
}

// 수송 생성·재결합·Publish의 실제 원문 순서를 같은 분배기에 잇는 부모다.
public static class ClientCampaignTransport
{
    /// <summary>같은 장기 표를 채우는 원문 순서로 전체 설치를 한 부모에서 호출한다. 0.967에서 추가됐다.</summary>
    /// <param name="game">같은 실제 Player/권한/현재 세션 소유자다.</param>
    /// <param name="connections">같은 실제 연결 Registry/Source 장부다.</param>
    /// <param name="surface">같은 실제 표면/시계/커널이다.</param>
    /// <param name="chat">창이 없어도 존재하는 실제 마스크/현재 UI 슬롯이다.</param>
    /// <param name="receive">실제 Chat UI 수신 입구다.</param>
    /// <param name="assertion">원문 진단의 실제 선택 줄 소비자다.</param>
    /// <param name="activity">장기 연결 활성의 실제 벽시계/프레임/서버 조회다.</param>
    /// <param name="challenge">같은 Client 수명으로 유지하는 실제 CS 슬롯·할당 owner다.</param>
    /// <param name="role">선택하는 실제 내부 분기다. InternalMission만 지원한다.</param>
    /// <param name="clock">원점/정지 상태를 복제하지 않는 실제 Client 시계다.</param>
    /// <param name="wallMilliseconds">현재 OS 밀리초다.</param>
    /// <param name="record">송수신 방향 기록 입구다. 거짓이면 수신, 참이면 송신이다.</param>
    /// <param name="existing">재진입 때 같은 실제 전송 owner 묶음이다. 비어 있으면 새로 만든다.</param>
    /// <returns>같은 기본 표를 채우고 Source 장부에 공개한 실제 수송이다.</returns>
    public static CompositeMissionTransport Make(
        ClientGameplayGlobals game,
        ClientConnectionGlobals connections,
        ClientSurfaceGlobals surface,
        ClientChatWindowGlobals chat,
        Action<Memory<byte>> receive,
        Action<string, string, uint> assertion,
        ConnectionActivityHooks activity,
        ClientChallengeGlobals challenge,
        CompositeTransportRole role,
        OriginalGameClock clock,
        Func<uint> wallMilliseconds,
        Action<bool, PacketConnection, ReadOnlyMemory<byte>, ushort> record,
        MissionNetwork? existing)
    {
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(connections);
        ArgumentNullException.ThrowIfNull(surface);
        ArgumentNullException.ThrowIfNull(chat);
        ArgumentNullException.ThrowIfNull(receive);
        ArgumentNullException.ThrowIfNull(assertion);
        ArgumentNullException.ThrowIfNull(activity);
        ArgumentNullException.ThrowIfNull(challenge);
        ArgumentNullException.ThrowIfNull(clock);
        ArgumentNullException.ThrowIfNull(wallMilliseconds);
        ArgumentNullException.ThrowIfNull(record);
        if (role != CompositeTransportRole.InternalMission) throw new NotSupportedException("수송 팩토리의 Editor/LAN 실제 backend는 후속 이식 대상");
        if (!game.PlayerRecords.Initialized) throw new InvalidOperationException("수송 생성의 실제 Player9 미초기화");
        // DN 분배기는 10.78로 고정하므로 원문 판본 분기를 두지 않는다. 월드 세션 기록 동일성 검사는 GameWorld 이식 뒤에 잇는다.
        ReadOnlyMemory<byte> players = game.PlayerRecords.Records;
        if (players.Length != 1620) throw new ArgumentException("수송 팩토리의 실제 Player9 누락", nameof(game));
        Func<bool> server = () => game.Authority.Server;
        var globals = new ClientTransportGlobals(game.PlayerRecords, game, clock);
        var hooks = new PacketDispatchHooks
        {
            Delay = connections.PacketDelays(),
            Players = () => game.PlayerRecords.Records,
            Now = () => unchecked((uint)clock.WallSeconds(wallMilliseconds())),
            Sent = (connection, packet, length) => record(true, connection, packet, length),
            Record = (connection, packet, length) => record(false, connection, packet, length),
            Terminate = connection => connection.Flags |= 0x40u,
            Close = _ => throw new InvalidOperationException("수송 팩토리의 실제 닫기 backend 미연결(R11)"),
            Release = _ => throw new InvalidOperationException("수송 팩토리의 실제 반납 backend 미연결(R11)"),
        };
        CompositeMissionTransport transport;
        if (existing?.Composite is null)
        {
            transport = new CompositeMissionTransport(role, players, server, hooks, connections.Registry, globals);
        }
        else
        {
            transport = existing.Composite;
            if (!ReferenceEquals(transport.Registry, connections.Registry)) throw new InvalidOperationException("일반 State7 재진입의 같은 실제 전송 owner 불일치");
            if (!transport.SharesNetwork(existing)) throw new InvalidOperationException("일반 State7 재진입의 같은 실제 전송 owner 불일치");
            transport.RebindWorld(role, players, server, hooks, globals);
        }

        ClientPacketDispatch.InstallDefaults(transport.Dispatch, game, connections, surface, chat, receive, assertion, activity, challenge);
        connections.PacketSources.Publish(transport);
        return transport;
    }
}
