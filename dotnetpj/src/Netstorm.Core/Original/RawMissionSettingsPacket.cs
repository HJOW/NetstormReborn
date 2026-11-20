using static Netstorm.Core.Original.RawMissionPacketFields;

// 로그인 설정7DWORD의 현재 읽기/쓰기 순서와 실제 송신을 공유한다.
namespace Netstorm.Core.Original;

/// <summary>payload의 필드 번호이며 실제 송신 조회 순서와 구별한다.</summary>
public enum MissionSettingsField : byte
{
    /// <summary>005326cc의 현재 제네레이터 타이머다.</summary>
    GeneratorTimer,
    /// <summary>0055a488의 현재 이동 디버그 설정이다.</summary>
    HackMover,
    /// <summary>0055a48c의 현재 피해 디버그 설정이다.</summary>
    NoDamageDebug,
    /// <summary>0055a490의 현재 아군 사격 설정이다.</summary>
    ShootMyFriends,
    /// <summary>0055a494의 현재 탱크 설정 DWORD다.</summary>
    FillMyTank,
    /// <summary>005326d0의 현재 전투 패널 열 수다.</summary>
    CombatGumpColumns,
    /// <summary>005326d4의 현재 전투 패널 행 수다.</summary>
    CombatGumpRows,
}

/// <summary>실제 현재 설정과 현재 연결 gate를 지나는 송신만 외부 장치다.</summary>
public sealed record MissionSettingsPacketHooks
{
    /// <summary>각 원문 load 위치에서 같은 현재 DWORD를 읽는다.</summary>
    public Func<MissionSettingsField, uint>? Current { get; init; }
    /// <summary>각 원문 store 위치에서 같은 실제 DWORD를 쓴다.</summary>
    public Action<MissionSettingsField, uint>? SetCurrent { get; init; }
    /// <summary>실제 현재 연결 gate로32바이트를 보낸다.</summary>
    public Action<int, ReadOnlyMemory<byte>>? Send { get; init; }
}

/// <summary>RawMissionSettingsPacket.cpp의0044dbc0/0044dc20 전체 송수신 본체다.</summary>
public sealed class RawMissionSettingsPacket
{
    /// <summary>실제 현재 설정/송신 장치이며 값 캐시를 만들지 않는다.</summary>
    private readonly MissionSettingsPacketHooks _hooks;
    /// <summary>PATCH와 실제 현재 getter를 확인하며 설정/패킷은 만들지 않는다.</summary>
    public RawMissionSettingsPacket(OriginalEdition edition, MissionSettingsPacketHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); if (edition != OriginalEdition.Patch1078 || hooks.Current is null) throw new ArgumentException("미션 설정 패킷 PATCH 현재 설정 조회 누락"); _hooks = hooks;
    }
    /// <summary>원문1/2/0/4/5/3/6 조회와 앞선 지역값 보존·실제 packet쓰기 위치를 유지한다.</summary>
    public void Send(int target)
    {
        byte[] packet = new byte[32]; packet[0] = 1; packet[1] = 0x32; packet[2] = 32;
        uint hack = _hooks.Current!(MissionSettingsField.HackMover), noDamage = _hooks.Current(MissionSettingsField.NoDamageDebug), generator = _hooks.Current(MissionSettingsField.GeneratorTimer); Put(packet, 8, hack);
        uint fill = _hooks.Current(MissionSettingsField.FillMyTank); Put(packet, 12, noDamage); uint columns = _hooks.Current(MissionSettingsField.CombatGumpColumns); Put(packet, 4, generator);
        uint shoot = _hooks.Current(MissionSettingsField.ShootMyFriends); Put(packet, 20, fill); Put(packet, 24, columns); Put(packet, 16, shoot); uint rows = _hooks.Current(MissionSettingsField.CombatGumpRows); Put(packet, 28, rows);
        Need(_hooks.Send)(target, packet);
    }
    /// <summary>각 실제 설정 store 뒤 바뀐 같은 패킷의 다음 DWORD를 그때 읽는다.</summary>
    public void Receive(ReadOnlyMemory<byte> packet)
    {
        // payload0..6의 원문 store순서이며 송신의 조회 순서를 수신에 적용하지 않는다.
        for (int index = 0; index < 7; index++) { uint value = Get(packet.Span, 4 + index * 4); Need(_hooks.SetCurrent)((MissionSettingsField)index, value); }
    }
    /// <summary>비Login 기본F도 같은 실제7DWORD에 순서대로 쓰며 이전 store를 되돌리지 않는다.</summary>
    public static void ReceiveCurrent(Memory<uint> current, ReadOnlyMemory<byte> packet)
    {
        if (current.Length != 7) throw new ArgumentException("미션 설정의 실제7DWORD 누락", nameof(current));
        // 현재 입력을 매 store 직전에 읽어 별도 payload 사본을 만들지 않는다.
        for (int index = 0; index < 7; index++) current.Span[index] = Get(packet.Span, 4 + index * 4);
    }
}
