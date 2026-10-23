using Netstorm.Assets;
using Netstorm.Core.Bridges;

namespace Netstorm.Core.Original;

/// <summary>
/// 자산 루트의 원본 vtable +0x10 삭제 재정의를 공통 삭제 본체 앞에 연결하는 분배기
/// (원본 RawSquidDestroyDispatch, 10.78 주소 004220f0).
/// 지금 옮긴 재정의는 다리 하나이며, 그 외 자산 타입은 공통 본체로 그대로 넘긴다.
/// 판정 순서는 원본 그대로 "가상 표 확인 → 편집기·권한·abstract/매립 단락 → 유효 Graph 번호와 표면 수 →
/// 현재 프레임 코드 → 단단한 프레임·디버그 유지 거부"이며, 거부하면 슬롯·장부·표시·프로세스를 하나도 바꾸지 않는다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 가상 표 값과 프레임 배치가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawSquidDestroyDispatch.cpp (W03 삭제·수명 조립의 자산 분배 조각, 2026-10-23).
/// </summary>
public sealed class RawSquidDestroyDispatch
{
    /// <summary>10.78 다리 클래스의 가상 표 값(원본 kPatchBridgeVtable).</summary>
    private const uint PatchBridgeVtable = 0x005034C8;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 Graph 번호(1바이트)의 위치(10.78).</summary>
    private const int GraphOffset = 30;

    /// <summary>슬롯 안 현재 프레임 번호(4바이트)의 위치(10.78).</summary>
    private const int FrameOffset = 36;

    /// <summary>슬롯 안 추가 비트(abstract 1·매립 8) 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>abstract·매립 비트 마스크. 하나라도 켜지면 다리 재정의를 건너뛴다.</summary>
    private const byte AbstractOrBuried = 9;

    /// <summary>다리 재정의가 프레임을 보는 최소 Graph 표면 수(원본 kBridgeMinimumGraphSurfaces).</summary>
    private const short MinimumGraphSurfaces = 5;

    /// <summary>공통 삭제 본체. 재정의가 통과하면 이 몸체가 실제 삭제를 수행한다.</summary>
    private readonly RawSquidDestroy _baseDestroy;

    /// <summary>발자국·프레임 코드를 읽는 이웃 탐색기. 같은 풀이어야 한다.</summary>
    private readonly RawSquidNeighbors _neighbors;

    /// <summary>지금 실행 상태(편집기·권한·디버그 유지)를 매 호출 읽는 공급자. 중간 변경을 다음 호출부터 반영한다.</summary>
    private readonly Func<BridgeDecayMode> _mode;

    /// <summary>표면 수를 읽는 Graph. 연결하지 않으면 표면 수를 0 으로 본다(원본의 비활성 표와 같다).</summary>
    private readonly RawGraph? _graph;

    /// <summary>
    /// 고정된 실행 상태로 분배기를 만든다. 상태를 실행 중에 바꾸려면 공급자를 받는 생성자를 쓴다.
    /// </summary>
    /// <param name="baseDestroy">공통 삭제 본체.</param>
    /// <param name="neighbors">같은 풀의 이웃·프레임 탐색기.</param>
    /// <param name="mode">현재 실행 상태.</param>
    /// <param name="graph">같은 풀·타입 표의 Graph. 없으면 표면 수 0 으로 판정한다.</param>
    public RawSquidDestroyDispatch(RawSquidDestroy baseDestroy, RawSquidNeighbors neighbors, BridgeDecayMode mode,
        RawGraph? graph = null)
        : this(baseDestroy, neighbors, () => mode, graph)
    {
    }

