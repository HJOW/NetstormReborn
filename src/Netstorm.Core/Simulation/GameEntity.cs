using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 게임 세션 안의 오브젝트 하나 (맵에 저장되어 있던 것 또는 게임 중 놓거나 지은 것).
/// 모양·비용·발자국 같은 값은 .type 정의(<see cref="Type"/>)에서 읽는 데이터 구동 방식이다.
/// 사제의 위치·운반 결정, 체력·기절 및 포대의 목표·재장전 상태를 저장한다.
/// </summary>
public sealed class GameEntity
{
    /// <summary>오브젝트 번호 (세션 안에서 유일, BattleMap 과 같은 번호 체계)</summary>
    public int Id { get; }

    /// <summary>.type 타입 (로딩 목록의 항목)</summary>
    public TypeInfo Type { get; }

    /// <summary>규칙상 분류 (템플·워크샵·유닛 종류 등)</summary>
    public ObjectKind Kind { get; }

    /// <summary>소유 플레이어 번호 (없으면 0)</summary>
    public int Owner { get; }

    /// <summary>차지하는 칸 (기준점은 오른쪽 아래 칸)</summary>
    public Footprint Footprint { get; internal set; }

    /// <summary>사제가 현재 운반 중인 Storm Crystal 수(현재 경로 모델은 0 또는 1).</summary>
    public int CarriedCrystals { get; internal set; }

    /// <summary>기준점 칸이 속한 섬 영역 번호 (섬 밖이면 null)</summary>
    public int? Territory { get; }

    /// <summary>맵 파일에 저장되어 있던 오브젝트면 그 원본, 게임 중 새로 만든 것이면 null</summary>
    public FortMapObject? Source { get; }

    /// <summary>건설이 끝났는지 (게임 중 놓은 유닛은 곧바로 완성, 사제가 짓는 건물은 건설 시간이 지나야 완성)</summary>
    public bool IsComplete { get; internal set; } = true;

    /// <summary>건설이 끝나는 틱 (완성된 오브젝트는 0)</summary>
    public long CompleteTick { get; internal set; }

    /// <summary>건설이 시작된 틱 (건설 진행률 계산용, 처음부터 완성인 오브젝트는 0)</summary>
    public long StartTick { get; internal set; }

    /// <summary>타입의 최대 체력. 0이면 현재 전투 모델에서 피해 대상이 아니다.</summary>
    public double MaxHitPoints { get; }

    /// <summary>현재 체력. 원본 저장 맵은 최대 체력으로 시작한다.</summary>
    public double HitPoints { get; internal set; }

    /// <summary>체력 절반에서 보호막을 두른 사제인지. 포획은 후속 구현이다.</summary>
    public bool IsStunned { get; internal set; }

    /// <summary>유지 중인 사격 목표 번호. 0이면 목표가 없다.</summary>
    public int AttackTargetId { get; internal set; }

    /// <summary>다음 발사를 허용할 틱.</summary>
    public long NextAttackTick { get; internal set; }

    /// <summary>기지가 생성한 공중 공격체의 비행 상태. 지상 오브젝트는 null이다.</summary>
    public FlyerFlight? Flight { get; internal set; }

    /// <summary>전투·표시에 쓰는 중심 x. 비행체는 칸 사이도 연속 이동한다.</summary>
    public double WorldX => Flight?.X ?? Footprint.CenterX;

    /// <summary>전투·표시에 쓰는 중심 y.</summary>
    public double WorldY => Flight?.Y ?? Footprint.CenterY;

    /// <summary>오브젝트를 만든다</summary>
    /// <param name="id">오브젝트 번호</param>
    /// <param name="type">타입</param>
    /// <param name="kind">분류</param>
    /// <param name="owner">소유 플레이어</param>
    /// <param name="footprint">차지하는 칸</param>
    /// <param name="territory">섬 영역 번호</param>
    /// <param name="source">맵 파일의 원본 오브젝트 (없으면 null)</param>
    internal GameEntity(int id, TypeInfo type, ObjectKind kind, int owner, Footprint footprint, int? territory, FortMapObject? source)
    {
        Id = id;
        Type = type;
        Kind = kind;
        Owner = owner;
        Footprint = footprint;
        Territory = territory;
        Source = source;
        MaxHitPoints = Math.Max(0, type.Definition.GetDouble("maxHitPoints") ?? 0);
        HitPoints = MaxHitPoints;
    }

    /// <summary>화면에 보이는 설명 이름 (.type 의 description, 없으면 타입 이름)</summary>
    public string DisplayName => Type.Definition.GetString("description") ?? Type.Name;

    /// <summary>회수·파괴 때의 비용 기준 값 (.type 의 cost, 없으면 0)</summary>
    public int Cost => Type.Definition.GetInt("cost") ?? 0;
}
