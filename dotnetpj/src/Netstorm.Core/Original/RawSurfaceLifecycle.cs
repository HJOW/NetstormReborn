// 복원된 섬·다리·noIsland 삭제 훅을 공통 삭제와 연결하는 호스트 분배기다. 원본에 새 게임 규칙을 더하지 않는다.
namespace Netstorm.Core.Original;

// 표면 삭제 분배기 모듈을 담는 네임스페이스다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.

// 프로세스·재귀 삭제·아직 복원하지 않은 표시/소리 경계는 호출자가 공급한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
public sealed class SurfaceLifecycleHooks
{
    /// <summary>실제 ScheduleBridgeFall로 연결한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    public Action<int, float, float>? ScheduleFall { get; set; }

    /// <summary>가상 +0x10이다. DestroyDispatch와 ProcessHost::Hooks로 재진입한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>섬 표면과 다리의 가상 walker 낙하를 공유한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    public Action<int>? FallWalker { get; set; }

    /// <summary>파편 생성 요청과 낙하 소리만 전달한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    public Action<BridgeLifecycleEvent>? BridgeEffect { get; set; }

    /// <summary>현재 훅 묶음의 얕은 사본을 만든다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    /// <returns>복사된 훅 묶음이다.</returns>
    public SurfaceLifecycleHooks Clone() => new()
    {
        ScheduleFall = ScheduleFall,
        Destroy = Destroy,
        FallWalker = FallWalker,
        BridgeEffect = BridgeEffect,
    };
}

// 복원된 섬·다리·noIsland 삭제 훅을 공통 삭제와 연결한다. 원본에 새 게임 규칙을 더하지 않는 호스트 분배기다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
public sealed class RawSurfaceLifecycle
{
    /// <summary>자산 뿌리가 될 수 있는 가장 작은 타입 번호다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint FirstAssetType = 70u;

    /// <summary>10.78 다리 클래스의 가상 표 값이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint PatchBridgeVtable = 0x005034c8u;

    /// <summary>CD판 다리 클래스의 가상 표 값이다. 첫 구현은 PATCH 전용이라 관측되지 않으며 후속 단위에서 잇는다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint CdBridgeVtable = 0x00501ab0u;

    /// <summary>10.78 섬 받침 클래스의 가상 표 값이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint PatchIslandVtable = 0x00506d98u;

    /// <summary>CD판 섬 받침 클래스의 가상 표 값이다. 첫 구현은 PATCH 전용이라 관측되지 않으며 후속 단위에서 잇는다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint CdIslandVtable = 0x00506960u;

    /// <summary>10.78 무인도 표면 클래스의 가상 표 값이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint PatchNoIslandVtable = 0x00506f08u;

    /// <summary>CD판 무인도 표면 클래스의 가상 표 값이다. 첫 구현은 PATCH 전용이라 관측되지 않으며 후속 단위에서 잇는다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private const uint CdNoIslandVtable = 0x00506ad0u;

    /// <summary>객체 슬롯의 생성·조회·반납에 사용하는 SID 풀이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>연결부 타입이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private readonly uint _connectorType;

    /// <summary>파생 타입 사이에 공유하는 기본 처리기 또는 상태다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private readonly SquidDestroyHooks _common;

    /// <summary>외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private readonly SurfaceLifecycleHooks _hooks;

    /// <summary>같은 풀/타입으로 만든 내부 삭제 탐색기다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private readonly RawSquidFinder _finder;

    /// <summary>다리 삭제 몸체다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private readonly RawBridgeLifecycle _bridge;

    /// <summary>섬 삭제 몸체다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private readonly RawIslandLifecycle _island;

    /// <summary>내부 다리 탐색 훅이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    private readonly BridgeLifecycleHooks _bridgeHooks;

    /// <summary>
    /// 공통 삭제 훅을 각 파생 Base 호출에 연결하고 내부 탐색기의 수명을 분배기와 맞춘다. 이 첫 구현은 PATCH 전용이며 CD 판본·전수 장면 재생은 후속 단위에서 잇는다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="neighbors">이웃 탐색기다.</param>
    /// <param name="connectorType">연결부 타입이다.</param>
    /// <param name="common">공통 삭제 훅이다.</param>
    /// <param name="hooks">외부 효과 묶음이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">표면 삭제 연결 객체 타입 범위 오류다.</exception>
    /// <exception cref="ArgumentException">표면 삭제 공통/외부 훅 누락이다.</exception>
    public RawSurfaceLifecycle(
        RawSidPool pool,
        RawSquidNeighbors neighbors,
        uint connectorType,
        SquidDestroyHooks common,
        SurfaceLifecycleHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(neighbors);
        ArgumentNullException.ThrowIfNull(common);
        ArgumentNullException.ThrowIfNull(hooks);
        if (connectorType < FirstAssetType || connectorType >= (uint)neighbors.Types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(connectorType), "표면 삭제 연결 객체 타입 범위 오류");
        }

        if (common.Emit is null || common.Selected is null ||
            hooks.ScheduleFall is null || hooks.Destroy is null || hooks.FallWalker is null || hooks.BridgeEffect is null)
        {
            throw new ArgumentException("표면 삭제 공통/외부 훅 누락");
        }

