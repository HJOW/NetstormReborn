// 같은 풀·공간 지도에서 표면 객체의 생성·등록·수명·표시를 관리하는 월드의 껍데기를 공유한다.
using Netstorm.Core.Bridges;
using Netstorm.Core.Display;
using Netstorm.Core.Simulation;
using System.Globalization;
using System.Text;

namespace Netstorm.Core.Original;

// 서피스 월드 모듈을 담는 네임스페이스다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.

// 판본·타입·지형 종류·물리 저장소의 같은 소유 정체성만 두고 실제 조립은 잇지 않는 월드 껍데기다. 적재·표시·삭제·프로세스 본체 조립은 후속 단위에서 잇는다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.
public sealed class RawSurfaceWorld
{
    /// <summary>월드가 따르는 원본 판본이다. 슬롯 오프셋이 다른 모듈이 이 값으로 갈라진다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    private readonly OriginalEdition _edition;

    /// <summary>호출자가 소유하는 같은 전역 TYPE 표다. 조립 때 실제 물리로 옮긴다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>호출자가 소유하는 같은 장기 프레임 표다. 표면·이웃 조회가 프레임 자료를 읽을 때 쓴다. 비우면 빈 목록을 둔다. 2026-11-07 F2d 월드 본체 프레임 소유 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>초기 지면에 쓰는 지형 종류 번호다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    private readonly uint _terrainType;

    /// <summary>같은 물리 저장소다. 없으면 이 월드가 새로 만든다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    private readonly RawSurfaceStorage _storage;

    /// <summary>후속 갱신이 필요한 상태 변경이 남았는지다. 원본 Impl.changed에 대응하며 초기값은 true다. 표시 억제와 무관하게 TakeChanged로 한 번만 소비한다. 2026-11-07 F2d 월드 본체 TakeChanged 이식에서 추가됐다.</summary>
    private bool _changed = true;

    /// <summary>중첩 로딩의 표면/사제/자산 후처리가 같은 값을 읽는 로딩 상태다. 원본 Impl.loadState에 대응하며 초기 깊이는 1이다. 런타임 전달과 저장 게임 공유 연결은 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 SetLoadingDepth 이식에서 추가됐다.</summary>
    private readonly IslandPostPopState _loadState = new() { LoadingDepth = 1u };

    /// <summary>붕괴 처리가 읽는 원본 실행 상태 전역이다. 원본 Impl.mode에 대응하며 초기값은 싱글 플레이(로컬이 서버) 전투 화면이다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    private BridgeDecayMode _mode = BridgeDecayMode.SinglePlayer;

    /// <summary>런타임 경계를 설치했는지다. 원본 Impl.runtimeBound에 대응한다. 첫 객체 생성 전 한 번만 연결하며 BeginLoad·FinishLoad·BindRuntime 단위에서 갱신한다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    private bool _runtimeBound = false;

    /// <summary>적재 작업 중인지다. 원본 Impl.loading에 대응한다. BeginLoad·FinishLoad 단위에서 갱신한다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    private bool _loading = false;

    /// <summary>현재 자료의 적재가 끝났는지다. 원본 Impl.loaded에 대응한다. FinishLoad 단위에서 갱신한다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    private bool _loaded = false;

    /// <summary>프레임의 게임 시각과 벽 시각 및 시간차다. 원본 Impl.time에 대응하며 초기값은 0이다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    private FrameTime _time;

    /// <summary>프로세스 실행·부착·이벤트 분배에 쓰는 같은 저장소 상태다. 원본 Impl.processState에 대응하며 호출자 저장소보다 오래 살 수 없다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    private readonly SquidProcessState _processState;

    /// <summary>객체 종류·소유자별 등록 목록과 개수를 관리하는 제어 장부다. 원본 Impl.bookkeeping에 대응한다. 공유 집계 연결은 세션 조립에서 잇는다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    private readonly RawPostPopState _bookkeeping = new();

    /// <summary>서로 연결된 표면의 Graph 소속을 관리하는 처리기다. 원본 Impl.graph에 대응한다. 188개 TYPE가 아니면 비워 두며 CD 연결은 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 그래프 적재 이식에서 추가됐다.</summary>
    private readonly RawGraph? _graph;

    /// <summary>자산 뿌리의 공간 해제기다. 원본 Impl.unpop에 대응한다. 188개 TYPE가 아니면 비워 두며 CD 연결은 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    /// <summary>타입에 맞는 객체를 생성하는 처리기다. 원본 Impl.factory에 대응하며 188개 TYPE가 아니면 비워 둔다. 진단·분리·전송 묶음은 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 생성 연결 이식에서 추가됐다.</summary>
    private readonly RawSquidFactory? _factory;

    /// <summary>타입·프레임 자료와 카메라에서 객체 출력 요청을 만드는 처리기다. 원본 Impl.display에 대응하며 같은 수의 셰이프 표가 없으면 비워 둔다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    private readonly SquidDisplay? _display;

    /// <summary>해상도 변경으로 Renderer가 교체되어도 현재 장치에 전달하는 표시 콜백이다. 원본 Impl.displayHooks에 대응한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    private SurfaceDisplayHooks _displayHooks = new();

    /// <summary>현재 월드의 실제 메타 표시 갱신 장치다. 원본 Impl.metaRefresh에 대응하며 설치 전에는 비어 있다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    private Action<ReadOnlySpan<byte>>? _metaRefresh;

    /// <summary>무인도 최초 등록과 3×3 받침 소유자 전파를 복원한 처리기다. 원본 Impl.surface에 대응하며 188개 TYPE·같은 수 프레임이 아니면 비워 둔다. 연결 조회는 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 받침 소유 지정 이식에서 추가됐고 0.251에서 표시 갱신을 연결했다.</summary>
    private readonly RawIslandPostPop? _surface;

    /// <summary>타입별 서로 다른 fallback 사슬을 같은 분배기로 모아 혼합 종속 체인의 중첩 삭제를 지원한다. 원본 Impl.formDispatch에 대응하며 188개 TYPE·같은 수 프레임이 있으면 비워 둔다. 2026-11-07 F2d 월드 본체 삭제 분배 이식에서 추가됐다.</summary>
    private readonly RawSquidFormDispatch? _formDispatch;

    /// <summary>타입별 가상 삭제 경로를 선택하는 분배기다. 원본 Impl.virtualDestroy에 대응하며 188개 TYPE·같은 수 프레임이 있으면 비워 둔다. 2026-11-07 F2d 월드 본체 삭제 분배 이식에서 추가됐다.</summary>
    private readonly RawSquidDestroyDispatch? _virtualDestroy;

    /// <summary>원본 타입 전역과 현재 점검 전역이다. 원본 Impl.buildingState에 대응한다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    private readonly BuildingSpaceState _buildingState = new();

    /// <summary>일반 건물의 공간 등록·해제를 반영하는 처리기다. 원본 Impl.building에 대응하며 188개 TYPE가 아니면 비워 둔다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    private readonly RawBuildingSpace? _building;

    /// <summary>객체 Pop 이후 목록·공간 후처리를 수행하는 처리기다. 원본 Impl.postPop에 대응하며 188개 TYPE가 아니면 비워 둔다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    private readonly RawPostPop? _postPop;

    /// <summary>객체를 공간 지도에 등록하는 처리기다. 원본 Impl.pop에 대응하며 188개 TYPE가 아니면 비워 둔다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    private readonly RawPop? _pop;

    /// <summary>발자국에 인접한 표면·객체를 조회하는 검색기다. 원본 Impl.neighbors에 대응한다. 같은 수 프레임 표가 없으면 비워 둔다. 2026-11-07 F2d 월드 본체 연결 조회 이식에서 추가됐다.</summary>
    private readonly RawSquidNeighbors? _neighbors;

