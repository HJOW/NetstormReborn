using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 2번 연결 종료의 같은 원문 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientConnectionEndedReceiver(004b6660)와
// Netstorm.Net/ConnectionEnded.cs 의 ReceiveConnectionEnded(004b6660 본체 공유)를 이식한다.
// 0.953에서 R4-6 전송·수신 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 2번 연결 종료의 실제 외부 장치다. 장기 본체는 Netstorm.Net/ConnectionEnded.cs를 재사용한다.
public static class ClientConnectionEndedReceiver
{
    /// <summary>원문 2번 연결 종료 종류다. 0.953에서 추가됐다.</summary>
    public const uint Type = 2;

    /// <summary>원문 2번 수신의 실제 고정 F 주소다. 0.953에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b6660;

    /// <summary>같은 실제 소유자만 요구하며 실제 전송·세션을 선조회하지 않는다. 0.953에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("연결 종료 수신의 선택된 실제 장치 미연결");

    /// <summary>같은 실제 상태 블록의 현재 DWORD를 읽는다. 0.953에서 추가됐다.</summary>
    private static uint ReadWord(Memory<byte> block, int offset)
    {
        // 상태 블록 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > block.Length || 4 > block.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(block), "연결 종료 수신의 실제 상태 물리 부족");
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(block.Span.Slice(offset, 4));
    }

    /// <summary>같은 실제 상태 블록의 현재 DWORD만 쓴다. 0.953에서 추가됐다.</summary>
    private static void WriteWord(Memory<byte> block, int offset, uint value)
    {
        // 상태 블록 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > block.Length || 4 > block.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(block), "연결 종료 수신의 실제 상태 물리 부족");
        }

        BinaryPrimitives.WriteUInt32LittleEndian(block.Span.Slice(offset, 4), value);
    }

    /// <summary>같은 4BYTE 통지의 observer/version/length 표는 바꾸지 않고 실제 고정 F만 설치한다. 0.953에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="game">같은 실제 Player/권한 소유자다.</param>
    /// <param name="connections">같은 실제 연결 Registry/로컬 상태 owner다.</param>
    /// <param name="activity">같은 실제 22칸 활성 owner다.</param>
    /// <param name="players">실제 Player9 기록을 읽고 쓰는 바이트 효과다.</param>
    /// <param name="saveServerFailure">실제 9번 장애 저장 효과다.</param>
    /// <param name="broadcastPlayer">실제 소유자 방송 효과다.</param>
    public static void Install(
        RawPacketDispatch dispatch,
        ClientGameplayGlobals game,
        ClientConnectionGlobals connections,
        ClientConnectionActivityGlobals activity,
        Func<byte[]> players,
        Action saveServerFailure,
        Action<uint> broadcastPlayer)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(connections);
        ArgumentNullException.ThrowIfNull(activity);
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(saveServerFailure);
        ArgumentNullException.ThrowIfNull(broadcastPlayer);
        if (!ReferenceEquals(connections.LocalState.Gameplay, game))
        {
            throw new ArgumentException("연결 종료 수신의 같은 실제 Client 소유자 필요", nameof(connections));
        }

        // 저장 F는 같은 장기 저장소만 보유하며 지난 World/Meta/Session을 캡처하지 않는다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (sender, connection, _) =>
        {
            ClientLocalStateGlobals local = connections.LocalState;
            var globals = new ConnectionEndedGlobals(activity.Frames, players)
            {
                Server = game.Authority.Server,
                Synchronization = local.ClockSynchronization.Connection.Value,
                Phase = ReadWord(local.State.Phase, 0),
                NextPhase = ReadWord(local.State.Next, 0),
            };
            var effects = new ConnectionEndedHooks
            {
                // 실제 9번 장애 저장이 선택된 하위에서만 현재 전체 owner를 보유한다.
                SaveServerFailure = saveServerFailure,
                // 원문 sender의 현재 가상 연결을 조회하며 Source를 sender/phase로 추정하지 않는다.
                Disconnect = id => connections.Registry.Disconnect(id),
                // 선행 flags 저장 뒤 선택된 현재 방송 하위만 보유한다.
                BroadcastPlayer = broadcastPlayer,
            };
            // 원문 param2는 비교만 하므로 실제 NULL은 0으로 전달하고 가짜 노드/주소를 만들지 않는다.
            uint token = connection is null ? 0u : connections.Registry.TokenFor(connection);
            ConnectionEnded.ReceiveConnectionEnded(unchecked((int)sender), token, globals, effects);
            local.ClockSynchronization.Connection.Value = globals.Synchronization;
            WriteWord(local.State.Next, 0, globals.NextPhase);
        });
    }
}
