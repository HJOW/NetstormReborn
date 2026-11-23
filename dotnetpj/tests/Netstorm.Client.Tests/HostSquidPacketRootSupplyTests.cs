// Root 공급값으로 고정 26F 중 세션 독립 수신이 실제 수신을 마치는지 검사한다.
// cpppj/src/client/RawCampaignSession.cpp 5325~5562행의 ReceiveFixedGameplayPacket 중
// Root 공급만으로 동작하는 항목(텔·카메라·검증·공통·위치·공유 정기)을 이식한다.
// 0.903에서 RU-2 나머지 Bind 결선용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>같은 실제 설정·화면·로그·입력 owner를 빌리는 Root 수신 결선의 계약이다.</summary>
public sealed class HostSquidPacketRootSupplyTests
{
    /// <summary>같은 실제 Surface와 미션 공장/사제 공간을 수신 전에 구성한다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface) SquidPacketScene(RawSurfaceTransmissionDevices? transmission = null)
    {
        var game = new ClientGameplayGlobals();
        game.Authority.Server = true;
        var local = new ClientLocalStateGlobals(game, new());
        var surface = new ClientSurfaceGlobals(game, new(local), new Kernel(), () => 0, transmission);
        var (session, types) = PriestScene(surface, initializeSurfaceAssets: transmission is not null);
        session.BuildPriestWorldDevices();
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) => throw new InvalidOperationException($"객체 패킷 진단:{text} {file}:{line:x}"));
        return (session, types, surface);
    }

    /// <summary>실자산과 시작 때 초기화한 SP·장기 목록을 같은 세션에 연결한다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) PriestScene(ClientSurfaceGlobals sharedSurface, bool initializeSurfaceAssets = false)
    {
        string? data = Netstorm.Assets.GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var random = new GameRandom(1);
        var globals = new ClientSpGlobals(random, new UIntBox { Value = 1 }, new UIntBox(), () => 1760000000);
        globals.Store.Initialize();
        var session = new HostSession(sharedSurface.Kernel, random, new CrtRandom(), sharedSp: globals.Store,
            sharedGameplay: sharedSurface.Gameplay, pool: sharedSurface.Storage.Pool);
        RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, session.BattleOptions);
        session.MissionPlayer.PlayerKnown = 1;
        session.MissionPlayer.BattleOwner = 1;
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        var shapes = TerrainDisplayShapes.Build(definitions, new Netstorm.Assets.ShapeDatabase(startup.Files.ReadAllBytes("d/_shapes.shp")));
        IReadOnlyList<OriginalTypeRecord> types = table.Types;
        if (initializeSurfaceAssets)
        {
            sharedSurface.Storage.InitializeAssetTables(types, session.WorldFrames, shapes);
            types = sharedSurface.Storage.Types;
        }

        session.BuildTerrainPopDevices(types, shapes, hash: sharedSurface.Storage.Hash, spots: sharedSurface.Storage.Spots);
        session.BuildFortObjectDevices(types, session.WorldFrames);
        session.BuildWorldGraphDevices(types);
        return (session, types);
    }

    /// <summary>같은 실제 설정 owner의 언어·치환으로 영어 이름 장치를 만든다.</summary>
    private static RawSquidName RootNames(HostSession session, IReadOnlyList<OriginalTypeRecord> types, string languageName, ConfigInterface config)
    {
        return new RawSquidName(session.Pool, types, new()
        {
            Language = () => ClientSurfaceGlobals.RootLanguageNumber(languageName),
            Substitute = bytes => ExtendedText.EncodeDisplayText(config.SubstituteLocal(ExtendedText.DecodeDisplayText(bytes.Span))),
        });
    }

    /// <summary>Root 공급값의 언어 번호가 실제 수신의 번역 생략 관문과 같다. 0.903에서 추가됐다.</summary>
    [Fact]
    public void RootLanguageMatchesEnglishBypassGate()
    {
        Assert.Equal(0u, ClientSurfaceGlobals.RootLanguageNumber("english"));
        var (session, types, surface) = SquidPacketScene();
        var config = new ConfigInterface(_ => null);
        config.Configuration.Assign("width=800\n");
        var names = RootNames(session, types, "english", config);
        session.BindSquidPacketDevices(surface.Storage, names, new());
        int sid = session.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        session.Pool.AllocatedBytes(sid).Clear();
        session.Pool.AllocatedBytes(sid)[10] = 158;
        byte[] description = ExtendedText.EncodeDisplayText(types[158].Description);
        byte[] based = names.BaseName(sid);
        Assert.Equal(description.TakeWhile(b => b != 0).ToArray(), based.TakeWhile(b => b != 0).ToArray());
        session.ReleasePlayerProcesses();
    }

    /// <summary>같은 실제 Screen의 크기로 카메라 중심 수신이 실제 화면을 읽는다. 0.903에서 추가됐다.</summary>
    [Fact]
    public void RootScreenSizeSuppliesCameraCenter()
    {
        var (session, types, surface) = SquidPacketScene();
        var config = new ConfigInterface(_ => null);
        config.Configuration.Assign("width=800\n");
        var names = RootNames(session, types, "english", config);
        var screen = new Screen(1024, 768);
        session.BindSquidPacketDevices(surface.Storage, names, new()
        {
            ScreenSize = () => ClientSurfaceGlobals.RootScreenSize(screen),
        });
        var packet = new byte[12];
        BinaryPrimitives.WriteSingleLittleEndian(packet.AsSpan(4, 4), 100);
        BinaryPrimitives.WriteSingleLittleEndian(packet.AsSpan(8, 4), 200);
        session.ReceiveFixedGameplayPacket(0x4333f0, packet);
        Assert.Equal((1024, 768), ClientSurfaceGlobals.RootScreenSize(screen));
        session.ReleasePlayerProcesses();
    }

    /// <summary>같은 실제 로그·Tell owner로 원격 Tell과 검증 수신이 실제 수신을 마친다. 0.903에서 추가됐다.</summary>
    [Fact]
    public void RootTellAndLogCompleteRemoteTellAndVerification()
    {
        var (session, types, surface) = SquidPacketScene();
        var config = new ConfigInterface(_ => null);
        config.Configuration.Assign("width=800\n");
        var names = RootNames(session, types, "english", config);
        byte[]? told = null;
        int logs = 0;
        session.BindSquidPacketDevices(surface.Storage, names, new()
        {
            Tell = bytes => told = bytes.ToArray(),
            Log = (level, text) => logs++,
        });
        byte[] packet = [0x30, 0xcf, 1, 0, 0, 0, 0, 0, (byte)'h', (byte)'i', 0];
        session.ReceiveFixedGameplayPacket(0x4cf980, packet);
        Assert.NotNull(told);
        Assert.Equal([(byte)'h', (byte)'i'], told);
        int sid = session.BuildPlayerWorldHooks().Create!(158, 0);
        var first = new byte[6];
        BinaryPrimitives.WriteUInt16LittleEndian(first.AsSpan(4, 2), (ushort)sid);
        using (HostMissionRuntime owner = HostMissionRuntime.Create(session, surface.Storage))
        {
            using (session.Gameplay.PinCurrentRuntime(session))
                session.ReceiveFixedGameplayPacket(0x4326e0, first);
            var free = new byte[7];
            first.CopyTo(free, 0);
            using (session.Gameplay.PinCurrentRuntime(session))
                session.ReceiveFixedGameplayPacket(0x432710, free);
        }

        Assert.True(logs > 0);
        session.ReleasePlayerProcesses();
    }

    /// <summary>공유 정기 수신이 Root 공급 결선 뒤에도 실제 현재 TYPE로 동작한다. 0.903에서 추가됐다.</summary>
    [Fact]
    public void RootSupplyKeepsSharedRegularReception()
    {
        var (session, types, surface) = SquidPacketScene();
        var config = new ConfigInterface(_ => null);
        config.Configuration.Assign("width=800\n");
        var names = RootNames(session, types, "english", config);
        session.BindSquidPacketDevices(surface.Storage, names, new());
        int parent = session.BuildPlayerWorldHooks().Create!(152, 0);
        session.GameAux.SharedRegularProcessType = 60;
        session.ProcessTime.Value = 123;
        var packet = new byte[28];
        packet[1] = 0x31;
        packet[4] = 0xff;
        packet[5] = 0xff;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), (ushort)parent);
        BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(8, 8), 1.25);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(16, 4), 12345);
        BinaryPrimitives.WriteSingleLittleEndian(packet.AsSpan(20, 4), 2.5f);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(24, 4), 0x11223344);
        session.ReceiveFixedGameplayPacket(0x497030, packet);
        RegularProcess process = Assert.IsType<RegularProcess>(session.PlayerProcesses.FindEvent(parent, 12345));
        Assert.True(process.Shared());
        session.ReleasePlayerProcesses();
    }

    /// <summary>큰 본체·짧은 패킷은 일반 성공으로 꾸미지 않고 명시 실패한다. 0.903에서 추가됐다.</summary>
    [Fact]
    public void MissingLargeBodiesAndShortPacketsFailExplicitlyInsteadOfNoOp()
    {
        var (session, types, surface) = SquidPacketScene();
        var config = new ConfigInterface(_ => null);
        config.Configuration.Assign("width=800\n");
        var names = RootNames(session, types, "english", config);
        session.BindSquidPacketDevices(surface.Storage, names, new());
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e410, new byte[8]));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x46a0c0, new byte[8]));
        session.ReleasePlayerProcesses();
    }
}
