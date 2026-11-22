using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 실제 장기 Player/Deck/물리와 Session 설치 전 경계를 검사한다.

/// <summary>Source ClientProductionSync가 빈 메뉴도 생략하지 않고 같은 현재 소유자를 쓰는지 확인한다.</summary>
public sealed class ClientProductionSyncTests
{
    /// <summary>실제 자산과 같은 장기 Player/Clock/Surface만 준비한 메뉴 상태다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 실제 게임 전역이다.</summary>
        public ClientGameplayGlobals Game { get; } = new();
        /// <summary>장기 물리와 동일 Kernel/Clock의 실제 owner다.</summary>
        public ClientSurfaceGlobals Surface { get; }
        /// <summary>같은 로딩 깊이 주소를 Surface와 Session에 공급하는 실제 Template owner다.</summary>
        public CampaignTemplateStorage Templates { get; } = new();
        /// <summary>같은 실제 Player/옵션/물리 참조다.</summary>
        public ProductionControlContext Context { get; }
        /// <summary>TYPE188을 같은 물리에 설치하고 원문 첫 Player 초기화를 실행한다.</summary>
        public Scene()
        {
            Surface = new(Game, new(new ClientLocalStateGlobals(Game, new())), new(), () => 0,
                installDevices: surface => surface.Storage.BindSerializationGlobals(Templates.SerializationGlobals));
            var startup = new HostStartupScene(GameDataLocator.FindDataDirectory() ?? throw new InvalidOperationException("실제 검사 자산 없음"));
            _ = new HostAssetTables(startup, Surface.Storage); ClientPlayerRuntime.Initialize(Game);
            Context = new() { Storage = Surface.Storage, Gameplay = Game, Players = Game.PlayerRecords, Options = Game.BattleOptions };
        }
        /// <summary>같은 현재 Player+68에 기록된 실제 Deck 토큰이다.</summary>
        public uint Token(uint owner) => BinaryPrimitives.ReadUInt32LittleEndian(Game.PlayerRecords.Records.Span.Slice((int)owner * 180 + 0x68, 4));
    }

    /// <summary>known0에서도 실제 Deck getter가 Player+68에 쓰며 다른 필드나 물리/검색 owner를 바꾸지 않는다.</summary>
    [Fact]
    public void WorldlessUnknownPlayerStillAllocatesActualDeckOnce()
    {
        var scene = new Scene(); scene.Game.Player.TemplateOwner = 2; byte[] before = scene.Game.PlayerRecords.Records.ToArray(); byte[] pool = scene.Surface.Storage.Pool.Bytes.ToArray(); var finder = scene.Game.ProductionFinder;
        Assert.Equal(0u, scene.Token(2)); ClientProductionSync.SynchronizeProductionControlContents(scene.Context);
        uint token = scene.Token(2); Assert.NotEqual(0u, token); Assert.False(scene.Game.Borrowed()); Assert.False(scene.Surface.Storage.Borrowed());
        byte[] expected = before.ToArray(); BinaryPrimitives.WriteUInt32LittleEndian(expected.AsSpan(2 * 180 + 0x68), token);
        Assert.Equal(expected, scene.Game.PlayerRecords.Records.ToArray()); Assert.Equal(pool, scene.Surface.Storage.Pool.Bytes.ToArray()); Assert.Same(finder, scene.Game.ProductionFinder);
        Assert.Equal(0, BinaryPrimitives.ReadInt32LittleEndian(scene.Game.ProductionDeckStorage.Bytes(token).Span[0x14c..]));
        ClientProductionSync.SynchronizeProductionControlContents(scene.Context); Assert.Equal(token, scene.Token(2)); Assert.Equal(expected, scene.Game.PlayerRecords.Records.ToArray());
    }

    /// <summary>현재 templateOwner를 매 호출 읽고 이미 할당된 같은 블록을 보존한다.</summary>
    [Fact]
    public void CurrentTemplateOwnerSelectsSameLongLivedDeckStorage()
    {
        var scene = new Scene(); scene.Game.Player.TemplateOwner = 1; ClientProductionSync.SynchronizeProductionControlContents(scene.Context); uint first = scene.Token(1);
        scene.Game.Player.TemplateOwner = 3; ClientProductionSync.SynchronizeProductionControlContents(scene.Context); uint second = scene.Token(3);
        Assert.NotEqual(first, second); Assert.Equal(first, scene.Token(1)); Assert.Same(scene.Game.ProductionDeckStorage, scene.Context.Players.ProductionDeckStorage);
        scene.Game.Player.TemplateOwner = 1; ClientProductionSync.SynchronizeProductionControlContents(scene.Context); Assert.Equal(first, scene.Token(1)); Assert.Equal(second, scene.Token(3));
    }

    /// <summary>빈 메뉴라는 이유로 현재 Deck의 손상된 개수를 건너뛰지 않는다.</summary>
    [Fact]
    public void UnknownPlayerDoesNotSkipCurrentDeckValidation()
    {
        var scene = new Scene(); ClientProductionSync.SynchronizeProductionControlContents(scene.Context); uint token = scene.Token(scene.Game.Player.TemplateOwner);
        Memory<byte> deck = scene.Game.ProductionDeckStorage.Bytes(token); BinaryPrimitives.WriteInt32LittleEndian(deck.Span[0x14c..], 67); byte[] before = deck.ToArray();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientProductionSync.SynchronizeProductionControlContents(scene.Context));
        Assert.Equal(before, deck.ToArray()); Assert.Equal(token, scene.Token(scene.Game.Player.TemplateOwner));
    }

    /// <summary>실제 생성 효과를 선택하면 Session 부재를 드러내고 Deck을 대신 수정하거나 가짜 생성하지 않는다.</summary>
    [Fact]
    public void SelectedCreateRequiresActualSessionAndPreservesDeck()
    {
        var scene = new Scene(); ClientProductionSync.SynchronizeProductionControlContents(scene.Context); Memory<byte> deck = scene.Game.ProductionDeckStorage.Bytes(scene.Token(scene.Game.Player.TemplateOwner));
        deck.Span[0] = 82; deck.Span[2] = 1; BinaryPrimitives.WriteInt32LittleEndian(deck.Span[0x14c..], 1); byte[] before = deck.ToArray();
        var error = Assert.Throws<InvalidOperationException>(() => ClientProductionSync.SynchronizeProductionControlContents(scene.Context)); Assert.Contains("Runtime 미연결", error.Message); Assert.Equal(before, deck.ToArray());
    }

    /// <summary>known 상태에서 현재 owner/기술 DWORD를 실제 기록에서 읽고 전체 폭 범위를 거부한다.</summary>
    [Theory]
    [InlineData(9u, 0u)]
    [InlineData(1u, uint.MaxValue)]
    public void CurrentKnownPlayerFieldsRejectInvalidOwnerAndTechnology(uint owner, uint technology)
    {
        var scene = new Scene(); scene.Game.Player.PlayerKnown = 1; scene.Game.Player.BattleOwner = owner;
        if (owner < 9) BinaryPrimitives.WriteUInt32LittleEndian(scene.Game.PlayerRecords.Records.Span.Slice((int)owner * 180 + 0x9c, 4), technology);
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientProductionSync.SynchronizeProductionControlContents(scene.Context));
    }

    /// <summary>Surface만 대여 중이어도 Session 선택이 Deck getter보다 먼저 실패한다.</summary>
    [Fact]
    public void BorrowedSurfaceDoesNotFallThroughToWorldlessDeck()
    {
        var scene = new Scene(); byte[] before = scene.Game.PlayerRecords.Records.ToArray(); using var use = scene.Surface.Storage.AcquirePrepared();
        Assert.Throws<InvalidOperationException>(() => ClientProductionSync.SynchronizeProductionControlContents(scene.Context)); Assert.Equal(before, scene.Game.PlayerRecords.Records.ToArray());
    }

    /// <summary>실제 공개 Runtime이 있어도 부분 적재를 Source loaded로 간주하지 않고 설치 관문이 먼저 거부한다.</summary>
    [Fact]
    public void PublishedRuntimeDoesNotImplySourceInstallationComplete()
    {
        var scene = new Scene(); var session = new HostSession(scene.Surface.Kernel, new GameRandom(1), new CrtRandom(), sharedSurface: scene.Surface, sharedTemplates: scene.Templates, adoptInitializedPlayers: true);
        using (HostMissionRuntime.Create(session, scene.Surface.Storage))
        {
            byte[] before = scene.Game.PlayerRecords.Records.ToArray();
            var error = Assert.Throws<InvalidOperationException>(() => ClientProductionSync.SynchronizeProductionControlContents(scene.Context));
            Assert.Equal("설치 전 생산 패널 갱신", error.Message); Assert.Equal(before, scene.Game.PlayerRecords.Records.ToArray());
            Assert.Throws<InvalidOperationException>(session.CancelProductionCursor);
        }
        session.ReleasePlayerRecordViews();
    }
}
