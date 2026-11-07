// 일반 지형 분기·요청·농지 후처리 진단과 진단 묶음을 공유한다.
namespace Netstorm.Core.Original;

// 일반 지형 요청 모듈을 담는 네임스페이스다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.

// 호출자가 실제 상위 분기를 확인한 결과다. 군도와 미복원 부모는 정상 지형으로 대체하지 않는다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.
public enum NormalTerrainBranch
{
    /// <summary>일반 성장 분기를 실행한다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    Normal,

    /// <summary>섬 확장·플레이어 배치를 포함하므로 이 소비자는 거부한다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    Archipelago,

    /// <summary>전체 Fort/IslandBuilder 로드 완료로 해석할 수 없는 요청을 거부한다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    UnrestoredParent,
}

// 원본 일반 wrapper에 들어가는 소유자·청크 범위·선택 지역과 확인한 분기다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.
public readonly record struct NormalTerrainRequest(
    /// <summary>context가 저장하는 요청 소유자이며 초기 지면의 owner 0과 구별한다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    uint Owner,
    /// <summary>right/bottom을 제외하는 현재 실제 청크 범위다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    ChunkRect Bounds,
    /// <summary>현재 noRegion이면 모든 섬의 후처리를 실행한다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    int SelectedRegion,
    /// <summary>호출자가 정상 분기임을 명시해야 한다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    NormalTerrainBranch Branch)
{
    /// <summary>기본 요청은 미복원 부모 분기다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    public static NormalTerrainRequest Unrestored => new(0u, default, 127, NormalTerrainBranch.UnrestoredParent);
}

// 기존 Bounds/첫 record helper가 보고하는 진단을 서로 구분한다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.
public enum TerrainFarmPostpassAssertion
{
    /// <summary>CD 첫 record 조회의 현재 Exists 검사가 실패했다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    RecordMissingIsland,

    /// <summary>CD 첫 record 조회가 해당 지역 청크를 찾지 못했다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    RecordNotFound,

    /// <summary>두 판본의 Bounds 앞 현재 Exists 검사가 실패했다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    BoundsMissingIsland,

    /// <summary>CD Bounds의 found assertion이 실패했다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    BoundsNotFound,
}

// 실제 진단 경계는 모두 필수이며 생성/HP/프레임/공간 효과를 대신 제공하지 않는다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.
public sealed class NormalTerrainDiagnostics
{
    /// <summary>원본 context의 bounds 진단이다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    public Action<TerrainContextAssertion>? Context { get; set; }

    /// <summary>실제 frame 검색과 stamp 타입 진단이다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    public Action<TerrainStampAssertion>? Stamp { get; set; }

    /// <summary>원본 지역 setter의 판본별 진단이다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    public Action<SquidRegionAssert>? Region { get; set; }

    /// <summary>군집 내부 지역 seed 조회의 진단이다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    public Action<FarmClusterAssertion>? Farm { get; set; }

    /// <summary>상위의 첫 record/Bounds 조회 진단이다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    public Action<TerrainFarmPostpassAssertion>? Postpass { get; set; }

    /// <summary>현재 Canon 진단 owner를 같은 섬/지형/설치 수명 동안 빌린다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    public CanonDecoderDiagnostics? DecoderDiagnostics { get; set; }

    /// <summary>현재 진단 묶음의 얕은 사본을 만든다. 2026-11-07 F2c 일반지형 바인딩 요청형 이식에서 추가됐다.</summary>
    /// <returns>복사된 진단 묶음이다.</returns>
    public NormalTerrainDiagnostics Clone() => new()
    {
        Context = Context,
        Stamp = Stamp,
        Region = Region,
        Farm = Farm,
        Postpass = Postpass,
        DecoderDiagnostics = DecoderDiagnostics,
    };
}
