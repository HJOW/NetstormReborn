using static Netstorm.Core.Original.RawMissionPacketFields;

// 같은720바이트에서 현재 상태를 실행하고 원문 공통부가 전체 블록을 확정한다.
namespace Netstorm.Core.Original;

/// <summary>RawLocalMissionState.cpp의 현재 상태와 비동기 수신 경계를 원문 순서로 실행한다.</summary>
public sealed class RawLocalMissionState
{
    /// <summary>old/phase/immediate/current/next의 같은 실제 블록이다.</summary>
    private readonly LocalMissionStateStorage _state;
    /// <summary>현재 getter/setter와 실제 상태 상자다.</summary>
    private readonly LocalMissionStateGlobals _globals;
    /// <summary>선택된 실제 큰 하위 효과다.</summary>
    private readonly LocalMissionStateHooks _hooks;
    /// <summary>상태 확정 중 동기 수신/재진입을 막는다.</summary>
    private bool _running;
    /// <summary>실제 수신 순서이며 상태 공통 확정 뒤에만 소비한다.</summary>
    private readonly Queue<Action> _receives = new();
    /// <summary>서로 다른 다섯144바이트와 실제 immediate+84 메뉴창 별칭을 확인한다.</summary>
    public RawLocalMissionState(OriginalEdition edition, LocalMissionStateStorage state, LocalMissionStateGlobals globals, LocalMissionStateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        Memory<byte>[] blocks = [state.Old, state.Phase, state.Immediate, state.Current, state.Next];
        // 모든 원본 블록은144바이트이며 서로 겹치지 않는 실제 저장소여야 한다.
        for (int first = 0; first < blocks.Length; first++)
        {
            if (blocks[first].Length != 144) throw new ArgumentException("로컬 미션 상태144바이트 누락");
            // 다른 상태를 같은 주소로 가장하는 구성을 거부한다.
            for (int second = first + 1; second < blocks.Length; second++) if (blocks[first].Span.Overlaps(blocks[second].Span)) throw new ArgumentException("로컬 미션 상태 블록 중첩");
        }
        if (edition != OriginalEdition.Patch1078 || !globals.MenuWindow.MatchesStorage(state.Immediate.Slice(132, 4))) throw new ArgumentException("로컬 미션 상태PATCH/실제 immediate+84 별칭 오류");
        _state = state; _globals = globals; _hooks = hooks;
    }
    /// <summary>같은 실제 블록에서 원문 DWORD를 읽는다.</summary>
    private static uint Word(ReadOnlyMemory<byte> block, int index) => Get(block.Span, checked(index * 4));
    /// <summary>같은 실제 블록의 지정 DWORD만 변경한다.</summary>
    private static void Word(Memory<byte> block, int index, uint value) => Put(block.Span, checked(index * 4), value);
    /// <summary>원본 _ftol의 signed64 절삭 하위DWORD이며 범위 밖/비유한은0이다.</summary>
    private static uint Truncate(double value) => !double.IsFinite(value) || value >= 9223372036854775808.0 || value < -9223372036854775808.0 ? 0u : unchecked((uint)(long)value);
    /// <summary>진단이 복귀하면 실제 인자로 선택한 요청을 같은 immediate 첫DWORD에 쓴다.</summary>
    public void Request(uint state) { if (Word(_state.Immediate, 0) != 0) Need(_hooks.Assertion)(); Word(_state.Immediate, 0, state); }
    /// <summary>현재 메타 선행 효과 뒤 next가 있는 경우만 current로 옮겨 실행한다.</summary>
    public void Poll() => PollBody(null);
    /// <summary>실제 next7과 동기 부모가 있어야 같은 공통 Poll을 사용한다.</summary>
    public void PollStarting(Action body)
    {
        if (Word(_state.Next, 0) != 7 || body is null) throw new InvalidOperationException("실제 next7/부모 몸체 계약 오류"); PollBody(body);
    }
    /// <summary>메타 효과가 바꾼 현재 next와 모드를 원문 위치에서 재조회한다.</summary>
    private void PollBody(Action? starting)
    {
        if (_globals.MetaRebuild.Value != 0 && _globals.MetaRefresh.Value == 0 && Need(_globals.Metamap)()) { Need(_hooks.MetaFirst)(); _globals.MetaRebuild.Value = 0; }
        if (_globals.MetaRefresh.Value != 0 && Need(_globals.Metamap)()) { Need(_hooks.MetaSecond)(); Need(_hooks.MetaThird)(); _globals.MetaRefresh.Value = 0; }
        if (Word(_state.Next, 0) != 0) { _state.Next.Span.CopyTo(_state.Current.Span); Word(_state.Next, 0, 0); RunBody(starting); }
    }
    /// <summary>정리 전 현재Zone 보관·제목 첫BYTE0·선택된 시각/출처 보관 순서다.</summary>
    public static LocalMissionStartingSnapshot CaptureStarting(ReadOnlyMemory<byte> current, UIntBox zonePending, Memory<byte> title, UIntBox ticks, UIntBox source)
    {
        uint zone = zonePending.Value; title.Span[0] = 0;
        return Word(current, 33) != 0 ? new() { ZonePending = zone, GameTicks = ticks.Value, SourceType = source.Value } : new() { ZonePending = zone };
    }
    /// <summary>정리 뒤 현재+84를 다시 읽어 시각→출처 순서로 복원한다.</summary>
    public static void RestoreStarting(ReadOnlyMemory<byte> current, LocalMissionStartingSnapshot saved, UIntBox ticks, UIntBox source)
    {
        if (Word(current, 33) != 0) { ticks.Value = saved.GameTicks; source.Value = saved.SourceType; }
    }
    /// <summary>상태7의 정리/CD/Zone/Renderer/진행창과 원문 즉시8 요청이다.</summary>
    private void Starting()
    {
        Need(_hooks.SelectDefaultDenominator)(); LocalMissionStartingSnapshot saved = CaptureStarting(_state.Current, _globals.ZonePending, _globals.TitleInitial, _globals.GameTicks, _globals.SourceType);
        Need(_hooks.CleanupBattle)(1); _globals.CdPresent.Value = Need(_hooks.CdPresent)(); Need(_hooks.RefreshCommands)(); RestoreStarting(_state.Current, saved, _globals.GameTicks, _globals.SourceType);
        Need(_globals.SetInBattle)(true); Need(_hooks.StopMusic)(); if (Need(_globals.MissionWorld)() && Need(_globals.MissionPost)()) Need(_hooks.RefreshMission)();
        if (Need(_globals.Simulconnect)()) Need(_hooks.ListenServer)();
        if (saved.ZonePending != 0)
        {
            Need(_hooks.InstallZone)(Word(_state.Current, 32)); Need(_hooks.FreeZone)(Word(_state.Current, 32)); Word(_state.Current, 32, 0); Word(_state.Next, 32, 0); Need(_hooks.InstallMapHandlers)();
        }
        Need(_hooks.InitializeRenderer)(); _globals.Player.PlayerKnown = 0; Need(_hooks.UiMode)(2);
        _globals.ActivityEnd.Value = Need(_hooks.WallSeconds)() + 10; Word(_state.Immediate, 1, 20); Request(8);
        if (Word(_state.Current, 33) != 0) { Need(_globals.SetSavedGame)(true); Need(_hooks.LoadReconnectFort)(); Need(_hooks.Progress)(LocalMissionProgress.ReconnectStarting, 0); }
        else if (Need(_globals.MissionPost)()) Need(_hooks.Progress)(LocalMissionProgress.MissionStarting, 0);
        else
        {
            if (Need(_globals.Server)()) Need(_hooks.InitializeDensity)(); Need(_hooks.ClearChat)(0x40); Need(_hooks.ClearChat)(0x80); Need(_hooks.Progress)(LocalMissionProgress.ConnectingStarting, 0);
        }
        uint palette = Need(_hooks.BattlePalette)(); Need(_hooks.ApplyPalette)(palette);
    }
    /// <summary>상태13의 실제Template Core·주소/시각·UI·연결/타이머 효과를 현재값으로 실행한다.</summary>
    private void Connected()
    {
        _globals.ConnectEnd.Value = 0; Need(_hooks.CloseProgress)(); if (!Need(_globals.Server)()) { uint connection = Need(_hooks.ServerConnection)(); Need(_hooks.AttachCommander)(connection); }
        if (!Need(_hooks.FortAvailable)(_globals.Player.TemplateOwner)) Need(_hooks.LoadCurrentFort)(); Memory<byte> core = Need(_hooks.FortCore)(_globals.Player.TemplateOwner);
        if (core.Length < 24) throw new ArgumentOutOfRangeException(nameof(core), "현재 FortCore24바이트 누락"); core.Span[0] = (byte)((core.Span[0] & 0xfb) | 2);
        ReadOnlyMemory<byte> address = Need(_hooks.LocalAddress)(); if (address.Length != 20) throw new ArgumentOutOfRangeException(nameof(address), "현재 로컬 주소20바이트 누락");
        Put(core.Span, 4, Get(address.Span, 4)); uint port = Get(address.Span, Get(address.Span, 0, 2) == 6 ? 12 : 2, 2); Put(core.Span, 8, ((port & 255) << 8) | (port >> 8), 2); Put(core.Span, 12, Truncate(Need(_globals.GameSeconds)()));
        byte battle = Need(_globals.BattleByte)(); Put(core.Span, 0x16, Get(core.Span, 0x16, 2) + 1, 2); core.Span[0x10] = battle; Need(_hooks.TouchFort)(_globals.Player.TemplateOwner);
        Need(_hooks.UiMode)(3); Need(_hooks.RefreshFort)(); _globals.MenuWindow.Value = 0; Word(_state.Next, 33, 0); Request(14);
        if (Word(_state.Current, 33) != 0) Need(_hooks.SetReconnectSp)((float)unchecked((int)Word(_state.Current, 34)));
        if (!Need(_hooks.ConnectionOpen)(9)) { _globals.SourceType.Value = 0; Word(_state.Next, 0, 3); }
        else { Need(_hooks.LoginTimer)(true); Need(_hooks.InputMode)(false); }
    }

