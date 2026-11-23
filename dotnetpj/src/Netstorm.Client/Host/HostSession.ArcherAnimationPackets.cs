using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 같은 실제 전투 공장의 3b/3c 수신과 공통 송신 체인을 현재 Host에 공급한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession.cpp의 00466d30/00466b60과 전투 애니메이션 장치 공급이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 실제 World/Host를 빌리는 전투 애니메이션 공장이다. 공급 전 수신은 실패한다.</summary>
    private RawCampaignArcherBindings? _archerAnimationCombat;
    /// <summary>같은 실제 전투 공장의 현재 전송 체인이다. 반납 뒤는 해제한다.</summary>
    private ProcessFormTransmission? _archerAnimationPacketTransmission;
    /// <summary>후속 3b/3c 수신·송신도 같은 전투 공장을 빌린다. 반납 뒤는 실패한다.</summary>
    public RawCampaignArcherBindings ArcherAnimationCombat => FixedPacketNeed(_archerAnimationCombat);
    /// <summary>현재 실제 Pool을 공유하는 전투 공장을 공급하고 공통 송신 체인에 잇는다. 다른 월드의 공장으로 대체하지 않는다.</summary>
    public void BindArcherAnimationCombat(RawCampaignArcherBindings combat)
    {
        ArgumentNullException.ThrowIfNull(combat);
        if (!ReferenceEquals(combat.Pool, Pool)) throw new ArgumentException("전투 애니메이션의 같은 실제 Pool 필요", nameof(combat));
        if (_playerRecordsReleased || ProcessBindingsReturned() || _playerProcessRuntime is null) throw new InvalidOperationException("전투 애니메이션 공급의 실제 공장/공간 수명 미연결");
        if (_archerAnimationCombat is not null) return;
        _archerAnimationCombat = combat; BindArcherAnimationPacketTransmission();
    }
    /// <summary>같은 실제 전투 공장의 writer를 현재 전송 체인의 바깥에 둔다. 다른 파생은 기존 fallback을 사용한다.</summary>
    private void BindArcherAnimationPacketTransmission()
    {
        RawCampaignArcherBindings combat = FixedPacketNeed(_archerAnimationCombat);
        ProcessFormTransmission other = PlayerProcesses.Transmission()?.Clone() ?? new();
        _archerAnimationPacketTransmission = combat.Transmission(buffer => FixedPacketNeed(_squidPacketEffects?.FormPacket)(buffer), other);
        PlayerProcesses.SetFormTransmit(_archerAnimationPacketTransmission);
    }
    /// <summary>00466d30/00466b60은 저장된 불변 코드값으로 같은 실제 전투 공장에만 전달한다.</summary>
    private void ReceiveArcherAnimationPacket(uint message, ReadOnlyMemory<byte> packet)
    {
        RawCampaignArcherBindings combat = _archerAnimationCombat ?? throw new InvalidOperationException("애니메이션 수신의 실제 전투 본체 미연결");
        combat.ReceiveAnimationPacket(message, packet);
    }
    /// <summary>Kernel/Host 반납 뒤 현재 전투 공장/전송 참조만 해제한다.</summary>
    private void ReleaseArcherAnimationCombatDevices() { _archerAnimationCombat = null; _archerAnimationPacketTransmission = null; }
}
