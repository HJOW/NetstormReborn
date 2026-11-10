// 단독·로컬 전투의 Player 연결 표를 읽는 송신 분배 몸체의 사전 구현을 공유한다.
// 근거: cpppj `src/o/RawBattleBroadcast.h`·`RawBattleBroadcast.cpp`(004e2e30·004e2e80).
// 분배기·세션 연결은 3차 로컬 구현 때 잇는다. 호출자는 players·peers를 같은 수명으로 둔다.
// 2026-11-11 멀티플레이 사전 구현에서 추가했다. 지시대로 검사는 건너뛴다.
using Netstorm.Core.Original;

namespace Netstorm.Net;

// 전투 방송 범주를 담는 네임스페이스다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.

// 실제 0x86 비행 명령 packet 작성이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public static class BattlePacketWriter
{
    /// <summary>비행 명령 packet 길이 20바이트다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public const int PacketLength = 20;

    /// <summary>004e2e30 계열의 비행 명령 packet을 쓴다. 부모 SID·명령 DWORD를 little endian으로 둔다.</summary>
    /// <param name="packet">쓸 20바이트 버퍼다.</param>
    /// <param name="parent">부를 부모 SID 번호다.</param>
    /// <param name="command">부를 명령 DWORD다.</param>
    /// <exception cref="ArgumentOutOfRangeException">packet 20바이트 미만이다.</exception>
    public static void WriteFlyerCommandPacket(Span<byte> packet, int parent, uint command)
    {
        if (packet.Length < PacketLength)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "비행 명령 packet 20바이트 필요");
        }

        packet[0] = 1;
        packet[1] = 0x86;
        packet[2] = (byte)PacketLength;
        packet[3] = 0;
        // 부호·부모 SID·명령 DWORD를 낮은 바이트부터 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            packet[4 + index] = unchecked((byte)(unchecked((uint)parent) >> (index * 8)));
            packet[12 + index] = unchecked((byte)(command >> (index * 8)));
        }
    }

    /// <summary>004e2e30 계열의 섬 떨구기 packet을 쓴다. 좌표·깃발·소유자 DWORD 4개를 순서대로 둔다.</summary>
    /// <param name="packet">쓸 20바이트 버퍼다.</param>
    /// <param name="x">떨굴 X 좌표 DWORD 비트다.</param>
    /// <param name="y">떨굴 Y 좌표 DWORD 비트다.</param>
    /// <param name="flags">떨구기 깃발 DWORD다.</param>
    /// <param name="owner">떨군 뒤 소유자 번호다.</param>
    /// <exception cref="ArgumentOutOfRangeException">packet 20바이트 미만이다.</exception>
    public static void WriteDropIslandPacket(Span<byte> packet, uint x, uint y, uint flags, uint owner)
    {
        if (packet.Length < PacketLength)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "섬 떨구기 packet 20바이트 필요");
        }

        packet[0] = 1;
        packet[1] = 0x35;
        packet[2] = (byte)PacketLength;
        packet[3] = 0;
        uint[] payload = [x, y, flags, owner];
        // 호출자 +4·+8·+12·+16 순서 그대로 DWORD를 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (int field = 0; field < payload.Length; ++field)
        {
            for (int index = 0; index < 4; ++index)
            {
                packet[4 + field * 4 + index] = unchecked((byte)(payload[field] >> (index * 8)));
            }
        }
    }
}

