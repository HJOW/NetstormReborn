using Netstorm.Core.Simulation;

// 같은 물리와 장부/표시/생성/전송을 공유하는 장기 공간 실행기를 조립한다.
namespace Netstorm.Core.Original;

/// <summary>PostPop/Pop·프레임·연결/표면 실행기를 World와 독립인 같은 수명에 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 깊이/비용/생산 장부와 실제 현재 접두를 사용하는 후처리다.</summary>
    private RawPostPop? _postPopExecutor;
    /// <summary>같은 Hash/spot/표시/후처리와 현재 전송을 사용하는 등록기다.</summary>
    private RawPop? _popExecutor;
    /// <summary>같은 표시/Unpop/Pop을 원문 순서로 사용하는 프레임 실행기다.</summary>
    private SquidFrame? _frameExecutor;
    /// <summary>같은 실제 이웃과 생성/공간 효과를 사용하는 연결기다.</summary>
    private RawBridgeConnect? _connectionsExecutor;
    /// <summary>같은 표면 지도/현재 로딩 깊이와 받침/종유석 효과를 사용하는 실행기다.</summary>
    private RawIslandPostPop? _surfaceExecutor;

    /// <summary>현재 TYPE/진단과 같은 장기 Graph·실제 접두만 연결하고 효과를 앞당기지 않는다.</summary>
    public RawPostPop PostPopExecutor()
    {
        if (_postPopExecutor is null)
        {
            var post = new RawPostPop(Pool, Types, PostPopState, GraphExecutor(), sid => ConnectionsExecutor().Connect(sid));
            if (_edition == OriginalEdition.Patch1078)
            {
                post.BindCurrentTypes(Types);
                if (Source() is { } source) post.BindLoadedCosts(source.LoadedTypes ?? throw new InvalidOperationException("실제 최초 자산 TYPE 표 누락"));
            }
            post.BindManaDiagnostics(new() { Checking = new(() => Checking().Read()), Assertion = AssertDisplay });
            post.SetIslandPrefix((sid, flags) => ConnectionsExecutor().IslandPostPopPrefix(sid, flags)); post.SetSurfacePrefix((sid, flags) => SurfaceExecutor().Prefix(sid, flags));
            _postPopExecutor = post;
        }
        return _postPopExecutor;
    }
    /// <summary>실제 표시/후처리·건물 공간을 한 등록기에 연결하고 PATCH만 공통 송신을 사용한다.</summary>
    public RawPop PopExecutor()
    {
        if (_popExecutor is null)
        {
            var pop = new RawPop(Pool, Hash, Spots, DisplayExecutor(), PostPopExecutor());
            pop.BindDiagnostics(new() { Checking = new(() => Checking().Read()), Types = Types, Metamap = () => OwnerMode.Challenge, Trace = TraceGraph, Assertion = AssertDisplay });
            pop.SetBuildingSide(BuildingExecutor()); if (_edition == OriginalEdition.Patch1078) pop.BindTransmission(TransmitPop); _popExecutor = pop;
        }
        return _popExecutor;
    }
    /// <summary>같은 세 실행기를 빌려 프레임 변경 뒤 공간 재등록/표시 순서를 보존한다.</summary>
    public SquidFrame FrameExecutor()
    {
        if (_frameExecutor is null)
        {
            var frame = new SquidFrame(Pool, Types, SquidFrameBinding.MakeSquidFrameHooks(Pool, Types, DisplayExecutor(), UnpopExecutor(), PopExecutor()));
            if (_edition == OriginalEdition.Patch1078) frame.BindCurrentTypes(Types); _frameExecutor = frame;
        }
        return _frameExecutor;
    }
    /// <summary>같은 장기 공장/소유자/Pop/프레임/건물에 원문 표면 효과를 연결한다.</summary>
    private BridgeConnectHooks SurfaceObjects()
    {
        Action<int, uint> owner = RawAssetOwner.MakeAssetOwnerDispatch(Pool, Links, PostPopState.ProductionDirtyBox, (sid, player) => OwnerExecutor().Set(sid, player));
        return new()
        {
            Create = (type, flags) => { NoteChanged(); return FactoryExecutor().Create(type, (SidAllocation)flags); },
            SetOwner = (sid, player) => { owner(sid, player); NoteChanged(); },
            Pop = (sid, x, y, flags) => PopAsset(sid, x, y, flags),
            SetFrame = (sid, frame, flags) => { FrameExecutor().Set(sid, frame, flags); NoteChanged(); },
            NotifySurface = sid => BuildingExecutor().NotifySurface(sid),
        };
    }
    /// <summary>한 이웃 조회기와 실제 생성/공간 효과를 연결하며 World를 만들지 않는다.</summary>
    public RawBridgeConnect ConnectionsExecutor() => _connectionsExecutor ??= new(Pool, NeighborsExecutor(), Links, SurfaceObjects());
    /// <summary>현재 실제 받침/표면 조회·표시와 장기 진단을 같은 표면 지도에 연결한다.</summary>
    public RawIslandPostPop SurfaceExecutor()
    {
        if (_surfaceExecutor is null)
        {
            var hooks = new IslandPostPopHooks
            {
                Objects = SurfaceObjects(), Connect = sid => ConnectionsExecutor().Connect(sid), FindTypeAt = (x, y, type) => ConnectionsExecutor().FindTypeAt(x, y, type),
                UpdateDisplay = sid => DisplayExecutor().Update(Pool.Slot(sid)), IncompleteSupport = (_, _, _) => throw new InvalidOperationException("현재 표면의 불완전한 받침 묶음"),
                Assertion = AssertDisplay, Detail = RecordAssertionDetail, TypeWas = () => GameAux().SupportTypeWas,
            };
            _surfaceExecutor = new(Pool, new SurfaceMapView(Hash), Types, Frames, Links, IslandPostPopState, hooks);
        }
        return _surfaceExecutor;
    }
    /// <summary>현재 raw 프레임의 실제 SHP를 읽어 같은 공간에 등록하고 변경 때만 기존 World 관찰을 알린다.</summary>
    public void PopAsset(int sid, float x, float y, uint flags)
    {
        RawPopResult result;
        if (_edition == OriginalEdition.Patch1078) result = PopExecutor().PopCurrent(sid, x, y, flags);
        else
        {
            ReadOnlySpan<byte> raw = Pool.Slot(sid); float[] size = DisplayExecutor().FrameSize(_edition, raw, raw[34]);
            result = PopExecutor().Pop(sid, Types[raw[10]], size[0], size[1], x, y, flags);
        }
        if (result != RawPopResult.Unchanged) NoteChanged();
    }

    /// <summary>같은 Hash0의 현재 WORD를 복사 지도 없이 읽는 관리 어댑터다.</summary>
    /// <param name="hash">실제 장기 공간 해시이며 이 보기보다 오래 산다.</param>
    private sealed class SurfaceMapView(RawSquidHash hash) : IReadOnlyList<ushort>
    {
        /// <summary>현재 실제 0단계 지도 크기다.</summary>
        public int Count => hash.Entries(0).Length;
        /// <summary>선택 시점의 같은 실제 칸을 읽는다.</summary>
        public ushort this[int index] => hash.Entries(0)[index];
        /// <summary>현재 지도 칸을 원문 행 순서로 열거한다.</summary>
        public IEnumerator<ushort> GetEnumerator()
        {
            // 각 칸을 실제 배열에서 다시 읽고 독립 사본으로 만들지 않는다.
            for (int index = 0; index < Count; index++) yield return this[index];
        }
        /// <summary>비제네릭 호출도 같은 현재 열거기를 사용한다.</summary>
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => GetEnumerator();
    }
}
