// 도착 부모의 현재 권한/동맹/TYPE와 큰 하위 효과의 실제 함수 값을 보관한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawCarrierArrival.h의 현재 mutable 상태다.</summary>
public sealed class CarrierArrivalState
{
    /// <summary>독립 원문 입력의 권한이다.</summary>
    public bool Boss { get; set; } = true;
    /// <summary>실제 장기 Client의 현재 권한 조회다.</summary>
    public Func<bool>? SharedBoss { get; set; }
    /// <summary>PATCH의 현재 Robust 진단 뷰다.</summary>
    public RawCheckingFlag Debug { get; set; } = new();
    /// <summary>독립 입력의 동맹 사제 포획 허용이다.</summary>
    public bool AllowAlliedPickup { get; set; }
    /// <summary>현재 사제 TYPE다.</summary>
    public uint PriestType { get; set; } = 158;
    /// <summary>독립 현재 너겟 TYPE다.</summary>
    public uint NuggetType { get; set; } = 154;
    /// <summary>현재 제단 TYPE다.</summary>
    public uint DaisType { get; set; } = 165;
    /// <summary>독립 현재 간헐천 TYPE다.</summary>
    public uint GeyserType { get; set; } = 122;
    /// <summary>현재 아웃포스트 TYPE다.</summary>
    public uint OutpostType { get; set; } = 126;
    /// <summary>현재 회피 장애물 TYPE다.</summary>
    public uint IsleStalagType { get; set; } = 84;
    /// <summary>원문 방향별9×9 독립 동맹 DWORD다.</summary>
    public uint[] Alliances { get; } = new uint[81];
    /// <summary>원문 독립 재생의 Player AI 존재 입력이다.</summary>
    public bool[] ControlledOwners { get; } = new bool[9];
    /// <summary>실제 포획 허용 DWORD의 비영 전체를 조회한다.</summary>
    public Func<int>? SharedAllowAlliedPickup { get; set; }
    /// <summary>실제 장기 간헐천 TYPE owner다.</summary>
    public UIntBox? SharedGeyserType { get; set; }
    /// <summary>실제 장기 너겟 TYPE owner다.</summary>
    public UIntBox? SharedNuggetType { get; set; }
    /// <summary>현재 권한을 소비 위치에서 읽는다.</summary>
    public bool CurrentBoss() => SharedBoss?.Invoke() ?? Boss;
    /// <summary>현재 포획 허용을 소비 위치에서 읽는다.</summary>
    public bool CurrentAllowAlliedPickup() => SharedAllowAlliedPickup is { } read ? read() != 0 : AllowAlliedPickup;
    /// <summary>현재 간헐천 TYPE를 소비 위치에서 읽는다.</summary>
    public uint CurrentGeyserType() => SharedGeyserType?.Value ?? GeyserType;
    /// <summary>현재 너겟 TYPE를 소비 위치에서 읽는다.</summary>
    public uint CurrentNuggetType() => SharedNuggetType?.Value ?? NuggetType;
}

/// <summary>큰 포획·귀환·경제·프로토콜·회피 몸체의 실제 호출 경계다.</summary>
public sealed record CarrierArrivalHooks
{
    /// <summary>같은 현재 Host에서 매번 실제 경로를 조회한다.</summary>
    public Func<int, RawPathRuntime?>? Path { get; init; }
    /// <summary>기존 실제 위치/부류 getter다.</summary>
    public Func<float, float, uint, int>? GenusAt { get; init; }
    /// <summary>CD의 현재 priestPickupPriest 설정이다.</summary>
    public Func<bool>? CdPriestPickupPriest { get; init; }
    /// <summary>실제 포획 요청 부모다.</summary>
    public Action<int>? Pickup { get; init; }
    /// <summary>현재 지연·사건으로 회피를 요청하고 원문 상태를 반환한다.</summary>
    public Func<int, float, uint, uint>? Escape { get; init; }
    /// <summary>실제 귀환 후보 선택이다.</summary>
    public Func<int, int>? ChooseReturn { get; init; }
    /// <summary>실제 간헐천 후보 선택이다.</summary>
    public Func<int, int>? ChooseGeyser { get; init; }
    /// <summary>선택된 귀환 장소로 실제 새 이동을 요청한다.</summary>
    public Action<int, int>? GoReturn { get; init; }
    /// <summary>선택된 간헐천으로 실제 새 이동을 요청한다.</summary>
    public Action<int, int>? GoGeyser { get; init; }
    /// <summary>너겟 생성 뒤 지면의 실제 mined 패킷을 보낸다.</summary>
    public Action<int>? MinedPacket { get; init; }
    /// <summary>포획 Form/너겟의 실제 가상10 삭제다.</summary>
    public Action<int, uint>? Destroy { get; init; }
    /// <summary>CD의 현재 owner SP 가산이다.</summary>
    public Action<uint, int>? CdAddSp { get; init; }
    /// <summary>CD의 실제 owner 갱신 통지다.</summary>
    public Action<uint>? CdOwnerPacket { get; init; }
    /// <summary>CD 귀환 장소의 실제 위치 소리다.</summary>
    public Action<float, float, string, uint>? PlayAt { get; init; }
    /// <summary>PATCH의 실제 전체 객체/내용물 Robust 검사다.</summary>
    public Func<int, bool>? Robust { get; init; }
    /// <summary>실제 Player의 현재 AI 존재를 조회한다.</summary>
    public Func<uint, bool>? CurrentControlled { get; init; }
    /// <summary>실제 현재 동맹표를 원문 방향으로 읽는다.</summary>
    public Func<uint, uint, uint>? CurrentAlliance { get; init; }
}
