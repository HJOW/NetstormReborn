using System.Buffers.Binary;
using Netstorm.Core.Original;

// State 작은 자식이 실제 장기 저장소를 선택해서만 변경하는지 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 대여 관문·같은 주소·표 경계와 Frame 시각을 검증한다.</summary>
public sealed class ClientStateStorageCleanupTests
{
    /// <summary>게임값을 별도 복제하지 않고 실제 Client/Frame/표면을 구성한다.</summary>
    private static (ClientSurfaceGlobals Surface, ClientLocalStateGlobals Local, IntBox Multiple, ClientStateStorageCleanup Cleanup) Scene()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var surface = new ClientSurfaceGlobals(game, new(local), new(), () => throw new IOException("정리중fresh시각조회금지")); var multiple = new IntBox(); return (surface, local, multiple, new(surface, local, multiple));
    }

    /// <summary>빌린 월드의 색 표는 지우되 다른 아홉 물리와 Pool은 반납 전 거부한다.</summary>
    [Fact]
    public void OnlyOwnerColorsMayResetBeforeWorldReturn()
    {
        var (surface, local, multiple, cleanup) = Scene(); var storage = surface.Storage; Array.Fill(storage.Links.OwnerColors, 6); Array.Fill(local.PlayerNoMakeWarnings, 0x80000001u); var scan = storage.BridgeDecayScan; scan.First = 31; scan.Next = -123.5;
        using (storage.AcquirePrepared())
        {
            cleanup.ResetWorldStorage(StateWorldCleanupEffect.Clear43b540); Assert.Equal(Enumerable.Range(0, 9), storage.Links.OwnerColors);
            // 해당 효과가 다른 표나 Pool의 선행 반납을 생략할 수 없다.
            foreach (var effect in new[] { StateWorldCleanupEffect.Clear472830, StateWorldCleanupEffect.Clear4c2450, StateWorldCleanupEffect.Clear4b2b50, StateWorldCleanupEffect.Clear4328a0, StateWorldCleanupEffect.Clear4ab940, StateWorldCleanupEffect.Reset568b60, StateWorldCleanupEffect.Reset54c8a0, StateWorldCleanupEffect.ResetGraph, StateWorldCleanupEffect.Reset422490 }) Assert.Throws<InvalidOperationException>(() => cleanup.ResetWorldStorage(effect));
            Assert.Throws<InvalidOperationException>(() => cleanup.ResetPool()); Assert.Equal(31, scan.First); Assert.Equal(-123.5, scan.Next); Assert.All(local.PlayerNoMakeWarnings, v => Assert.Equal(0x80000001u, v));
        }
        cleanup.ResetWorldStorage(StateWorldCleanupEffect.Clear4328a0); Assert.All(local.PlayerNoMakeWarnings, v => Assert.Equal(0u, v)); Assert.Throws<ArgumentOutOfRangeException>(() => cleanup.ResetWorldStorage(StateWorldCleanupEffect.ReplaceUber));
        Assert.Throws<ArgumentException>(() => new ClientStateStorageCleanup(surface, new(new(), new()), multiple));
    }

    /// <summary>등록 목록은 개수만 지우고 세 통계표255칸과 spot/Graph의 정확한 보존 영역을 지킨다.</summary>
    [Fact]
    public void ResetsSelectedListsCountsSpotsAndGraphWithoutClearingNeighbors()
    {
        var (surface, _, multiple, cleanup) = Scene(); var s = surface.Storage; var headers = s.SidLists.RegisteredHeaders.ToArray(); var list = s.SidLists.Factories; list.Count = 1; list.Storage[0] = 0x12345678; multiple.Value = 8;
        cleanup.ResetWorldStorage(StateWorldCleanupEffect.Clear472830); Assert.Equal(0u, list.Count); Assert.Equal(0x12345678u, list.Storage[0]); Assert.Equal(0, multiple.Value); Assert.Equal(headers, s.SidLists.RegisteredHeaders);
        Array.Fill(s.TotalMade.LocalCounts, 0x12345678u); Array.Fill(s.TotalMade.LocalSecondaryCounts, 0xabcdef01u); Array.Fill(s.TotalMade.GlobalCounts, 0x87654321u); cleanup.ResetWorldStorage(StateWorldCleanupEffect.Clear4c2450);
        Assert.All(s.TotalMade.LocalCounts[..255], v => Assert.Equal(0u, v)); Assert.Equal(0x12345678u, s.TotalMade.LocalCounts[255]); Assert.Equal(0xabcdef01u, s.TotalMade.LocalSecondaryCounts[255]); Assert.Equal(0x87654321u, s.TotalMade.GlobalCounts[255]);
        Array.Fill(s.Spots, (byte)0xb3); cleanup.ResetWorldStorage(StateWorldCleanupEffect.Clear4ab940); Assert.All(s.Spots, v => Assert.Equal(0, v));
        s.Graph.Records[0].Reserved = unchecked((short)0xabcd); s.Graph.Records[252].Surfaces = 71; s.Graph.Stack[0] = 0x12345678; cleanup.ResetWorldStorage(StateWorldCleanupEffect.ResetGraph); Assert.Equal(unchecked((short)0xab01), s.Graph.Records[0].Reserved); Assert.Equal(71, s.Graph.Records[252].Surfaces); Assert.Equal(0x12345678u, s.Graph.Stack[0]); Assert.Equal(0x7dfd, s.Graph.Records[254].InUse);
    }

    /// <summary>시계 동기화·Zone은 인접 로컬 블록/프레임을 유지하고 스캔은 현재 캐시Frame 시각을 읽는다.</summary>
    [Fact]
    public void ClockZoneAndScanKeepSharedOwnersAndUseCachedFrame()
    {
        var (surface, local, _, cleanup) = Scene(); local.ClockSynchronization.Connection.Value = 7; local.ClockSynchronization.ReplyMode.Value = 9; local.GameTicks.Value = 11; local.SourceType.Value = 13;
        local.ZoneStorage.Span.Fill(0xa5); BinaryPrimitives.WriteUInt32LittleEndian(local.State.Current.Span, 33); surface.Clock.Frame.Value.Game = 37.25;
        cleanup.ResetClockSynchronization(); Assert.Equal(0u, local.GameTicks.Value); Assert.Equal(0u, local.ClockSynchronization.Connection.Value); Assert.Equal(0u, local.ClockSynchronization.ReplyMode.Value); Assert.Equal(13u, local.SourceType.Value); Assert.Equal(37.25, surface.Clock.Frame.Value.Game);
        cleanup.ResetZone(); Assert.Equal(new byte[6], local.Zone.Span.ToArray()); Assert.Equal(0xa5, local.ZoneStorage.Span[6]); Assert.Equal(0xa5, local.ZoneStorage.Span[7]); Assert.Equal(33u, BinaryPrimitives.ReadUInt32LittleEndian(local.State.Current.Span));
        var scan = surface.Storage.BridgeDecayScan; surface.Storage.BridgeDecayPeriod = 7.75f; cleanup.ResetWorldStorage(StateWorldCleanupEffect.Reset422490); Assert.Same(scan, surface.Storage.BridgeDecayScan); Assert.Equal(15000, scan.First); Assert.Equal(23001, scan.Last); Assert.Equal(15000, scan.Cursor); Assert.Equal(45, scan.Next); Assert.Equal(7.75f, surface.Storage.BridgeDecayPeriod);
    }

    /// <summary>Pool 부모가 같은 실패점을 직접 쓰고 별도 모드/Graph/Player는 바꾸지 않는다.</summary>
    [Fact]
    public void PoolResetWritesSameInvalidPointAndPreservesOtherStorage()
    {
        var (surface, _, _, cleanup) = Scene(); var s = surface.Storage; s.Pool.PredictableCursorBox.Value = 73; s.BoardTargetState.Invalid = new() { X = 17, Y = 19 }; s.GameAux().ResourcePool = 71; s.Graph.Stack[0] = 73;
        cleanup.ResetPool(); Assert.Equal(1u, s.Pool.PredictableCursor); Assert.Equal(-10000f, s.BoardTargetState.Invalid.X); Assert.Equal(-10000f, s.BoardTargetState.InvalidStorage.Y); Assert.Equal(71, s.GameAux().ResourcePool); Assert.Equal(73u, s.Graph.Stack[0]);
    }
}
