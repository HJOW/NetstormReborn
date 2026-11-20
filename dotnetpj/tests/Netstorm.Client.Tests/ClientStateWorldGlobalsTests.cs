using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 Client 소유자와 Native 폭별 접근의 같은 물리를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>전역 복사 없이 현재 owner를 읽고 쓰며 정리 부모와 실제 Session이 공유하는지 확인한다.</summary>
public sealed class ClientStateWorldGlobalsTests
{
    /// <summary>동일한 실제 장기 owner를 구성하고 설치 자체의 무효과를 검사하는 입력이다.</summary>
    private static ClientStateWorldContext Context()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        return new() { Gameplay = game, Local = local, Connections = new(local), Surface = new(OriginalEdition.Patch1078, game.Authority, game.GameAux), Templates = new(), Cloud = new(), Popup = new(), NsInfoZone = new(), FortCoreSeed = new(), SongEnd = new() };
    }

    /// <summary>활동 표 전체와 이웃 Zone/벽시각의 독립성, lowWORD 및 아홉 double의 원시 비트를 검증한다.</summary>
    [Fact]
    public void WidthStoresPreserveActivityNeighborsSeedAndWideBits()
    {
        var c = Context(); var g = ClientStateWorldGlobals.Bind(c); var a = c.Connections.Activity;
        a.ActiveStamp.Value = 0x12345678; a.ActiveZoneWord.Value = 0x87654321; Array.Fill(a.WallSeconds, 0xabcddcbau);
        // 같은22칸만 쓰고 간극과 벽시각 표를 보존한다.
        for (uint i = 0; i < 22; i++) { a.Frames[i] = i + 1; Assert.Equal(i + 1, g.Read32!(0x5c84e8 + i * 4)); g.Write32!(0x5c84e8 + i * 4, uint.MaxValue); Assert.Equal(uint.MaxValue, a.Frames[i]); }
        Assert.Equal(0x12345678u, a.ActiveStamp.Value); Assert.Equal(0x87654321u, a.ActiveZoneWord.Value); Assert.All(a.WallSeconds, v => Assert.Equal(0xabcddcbau, v));
        c.FortCoreSeed.Value = 0xabcd7654; g.Write16!(0x5c89b4, 0x1234); Assert.Equal(0xabcd1234u, c.FortCoreSeed.Value); g.Write16(0x5c89b4, 0); Assert.Equal(0xabcd0000u, c.FortCoreSeed.Value);
        (uint Address, Func<double> Read)[] wide = [(0x565e68, () => c.SongEnd.Value), (0x5950b8, () => c.Gameplay.GameAux.GeyserDeadline), (0x5950c0, () => c.Gameplay.GameAux.CleanupDeadline), (0x5950c8, () => c.Gameplay.GameAux.SecondaryGeyserDeadline), (0x595358, () => c.Gameplay.GameAux.ScheduledDrawDeadline), (0x5c85f0, () => c.Local.LoginEnd.Value), (0x5c85f8, () => c.Local.ConnectEnd.Value), (0x5c8610, () => c.Local.ReconnectSaveEnd.Value), (0x5c8618, () => c.Local.CaptureEnd.Value)];
        // NaN payload와 부호 있는0을 원문64비트로 보존한다.
        foreach (var field in wide) foreach (ulong bits in new ulong[] { 0x8000000000000000, 0x7ff8000012345678, 0 }) { g.Write64Bits!(field.Address, bits); Assert.Equal(bits, BitConverter.DoubleToUInt64Bits(field.Read())); }
    }

    /// <summary>bool 쓰기의 선행 거부와 Fort/Player·Battle/Bridge 별칭, signed/float 비트 보존을 검사한다.</summary>
    [Fact]
    public void BoolModesAndRawSignedFloatBitsUseActualOwners()
    {
        var c = Context(); var g = ClientStateWorldGlobals.Bind(c); var aux = c.Gameplay.GameAux;
        // bool을 표현하는 실제 여덟 저장 위치만 정상0/1을 허용한다.
        foreach (uint address in new uint[] { 0x540bc0, 0x540bc4, 0x594fa4, 0x594fb8, 0x594fbc, 0x594fc8, 0x5c85d4, 0x5ca8f0 }) { g.Write32!(address, 0); Assert.Equal(0u, g.Read32!(address)); g.Write32(address, 1); Assert.Throws<ArgumentOutOfRangeException>(() => g.Write32(address, 2)); Assert.Equal(1u, g.Read32(address)); }
        Assert.True(c.Gameplay.Player.FortMode); Assert.True(c.Surface.Links.Battle);
        g.Write32!(0x594fb8, 0); g.Write32(0x594fbc, 0); Assert.False(c.Gameplay.Player.FortMode); Assert.False(c.Surface.Links.Battle);
        // 부호와 NaN을 bool처럼 축약하지 않는다.
        foreach (uint bits in new uint[] { 0, 0x80000000, 0xffffffff, 0x7fc12345 })
        {
            // 각 Native DWORD의 같은 전체 비트를 직접 쓴다.
            foreach (uint address in new uint[] { 0x595088, 0x59508c, 0x5453e4, 0x5c848c, 0x5c8490, 0x568afc, 0x568b00 }) { g.Write32!(address, bits); Assert.Equal(bits, g.Read32!(address)); }
            Assert.Equal(bits, aux.ResourcePoolBits()); Assert.Equal(bits, unchecked((uint)aux.TotalCost)); Assert.Equal(bits, unchecked((uint)aux.AllowAnyCapture)); Assert.Equal(bits, BitConverter.SingleToUInt32Bits(aux.CentralPosition[0]));
        }
    }

    /// <summary>현재 Player 재할당·Template 헤더 변경·TYPE 교체가 캡처 사본을 통하지 않고 보인다.</summary>
    [Fact]
    public void ReadsCurrentPlayerTypeAndHeaderAfterBinding()
    {
        var c = Context(); var g = ClientStateWorldGlobals.Bind(c); Assert.True(g.Players!().IsEmpty);
        c.Gameplay.PlayerRecords.Initialize((_, _) => { }); var first = g.Players(); uint oldBase = c.Gameplay.PlayerRecords.Base;
        c.Gameplay.PlayerRecords.Initialize((_, _) => { }); Assert.NotEqual(oldBase, c.Gameplay.PlayerRecords.Base); Assert.NotEqual(first, g.Players()); Assert.Equal(c.Gameplay.PlayerRecords.Records, g.Players());
        BinaryPrimitives.WriteUInt32LittleEndian(c.Templates.Headers.Span.Slice(20, 4), 0x12345678); Assert.Equal(0x12345678u, g.TemplateManager!(1)); BinaryPrimitives.WriteUInt32LittleEndian(c.Templates.Headers.Span.Slice(20, 4), 0x87654321); Assert.Equal(0x87654321u, g.TemplateManager(1));
        c.Surface.Types.AddRange([new OriginalTypeRecord { Cost = 1 }]); Assert.Same(c.Surface.Types[0], g.TypeDefault!(0)); g.TypeDefault(0).Cost = 123.5f; Assert.Equal(123.5f, c.Surface.Types[0].Cost);
        c.Surface.Types[0] = new() { Cost = 77 }; Assert.Same(c.Surface.Types[0], g.TypeDefault(0));
        BinaryPrimitives.WriteUInt32LittleEndian(c.Local.State.Phase.Span, 33); Assert.Equal(33u, g.Read32!(0x5c8758));
    }

    /// <summary>정리 부모가 실제 Player/TYPE/로컬 owner를 쓰며 하위 효과는 명시된 관측 경계에 남긴다.</summary>
    [Fact]
    public void NativeParentWritesSameOwnersAndPreservesPopupArgument()
    {
        var c = Context(); var game = c.Gameplay; var aux = game.GameAux; game.PlayerRecords.Initialize((_, _) => { });
        c.Surface.Types.AddRange(Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()));
        BinaryPrimitives.WriteUInt32LittleEndian(c.Local.State.Phase.Span, 33); aux.PriestQuarterHp = true; game.HideBuildings.Value = 7; c.Popup.Command = 0xabcdef; Array.Fill(c.Popup.ArgumentBytes, (byte)'z'); c.FortCoreSeed.Value = 0xabcdefff;
        var effects = new List<StateWorldCleanupEffect>(); var disconnected = new List<uint>(); int ai = 0, priest = 0, other = 0, pools = 0;
        RawStateWorldCleanup.CleanupStateWorld(0, ClientStateWorldGlobals.Bind(c), new()
        {
            Effect = effects.Add, Disconnect = disconnected.Add, PauseDepth = () => 0, ResetAi = _ => ai++, ResetPriest = _ => priest++, ResetOtherPlayer = (_, value) => { Assert.Equal(0u, value); other++; }, SetSp = (_, _) => { }, SelectCore = (_, _) => { }, ResetPool = mode => { Assert.Equal(0u, mode); pools++; }, WallSeconds = () => 123.5
        });
        Assert.Equal(new uint[] { 10, 11 }, disconnected); Assert.Equal(8, ai); Assert.Equal(8, priest); Assert.Equal(8, other); Assert.Equal(1, pools);
        Assert.Equal(StateWorldCleanupEffect.CloseAllConnections, effects[0]); Assert.Equal(StateWorldCleanupEffect.MapMode, effects[^1]);
        Assert.False(aux.PriestQuarterHp); Assert.Equal(0u, game.HideBuildings.Value); Assert.Equal(0u, c.Popup.Command); Assert.Equal((byte)'z', c.Popup.ArgumentBytes[0]); Assert.Equal(0xabcd0000u, c.FortCoreSeed.Value); Assert.Equal(133.5, c.Local.ReconnectSaveEnd.Value);
        Assert.False(game.Authority.Server); Assert.False(game.Authority.Boss); Assert.Equal(2000f, c.Surface.Types[122].Cost); Assert.Equal(uint.MaxValue, c.Local.CachedPeerIndex.Value); Assert.Equal(33u, BinaryPrimitives.ReadUInt32LittleEndian(c.Local.State.Phase.Span));
    }

    /// <summary>미등록 폭/주소·활동 표 간극·잘못된 실제 소유자는 필드 변경 전에 거부한다.</summary>
    [Fact]
    public void RejectsUnknownWidthsAndForeignOwnersBeforeWriting()
    {
        var c = Context(); c.Gameplay.FullDeckBox.Value = 0x81234567; var g = ClientStateWorldGlobals.Bind(c);
        // 표 끝과 이웃 Zone/벽시각을 활동 프레임으로 쓰지 않는다.
        foreach (uint address in new uint[] { 0x5c84e9, 0x5c8540, 0x5c8544, 0x5c8548, 0xdeadbeef }) { Assert.Throws<ArgumentOutOfRangeException>(() => g.Read32!(address)); Assert.Throws<ArgumentOutOfRangeException>(() => g.Write32!(address, 0)); }
        Assert.Throws<ArgumentOutOfRangeException>(() => g.Write16!(0x5c89b6, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => g.Write64Bits!(0x5c8614, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => g.TemplateManager!(9));
        Assert.Throws<ArgumentException>(() => ClientStateWorldGlobals.Bind(c with { Surface = new() }));
        Assert.Throws<ArgumentException>(() => ClientStateWorldGlobals.Bind(c with { Connections = new(new(new(), new())) })); Assert.Equal(0x81234567u, c.Gameplay.FullDeckBox.Value);
    }

    /// <summary>두 실제 Session의 월드 모드와 지형 갱신은 장기 Gameplay 필드의 같은 주소로 유지된다.</summary>
    [Fact]
    public void SessionsUseSameWorldAndRevisionWithoutConstructorReset()
    {
        var game = new ClientGameplayGlobals(); game.World.Editor = 7; game.World.MissionPost = false; game.MapRevision.Value = uint.MaxValue;
        var first = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: game); Assert.Same(game.World, first.WorldGlobals); Assert.Equal(uint.MaxValue, first.IslandRevision);
        first.IslandRevision = 73; var second = new HostSession(new Kernel(), new GameRandom(29), new CrtRandom(31), sharedGameplay: game); Assert.Same(first.WorldGlobals, second.WorldGlobals); Assert.Equal(73u, second.IslandRevision); Assert.Equal(7u, second.WorldGlobals.Editor); Assert.False(second.WorldGlobals.MissionPost);
    }
}