    /// <summary>선택 상태의 일반 반환은 공통 확정으로 가며 timeout/큰 부모의 반환만 건너뛴다.</summary>
    private bool Step()
    {
        uint state = Word(_state.Current, 0);
        switch (state)
        {
            case 7: Starting(); break;
            case 8:
                if (_globals.ConnectEnd.Value == 0) { double now = Need(_hooks.WallSeconds)(); _globals.ConnectEnd.Value = now + 30; if (!Need(_globals.MissionPost)()) Need(_hooks.Progress)(Need(_globals.SavedGame)() ? LocalMissionProgress.Reconnecting : LocalMissionProgress.Connecting, 0); }
                Need(_hooks.ConnectServer)(_state.Current.Slice(0x54, 20), Word(_state.Current, 31)); Word(_state.Next, 0, 9); break;
            case 9:
            {
                int connected = Need(_hooks.ConnectionStatus)(9);
                if (connected < 1 && !Need(_globals.Server)()) if (Need(_hooks.ConnectionStatus)(-1) != 0) { int status = Need(_hooks.ConnectionStatus)(-1); Need(_hooks.LogConnection)(0x401, status); Need(_hooks.CloseConnection)(9); Need(_hooks.RenameConnection)(-1, 9); connected = 1; }
                double now = Need(_hooks.WallSeconds)(); if (_globals.ConnectEnd.Value <= now) { _globals.SourceType.Value = 0; Word(_state.Next, 0, 3); return true; }
                bool open = Need(_hooks.ConnectionOpen)(9);
                if (!open || connected < 0) { bool current = Need(_hooks.ConnectionOpen)(9); Need(_hooks.LogConnection)(0x40f, current ? 1 : 0); Need(_hooks.CloseConnection)(9); Request(8); }
                else if (connected < 1) { int remaining = unchecked((int)Truncate(_globals.ConnectEnd.Value - Need(_globals.WallCached)())); Need(_hooks.Progress)(Need(_globals.SavedGame)() ? LocalMissionProgress.ReconnectCountdown : LocalMissionProgress.ConnectingCountdown, remaining); Word(_state.Next, 0, 9); }
                else { Need(_hooks.SendLogin)(); Need(_hooks.ConnectedProgress)(); Request(10); }
                break;
            }
            case 10:
                if (!Need(_hooks.ConnectionOpen)(9)) { Need(_hooks.Progress)(LocalMissionProgress.ServerLost, 0); _globals.SourceType.Value = 0; Word(_state.Next, 0, 3); } else Word(_state.Next, 0, 10); break;
            case 13: Connected(); break;
            case 15: Need(_hooks.TakeServer)(); break;
            case 4: case 12: case 14: case 16: break;
            case 1: case 2: case 3: case 6: case 19: case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39: case 40: case 41: case 42:
                return Need(_hooks.ParentState)(state) == LocalMissionParentResult.ReturnWithoutCommit;
            default: Need(_hooks.InvalidState)(state); break;
        }
        return false;
    }
    /// <summary>현재 같은 상태와0x90바이트 공통 확정을 실행한다.</summary>
    public void Run() => RunBody(null);
    /// <summary>현재 실제7의 동기 부모를 같은 공통부로 실행하며7로 강제하지 않는다.</summary>
    public void RunStarting(Action body)
    {
        if (Word(_state.Current, 0) != 7 || body is null) throw new InvalidOperationException("실제 State7 몸체/현재 상태 계약 오류"); RunBody(body);
    }
    /// <summary>모든 선택 효과 뒤 전체블록 확정·현재 첫DWORD0·즉시요청 반복을 수행한다.</summary>
    private void RunBody(Action? starting)
    {
        if (_running) throw new InvalidOperationException("상태 실행 중 동기 수신 재진입: 실제 수신 큐 필요"); _running = true;
        try
        {
            // 현재 즉시 요청이 있을 때만 같은 Run에서 다음 상태를 처리한다.
            for (;;)
            {
                _state.Current.Span.CopyTo(_state.Next.Span); _state.Current.Span.CopyTo(_state.Immediate.Span); Word(_state.Next, 0, 0); Word(_state.Immediate, 0, 0); Need(_hooks.LogState)(Word(_state.Current, 0));
                if (Word(_state.Current, 0) == 7 && starting is not null) starting(); else if (Step()) break;
                bool again = Word(_state.Immediate, 0) != 0; _state.Phase.Span.CopyTo(_state.Old.Span); _state.Current.Span.CopyTo(_state.Phase.Span); Word(_state.Current, 0, 0); if (!again) break; _state.Immediate.Span.CopyTo(_state.Current.Span);
            }
        }
        finally { _running = false; }
    }
    /// <summary>송신 callback은 실제 수신을 큐에 예약하고 상태 공통 확정까지 반환한다.</summary>
    public void EnqueueReceive(Action receive) { ArgumentNullException.ThrowIfNull(receive); _receives.Enqueue(receive); }
    /// <summary>현재 Run이 끝난 뒤 실제 수신을 순서대로 소비하고 새로 예약된 꼬리도 같은 원문 큐에서 처리한다.</summary>
    public void DispatchQueued()
    {
        if (_running) throw new InvalidOperationException("상태 확정 전 수신 분배 금지");
        // 현재 항목은 실행 전에 반납하여 callback이 큐를 바꿔도 이미 선택한 함수를 유지한다.
        while (_receives.Count != 0) { Action receive = _receives.Dequeue(); receive(); }
    }
}