// 전투 방송 송신 몸체다. 1..8 소유자의 연결 bit·peer·자신을 가려 보낸다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class RawBattleBroadcast
{
    /// <summary>같은 9칸 송신 표다. 0번은 쓰지 않으며 비어 있으면 건너뛴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly Action<byte[]>?[] _peers;

    /// <summary>같은 9개 Player 바이트다. 호출자가 소유하며 세션 수명과 같이 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly byte[] _players;

    /// <summary>현재 로컬 소유자 번호 읽기다. 원본 참조 대신 읽을 때마다 묻는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly Func<uint> _localOwner;

    /// <summary>현재 서버 여부 읽기다. 원본 참조 대신 읽을 때마다 묻는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly Func<bool> _server;

    /// <summary>비서버 제외 송신 때 부르는 진단이다. 없으면 명시 오류로 멈춘다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly Action? _assertion;

    /// <summary>같은 실제 Registry mutable 송신이다. 묶기 전에는 비어 있다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private Action<uint, byte[]>? _mutableSend;

    /// <summary>판본별 Player 한 기록 크기다. PATCH는 0xb4, CD는 0xac이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly int _stride;

    /// <summary>연결 bit의 기록 안 위치다. 0x50 고정이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private const int ConnectedOffset = 0x50;

    /// <summary>같은 Player9·송신 표·현재 읽기 효과로 방송 몸체를 만든다. 분배기·세션 연결은 3차 로컬 구현 때 잇는다.</summary>
    /// <param name="edition">쓸 판본이다. 기록 크기를 가른다.</param>
    /// <param name="players">읽을 9개 Player 바이트다. 9×기록 크기여야 한다.</param>
    /// <param name="peers">쓸 9칸 송신 표다. 호출자가 소유한다.</param>
    /// <param name="localOwner">읽을 현재 로컬 소유자 번호다.</param>
    /// <param name="server">읽을 현재 서버 여부다.</param>
    /// <param name="assertion">부를 비서버 진단이다. 없으면 제외 송신 때 멈춘다.</param>
    /// <exception cref="ArgumentException">Player9 크기 오류다.</exception>
    public RawBattleBroadcast(OriginalEdition edition, byte[] players, Action<byte[]>?[] peers, Func<uint> localOwner, Func<bool> server, Action? assertion = null)
    {
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(peers);
        ArgumentNullException.ThrowIfNull(localOwner);
        ArgumentNullException.ThrowIfNull(server);
        int stride = edition == OriginalEdition.Patch1078 ? 0xb4 : 0xac;
        if (players.Length != 9 * stride)
        {
            throw new ArgumentException("전투 방송의 실제 Player9 크기 오류", nameof(players));
        }

        _players = players;
        _peers = peers;
        _localOwner = localOwner;
        _server = server;
        _assertion = assertion;
        _stride = stride;
    }

    /// <summary>연결된 1..8 소유자 중 자신이 아닌 끝에만 같은 packet을 보낸다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="packet">보낼 packet이다. 호출자가 소유하며 그대로 넘긴다.</param>
    public void Send(byte[] packet)
    {
        ArgumentNullException.ThrowIfNull(packet);
        uint local = _localOwner();
        // 1..8을 돌며 Player0은 건너뛰고 연결·peer·자기를 가린다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (uint owner = 1u; owner < 9u; ++owner)
        {
            if ((_players[(int)(owner * (uint)_stride) + ConnectedOffset] & 1u) == 0u)
            {
                continue;
            }

            if (owner >= (uint)_peers.Length || _peers[(int)owner] is null || owner == local)
            {
                continue;
            }

            _peers[(int)owner]!(packet);
        }
    }

    /// <summary>같은 실제 Registry mutable 송신을 묶는다. 세션 연결 때 실제 분배기로 바꾼다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="send">묶을 mutable 송신 효과다. 소유자 번호와 packet을 그대로 받는다.</param>
    public void BindMutableSend(Action<uint, byte[]> send)
    {
        ArgumentNullException.ThrowIfNull(send);
        _mutableSend = send;
    }

    /// <summary>연결된 1..8 소유자 중 자신이 아닌 끝에만 mutable packet을 보낸다. 묶기 전에는 멈춘다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="packet">보낼 mutable packet이다. 호출자가 소유한다.</param>
    /// <exception cref="InvalidOperationException">실제 mutable 송신 미연결이다.</exception>
    public void SendMutable(byte[] packet)
    {
        ArgumentNullException.ThrowIfNull(packet);
        uint local = _localOwner();
        // 1..8을 돌며 같은 가림 뒤 묶은 mutable 효과로 target까지 보낸다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (uint owner = 1u; owner < 9u; ++owner)
        {
            if ((_players[(int)(owner * (uint)_stride) + ConnectedOffset] & 1u) == 0u)
            {
                continue;
            }

            if (owner >= (uint)_peers.Length || _peers[(int)owner] is null || owner == local)
            {
                continue;
            }

            if (_mutableSend is null)
            {
                throw new InvalidOperationException("전투 방송의 실제 mutable 송신 미연결");
            }

            _mutableSend(owner, packet);
        }
    }

    /// <summary>서버면 제외 번호·자기를 빼고 보내고, 비서버면 진단 뒤 같은 가림으로 보낸다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="excluded">뺄 소유자 번호다.</param>
    /// <param name="packet">보낼 packet이다. 호출자가 소유하며 그대로 넘긴다.</param>
    /// <exception cref="InvalidOperationException">비서버 진단 미연결이다.</exception>
    public void SendExcept(int excluded, byte[] packet)
    {
        ArgumentNullException.ThrowIfNull(packet);
        if (!_server())
        {
            if (_assertion is null)
            {
                throw new InvalidOperationException("전투 방송의 비서버 진단 미연결");
            }

            _assertion();
        }

        uint local = _localOwner();
        // 연결 bit·제외·자기·peer를 같은 순서로 가린다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (uint owner = 1u; owner < 9u; ++owner)
        {
            if ((_players[(int)(owner * (uint)_stride) + ConnectedOffset] & 1u) == 0u)
            {
                continue;
            }

            if ((int)owner == excluded || owner == local)
            {
                continue;
            }

            if (owner >= (uint)_peers.Length || _peers[(int)owner] is null)
            {
                continue;
            }

            _peers[(int)owner]!(packet);
        }
    }
}
