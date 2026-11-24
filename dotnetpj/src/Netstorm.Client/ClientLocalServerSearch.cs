// 0x66 로컬 서버 검색의 송신 패킷 작성과 검색 기한·재송신 순서를 같은 장기 장치에 연결한다.
// cpppj/src/o/RawLocalServerDiscovery.cpp 의 MakeLocalServerDiscoveryPacket/AnnounceLocalServer/BroadcastLocalServerDiscovery/BeginLocalServerDiscovery/StartLocalServerSearch/ContinueLocalServerSearch 를 이식한다.
// 0.950에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client;

// 004b3fc0/004b4050/State34·35 검색이 선택한 효과의 실제 외부 장치다.
public sealed class ClientLocalServerSearchHooks
{
    /// <summary>같은 실제 로컬 상태 블록 owner다.</summary>
    public LocalMissionStateStorage? State { get; set; }

    /// <summary>같은 실제 검색 토큰 owner다.</summary>
    public UIntBox? Token { get; set; }

    /// <summary>같은 실제 검색 만료 기한 owner다.</summary>
    public DoubleBox? SearchEnd { get; set; }

    /// <summary>같은 실제 재송신 기한 owner다.</summary>
    public DoubleBox? RetryEnd { get; set; }

    /// <summary>호출 위치마다 새로 읽는 실제 벽시각이다.</summary>
    public Func<double>? Wall { get; set; }

    /// <summary>실제 UDP 열기이며 성공 합성을 대신하지 않는다.</summary>
    public Action? Open { get; set; }

    /// <summary>실제 12BYTE UDP 브로드캐스트 장치다.</summary>
    public Action<byte[]>? Broadcast { get; set; }

    /// <summary>State34의 현재 번역된 안내다.</summary>
    public Action? Progress { get; set; }

    /// <summary>State34의 실제 팔레트 변경 부모다.</summary>
    public Action? Palette { get; set; }

    /// <summary>Netstorm Root Server 제목을 가진 실제 창 검색이다.</summary>
    public Func<bool>? ServerWindowExists { get; set; }
}

// Native004b3fc0/004b4050/State34·35의 원문 순서를 보존한다. 실제 전송·창 존재·상태 실행은 호출자의 장치다.
public static class ClientLocalServerSearch
{
    /// <summary>원문 0x66 패킷의 고정 종류 길이다. 0.950에서 추가됐다.</summary>
    private const int PacketLength = 12;

    /// <summary>State34의 첫 검색 만료 기한이다. 0.950에서 추가됐다.</summary>
    private const double SearchSeconds = 5.0;

    /// <summary>검색 만료 뒤 재검색 유예 기한이다. 0.950에서 추가됐다.</summary>
    private const double ResearchSeconds = 4.0;

    /// <summary>경쟁 검색 연장 시 더하는 실제 12초다. 0.950에서 추가됐다.</summary>
    private const double ExtendSeconds = 12.0;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.950에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("로컬 서버 검색의 선택된 실제 장치 미연결");

    /// <summary>원문 _ftol의 signed64 절삭 하위 DWORD이며 비유한/범위 밖은 0이다. 0.950에서 추가됐다.</summary>
    private static uint Seed(double value)
    {
        // 원문 ftol의 범위 밖·무한대 입력은 하위값 0으로 둔다.
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((uint)checked((long)value));
    }

    /// <summary>원문 현재 15비트 난수를 한 번만 소비하고 지연을 반환한다. 0.950에서 추가됐다.</summary>
    private static double RetryDelay(CrtRandom random)
    {
        ArgumentNullException.ThrowIfNull(random);
        return random.Next() % 100U * 0.001 + 0.5;
    }

