using Netstorm.Core.Rules;
using Netstorm.Core.Bridges;
using Netstorm.Assets;

namespace Netstorm.Core.Simulation;

/// <summary>플레이 화면의 일반 이동·정지 명령.</summary>
public sealed partial class BattleSession
{
    /// <summary>바람·비·번개 워크샵의 업그레이드 비용.</summary>
    public const int WorkshopUpgradeCost = 1000;
    /// <summary>녹화의 Sun Workshop 메뉴에 표시된 업그레이드 비용.</summary>
    public const int SunWorkshopUpgradeCost = 800;
    /// <summary>태양 워크샵의 800 SP 예외를 메뉴와 실제 명령 처리에 함께 적용한다.</summary>
    public static int WorkshopUpgradeCostFor(TypeDefinition type) =>
        Elements.FromTheme(type.GetString("theme")) == Element.Sun ? SunWorkshopUpgradeCost : WorkshopUpgradeCost;

    /// <summary>일반 이동·정지를 명령할 수 있는 내 자유 이동체인지 검사한다.</summary>
    private CommandResult CheckMobile(int owner, int id, out GameEntity? entity)
    {
        entity = Entity(id);
        if (entity == null) return new CommandResult(CommandFailure.NoSuchEntity);
        if (entity.Owner != owner) return new CommandResult(CommandFailure.NotOwner);
        if (entity.Kind is not (ObjectKind.Priest or ObjectKind.Transport)) return new CommandResult(CommandFailure.WrongKind);
        if (entity.IsStunned || entity.Captivity != PriestCaptivity.Free) return new CommandResult(CommandFailure.NotCapturable);
        if (!entity.IsComplete) return new CommandResult(CommandFailure.NotComplete);
        return CommandResult.Ok();
    }

    /// <summary>
    /// 목표 발자국까지의 길을 정한다. 걷는 도중이면 진행 중인 걸음을 마저 걷고(칸 사이에서 뒤로 튀지 않게) 그 다음 칸에서 새 경로를 잇는다.
    /// 다음 칸에서 길이 없으면 현재 칸에서 다시 찾는다. 길이 없으면 null.
    /// </summary>
    /// <param name="entity">움직일 오브젝트</param>
    /// <param name="goal">목표 발자국</param>
    /// <param name="exact">목표 칸 자체까지(true) 또는 목표 둘레까지(false)</param>
    /// <param name="carried">새 작업이 이어받을 걸음 진행량 (이어 걷지 않으면 0)</param>
    private List<(int X, int Y)>? PlanPath(GameEntity entity, Footprint goal, bool exact, out double carried)
    {
        MovementRoute? current = RouteOf(entity.Id);
        (int X, int Y)? stepEnd = null;
        carried = 0;
        if (current is { IsBlocked: false, Progress: > 0 } && current.NextIndex < current.Path.Count
            && current.Path[current.NextIndex - 1] == (entity.Footprint.AnchorX, entity.Footprint.AnchorY))
        {
            stepEnd = current.Path[current.NextIndex];
            carried = current.Progress;
        }
        List<(int X, int Y)>? path = stepEnd is { } next
            ? FindMovePath(entity, goal, exact, new Footprint(next.X, next.Y, 1, 1))
            : FindMovePath(entity, goal, exact);
        // 다음 칸에서 길이 없으면 현재 칸에서 다시 찾아 본다.
        if (path == null && stepEnd != null)
        {
            stepEnd = null;
            carried = 0;
            path = FindMovePath(entity, goal, exact);
        }
        if (path != null && stepEnd != null) path.Insert(0, (entity.Footprint.AnchorX, entity.Footprint.AnchorY));
        return path;
    }

    /// <summary>선택한 이동체를 목표 칸으로 보낸다. 실패하면 기존 작업을 유지한다.</summary>
    private CommandResult ExecuteMoveEntity(MoveEntityCommand command)
    {
        CommandResult check = CheckMobile(command.Player, command.EntityId, out GameEntity? entity);
        if (!check.Accepted) return check;
        var goal = new Footprint(command.X, command.Y, 1, 1);
        if (command.X < 0 || command.Y < 0 || command.X >= BridgeGrid.WorldSize || command.Y >= BridgeGrid.WorldSize)
            return new CommandResult(CommandFailure.NoRoute);
        List<(int X, int Y)>? path = PlanPath(entity!, goal, exact: true, out double carried);
        if (path == null) return new CommandResult(CommandFailure.NoRoute);
        _harvestTasks.Remove(entity!.Id);
        _moveTasks[entity.Id] = new UnitMoveTask(entity.Id, UnitMovePurpose.MoveToCell, 0, 0, path, Bridges.Version, goal) { Progress = carried };
        return CommandResult.Ok();
    }

    /// <summary>운반물을 그대로 둔 채 이동·수확을 중지한다.</summary>
    private CommandResult ExecuteStopEntity(StopEntityCommand command)
    {
        CommandResult check = CheckMobile(command.Player, command.EntityId, out GameEntity? entity);
        if (!check.Accepted) return check;
        _harvestTasks.Remove(entity!.Id); _moveTasks.Remove(entity.Id);
        return CommandResult.Ok();
    }
}
