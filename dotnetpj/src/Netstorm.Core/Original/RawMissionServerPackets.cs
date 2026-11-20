using System.Globalization;
using System.Text;
using static Netstorm.Core.Original.RawMissionPacketFields;

// 현재 Client 전역과 같은 Player9에서 로그인 서버의 실제 writer를 실행한다.
namespace Netstorm.Core.Original;

/// <summary>같은 실제 소유자의 현재 전역을 각 원문 읽기 위치에서 조회한다.</summary>
public sealed record MissionServerPacketState
{
    /// <summary>00540bc0의 현재 서버 모드다.</summary>
    public Func<bool>? Server { get; init; }
    /// <summary>00594fc8이며 Player 번호 조회마다 진단한다.</summary>
    public Func<bool>? Metamap { get; init; }
    /// <summary>00540c70의 현재 자기 번호다.</summary>
    public Func<uint>? LocalOwner { get; init; }
    /// <summary>005c85a0의 현재 중앙섬 생성 표시다.</summary>
    public Func<uint>? CentralCreated { get; init; }
    /// <summary>005c8bb4의 현재 밀도이며 패킷에는 낮은BYTE만 쓴다.</summary>
    public Func<uint>? Archipelago { get; init; }
    /// <summary>같은 실제 Zone6바이트를 빌린다. 현재 앞DWORD의100비트를 읽는다.</summary>
    public ReadOnlyMemory<byte> ZoneAddress { get; init; }
}

/// <summary>주소·송신·중앙섬 생성의 실제 하위만 외부 장치로 받는다.</summary>
public sealed record MissionServerPacketHooks
{
    /// <summary>진단 후 반환하면 같은 본체가 원문 순서로 계속한다.</summary>
    public Action<string>? Assertion { get; init; }
    /// <summary>현재 연결의 원본 좁은 주소 문자열을 반환한다.</summary>
    public Func<uint, string>? PeerAddress { get; init; }
    /// <summary>전체 mutable 예약을 실제 연결 gate로 보낸다. observer가 뒤에서 선언길이를 확정한다.</summary>
    public Action<int, Memory<byte>>? Send { get; init; }
    /// <summary>중앙섬 생성의 실제 화면/파일 로그 소비자다.</summary>
    public Action<string>? Log { get; init; }
    /// <summary>번역된 Creating Islands 안내를 표시하는 실제 하위다.</summary>
    public Action? CreatingIslands { get; init; }
    /// <summary>같은 실제 밀도 DWORD를 변경한다.</summary>
    public Action<uint>? SetArchipelago { get; init; }
    /// <summary>같은 실제 중앙섬 생성 DWORD를 변경한다.</summary>
    public Action<uint>? SetCentralCreated { get; init; }
    /// <summary>생성 안내와 밀도 쓰기 뒤 현재 배치 BYTE를 읽는다.</summary>
    public Func<byte>? Layout { get; init; }
    /// <summary>실제 owner0 중앙섬 생성 부모에 값형 Zone6바이트를 전달한다.</summary>
    public Action<ReadOnlyMemory<byte>>? RandomCentral { get; init; }
    /// <summary>배치2의 SURROUNDED Template 생성 부모다.</summary>
    public Action? SurroundedCentral { get; init; }
    /// <summary>생성 후 현재 SID Pool의 predictableCount를 조회한다.</summary>
    public Func<uint>? PredictableCount { get; init; }
}

/// <summary>PATCH0048f9f0/004a6b40와004b3bc0의 현재 Player·metadata 송수신 순서를 보존한다.</summary>
public sealed class RawMissionServerPackets
{
    /// <summary>실제 Player 한 명의 바이트 폭이다.</summary>
    private const int Stride = 180;
    /// <summary>실제 Player 기록 수다.</summary>
    private const uint Count = 9;
    /// <summary>Player writer의 원본 스택 예약 크기다.</summary>
    private const int PlayerPacketReserve = 100;
    /// <summary>이름 정제도 직접 반영할 같은 실제 Player9 저장소다.</summary>
    private readonly Memory<byte> _players;
    /// <summary>원문 전역 참조를 현재 getter로 빌린다.</summary>
    private readonly MissionServerPacketState _state;
    /// <summary>선택 위치에서만 요구할 실제 소비자 묶음이다.</summary>
    private readonly MissionServerPacketHooks _hooks;

    /// <summary>참가자 전체가 공유하는 Player9와 Zone6바이트만 허용하며 접속/생성은 수행하지 않는다.</summary>
    public RawMissionServerPackets(Memory<byte> players, MissionServerPacketState state, MissionServerPacketHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (players.Length != Stride * Count || state.ZoneAddress.Length != 6) throw new ArgumentException("미션 서버의 같은 Player9/Zone6바이트 누락");
        _players = players; _state = state; _hooks = hooks;
    }

