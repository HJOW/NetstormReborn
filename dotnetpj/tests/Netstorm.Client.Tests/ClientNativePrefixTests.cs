using System.Buffers.Binary;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 첫메뉴전의실제장기시계/보정/Player초기화를같은소유자에서검사한다.
namespace Netstorm.Client.Tests;

/// <summary>원문init full time과known0부모의정확한쓰기범위/주소수명을검증한다.</summary>
public sealed class ClientNativePrefixTests
{
    /// <summary>같은실제PlayerDWORD를읽는다.</summary>
    private static uint Read(ClientGameplayGlobals game, int at) => BinaryPrimitives.ReadUInt32LittleEndian(game.PlayerRecords.Records.Span[at..]);
    /// <summary>소유자생성은값을쓰지않고명시시계초기화는같은객체/외부Frame주소/실제보정을공유한다.</summary>
    [Fact]
    public void ClockOwnerPreservesObjectsAndUsesCurrentSynchronizationWord()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); local.GameTicks.Value = 0; var owner = new ClientClockGlobals(local); GameClock clock = owner.Clock; FrameTimeBox frame = owner.Frame;
        frame.Value = new() { Game = 77, Number = 8 }; Assert.False(owner.Initialized); Assert.False(game.PlayerRecords.Initialized); Assert.Throws<InvalidOperationException>(() => owner.CaptureFrame(1));
        owner.InitializeFullTime(1000); Assert.Same(clock, owner.Clock); Assert.Same(frame, owner.Frame); Assert.Equal(77, frame.Value.Game); Assert.Equal(8ul, frame.Value.Number);
        FrameTime first = owner.CaptureFrame(1025); Assert.Equal(0.025, first.Game); Assert.Equal(first.Game, frame.Value.Game); Assert.Equal(1ul, frame.Value.Number);
        local.GameTicks.Value = 10; FrameTime second = owner.CaptureFrame(1050); Assert.Equal(0.04, second.Game); Assert.Equal(0.05, second.Wall); Assert.Equal(2ul, second.Number); Assert.Equal(second.Delta, frame.Value.Delta);
        Assert.Throws<InvalidOperationException>(() => owner.InitializeFullTime(99)); Assert.Equal(10u, local.GameTicks.Value); Assert.Equal(2ul, frame.Value.Number);
    }
    /// <summary>known0공통시작이아홉공장과81동맹을쓰고기존옵션/숨김/현재Player상태를읽거나덮지않는다.</summary>
    [Fact]
    public void PlayerPrefixInitializesSamePhysicalOwnerBeforeAnySession()
    {
        var game = new ClientGameplayGlobals(); Array.Fill(game.BattleOptions, (byte)0xa5); game.Player.BattleOwner = 0x12345678; game.Player.TemplateOwner = 0x87654321;
        byte[] options = game.BattleOptions.ToArray(); ClientPlayerRuntime.Initialize(game); Assert.True(game.PlayerRecords.Initialized); Assert.True(game.CoreData.Players.Equals(game.PlayerRecords.Records));
        Assert.Equal(0u, game.Player.PlayerKnown); Assert.Equal(0x12345678u, game.Player.BattleOwner); Assert.Equal(0x87654321u, game.Player.TemplateOwner); Assert.Equal(options, game.BattleOptions); Assert.False(game.Borrowed());
        // 같은실제9행의자기마스크/용량/방향표를모두대조한다.
        for (int owner = 0; owner < 9; owner++)
        {
            Assert.Equal(30u, Read(game, owner * 180 + 4)); Assert.Equal(0u, Read(game, owner * 180 + 8)); Assert.Equal(1u << owner, Read(game, owner * 180 + 0x54));
            // 각방향DWORD는현재행의자기대상만켜져야한다.
            for (int target = 0; target < 9; target++) Assert.Equal(owner == target ? 1u : 0u, game.PlayerRecords.Alliances[owner * 9 + target]);
        }
        uint previous = game.PlayerRecords.Base; ClientPlayerRuntime.Initialize(game); Assert.NotEqual(previous, game.PlayerRecords.Base); Assert.Equal(0u, game.PlayerRecords.FortCountBox.Value); Assert.Equal(0u, game.PlayerRecords.LastLoginPlayerBox.Value);
    }
    /// <summary>초기known관문은할당/쓰기전에거부하며살아있는고정뷰대여도기존Player물리를보존한다.</summary>
    [Fact]
    public void KnownAndCurrentViewGatesPreserveExistingOwner()
    {
        var game = new ClientGameplayGlobals(); game.Player.PlayerKnown = 1; Assert.Throws<InvalidOperationException>(() => ClientPlayerRuntime.Initialize(game)); Assert.False(game.PlayerRecords.Initialized);
        game.Player.PlayerKnown = 0; ClientPlayerRuntime.Initialize(game); byte[] before = game.PlayerRecords.Records.ToArray(); uint address = game.PlayerRecords.Base;
        using var use = game.PlayerRecords.Acquire(); Assert.Throws<InvalidOperationException>(() => ClientPlayerRuntime.Initialize(game)); Assert.Equal(address, game.PlayerRecords.Base); Assert.Equal(before, game.PlayerRecords.Records.ToArray());
    }
}
