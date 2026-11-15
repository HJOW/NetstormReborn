// 같은 장기 장부의 일반 fresh 대입을 물리 주소 교체 없이 실행한다.
namespace Netstorm.Core.Original;

/// <summary>현재 장부가 빌린 전역/목록/깊이의 정체성을 유지한다.</summary>
public sealed partial class RawPostPopState
{
    /// <summary>일반 World의 새 값 의미만 복원하고 세 공유 통계표의 주소와 정적 목록 자체는 유지한다.</summary>
    internal void ResetForGeneralWorld(IntBox cost, RawPostPopList providers, RawPostPopList factories, BoolBox? suppressed)
    {
        Suppressed = false; GraphsEnabled = false; PendingPlacement = false; LocalOwner = 0; InvalidGraph = 254;
        Array.Clear(AiAttached); TotalCost = 0; ProductionDirty = 0; Depth = 0;
        SharedTotalCost = cost; SharedProviders = providers; SharedFactories = factories; SharedSuppressed = suppressed;
        ClearOwnedList(_providers); ClearOwnedList(_factories);
        // owner별 값형 목록은 새 상태 대입처럼 현재 연결/독립 용량을 반납하며 목록 객체 주소는 유지한다.
        foreach (RawPostPopList list in OwnerFactories) ClearOwnedList(list);
    }

    /// <summary>일반 fresh에서 독립/짧은 목록의 바인딩과 벡터만 놓고 장기 정적 목록은 건드리지 않는다.</summary>
    private static void ClearOwnedList(RawPostPopList list) { list.Current = new(); list.Entries = []; list.Count = 0; }
}
