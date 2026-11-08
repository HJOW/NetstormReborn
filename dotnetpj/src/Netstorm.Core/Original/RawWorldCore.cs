// 비표면 runtime이 참조하는 실제 월드 자료다. 원본 배열·풀·해시·장부의 사본을 만들지 않는다.
namespace Netstorm.Core.Original;

// 비표면 runtime이 참조하는 실제 월드 자료를 담는 네임스페이스다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.

// 비표면 runtime이 참조하는 실제 월드 자료다. span 대신 같은 수명의 직접 참조를 빌리며 풀·해시·장부의 사본을 만들지 않는다. 이 보기는 월드보다 오래 살 수 없다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.
public sealed class RawWorldCore
{
    /// <summary>같은 월드의 실제 월드 자료 보기를 만든다. 모든 참조는 호출자 월드가 소유하며 이 보기보다 오래 살아야 한다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    /// <param name="pool">객체 슬롯의 생성·조회·반납에 사용하는 SID 풀이다.</param>
    /// <param name="hash">좌표별 객체 후보를 찾는 공간 해시다.</param>
    /// <param name="spots">칸별 점유·통행 비트를 읽는 공간 지도다.</param>
    /// <param name="types">타입 번호로 속성·플래그를 조회하는 타입 표다.</param>
    /// <param name="frames">타입별 프레임 자료다.</param>
    /// <param name="shapes">프레임의 크기·그림 데이터를 조회하는 셰이프 자료다.</param>
    /// <param name="bookkeeping">객체 종류·소유자별 등록 목록과 개수를 관리하는 장부다.</param>
    /// <param name="ownerMode">판본·서버·로딩 상태에 따른 소유권 처리 정책이다.</param>
    /// <param name="factory">타입에 맞는 객체를 생성하는 처리기다.</param>
    /// <param name="owner">소유 플레이어 변경을 객체 필드와 장부에 반영하는 처리기다.</param>
    /// <param name="frame">프레임 변경을 물리 프레임·통지·갱신 요청에 반영하는 처리기다.</param>
    /// <param name="pop">객체를 공간 지도에 등록하는 Pop 처리기다.</param>
    /// <param name="unpop">객체의 공간 등록을 해제하는 Unpop 처리기다.</param>
    /// <param name="postPop">객체 Pop 이후 목록·공간 후처리를 수행하는 처리기다.</param>
    /// <param name="display">객체·프레임에서 실제 화면 출력 요청을 만드는 처리기다.</param>
    /// <param name="building">일반 건물의 공간 등록·해제를 반영하는 처리기다.</param>
    /// <param name="graph">서로 연결된 표면의 Graph 소속을 관리하는 처리기다.</param>
    /// <param name="neighbors">발자국에 인접한 표면·객체를 조회하는 검색기다.</param>
    /// <param name="destroy">객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기다.</param>
    /// <param name="virtualDestroy">타입별 가상 삭제 경로를 선택하는 분배기다.</param>
    /// <param name="surfaces">표면 객체의 공간·연결·수명 처리 자료다.</param>
    /// <param name="bridgeEvents">다리의 연결·붕괴 변화에 따른 이벤트 처리기다.</param>
    /// <param name="forms">객체에 부착된 Form의 생성·통지·수명 처리기다.</param>
    /// <param name="dependencies">같은 풀·삭제 깊이·ProcessForm 통지를 사용하는 실제 감시 Form 쌍이다.</param>
    /// <param name="processes">공유 커널에 연결한 객체 프로세스 호스트다.</param>
    /// <param name="processState">프로세스 실행·부착·이벤트 분배에 사용하는 상태다.</param>
    /// <param name="kernel">프로세스 등록과 프레임 실행을 담당하는 커널이다.</param>
    /// <param name="bridgeState">같은 월드의 실제 표면 소유자 분배·연결이 읽는 현재 전역이다.</param>
    public RawWorldCore(
        RawSidPool pool,
        RawSquidHash hash,
        byte[] spots,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeFrames> frames,
        IReadOnlyList<SquidDisplayShape> shapes,
        RawPostPopState bookkeeping,
        SquidOwnerMode ownerMode,
        RawSquidFactory factory,
        SquidOwner owner,
        SquidFrame frame,
        RawPop pop,
        RawUnpop unpop,
        RawPostPop postPop,
        SquidDisplay display,
        RawBuildingSpace building,
        RawGraph graph,
        RawSquidNeighbors neighbors,
        RawSquidDestroy destroy,
        RawSquidDestroyDispatch virtualDestroy,
        RawSurfaceLifecycle surfaces,
        RawBridgeEvents bridgeEvents,
        RawSquidFormDispatch forms,
        RawDependFormHost dependencies,
        SquidProcessHost processes,
        SquidProcessState processState,
        Kernel kernel,
        BridgeConnectState bridgeState)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(shapes);
        ArgumentNullException.ThrowIfNull(bookkeeping);
        ArgumentNullException.ThrowIfNull(ownerMode);
        ArgumentNullException.ThrowIfNull(factory);
        ArgumentNullException.ThrowIfNull(owner);
        ArgumentNullException.ThrowIfNull(frame);
        ArgumentNullException.ThrowIfNull(pop);
        ArgumentNullException.ThrowIfNull(unpop);
        ArgumentNullException.ThrowIfNull(postPop);
        ArgumentNullException.ThrowIfNull(display);
        ArgumentNullException.ThrowIfNull(building);
        ArgumentNullException.ThrowIfNull(graph);
        ArgumentNullException.ThrowIfNull(neighbors);
        ArgumentNullException.ThrowIfNull(destroy);
        ArgumentNullException.ThrowIfNull(virtualDestroy);
        ArgumentNullException.ThrowIfNull(surfaces);
        ArgumentNullException.ThrowIfNull(bridgeEvents);
        ArgumentNullException.ThrowIfNull(forms);
        ArgumentNullException.ThrowIfNull(dependencies);
        ArgumentNullException.ThrowIfNull(processes);
        ArgumentNullException.ThrowIfNull(processState);
        ArgumentNullException.ThrowIfNull(kernel);
        ArgumentNullException.ThrowIfNull(bridgeState);
        Pool = pool;
        Hash = hash;
        Spots = spots;
        Types = types;
        Frames = frames;
        Shapes = shapes;
        Bookkeeping = bookkeeping;
        OwnerMode = ownerMode;
        Factory = factory;
        Owner = owner;
        Frame = frame;
        Pop = pop;
        Unpop = unpop;
        PostPop = postPop;
        Display = display;
        Building = building;
        Graph = graph;
        Neighbors = neighbors;
        Destroy = destroy;
        VirtualDestroy = virtualDestroy;
        Surfaces = surfaces;
        BridgeEvents = bridgeEvents;
        Forms = forms;
        Dependencies = dependencies;
        Processes = processes;
        ProcessState = processState;
        Kernel = kernel;
        BridgeState = bridgeState;
    }

    /// <summary>객체 슬롯의 생성·조회·반납에 사용하는 SID 풀이다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawSidPool Pool { get; }

    /// <summary>좌표별 객체 후보를 찾는 공간 해시다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawSquidHash Hash { get; }

    /// <summary>칸별 점유·통행 비트를 읽는 공간 지도다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public byte[] Spots { get; }

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public IReadOnlyList<OriginalTypeRecord> Types { get; }

    /// <summary>타입별 프레임 자료다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public IReadOnlyList<RiftTypeFrames> Frames { get; }

    /// <summary>프레임의 크기·그림 데이터를 조회하는 셰이프 자료다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public IReadOnlyList<SquidDisplayShape> Shapes { get; }

    /// <summary>객체 종류·소유자별 등록 목록과 개수를 관리하는 장부다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawPostPopState Bookkeeping { get; }

    /// <summary>판본·서버·로딩 상태에 따른 소유권 처리 정책이다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public SquidOwnerMode OwnerMode { get; }

    /// <summary>타입에 맞는 객체를 생성하는 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawSquidFactory Factory { get; }

    /// <summary>소유 플레이어 변경을 객체 필드와 장부에 반영하는 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public SquidOwner Owner { get; }

    /// <summary>프레임 변경을 물리 프레임·통지·갱신 요청에 반영하는 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public SquidFrame Frame { get; }

    /// <summary>객체를 공간 지도에 등록하는 Pop 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawPop Pop { get; }

    /// <summary>객체의 공간 등록을 해제하는 Unpop 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawUnpop Unpop { get; }

    /// <summary>객체 Pop 이후 목록·공간 후처리를 수행하는 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawPostPop PostPop { get; }

    /// <summary>객체·프레임에서 실제 화면 출력 요청을 만드는 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public SquidDisplay Display { get; }

    /// <summary>일반 건물의 공간 등록·해제를 반영하는 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawBuildingSpace Building { get; }

    /// <summary>서로 연결된 표면의 Graph 소속을 관리하는 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawGraph Graph { get; }

    /// <summary>발자국에 인접한 표면·객체를 조회하는 검색기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawSquidNeighbors Neighbors { get; }

    /// <summary>객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawSquidDestroy Destroy { get; }

    /// <summary>타입별 가상 삭제 경로를 선택하는 분배기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawSquidDestroyDispatch VirtualDestroy { get; }

    /// <summary>표면 객체의 공간·연결·수명 처리 자료다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawSurfaceLifecycle Surfaces { get; }

    /// <summary>다리의 연결·붕괴 변화에 따른 이벤트 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawBridgeEvents BridgeEvents { get; }

    /// <summary>객체에 부착된 Form의 생성·통지·수명 처리기다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawSquidFormDispatch Forms { get; }

    /// <summary>같은 풀·삭제 깊이·ProcessForm 통지를 사용하는 실제 감시 Form 쌍이다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public RawDependFormHost Dependencies { get; }

    /// <summary>공유 커널에 연결한 객체 프로세스 호스트다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public SquidProcessHost Processes { get; }

    /// <summary>프로세스 실행·부착·이벤트 분배에 사용하는 상태다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public SquidProcessState ProcessState { get; }

    /// <summary>프로세스 등록과 프레임 실행을 담당하는 커널이다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public Kernel Kernel { get; }

    /// <summary>같은 월드의 실제 표면 소유자 분배·연결이 읽는 현재 전역이다. 2026-11-08 F2d 월드 본체 Core 이식에서 추가됐다.</summary>
    public BridgeConnectState BridgeState { get; }
}
