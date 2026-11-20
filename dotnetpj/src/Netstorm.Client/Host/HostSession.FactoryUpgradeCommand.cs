using Netstorm.Core.Original;

// 같은 시설·SP·현재 시각·공유 정기를 업그레이드 요청/수신에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>실제 업그레이드 요청·수신 장치와 AI 요청의 단일 세션 진입점이다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 미션에서 입력·AI·수신이 함께 사용하는 요청 본체다.</summary>
    private RawFactoryUpgradeCommand? _worldFactoryUpgradeCommand;
    /// <summary>다음 실제 부착이 소비할 확보된 SharedRegular 객체다.</summary>
    private RegularProcess? _factoryUpgradePending;
    /// <summary>장치 조립 전이나 반납 뒤 요청을 명시 오류로 보고한다.</summary>
    public RawFactoryUpgradeCommand WorldFactoryUpgradeCommand => _worldFactoryUpgradeCommand
        ?? throw new InvalidOperationException("호스트 시설 업그레이드 요청 미연결");

    /// <summary>외부 송신·방송·로그만 받아 같은 현재 풀/TYPE/SP/시각/Host에 요청 본체를 연결한다.</summary>
    /// <remarks>근거: cpppj/src/client/RawCampaignSession.cpp의 FactoryUpgradeCommandHooks 조립이다.
    /// 채널9 실제 운반과 단독 미션 방송 정책은 호출자가 설치해야 한다.</remarks>
    public RawFactoryUpgradeCommand BuildFactoryUpgradeCommandWorldDevices(FactoryUpgradeCommandHooks devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        if (_worldFactoryUpgradeCommand is not null) throw new InvalidOperationException("호스트 시설 업그레이드 요청 중복 연결");
        if (devices.Send is null || devices.Broadcast is null || devices.Log is null)
            throw new ArgumentException("호스트 시설 업그레이드 실제 전송/로그 장치 누락", nameof(devices));
        _ = WorldFactoryUpgrade;
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("호스트 시설 업그레이드 현재 TYPE 미연결");
        RawSquidReward hp = _fortHp ?? throw new InvalidOperationException("호스트 시설 업그레이드 실제 SP/비용 미연결");
        var command = new RawFactoryUpgradeCommand(Pool, types, new()
        {
            Boss = () => Gameplay.Authority.Boss,
            Checking = new RawCheckingFlag(() => AiChecking.Read()),
            Now = () => ProcessTime.Value,
        }, new()
        {
            Cost = hp.TypeCostUnits, AddSp = hp.AddSp,
            ReserveShared = ReserveFactoryUpgradeShared,
            AddShared = AddFactoryUpgradeShared,
            Send = devices.Send, Broadcast = devices.Broadcast, Log = devices.Log,
            Assertion = text => throw new InvalidOperationException($"시설 업그레이드 원본 진단: {text}"),
        });
        _worldFactoryUpgradeCommand = command;
        return command;
    }

    /// <summary>원문 할당 위치에서 실제 메모리를 확보하며 실패할 때만 거짓이다.</summary>
    private bool ReserveFactoryUpgradeShared()
    {
        if (_factoryUpgradePending is not null) throw new InvalidOperationException("시설 업그레이드 SharedRegular 중복 예약");
        try { _factoryUpgradePending = new RegularProcess(0, 0); return true; }
        catch (OutOfMemoryException) { return false; }
    }

    /// <summary>확보한 같은 객체를 초기화/부착하고 부착 뒤의 현재 시각으로 사건을 재설정한다.</summary>
    private void AddFactoryUpgradeShared(int parent, uint ev, float delay)
    {
        RegularProcess memory = _factoryUpgradePending ?? throw new InvalidOperationException("시설 업그레이드 SharedRegular 예약 누락");
        _factoryUpgradePending = null;
        memory.InitializeReserved(ev, delay);
        RegularProcess attached = PlayerProcesses.AttachRegular(memory, parent, true)
            ?? throw new InvalidOperationException("시설 업그레이드 SharedRegular 실제 부착 실패");
        attached.ResetTask(ev, ProcessTime.Value);
    }

    /// <summary>프로세스 반납 뒤 외부 장치를 잡은 요청 본체와 미소비 예약 메모리를 놓는다.</summary>
    private void ReleaseFactoryUpgradeCommandWorld()
    {
        _factoryUpgradePending = null;
        _worldFactoryUpgradeCommand = null;
    }
}