        _pool = pool;
        _connectorType = connectorType;
        _common = common;
        _hooks = hooks.Clone();
        _finder = new RawSquidFinder(pool, neighbors.Hash, neighbors.Types);
        _bridge = new RawBridgeLifecycle(pool, neighbors.Types);
        // 내부 콜백이 분배기를 참조하므로 복사·이동 뒤 다른 인스턴스를 가리키는 연결을 금지한다. C# 클래스는 값 복사가 없어 같은 의미를 가진다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        SurfaceLifecycleHooks outer = _hooks;
        SquidDestroyHooks shared = _common;
        _island = new RawIslandLifecycle(pool, neighbors, new IslandLifecycleHooks
        {
            // 섬 삭제가 예약하는 실제 지연 낙하 경로다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
            ScheduleFall = (sid, x, y) => outer.ScheduleFall!(sid, x, y),
            // noIsland 칸 위 walker의 가상 낙하다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
            FallWalker = sid => outer.FallWalker!(sid),
            // 파생 접두 뒤에도 공통 preDestroy를 정확히 한 번 수행한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
            BasePreDestroy = (sid, flags) => shared.Emit!(new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, sid, 0, flags)),
            // 표면 낙하 뒤 공통 postDestroy로 삭제 깊이/장부를 마무리한다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
            BasePostDestroy = (sid, flags) => shared.Emit!(new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, sid, 0, flags)),
        });
        _bridgeHooks = RawBridgeLifecycle.MakeHooks(_finder, EmitBridge);
    }

    /// <summary>
    /// ProcessHost의 asset 훅으로 공급한다. form·종속 처리는 ProcessHost가 먼저 받고 자산만 여기로 넘긴다. 같은 인스턴스의 다리 탐색은 중첩 사용하지 않는다. 원본 private 분배를 직접 검사에서 호출할 수 있게 공개한다.
    /// </summary>
    /// <returns>파생 삭제 사건만 분배하는 훅 묶음이다.</returns>
    public SquidDestroyHooks Hooks()
    {
        SquidDestroyHooks shared = _common;
        return new SquidDestroyHooks
        {
            Emit = Emit,
            Selected = shared.Selected,
            UnpopForm = shared.UnpopForm,
        };
    }

    /// <summary>
    /// 실제 가상 표 기록값으로 bridge pre/post, island pre, noIsland post를 분배한다. 나머지는 공통 훅이다. 원본 private 분배를 직접 검사에서 호출할 수 있게 공개한다.
    /// </summary>
    /// <param name="event">삭제 효과 사건이다.</param>
    public void Emit(SquidDestroyEvent @event)
    {
        bool pre = @event.Effect == SquidDestroyEffect.PreDestroy;
        bool post = @event.Effect == SquidDestroyEffect.PostDestroy;
        if (pre || post)
        {
            uint vtable = Vtable(_pool.Slot(@event.Sid));
            bool patch = _pool.Edition == OriginalEdition.Patch1078;
            if (vtable == (patch ? PatchBridgeVtable : CdBridgeVtable))
            {
                if (pre)
                {
                    _bridge.PreDestroy(@event.Sid, @event.Flags, _connectorType, _bridgeHooks);
                }
                else
                {
                    _bridge.PostDestroy(@event.Sid, @event.Flags, _bridgeHooks);
                }

                return;
            }

            if (pre && vtable == (patch ? PatchIslandVtable : CdIslandVtable))
            {
                _island.IslandPreDestroy(@event.Sid, @event.Flags);
                return;
            }

            if (post && vtable == (patch ? PatchNoIslandVtable : CdNoIslandVtable))
            {
                _island.SurfacePostDestroy(@event.Sid, @event.Flags);
                return;
            }
        }

        _common.Emit!(@event);
    }

    /// <summary>
    /// 연결 객체는 실제 삭제, Base는 공통 훅, walker는 낙하 훅, 파편/소리는 외부 효과로 연결한다. 원본 private 분배를 직접 검사에서 호출할 수 있게 공개한다.
    /// </summary>
    /// <param name="event">다리 삭제 효과 사건이다.</param>
    public void EmitBridge(BridgeLifecycleEvent @event)
    {
        switch (@event.Effect)
        {
            case BridgeLifecycleEffect.DestroyLink:
                _hooks.Destroy!(@event.Sid, @event.Flags);
                break;
            case BridgeLifecycleEffect.BasePreDestroy:
                _common.Emit!(new SquidDestroyEvent(SquidDestroyEffect.PreDestroy, @event.Sid, 0, @event.Flags));
                break;
            case BridgeLifecycleEffect.BasePostDestroy:
                _common.Emit!(new SquidDestroyEvent(SquidDestroyEffect.PostDestroy, @event.Sid, 0, @event.Flags));
                break;
            case BridgeLifecycleEffect.FallWalker:
                _hooks.FallWalker!(@event.Sid);
                break;
            default:
                _hooks.BridgeEffect!(@event);
                break;
        }
    }

    /// <summary>슬롯 첫 DWORD의 원본 가상 표 기록값을 읽는다. 원본 주소를 호스트 포인터로 실행하지 않는다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <returns>합친 가상 표 값이다.</returns>
    private static uint Vtable(ReadOnlySpan<byte> raw)
    {
        uint value = 0u;
        // little endian 순서로 네 바이트를 합치는 반복문이다. 2026-11-07 F2d 서피스 월드 스택 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)raw[index] << (index * 8);
        }

        return value;
    }
}
