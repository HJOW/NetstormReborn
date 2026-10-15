using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>공통 postPop 통계(원본 Totalmade 장부)에 객체를 등록하고 삭제 때 내리는 세션 연결.</summary>
public sealed partial class BattleSession
{
    /// <summary>
    /// 완성된 일반 객체를 공통 postPop(004b0d30)의 통계에 한 번 등록한다: 로컬 소유자면 현재 수와 누적 제작 수를,
    /// 모든 소유자면 전체 현재 수를 올린다. 이미 등록한 객체는 다시 올리지 않는다.
    /// 건물은 완공할 때, 유닛은 자원 도착 후 실체화를 마칠 때, 개발 배치는 놓는 즉시 호출한다.
    /// </summary>
    /// <param name="entity">장부에 등록할 완성 객체.</param>
    /// <remarks>2026-10-15 감사 D14: 이름 기반 PlayerState 누적 표를 원본 세 장부(<see cref="TotalMade"/>)로 교체.</remarks>
    private void RecordPopped(GameEntity entity)
    {
        if (entity.LedgerCounted) return;
        entity.LedgerCounted = true;
        Ledger.RecordPopped(entity.Type.RuntimeIndex, entity.Owner == HumanPlayer);
    }

    /// <summary>
    /// 놓은 다리 칸마다 다리 타입을 공통 postPop 통계에 등록한다(원본은 칸마다 다리 오브젝트를 만들어 첫 등록 때 센다).
    /// 칸은 참조로 기억해 무너질 때 한 번만 내린다.
    /// </summary>
    /// <param name="cells">방금 놓은 다리 칸.</param>
    /// <param name="owner">칸의 소유자.</param>
    private void RecordBridgeCellsPopped(IEnumerable<BridgeCellState> cells, int owner)
    {
        TypeInfo? bridgeType = _types.Find(ProductionDeck.BridgeType);
        if (bridgeType == null) return;
        // 놓은 칸의 수만큼 현재 수와 누적 제작 수를 올린다.
        foreach (BridgeCellState cell in cells)
        {
            if (!_ledgerBridgeCells.Add(cell)) continue;
            Ledger.RecordPopped(bridgeType.RuntimeIndex, owner == HumanPlayer);
        }
    }

    /// <summary>
    /// 삭제되는 객체가 장부에 등록돼 있으면 현재 수만 내린다(삭제 준비 PreDestroy). 누적 제작 수는 줄이지 않는다.
    /// 맵에 처음부터 있던 객체나 공사장처럼 등록한 적 없는 객체는 건드리지 않는다.
    /// </summary>
    /// <param name="entity">삭제되는 객체.</param>
    private void ReleaseLedgerEntity(GameEntity entity)
    {
        if (!entity.LedgerCounted) return;
        entity.LedgerCounted = false;
        Ledger.RecordDestroyed(entity.Type.RuntimeIndex, entity.Owner == HumanPlayer);
    }

    /// <summary>무너지거나 약화로 사라진 다리 칸 가운데 장부에 등록한 칸만 현재 수를 한 번 내린다.</summary>
    /// <param name="cells">사라진 다리 칸.</param>
    private void ReleaseLedgerBridgeCells(IEnumerable<BridgeCellState> cells)
    {
        TypeInfo? bridgeType = _types.Find(ProductionDeck.BridgeType);
        if (bridgeType == null) return;
        // 등록한 칸만 내린다(저장된 다리 칸은 등록하지 않았다).
        foreach (BridgeCellState cell in cells)
            if (_ledgerBridgeCells.Remove(cell)) Ledger.RecordDestroyed(bridgeType.RuntimeIndex, cell.Owner == HumanPlayer);
    }

    /// <summary>세 장부와 허용 표의 0이 아닌 칸을 번호 순서대로 검사합에 섞는다(다음 단계 판정 결과를 바꾸는 상태다).</summary>
    /// <param name="hash">검사합.</param>
    private void AddLedgerChecksum(Fnv1a hash)
    {
        uint[][] tables = [Ledger.LocalCounts, Ledger.LocalSecondaryCounts, Ledger.GlobalCounts, Ledger.Restrictions];
        // 표마다 0이 아닌 칸의 번호와 값을 섞는다.
        foreach (uint[] table in tables)
        {
            hash.Add(-1);
            for (int type = 0; type < table.Length; type++)
            {
                if (table[type] == 0) continue;
                hash.Add(type);
                hash.Add(table[type]);
            }
        }
    }
}
