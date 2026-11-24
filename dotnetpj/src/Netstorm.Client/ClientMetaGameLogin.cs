// 0x7c 메타 게임 로그인의 상태 분기와 0x1f 마스터 알림·고정 F 설치를 같은 분배기에 연결한다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 ReceiveClientMetaGameLogin(004b2c00)/NotifyClientMetaMaster/InstallClientMetaGameLoginReceiver(0x7c 0x004b2cc0)를 이식한다.
// 0.946에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client;

// 004b2c00 메타 게임 로그인이 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaGameLoginHooks
{
    /// <summary>같은 실제 CS 전투10·참가자40 기록이다.</summary>
    public ClientChallengeGlobals? Challenge { get; set; }

    /// <summary>현재 전투 소유자 번호다.</summary>
    public Func<uint>? BattleOwner { get; set; }

    /// <summary>현재 구독자 번호다.</summary>
    public Func<uint>? CurrentSubscriber { get; set; }

    /// <summary>실제 연결표에 보내는 가변 패킷 송신 입구다.</summary>
    public Func<int, Memory<byte>, bool>? Send { get; set; }

    /// <summary>실제 연결표의 연결 끊기 입구다.</summary>
    public Action<int>? Disconnect { get; set; }

    /// <summary>실제 연결표의 pending·target 재지정 입구다. 셋째 값은 원문 서버 여부다.</summary>
    public Action<int, int, bool>? Assign { get; set; }

    /// <summary>현재 서버 여부를 읽는다.</summary>
    public Func<bool>? GetServer { get; set; }

    /// <summary>현재 서버 여부를 쓴다.</summary>
    public Action<bool>? SetServer { get; set; }

    /// <summary>현재 캐시한 참가자 번호를 쓴다.</summary>
    public Action<uint>? SetCachedPeer { get; set; }
}

// 004b2c00 전체이며 상태 2 이외는 원문 분기 순서와 실패 시 이전 서버값 미복원을 보존한다. 메뉴·세션을 요구하지 않는다.
public static class ClientMetaGameLogin
{
    /// <summary>원문 0x7c 메타 게임 로그인 종류다. 0.946에서 추가됐다.</summary>
    private const uint Type = 0x7c;

    /// <summary>원문 0x7c 수신의 실제 고정 F 주소다. 0.946에서 추가됐다.</summary>
    private const uint NativeEntry = 0x004b2cc0;

    /// <summary>원문 0x1f 마스터 알림 종류다. 0.946에서 추가됐다.</summary>
    private const uint NotifyType = 0x1f;