    /// <summary>같은 월드의 실제 연결 생성기를 빌린다. 원본 Impl.connect에 대응한다. 같은 수 프레임 표가 없으면 비워 둔다. 2026-11-07 F2d 월드 본체 연결 조회 이식에서 추가됐다.</summary>
    private readonly RawBridgeConnect? _connect;

    private readonly RawUnpop? _unpop;

    /// <summary>자산 삭제의 기본 몸체다. 원본 Impl.baseDestroy에 대응한다. 188개 TYPE가 아니면 비워 두며 CD 연결은 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    private readonly RawSquidDestroy? _baseDestroy;

    /// <summary>월드 기본 종속 해제다. 같은 물리와 장기 진단 장치만 빌리며 세션 파생 부모·입력 큐가 해제된 뒤에도 콜백을 보관하지 않는다. 2026-11-07 F2d 월드 본체 종속 묶음 이식에서 추가됐다.</summary>
    private readonly RawSquidContainedUnpop _containedUnpop;

    /// <summary>같은 풀의 Session 종단을 약한 참조로 빌린다. 반납·소멸 뒤에는 월드 기본 종단을 쓴다. 2026-11-07 F2d 월드 본체 종속 묶음 이식에서 추가됐다.</summary>
    private WeakReference<RawSquidContainedUnpop>? _sessionContainedUnpop;

    /// <summary>소유자 변경을 객체 필드와 장부에 반영하는 처리기다. 원본 Impl.owner에 대응한다. 판본에 맞는 TYPE 개수가 아니면 비워 두며 표면 받침 분배는 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 소유자 지정 이식에서 추가됐다.</summary>
    private readonly SquidOwner? _owner;

    /// <summary>생산·표면 재정의를 현재 가상 표로 분배하는 소유자 지정이다. 원본 Impl.assetOwner에 대응한다. 2026-11-07 F2d 월드 본체 소유자 지정 이식에서 추가됐다.</summary>
    private readonly Action<int, uint>? _assetOwner;

    /// <summary>교체한 현재 공통 삭제 몸체다. 원본 Impl.common에 대응하며 BindDeletion 전에는 비어 있다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    private RawSquidDestroyLifecycle? _common;

    /// <summary>공통 삭제 실제 소비자를 설치했는지다. 원본 Impl.deletionBound에 대응한다. 첫 객체 생성 전 한 번만 연결한다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    private bool _deletionBound;

    /// <summary>후설치와 같은 현재 원본 삭제 전역이다. 원본 Impl.deletion에 대응한다. 별도 세션 사본을 만들지 않으며 공유 좌표 연결은 BindDeletion 단위에서 잇는다. 2026-11-07 F2d 월드 본체 삭제 상태 이식에서 추가됐다.</summary>
    private readonly SquidDeletionState _deletion = new();

    /// <summary>모든 Form·경로·Regular가 쓰는 같은 실제 커널이다. 원본 Impl.kernel에 대응하며 호출자 저장소보다 오래 살 수 없다. 외부에서 빌린 커널의 수명은 호출자가 둔다. 2026-11-07 F2d 월드 본체 커널 차용 이식에서 추가됐다.</summary>
    private readonly Kernel _kernel;

    /// <summary>독립 관측과 실제 세션 효과를 구별하는 효과 요청 수다. 원본 Impl.effectRequests에 대응하며 런타임 연결 전에는 쌓이기만 한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    private ulong _effectRequests = 0ul;

    /// <summary>독립 관측과 실제 세션 효과를 구별하는 삭제 요청 수다. 원본 Impl.deletionRequests에 대응하며 BindDeletion 교체 뒤에는 공통 몸체가 센다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    private ulong _deletionRequests = 0ul;

    /// <summary>같은 판본·타입·지형으로 월드를 만들고 물리 저장소를 빌리거나 새로 만든다.</summary>
    /// <param name="edition">월드가 따르는 원본 판본이다.</param>
    /// <param name="types">같은 전역 TYPE 표다. 비어 있으면 안 된다.</param>
    /// <param name="terrainType">초기 지면에 쓰는 지형 종류 번호다.</param>
    /// <param name="storage">빌릴 물리 저장소다. null이면 같은 판본으로 새로 만든다.</param>
    /// <param name="frames">같은 장기 프레임 표다. null이면 빈 목록을 둔다.</param>
    public RawSurfaceWorld(OriginalEdition edition, IReadOnlyList<OriginalTypeRecord> types, uint terrainType, RawSurfaceStorage? storage = null, IReadOnlyList<RiftTypeFrames>? frames = null)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (types.Count == 0)
        {
            throw new ArgumentException("서피스 월드의 실제 TYPE 표가 비었습니다.", nameof(types));
        }

        _edition = edition;
        _types = types;
        _frames = frames ?? [];
        _terrainType = terrainType;
        _storage = storage ?? new RawSurfaceStorage(edition);
        _processState = _storage.Processes().State();
        _kernel = _storage.Processes().Kernel();
        _containedUnpop = new RawSquidContainedUnpop(_storage.Pool, new SquidContainedUnpopState
        {
            ContentFormType = new UIntBox { Value = 6u },
            GumpFormType = new UIntBox { Value = 3u },
            UiEvent = new UIntBox { Value = 0x02000039u },
            CheckingDead = _storage.Checking(),
            InMetamapMode = () => _storage.OwnerMode.Challenge,
            Boss = () => _storage.Authority().Boss,
        }, new SquidContainedUnpopHooks
        {
            Parent = sid => RawContainedLink.CommonContainedParent(_storage.Pool, sid),
            Previous = sid => RawContainedLink.CommonContainedPrevious(_storage.Pool, sid),
            SetPrevious = (sid, previous) => RawContainedLink.CommonContainedSetPrevious(_storage.Pool, sid, previous),
            Assertion = (expression, file, line) => _storage.AssertDisplay(expression, file, line),
        });
        if (_types.Count == OriginalConstructors.TypeCount)
        {
            _graph = new RawGraph(_storage.Pool, _storage.Hash, _storage.Spots, _types, _storage.Graph.Records, _storage.Graph.Stack, null, GraphRecovery.FullPool);
            _unpop = new RawUnpop(_storage.Pool, _storage.Hash, _storage.Spots);
            _factory = new RawSquidFactory(_storage.Pool, _types, false, _unpop);            if (_frames.Count == _types.Count)
            {
                _surface = new RawIslandPostPop(_storage.Pool, new HashLevelView(_storage.Hash), _types, _frames, _storage.Links, _loadState, new IslandPostPopHooks
                {
                    Objects = new BridgeConnectHooks { SetOwner = (sid, owner) => SetAssetOwner(sid, owner) },
                    Connect = _ => throw new InvalidOperationException("실제 표면 연결의 현재 생성기가 연결되지 않았습니다."),
                    FindTypeAt = (_, _, _) => throw new InvalidOperationException("실제 표면 연결의 현재 조회가 연결되지 않았습니다."),
                    UpdateDisplay = sid =>
                    {
                        _changed = true;
                        _display?.Update(_storage.Pool.Slot(sid));
                    },
                    IncompleteSupport = (_, _, _) => throw new InvalidOperationException("원본 표면 월드의 불완전한 받침 묶음입니다."),
                    Assertion = (expression, file, line) => _storage.AssertDisplay(expression, file, line),
                    Detail = text => _storage.RecordAssertionDetail(text),
                });
            }
            _baseDestroy = new RawSquidDestroy(_storage.Pool, _unpop, _types);
            _buildingState.IslandType = _terrainType;
            _buildingState.BridgeType = _storage.Links.BridgeType;
            _building = new RawBuildingSpace(_storage.Pool, _storage.Hash, _types, _buildingState);
            _postPop = new RawPostPop(_storage.Pool, _types, _bookkeeping, _graph);
            _pop = new RawPop(_storage.Pool, _storage.Hash, _storage.Spots, null, _postPop);
            _pop.SetBuildingSide(_building);
            _unpop.SetBuildingSide(_building);
            if (_frames.Count == _types.Count)
            {
                RawSquidFactory factory = _factory!;
                RawBuildingSpace building = _building!;
                RawSquidDestroy baseDestroy = _baseDestroy!;
                RawGraph graph = _graph!;
                _neighbors = new RawSquidNeighbors(_storage.Pool, _storage.Hash, _storage.Spots, _types, new RiftFrameCodeTables(_frames));
                _connect = new RawBridgeConnect(_storage.Pool, _neighbors, _storage.Links, new BridgeConnectHooks
                {
                    Create = (type, flags) =>
                    {
                        _changed = true;
                        return factory.Create(type, (SidAllocation)flags);
                    },
                    SetOwner = (sid, owner) =>
                    {
                        SetAssetOwner(sid, owner);
                        _changed = true;
                    },
                    Pop = (sid, x, y, flags) => Pop(sid, x, y, flags),
                    SetFrame = (_, _, _) => throw new InvalidOperationException("실제 프레임 지정이 연결되지 않았습니다."),
                    NotifySurface = sid => building.NotifySurface(sid),
                });
                RawSquidDestroyDispatch virtualDestroy = new(baseDestroy, _neighbors, () => _mode, graph);
                _virtualDestroy = virtualDestroy;
                _formDispatch = new RawSquidFormDispatch(_storage.Pool, baseDestroy, new SquidDestroyHooks
                {
                    Selected = () => _deletion.Selected,
                    Emit = effect =>
                    {
                        _deletionRequests++;
                        if (effect.Effect == SquidDestroyEffect.PreDestroy)
                        {
                            baseDestroy.CompletePreDestroy();
                        }
                        else if (effect.Effect == SquidDestroyEffect.PostDestroy)
                        {
                            baseDestroy.CompletePostDestroy();
                        }
                    },
                }, (sid, flags, hooks) => virtualDestroy.Destroy(sid, flags, hooks));
            }
        }

