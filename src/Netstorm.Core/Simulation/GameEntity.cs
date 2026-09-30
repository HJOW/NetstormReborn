using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 게임 세션 안의 오브젝트 하나 (맵에 저장되어 있던 것 또는 게임 중 놓거나 지은 것).
/// 모양·비용·발자국 같은 값은 .type 정의(<see cref="Type"/>)에서 읽는 데이터 구동 방식이다.
/// 이동·체력·전투 상태는 아직 없다 (해당 규칙 분석 후 추가).
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
    public Footprint Footprint { get; }

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
    }

    /// <summary>화면에 보이는 설명 이름 (.type 의 description, 없으면 타입 이름)</summary>
    public string DisplayName => Type.Definition.GetString("description") ?? Type.Name;

    /// <summary>회수·파괴 때의 비용 기준 값 (.type 의 cost, 없으면 0)</summary>
    public int Cost => Type.Definition.GetInt("cost") ?? 0;
}
