using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Client.Player;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제Host의Player고정뷰·Core0선행·현재공장/카운터와종료반납을검사한다.
namespace Netstorm.Client.Tests;

/// <summary>장기소유자와실제Session소비자의동일물리/수명계약을확인한다.</summary>
public sealed class HostPlayerRecordsOwnerTests
{
    /// <summary>명시공장입력과같은Core를빌리는실제소유자를만든다.</summary>
    private static ClientPlayerRecords Records(ClientGameplayGlobals game, Action<Span<byte>>? allocation = null)
    {
        var records = new ClientPlayerRecords(allocation ?? (bytes => bytes.Fill(0x45))); records.BindCoreData(game.CoreData); return records;
    }
    /// <summary>같은장기전역/Player를실제Host생성자에전달한다.</summary>
    private static HostSession Session(ClientGameplayGlobals game, ClientPlayerRecords records, bool adopt = false, RawSidPool? pool = null) =>
        new(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: game, pool: pool, sharedPlayerRecords: records, adoptInitializedPlayers: adopt);
    /// <summary>원문자기동맹인자만검사한다.</summary>
    private static void SelfAlliance(uint owner, uint mask) => Assert.Equal(1u << (int)owner, mask);
    /// <summary>같은실제PlayerDWORD를읽는다.</summary>
    private static uint Read(ClientPlayerRecords owner, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(owner.Records.Span[offset..]);

    /// <summary>실제Host가같은배열/동맹/카운터/현재공장헤더를복사없이소비한다.</summary>
    [Fact]
    public void NativeHostUsesSamePhysicalPlayerAllianceAndFactoryHeaders()
    {
        var game = new ClientGameplayGlobals(); var records = Records(game); var session = Session(game, records);
        Assert.Same(records, session.PlayerRecordsOwner); Assert.Same(game.Player, session.MissionPlayer); Assert.Same(records.Alliances, session.Alliances);
        Assert.True(records.Records.Equals((Memory<byte>)session.PlayerRecords)); Assert.True(game.CoreData.Players.Equals(records.Records));
        Assert.Same(records.FortCountBox, session.FortCountBox); Assert.Same(records.LastLoginPlayerBox, session.LastLoginPlayerBox);
        // 실제9행의최초자기동맹/공장용량/별칭목록을확인한다.
        for (uint owner = 0; owner < 9; owner++)
        {
            Assert.Equal(1u << (int)owner, Read(records, (int)owner * 180 + 0x54)); Assert.Equal(127u, Read(records, (int)owner * 180 + 0x78));
            Assert.Equal(30u, session.TerrainBookkeeping.OwnerFactories[owner].Capacity); Assert.True(session.TerrainBookkeeping.OwnerFactories[owner].MatchesCurrentSource(records, owner));
        }
        session.TerrainBookkeeping.OwnerFactories[2].Append(55); Assert.Equal(1u, Read(records, 2 * 180 + 8)); Assert.Equal(55u, records.Factory(2)[0]);
        records.FortCountBox.Value = 7; records.LastLoginPlayerBox.Value = 8; Assert.Equal(7u, session.FortCount); Assert.Equal(8u, session.LastLoginPlayer);
        session.PlayerRecords[100] = 0xaa; Assert.Equal(0xaa, records.Records.Span[100]); session.ReleasePlayerRecordViews();
    }

    /// <summary>재할당전실제known0/Core0선택이기존10바이트캐시를보존하고새base를공개한다.</summary>
    [Fact]
    public void NativeConstructorClearsCurrentCoreBeforeReallocationAndKeepsCache()
    {
        var game = new ClientGameplayGlobals(); var records = Records(game); records.Initialize(SelfAlliance); uint previous = records.Base;
        game.CoreData.Select(game.Player, 0, 2, 3); byte[] selected = Enumerable.Range(1, 10).Select(value => (byte)value).ToArray(); selected.AsSpan().CopyTo(game.CoreData.Current.Span);
        records.FortCountBox.Value = 7; records.LastLoginPlayerBox.Value = 8; var session = Session(game, records);
        Assert.NotEqual(previous, records.Base); Assert.Equal(selected, game.CoreData.Cached()); Assert.True(game.CoreData.Current.IsEmpty);
        Assert.Equal(0u, game.Player.PlayerKnown); Assert.Equal(0u, game.Player.BattleOwner); Assert.Equal(1u, game.Player.TemplateOwner);
        Assert.Equal(0u, session.FortCount); Assert.Equal(0u, session.LastLoginPlayer); Assert.True(game.CoreData.Players.Equals(records.Records)); session.ReleasePlayerRecordViews();
    }

    /// <summary>준비인계는실제주소/전체기록/현재Core/known과두카운터를초기화하지않는다.</summary>
    [Fact]
    public void PreparedAdoptionKeepsInitializedBytesCoreAndCurrentCounters()
    {
        int allocations = 0; var game = new ClientGameplayGlobals(); var records = Records(game, bytes => { allocations++; bytes.Fill(0x67); });
        records.Initialize(SelfAlliance); game.CoreData.Select(game.Player, 0, 4, 3); game.CoreData.Active = 0xabcdef;
        game.CoreData.Current.Span[0] = 0x71; records.FortCountBox.Value = 5; records.LastLoginPlayerBox.Value = 6;
        byte[] before = records.Records.ToArray(); var current = game.CoreData.Current; uint previous = records.Base; var session = Session(game, records, true);
        Assert.Equal(9, allocations); Assert.Equal(previous, records.Base); Assert.Equal(before, records.Records.ToArray()); Assert.True(current.Equals(game.CoreData.Current));
        Assert.Equal(1u, game.Player.PlayerKnown); Assert.Equal(4u, game.Player.BattleOwner); Assert.Equal(3u, game.Player.TemplateOwner); Assert.Equal(0xabcdefu, game.CoreData.ActiveBox.Value);
        Assert.Equal(5u, session.FortCount); Assert.Equal(6u, session.LastLoginPlayer); session.ReleasePlayerRecordViews();
    }

    /// <summary>기존고정뷰가살아있으면다른Session을현재known/Core쓰기전거부하고정리뒤다음할당만허용한다.</summary>
    [Fact]
    public void LiveSessionRejectsSecondBorrowBeforeTouchingPlayerState()
    {
        var game = new ClientGameplayGlobals(); var records = Records(game); var first = Session(game, records); uint previous = records.Base;
        game.Player.PlayerKnown = 7; first.PlayerRecords[100] = 0x98; byte[] before = records.Records.ToArray();
        Assert.Throws<InvalidOperationException>(() => Session(game, records)); Assert.Equal(7u, game.Player.PlayerKnown); Assert.Equal(before, records.Records.ToArray());
        first.ReleasePlayerRecordViews(); first.ReleasePlayerRecordViews(); Assert.Throws<ObjectDisposedException>(() => first.PlayerRecords);
        var second = Session(game, records); Assert.NotEqual(previous, records.Base); Assert.Equal(0u, game.Player.PlayerKnown); second.ReleasePlayerRecordViews();
    }

    /// <summary>초기화부분실패는실제prefix를보존하되실패한생성자의대여만반납한다.</summary>
    [Fact]
    public void ConstructorFailureReturnsBorrowWithoutRollingBackPlayerPrefix()
    {
        int allocations = 0; var game = new ClientGameplayGlobals(); var records = Records(game, bytes => { if (++allocations == 2) throw new IOException("실제두번째공장입력실패"); bytes.Fill(0x29); });
        Assert.Throws<IOException>(() => Session(game, records)); Assert.False(records.Initialized); Assert.NotEqual(0u, records.Base);
        Assert.Equal(30u, Read(records, 4)); Assert.Equal(127u, Read(records, 0x78)); Assert.Equal(0u, Read(records, 180)); Assert.Equal(1u, records.Alliances[0]);
        using (var probe = records.Acquire()) Assert.Throws<InvalidOperationException>(() => probe.AdoptCurrent());
        var recovered = Session(game, records); Assert.True(records.Initialized); recovered.ReleasePlayerRecordViews();
    }

    /// <summary>다른Core/판본/미준비인계는실제현재전역을바꾸기전에거부한다.</summary>
    [Fact]
    public void ForeignCoreEditionAndUninitializedAdoptionAreRejected()
    {
        var game = new ClientGameplayGlobals(); game.Player.PlayerKnown = 7; var other = Records(new ClientGameplayGlobals());
        Assert.Throws<ArgumentException>(() => Session(game, other)); Assert.Equal(7u, game.Player.PlayerKnown); Assert.Equal(0u, other.Base);
        var cd = new ClientPlayerRecords(bytes => bytes.Fill(1), OriginalEdition.Cd1072); cd.BindCoreData(game.CoreData);
        Assert.Throws<ArgumentException>(() => Session(game, cd)); Assert.Equal(7u, game.Player.PlayerKnown);
        var records = Records(game); Assert.Throws<InvalidOperationException>(() => Session(game, records, true)); Assert.Equal(7u, game.Player.PlayerKnown);
        using var use = records.Acquire(); Assert.Equal(0u, records.Base);
    }

    /// <summary>실제배송중자식정리/고정뷰반납을막고마지막Runtime반납뒤새대여를허용한다.</summary>
    [Fact]
    public void ActualRuntimeDeliveryGuardsPlayerViewReturnUntilUnwind()
    {
        var game = new ClientGameplayGlobals(); var records = Records(game); var storage = new RawSurfaceStorage(); var session = Session(game, records, pool: storage.Pool);
        var runtime = HostMissionRuntime.Create(session, storage, []); var pin = session.PinRuntime(); runtime.Dispose(); Assert.False(runtime.Released);
        Assert.Throws<InvalidOperationException>(session.ReleasePlayerRecordViews); Assert.Throws<InvalidOperationException>(() => records.Acquire());
        pin.Dispose(); Assert.True(runtime.Released); session.ReleasePlayerRecordViews();
        var next = Session(game, records); Assert.True(records.Initialized); next.ReleasePlayerRecordViews();
    }
}
