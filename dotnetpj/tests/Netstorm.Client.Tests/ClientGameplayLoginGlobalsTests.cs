using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 장기옵션/숨김부분뷰/Core현재DWORD와실제Client소비자의공유를검사한다.
namespace Netstorm.Client.Tests;

/// <summary>명시미션부모와세션대여가서로다른초기화책임을유지하는지확인한다.</summary>
public sealed class ClientGameplayLoginGlobalsTests
{
    /// <summary>같은Client전역을빌리는실제세션을만든다.</summary>
    private static HostSession Session(ClientGameplayGlobals gameplay) => new(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: gameplay);

    /// <summary>새Client는87바이트BSS0과빈Player만소유하고기존Deck/Core소유자를공유한다.</summary>
    [Fact]
    public void NewClientOwnsUnallocatedPlayerAndZeroOptionsWithoutReset()
    {
        var gameplay = new ClientGameplayGlobals(); Assert.Equal(87, gameplay.BattleOptions.Length); Assert.All(gameplay.BattleOptions, value => Assert.Equal(0, value));
        Assert.Equal(9, gameplay.HiddenPlayers.Length); Assert.Equal(0u, gameplay.PlayerRecords.Base); Assert.True(gameplay.PlayerRecords.Records.IsEmpty);
        Assert.Same(gameplay.CoreData, gameplay.PlayerRecords.BoundCoreData); Assert.True(gameplay.CoreData.Players.IsEmpty);
        Assert.Same(gameplay.ProductionDeckStorage, gameplay.PlayerRecords.ProductionDeckStorage);
    }

    /// <summary>두실제세션은같은옵션주소와+3e부분뷰를빌리며대여/반납이값을되감지않는다.</summary>
    [Fact]
    public void SessionsShareOptionsAndHiddenSliceWithoutConstructorReset()
    {
        var gameplay = new ClientGameplayGlobals(); RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, gameplay.BattleOptions);
        gameplay.BattleOptions[0x3d] = 0xaa; gameplay.BattleOptions[0x47] = 0xbb; gameplay.HiddenPlayers.Span[2] = 0x73;
        var storage = new RawSurfaceStorage(); var first = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: gameplay, pool: storage.Pool);
        Assert.Same(gameplay.BattleOptions, first.BattleOptions); Assert.Equal(0x73, first.HiddenPlayers.Span[2]);
        first.HiddenPlayers.Span[8] = 0x94; var second = Session(gameplay);
        Assert.Same(first.BattleOptions, second.BattleOptions); Assert.Equal(0x94, second.HiddenPlayers.Span[8]);
        Assert.Equal(0xaa, gameplay.BattleOptions[0x3d]); Assert.Equal(0xbb, gameplay.BattleOptions[0x47]);
        using (var runtime = HostMissionRuntime.Create(first, storage, []))
            Assert.Equal(0x73, gameplay.HiddenPlayers.Span[2]);
        Assert.Equal(0x94, second.HiddenPlayers.Span[8]); Assert.Equal(0x73, first.DumpPlayers().Hidden[2]);
    }

    /// <summary>실제참여모듈은Chat효과뒤현재옵션부분뷰를재조회하고숨김채널을구성한다.</summary>
    [Fact]
    public void ParticipationReadsCurrentHiddenByteAfterChatCallbacks()
    {
        var gameplay = new ClientGameplayGlobals(); var session = Session(gameplay); var added = new List<uint>();
        var participant = new PlayerParticipation(session.PlayerRecords, session.Alliances, session.HiddenPlayers,
            new Netstorm.Core.Simulation.PlayerParticipationState { LocalOwner = 1, PlayerKnown = 1 },
            new Netstorm.Core.Simulation.PlayerParticipationHooks
            {
                ClearChat = channel => { if (channel == 0x80) gameplay.BattleOptions[0x3e + 2] = 1; },
                AddChat = (_, _, channel) => added.Add(channel),
            });
        participant.SetAlliance(1, 2); Assert.Equal(new uint[] { 0x40, 0x80 }, added); Assert.Equal(1, session.HiddenPlayers.Span[2]);
    }

    /// <summary>같은activeDWORD와실제Player를선택/정리하는Core부모가현재주소/캐시를공유한다.</summary>
    [Fact]
    public void CoreActiveBoxAndActualPlayerAliasStaySharedAcrossClear()
    {
        var gameplay = new ClientGameplayGlobals(); var core = gameplay.CoreData;
        gameplay.PlayerRecords.Initialize((owner, mask) => Assert.Equal(1u << (int)owner, mask));
        core.ActiveBox.Value = 0xffff1234; Assert.Equal(0xffff1234u, core.Active); core.Active = 5; Assert.Equal(5u, core.ActiveBox.Value);
        var state = new PlayerWorldState(); core.Select(state, 0, 1, 1); core.Current.Span[0] = 0xab;
        Assert.Equal(0xab, gameplay.PlayerRecords.Records.Span[180 + 0x6c]); Assert.Equal(5u, core.ActiveBox.Value);
        core.Select(state, 0, 0, 0); Assert.Equal(0u, core.ActiveBox.Value); Assert.Equal(0xab, core.Cached()[0]); Assert.True(core.Current.IsEmpty);
    }

    /// <summary>같은밀도/Zone상자와기존GameAux속성이양방향으로공유되며현재옵션부모가직접쓴다.</summary>
    [Fact]
    public void ActualDensityParentWritesSameGameAuxBoxes()
    {
        var gameplay = new ClientGameplayGlobals(); var aux = gameplay.GameAux;
        Assert.Equal(1u, aux.TerrainEnabledBox.Value); Assert.Equal(15, aux.ArchipelagoMinimumBox.Value); Assert.Equal(35, aux.ArchipelagoMaximumBox.Value);
        aux.ArchipelagoBox.Value = 0xfefefefe; Assert.Equal(0xfefefefeu, aux.Archipelago); aux.TerrainEnabled = 0x12345678; Assert.Equal(0x12345678u, aux.TerrainEnabledBox.Value);
        aux.ArchipelagoMinimum = -7; Assert.Equal(-7, aux.ArchipelagoMinimumBox.Value); aux.ArchipelagoMaximumBox.Value = -3; Assert.Equal(-3, aux.ArchipelagoMaximum);
        aux.ArchipelagoPercent = uint.MaxValue; Assert.Equal(uint.MaxValue, aux.ArchipelagoPercentBox.Value);
        RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, gameplay.BattleOptions);
        var options = new RawMissionBattleOptions(OriginalEdition.Patch1078, new MissionBattleOptionState
        {
            Options = gameplay.BattleOptions, Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(),
            TypeUse = new byte[256], RangeTable = new float[] { 14, 22, 30, 38 }, Value = aux.RefundPercentBox,
        });
        options.InitializeDensity(new GameRandom(91), aux.ArchipelagoMinimumBox, aux.ArchipelagoMaximumBox, aux.ArchipelagoPercentBox);
        Assert.Equal(15, aux.ArchipelagoMinimum); Assert.Equal(35, aux.ArchipelagoMaximum); Assert.InRange(aux.ArchipelagoPercent, 14u, 32u);
        Assert.Equal(0xfefefefeu, aux.Archipelago); Assert.Equal(0x12345678u, aux.TerrainEnabled);
    }
}
