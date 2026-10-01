using Netstorm.Core.Rules;
using Netstorm.Core.Bridges;

namespace Netstorm.Core.Simulation;

/// <summary>플레이 화면의 일반 이동·정지 명령.</summary>
public sealed partial class BattleSession
{
    /// <summary>워크샵 단계마다 필요한 업그레이드 비용. 원본 도움말의 1,000 SP를 사용한다.</summary>
    public const int WorkshopUpgradeCost = 1000;

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

    /// <summary>선택한 이동체를 목표 칸으로 보낸다. 실패하면 기존 작업을 유지한다.</summary>
    private CommandResult ExecuteMoveEntity(MoveEntityCommand command)
    {
        CommandResult check = CheckMobile(command.Player, command.EntityId, out GameEntity? entity);
        if (!check.Accepted) return check;
        var goal = new Footprint(command.X, command.Y, 1, 1);
        if (command.X < 0 || command.Y < 0 || command.X >= BridgeGrid.WorldSize || command.Y >= BridgeGrid.WorldSize)
            return new CommandResult(CommandFailure.NoRoute);
        List<(int X, int Y)>? path = FindMovePath(entity!, goal, exact: true);
        if (path == null) return new CommandResult(CommandFailure.NoRoute);
        _harvestTasks.Remove(entity!.Id);
        _moveTasks[entity.Id] = new UnitMoveTask(entity.Id, UnitMovePurpose.MoveToCell, 0, 0, path, Bridges.Version, goal);
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
