using Netstorm.Assets;
using Netstorm.Core.Bridges;

// 같은 장기 물리/장부를 빌리는 공통 실행기를 한 번만 구성한다.
namespace Netstorm.Core.Original;

/// <summary>World 반납 뒤에도 같은 Graph/이웃/Owner 실행기와 현재 TYPE를 유지한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 기록/stack/현재 TYPE/공통 Graph 모드를 읽는 유일한 실행기다.</summary>
    private RawGraph? _graphExecutor;
    /// <summary>같은 Pool/Hash/spot의 현재 TYPE와 코드 사본을 사용하는 유일한 이웃 실행기다.</summary>
    private RawSquidNeighbors? _neighborsExecutor;
    /// <summary>같은 현재 모드와 장부/TYPE를 사용하는 유일한 공통 owner 실행기다.</summary>
    private SquidOwner? _ownerExecutor;

    /// <summary>Graph 물리를 초기화하지 않고 현재 진단·로그·시계와 실제 공통 상태만 연결한다.</summary>
    public RawGraph GraphExecutor()
    {
        if (_graphExecutor is null)
        {
            // Storage가 최초 복사한 코드 표를 값 변환한다. 현재 frames 교체로 초기 Graph 코드를 바꾸지 않는다.
            IReadOnlyList<IReadOnlyList<FrameCode>> codes = Codes.Select(row => (IReadOnlyList<FrameCode>)row.Select(code =>
                new FrameCode((char)code.Side, (char)code.Variant, code.Number, (FrameCodeFlags)code.Flags)).ToArray()).ToArray();
            var graph = new RawGraph(Pool, Hash, Spots, Types, Graph, codes, GraphRecovery.FullPool);
            if (_edition == OriginalEdition.Patch1078) graph.BindCurrentTypes(Types);
            graph.BindDiagnostics(new() { Checking = new(() => Checking().Read()), Assertion = AssertDisplay });
            graph.BindTrace(TraceGraph); graph.BindClock(ReadGraphClock);
            graph.BindDetachState(new RawCheckingFlag(() => PostPopState.GraphsEnabled ? 1u : 0u), new RawCheckingFlag(() => DeletionState.RebuildGraph ? 1u : 0u));
            _graphExecutor = graph;
        }
        return _graphExecutor;
    }
    /// <summary>추가 Pool/Hash/spot이나 World 대여 없이 같은 이웃 조회기를 처음 한 번 구성한다.</summary>
    public RawSquidNeighbors NeighborsExecutor()
    {
        if (_neighborsExecutor is null)
        {
            var neighbors = new RawSquidNeighbors(Pool, Hash, Spots, Types, new RiftFrameCodeTables(Frames));
            if (_edition == OriginalEdition.Patch1078) neighbors.BindCurrentTypes(Types);
            _neighborsExecutor = neighbors;
        }
        return _neighborsExecutor;
    }
    /// <summary>다음 World에서도 같은 현재 모드/장부/TYPE를 쓰는 공통 owner를 구성한다.</summary>
    public SquidOwner OwnerExecutor()
    {
        if (_ownerExecutor is null)
        {
            var owner = new SquidOwner(Pool, Types, PostPopState, OwnerMode);
            owner.BindDiagnostics(new() { Types = Types, Assertion = AssertDisplay }); _ownerExecutor = owner;
        }
        return _ownerExecutor;
    }
}