        // 소유자 처리는 판본에 맞는 개수(패치 188·CD 171)일 때만 둔다. 어긋난 기존 껍데기 호출은 명시 오류로 멈춘다.
        bool ownerReady = (_edition == OriginalEdition.Patch1078 && _types.Count == OriginalConstructors.TypeCount)
            || (_edition == OriginalEdition.Cd1072 && _types.Count == 171);        if (ownerReady)
        {
            SquidOwner owner = new(_storage.Pool, _types, _bookkeeping, _storage.OwnerMode);
            _owner = owner;
            _assetOwner = RawBridgeConnect.MakeOwnerDispatch(_storage.Pool, _storage.Links, owner.Set);
        }

        // 표시 수집은 판본과 무관하게 같은 수의 셰이프 표가 있을 때만 둔다. 자동 껍데기 저장소의 빈 표에서는 비워 둔다.
        if (_types.Count == _storage.Shapes.Count && _storage.Shapes.Count > 0)
        {
            _display = new SquidDisplay(_edition, _types, _storage.Shapes, new WorldDisplaySink(this), new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(0, 0, 640, 480) });
        }

        _bookkeeping.LocalOwner = 1;
    }

    /// <summary>월드가 따르는 원본 판본이다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    /// <returns>원본 판본이다.</returns>
    public OriginalEdition Edition() => _edition;

    /// <summary>초기 지면에 쓰는 지형 종류 번호다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    /// <returns>지형 종류 번호다.</returns>
    public uint TerrainType() => _terrainType;

    /// <summary>생성 때 받은 같은 전역 TYPE 표다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    /// <returns>TYPE 표다.</returns>
    public IReadOnlyList<OriginalTypeRecord> Types() => _types;

    /// <summary>생성 때 받은 같은 장기 프레임 표다. 비었으면 빈 목록이다. 2026-11-07 F2d 월드 본체 프레임 소유 이식에서 추가됐다.</summary>
    /// <returns>프레임 표다.</returns>
    public IReadOnlyList<RiftTypeFrames> Frames() => _frames;

    /// <summary>같은 장기 프레임 표의 수정 가능 범위를 표면·이웃 소비자에게만 빌린다. 새 사본을 만들지 않는다. 2026-11-07 F2d 월드 본체 프레임 소유 이식에서 추가됐다.</summary>
    /// <returns>생성 때 받은 같은 프레임 목록의 수정 가능 보기다.</returns>
    public IList<RiftTypeFrames> MutableFrames()
    {
        // 원본 span 프레임을 직역한다. 목록 자체가 바뀔 수 있으므로 같은 참조를 빌린다.
        if (_frames is IList<RiftTypeFrames> mutable)
        {
            return mutable;
        }

        throw new InvalidOperationException("서피스 월드의 실제 프레임 표가 수정 가능한 목록이 아닙니다.");
    }

    /// <summary>빌리거나 새로 만든 같은 물리 저장소다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    /// <returns>물리 저장소다.</returns>
    public RawSurfaceStorage Storage() => _storage;

    /// <summary>변경 요청을 한 번만 소비한다. 원본 표시가 전체 갱신으로 억제되어도 논리 변경은 남는다. 2026-11-07 F2d 월드 본체 TakeChanged 이식에서 추가됐다.</summary>
    /// <returns>소비 전에 변경이 남았으면 true, 이미 소비했으면 false다.</returns>
    public bool TakeChanged()
    {
        // 원본 std::exchange(impl_->changed, false)를 직역한다.
        bool previous = _changed;
        _changed = false;
        return previous;
    }

    /// <summary>같은 전역 TYPE의 수정 가능 범위를 원본 옵션 소비자에게만 빌린다. 새 사본을 만들지 않는다. 2026-11-07 F2d 월드 본체 MutableTypes 이식에서 추가됐다.</summary>
    /// <returns>생성 때 받은 같은 TYPE 목록의 수정 가능 보기다.</returns>
    public IList<OriginalTypeRecord> MutableTypes()
    {
        // 원본 std::span<RiftTypeRecord>를 직역한다. 목록 자체가 바뀔 수 있으므로 같은 참조를 빌린다.
        if (_types is IList<OriginalTypeRecord> mutable)
        {
            return mutable;
        }

        throw new InvalidOperationException("서피스 월드의 실제 TYPE 표가 수정 가능한 목록이 아닙니다.");
    }

    /// <summary>파생 로더의 중첩 깊이를 표면과 비표면 몸체에 동일하게 전달한다. 런타임 연결 뒤의 전달과 저장 게임 공유 연결은 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 SetLoadingDepth 이식에서 추가됐다.</summary>
    /// <param name="depth">쓸 로딩 중첩 깊이다.</param>
    public void SetLoadingDepth(uint depth)
    {
        // 원본 impl_->loadState.loadingDepth 대입을 직역한다. runtimeBound 때의 runtime.loadingDepth 전달은 BindRuntime 단위에서 잇는다.
        _loadState.LoadingDepth = depth;
    }

    /// <summary>현재 로딩 중첩 깊이를 돌려준다. 원본에 없는 C# 조회이며 검사·소비자 연결용이다. 2026-11-07 F2d 월드 본체 SetLoadingDepth 이식에서 추가됐다.</summary>
    /// <returns>현재 로딩 중첩 깊이다.</returns>
    public uint LoadingDepth() => _loadState.LoadingDepth;

    /// <summary>최초 객체·런타임 등록 전에 원본 모드의 실제 소유권·연결색·다리 붕괴 소비자를 함께 설정한다. 세션 전역만 바꾸고 비공개 편집 값은 남기지 않는다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    /// <param name="fort">현재(편집) 모드다. 켜져 있으면 작업장 목록을 고친다.</param>
    /// <param name="battle">전투 모드다. 연결색과 작업장 목록 조건에 쓴다.</param>
    /// <param name="challenge">도전 모드다. 켜지면 유효 소유자 번호가 1~39로 바뀐다.</param>
    /// <param name="editor">편집기다. 켜져 있으면 다리 붕괴 스캔을 돌리지 않는다.</param>
    public void ConfigureInstallModes(bool fort, bool battle, bool challenge, bool editor)
    {
        // 원본 첫 객체/Runtime 등록 전 관문을 직역한다. 지정 Take가 freeCount를 보존할 수 있어 실제 슬롯도 함께 검사한다.
        RawSidPool pool = _storage.Pool;
        if (_runtimeBound || _loading || _loaded || pool.FreeCount != (uint)(pool.Capacity - RawSidPool.FirstOrdinary))
        {
            throw new InvalidOperationException("실제 설치 모드는 첫 객체·런타임 등록 전에 설정해야 합니다.");
        }

        // 이미 만든 실제 슬롯이 하나라도 있으면 늦은 설정을 거부한다.
        for (int sid = RawSidPool.FirstOrdinary; sid < pool.Capacity; sid++)
        {
            if ((pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.FreeBit) == 0)
            {
                throw new InvalidOperationException("실제 설치 모드 설정 전에 객체가 존재합니다.");
            }
        }

        SquidOwnerMode ownerMode = _storage.OwnerMode;
        ownerMode.Fort = fort;
        ownerMode.Battle = battle;
        ownerMode.Challenge = challenge;
        _storage.Links.Battle = battle;
        _mode = _mode with { Editor = editor };
    }

    /// <summary>현재 붕괴 처리 실행 상태 전역을 돌려준다. 원본에 없는 C# 조회이며 검사·소비자 연결용이다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    /// <returns>현재 실행 상태 전역이다.</returns>
    public BridgeDecayMode DecayMode() => _mode;

    /// <summary>빈·죽음·무효·포함 상태를 동시에 거르는 마스크다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    private const byte InactiveMask = 15;

    /// <summary>정렬되지 않은 원본 필드를 리틀 엔디언으로 읽는다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다. CD의 인접 extra 바이트를 섞지 않게 지정 폭만 읽는다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint ReadSlot(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        // 지정 폭만 읽어 필드 밖의 바이트는 섞지 않는다.
        uint value = 0u;
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }

        return value;
    }

    /// <summary>원본 필드 밖의 바이트는 보존하며 지정 폭만 쓴다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    /// <param name="bytes">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 폭이다.</param>
    private static void WriteSlot(Span<byte> bytes, int offset, uint value, int width)
    {
        // 필드 밖의 바이트는 보존한다.
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (8 * i));
        }
    }

    /// <summary>표시·검사에 현재 살아 있는 자산만 제공한다. ProcessForm은 제외한다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    /// <returns>살아 있는 자산의 스냅샷 목록이다.</returns>
    public List<SurfaceSnapshot> Objects()
    {
        // 현재 살아 있는 자산 번호만 읽는다. ProcessForm·반납 슬롯은 표시 대상이 아니다.
        var result = new List<SurfaceSnapshot>();
        bool patch = _edition == OriginalEdition.Patch1078;
        RawSidPool pool = _storage.Pool;
        for (int sid = RawSidPool.FirstOrdinary; sid < pool.Capacity; sid++)
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            if ((raw[RawSidPool.StateOffset] & InactiveMask) != 0 || raw[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType)
            {
                continue;
            }

            uint type = raw[RawSidPool.TypeOffset];
            uint owner = raw[patch ? 34 : 32];
            int frame = unchecked((int)ReadSlot(raw, patch ? 36 : 34, patch ? 4 : 1));
            float x = BitConverter.UInt32BitsToSingle(ReadSlot(raw, 14, 4));
            float y = BitConverter.UInt32BitsToSingle(ReadSlot(raw, 18, 4));
            result.Add(new SurfaceSnapshot(sid, new SurfaceSeed(type, owner, frame, x, y)));
        }

        return result;
    }

    /// <summary>같은 시각만 공급한다. 실제 Client는 이후 전역 Kernel을 정확히 한 번 실행한다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    /// <param name="time">공급할 프레임 시각이다.</param>
    public void PrepareFrame(FrameTime time)
    {
        // 실제 현재 시각으로 독립 스냅샷의 게임 값만 교정한다.
        if (_processState.SharedNow is not null)
        {
            time.Game = _processState.Now;
        }

        if (!double.IsFinite(time.Game) || time.Game < _time.Game)
        {
            throw new ArgumentException("원본 월드의 역행·비유한 게임 시각입니다.", nameof(time));
        }

        // bound Client 시각은 스냅샷으로 역쓰기하지 않는다.
        if (_processState.SharedNow is null)
        {
            _processState.Now = time.Game;
        }

        _time = time;
    }

    /// <summary>현재 공급된 프레임 시각을 돌려준다. 원본에 없는 C# 조회이며 검사·소비자 연결용이다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    /// <returns>현재 프레임 시각이다.</returns>
    public FrameTime CurrentTime() => _time;

    /// <summary>원본 Fort 순서대로 Create·필드 저장·Pop하는 로더의 시작과 종료다. 본섬→행 우선 noIsland→다리 순으로 일반 Pop을 실행하고 마지막에 전체 Graph를 재구성한다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐고 0.254에서 초기 적재를 연결했다.</summary>
    /// <param name="seeds">초기 위치 목록이다.</param>
    public void Load(IReadOnlyList<SurfaceSeed> seeds)
    {
        ArgumentNullException.ThrowIfNull(seeds);
        if (_loaded)
        {
            throw new InvalidOperationException("원본 표면 월드 중복 적재입니다.");
        }

        if (_factory is null)
        {
            throw new InvalidOperationException("실제 월드 생성이 연결되지 않았습니다.");
        }

        // 쓰기 전에 전체 초기 입력의 지원 타입·정수 좌표·프레임·소유자를 검사한다.
        foreach (SurfaceSeed seed in seeds)
        {
            if (seed.Type != _terrainType && seed.Type != _storage.Links.NoIslandType && seed.Type != _storage.Links.BridgeType)
            {
                throw new ArgumentException("원본 표면 월드의 미지원 초기 타입입니다.", nameof(seeds));
            }

            if (!float.IsFinite(seed.X) || !float.IsFinite(seed.Y) || seed.X < 1 || seed.Y < 1 || seed.X >= 256 || seed.Y >= 256
                || seed.X != MathF.Truncate(seed.X) || seed.Y != MathF.Truncate(seed.Y) || seed.Owner > MaximumOwner
                || seed.Frame < 0 || seed.Frame >= _frames[(int)seed.Type].Codes.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(seeds), "원본 표면 월드 초기 필드 오류");
            }
        }

        // 비유한 좌표를 거부한 뒤 정렬한다. 받침 접두가 서·북 이웃을 읽으므로 noIsland는 행 우선이다.
        List<SurfaceSeed> sorted = seeds.OrderBy(seed => seed.Type == _terrainType ? 0 : seed.Type == _storage.Links.NoIslandType ? 1 : 2).ThenBy(seed => seed.Y).ThenBy(seed => seed.X).ToList();
        BeginLoad();
        // 실제 생성자·가상 소유자·Pop을 사용하며 아직 Graph를 변경하지 않는다.
        foreach (SurfaceSeed seed in sorted)
        {
            int sid = _factory.Create(seed.Type);
            SetAssetOwner(sid, seed.Owner);
            WriteSlot(_storage.Pool.AllocatedBytes(sid), _edition == OriginalEdition.Patch1078 ? 36 : 34, unchecked((uint)seed.Frame), _edition == OriginalEdition.Patch1078 ? 4 : 1);
            Pop(sid, seed.X, seed.Y, 0u);
        }

        FinishLoad();
    }

    /// <summary>소유자 번호의 최댓값이다. 원본 kPlayerCount(8)에 대응한다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    private const uint MaximumOwner = 8u;

    /// <summary>원본 Fort 순서대로 Create·필드 저장·Pop하는 로더의 시작이다. 적재 중 Graph·장부를 켠다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐고 0.254에서 초기 적재를 연결했다.</summary>
    public void BeginLoad()
    {
        // 적재 중에도 noGraph 표면의 254 지정과 건물 점유 영역 무효화가 필요해 미리 켠다.
        if (_loaded || _loading)
        {
            throw new InvalidOperationException("원본 월드 중복 적재 시작입니다.");
        }

        _bookkeeping.GraphsEnabled = true;
        _loading = true;
        SetLoadingDepth(1u);
    }

    /// <summary>실제 캠페인 부모가 Graph(false)를 이미 실행한 뒤 로딩 상태만 종료한다. Graph·풀·해시를 재구성하지 않는다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    public void FinishLoadAfterGraph()
    {
        if (!_loading || _loaded)
        {
            throw new InvalidOperationException("원본 월드 적재 종료 순서가 잘못됐습니다.");
        }

        _bookkeeping.GraphsEnabled = true;
        SetLoadingDepth(0u);
        _loading = false;
        _loaded = true;
        _changed = true;
    }

    /// <summary>모든 실제 객체를 등록한 뒤 원본 Graph·장부를 활성화한다. 2026-11-07 F2d 월드 본체 그래프 적재 이식에서 추가됐다.</summary>
    public void FinishLoad()
    {
        if (!_loading || _loaded)
        {
            throw new InvalidOperationException("원본 월드 로드 종료 순서 오류입니다.");
        }

        if (_graph is null)
        {
            throw new InvalidOperationException("원본 월드 그래프가 연결되지 않았습니다.");
        }

        _graph.Rebuild(true);
        FinishLoadAfterGraph();
    }

    /// <summary>같은 월드가 소유한 표면 Graph 처리기를 빌린다. 188개 TYPE가 아니면 비어 있다. 2026-11-07 F2d 월드 본체 그래프 적재 이식에서 추가됐다.</summary>
    /// <returns>표면 Graph 처리기다. 없으면 null이다.</returns>
    public RawGraph? Graph() => _graph;

    /// <summary>적재 작업 중인지 돌려준다. 원본에 없는 C# 조회이며 검사·소비자 연결용이다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    /// <returns>적재 중이면 참이다.</returns>
    public bool Loading() => _loading;

    /// <summary>현재 자료의 적재가 끝났는지 돌려준다. 원본에 없는 C# 조회이며 검사·소비자 연결용이다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    /// <returns>적재가 끝났으면 참이다.</returns>
    public bool Loaded() => _loaded;

    /// <summary>같은 월드가 소유한 제어 장부를 빌린다. 새 사본을 만들지 않는다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    /// <returns>제어 장부다.</returns>
    public RawPostPopState Bookkeeping() => _bookkeeping;

    /// <summary>후설치와 같은 현재 원본 삭제 전역이다. 별도 세션 사본을 만들지 않는다. 2026-11-07 F2d 월드 본체 삭제 상태 이식에서 추가됐다.</summary>
    /// <returns>현재 원본 삭제 전역이다.</returns>
    public SquidDeletionState DeletionState() => _deletion;

    /// <summary>첫 객체 생성 전에 공통 삭제의 관찰 경계를 실제 보상·현재 선택·선택해제·소리 소비자로 한 번 교체한다. 선택 함수 보관은 삭제 실행 단위에서 잇는다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    /// <param name="selected">현재 UI 선택을 읽는 함수다. null이면 안 된다.</param>
    /// <param name="hooks">선택 해제·보상·소리 효과와 파생 경계다. Emit·Forward가 없으면 안 된다.</param>
    public void BindDeletion(Func<int>? selected, SquidDeletionHooks hooks)
    {
        // Take는 freeCount를 보존할 수 있으므로 동일 물리 슬롯도 검사한다.
        RawSidPool pool = _storage.Pool;
        if (_deletionBound || _loading || _loaded || pool.FreeCount != (uint)(pool.Capacity - RawSidPool.FirstOrdinary))
        {
            throw new InvalidOperationException("공통 삭제 실제 소비자는 첫 객체 생성 전에 한 번 연결해야 합니다.");
        }

        // 이미 만든 실제 슬롯이 하나라도 있으면 늦은 설치를 거부한다.
        for (int sid = RawSidPool.FirstOrdinary; sid < pool.Capacity; sid++)
        {
            if ((pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.FreeBit) == 0)
            {
                throw new InvalidOperationException("공통 삭제 실제 소비자 설치 전 객체가 존재합니다.");
            }
        }

        if (_unpop is null || _baseDestroy is null || _graph is null)
        {
            throw new InvalidOperationException("공통 삭제 실제 소비자는 188개 TYPE 월드에서만 연결됩니다.");
        }

        ArgumentNullException.ThrowIfNull(selected);
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Emit is null || hooks.Forward is null)
        {
            throw new ArgumentException("공통 삭제의 실제 선택·효과·파생 경계가 없습니다.", nameof(hooks));
        }

        _common = new RawSquidDestroyLifecycle(pool, _types, _bookkeeping, _deletion, _baseDestroy, hooks, _graph);
        _deletionBound = true;
    }

    /// <summary>현재 후설치된 실제 공통 삭제의 검사·Pre·Post를 파생 자산과 공유한다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    /// <returns>현재 공통 삭제 몸체다.</returns>
    public RawSquidDestroyLifecycle CommonDeletion()
    {
        if (_common is null)
        {
            throw new InvalidOperationException("공통 삭제 실제 소비자가 설치되지 않았습니다.");
        }

        return _common;
    }

    /// <summary>같은 풀의 Session 종단을 약한 참조로 빌린다. 반납·소멸 뒤에는 월드 기본 종단을 쓴다. 2026-11-07 F2d 월드 본체 종속 묶음 이식에서 추가됐다.</summary>
    /// <param name="binding">빌릴 Session 종단이다. null이면 기본 종단으로 되돌린다.</param>
    public void BindContainedUnpop(RawSquidContainedUnpop? binding)
    {
        if (binding is not null && !ReferenceEquals(binding.Pool(), _storage.Pool))
        {
            throw new ArgumentException("종속 Unpop의 실제 SID 풀이 다릅니다.", nameof(binding));
        }

        _sessionContainedUnpop = binding is null ? null : new WeakReference<RawSquidContainedUnpop>(binding);
    }

    /// <summary>사제·신전의 전용 몸체를 상위에서 선택한 뒤 나머지 자산 +74를 현재 표로 호출한다. 받침 분배는 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 소유자 지정 이식에서 추가됐다.</summary>
    /// <param name="sid">지정할 객체 번호다.</param>
    /// <param name="owner">쓸 소유자 번호다.</param>
    public void SetAssetOwner(int sid, uint owner)
    {
        if (_assetOwner is null)
        {
            throw new InvalidOperationException("자산 소유자 지정이 연결되지 않았습니다.");
        }

        _assetOwner(sid, owner);
        _changed = true;
    }

    /// <summary>모든 Form·경로·Regular가 쓰는 같은 실제 커널을 빌린다. 새 커널을 만들지 않는다. 2026-11-07 F2d 월드 본체 커널 차용 이식에서 추가됐다.</summary>
    /// <returns>같은 실제 커널이다.</returns>
    public Kernel Kernel() => _kernel;

    /// <summary>외부 연결 모듈을 해제하기 전에 예약 객체를 먼저 정리한다. 월드 자료는 유지한다. 2026-11-07 F2d 월드 본체 커널 차용 이식에서 추가됐다.</summary>
    public void ClearProcesses() => _kernel.Clear();

    /// <summary>명시 전체 커널의 최종 소멸 전용이며 원문 Clear 한 번과 구별한다. 2026-11-07 F2d 월드 본체 커널 차용 이식에서 추가됐다.</summary>
    public void DrainProcesses()
    {
        // 소멸 중 새로 등록된 과정도 다음 순회에서 실제 현재값으로 처리한다.
        do
        {
            _kernel.Clear();
        }
        while (_kernel.Size() != 0);
    }

    /// <summary>같은 월드·전송·공간 재등록을 참가자별로 실행한다. 멀티플레이 3차 사전 구현이며 전송기가 없어 명시 오류로 멈춘다. 2026-11-07 F2d 월드 본체 전송 사전 구현에서 추가됐다.</summary>
    /// <param name="owner">동기화할 참가자 번호다.</param>
    public void SynchronizePeer(uint owner)
    {
        // 원본 squidTransmission 미설치 logic_error를 직역한다. 실제 PeerWorldSync 연결은 3차 로컬 구현 때 잇는다.
        throw new InvalidOperationException("현재 판본의 실제 Squid 전송기가 설치되지 않았습니다.");
    }

    /// <summary>공통 삭제의 패킷을 같은 현재 가상 기록기·버퍼·방송 부모로 전달한다. 멀티플레이 3차 사전 구현이며 전송기가 없어 명시 오류로 멈춘다. 2026-11-07 F2d 월드 본체 전송 사전 구현에서 추가됐다.</summary>
    /// <param name="sid">삭제 패킷을 보낼 객체 번호다.</param>
    /// <param name="flags">삭제 전달 플래그다.</param>
    public void TransmitDeletion(int sid, uint flags)
    {
        // 원본 squidTransmission 미설치 logic_error를 직역한다. 실제 Transmit 연결은 3차 로컬 구현 때 잇는다.
        throw new InvalidOperationException("현재 판본의 실제 삭제 전송기가 설치되지 않았습니다.");
    }

    /// <summary>원본 정밀도(유효 12자리)로 부동소수 값을 같은 CLI 관측 문자열로 만든다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    /// <param name="value">적을 값이다.</param>
    /// <returns>원본 기본 표시와 같은 문자열이다.</returns>
    private static string FormatReportDouble(double value)
    {
        // 원본 ostringstream 기본 표시(precision 12)와 같은 유효 자릿수로 적는다.
        return value.ToString("G12", CultureInfo.InvariantCulture);
    }

    /// <summary>원본 float 값을 같은 CLI 관측 문자열로 만든다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    /// <param name="value">적을 값이다.</param>
    /// <returns>원본 기본 표시와 같은 문자열이다.</returns>
    private static string FormatReportDouble(float value)
    {
        // 원본 float 스트림 삽입은 double로 정확히 넓힌 뒤 같은 자릿수로 적는다.
        return ((double)value).ToString("G12", CultureInfo.InvariantCulture);
    }

    /// <summary>복원된 타입 생성자를 같은 풀에서 실행한다. 2026-11-07 F2d 월드 본체 생성 연결 이식에서 추가됐다.</summary>
    /// <param name="type">만들 타입 번호다.</param>
    /// <param name="flags">할당 영역 비트(1 예측, 2 클라이언트, 0 서버)다.</param>
    /// <returns>새 객체 번호다.</returns>
    public int Create(uint type, uint flags = 0u)
    {
        if (_factory is null)
        {
            throw new InvalidOperationException("실제 월드 생성이 연결되지 않았습니다.");
        }

        _changed = true;
        return _factory.Create(type, (SidAllocation)flags);
    }

    /// <summary>저장소 해시 0단계 머리를 표면 지도 보기로 빌리는 어댑터다. 사본이 아니라 같은 실제 배열을 읽는다. 2026-11-07 F2d 월드 본체 받침 소유 지정 이식에서 추가됐다.</summary>
    private sealed class HashLevelView : IReadOnlyList<ushort>
    {
        /// <summary>머리를 빌릴 실제 공간 해시다. 월드·저장소보다 오래 살 수 없다. 2026-11-07 F2d 월드 본체 받침 소유 지정 이식에서 추가됐다.</summary>
        private readonly RawSquidHash _hash;

        /// <summary>실제 공간 해시를 감싼다.</summary>
        /// <param name="hash">0단계 머리를 읽을 해시다.</param>
        public HashLevelView(RawSquidHash hash)
        {
            _hash = hash;
        }

        /// <summary>지도 한 변의 칸 수(256×256)다.</summary>
        public int Count => _hash.Entries(0).Length;

        /// <summary>해당 칸의 머리 번호를 읽는다.</summary>
        /// <param name="index">읽을 칸 번호다.</param>
        /// <returns>머리 번호다.</returns>
        public ushort this[int index] => _hash.Entries(0)[index];

        /// <summary>머리를 순서대로 열거한다.</summary>
        /// <returns>머리 열거자다.</returns>
        public IEnumerator<ushort> GetEnumerator()
        {
            for (int i = 0; i < Count; i++)
            {
                yield return this[i];
            }
        }

        /// <summary>머리를 순서대로 열거한다.</summary>
        /// <returns>머리 열거자다.</returns>
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => GetEnumerator();
    }

    /// <summary>같은 실제 풀·공간 지도를 쓰는 표시 요청 싱크다. 억제·무효화·진단·메타 갱신을 현재 월드에 전달한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    private sealed class WorldDisplaySink : SquidDisplaySink
    {
        /// <summary>요청을 전달할 월드다. 월드보다 오래 살 수 없다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
        private readonly RawSurfaceWorld _world;

        /// <summary>현재 월드에 전달하는 싱크를 만든다.</summary>
        /// <param name="world">전달할 월드다.</param>
        public WorldDisplaySink(RawSurfaceWorld world)
        {
            _world = world;
        }

        /// <summary>원본 전체 갱신·표시 억제 전역에 대응한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
        /// <returns>억제 중이면 참이다.</returns>
        public override bool Suppressed() => _world._displayHooks?.Suppressed?.Invoke() ?? false;

        /// <summary>원본 순서대로 main·shadow 영역과 플래그를 전달한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
        /// <param name="rect">변경 영역이다.</param>
        /// <param name="flags">전달 플래그다.</param>
        public override void Invalidate(DisplayRect rect, uint flags)
        {
            _world._changed = true;
            _world._displayHooks?.Invalidate?.Invoke(rect, flags);
        }

        /// <summary>표시 뷰 교체와 무관하게 같은 Client DWORD를 다시 빌린다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
        /// <returns>현재 검사 뷰다.</returns>
        public override RawCheckingFlag Checking() => _world._storage.Checking();

        /// <summary>장치와 별도로 장기 상세 owner에 저장한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
        /// <param name="type">타입 번호다.</param>
        /// <param name="frameNumber">프레임 번호다.</param>
        /// <param name="abstractValue">추상 플래그다.</param>
        public override void FrameError(uint type, int frameNumber, uint abstractValue) => _world._storage.RecordDisplayFrameError(type, frameNumber, abstractValue);

        /// <summary>현재 진단 복귀를 실제 표시 부모에 돌려준다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
        /// <param name="expression">원문 식이다.</param>
        /// <param name="file">원문 파일이다.</param>
        /// <param name="line">원문 행이다.</param>
        public override void Assertion(string expression, string file, uint line) => _world._storage.AssertDisplay(expression, file, line);

        /// <summary>현재 월드의 실제 메타 표시 갱신 장치를 설치한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
        /// <param name="raw">현재 슬롯 바이트다.</param>
        public override void MetaRefresh(ReadOnlySpan<byte> raw)
        {
            if (_world._metaRefresh is null)
            {
                throw new InvalidOperationException("현재 월드의 실제 메타 표시 갱신 미연결");
            }

            _world._metaRefresh(raw);
        }
    }

    /// <summary>저장 건물의 소유자를 받침에 반영하는 기존 초기 월드 어댑터의 경계다. 연결 조회는 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 받침 소유 지정 이식에서 추가됐다.</summary>
    /// <param name="x">기준 가로 좌표다.</param>
    /// <param name="y">기준 세로 좌표다.</param>
    /// <param name="owner">쓸 소유자다.</param>
    public void SetSupportOwner(float x, float y, uint owner)
    {
        if (_surface is null)
        {
            throw new InvalidOperationException("실제 월드 받침 처리가 연결되지 않았습니다.");
        }

        _surface.SetSupportOwner(x, y, owner);
        _changed = true;
    }

    /// <summary>현재 raw 타입의 실제 공간·표시 해제 몸체를 호출한다. `Pop` 배치·표시 갱신 연결은 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 해제 연결 이식에서 추가됐다.</summary>
    /// <param name="sid">해제할 객체 번호다.</param>
    /// <param name="flags">해제 플래그다.</param>
    public void Unpop(int sid, uint flags = 0u)
    {
        if (_unpop is null)
        {
            throw new InvalidOperationException("실제 월드 공간 해제가 연결되지 않았습니다.");
        }

        _unpop.Unpop(sid, _types[_storage.Pool.Slot(sid)[RawSidPool.TypeOffset]], flags);
        _changed = true;
    }

    /// <summary>현재 raw 프레임의 실제 SHP 크기가 공간 등록에 쓰인다.</summary>
    /// <param name="sid">등록할 객체 번호다.</param>
    /// <param name="x">가로 좌표다.</param>
    /// <param name="y">세로 좌표다.</param>
    /// <param name="flags">등록 플래그다.</param>
    public void Pop(int sid, float x, float y, uint flags = 0u)
    {
        if (_pop is null || _display is null)
        {
            throw new InvalidOperationException("실제 월드 공간 등록이 연결되지 않았습니다.");
        }

        ReadOnlySpan<byte> raw = _storage.Pool.Slot(sid);
        float[] size = _display.FrameSize(_edition, raw, unchecked((int)ReadSlot(raw, 36, 4)));
        _pop.Pop(sid, _types[raw[RawSidPool.TypeOffset]], size[0], size[1], x, y, flags);
        _changed = true;
    }

    /// <summary>Unpop 뒤 현재 raw 좌표를 읽어 같은 공간에 다시 Pop한다. 원문 호출자가 해제 순서를 정한다.</summary>
    /// <param name="sid">다시 등록할 객체 번호다.</param>
    /// <param name="flags">등록 플래그다.</param>
    public void Repop(int sid, uint flags = 0u)
    {
        ReadOnlySpan<byte> raw = _storage.Pool.Slot(sid);
        Pop(sid, BitConverter.UInt32BitsToSingle(ReadSlot(raw, 14, 4)), BitConverter.UInt32BitsToSingle(ReadSlot(raw, 18, 4)), flags);
    }

    /// <summary>다리 거부 조건과 파생 pre·post를 동일한 호출로 실행한다.</summary>
    /// <param name="sid">삭제할 객체 번호다.</param>
    /// <param name="flags">삭제 플래그다.</param>
    /// <returns>삭제했으면 참이다.</returns>
    public bool Destroy(int sid, uint flags = 0u)
    {
        if (_formDispatch is null)
        {
            throw new InvalidOperationException("실제 월드 삭제 분배가 연결되지 않았습니다.");
        }

        bool result = _formDispatch.Destroy(sid, flags);
        if (result)
        {
            _changed = true;
        }

        return result;
    }

    /// <summary>실제 월드가 소유한 연결 생성기를 빌리며 별도 생성기를 만들지 않는다.</summary>
    /// <returns>현재 연결 생성기다.</returns>
    public RawBridgeConnect BridgeConnections()
    {
        if (_connect is null)
        {
            throw new InvalidOperationException("실제 월드 연결 생성기가 연결되지 않았습니다.");
        }

        return _connect;
    }

    /// <summary>지정 위치의 실제 객체·받침을 조회한다. 없으면 0이다.</summary>
    /// <param name="x">가로 좌표다.</param>
    /// <param name="y">세로 좌표다.</param>
    /// <param name="type">찾을 타입 번호다.</param>
    /// <returns>찾은 번호다. 없으면 0이다.</returns>
    public int Find(float x, float y, uint type)
    {
        if (_connect is null)
        {
            throw new InvalidOperationException("실제 월드 연결 조회가 연결되지 않았습니다.");
        }

        return _connect.FindTypeAt(x, y, type);
    }

    /// <summary>현재 카메라의 원본 픽셀 기준값과 GUI 변경 표를 연결한다.</summary>
    /// <param name="view">바꿀 카메라·화면이다.</param>
    /// <param name="hooks">바꿀 표시 콜백 묶음이다.</param>
    public void SetDisplay(SquidDisplayView view, SurfaceDisplayHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(view);
        ArgumentNullException.ThrowIfNull(hooks);
        if (_display is null)
        {
            throw new InvalidOperationException("실제 월드 표시 수집이 연결되지 않았습니다.");
        }

        _display.SetView(view);
        _displayHooks = hooks.Clone();
    }

    /// <summary>현재 월드의 실제 메타 표시 갱신 장치를 설치한다.</summary>
    /// <param name="refresh">설치할 장치다.</param>
    public void BindMetaRefresh(Action<ReadOnlySpan<byte>> refresh)
    {
        if (refresh is null)
        {
            throw new ArgumentException("현재 메타 표시 갱신 장치 누락", nameof(refresh));
        }

        _metaRefresh = refresh;
    }

    /// <summary>깊이·Graph·프로세스·공간의 실제 상태를 읽기 전용으로 노출한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    /// <returns>원본 CLI 관측과 같은 여러 줄 보고서다.</returns>
    public string Report()
    {
        // 원본은 Graph·삭제 몸체를 항상 역참조하므로 188개가 아니면 명시 오류로 멈춘다.
        if (_graph is null || _baseDestroy is null)
        {
            throw new InvalidOperationException("실제 월드 보고서는 188개 TYPE 월드에서만 만듭니다.");
        }

        if (_frames.Count != _types.Count)
        {
            throw new InvalidOperationException("실제 월드 보고서의 프레임 표가 TYPE 표와 맞지 않습니다.");
        }

        bool patch = _edition == OriginalEdition.Patch1078;
        var text = new StringBuilder();
        uint surfaces = 0u;
        uint graphs = 0u;
        // 예약 0번 다음의 실제 연결 성분을 센다. 원본처럼 부호 없는 16비트 재해석으로 더한다.
        for (int i = 1; i < SurfaceGraph.Count; i++)
        {
            GraphRecord record = _graph.Records[i];
            if (record.InUse != 0)
            {
                graphs++;
                surfaces += unchecked((ushort)record.Surfaces);
            }
        }

        text.Append("raw_world\t").Append(FormatReportDouble(_time.Game)).Append('\t').Append(_time.Number).Append('\t').Append(_kernel.Size()).Append('\t').Append(surfaces).Append('\t').Append(graphs).Append('\t').Append(_bookkeeping.TotalCost).Append('\t').Append(_bookkeeping.Depth).Append('\t').Append(_baseDestroy.PreDepth).Append('\t').Append(_baseDestroy.PostDepth).Append('\t').Append(_effectRequests).Append('\t').Append(_deletionRequests).Append('\n');
        // 실제 Graph 표의 앞 두 부호 WORD를 그대로 관측한다. 배치 직후 장부와 실제 소속 수의 차이는 보정하지 않는다.
        for (int index = 0; index < SurfaceGraph.Count; index++)
        {
            GraphRecord record = _graph.Records[index];
            text.Append("raw_graph_record\t").Append(index).Append('\t').Append(record.Surfaces).Append('\t').Append(record.InUse).Append('\n');
        }

        BridgeDecayScanState scan = _storage.BridgeDecayScan;
        text.Append("bridge_decay\t").Append(scan.First).Append('\t').Append(scan.Last).Append('\t').Append(scan.Cursor).Append('\t').Append(FormatReportDouble(scan.Next)).Append('\t').Append(FormatReportDouble(_storage.BridgeDecayPeriod)).Append('\n');
        // 실제 지면의 기본·원소 프레임 범위를 읽기만 한다.
        RiftTypeFrames.LetterRun terrainRun = _frames[(int)_terrainType].Run((byte)'J');
        text.Append("raw_terrain_type\t").Append(_terrainType).Append('\t').Append(terrainRun.First).Append('\t').Append(terrainRun.Count).Append('\n');
        // 실제 신전 참조·존재·소유자를 관측하며 목록이나 테마를 다시 계산하지 않는다.
        byte[] islands = _storage.Islands.Records;
        uint islandCount = ReadSlot(islands, 0, 4);
        text.Append("raw_islands\t").Append(islandCount).Append('\n');
        for (uint index = 0; index < Math.Min(islandCount, 100u); index++)
        {
            int at = 4 + (int)index * 32;
            text.Append("raw_island\t").Append(index).Append('\t').Append(ReadSlot(islands, at, 4)).Append('\t').Append(islands[at + 4]).Append('\t').Append(ReadSlot(islands, at + 8, 4)).Append('\n');
        }

        // 렌더링과 같은 raw 조회 결과 번호별로 제공한다.
        foreach (SurfaceSnapshot snapshot in Objects())
        {
            SurfaceSeed seed = snapshot.Object;
            text.Append("raw_object\t").Append(snapshot.Sid).Append('\t').Append(seed.Type).Append('\t').Append(seed.Owner).Append('\t').Append(seed.Frame).Append('\t').Append(FormatReportDouble(seed.X)).Append('\t').Append(FormatReportDouble(seed.Y)).Append('\n');
            ReadOnlySpan<byte> raw = _storage.Pool.Slot(snapshot.Sid);
            // 같은 실제 슬롯의 상태·extra·HP 저장 비트다.
            text.Append("raw_asset_fields\t").Append(snapshot.Sid).Append('\t').Append((uint)raw[RawSidPool.StateOffset]).Append('\t').Append((uint)raw[patch ? 40 : 35]).Append('\t').Append(ReadSlot(raw, 26, patch ? 4 : 2)).Append('\n');
            if ((_types[raw[RawSidPool.TypeOffset]].Flags1 & TypeFlag1.Surface) != 0)
            {
                // 렌더링에 쓰는 같은 표면의 실제 소속 바이트며 번호 재할당이나 flood를 실행하지 않는다.
                text.Append("raw_surface_graph\t").Append(snapshot.Sid).Append('\t').Append((uint)raw[patch ? 30 : 28]).Append('\n');
            }

            if ((_types[raw[RawSidPool.TypeOffset]].Flags2 & 4u) != 0)
            {
                IReadOnlyList<RiftFrameCode> codes = _frames[raw[RawSidPool.TypeOffset]].Codes;
                if (seed.Frame < 0 || (long)seed.Frame >= (long)codes.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(seed.Frame), "다리 현재 수명 보고의 프레임 범위 오류");
                }

                text.Append("raw_bridge_fields\t").Append(snapshot.Sid).Append('\t').Append(ReadSlot(raw, 12, 2)).Append('\t').Append((uint)codes[seed.Frame].Flags).Append('\n');
            }

            uint table = ReadSlot(raw, 0, 4);
            // 타입 전환·저장 재적재 전후에 같은 실제 Form과 현재 Regular 예약을 관측한다.
            if (patch && RawObjectConversion.ObjectConversionBody(table) != 0)
            {
                int visited = 0;
                int regulars = 0;
                // 현재 부모의 실제 종속 체인만 따라가며 임의 프로세스를 찾아 붙이지 않는다.
                for (int form = (int)ReadSlot(raw, 6, 2); form != 0;)
                {
                    if (++visited > _storage.Pool.Capacity)
                    {
                        throw new InvalidOperationException("비석·주문 진단의 Form 체인 순환");
                    }

                    ReadOnlySpan<byte> content = _storage.Pool.Slot(form);
                    // 같은 50바이트 물리의 종류와 다음 링크다.
                    if (content[RawSidPool.TypeOffset] == 46 || content[RawSidPool.TypeOffset] == 61)
                    {
                        RegularProcess? process = _storage.Processes().Host().FromForm(form) as RegularProcess;
                        if (process is null || process.Form() != form || process.Parent() != snapshot.Sid)
                        {
                            throw new InvalidOperationException("비석·주문 진단의 실제 Regular 등록 불일치");
                        }

                        text.Append("raw_object_regular\t").Append(snapshot.Sid).Append('\t').Append(form).Append('\t').Append((uint)content[RawSidPool.TypeOffset]).Append('\t').Append(process.Event()).Append('\t').Append(process.Count()).Append('\t').Append(BitConverter.SingleToUInt32Bits(process.Payload())).Append('\t').Append(FormatReportDouble(process.Time())).Append('\n');
                        regulars++;
                    }

                    form = (int)ReadSlot(content, 4, 2);
                }

                text.Append("raw_object_regular_count\t").Append(snapshot.Sid).Append('\t').Append(regulars).Append('\n');
            }

            if (patch ? (table == 0x509148u || table == 0x509210u) : (raw[RawSidPool.TypeOffset] == 85 || raw[RawSidPool.TypeOffset] == 126))
            {
                int visited = 0;
                // 같은 실제 Form 체인 전체를 읽고 저장 종류·flags와 client/server SID를 기록한다.
                for (int form = (int)ReadSlot(raw, 6, 2); form != 0;)
                {
                    if (++visited > _storage.Pool.Capacity)
                    {
                        throw new InvalidOperationException("Factory 진단의 Form 체인 순환");
                    }

                    ReadOnlySpan<byte> content = _storage.Pool.Slot(form);
                    if (content[RawSidPool.TypeOffset] == 6)
                    {
                        text.Append("raw_factory_content\t").Append(snapshot.Sid).Append('\t').Append(form).Append('\t').Append(ReadSlot(content, 18, 2)).Append('\t').Append(ReadSlot(content, 20, 2)).Append('\n');
                    }

                    form = (int)ReadSlot(content, 4, 2);
                }
            }
        }

        return text.ToString();
    }
}
