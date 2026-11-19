using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 미션 시작의 실제 AI 생성 live 결선과 brain 행 대조를 검사하는 클래스다. 2026-11-19 0.639에서 R4-5 미션 시작 CreateAiBrains live 연결용으로 추가했고 0.640에서 live 초기 갱신·시작 흐름 검사를 이었다.
// 근거: cpppj src/client/RawCampaignSession.cpp 2807~2830행의 createHooks와 GameWorld.cpp 867~884행의 brain 행이다.
public sealed class HostSessionAiLiveCreateTests
{    /// <summary>thewarbegins 대본 조회 함수를 연다. 호출자가 닫는다.</summary>
    /// <returns>시작 장면과 대본이다.</returns>
    private static (HostStartupScene Startup, Netstorm.Client.Mission.MissionScript Script) OpenScript()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        return (startup, new Netstorm.Client.Mission.MissionScript(startup.Configuration, "thewarbegins"));
    }

    /// <summary>바이트 배열에서 리틀 엔디언 DWORD를 읽는다.</summary>
    /// <param name="bytes">원본 바이트 배열이다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>읽은 DWORD다.</returns>
    private static uint Get(byte[] bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset, 4));

    /// <summary>live 생성자는 실제 0xb08 본체를 세션 메모리에 두고 같은 Player 기록·brain 행에 부착하는지 확인한다.</summary>
    [Fact]
    public void LiveConstructAttachesRealBodyAndSettingsPrefix()
    {
        var (startup, script) = OpenScript();
        try
        {
            RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
            IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
            MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, table, 6500);
            var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
            session.PrepareAiSettings(players, key => script.Get(key));
            session.StartPlayers(players, "Youpson");
            var updated = new List<uint>();
            int before = session.AiBrainMemory.Allocations.Count;
            session.CreateAiBrainsLive(players, table.Types, definitions, initialUpdate: (brain, _) => updated.Add(brain));
            // 인공지능 소유자(2번)만 만든다.
            Assert.Equal(new[] { 2u }, session.AiBrains.Keys.OrderBy(key => key).ToArray());
            uint brain = session.AiBrains[2];
            Assert.NotEqual(0u, brain);
            Assert.True(session.AiBrainMemory.Contains(brain));
            Assert.Equal(RawAiBrainConstructor.BytesCount, session.AiBrainMemory.Bytes(brain).Length);
            // 같은 Player 기록에 부착한다.
            var participation = new Netstorm.Core.Simulation.PlayerParticipation(session.PlayerRecords, session.Alliances, session.HiddenPlayers, new Netstorm.Core.Simulation.PlayerParticipationState(), new Netstorm.Core.Simulation.PlayerParticipationHooks());
            Assert.Equal(brain, participation.Brain(2));
            Assert.NotEqual(0u, participation.Flags(2) & Netstorm.Core.Simulation.PlayerParticipation.BrainFlag);
            Assert.Equal(0u, participation.Brain(1));
            // 본체 앞 0x258은 설정 복사본과 같다(GameWorld raw_ai_settings 행).
            byte[] raw = session.AiBrainMemory.Bytes(brain);
            Assert.Equal(session.AiSettings[2][..0x258], raw[..0x258]);
            // 본체 owner·지갑은 생성자가 설정에서 옮긴 값이다.
            Assert.Equal(2u, Get(raw, 0x260));
            Assert.Equal(Get(session.AiSettings[2], 0x22c), Get(raw, 0x2d8));
            Assert.Equal(new[] { brain }, updated.ToArray());
            // 두 번째 호출은 기존 brain을 재사용하고 새로 할당하지 않는다.
            int afterFirst = session.AiBrainMemory.Allocations.Count;
            Assert.True(afterFirst > before);
            session.CreateAiBrainsLive(players, table.Types, definitions, initialUpdate: (_, _) => { });
            Assert.Equal(brain, session.AiBrains[2]);
            Assert.Equal(afterFirst, session.AiBrainMemory.Allocations.Count);
        }
        finally { script.Dispose(); }
    }

    /// <summary>live 색 변경은 같은 실제 색 표를 쓰고 외부 통지에 닿는지 확인한다.</summary>
    [Fact]
    public void LiveConstructWritesColorTableAndNotifies()
    {
        var (startup, script) = OpenScript();
        try
        {
            RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
            IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
            MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, table, 6500);
            var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
            session.PrepareAiSettings(players, key => script.Get(key));
            session.StartPlayers(players, "Youpson");
            uint color = Get(session.AiSettings[2], 0x250);
            var notified = new List<(uint Owner, uint Color)>();
            session.CreateAiBrainsLive(players, table.Types, definitions, initialUpdate: (_, _) => { }, changeNotify: (owner, value) => notified.Add((owner, value)));
            // 색이 있으면 같은 실제 색 표에 쓰고 통지한다.
            if (color != 0)
            {
                Assert.Equal(unchecked((int)color), session.BridgeState.OwnerColors[2]);
                Assert.Equal([(2u, color)], notified);
            }
            else
            {
                Assert.Empty(notified);
            }
        }
        finally { script.Dispose(); }
    }

    /// <summary>초기 갱신이 없으면 생성 전에 명시 거부하고 brain을 부착하지 않는지 확인한다.</summary>
    [Fact]
    public void LiveConstructWithoutInitialUpdateNeverAttaches()
    {
        var (startup, script) = OpenScript();
        try
        {
            RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
            IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
            MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, table, 6500);
            var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
            session.PrepareAiSettings(players, key => script.Get(key));
            session.StartPlayers(players, "Youpson");
            int before = session.AiBrainMemory.Allocations.Count;
            Assert.Throws<InvalidOperationException>(() => session.CreateAiBrainsLive(players, table.Types, definitions));
            var participation = new Netstorm.Core.Simulation.PlayerParticipation(session.PlayerRecords, session.Alliances, session.HiddenPlayers, new Netstorm.Core.Simulation.PlayerParticipationState(), new Netstorm.Core.Simulation.PlayerParticipationHooks());
            Assert.Equal(0u, participation.Brain(2));
            Assert.Empty(session.AiBrains);
            Assert.Equal(before, session.AiBrainMemory.Allocations.Count);
        }
        finally { script.Dispose(); }
    }

    /// <summary>live 초기 갱신은 같은 세션 풀에 기술 부모·목록을 만들고 이름을 같은 Player 기록에 쓰는지 확인한다.</summary>
    [Fact]
    public void LiveInitialUpdateEnsuresTechnologyAndWritesName()
    {
        var (startup, script) = OpenScript();
        try
        {
            RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
            IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
            MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, table, 6500);
            var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
            session.PrepareAiSettings(players, key => script.Get(key));
            session.StartPlayers(players, "Youpson");
            // 구역 설치 뒤 Zone이 PlayerKnown을 1로 두므로 같은 값을 둔다. 원문 Start 루프는 화신 생성 전에 안다.
            session.MissionPlayer.PlayerKnown = 1u;
            session.CreateAiBrainsLive(players, table.Types, definitions, typeTable: table);
            uint brain = session.AiBrains[2];
            Assert.NotEqual(0u, brain);
            byte[] raw = session.AiBrainMemory.Bytes(brain);
            Assert.Equal(RawAiBrainConstructor.BytesCount, raw.Length);
            // 본체 앞 0x258은 설정 복사본과 같다(GameWorld raw_ai_settings 행).
            Assert.Equal(session.AiSettings[2][..0x258], raw[..0x258]);
            // 초기 갱신이 본체 지갑을 바꾸지 않는다(GameWorld raw_ai_money 행).
            Assert.Equal(Get(session.AiSettings[2], 0x22c), Get(raw, 0x2d8));
            // 실제 기술 부모를 같은 Player 기록 +0x9c에 둔다.
            uint parent = Get(session.PlayerRecords, 2 * 180 + 0x9c);
            Assert.NotEqual(0u, parent);
            // 시작 기술 3종이 같은 부모 체인에 들어간다.
            var contained = new HashSet<uint>();
            var cursor = new RawContainedFinder(session.Pool, new ContainedFinderState());
            // 체인 head에서 순서대로 훑는 반복문이다.
            for (int sid = cursor.Begin((int)parent); sid != 0; sid = cursor.Next())
            {
                ReadOnlySpan<byte> piece = session.Pool.Slot(sid);
                contained.Add(BinaryPrimitives.ReadUInt16LittleEndian(piece.Slice(18, 2)));
            }

            Assert.Contains((uint)table.Find("sunWalker"), contained);
            Assert.Contains((uint)table.Find("windBattery"), contained);
            Assert.Contains((uint)table.Find("sunArcher"), contained);
            // 미션 이름이 있으면 기본 이름 조회 없이 같은 Player 기록 +0xc에 쓴다.
            int end = Array.IndexOf(session.PlayerRecords, (byte)0, 2 * 180 + 12);
            Assert.Equal("The Jailer", System.Text.Encoding.Latin1.GetString(session.PlayerRecords, 2 * 180 + 12, end - (2 * 180 + 12)));
        }
        finally { script.Dispose(); }
    }

    /// <summary>미션 시작 생성은 사제가 없으면 brain을 만들지 않고 조용히 넘어가는지 확인한다.</summary>
    [Fact]
    public void CreateMissionAiBrainsSkipsWithoutPriests()
    {
        var (startup, script) = OpenScript();
        try
        {
            RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
            IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
            MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, table, 6500);
            var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
            session.PrepareAiSettings(players, key => script.Get(key));
            session.StartPlayers(players, "Youpson");
            int before = session.AiBrainMemory.Allocations.Count;
            // 적재 전에는 사제가 없으므로 언어 조회에도 닿지 않아야 한다.
            session.CreateMissionAiBrains(players, table, definitions, () => throw new InvalidOperationException("사제 없는 미션의 기본 이름 조회"));
            Assert.Empty(session.AiBrains);
            Assert.Equal(before, session.AiBrainMemory.Allocations.Count);
        }
        finally { script.Dispose(); }
    }
}
