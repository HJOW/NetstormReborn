using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 사제 포획 → 제단 운반 → 희생 의식 → 미션 승패 이벤트.
/// 규칙은 원본 도움말(help.english "How To Capture and Sacrifice")과 exe(FUN_00449f40 의식 단계, FUN_004c36c0 승패 판정),
/// 시간은 캠페인 1-1 녹음과 YouTube 영상 측정값이다. 확정·추정 구분은 docs/gameplay/sacrifice.md.
/// </summary>
public sealed partial class BattleSession
{
    /// <summary>의식 시작부터 첫 룬 음성(forWind2)까지(초). 캠페인 1-1 녹음: 희생 음악 12:06.4 → 첫 룬 12:07.6.</summary>
    public const double SacrificeRuneLeadSeconds = 1.2;

    /// <summary>룬 사이 간격(초). 캠페인 1-1 녹음의 다섯 룬 음성 간격 평균 14.8초.</summary>
    public const double SacrificeRuneIntervalSeconds = 14.8;

    /// <summary>룬 음성부터 그 룬이 타서 사라질 때(altarBurnCollapse)까지(초). 영상·녹음 측정 평균은 약 12.1초.</summary>
    public const double SacrificeRuneBurnSeconds = 12.1;

    /// <summary>의식의 룬 수 (도움말 "Your Priest must ward five runes")</summary>
    public const int SacrificeRuneCount = 5;

    /// <summary>의식 완료(itIsDone2) 뒤 희생 음성(priestSacrifice2)까지(초). exe 상수 0x532680 = 4.0, 녹음과 일치.</summary>
    public const double SacrificeKillDelaySeconds = 4.0;

    /// <summary>의식 완료 뒤 제단 소멸까지(초). 캠페인 1-1·1-5 두 관찰의 평균 약 9.3초.</summary>
    public const double AltarConsumeDelaySeconds = 9.3;

    /// <summary>제단이 소멸한 뒤 희생된 사제가 승패 판정에서 제거될 때까지(초). 영상에서 성공 창은 제단 폭발 3초 뒤 떴다.</summary>
    public const double SacrificedPriestRemovalDelaySeconds = 3.0;

    /// <summary>의식 중 제단 체력이 이 비율 아래로 떨어지면 묶인 사제가 달아난다 (도움말 "too much damage", 비율은 임시).</summary>
    public const double AltarBreakHealthRatio = 0.5;

    /// <summary>룬 이름 (원본 음성 forWind2·forSun2·forRain2·forThunder2·forStorm2 순서, 녹음 확인)</summary>
    public static readonly IReadOnlyList<string> SacrificeRuneNames = ["Wind", "Sun", "Rain", "Thunder", "Storm"];

    /// <summary>수송 유닛·사제 이동 작업 (이동하는 오브젝트 번호순)</summary>
    private readonly SortedDictionary<int, UnitMoveTask> _moveTasks = [];

    /// <summary>진행 중인 희생 의식 (제단 번호순)</summary>
    private readonly SortedDictionary<int, AltarRitual> _rituals = [];

    /// <summary>이미 알린 미션 스크립트 섹션 이름 (원본은 이벤트마다 미션당 한 번만 Tell 한다)</summary>
    private readonly SortedSet<string> _toldSections = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>
    /// 살아 있는 것을 한 번이라도 본 대상 (신전·사제·팀 단위, 예: "temple:2", "priest:2", "bad", "good", "me").
    /// 지도에 처음부터 없는 신전·사제를 "죽었다"고 보고 시작하자마자 승패 창이 뜨지 않도록, 사망·전멸 이벤트는
    /// 해당 대상이 살아 있던 적이 있어야 낸다. (원본은 AI 의 신전·사제를 미션 시작에 연결해 두므로 정상 미션에서는 같은 결과다.)
    /// </summary>
    private readonly SortedSet<string> _seenAlive = new(StringComparer.Ordinal);

    /// <summary>이미 알린 미션 이벤트 섹션 이름 (화면·검사용)</summary>
    public IReadOnlyCollection<string> ToldSections => _toldSections;

    /// <summary>진행 중인 희생 의식 목록 (화면 표시·음악 판정용)</summary>
    public IReadOnlyCollection<AltarRitual> Rituals => _rituals.Values;

    /// <summary>플레이어의 제단에서 희생 의식이 진행 중인지 (원본 FUN_00449220: 단계 1 이상 = 희생 음악 유지)</summary>
    /// <param name="player">플레이어</param>
    public bool IsSacrificeInProgress(int player) =>
        _rituals.Values.Any(r => !r.AltarConsumed && Entity(r.AltarId)?.Owner == player);

    /// <summary>이동 중인 오브젝트의 목적 (화면 표시용, 없으면 null)</summary>
    /// <param name="entityId">수송 유닛 또는 사제 번호</param>
    public UnitMovePurpose? MovePurposeOf(int entityId) => _moveTasks.TryGetValue(entityId, out UnitMoveTask? task) ? task.Purpose : null;

