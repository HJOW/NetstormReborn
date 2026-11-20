using Netstorm.Core.Original;

// 첫메뉴전과State정리의명시위치에서같은장기Player초기화를호출한다.
namespace Netstorm.Client.Player;

/// <summary>ClientPlayerRuntime.cpp의10.78 known0초기화부모다.</summary>
public static class ClientPlayerRuntime
{
    /// <summary>같은Player9/공장/81방향동맹을구성한다. 초기known관문뒤미선택World/Chat장치를요구한경우그위치에서실패한다.</summary>
    public static void Initialize(ClientGameplayGlobals game)
    {
        ArgumentNullException.ThrowIfNull(game);
        if (game.Player.PlayerKnown != 0) throw new InvalidOperationException("공통Player시작의실제known0관문");
        game.PlayerRecords.Initialize((owner, mask) =>
        {
            var initial = new RawPlayerParticipation(OriginalEdition.Patch1078, game.PlayerRecords.Records, game.PlayerRecords.Alliances, new byte[9], new(), new());
            // 초기known0에서읽지않는World관문을상수모드로대체하지않는다.
            static bool Unavailable() => throw new InvalidOperationException("공통Player동맹시작의미선택World장치");
            initial.BindCurrentInputs(new() { LocalOwner = () => game.Player.BattleOwner, PlayerKnown = () => game.Player.PlayerKnown, Battle = Unavailable, Metamap = Unavailable, LocalTransport = Unavailable });
            initial.SetAlliance(owner, mask);
        });
    }
}
