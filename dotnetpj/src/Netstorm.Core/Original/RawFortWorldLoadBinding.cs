// 기존 정상지형·Money·Graph 선택을 한 실제 월드 부모의 필수 경계에 연결한다.
namespace Netstorm.Core.Original;

// Fort 적재 바인딩 모듈을 담는 네임스페이스다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.

// 자식 적재·지형 생성·Money·Graph 재구성을 같은 풀·몸체에 묶는 바인딩이다. 정상 바인딩 본체 조립은 후속 단위에서 잇는다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.
public static class RawFortWorldLoadBinding
{
    /// <summary>누락 효과나 서로 다른 풀을 부모 실행 전에 거부하며 모든 선택 경계의 수명을 호출자가 보장한다. 2026-11-07 F2c Fort 적재 바인딩 이식에서 추가됐다.</summary>
    /// <param name="normalPool">정상 지형 바인딩이 쓰는 같은 풀이다.</param>
    /// <param name="corePool">월드가 쓰는 같은 풀이다.</param>
    /// <param name="graph">같은 월드의 실제 그래프다.</param>
    /// <param name="objects">객체 적재 몸체다.</param>
    /// <param name="money">Money 적재 몸체다.</param>
    /// <param name="meta">배지·기술 적재 몸체다.</param>
    /// <param name="moneyStream">현재 Template·owner의 Money cursor 선택이다.</param>
    /// <param name="selectMeta">각 child의 현재 Template 동기화 콜백이다.</param>
    /// <param name="metaStream">실제 섹션을 선택해 현재 cursor를 반환하는 콜백이다.</param>
    /// <param name="generate">바인딩 본체의 지형 생성을 대신하는 대리자다. 월드 조립 때 고정한다.</param>
    /// <param name="hooks">기반 훅 묶음이다.</param>
    /// <returns>자식·생성·Money·Graph가 연결된 새 훅 묶음이다.</returns>
    public static FortWorldLoadHooks MakeFortWorldLoadBinding(
        RawSidPool normalPool,
        RawSidPool corePool,
        RawGraph graph,
        RawFortObjectLoad objects,
        RawFortMoneyLoad money,
        RawFortMetaLoad meta,
        Func<FortTemplate, uint, FortMoneyCursor>? moneyStream,
        Action<FortTemplate, uint, bool>? selectMeta,
        Func<FortTemplate, bool, FortMetaStream>? metaStream,
        Action<RawIslandBuilderState, int> generate,
        FortWorldLoadHooks hooks)
    {
        if (!ReferenceEquals(normalPool, corePool) || moneyStream is null)
        {
            throw new ArgumentException("Fort 부모의 실제 정상 월드/현재 Money 선택 누락");
        }

        ArgumentNullException.ThrowIfNull(graph);
        ArgumentNullException.ThrowIfNull(money);
        ArgumentNullException.ThrowIfNull(generate);
        FortWorldLoadHooks result = RawFortWorldLoad.MakeFortWorldChildrenHooks(corePool, objects, meta, selectMeta, metaStream, hooks);
        // 호출자가 바인딩 본체의 GenerateOriginal을 대리로 넘긴다. 월드 조립 때 고정한다
        result.Generate = generate;
        // 실제 Money 선택은 wrapper의 깊이 증가/진단 뒤에 수행한다. C++ 인수 평가로 먼저 선택하지 않는다
        result.Money = (fort, owner) => money.LoadSelected(() => moneyStream(fort, owner));
        result.RebuildGraph = resetAll => graph.Rebuild(resetAll);
        return result;
    }
}