    /// <summary>같은Session의실제Player물리인지연결부모가검증할수있게빌린다.</summary>
    public Memory<byte> Players => _players;
    /// <summary>각 Player 번호 조회에서 현재 메타맵 진단을 수행하고 낮은BYTE를 반환한다.</summary>
    private uint Owner(uint owner)
    {
        if (Need(_state.Metamap)()) Need(_hooks.Assertion)("!inMetamapMode");
        if (owner >= Count) throw new ArgumentOutOfRangeException(nameof(owner), "미션 서버 Player 번호 범위");
        return owner & 255;
    }

    /// <summary>현재 같은 Player 저장소에서 필요한 폭만 읽는다.</summary>
    private uint Read(uint owner, int offset, int width = 4)
    {
        if (owner >= Count || (uint)offset > Stride || width > Stride - offset) throw new ArgumentOutOfRangeException(nameof(owner), "미션 서버 Player 필드 범위");
        return Get(_players.Span, checked((int)owner * Stride + offset), width);
    }

    /// <summary>전체ff 예약과 version/type/초기 선언길이4만 준비한다.</summary>
    private static byte[] Packet(byte type, int size)
    {
        byte[] packet = new byte[size]; Array.Fill(packet, (byte)255); packet[0] = 1; packet[1] = type; Put(packet, 2, 4, 2); return packet;
    }

    /// <summary>현재 이름 정제·주소·각 Player 필드의 원문 재조회 뒤 전체100바이트 예약을 보낸다.</summary>
    public void SendPlayer(uint owner, int target)
    {
        if (!Need(_state.Server)()) Need(_hooks.Assertion)("server");
        byte[] packet = Packet(0x2b, PlayerPacketReserve); packet[0x32] = unchecked((byte)Owner(owner));
        Memory<byte> name = _players[(checked((int)owner) * Stride + 12)..]; SanitizePlayerName(Read(owner, 0x2c), name.Span);
        int length = name.Span.IndexOf((byte)0);
        if (length < 0) throw new ArgumentOutOfRangeException(nameof(owner), "미션 서버 현재 이름 NUL 누락");
        packet[0x33] = unchecked((byte)(length + 1)); CopyCString(packet, 0x3b, name.Span[..length]); Put(packet, 0x26, Read(owner, 0x2c));
        if ((Read(owner, 0x50, 1) & 1) == 0) packet[4] = 0;
        else CopyCString(packet, 4, Encoding.Latin1.GetBytes(Need(_hooks.PeerAddress)(Owner(owner))));
        Put(packet, 0x2a, Read(owner, 0x50)); Put(packet, 0x22, Read(owner, 0x54)); packet[0x34] = (byte)Read(owner, 0x5c, 1); Put(packet, 0x35, Read(owner, 0x60, 2), 2);
        packet[0x37] = (byte)Read(owner, 0xa0, 1); packet[0x38] = (byte)Read(owner, 0xa4, 1); packet[0x39] = (byte)Read(owner, 0xa5, 1); packet[0x3a] = (byte)Read(owner, 0x71, 1);
        Need(_hooks.Send)(target, packet);
    }

    /// <summary>Zone 관문 뒤 실제 안내·현재 밀도·두 중앙섬 경로를 순서대로 호출한다.</summary>
    public void ReceiveMetadata(ReadOnlyMemory<byte> packet)
    {
        if ((Get(_state.ZoneAddress.Span, 0) & 0x100) == 0) { Need(_hooks.Log)("No Central Island.\n"); return; }
        if (packet.Length <= 10) throw new ArgumentOutOfRangeException(nameof(packet), "중앙섬 metadata 원문 부족");
        if (packet.Span[10] != (byte)'$') return;
        Need(_hooks.CreatingIslands)(); Need(_hooks.Log)("createArchipelago... "); Need(_hooks.SetArchipelago)(Get(packet.Span, 5, 1));
        if (Need(_hooks.Layout)() == 2) { Need(_hooks.SurroundedCentral)(); return; }
        byte[] record = _state.ZoneAddress.ToArray(); Need(_hooks.RandomCentral)(record); Need(_hooks.SetCentralCreated)(1);
        Need(_hooks.Log)("predictableCount = " + unchecked((int)Need(_hooks.PredictableCount)()).ToString(CultureInfo.InvariantCulture) + "\n");
    }

    /// <summary>같은 원문 metadata observer를 현재 진단 장치로 호출한다.</summary>
    public void ObserveMetadata(Memory<byte> packet, uint sending) => RawMissionPacketFields.ObserveMetadata(packet, sending, _hooks.Assertion);

    /// <summary>$Random 작성·현재 자기 수신·현재 자기 번호 비교 뒤 다른 대상에만 전체 예약을 보낸다.</summary>
    public void SendMetadata(int target)
    {
        ReadOnlySpan<byte> name = "$Random"u8; byte[] packet = Packet(0x72, name.Length + 11); packet[4] = 0; Put(packet, 6, (uint)name.Length + 1); CopyCString(packet, 10, name);
        packet[5] = unchecked((byte)Need(_state.Archipelago)());
        if (Need(_state.CentralCreated)() == 0) { if (!Need(_state.Server)()) Need(_hooks.Assertion)("server"); ReceiveMetadata(packet); }
        if (unchecked((uint)target) != Need(_state.LocalOwner)()) Need(_hooks.Send)(target, packet);
    }
}
