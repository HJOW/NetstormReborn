using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 전체State부모의정확한효과위치에서실제장기자식과선택된외부장치를호출한다.
namespace Netstorm.Client;

/// <summary>현재Client의커서/UI/음향/저장등실제외부장치다. 누락은도달한호출위치에서실패한다.</summary>
public sealed class ClientStateWorldDevices
{
    /// <summary>실제Clock/UI정지표식을함께반납하는원문PopGamePause다.</summary>
    public Action? ResumeGame { get; init; }
    /// <summary>같은음향소유자의실제PopMute다.</summary>
    public Action? ResumeAudio { get; init; }
    /// <summary>현재Runtime의커서가있으면그Cancel,없으면같은Client의Cancel/Prepare를선택한다.</summary>
    public Action? CancelCursor { get; init; }
    /// <summary>같은100×48BYTE점유채팅행의실제ResetOccupied다.</summary>
    public Action? ResetChatChannels { get; init; }
    /// <summary>실제 선택/강조 부모가 선택할 현재 하드웨어/소프트웨어 커서 종류다.</summary>
    public Action<uint>? SelectCursor { get; init; }
    /// <summary>같은공통Gump등록표의실제Kill이다.</summary>
    public Action? KillGumps { get; init; }
    /// <summary>같은공통Gump등록표의실제Cleanup이다.</summary>
    public Action? CleanupGumps { get; init; }
    /// <summary>현재전체Uber/HUD를원문공장으로교체한다.</summary>
    public Action? ReplaceUber { get; init; }
    /// <summary>현재예산serial의실제Push/Attach갱신이다.</summary>
    public Action? RefreshSp { get; init; }
    /// <summary>자식정리증빙뒤현재같은Runtime/UI/Guide/입력소유자를동기로반납한다.</summary>
    public Action? RetireRuntime { get; init; }
    /// <summary>현재metamap비영에서만호출하는실제Leave부모다.</summary>
    public Action? LeaveMetamap { get; init; }
    /// <summary>선택된현재Template의실제Fort snapshot/저장부모다.</summary>
    public Action<uint>? SaveFort { get; init; }
    /// <summary>비mission전투phase14에서선택하는실제snapshot이다.</summary>
    public Action<uint>? SaveBattle { get; init; }
    /// <summary>원문중복모드등의진단행을동일호출위치에서처리한다.</summary>
    public Action<uint>? Assertion { get; init; }
    /// <summary>현재battle모드를인계받아동일장기음악소유자의실제Start를호출한다.</summary>
    public Action<bool>? StartSceneMusic { get; init; }
    /// <summary>동일소리재생기의실제StopLoops다.</summary>
    public Action? StopSoundLoops { get; init; }
    /// <summary>전체State후원문위치에서현재실제옵션소비자를재해석한다.</summary>
    public Action? InterpretOptions { get; init; }
}

/// <summary>004b2df0본체를재구현하지않고실제Client/Surface/Runtime/SP와34효과에연결한다.</summary>
public static class ClientStateWorldCleanup
{
    /// <summary>미선택장치를미리요구하지않고선택된실제자식만해당위치에서검증한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("전체State정리의선택된실제외부장치누락");