    /// <summary>
    /// 분배기를 만든다. 삭제 본체·이웃 탐색기·Graph 가 같은 풀을 쓰고, Graph 의 타입 표가 이웃 탐색기의 표와
    /// 같아야 한다. 하나라도 어긋나면 첫 삭제 전에 거부한다.
    /// </summary>
    /// <param name="baseDestroy">공통 삭제 본체.</param>
    /// <param name="neighbors">같은 풀의 이웃·프레임 탐색기.</param>
    /// <param name="mode">실행 상태 공급자. 호출마다 현재 값을 읽는다.</param>
    /// <param name="graph">같은 풀·타입 표의 Graph. 없으면 표면 수 0 으로 판정한다.</param>
    public RawSquidDestroyDispatch(RawSquidDestroy baseDestroy, RawSquidNeighbors neighbors,
        Func<BridgeDecayMode> mode, RawGraph? graph = null)
    {
        ArgumentNullException.ThrowIfNull(baseDestroy);
        ArgumentNullException.ThrowIfNull(neighbors);
        ArgumentNullException.ThrowIfNull(mode);
        if (!ReferenceEquals(baseDestroy.Pool, neighbors.Pool))
        {
            throw new ArgumentException("가상 삭제/프레임 SID 풀이 다릅니다", nameof(neighbors));
        }
        if (graph is not null)
        {
            if (!ReferenceEquals(graph.Pool, baseDestroy.Pool))
            {
                throw new ArgumentException("가상 삭제/Graph SID 풀이 다릅니다", nameof(graph));
            }
            baseDestroy.ValidateGraph(graph);
            graph.ValidateTypes(neighbors.Types);
        }
        _baseDestroy = baseDestroy;
        _neighbors = neighbors;
        _mode = mode;
        _graph = graph;
    }

    /// <summary>이 분배기가 쓰는 슬롯 풀. 호출자가 같은 월드인지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _baseDestroy.Pool;

    /// <summary>표면 수를 읽는 Graph. 연결하지 않았으면 null 이다.</summary>
    public RawGraph? Graph => _graph;

    /// <summary>
    /// 자산 루트 하나를 삭제한다(원본 004220f0). 다리 가상 표이고 재정의 조건이 성립하면 단단한 프레임·디버그 유지에서
    /// 거짓을 돌려주며 아무것도 바꾸지 않는다. 그 밖에는 공통 본체로 넘기고 참을 돌려준다.
    /// </summary>
    /// <param name="sid">지울 자산 루트 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    /// <param name="hooks">공통 본체가 쓸 바깥 효과 콜백 묶음.</param>
    public bool Destroy(int sid, uint flags, SquidDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ReadOnlySpan<byte> raw = _baseDestroy.Pool.Slot(sid);
        // 가상 표가 다리 클래스일 때만 재정의를 검사한다
        if (Dword(raw, 0) == PatchBridgeVtable)
        {
            byte extra = raw[ExtraOffset];
            BridgeDecayMode mode = _mode();
            // 편집기·권한 없음·abstract·매립은 Graph·프레임을 읽기 전에 단락한다
            if (!mode.Editor && mode.Authority && (extra & AbstractOrBuried) == 0)
            {
                short surfaces = 0;
                if (_graph is not null)
                {
                    byte number = raw[GraphOffset];
                    // 원본은 무효 번호의 NULL 을 역참조한다. 호스트에서는 읽기 전에 예외로 표시한다
                    if (number == RawGraph.Invalid)
                    {
                        throw new InvalidOperationException("가상 다리 삭제의 무효 Graph 번호");
                    }
                    if (number >= _graph.Records.Count)
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), number, "가상 다리 삭제의 Graph 표 범위 오류");
                    }
                    surfaces = _graph.Records[number].Surfaces;
                }
                // 표면 수가 경계 이상일 때만 현재 프레임을 읽고 거부 여부를 정한다
                if (surfaces >= MinimumGraphSurfaces)
                {
                    int frame = (int)UInt(raw, FrameOffset);
                    IReadOnlyList<FrameCode> codes = _neighbors.Frames(raw[TypeOffset]);
                    if (frame < 0 || frame >= codes.Count)
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), frame, "가상 다리 삭제 프레임 범위 오류");
                    }
                    if (!BridgeDecayRules.DestroyProceeds(mode, extra, surfaces, codes[frame].Flags))
                    {
                        return false;
                    }
                }
            }
        }
        _baseDestroy.Destroy(sid, flags, hooks);
        return true;
    }

    /// <summary>슬롯의 4바이트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint UInt(ReadOnlySpan<byte> raw, int offset)
    {
        // 호스트 바이트 순서에 기대지 않고 낮은 바이트부터 합친다
        return (uint)(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
    }

    /// <summary>슬롯 앞 4바이트(가상 표)를 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Dword(ReadOnlySpan<byte> raw, int offset) => UInt(raw, offset);
}
