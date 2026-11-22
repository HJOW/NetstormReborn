using Netstorm.Core.Original;

namespace Netstorm.Client; // 생산 UI와 모든 Session이 같은 현재 내용물 검색 전역을 빌린다.

/// <summary>Source ClientGameplayGlobals.finder의 장기 실제 소유 부분이다.</summary>
public sealed partial class ClientGameplayGlobals
{
    /// <summary>최초 SID0 조회부터 AI/생산이 같은 주소로 읽는 실제 검색 전역이다.</summary>
    public ContainedFinderState ProductionFinder { get; } = new();
    /// <summary>Session 전체 대신 같은 장기 권한/TYPE/checking owner만 연결한다.</summary>
    private void BindProductionFinder()
    {
        RawAuthorityState authority = Authority; UIntBox contentType = ContentFormTypeBox; ClientAiDebugGlobals debug = AiDebug;
        ProductionFinder.SharedBoss = () => authority.Boss; ProductionFinder.SharedContainedType = () => contentType.Value; ProductionFinder.CheckingDead = new(() => debug.Checking.Value);
    }
    /// <summary>일반 fresh World에서만 원문 finder 기본값을 같은 주소에 쓰며 prepared 대여/반납은 호출하지 않는다.</summary>
    public void ResetProductionFinderForGeneralWorld()
    {
        ProductionFinder.Boss = true; ProductionFinder.ContainedType = 6; ContentFormTypeBox.Value = 6; ProductionFinder.Assertion = null; BindProductionFinder();
    }
}