    /// <summary>원문 0x1f 마스터 알림의 전송 대상 연결 번호다. 0.946에서 추가됐다.</summary>
    private const int NotifyTarget = 11;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.946에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 게임 로그인의 선택된 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.946에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 게임 로그인의 실제 읽기 범위");
        }

        uint value = 0;
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>부분 효과를 보존하고 원문 폭만큼 낮은 자리부터 쓴다. 0.946에서 추가됐다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 게임 로그인의 실제 쓰기 범위");
        }

        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>현재 자기 CS 기록을 먼저 캡처하고 flags 쓰기 뒤 전역11의 실제 가상 송신만 선택한다. 0.946에서 추가됐다.</summary>
    /// <param name="hooks">실제 CS·소유자·구독자·송신 장치다.</param>
    public static void NotifyMaster(ClientMetaGameLoginHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientChallengeGlobals challenge = hooks.Challenge ?? throw new InvalidOperationException("메타 게임 로그인의 실제 CS 미연결");
        uint owner = Need(hooks.BattleOwner)();
        Memory<byte> own = challenge.Participant(owner);
        byte[] packet = new byte[12];
        packet.AsSpan().Fill(0xff);
        Put(packet, 0, 1, 1);
        Put(packet, 1, NotifyType, 1);
        Put(packet, 2, 12, 2);
        Put(packet, 4, Need(hooks.CurrentSubscriber)());
        uint flags = Read(own.Span, 0x64) | 0x2000u;
        Put(own.Span, 0x64, flags);
        Put(packet, 8, flags);
        // Source는 실제11 송신 Binding 선택 뒤에만 pin한다.
        Need(hooks.Send)(NotifyTarget, packet);
    }

    /// <summary>원문 status 각 분기에서 선택한 현재 CS만 쓰고 Meta/Session을 앞에서 요구하지 않는다. 0.946에서 추가됐다.</summary>
    /// <param name="hooks">실제 CS·소유자·구독자·연결·서버·캐시 장치다.</param>
    /// <param name="sender">원문 송신자 번호다.</param>
    /// <param name="packet">원문 0x7c 패킷이다.</param>
    public static void Receive(ClientMetaGameLoginHooks hooks, uint sender, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint status = Read(packet.Span, 4);
        ClientChallengeGlobals challenge = hooks.Challenge ?? throw new InvalidOperationException("메타 게임 로그인의 실제 CS 미연결");
        if (status == 0)
        {
            bool accepted = false;
            uint owner = Need(hooks.BattleOwner)();
            if (unchecked((uint)ClientMetaBattleMaster.Find(challenge, owner)) == owner)
            {
                Memory<byte> record = challenge.Participant(Read(packet.Span, 12));
                uint subscriber = Read(record.Span, 4);
                if (subscriber == Read(packet.Span, 16))
                {
                    uint battle = Read(record.Span, 0xa4);
                    if (battle == Read(packet.Span, 20))
                    {
                        uint seat = Read(record.Span, 0xa8);
                        if (seat == Read(packet.Span, 24))
                        {
                            accepted = true;
                        }
                    }
                }
            }

            byte[] reply = new byte[28];
            reply.AsSpan().Fill(0xff);
            Put(reply, 0, 1, 1);
            Put(reply, 1, Type, 1);
            Put(reply, 2, 28, 2);
            Put(reply, 4, accepted ? 1u : 2u);
            Put(reply, 8, Need(hooks.CurrentSubscriber)());
            int pending = unchecked((int)sender);
            Need(hooks.Send)(pending, reply);
            if (!accepted)
            {
                return;
            }

            int target = unchecked((int)(Read(packet.Span, 24) + 1u));
            Need(hooks.Disconnect)(target);
            Need(hooks.SetServer)(true);
            Need(hooks.Assign)(pending, target, Need(hooks.GetServer)());
            Need(hooks.SetServer)(false);
            return;
        }

        if (status == 1)
        {
            int master = ClientMetaBattleMaster.Find(challenge, Need(hooks.BattleOwner)());
            if (master < 0)
            {
                return;
            }

            uint subscriber = Read(challenge.Participant(unchecked((uint)master)).Span, 4);
            if (subscriber == Read(packet.Span, 8))
            {
                NotifyMaster(hooks);
            }

            return;
        }

        // 그외는 현재 캐시한 DWORD만 쓰고 CS/Source/Session을 읽지 않는다.
        Need(hooks.SetCachedPeer)(0xffffffffu);
    }

    /// <summary>0x7c의 원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.946에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="game">같은 실제 Player/구독자/연결 소유자다.</param>
    /// <param name="connections">같은 실제 연결 Registry/LocalState owner다.</param>
    /// <param name="challenge">같은 실제 CS owner다.</param>
    /// <param name="storage">같은 실제 장기 진단 owner다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientGameplayGlobals game, ClientConnectionGlobals connections, ClientChallengeGlobals challenge, RawSurfaceStorage storage)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(connections);
        ArgumentNullException.ThrowIfNull(challenge);
        ArgumentNullException.ThrowIfNull(storage);
        if (!ReferenceEquals(connections.LocalState.Gameplay, game))
        {
            throw new ArgumentException("메타 게임 로그인의 같은 실제 Client 소유자 필요", nameof(connections));
        }

        dispatch.Observe(Type, static _ => { });
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, 28);
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (sender, _, packet) => Receive(new ClientMetaGameLoginHooks
        {
            Challenge = challenge,
            BattleOwner = () => game.Player.BattleOwner,
            CurrentSubscriber = () => (game.Subscribers ?? throw new InvalidOperationException("메타 게임 로그인의 실제 구독자 없음")).MySubscriber.Value,
            Send = (id, bytes) => connections.Registry.SendMutableNative(id, bytes, game.PlayerRecords.Records, storage.AssertDisplay),
            Disconnect = id => connections.Registry.DisconnectNative(id, storage.AssertDisplay),
            Assign = (pending, target, server) => connections.Registry.AssignNative(pending, target, server ? 1u : 0u, storage.AssertDisplay),
            GetServer = () => game.Authority.Server,
            SetServer = value => game.Authority.Server = value,
            SetCachedPeer = value => connections.LocalState.CachedPeerIndex.Value = value,
        }, sender, packet));
    }
}