    /// <summary>같은실제소유자검증뒤원문부모를실행한다. Root는현재실제커서/채팅을직접제공하며독립장치입력보다우선한다. 실패한부분쓰기는되돌리지않는다.</summary>
    public static void Cleanup(uint reason, ClientStateWorldContext context, ClientSurfaceGlobals surface,
        ClientSpGlobals sp, ClientStateWorldDevices devices, Action<uint>? selectCursor = null, Action? cancelCursor = null, Action? resetChatChannels = null)
    {
        ArgumentNullException.ThrowIfNull(context); ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(sp);
        ArgumentNullException.ThrowIfNull(devices);
        if (!ReferenceEquals(context.Surface, surface.Storage) || !ReferenceEquals(context.Gameplay, surface.Gameplay))
            throw new ArgumentException("전체State정리의같은실제Client/Surface필요", nameof(surface));
        var game = surface.Gameplay;
        if (game.Borrowed())
        {
            var session = game.RequireSession(); session.EnsureStateRuntimeIdle();
            if (!ReferenceEquals(session.Sp, sp.Store) || !ReferenceEquals(session.Game, sp.Random))
                throw new ArgumentException("전체State정리의같은실제SP/게임난수필요", nameof(sp));
        }
        StateWorldCleanupGlobals globals = ClientStateWorldGlobals.Bind(context);
        var runtime = new ClientStateRuntimeRetirement(surface);
        var physical = new ClientStateStorageCleanup(surface, context.Local, game.Selection.MultipleCount);
        RawStateWorldCleanup.CleanupStateWorld(reason, globals, new()
        {
            Disconnect = owner => context.Connections.Registry.Disconnect(unchecked((int)owner)),
            Assertion = line => Need(devices.Assertion)(line), PauseDepth = () => surface.Clock.Clock.IsPaused() ? 1u : 0u,
            SaveFort = slot => Need(devices.SaveFort)(slot), SaveBattle = slot => Need(devices.SaveBattle)(slot),
            ResetAi = runtime.ResetAi,
            ResetPriest = owner => RawPriestPostPop.ResetPriestOwnerState(surface.Storage.PriestOwnerState, owner),
            ResetOtherPlayer = (owner, amount) => RawFortMoneyLoad.SetPlayerSpAmount(unchecked((int)owner), unchecked((int)amount), sp.Store,
                new() { ClientActive = () => game.Player.PlayerKnown, LocalOwner = () => game.Player.BattleOwner, Editor = () => game.World.Editor }, () => Need(devices.RefreshSp)()),
            SetSp = (owner, value) => sp.Store.Set(owner, BitConverter.SingleToUInt32Bits(value)),
            SelectCore = (owner, template) => game.CoreData.Select(game.Player, surface.Storage.OwnerMode.Challenge ? 1u : 0u, owner, template),
            ResetPool = capacity => { runtime.Retire(devices.RetireRuntime); physical.ResetPool(unchecked((int)capacity)); },
            WallSeconds = surface.WallSeconds,
            Effect = effect =>
            {
                switch (effect)
                {
                    case StateWorldCleanupEffect.CloseAllConnections: context.Connections.Registry.CloseAll(); return;
                    case StateWorldCleanupEffect.EndConnections: context.Connections.Registry.EndPlayers(game.Authority.Server ? 1u : 0u); return;
                    case StateWorldCleanupEffect.ResumeGame: Need(devices.ResumeGame)(); return;
                    case StateWorldCleanupEffect.ResumeAudio: Need(devices.ResumeAudio)(); return;
                    case StateWorldCleanupEffect.Clear446e70: Need(cancelCursor ?? devices.CancelCursor)(); return;
                    case StateWorldCleanupEffect.Clear4c1f20: physical.ResetClockSynchronization(); return;
                    case StateWorldCleanupEffect.ResetClock: surface.Clock.Clock.Reset(); return;
                    case StateWorldCleanupEffect.Clear42b1b0: physical.ClearMetaZones(); return;
                    case StateWorldCleanupEffect.RefreshSp: Need(devices.RefreshSp)(); return;
                    case StateWorldCleanupEffect.ClearTemplates: context.Templates.ClearAll(); return;
                    case StateWorldCleanupEffect.LeaveMetamap: Need(devices.LeaveMetamap)(); return;
                    case StateWorldCleanupEffect.Clear445140: Need(resetChatChannels ?? devices.ResetChatChannels)(); return;
                    case StateWorldCleanupEffect.ClearCursor: ClientUserSelectionCleanup.Clear(game, surface.Storage, selectCursor ?? devices.SelectCursor); return;
                    case StateWorldCleanupEffect.ClearPlayerLinks: runtime.ClearPlayerLinks(); return;
                    case StateWorldCleanupEffect.ClearKernel: runtime.ClearKernel(); return;
                    case StateWorldCleanupEffect.KillGumps: Need(devices.KillGumps)(); return;
                    case StateWorldCleanupEffect.CleanupGumps: Need(devices.CleanupGumps)(); return;
                    case StateWorldCleanupEffect.ReplaceUber: Need(devices.ReplaceUber)(); return;
                    case StateWorldCleanupEffect.Clear43b540: case StateWorldCleanupEffect.Reset422490: case StateWorldCleanupEffect.Clear4c2450:
                    case StateWorldCleanupEffect.Clear472830: case StateWorldCleanupEffect.Clear4b2b50: case StateWorldCleanupEffect.Clear4328a0:
                    case StateWorldCleanupEffect.Clear4ab940: case StateWorldCleanupEffect.Reset568b60: case StateWorldCleanupEffect.Reset54c8a0:
                    case StateWorldCleanupEffect.ResetGraph: physical.ResetWorldStorage(effect); return;
                    case StateWorldCleanupEffect.MapMode: return; // 보호PE004437f0의실제C3한개이며미연결효과의성공대체가아니다.
                    case StateWorldCleanupEffect.SceneMusic: Need(devices.StartSceneMusic)(surface.Storage.OwnerMode.Battle); return;
                    case StateWorldCleanupEffect.Clear4a9e50: Need(devices.StopSoundLoops)(); return;
                    case StateWorldCleanupEffect.InitializePlayers: ClientPlayerRuntime.Initialize(game); return;
                    case StateWorldCleanupEffect.ResetZone: physical.ResetZone(); return;
                    case StateWorldCleanupEffect.InterpretOptions: Need(devices.InterpretOptions)(); return;
                    default: throw new ArgumentOutOfRangeException(nameof(effect), "전체State정리의미등록효과");
                }
            },
        });
    }
}