    // ───────────────────────── 명령 ─────────────────────────

    /// <summary>수송 유닛을 사제에게 보낸다 (집은 뒤 제단 번호가 있으면 이어서 운반).</summary>
    private CommandResult ExecuteCapturePriest(CapturePriestCommand command)
    {
        CommandResult check = CheckTransport(command.Player, command.TransportId, out GameEntity? transport);
        if (!check.Accepted)
        {
            return check;
        }
        if (transport!.CarriedPriestId != 0)
        {
            return new CommandResult(CommandFailure.AlreadyCarrying);
        }
        GameEntity? priest = Entity(command.PriestId);
        if (priest == null)
        {
            return new CommandResult(CommandFailure.NoSuchEntity);
        }
        if (priest.Kind != ObjectKind.Priest)
        {
            return new CommandResult(CommandFailure.WrongKind);
        }
        if (!CanCapture(command.Player, priest))
        {
            return new CommandResult(CommandFailure.NotCapturable);
        }
        // 허공의 사제는 바로 옆에 있어도 지상 수송으로 집을 수 없다.
        if (priest.IsSuspended && !IsAirborneTransport(transport)) return new CommandResult(CommandFailure.NoRoute);
        if (command.AltarId != 0)
        {
            CommandResult altarCheck = CheckAltar(command.Player, command.AltarId);
            if (!altarCheck.Accepted)
            {
                return altarCheck;
            }
        }
        List<(int X, int Y)>? path = FindMovePath(transport, priest.Footprint);
        if (path == null)
        {
            return new CommandResult(CommandFailure.NoRoute);
        }
        _harvestTasks.Remove(transport.Id);
        _moveTasks[transport.Id] = new UnitMoveTask(transport.Id, UnitMovePurpose.PickUpPriest, priest.Id, command.AltarId, path, Bridges.Version);
        return CommandResult.Ok($"{transport.DisplayName} → {priest.DisplayName} 포획 출발");
    }

    /// <summary>운반 중인 수송 유닛을 제단으로 보낸다.</summary>
    private CommandResult ExecuteDeliverPriest(DeliverPriestCommand command)
    {
        CommandResult check = CheckTransport(command.Player, command.TransportId, out GameEntity? transport);
        if (!check.Accepted)
        {
            return check;
        }
        if (transport!.CarriedPriestId == 0)
        {
            return new CommandResult(CommandFailure.NotCarrying);
        }
        CommandResult altarCheck = CheckAltar(command.Player, command.AltarId);
        if (!altarCheck.Accepted)
        {
            return altarCheck;
        }
        List<(int X, int Y)>? path = FindMovePath(transport, Entity(command.AltarId)!.Footprint);
        if (path == null)
        {
            return new CommandResult(CommandFailure.NoRoute);
        }
        _moveTasks[transport.Id] = new UnitMoveTask(transport.Id, UnitMovePurpose.DeliverPriest, command.AltarId, command.AltarId, path, Bridges.Version);
        return CommandResult.Ok();
    }

    /// <summary>운반 중인 수송 유닛을 칸으로 보내 사제를 내려놓게 한다.</summary>
    private CommandResult ExecuteDropPriest(DropPriestCommand command)
    {
        CommandResult check = CheckTransport(command.Player, command.TransportId, out GameEntity? transport);
        if (!check.Accepted)
        {
            return check;
        }
        if (transport!.CarriedPriestId == 0)
        {
            return new CommandResult(CommandFailure.NotCarrying);
        }
        var spot = new Footprint(command.X, command.Y, 1, 1);
        List<(int X, int Y)>? path = transport.Footprint.AnchorX == command.X && transport.Footprint.AnchorY == command.Y
            ? [(command.X, command.Y)] : FindMovePath(transport, spot);
        if (path == null)
        {
            return new CommandResult(CommandFailure.NoRoute);
        }
        // 목표 칸이 오브젝트가 아니므로 다리가 바뀌었을 때 다시 찾을 수 있도록 고정 목표를 함께 저장한다.
        _moveTasks[transport.Id] = new UnitMoveTask(transport.Id, UnitMovePurpose.DropPriest, 0, 0, path, Bridges.Version, spot);
        return CommandResult.Ok();
    }