    /// <summary>원문 header 뒤 두 DWORD만 caller 값으로 쓰는 12BYTE를 작성한다. 0.950에서 추가됐다.</summary>
    /// <param name="command">원문 명령 번호다.</param>
    /// <param name="value">뒤따르는 payload DWORD다.</param>
    public static byte[] MakePacket(uint command, uint value)
    {
        byte[] packet = new byte[PacketLength];
        packet[0] = 1;
        packet[1] = 0x66;
        packet[2] = 12;
        packet[3] = 0;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), command);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(8, 4), value);
        return packet;
    }

    /// <summary>같은 서버 발견 패킷을 실제 UDP 노드의 가상 송신으로 세번 전달한다. 0.950에서 추가됐다.</summary>
    /// <param name="basePort">명령1의 +8에 쓰는 현재 서버 basePort다. 클라이언트 검색 토큰이 아니다.</param>
    /// <param name="broadcast">실제 12BYTE UDP 브로드캐스트 장치다.</param>
    public static void Announce(uint basePort, Action<byte[]> broadcast)
    {
        ArgumentNullException.ThrowIfNull(broadcast);
        byte[] packet = MakePacket(1, basePort);
        for (int attempt = 0; attempt < 3; attempt++)
        {
            Need(broadcast)(packet);
        }
    }

    /// <summary>초기 검색과 이후 재송신이 같은 현재 토큰을 다시 읽는다. 0.950에서 추가됐다.</summary>
    public static void Broadcast(ClientLocalServerSearchHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        UIntBox token = hooks.Token ?? throw new InvalidOperationException("로컬 서버 검색의 실제 토큰 미연결");
        Need(hooks.Broadcast)(MakePacket(0, token.Value));
    }

    /// <summary>열기 뒤 srand/네 번 rand의 lowBYTE를 첫 토큰으로 저장하고 첫 송신한다. 0.950에서 추가됐다.</summary>
    public static void Begin(ClientLocalServerSearchHooks hooks, CrtRandom random)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(random);
        UIntBox token = hooks.Token ?? throw new InvalidOperationException("로컬 서버 검색의 실제 토큰 미연결");
        Need(hooks.Open)();
        random.SetState(Seed(Need(hooks.Wall)() * 1000.0));
        uint current = 0;
        for (int index = 0; index < 4; index++)
        {
            current = (current << 8) | (random.Next() & 0xffu);
        }

        token.Value = current;
        Broadcast(hooks);
    }

    /// <summary>State34의 안내→첫 검색→5초 기한→0.500~0.599초 재송신→팔레트→즉시35를 수행한다. 0.950에서 추가됐다.</summary>
    public static void Start(ClientLocalServerSearchHooks hooks, CrtRandom random)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(random);
        LocalMissionStateStorage state = hooks.State ?? throw new InvalidOperationException("로컬 서버 검색의 실제 상태 미연결");
        DoubleBox searchEnd = hooks.SearchEnd ?? throw new InvalidOperationException("로컬 서버 검색의 실제 기한 미연결");
        DoubleBox retryEnd = hooks.RetryEnd ?? throw new InvalidOperationException("로컬 서버 검색의 실제 재송신 기한 미연결");
        Need(hooks.Progress)();
        Begin(hooks, random);
        searchEnd.Value = Need(hooks.Wall)() + SearchSeconds;
        double delay = RetryDelay(random);
        retryEnd.Value = delay + Need(hooks.Wall)();
        Need(hooks.Palette)();
        BinaryPrimitives.WriteUInt32LittleEndian(state.Immediate.Span.Slice(0, 4), 35);
    }

    /// <summary>State35의 재송신/만료/서버 창 확인/즉시37와 다음35를 보존한다. 0.950에서 추가됐다.</summary>
    public static void Continue(ClientLocalServerSearchHooks hooks, CrtRandom random)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(random);
        LocalMissionStateStorage state = hooks.State ?? throw new InvalidOperationException("로컬 서버 검색의 실제 상태 미연결");
        DoubleBox searchEnd = hooks.SearchEnd ?? throw new InvalidOperationException("로컬 서버 검색의 실제 기한 미연결");
        DoubleBox retryEnd = hooks.RetryEnd ?? throw new InvalidOperationException("로컬 서버 검색의 실제 재송신 기한 미연결");
        if (retryEnd.Value <= Need(hooks.Wall)())
        {
            Broadcast(hooks);
            double delay = RetryDelay(random);
            retryEnd.Value = delay + Need(hooks.Wall)();
        }

        if (searchEnd.Value != 0.0 && searchEnd.Value <= Need(hooks.Wall)())
        {
            searchEnd.Value = 0.0;
            if (!Need(hooks.ServerWindowExists)())
            {
                BinaryPrimitives.WriteUInt32LittleEndian(state.Immediate.Span.Slice(0, 4), 37);
                BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0, 4), 35);
                return;
            }

            searchEnd.Value = Need(hooks.Wall)() + ResearchSeconds;
        }

        BinaryPrimitives.WriteUInt32LittleEndian(state.Next.Span.Slice(0, 4), 35);
    }
}
