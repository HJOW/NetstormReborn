// World보다 오래 사는 장부/삭제/로딩 상태를 같은 실제 전역에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>현재 Native/독립 World가 빌리는 공통 상태와 초기 대여 장치다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 세 통계표·정적 공급원/공장·비용·깊이를 모든 World가 빌리는 장기 본체다.</summary>
    public RawPostPopState PostPopState { get; }
    /// <summary>같은 손실 좌표와 Graph 재구성 정책을 빌리는 장기 삭제 상태다.</summary>
    public SquidDeletionState DeletionState { get; }
    /// <summary>표면 접두가 같은 로딩 깊이/저장 게임을 읽는 장기 상태다.</summary>
    public IslandPostPopState IslandPostPopState { get; } = new() { LoadingDepth = 1 };
    /// <summary>독립 terrain 입력과 현재 실제 타입/진단이 함께 쓰는 건물 공간 상태다.</summary>
    public BuildingSpaceState BuildingSpaceState { get; } = new();
    /// <summary>독립 표면의 종전 로딩 깊이1이며 일반 대여에서 덮어쓰지 않는다.</summary>
    private readonly UIntBox _localLoadingDepth = new() { Value = 1 };
    /// <summary>현재 Template/Core의 같은 직렬화 전역 owner다.</summary>
    private FortSerializationGlobals? _serializationGlobals;
    /// <summary>로딩 깊이의 현재 실제 DWORD owner다. Template이 없으면 기존 독립 owner다.</summary>
    public UIntBox LoadingDepthBox => _serializationGlobals?.LoadingDepthBox ?? _localLoadingDepth;
    /// <summary>같은 현재 로딩 깊이를 직접 읽고 쓴다.</summary>
    public uint LoadingDepth { get => LoadingDepthBox.Value; set => LoadingDepthBox.Value = value; }
    /// <summary>원래 owner 주소만 연결하고 대여/게임 값을 초기화하지 않는다.</summary>
    public void BindSerializationGlobals(FortSerializationGlobals globals)
    {
        ArgumentNullException.ThrowIfNull(globals);
        if ((_serializationGlobals is not null && !ReferenceEquals(_serializationGlobals, globals)) || (_borrowed && _serializationGlobals is null))
            throw new ArgumentException("표면/Template의 동일 로딩 깊이 owner 대여 계약", nameof(globals));
        _serializationGlobals = globals;
    }
    /// <summary>일반 fresh 상태 대입을 같은 본체에서 수행하며 준비 대여에는 호출하지 않는다.</summary>
    private void ResetSharedStatesForGeneralWorld()
    {
        // 장기 Native 장치의 설치/보유는 후속 공간 조립에서 연결하며 현재 독립 World의 기본 관문은 null이다.
        PostPopState.ResetForGeneralWorld(_gameAux.TotalCostBox, SidLists.Providers, SidLists.Factories, null);
        SquidDeletionState state = DeletionState;
        state.Selected = 0; state.PlacementType = 0; state.PlacementX = 0; state.PlacementY = 0; state.LostX = 0; state.LostY = 0;
        state.UnitLostSuppressed1 = false; state.UnitLostSuppressed2 = false; state.SilentType = 0; state.RegionMask = 0x50444200; state.RebuildGraph = false;
        state.SpecialSurfaceType = 157; state.SpecialSurfaceReplacementType = 162; state.LargeSurfaceType = 162;
        state.SharedLostX = _gameAux.LostXBox; state.SharedLostY = _gameAux.LostYBox; state.SharedLoadingDepth = null; state.SharedFortMode = null;
        state.SharedSilentType = null; state.SharedSpecialSurfaceType = null; state.SharedReplacementSurfaceType = null; state.SharedLargeSurfaceType = null;
        IslandPostPopState.SharedLoadingDepth = null; IslandPostPopState.LoadingDepth = 1;
        IslandPostPopState.OwnerPropagationSuppressed = false; IslandPostPopState.SavedGameSource = null;
    }
}