    /// <summary>내 사제를 제단 옆으로 보낸다 (수확 작업은 취소).</summary>
    private CommandResult ExecuteMovePriestToAltar(MovePriestToAltarCommand command)
    {
        if (!_players.ContainsKey(command.Player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        GameEntity? priest = command.PriestId == 0 ? OwnFreePriest(command.Player) : Entity(command.PriestId);
        if (priest == null)
        {
            return new CommandResult(CommandFailure.NoPriest);
        }
        if (priest.Kind != ObjectKind.Priest || priest.Owner != command.Player || priest.IsStunned
            || priest.Captivity != PriestCaptivity.Free)
        {
            return new CommandResult(CommandFailure.NoPriest);
        }
        GameEntity? altar = Entity(command.AltarId);
        if (altar == null)
        {
            return new CommandResult(CommandFailure.NoSuchEntity);
        }
        if (altar.Kind != ObjectKind.Altar)
        {
            return new CommandResult(CommandFailure.WrongKind);
        }
        if (altar.Owner != command.Player)
        {
            return new CommandResult(CommandFailure.NotOwner);
        }
        if (!altar.IsComplete)
        {
            return new CommandResult(CommandFailure.NotComplete);
        }
        _harvestTasks.Remove(priest.Id);
        if (IsBeside(priest.Footprint, altar.Footprint))
        {
            _moveTasks.Remove(priest.Id);
            return CommandResult.Ok($"{priest.DisplayName} 이미 제단 옆");
        }
        List<(int X, int Y)>? path = FindMovePath(priest, altar.Footprint);
        if (path == null)
        {
            return new CommandResult(CommandFailure.NoRoute);
        }
        _moveTasks[priest.Id] = new UnitMoveTask(priest.Id, UnitMovePurpose.PriestToAltar, altar.Id, altar.Id, path, Bridges.Version);
        return CommandResult.Ok();
    }

    /// <summary>명령한 플레이어의 완성된 수송 유닛인지 확인한다.</summary>
    private CommandResult CheckTransport(int player, int transportId, out GameEntity? transport)
    {
        transport = null;
        if (!_players.ContainsKey(player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        transport = Entity(transportId);
        if (transport == null)
        {
            return new CommandResult(CommandFailure.NoSuchEntity);
        }
        if (transport.Kind != ObjectKind.Transport)
        {
            return new CommandResult(CommandFailure.WrongKind);
        }
        if (transport.Owner != player)
        {
            return new CommandResult(CommandFailure.NotOwner);
        }
        return transport.IsComplete ? CommandResult.Ok() : new CommandResult(CommandFailure.NotComplete);
    }

    /// <summary>사제를 묶을 수 있는 내 완성 제단인지 확인한다.</summary>
    private CommandResult CheckAltar(int player, int altarId)
    {
        GameEntity? altar = Entity(altarId);
        if (altar == null)
        {
            return new CommandResult(CommandFailure.NoSuchEntity);
        }
        if (altar.Kind != ObjectKind.Altar)
        {
            return new CommandResult(CommandFailure.WrongKind);
        }
        if (altar.Owner != player)
        {
            return new CommandResult(CommandFailure.NotOwner);
        }
        if (!altar.IsComplete)
        {
            return new CommandResult(CommandFailure.NotComplete);
        }
        return BoundPriestOf(altar.Id) != null ? new CommandResult(CommandFailure.AltarOccupied) : CommandResult.Ok();
    }

    /// <summary>
    /// 사제를 집을 수 있는지: 자유 상태여야 하고, 보통은 기절한 적 사제만(체력 50% 초과면 저항).
    /// allowAnyCapture 미션은 동맹·기절하지 않은 사제도 집는다 (구출 미션).
    /// </summary>
    private bool CanCapture(int player, GameEntity priest)
    {
        if (priest.Captivity != PriestCaptivity.Free || priest.Owner == player)
        {
            return false;
        }
        if (Mission?.AllowAnyCapture == true)
        {
            return true;
        }
        return priest.IsStunned && !Map.AreAllied(player, priest.Owner);
    }

    /// <summary>플레이어의 자유롭고 기절하지 않은 사제 (없으면 null)</summary>
    private GameEntity? OwnFreePriest(int player) =>
        _entities.Values.FirstOrDefault(e => e.Kind == ObjectKind.Priest && e.Owner == player && !e.IsStunned && e.Captivity == PriestCaptivity.Free);

    /// <summary>제단에 묶인 사제 (없으면 null)</summary>
    private GameEntity? BoundPriestOf(int altarId) =>
        _entities.Values.FirstOrDefault(e => e.Captivity == PriestCaptivity.Bound && e.CaptorId == altarId);

    // ───────────────────────── 틱 처리 ─────────────────────────

    /// <summary>수송 유닛·사제를 한 틱만큼 움직이고 도착하면 집기·묶기·내려놓기를 처리한다.</summary>
    private void UpdateUnitMoves()
    {
        // 이동하는 오브젝트 번호순으로 처리한다.
        foreach (UnitMoveTask task in _moveTasks.Values.ToArray())
        {
            GameEntity? mover = Entity(task.MoverId);
            if (mover == null || mover.IsStunned || mover.Captivity != PriestCaptivity.Free)
            {
                _moveTasks.Remove(task.MoverId);
                continue;
            }
            Footprint? goal = MoveGoal(task);
            if (goal == null)
            {
                _moveTasks.Remove(task.MoverId);
                continue;
            }
            if (!PrepareMovement(mover, task, goal.Value)) continue;
            AdvanceMovement(mover, task);
            if (task.NextIndex >= task.Path.Count)
            {
                _moveTasks.Remove(task.MoverId);
                ArriveMove(mover, task);
            }
        }
    }

    /// <summary>작업의 현재 목표 발자국 (내려놓기는 고정 칸, 그 외는 목표 오브젝트의 발자국, 목표가 사라졌으면 null)</summary>
    private Footprint? MoveGoal(UnitMoveTask task) => task.FixedGoal ?? Entity(task.TargetId)?.Footprint;

    /// <summary>오브젝트를 칸으로 옮긴다. 운반 중인 사제도 함께 옮긴다(점유는 등록하지 않음).</summary>
    private void MoveEntityTo(GameEntity entity, int x, int y, bool occupies)
    {
        if (occupies)
        {
            Map.RemoveOccupant(entity.Footprint);
        }
        entity.Footprint = Footprint.ForType(entity.Type.Definition, x, y);
        if (occupies)
        {
            Map.AddOccupant(entity.Footprint);
        }
        if (entity.CarriedPriestId != 0 && Entity(entity.CarriedPriestId) is { } carried)
        {
            carried.Footprint = Footprint.ForType(carried.Type.Definition, x, y);
        }
    }

    /// <summary>도착했을 때 목적별 처리</summary>
    private void ArriveMove(GameEntity mover, UnitMoveTask task)
    {
        switch (task.Purpose)
        {
            case UnitMovePurpose.PickUpPriest:
                GameEntity? priest = Entity(task.TargetId);
                if (priest == null || !CanCapture(mover.Owner, priest) || !IsBeside(mover.Footprint, priest.Footprint)
                    || priest.IsSuspended && !IsAirborneTransport(mover))
                {
                    Emit(SessionEventKind.PriestResisted, mover.Owner, task.TargetId, $"{mover.DisplayName} 포획 실패 (사제가 회복했거나 떠남)");
                    return;
                }
                PickUpPriest(mover, priest);
                if (task.AltarId != 0 && CheckAltar(mover.Owner, task.AltarId).Accepted)
                {
                    List<(int X, int Y)>? toAltar = FindMovePath(mover, Entity(task.AltarId)!.Footprint);
                    var delivery = new UnitMoveTask(mover.Id, UnitMovePurpose.DeliverPriest, task.AltarId, task.AltarId,
                        toAltar ?? [(mover.Footprint.AnchorX, mover.Footprint.AnchorY)], Bridges.Version);
                    _moveTasks[mover.Id] = delivery;
                    ReplaceMovementRoute(mover, delivery, toAltar);
                }
                break;
            case UnitMovePurpose.DeliverPriest:
                if (Entity(task.TargetId) is { } altar && CheckAltar(mover.Owner, altar.Id).Accepted && IsBeside(mover.Footprint, altar.Footprint)
                    && Entity(mover.CarriedPriestId) is { } captive)
                {
                    BindPriest(mover, captive, altar);
                }
                break;
            case UnitMovePurpose.DropPriest:
                if (Entity(mover.CarriedPriestId) is { } dropped)
                {
                    ReleasePriest(dropped, mover.Footprint.AnchorX, mover.Footprint.AnchorY, "내려놓음");
                    mover.CarriedPriestId = 0;
                }
                break;
            case UnitMovePurpose.PriestToAltar:
                // 의식 시작은 UpdateSacrifices 가 조건(묶인 사제·내 사제 옆)을 보고 정한다.
                break;
        }
    }

    /// <summary>사제를 집는다: 운반 상태가 되고 지상 점유·수확 작업에서 빠진다.</summary>
    private void PickUpPriest(GameEntity transport, GameEntity priest)
    {
        _harvestTasks.Remove(priest.Id);
        _moveTasks.Remove(priest.Id);
        if (priest.OccupiesGround) Map.RemoveOccupant(priest.Footprint);
        priest.IsSuspended = false;
        priest.Captivity = PriestCaptivity.Carried;
        priest.CaptorId = transport.Id;
        priest.CarriedCrystals = 0;
        priest.IsStunned = true;
        priest.Footprint = Footprint.ForType(priest.Type.Definition, transport.Footprint.AnchorX, transport.Footprint.AnchorY);
        transport.CarriedPriestId = priest.Id;
        Emit(SessionEventKind.PriestCaptured, transport.Owner, priest.Id, $"{transport.DisplayName}이(가) {priest.DisplayName}(플레이어 {priest.Owner}) 포획");
    }

    /// <summary>운반한 사제를 제단의 희생의 원에 묶는다.</summary>
    private void BindPriest(GameEntity transport, GameEntity priest, GameEntity altar)
    {
        transport.CarriedPriestId = 0;
        priest.Captivity = PriestCaptivity.Bound;
        priest.CaptorId = altar.Id;
        priest.Footprint = Footprint.ForType(priest.Type.Definition, (int)Math.Floor(altar.Footprint.CenterX), (int)Math.Floor(altar.Footprint.CenterY));
        Emit(SessionEventKind.PriestBound, altar.Owner, priest.Id, $"{priest.DisplayName}을(를) 제단에 묶음");
    }

    /// <summary>
    /// 포획된 사제를 풀어 준다. 운반 유닛에게서 생명력을 얻었으므로 완전히 회복하고 보호막이 풀린다(도움말).
    /// 풀려난 칸이 허공이면 그 자리에서 기절한다. 가까운 지면으로 순간이동시키지 않는다.
    /// </summary>
    private void ReleasePriest(GameEntity priest, int x, int y, string reason)
    {
        priest.Captivity = PriestCaptivity.Free;
        priest.CaptorId = 0;
        priest.IsStunned = false;
        priest.IsSuspended = false;
        priest.HitPoints = priest.MaxHitPoints;
        priest.Footprint = Footprint.ForType(priest.Type.Definition, x, y);
        if (HasGroundSupport(x, y)) Map.AddOccupant(priest.Footprint);
        else SuspendPriest(priest, removeOccupancy: false);
        Emit(SessionEventKind.PriestReleased, priest.Owner, priest.Id, $"{priest.DisplayName} 풀려남 ({reason})");
    }

    /// <summary>
    /// 희생 의식: 묶인 사제가 있는 제단 옆에 주인의 사제가 서 있으면 시작하고, 시간표대로 룬·완료·희생·제단 소멸을 알린다.
    /// 내 사제가 기절·이동하거나 제단이 크게 다치면 깨지고 묶인 사제가 달아난다.
    /// </summary>
    private void UpdateSacrifices()
    {
        // 의식을 시작할 수 있는 제단을 번호순으로 찾는다
        foreach (GameEntity altar in _entities.Values.Where(e => e.Kind == ObjectKind.Altar && e.IsComplete && !_rituals.ContainsKey(e.Id)).ToArray())
        {
            GameEntity? victim = BoundPriestOf(altar.Id);
            GameEntity? performer = OwnFreePriest(altar.Owner);
            if (victim == null || performer == null || !IsBeside(performer.Footprint, altar.Footprint))
            {
                continue;
            }
            _moveTasks.Remove(performer.Id);
            _harvestTasks.Remove(performer.Id);
            _rituals[altar.Id] = new AltarRitual(altar.Id, performer.Id, victim.Id, Tick);
            Emit(SessionEventKind.SacrificeStarted, altar.Owner, altar.Id, $"{altar.DisplayName} 희생 의식 시작");
        }
        // 진행 중인 의식의 시간표를 처리한다
        foreach (AltarRitual ritual in _rituals.Values.ToArray())
        {
            GameEntity? altar = Entity(ritual.AltarId);
            GameEntity? performer = Entity(ritual.PerformerId);
            GameEntity? victim = Entity(ritual.VictimId);
            if (!ritual.Completed)
            {
                bool broken = altar == null || victim == null || performer == null || performer.IsStunned
                    || performer.Captivity != PriestCaptivity.Free || !IsBeside(performer.Footprint, altar.Footprint)
                    || altar.MaxHitPoints > 0 && altar.HitPoints < altar.MaxHitPoints * AltarBreakHealthRatio;
                if (broken)
                {
                    BreakRitual(ritual, altar, victim);
                    continue;
                }
            }
            double elapsed = (Tick - ritual.StartTick) / (double)TicksPerSecond;
            // 지난 룬 음성·소멸을 순서대로 알린다
            while (ritual.RunesStarted < SacrificeRuneCount && elapsed >= RuneStartSeconds(ritual.RunesStarted))
            {
                Emit(SessionEventKind.SacrificeRune, altar!.Owner, altar.Id, SacrificeRuneNames[ritual.RunesStarted]);
                ritual.RunesStarted++;
            }
            while (ritual.RunesBurned < SacrificeRuneCount && elapsed >= RuneStartSeconds(ritual.RunesBurned) + SacrificeRuneBurnSeconds)
            {
                ritual.RunesBurned++;
                if (ritual.RunesBurned == SacrificeRuneCount)
                {
                    ritual.Completed = true;
                    Emit(SessionEventKind.SacrificeCompleted, altar!.Owner, altar.Id, "의식 완료 (It is done)");
                }
                else
                {
                    Emit(SessionEventKind.SacrificeRuneBurned, altar!.Owner, altar.Id, $"룬 {ritual.RunesBurned} 소멸");
                }
            }
            double completed = CompletionSeconds;
            if (ritual.Completed && !ritual.VictimKilled && elapsed >= completed + SacrificeKillDelaySeconds)
            {
                ritual.VictimKilled = true;
                Emit(SessionEventKind.PriestSacrificed, altar?.Owner ?? 0, ritual.VictimId, $"{victim?.DisplayName} 희생");
            }
            double altarConsumedAt = completed + AltarConsumeDelaySeconds;
            if (ritual.Completed && !ritual.AltarConsumed && elapsed >= altarConsumedAt)
            {
                // 제단이 사라져도 승패 이벤트가 날 때까지 묶인 사제를 남겨 둔다.
                if (victim != null)
                {
                    // 제단 제거 정리가 포로를 풀지 않도록 연결만 먼저 끊는다.
                    victim.CaptorId = 0;
                }
                ritual.AltarConsumed = true;
                if (altar != null)
                {
                    RemoveEntity(altar);
                    Emit(SessionEventKind.AltarConsumed, altar.Owner, altar.Id, $"{altar.DisplayName} 소멸");
                }
            }
            if (ritual.Completed && ritual.AltarConsumed && elapsed >= altarConsumedAt + SacrificedPriestRemovalDelaySeconds)
            {
                // 원본은 제단 폭발보다 약 3초 뒤 포로를 제거해 그때 BadTeamDead 조건이 참이 된다.
                // 다른 소멸 경로(전투 파괴·회수)와 같은 RemoveEntity 로 지워 선택·이동 작업 등 뒷정리를 일원화한다.
                _rituals.Remove(ritual.AltarId);
                if (victim != null)
                {
                    RemoveEntity(victim);
                }
            }
        }
    }

    /// <summary>룬 index 의 음성 시각(의식 시작 기준 초)</summary>
    private static double RuneStartSeconds(int index) => SacrificeRuneLeadSeconds + index * SacrificeRuneIntervalSeconds;

    /// <summary>의식 시작부터 완료(다섯 번째 룬 소멸)까지(초)</summary>
    public static double CompletionSeconds => RuneStartSeconds(SacrificeRuneCount - 1) + SacrificeRuneBurnSeconds;

    /// <summary>의식이 깨져 묶인 사제가 달아난다.</summary>
    private void BreakRitual(AltarRitual ritual, GameEntity? altar, GameEntity? victim)
    {
        _rituals.Remove(ritual.AltarId);
        Emit(SessionEventKind.SacrificeBroken, altar?.Owner ?? 0, ritual.AltarId, "희생 의식이 깨짐");
        if (victim != null && victim.Captivity == PriestCaptivity.Bound)
        {
            Footprint around = altar?.Footprint ?? victim.Footprint;
            ReleasePriest(victim, around.AnchorX + 1, around.AnchorY + 1, "의식 중단");
        }
    }

    /// <summary>
    /// 오브젝트가 사라질 때 포획 관계를 정리한다: 운반 유닛이면 사제를 그 자리에 풀어 주고, 제단이면 묶인 사제를 풀어 준다.
    /// 그 오브젝트를 목표로 하던 이동 작업도 취소한다.
    /// </summary>
    private void ReleaseCaptivesOf(GameEntity entity)
    {
        if (entity.CarriedPriestId != 0 && Entity(entity.CarriedPriestId) is { } carried)
        {
            ReleasePriest(carried, entity.Footprint.AnchorX, entity.Footprint.AnchorY, "운반 유닛 파괴");
        }
        if (entity.Kind == ObjectKind.Altar && BoundPriestOf(entity.Id) is { } bound)
        {
            _rituals.Remove(entity.Id);
            ReleasePriest(bound, entity.Footprint.AnchorX + 1, entity.Footprint.AnchorY + 1, "제단 파괴");
        }
        _moveTasks.Remove(entity.Id);
        // 이 오브젝트를 집으러·묶으러 가던 작업을 취소한다
        foreach (UnitMoveTask task in _moveTasks.Values.Where(t => t.TargetId == entity.Id || t.AltarId == entity.Id).ToArray())
        {
            _moveTasks.Remove(task.MoverId);
        }
    }

    /// <summary>
    /// 미션 승패·AI 이벤트를 판정한다 (원본 FUN_004c36c0 과 같은 조건, 각 섹션 이름은 미션당 한 번만).
    /// AI(사람 외 플레이어 1~8)마다 TempleHalfDead·TempleDead·PriestDead·PriestCaptured·PriestSaved,
    /// 전체로 GoodTeamDead(동맹 AI 사제 없음)·BadTeamDead(적 AI 사제 없음 = 성공)·Failed(내 사제 없음).
    /// </summary>
    private void UpdateMissionEvents()
    {
        if (Mission == null || Mission.TutorialNumber != null)
        {
            return;
        }
        int good = 0;
        int bad = 0;
        // AI 플레이어를 번호순으로 검사한다
        foreach (PlayerState player in _players.Values.Where(p => p.Number != HumanPlayer && p.Number <= MissionStart.MaximumPlayer))
        {
            int n = player.Number;
            GameEntity? temple = _entities.Values.FirstOrDefault(e => e.Kind == ObjectKind.Temple && e.Owner == n);
            GameEntity? priest = _entities.Values.FirstOrDefault(e => e.Kind == ObjectKind.Priest && e.Owner == n);
            bool templeSeen = Seen($"temple:{n}", temple != null);
            bool priestSeen = Seen($"priest:{n}", priest != null);
            TellOnce($"ai{n}TempleHalfDead", templeSeen && (temple == null || temple.MaxHitPoints > 0 && temple.HitPoints < temple.MaxHitPoints / 2), n);
            TellOnce($"ai{n}TempleDead", templeSeen && temple == null, n);
            TellOnce($"ai{n}PriestDead", priestSeen && priest == null, n);
            TellOnce($"ai{n}PriestCaptured", priest != null && priest.Captivity != PriestCaptivity.Free, n);
            bool saved = priest is { Captivity: PriestCaptivity.Free } && Map.TerritoryAt(priest.Footprint.AnchorX, priest.Footprint.AnchorY) is int territory
                && Map.Ownership.OwnerOf(territory) == HumanPlayer;
            TellOnce($"ai{n}PriestSaved", saved, n);
            if (priest != null)
            {
                if (Map.AreAllied(HumanPlayer, n))
                {
                    good++;
                }
                else
                {
                    bad++;
                }
            }
        }
        TellOnce("GoodTeamDead", Seen("good", good > 0) && good == 0, HumanPlayer);
        TellOnce("BadTeamDead", Seen("bad", bad > 0) && bad == 0, HumanPlayer);
        bool myPriest = _entities.Values.Any(e => e.Kind == ObjectKind.Priest && e.Owner == HumanPlayer);
        TellOnce("Failed", Seen("me", myPriest) && !myPriest, HumanPlayer);
    }

    /// <summary>대상이 지금 살아 있으면 "본 적 있음"으로 기록하고, 본 적이 있는지 돌려준다.</summary>
    /// <param name="key">대상 키</param>
    /// <param name="aliveNow">지금 살아 있는지</param>
    private bool Seen(string key, bool aliveNow)
    {
        if (aliveNow)
        {
            _seenAlive.Add(key);
        }
        return _seenAlive.Contains(key);
    }

    /// <summary>조건이 참이고 아직 알리지 않은 섹션이면 MissionTell 이벤트를 낸다.</summary>
    private void TellOnce(string section, bool condition, int player)
    {
        if (condition && _toldSections.Add(section))
        {
            Emit(SessionEventKind.MissionTell, player, 0, section);
        }
    }

    // ───────────────────────── 이동 경로 ─────────────────────────

    /// <summary>공중 수송(balloon)인지. 공중 수송은 다리 없이 곧게 날아간다(도움말 "Harvesting Storm Power").</summary>
    private static bool IsAirborneTransport(GameEntity entity) => entity.Type.Definition.HasFlag("balloon");

    /// <summary>오브젝트가 목표 발자국 옆까지 가는 칸 경로 (지상은 섬·내 다리, 공중은 직선)</summary>
    private List<(int X, int Y)>? FindMovePath(GameEntity mover, Footprint target) =>
        IsAirborneTransport(mover) ? StraightPath(mover.Footprint, target) : FindHarvestPath(mover.Footprint, target, mover.Owner);

    /// <summary>목표 둘레 중 가장 가까운 칸까지 곧은 칸 경로 (브레젠험 선, 공중 이동용)</summary>
    private static List<(int X, int Y)> StraightPath(Footprint start, Footprint target)
    {
        (int gx, int gy) = target.BorderCells()
            .OrderBy(c => (c.X - start.AnchorX) * (c.X - start.AnchorX) + (c.Y - start.AnchorY) * (c.Y - start.AnchorY))
            .ThenBy(c => c.Y).ThenBy(c => c.X).First();
        var path = new List<(int X, int Y)>();
        int x = start.AnchorX;
        int y = start.AnchorY;
        int dx = Math.Abs(gx - x);
        int dy = -Math.Abs(gy - y);
        int sx = x < gx ? 1 : -1;
        int sy = y < gy ? 1 : -1;
        int error = dx + dy;
        // 목표 칸에 닿을 때까지 브레젠험 방식으로 한 칸씩 나아간다
        while (true)
        {
            path.Add((x, y));
            if (x == gx && y == gy)
            {
                return path;
            }
            int doubled = 2 * error;
            if (doubled >= dy)
            {
                error += dy;
                x += sx;
            }
            if (doubled <= dx)
            {
                error += dx;
                y += sy;
            }
        }
    }

    /// <summary>발자국 a 의 기준점이 b 를 한 칸 넓힌 사각형 안에 있는지 (옆에 붙어 있음)</summary>
    private static bool IsBeside(Footprint a, Footprint b) =>
        a.AnchorX >= b.Left - 1 && a.AnchorX <= b.AnchorX + 1 && a.AnchorY >= b.Top - 1 && a.AnchorY <= b.AnchorY + 1;

    /// <summary>포획·의식·미션 이벤트 상태를 검사합에 넣는다.</summary>
    private void AddSacrificeChecksum(Fnv1a hash)
    {
        // 이동 작업은 이동하는 오브젝트 번호순이다
        foreach (UnitMoveTask task in _moveTasks.Values)
        {
            hash.Add(task.MoverId);
            hash.Add((int)task.Purpose);
            hash.Add(task.TargetId);
            hash.Add(task.AltarId);
            AddMovementChecksum(hash, task);
            hash.Add(task.FixedGoal == null ? 0 : 1);
            if (task.FixedGoal is { } goal)
            {
                hash.Add(goal.AnchorX);
                hash.Add(goal.AnchorY);
                hash.Add(goal.Width);
                hash.Add(goal.Height);
            }
        }
        // 의식은 제단 번호순이다
        foreach (AltarRitual ritual in _rituals.Values)
        {
            hash.Add(ritual.AltarId);
            hash.Add(ritual.PerformerId);
            hash.Add(ritual.VictimId);
            hash.Add(ritual.StartTick);
            hash.Add(ritual.RunesStarted);
            hash.Add(ritual.RunesBurned);
            hash.Add(ritual.VictimKilled ? 1 : 0);
            hash.Add(ritual.AltarConsumed ? 1 : 0);
        }
        // 알린 섹션 이름과 본 적 있는 대상은 정렬된 순서다
        foreach (string section in _toldSections)
        {
            hash.Add(section);
        }
        foreach (string key in _seenAlive)
        {
            hash.Add(key);
        }
    }

    /// <summary>수송 유닛·사제 이동 한 건 (경로와 다음 칸까지의 이동량)</summary>
    private sealed class UnitMoveTask(int moverId, UnitMovePurpose purpose, int targetId, int altarId, List<(int X, int Y)> path, int routeVersion,
        Footprint? fixedGoal = null) : MovementRoute(path, routeVersion)
    {
        /// <summary>움직이는 오브젝트 번호</summary>
        public int MoverId { get; } = moverId;

        /// <summary>이동 목적</summary>
        public UnitMovePurpose Purpose { get; } = purpose;

        /// <summary>목표 오브젝트 번호 (집을 사제·묶을 제단, 내려놓기는 0)</summary>
        public int TargetId { get; } = targetId;

        /// <summary>집은 뒤 이어서 갈 제단 번호 (없으면 0)</summary>
        public int AltarId { get; } = altarId;

        /// <summary>오브젝트가 아닌 고정 칸 목표 (내려놓기 전용, 그 외는 null). 다리가 바뀌어도 이 칸으로 길을 다시 찾는다.</summary>
        public Footprint? FixedGoal { get; } = fixedGoal;

    }
}

/// <summary>수송 유닛·사제 이동의 목적</summary>
public enum UnitMovePurpose
{
    /// <summary>사제를 집으러 감</summary>
    PickUpPriest,

    /// <summary>운반한 사제를 제단에 묶으러 감</summary>
    DeliverPriest,

    /// <summary>운반한 사제를 내려놓으러 감</summary>
    DropPriest,

    /// <summary>내 사제가 제단으로 감</summary>
    PriestToAltar,
}

/// <summary>제단 하나의 희생 의식 진행 상태</summary>
/// <param name="altarId">제단 번호</param>
/// <param name="performerId">의식을 행하는 내 사제 번호</param>
/// <param name="victimId">묶인 사제 번호</param>
/// <param name="startTick">의식 시작 틱</param>
public sealed class AltarRitual(int altarId, int performerId, int victimId, long startTick)
{
    /// <summary>제단 번호</summary>
    public int AltarId { get; } = altarId;

    /// <summary>의식을 행하는 사제 번호</summary>
    public int PerformerId { get; } = performerId;

    /// <summary>묶인 사제 번호</summary>
    public int VictimId { get; } = victimId;

    /// <summary>의식 시작 틱</summary>
    public long StartTick { get; } = startTick;

    /// <summary>음성이 나온(지키기 시작한) 룬 수</summary>
    public int RunesStarted { get; internal set; }

    /// <summary>타서 사라진 룬 수 (화면에 남은 룬 = 5 − 이 값)</summary>
    public int RunesBurned { get; internal set; }

    /// <summary>다섯 룬을 모두 지켰는지</summary>
    public bool Completed { get; internal set; }

    /// <summary>희생 음성이 나왔는지</summary>
    public bool VictimKilled { get; internal set; }

    /// <summary>제단은 소멸했지만 승패 판정을 위해 포로를 아직 남겨 두는 상태인지.</summary>
    public bool AltarConsumed { get; internal set; }
}
