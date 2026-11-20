using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// Native State의 실제 자식 정상 복귀와 현재 Session 반납을 연결한다.
namespace Netstorm.Client.Host;

/// <summary>AI·Player 링크·단일 Kernel Clear 증빙을 같은 실제 모듈에서만 소비한다.</summary>
public sealed partial class HostSession
{
    /// <summary>실제 장기 Host의 반납 뒤에도 Regular 잔류가 빌리는 같은 Kernel을 유지한다.</summary>
    private RawProcessHostStorage.Returned? _returnedPlayerProcesses;

    /// <summary>현재 실제 Deck의 Player 링크 정리를 실행하고 정상 복귀 증빙만 발급한다.</summary>
    public RawProductionDeck.PlayerLinksPass ClearPlayerLinksForState()
    {
        EnsureRuntimeCanRelease();
        return WorldProductionDeck.CleanupPlayerLinksForState(() => PlayerRecords);
    }

    /// <summary>같은 실제 Deck에서 발급한 증빙을 소비하며 다른 모듈의 증빙을 거부한다.</summary>
    public void ReturnPlayerLinksForState(RawProductionDeck.PlayerLinksPass pass)
    {
        EnsureRuntimeCanRelease();
        WorldProductionDeck.ReturnPlayerLinksForState(pass);
    }

    /// <summary>빈 brain 추정 대신 현재 실제 참여 모듈의 owner별 반납 증빙을 조회한다.</summary>
    public bool BrainReturnedForState(uint owner) => _aiParticipation.BrainReturnedForState(owner);

    /// <summary>실제 단일 Clear가 정상 복귀한 경우에만 같은 현재 Host의 증빙을 발급한다.</summary>
    public RawProcessHostStorage.ClearPass ClearProcessesForState()
    {
        EnsureRuntimeCanRelease();
        return (_playerProcessUse ?? throw new InvalidOperationException("장기 Host의 현재 대여 없는 State Clear")).ClearForState();
    }

    /// <summary>같은 증빙과 Regular 잔류 관문 뒤에만 현재 처리기를 반납하고 건설 장치를 놓는다.</summary>
    public void ReturnProcessBindingsForState(RawProcessHostStorage.ClearPass pass)
    {
        ArgumentNullException.ThrowIfNull(pass);
        EnsureRuntimeCanRelease();
        if (_playerProcessUse is null || _returnedPlayerProcesses is not null)
            throw new InvalidOperationException("State Host 반납의 단일 실제 대여 필요");
        _returnedPlayerProcesses = _playerProcessUse.ReturnForState(pass);
        _playerProcessUse = null;
        ReleaseConstructionWorld();
    }

    /// <summary>강제 Drain이나 빈 성공값과 구별되는 실제 처리기 반납 결과만 조회한다.</summary>
    public bool ProcessBindingsReturned() => _returnedPlayerProcesses is not null;

    /// <summary>모든 실제 Player 소비자가 명시 반납됐는지 조회하며 값을 초기화하지 않는다.</summary>
    public bool PlayerRecordViewsReleased => _playerRecordsReleased;

    /// <summary>known0/Core0과 실제 자식 증빙 뒤에만 현재 Player 고정뷰 소비자를 반납한다.</summary>
    public void ReleasePlayerRecordViewsForState()
    {
        EnsureRuntimeCanRelease();
        if (MissionPlayer.PlayerKnown != 0 || Gameplay.CoreData.Active != 0 || !Gameplay.CoreData.Current.IsEmpty)
            throw new InvalidOperationException("State Runtime 반납 전 실제 known0/Core0 선택 필요");
        if (!WorldProductionDeck.PlayerLinksReturnedForState)
            throw new InvalidOperationException("State Runtime 반납 전 실제 Player 링크 증빙 필요");
        // 원문 여덟 owner 각각의 정상 반납을 검사하며 개수나 brain0으로 대신하지 않는다.
        for (uint owner = 1; owner < PlayerParticipation.PlayerCount; owner++)
            if (!BrainReturnedForState(owner)) throw new InvalidOperationException("State Runtime 반납 전 실제 여덟 AI 증빙 필요");
        if (!ProcessBindingsReturned()) throw new InvalidOperationException("State Runtime 반납 전 실제 단일 Kernel Clear 반납 필요");
        ReleasePlayerRecordViews();
    }
}
