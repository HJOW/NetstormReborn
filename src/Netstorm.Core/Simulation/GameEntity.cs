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

    /// <summary>체력 절반에서 보호막을 두른 사제인지. 수송 유닛이 이 상태의 적 사제를 포획한다.</summary>
    public bool IsStunned { get; internal set; }

    /// <summary>발판을 잃어 허공에서 기절한 자유 사제. 지상 점유·신전 회복에서 제외한다.</summary>
    public bool IsSuspended { get; internal set; }

    /// <summary>지상 배치를 막는지. 비행 수송·공격체·포로·허공 사제는 지상을 점유하지 않는다.</summary>
    public bool OccupiesGround => Kind != ObjectKind.Flyer && !Type.Definition.HasFlag("balloon")
        && Captivity == PriestCaptivity.Free && !IsSuspended;

    /// <summary>사제의 포획 상태 (자유 / 수송 유닛이 운반 중 / 제단에 묶임).</summary>
    public PriestCaptivity Captivity { get; internal set; }

    /// <summary>포획된 사제를 잡고 있는 오브젝트 번호 (운반 중이면 수송 유닛, 묶였으면 제단). 자유면 0.</summary>
    public int CaptorId { get; internal set; }

    /// <summary>수송 유닛이 운반 중인 사제 번호. 없으면 0.</summary>
    public int CarriedPriestId { get; internal set; }

    /// <summary>유지 중인 사격 목표 번호. 0이면 목표가 없다.</summary>
    public int AttackTargetId { get; internal set; }

    /// <summary>다음 발사를 허용할 틱.</summary>
    public long NextAttackTick { get; internal set; }

    /// <summary>0=북, 1=동, 2=남, 3=서. 고정 캐논·Crossbow의 사격 방위와 Wind Tower의 면역 면은 설치 후 유지한다.</summary>
    public int CannonDirection { get; internal set; }

    /// <summary>현재 캐논 사격 동작의 시작 틱. -1이면 동작하지 않는다.</summary>
    public long AttackStartedTick { get; internal set; } = -1;

    /// <summary>마지막 발사 틱. -1이면 아직 발사하지 않았다. 발사 그림을 피해 예약과 맞춘다.</summary>
    public long LastShotTick { get; internal set; } = -1;

    /// <summary>태양 캐논의 실제 접기·회전·발사 그림 번호. 조준 방위와 별도로 저장해 중간 자세를 유지한다.</summary>
    public int SunCannonFrame { get; internal set; }

    /// <summary>태양 캐논이 다음 회전·발사 그림으로 진행할 틱.</summary>
    public long NextSunAnimationTick { get; internal set; }

    /// <summary>석궁의 현재 조준·장전 그림 번호. 설치 사격 방위와 별개로 목표를 따라 움직인다.</summary>
    public int CrossbowFrame { get; internal set; }

    /// <summary>석궁이 장전 그림을 진행하며 발사를 준비 중인지.</summary>
    public bool CrossbowFiring { get; internal set; }

    /// <summary>석궁이 다음 조준·장전 그림으로 진행할 틱.</summary>
    public long NextCrossbowAnimationTick { get; internal set; }

    /// <summary>파괴된 아이스 타워가 같은 받침에서 다시 자라는지.</summary>
    public bool IsRegenerating { get; internal set; }

    /// <summary>아이스 타워 재성장의 시작 틱.</summary>
    public long RegenerationStartTick { get; internal set; }

    /// <summary>기지가 생성한 공중 공격체의 비행 상태. 지상 오브젝트는 null이다.</summary>
    public FlyerFlight? Flight { get; internal set; }

    /// <summary>
    /// 마지막으로 걸어간 방향 (<see cref="UnitHeading"/>, 움직인 적이 없으면 <see cref="UnitHeading.None"/>).
    /// 화면이 걷는 그림의 방향을 고르는 데만 쓰며 규칙·검사합과 무관하다.
    /// </summary>
    public int Heading { get; internal set; } = UnitHeading.None;

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
        // 저장된 캐논의 L/M/N/O 측면 글자를 북/동/남/서로 읽는다. 기본 그림도 같은 규칙이다.
        int frame = source?.Object.Frame ?? type.Definition.Frames.DefaultFrame;
        if (type.Name.Equals("sunCannon", StringComparison.OrdinalIgnoreCase))
            SunCannonFrame = frame is >= 0 and < 28 ? frame : 0;
        if (frame >= 0 && frame < type.Definition.Frames.Codes.Count &&
            type.Definition.Frames.Codes[frame].Side is >= 'L' and <= 'O' and var side)
            CannonDirection = side - 'L';
        // Crossbow의 설치 방위는 묶음 글자가 아니라 원본의 실제 20각도표로 복원한다.
        if (type.Name.Equals("windArcher", StringComparison.OrdinalIgnoreCase))
        {
            CannonDirection = EmplacementDirection.SavedDirection(type, frame);
            CrossbowFrame = frame is >= 0 and < 100 ? frame : 0;
        }
        // 가이저는 타입 cost 만큼의 Storm Power 를 품는다. 싱글 플레이 원본 값(exe 타입 로더 → FUN_004b2df0, geyser.type cost = 2000).
        if (kind == ObjectKind.Geyser) StoredStormPower = type.Definition.GetInt("cost") is int stored and > 0 ? stored : int.MaxValue;
    }

    /// <summary>가이저에 남은 Storm Power. 가이저가 아니면 0. 0 이 되면 빈 가이저(emptyGeyser)가 된다.</summary>
    public int StoredStormPower { get; internal set; }

    /// <summary>다 써서 비어 버린 가이저인지 (원본 "Empty Storm Geyser").</summary>
    public bool IsDepletedGeyser => Kind == ObjectKind.Geyser && StoredStormPower <= 0;

    /// <summary>화면에 보이는 설명 이름 (.type 의 description, 없으면 타입 이름)</summary>
    public string DisplayName => Type.Definition.GetString("description") ?? Type.Name;

    /// <summary>회수·파괴 때의 비용 기준 값 (.type 의 cost, 없으면 0)</summary>
    public int Cost => StormPower.TypeCost(Type.Definition);

    /// <summary>재성장 중인 아이스 타워는 원본 growingRainBlocker처럼 회수 대금이 없다.</summary>
    public int SalvageRefund => IsRegenerating ? 0 : StormPower.SalvageValue(Cost);
}

/// <summary>사제의 포획 상태 (도움말 "How To Capture and Sacrifice", docs/gameplay/sacrifice.md).</summary>
public enum PriestCaptivity
{
    /// <summary>자유 (기절 여부와 무관)</summary>
    Free,

    /// <summary>수송 유닛이 운반 중. 운반 유닛에게서 생명력을 얻어 풀려나면 완전히 회복한다.</summary>
    Carried,

    /// <summary>제단의 희생의 원(Sacrificial Circle)에 묶임.</summary>
    Bound,
}
