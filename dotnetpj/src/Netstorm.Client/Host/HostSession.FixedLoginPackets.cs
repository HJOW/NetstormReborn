using System.Runtime.InteropServices;
using Netstorm.Core.Original;
using Netstorm.Net;

// 고정Native엔트리가현재같은Session의실제Login/동맹본문만선택하게한다.
namespace Netstorm.Client.Host;

/// <summary>동일Player를빌린실제세Login본문과선택된현재UI상태효과다.</summary>
public sealed record HostLoginPacketModules
{
    /// <summary>27h/63h의실제서버/재접속본문이다.</summary>
    public required RawMissionLogin<PacketConnection> Server { get; init; }
    /// <summary>실제클라이언트8종본문이다. App67h는별도전역입구를유지한다.</summary>
    public required RawMissionClientPackets Client { get; init; }
    /// <summary>72h의실제전체메타데이터본문이다.</summary>
    public required RawMissionServerPackets Metadata { get; init; }
    /// <summary>현재Login이있는동맹알림의실제상태창효과다. 도달한위치에서만필수다.</summary>
    public Action<uint, uint, uint>? Status { get; init; }
}

/// <summary>Login/Editor가같은Player동맹본문을사용하는실제효과다.</summary>
public sealed record HostAlliancePacketEffects
{
    /// <summary>현재실제boss관문이다.</summary>
    public Func<bool>? Boss { get; init; }
    /// <summary>원문수신/수정버퍼를그대로읽는실제방송이다.</summary>
    public Action<ReadOnlyMemory<byte>>? Broadcast { get; init; }
    /// <summary>Login없는Editor의실제MessageBoard효과다.</summary>
    public Action<uint, uint, uint>? Status { get; init; }
}

/// <summary>현재모듈선택을호출동안보유하고packet종류/현재등록표로입구를다시선택하지않는다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재실제Login본문묶음이다. 반납/재연결이기존호출의지역참조를무효화하지않는다.</summary>
    private HostLoginPacketModules? _fixedLoginModules;
    /// <summary>현재같은실제Player/방향표/옵션숨김을빌리는동맹부모다.</summary>
    private RawPlayerParticipation? _fixedAllianceParticipation;
    /// <summary>값복사한현재동맹의실제장치다.</summary>
    private HostAlliancePacketEffects? _fixedAllianceEffects;
    /// <summary>같은실제배열/현재상태/옵션을빌린모듈만현재Login으로공개한다.</summary>
    public void BindFixedLoginPackets(HostLoginPacketModules? modules)
    {
        if (modules is not null && (modules.Server is null || modules.Client is null || modules.Metadata is null
            || !modules.Server.Players.Equals((Memory<byte>)PlayerRecords) || !modules.Client.Players.Equals((Memory<byte>)PlayerRecords)
            || !modules.Metadata.Players.Equals((Memory<byte>)PlayerRecords) || !ReferenceEquals(modules.Client.PlayerState, MissionPlayer)
            || !modules.Server.BattleOptions.Equals((ReadOnlyMemory<byte>)BattleOptions) || !modules.Client.BattleOptions.Equals((Memory<byte>)BattleOptions)))
            throw new ArgumentException("고정Login의실제Session/Player/현재상태/옵션불일치", nameof(modules));
        _fixedLoginModules = modules;
    }
    /// <summary>실제동맹본문은같은Player/방향표/숨김부분뷰만허용하고효과함수값을보관한다.</summary>
    public void BindFixedAlliancePackets(RawPlayerParticipation participation, HostAlliancePacketEffects effects)
    {
        ArgumentNullException.ThrowIfNull(participation); ArgumentNullException.ThrowIfNull(effects);
        if (!participation.Players.Equals((Memory<byte>)PlayerRecords) || !ReferenceEquals(participation.Alliances, Alliances)
            || !participation.Hidden.Equals(HiddenPlayers)) throw new ArgumentException("고정동맹의실제Player/방향표/숨김부분뷰불일치", nameof(participation));
        _fixedAllianceParticipation = participation; _fixedAllianceEffects = effects with { };
    }
    /// <summary>선택된실제큰하위만요구하고앞선메모리/방송효과를보존한다.</summary>
    private static T FixedPacketNeed<T>(T? effect) where T : class => effect ?? throw new InvalidOperationException("고정수신의실제선택본문/효과미연결");
    /// <summary>동맹두본문은현재Login없이도실행하며현재Login은최초선택해끝까지보유한다.</summary>
    private void ReceiveFixedAlliancePacket(uint entry, ReadOnlyMemory<byte> packet)
    {
        RawPlayerParticipation participant = FixedPacketNeed(_fixedAllianceParticipation);
        HostAlliancePacketEffects effects = FixedPacketNeed(_fixedAllianceEffects);
        if (entry == 0x00491220)
        {
            participant.ReceiveAlliance(MemoryMarshal.AsMemory(packet).Span, _ => FixedPacketNeed(effects.Broadcast)(packet)); return;
        }
        if (entry == 0x0048f880)
        {
            HostLoginPacketModules? currentLogin = _fixedLoginModules;
            uint owner = packet.Span[4], target = packet.Span[5];
            if (FixedPacketNeed(effects.Boss)())
            {
                participant.AddAlly(owner, target); participant.AddAlly(target, owner); FixedPacketNeed(effects.Broadcast)(packet);
            }
            FixedPacketNeed(currentLogin is not null ? currentLogin.Status : effects.Status)(7, owner, target); return;
        }
        throw new ArgumentException("미등록고정동맹Native엔트리", nameof(entry));
    }
    /// <summary>저장된불변F가선택한Native엔트리로현재실제본문을호출한다. nullable연결/원문버퍼를그대로전달한다.</summary>
    public void ReceiveFixedLoginPacket(uint entry, uint sender, PacketConnection? connection, ReadOnlyMemory<byte> packet)
    {
        if (_playerRecordsReleased) throw new ObjectDisposedException(nameof(HostSession), "고정수신의Player고정뷰반납됨");
        if (entry is 0x00491220 or 0x0048f880) { ReceiveFixedAlliancePacket(entry, packet); return; }
        HostLoginPacketModules login = FixedPacketNeed(_fixedLoginModules);
        switch (entry)
        {
            case 0x004a7800: login.Server.ReceiveDirect(unchecked((int)sender), connection, packet); return;
            case 0x004a7510: login.Server.ReceiveReconnect(unchecked((int)sender), packet); return;
            case 0x004b3b40: case 0x004912d0: case 0x004b3d30: case 0x004b8940:
            case 0x004b3e90: case 0x004a6f30: case 0x004a7050: login.Client.ReceiveFixed(entry, sender, packet); return;
            case 0x004b3bc0: login.Metadata.ReceiveMetadata(packet); return;
            default: throw new ArgumentException("미등록고정Login Native엔트리", nameof(entry));
        }
    }
}
