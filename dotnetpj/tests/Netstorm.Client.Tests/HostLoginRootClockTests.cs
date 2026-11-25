using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// SL-1 전체 로그인·Root 등록·Clock의 제품 순서를 세션에서 고정한다.
// 2026-11-26 1.051에서 추가했다. 제품 StartMission·PrepareNativeFrameInput·
// InitializeNativePrefix·Dispose와 같은 순서를 돌며 미연결 진단을 남기지 않는다.
public sealed class HostLoginRootClockTests
{
    /// <summary>thewarbegins 대본·요새·타입 표를 실제 자산에서 읽는다.</summary>
    /// <returns>시작 장면과 대본·요새·타입 표다.</returns>
    private static (HostStartupScene Startup, Netstorm.Client.Mission.MissionScript Script, MissionFortLoadResult Fort, RiftTypeTable Types) LoadTheWarBeginsAssets()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        var script = new Netstorm.Client.Mission.MissionScript(startup.Configuration, "thewarbegins");
        RiftTypeTable types = MissionFortLoad.LoadTypeTable(startup.Files);
        return (startup, script, fort, types);
    }

    /// <summary>제품 StartMission과 같은 바이트로 단일 바인딩 Zone을 만든다.</summary>
    /// <param name="handle">클라이언트 이름이다.</param>
    /// <returns>헤더 15바이트+슬롯 71바이트 Zone이다.</returns>
    private static byte[] ProductSingleBindingZone(string handle)
    {
        var zone = new byte[LocalMissionZone.ZoneHead + LocalMissionZone.ZoneClient];
        zone[14] = 1;
        int zoneSlot = LocalMissionZone.ZoneHead;
        zone[zoneSlot] = 0;
        for (int index = 0; index < 4; ++index)
            zone[zoneSlot + 1 + index] = unchecked((byte)(1u >> (8 * index)));
        byte[] zoneHandle = System.Text.Encoding.Latin1.GetBytes(handle);
        Array.Copy(zoneHandle, 0, zone, zoneSlot + 9, zoneHandle.Length);
        return zone;
    }

    /// <summary>제품과 같은 선행 바이트에서 같은 기록 DWORD를 읽는다.</summary>
    private static uint DwordAt(byte[] records, int owner, int offset) =>
        System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(records.AsSpan(owner * 180 + offset, 4));

    /// <summary>제품 로그인 순서(Zone→SP→AI설정→Players→Masks→순서→가입→수신→Ready→송신→이름)가 기준선 상태에 닿는다.</summary>
    [Fact]
    public void ProductLoginOrderReachesJoinReadyAndBattleName()
    {
        (HostStartupScene startup, Netstorm.Client.Mission.MissionScript script, MissionFortLoadResult fort, RiftTypeTable types) = LoadTheWarBeginsAssets();
        try
        {
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), fort.Fort, types, 6500);
            // 호스트 시작 때 1회 초기화한 공유 SP 저장소를 그대로 쓴다. 미션마다 reseed하지 않는다.
            var gameRandom = new GameRandom();
            var crtBox = new UIntBox { Value = 1u };
            var modeBox = new UIntBox { Value = 0 };
            var seed = new RawGameRandomSeed(gameRandom, crtBox, modeBox, () => 946684800);
            var shared = new RawSpStore(gameRandom, () => seed.Reseed());
            shared.Initialize();
            var session = new HostSession(new Kernel(), gameRandom, new CrtRandom(), sharedSp: shared, sharedCrtBox: crtBox);
            session.MissionPlayer.PlayerKnown = 1; session.MissionPlayer.BattleOwner = 1;
            LocalMissionZone.Install(ProductSingleBindingZone("Youpson"), session.PlayerRecords, session.MissionPlayer,
                new LocalMissionZoneGlobals(),
                new LocalMissionZoneHooks
                {
                    LoginTimeout = () => 30,
                    Seconds = () => 0.0,
                    StoreName = (_, _) => { },
                    Serve = _ => { },
                    Assertion = _ => throw new InvalidOperationException("제품 로그인 순서의 Zone 진단 미연결"),
                }, 6799u);
            Assert.Equal(1u, session.MissionPlayer.BattleOwner);
            session.StartPlayerSp(players);
            Assert.Equal(1161527296u, session.Sp.Get(1));
            session.PrepareAiSettings(players, key => script.Get(key));
            Assert.True(session.AiSettings.ContainsKey(2));
            session.StartPlayers(players, "Youpson");
            session.StartPlayerMasks(players);
            session.WriteFortOrder(1u);
            Assert.Equal(1u, session.FortCount);
            var sent = new List<(uint Owner, uint Target)>();
            session.JoinPlayer(1u, 1u, 1u, new PlayerJoinHooks { SendPlayer = (owner, target) => sent.Add((owner, target)) });
            session.WriteFortReceived(1u);
            session.MarkReady(1u);
            var broadcasts = new List<byte[]>();
            session.SendReady(0u, new PlayerReadyHooks { IsBoss = true, Broadcast = broadcasts.Add, Assertion = _ => { } });
            // 제품과 같은 번역 이름("You")을 전투 소유자 기록에 덮어쓴다.
            byte[]? languageBytes = startup.Files.TryReadAllBytes("d/xlat.english");
            var table = languageBytes is null ? null : XlatTable.FromFileBytes(languageBytes);
            byte[] encoded = ExtendedText.EncodeDisplayText(table?.Translate("You") ?? "You");
            var translated = new byte[encoded.Length + 1];
            Array.Copy(encoded, 0, translated, 0, encoded.Length);
            session.FillHandleName(translated);
            session.WriteBattleName();
            // Zone 0x08·가입 0x03·Ready 0x40 순서 그대로 기준선과 같다.
            Assert.Equal(0x4bu, DwordAt(session.PlayerRecords, 1, 0x50));
            Assert.Equal(0x2u, DwordAt(session.PlayerRecords, 1, 0x54));
            Assert.Equal(0x2u, session.ReadyMask());
            Assert.Single(broadcasts);
            Assert.Equal(new byte[] { 1, 0x6e, 8, 0, 2, 0, 0, 0 }, broadcasts[0]);
            // 7자 "Youpson"에 "You\0"을 덮어쓰면 "You\0son\0"이 된다.
            Assert.Equal("You", System.Text.Encoding.Latin1.GetString(session.PlayerRecords, 180 + 12, 3));
            Assert.Equal(0, session.PlayerRecords[180 + 15]);
            Assert.Equal("son", System.Text.Encoding.Latin1.GetString(session.PlayerRecords, 180 + 16, 3));
        }
        finally { script.Dispose(); }
    }

    /// <summary>제품 Root 등록 순서(대여→공개→owner 결속→pin→반납)가 현재 공개를 잇는다.</summary>
    [Fact]
    public void ProductRootRegistrationPublishesBindsAndRetires()
    {
        var storage = new RawSurfaceStorage();
        var session = new HostSession(new Kernel(), new GameRandom(73), new CrtRandom(), pool: storage.Pool);
        ClientGameplayGlobals game = session.Gameplay;
        Assert.False(game.Borrowed());
        Assert.Throws<InvalidOperationException>(game.RequireSession);
        // 제품 StartMission은 CreatePrepared 안에서 같은 순서로 대여·공개·결속한다.
        using RawSurfaceStorage.Use surfaceUse = storage.AcquirePrepared();
        using HostMissionRuntime runtime = HostMissionRuntime.CreatePrepared(session, storage, surfaceUse, new object());
        Assert.True(game.Borrowed());
        Assert.Same(session, game.RequireSession());
        Assert.Throws<InvalidOperationException>(() => game.AcquirePrepared());
        using (IDisposable pin = game.PinCurrentRuntime(session)) Assert.True(game.RuntimeCallActive(session));
        Assert.False(game.RuntimeCallActive(session));
        // 제품 Dispose는 Runtime 반납으로 공개·대여를 거둔다.
        runtime.Dispose();
        Assert.True(runtime.Released);
        Assert.False(game.Borrowed());
        Assert.Throws<InvalidOperationException>(game.RequireSession);
    }

    /// <summary>제품 Clock 순서(prefix 초기화→매 프레임 캡처)가 게임·벽·차이·번호를 전진시킨다.</summary>
    [Fact]
    public void ProductClockOrderCapturesFramesAfterPrefixInit()
    {
        var game = new ClientGameplayGlobals();
        var local = new ClientLocalStateGlobals(game, new());
        var clock = new ClientClockGlobals(local);
        Assert.False(clock.Initialized);
        Assert.Throws<InvalidOperationException>(() => clock.CaptureFrame(1025));
        clock.InitializeFullTime(1000);
        Assert.True(clock.Initialized);
        FrameTime first = clock.CaptureFrame(1025);
        Assert.True(Math.Abs(0.025 - first.Game) < 1e-9);
        Assert.True(Math.Abs(0.025 - first.Wall) < 1e-9);
        Assert.True(Math.Abs(0.025 - first.Delta) < 1e-9);
        Assert.Equal(1ul, first.Number);
        Assert.Equal(first.Game, clock.Frame.Value.Game);
        FrameTime second = clock.CaptureFrame(1075);
        Assert.True(Math.Abs(0.075 - second.Game) < 1e-9);
        Assert.True(Math.Abs(0.05 - second.Delta) < 1e-9);
        Assert.Equal(2ul, second.Number);
        Assert.Throws<InvalidOperationException>(() => clock.InitializeFullTime(2000));
    }
}
