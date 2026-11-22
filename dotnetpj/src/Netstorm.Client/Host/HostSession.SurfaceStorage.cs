using Netstorm.Assets;
using Netstorm.Core.Original;

// 현재 Host가 장기 Surface의 물리와 원문 상태를 복제 없이 빌리는 경계다.
namespace Netstorm.Client.Host;

/// <summary>독립 조합의 기본값과 실제 Client가 준비한 상태의 대여를 구별한다.</summary>
public sealed partial class HostSession
{
    /// <summary>외곽 Runtime이 대여하는 같은 장기 저장소다. 세션 생성/반납은 이 상태를 초기화하지 않는다.</summary>
    private readonly RawSurfaceStorage? _surfaceStorage;
    /// <summary>독립 조합만 소유하는 로딩 깊이다. 표면/사제/Template 조회도 같은 주소를 읽는다.</summary>
    private readonly UIntBox _localWorldLoadingDepth = new();

    /// <summary>Player 고정뷰를 획득하기 전에 명시 인자가 같은 실제 Client 물리인지 검증한다.</summary>
    private static void ValidateSharedSurface(ClientSurfaceGlobals? surface, Kernel kernel, ClientGameplayGlobals? gameplay,
        RawStateSidLists? lists, RawSidPool? pool, RawProcessHostStorage? processes, CampaignTemplateStorage? templates,
        DoubleBox? processTime, ClientLocalStateGlobals? local, ClientConnectionGlobals? connections)
    {
        if (surface is null) return;
        RawSurfaceStorage storage = surface.Storage;
        if (!ReferenceEquals(surface.Kernel, kernel) || (gameplay is not null && !ReferenceEquals(surface.Gameplay, gameplay))
            || (lists is not null && !ReferenceEquals(storage.SidLists, lists)) || (pool is not null && !ReferenceEquals(storage.Pool, pool))
            || (processes is not null && !ReferenceEquals(surface.Processes, processes))
            || (processTime is not null && !ReferenceEquals(surface.Clock.GameTime, processTime))
            || (local is not null && !ReferenceEquals(surface.Clock.LocalState, local))
            || (connections is not null && !ReferenceEquals(surface.Clock.LocalState, connections.LocalState)))
            throw new ArgumentException("Host의 동일 Surface/Kernel/Gameplay/Pool/목록/프로세스 소유자 필요", nameof(surface));
        if (templates is null || !ReferenceEquals(storage.LoadingDepthBox, templates.SerializationGlobals.LoadingDepthBox))
            throw new ArgumentException("Host의 동일 Surface/Template 로딩 깊이 소유자 필요", nameof(templates));
    }

    /// <summary>공유 Host가 다른 TYPE/공장/공간을 섞기 전에 거부하고 생략된 장치는 같은 저장소에서 선택한다.</summary>
    private void SelectSharedTerrainDevices(IReadOnlyList<OriginalTypeRecord> types, List<SquidDisplayShape> shapes,
        ref RawSquidFactory? factory, ref RawSquidHash? hash, ref byte[]? spots)
    {
        if (_surfaceStorage is not { } storage) return;
        if (!ReferenceEquals(types, storage.Types) || !ReferenceEquals(shapes, storage.Shapes)
            || (factory is not null && !ReferenceEquals(factory, storage.FactoryExecutor()))
            || (hash is not null && !ReferenceEquals(hash, storage.Hash)) || (spots is not null && !ReferenceEquals(spots, storage.Spots)))
            throw new ArgumentException("공유 Host 지형의 동일 TYPE/셰이프/공장/해시/점유지 필요");
        factory ??= storage.FactoryExecutor(); hash ??= storage.Hash; spots ??= storage.Spots;
    }

    /// <summary>요새 객체도 같은 실제 TYPE/공장/건물 공간을 빌려 삭제·Graph와 다른 풀을 섞지 않는다.</summary>
    private void SelectSharedFortDevices(IReadOnlyList<OriginalTypeRecord> types, ref RawSquidFactory? factory, ref RawBuildingSpace? building)
    {
        if (_surfaceStorage is not { } storage) return;
        if (!ReferenceEquals(types, storage.Types) || (factory is not null && !ReferenceEquals(factory, storage.FactoryExecutor()))
            || (building is not null && !ReferenceEquals(building, storage.BuildingExecutor())))
            throw new ArgumentException("공유 Host 요새의 동일 TYPE/공장/건물 공간 필요");
        factory ??= storage.FactoryExecutor(); building ??= storage.BuildingExecutor();
    }
}
