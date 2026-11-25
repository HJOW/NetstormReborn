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

    /// <summary>게임 실행 없이 AI 생성과 반납을 검사할 세션과 빈 타입 표를 만든다. 0.641에서 추가했다.</summary>
    private static (HostSession Session, MissionPlayers Players, OriginalTypeRecord[] Types, RiftTypeDefinition[] Definitions) NewSyntheticScene()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        var players = new MissionPlayers();
        players.Players[2].Active = true;
        players.Players[2].Computer = true;
        session.AiSettings[2] = new byte[RawMissionAiSettings.Size];
        return (session, players, Enumerable.Range(0, HostSession.WorldFrameCount).Select(_ => new OriginalTypeRecord()).ToArray(),
            Enumerable.Range(0, 116).Select(_ => new RiftTypeDefinition()).ToArray());
    }

    /// <summary>해제 후 다시 생성할 수 있고 같은 세션에서 반납을 확정하면 다시 붙일 수 없는지 검사한다. 0.641에서 추가했다.</summary>
    [Fact]
    public void ResetAndReturnShareTheSessionThatCreatedTheBrain()
    {
        var (session, players, types, definitions) = NewSyntheticScene();
        session.CreateAiBrainsLive(players, types, definitions, initialUpdate: (_, _) => { });
        uint brain = session.AiBrains[2];
        var allocated = session.AiBrainMemory.Allocations.Select(entry => entry.Token).Order().ToArray();

        _ = session.ResetAiForState(2);

        Assert.Equal(allocated, session.AiBrainMemory.Frees.Order());
        Assert.Equal(brain, session.AiBrainMemory.Frees[^1]);
        Assert.Empty(session.AiBrains);
        Assert.Equal(0u, Get(session.PlayerRecords, 2 * PlayerParticipation.PlayerStride + PlayerParticipation.BrainOffset));

        session.CreateAiBrainsLive(players, types, definitions, initialUpdate: (_, _) => { });
        Assert.NotEqual(brain, session.AiBrains[2]);
        PlayerParticipation.BrainResetPass pass = session.ResetAiForState(2);
        var other = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Throws<ArgumentException>(() => other.ReturnAiForState(pass));
        session.ReturnAiForState(pass);
        Assert.Throws<ArgumentException>(() => session.ReturnAiForState(pass));
        Assert.Throws<InvalidOperationException>(() =>
            session.CreateAiBrainsLive(players, types, definitions, initialUpdate: (_, _) => { }));
    }

    /// <summary>초기 갱신 중 실패하여 목록에 아직 등록되지 않은 AI도 미션 종료 시 해제하는지 검사한다. 0.641에서 추가했다.</summary>
    [Fact]
    public void ReleaseCleansUpAnAttachedBrainAfterInitialUpdateFails()
    {
        var (session, players, types, definitions) = NewSyntheticScene();
        session.CreateAiBrainsLive(players, types, definitions, initialUpdate: (_, _) => { });
        players.Players[3].Active = true;
        players.Players[3].Computer = true;
        session.AiSettings[3] = new byte[RawMissionAiSettings.Size];
        Assert.Throws<InvalidOperationException>(() => session.CreateAiBrainsLive(players, types, definitions,
            initialUpdate: (_, _) => throw new InvalidOperationException("테스트용 초기 갱신 실패")));
        Assert.False(session.AiBrains.ContainsKey(3));
        Assert.NotEqual(0u, Get(session.PlayerRecords, 3 * PlayerParticipation.PlayerStride + PlayerParticipation.BrainOffset));

        session.ReleaseMissionAiBrains();
        session.ReleaseMissionAiBrains();

        Assert.Empty(session.AiBrains);
        Assert.Equal(0u, Get(session.PlayerRecords, 3 * PlayerParticipation.PlayerStride + PlayerParticipation.BrainOffset));
        Assert.Equal(session.AiBrainMemory.Allocations.Select(entry => entry.Token).Order(), session.AiBrainMemory.Frees.Order());
    }

    /// <summary>값을 리틀 엔디언으로 쓴다. NoTemple 설정·첫 지역 기록용으로 추가했다.</summary>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>신전 삭제 결선을 검사할 실제 장면을 만든다. AI 런타임·화신·파괴 장치를 모두 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표·자산 정의다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, IReadOnlyList<RiftTypeDefinition> Definitions) NewTempleScene()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        session.BuildIslandPostPopDevices(table.Types, session.WorldFrames);
        session.BuildPlayerWorldHooks();
        session.BuildTerrainBinding(table.Types, session.TerrainDiagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        session.BuildRegionOwnershipDevices(table.Types, () => 1u);
        return (session, table.Types, definitions);
    }

    /// <summary>NoTemple 설정의 활성 전산 소유자를 만들고 첫 지역을 기록한다.</summary>
    /// <param name="session">설정·기록을 쓸 세션이다.</param>
    /// <param name="region">소유자의 첫 지역 번호다.</param>
    /// <returns>소유자 2번만 활성 전산으로 둔 미션 목록이다.</returns>
    private static MissionPlayers NewNoTemplePlayers(HostSession session, uint region)
    {
        var players = new MissionPlayers();
        players.Players[2].Active = true;
        players.Players[2].Computer = true;
        byte[] settings = new byte[RawMissionAiSettings.Size];
        Put(settings, 0x244, 1u);
        session.AiSettings[2] = settings;
        Put(session.PlayerRecords, 2 * 180 + 0x78, region);
        return players;
    }

    /// <summary>NoTemple 소유자는 섬 신전을 실제 파괴로 지우고 brain을 부착하지 않는다.</summary>
    /// <remarks>2026-11-25 1.032에서 R4-5c 신전 삭제용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 2816~2818행의 destroyTemple과 RawAiCreate의 NoTemple 분기다. 파괴 플래그는 원본 0x400000이다.</remarks>
    [Fact]
    public void NoTempleDestroysSessionTempleAndAttachesNoBrain()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeDefinition> definitions) = NewTempleScene();
        MissionPlayers players = NewNoTemplePlayers(session, 3);
        // 섬 기록 0~3을 두어 3번에 신전을 둔다.
        for (int i = 0; i < 4; i++)
        {
            session.AiIslands.Add(1, unchecked((byte)i));
        }

        int temple = new RawSquidFactory(session.Pool, types).CreateContained(2u);
        session.AiIslands.Temple(3, unchecked((uint)temple));
        var calls = new List<(uint Sid, uint Flags)>();
        Action<uint, uint> destroy = session.BuildAiTempleDestroy();
        AiCreateHooks hooks = session.BuildLiveAiCreateHooks(types, definitions,
            initialUpdate: (_, _) => { },
            destroyTemple: (sid, flags) => { calls.Add((sid, flags)); destroy(sid, flags); });
        session.CreateAiBrains(players, hooks);
        Assert.Equal([(unchecked((uint)temple), 0x400000u)], calls);
        // NoTemple 분기는 0을 기록하고 Player 기록·brain 메모리에는 부착하지 않는다.
        Assert.Equal(0u, session.AiBrains[2]);
        Assert.Equal(0u, Get(session.PlayerRecords, 2 * 180 + PlayerParticipation.BrainOffset));
        Assert.Throws<InvalidOperationException>(() => session.Pool.AllocatedBytes(temple));
    }

    /// <summary>섬 신전이 없으면 NoTemple 소유자는 파괴 없이 brain 없이 넘어간다.</summary>
    /// <remarks>2026-11-25 1.032에서 R4-5c 신전 삭제용으로 추가했다. 원문 NoTemple 분기의 temple == 0 경로와 같다.</remarks>
    [Fact]
    public void NoTempleWithoutIslandTempleSkipsDestroyAndAttachesNoBrain()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeDefinition> definitions) = NewTempleScene();
        MissionPlayers players = NewNoTemplePlayers(session, 3);
        // 섬 기록 0~3을 두되 신전은 두지 않는다.
        for (int i = 0; i < 4; i++)
        {
            session.AiIslands.Add(1, unchecked((byte)i));
        }

        int decoy = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(decoy).Clear();
        var calls = new List<(uint Sid, uint Flags)>();
        AiCreateHooks hooks = session.BuildLiveAiCreateHooks(types, definitions,
            initialUpdate: (_, _) => { },
            destroyTemple: (sid, flags) => calls.Add((sid, flags)));
        session.CreateAiBrains(players, hooks);
        Assert.Empty(calls);
        Assert.Equal(0u, session.AiBrains[2]);
        Assert.Equal(0u, Get(session.PlayerRecords, 2 * 180 + PlayerParticipation.BrainOffset));
        Assert.Null(Record.Exception(() => session.Pool.AllocatedBytes(decoy)));
    }

    /// <summary>신전 제거 효과는 풀 범위를 벗어난 SID를 원문 순서대로 거부한다.</summary>
    /// <remarks>2026-11-25 1.032에서 R4-5c 신전 삭제용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 2817행의 범위 검사다.</remarks>
    [Theory]
    [InlineData(70000u)]
    [InlineData(70001u)]
    public void TempleDestroyRejectsOutOfRangeSid(uint temple)
    {
        (HostSession session, _, _) = NewTempleScene();
        Action<uint, uint> destroy = session.BuildAiTempleDestroy();
        if (temple < (uint)session.Pool.Capacity)
        {
            temple = (uint)session.Pool.Capacity;
        }

        Assert.Throws<ArgumentOutOfRangeException>(() => destroy(temple, 0x400000u));
    }

    /// <summary>화신 장치가 없으면 신전 제거 효과를 만들 수 없다.</summary>
    /// <remarks>2026-11-25 1.032에서 R4-5c 신전 삭제용으로 추가했다. 시작 흐름의 장치 순서를 고정한다.</remarks>
    [Fact]
    public void TempleDestroyWithoutProcessesThrows()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.BuildAiTempleDestroy());
    }

    /// <summary>미션 설정 길이가 맞지 않으면 AI 메모리 할당 전에 거부하는지 검사한다. 0.641에서 추가했다.</summary>
    [Fact]
    public void LiveCreateRejectsTruncatedSettingsBeforeAllocation()
    {
        var (session, players, types, definitions) = NewSyntheticScene();
        session.AiSettings[2] = new byte[RawMissionAiSettings.Size - 1];

        Assert.Throws<ArgumentException>(() => session.CreateAiBrainsLive(players, types, definitions, initialUpdate: (_, _) => { }));

        Assert.Empty(session.AiBrainMemory.Allocations);
        Assert.Empty(session.AiBrains);
    }

    /// <summary>생성 경로의 난이도 배율이 적 AI에만 적용되며 입력 설정은 보존하는지 검사한다. 0.641에서 추가했다.</summary>
    [Theory]
    [InlineData(AiDifficultyLevel.Easy, false, false, 32f)]
    [InlineData(AiDifficultyLevel.Hard, false, false, 4f)]
    [InlineData(AiDifficultyLevel.Normal, false, false, 8f)]
    [InlineData(AiDifficultyLevel.Easy, true, false, 8f)]
    [InlineData(AiDifficultyLevel.Hard, false, true, 8f)]
    public void MissionDifficultyUsesBothAllianceInputs(AiDifficultyLevel level, bool declared, bool recorded, float expectedCycle)
    {
        var (session, players, types, definitions) = NewSyntheticScene();
        session.MissionPlayer.BattleOwner = 3;
        int localMask = 3 * PlayerParticipation.PlayerStride + PlayerParticipation.AllyMaskOffset;
        byte[] settings = session.AiSettings[2];
        BinaryPrimitives.WriteUInt32LittleEndian(settings.AsSpan(AiAllianceSide.SettingsAllyMaskOffset), declared ? 1u << 3 : 0u);
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(localMask), recorded ? 1u << 2 : 0u);
        BinaryPrimitives.WriteSingleLittleEndian(settings.AsSpan(0x220), 8f);
        byte[] before = settings.ToArray();
        float updateCycle = 0;

        session.CreateAiBrainsLive(players, types, definitions,
            initialUpdate: (_, active) => updateCycle = BinaryPrimitives.ReadSingleLittleEndian(active.Span[0x220..]),
            difficulty: (owner, active) => session.ResolveMissionAiDifficulty(level, owner, active));

        ReadOnlySpan<byte> brain = session.AiBrainMemory.Bytes(session.AiBrains[2]);
        Assert.Equal(expectedCycle, BinaryPrimitives.ReadSingleLittleEndian(brain[0x220..]));
        Assert.Equal(expectedCycle, updateCycle);
        Assert.Equal(before, settings);
    }
}
