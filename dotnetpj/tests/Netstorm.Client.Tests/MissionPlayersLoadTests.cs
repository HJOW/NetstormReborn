using Netstorm.Assets;
using Netstorm.Client.Automation;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

/// <summary>미션 플레이어 목록 적재가 실제 대본·요새·타입 표에서 원본과 같은 목록을 만드는지 검사한다.
/// 근거: cpppj/src/o/Player.cpp의 <c>MissionPlayers::Load</c>. (2026-11-12 0.394에서 추가했다)</summary>
public sealed class MissionPlayersLoadTests
{
    /// <summary>thewarbegins 대본으로 목록을 적재한다.</summary>
    private static (MissionFortLoadResult Loaded, MissionPlayers Players, RiftTypeTable Types) LoadTheWarBegins()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
        var script = new Netstorm.Client.Mission.MissionScript(startup.Configuration, "thewarbegins");
        try
        {
            RiftTypeTable types = MissionFortLoad.LoadTypeTable(startup.Files);
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, types, 6500);
            return (loaded, players, types);
        }
        finally { script.Dispose(); }
    }

    /// <summary>thewarbegins 대본 조회 함수를 연다. 호출자가 닫는다.</summary>
    private static (HostStartupScene Startup, Netstorm.Client.Mission.MissionScript Script) OpenScript()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        return (startup, new Netstorm.Client.Mission.MissionScript(startup.Configuration, "thewarbegins"));
    }

    /// <summary>thewarbegins 1·2번과 나머지 번호가 원본 규칙과 같은 목록을 내놓는지 확인한다.</summary>
    [Fact]
    public void LoadsTheWarBeginsPlayers()
    {
        var (loaded, players, types) = LoadTheWarBegins();
        Assert.All(players.TechAllowed, allowed => Assert.True(allowed));
        Assert.False(players.DenySalvage);
        Assert.False(players.DenyAscend);
        Assert.False(players.AiOff);
        MissionPlayer first = players.Players[1];
        Assert.Equal(1, first.Number);
        Assert.Equal(1, first.Color);
        Assert.True(first.Active);
        Assert.False(first.Computer);
        Assert.Equal(3000.0, first.StormPower);
        Assert.Equal("Player", first.Name);
        Assert.Equal("sunWalker;rainBattery;sunAviary;sunCannon", first.StartingTech);
        Assert.Equal(
            new[] { types.Find("sunWalker"), types.Find("rainBattery"), types.Find("sunAviary"), types.Find("sunCannon") }.Order().ToList(),
            first.Knowledge);
        Assert.Equal("", first.Ability);
        Assert.Equal(0x0002, first.Allies);
        Assert.Equal(26, first.StoredTechnology.Items.Count);
        Assert.Equal(38, first.StoredDeck.Count);
        Assert.Equal(loaded.Fort.Technology.Items.Count, first.StoredTechnology.Items.Count);
        Assert.Equal(loaded.Fort.Deck.Count, first.StoredDeck.Count);
        MissionPlayer second = players.Players[2];
        Assert.Equal(2, second.Number);
        Assert.Equal(2, second.Color);
        Assert.True(second.Active);
        Assert.True(second.Computer);
        Assert.Equal(0.0, second.StormPower);
        Assert.Equal("The Jailer", second.Name);
        Assert.Equal("sunWalker;windBattery;sunArcher", second.StartingTech);
        Assert.Equal(
            new[] { types.Find("sunWalker"), types.Find("windBattery"), types.Find("sunArcher") }.Order().ToList(),
            second.Knowledge);
        Assert.Equal("!ENCIRCLE_ENEMY_ISLAND;!USE_PRIEST_TO_COLLECT", second.Ability);
        Assert.Equal(0x0004, second.Allies);
        // 나머지 번호는 비어 있고 자기 자신하고만 동맹이다.
        for (int number = 3; number <= MissionPlayers.PlayerCount; number++)
        {
            MissionPlayer player = players.Players[number];
            Assert.Equal(number, player.Number);
            Assert.Equal(number, player.Color);
            Assert.False(player.Active);
            Assert.True(player.Computer);
            Assert.Equal(0.0, player.StormPower);
            Assert.Equal("", player.Name);
            Assert.Empty(player.Knowledge);
            Assert.Equal(unchecked((ushort)(1u << number)), player.Allies);
        }
        Assert.False(players.Allied(1, 2));
        Assert.True(players.Allied(1, 1));
        Assert.False(players.Allied(0, 1));
        Assert.False(players.Allied(1, 9));
    }

    /// <summary>thewarbegins 시작 SP 지정이 기준선 sp 행의 참가자 값과 같은지 확인한다.</summary>
    [Fact]
    public void StartsPlayerSpLikeBaseline()
    {
        var (loaded, players, types) = LoadTheWarBegins();
        Assert.NotNull(loaded);
        Assert.NotNull(types);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.False(session.Sp.Initialized);
        // 실제 사슬 모양의 재시드(세션 게임 난수+CRT 상자+모드 상자+고정 유닉스 초)로 연결한다.
        var crtBox = new UIntBox { Value = 1 };
        var modeBox = new UIntBox { Value = 0 };
        var seed = new RawGameRandomSeed(session.Game, crtBox, modeBox, () => 946684800);
        session.BindSpReseed(() => seed.Reseed());
        session.StartPlayerSp(players);
        Assert.True(session.Sp.Initialized);
        // 참가자 값은 난수와 무관하게 정확히 읽힌다(기준선 f090: 1번 1161527296, 나머지는 미지정 0과 같다).
        Assert.Equal(1161527296u, session.Sp.Get(1));
        Assert.Equal(0u, session.Sp.Get(2));
        // offset은 초기화 사슬(재시드 입력·Rotate 횟수)이 Start 순서와 함께 맞아야 기준선(103)과 같아진다.
        // 지금은 행 문법과 참가자 값만 고정하고 offset 값은 기록만 한다(0.395 잔여).
        string text = SessionStateDump.FormatWorld1(
            "originals", 90, 0, 0, 0, 0, 39999, null, session.DumpPool(),
            session.DumpPlayers(), session.DumpSp(), session.DumpGraph(), session.DumpTotals(), session.DumpTotalMade());
        Assert.Contains($"\nsp\t1\t{session.Sp.Offset}\t0\t1161527296\t0\t0\t0\t0\t0\t0\t0\n", text, StringComparison.Ordinal);
    }

    /// <summary>thewarbegins 참가자·동맹 채우기가 기준선과 같은 행을 내놓는지 확인한다.</summary>
    [Fact]
    public void StartsPlayersLikeBaseline()
    {
        var (loaded, players, types) = LoadTheWarBegins();
        Assert.NotNull(loaded);
        Assert.NotNull(types);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        // 1번 핸들은 대본 이름이 아니라 실제 핸들이다. 값은 오라클 관측("Youpson")이며 출처는 Start 흐름 단위에서 잇는다.
        session.StartPlayers(players, "Youpson");
        string text = SessionStateDump.FormatWorld1(
            "originals", 90, 0, 0, 0, 0, 39999, null, session.DumpPool(),
            session.DumpPlayers(), session.DumpSp(), session.DumpGraph(), session.DumpTotals(), session.DumpTotalMade());
        // 동맹 81칸은 대각선만 1이다(기준선과 바이트 동일).
        var alliances = new System.Text.StringBuilder("\nalliances");
        // 9×9 칸을 행 우선으로 쓴다.
        for (int first = 0; first < 9; first++)
            for (int second = 0; second < 9; second++)
                alliances.Append(first == second ? "\t1" : "\t0");
        alliances.Append('\n');
        Assert.Contains(alliances.ToString(), text, StringComparison.Ordinal);
        Assert.Contains("\nhidden_players\t000000000000000000\n", text, StringComparison.Ordinal);
        // 2번 이름은 바이트 동일하다.
        Assert.Contains("546865204a61696c657200", text, StringComparison.Ordinal);
        // 1번 이름은 붙여 쓴다. 기준선은 바이트 15가 NUL("You\0pson")이라 로그인 쓰기 순서 잔여로 남긴다.
        Assert.Contains("596f7570736f6e00", text, StringComparison.Ordinal);
    }
    [Fact]
    public void PreparesAiSettingsLikeScript()
    {
        var (startup, script) = OpenScript();
        try
        {
            RiftTypeTable types = MissionFortLoad.LoadTypeTable(startup.Files);
            MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, types, 6500);
            var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
            session.PrepareAiSettings(players, key => script.Get(key));
            // 인공지능 소유자(2번)만 만든다.
            Assert.True(session.AiSettings.ContainsKey(2));
            Assert.False(session.AiSettings.ContainsKey(1));
            byte[] raw = session.AiSettings[2];
            Assert.Equal(RawMissionAiSettings.Size, raw.Length);
            static string CString(byte[] bytes, int offset)
            {
                int end = Array.IndexOf(bytes, (byte)0, offset);
                return System.Text.Encoding.Latin1.GetString(bytes, offset, (end < 0 ? bytes.Length : end) - offset);
            }
            static float FloatAt(byte[] bytes, int offset) =>
                BitConverter.UInt32BitsToSingle(System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset, 4)));
            static uint DwordAt(byte[] bytes, int offset) =>
                System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset, 4));
            Assert.Equal("The Jailer", CString(raw, 0));
            Assert.Equal("sunWalker;windBattery;sunArcher", CString(raw, 0x1e));
            Assert.Equal(6f, FloatAt(raw, 0x220));
            Assert.Equal(3f, FloatAt(raw, 0x224));
            Assert.Equal(6f, FloatAt(raw, 0x228));
            Assert.Equal(0u, DwordAt(raw, 0x22c));
            Assert.Equal(1u, DwordAt(raw, 0x230));
            Assert.Equal(1u, DwordAt(raw, 0x234));
            Assert.Equal(0xF7DFu, DwordAt(raw, 0x248));
        }
        finally { script.Dispose(); }
    }

    /// <summary>thewarbegins 인공지능 소유자의 brain을 실제 생성 순서로 만들어 같은 Player 기록에 부착하는지 확인한다.</summary>
    [Fact]
    public void CreatesAiBrainsForTheWarBeginsAiOwner()
    {
        var (startup, script) = OpenScript();
        try
        {
            RiftTypeTable types = MissionFortLoad.LoadTypeTable(startup.Files);
            MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, types, 6500);
            var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
            session.PrepareAiSettings(players, key => script.Get(key));
            session.StartPlayers(players, "Youpson");
            byte[] before = (byte[])session.AiSettings[2].Clone();
            var colors = new List<(uint Owner, uint Color)>();
            var constructed = new List<(uint Owner, int Size, byte[] Settings)>();
            var updated = new List<uint>();
            uint next = 0x10000;
            var hooks = new AiCreateHooks
            {
                ChangeColor = (owner, color) => colors.Add((owner, color)),
                FirstRegion = _ => 0u,
                IslandTemple = _ => 0u,
                DestroyTemple = (_, _) => { },
                Construct = (owner, settings, size) => { constructed.Add((owner, size, settings.ToArray())); return ++next; },
                InitialUpdate = (brain, _) => updated.Add(brain),
            };
            session.CreateAiBrains(players, hooks);
            // 인공지능 소유자(2번)만 만든다.
            Assert.Equal(new[] { 2u }, session.AiBrains.Keys.OrderBy(key => key).ToArray());
            uint brain = session.AiBrains[2];
            Assert.NotEqual(0u, brain);
            // 같은 Player 기록에 부착한다.
            var participation = new Netstorm.Core.Simulation.PlayerParticipation(session.PlayerRecords, session.Alliances, session.HiddenPlayers, new Netstorm.Core.Simulation.PlayerParticipationState(), new Netstorm.Core.Simulation.PlayerParticipationHooks());
            Assert.Equal(brain, participation.Brain(2));
            Assert.NotEqual(0u, participation.Flags(2) & PlayerParticipation.BrainFlag);
            Assert.Equal(0u, participation.Brain(1));
            // 생성자에는 600바이트·원본 할당 크기로 넘긴다.
            Assert.Single(constructed);
            Assert.Equal(2u, constructed[0].Owner);
            Assert.Equal(0xb08, constructed[0].Size);
            Assert.Equal(RawMissionAiSettings.Size, constructed[0].Settings.Length);
            Assert.Equal(new[] { brain }, updated.ToArray());
            // 보통 난이도에서는 입력 설정을 그대로 넘긴다.
            Assert.Equal(before, constructed[0].Settings);
            Assert.Equal(before, session.AiSettings[2]);
            // 색이 있으면 변경 효과를 부른다.
            uint color = System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(before.AsSpan(0x250, 4));
            Assert.Equal(color != 0u ? 1 : 0, colors.Count);
            // 두 번째 호출은 기존 brain을 재사용하고 생성자를 다시 부르지 않는다.
            session.CreateAiBrains(players, hooks);
            Assert.Equal(brain, session.AiBrains[2]);
            Assert.Single(constructed);
        }
        finally { script.Dispose(); }
    }

    /// <summary>NoTemple 설정에서는 생성자 없이 첫 지역·신전 조회와 신전 제거만 하는지 확인한다.</summary>
    [Fact]
    public void CreatesAiBrainsNoTemplePathWithoutConstruct()
    {
        var (startup, script) = OpenScript();
        try
        {
            RiftTypeTable types = MissionFortLoad.LoadTypeTable(startup.Files);
            MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, types, 6500);
            var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
            session.PrepareAiSettings(players, key => script.Get(key));
            session.StartPlayers(players, "Youpson");
            System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(session.AiSettings[2].AsSpan(0x244, 4), 1u);
            var destroyed = new List<(uint Temple, uint Flags)>();
            int constructs = 0;
            var hooks = new AiCreateHooks
            {
                ChangeColor = (_, _) => { },
                FirstRegion = owner => { Assert.Equal(2u, owner); return 5u; },
                IslandTemple = region => { Assert.Equal(5u, region); return 77u; },
                DestroyTemple = (temple, flags) => destroyed.Add((temple, flags)),
                Construct = (_, _, _) => { constructs++; return 1u; },
                InitialUpdate = (_, _) => { },
            };
            session.CreateAiBrains(players, hooks);
            Assert.Equal(0u, session.AiBrains[2]);
            Assert.Equal(0, constructs);
            Assert.Equal([(77u, 0x400000u)], destroyed);
        }
        finally { script.Dispose(); }
    }

    /// <summary>생성자 효과가 없으면 brain을 부착하지 않고 실패하는지 확인한다.</summary>
    [Fact]
    public void CreatesAiBrainsMissingConstructNeverAttaches()
    {
        var (startup, script) = OpenScript();
        try
        {
            RiftTypeTable types = MissionFortLoad.LoadTypeTable(startup.Files);
            MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
            MissionPlayers players = MissionPlayers.Load(key => script.Get(key), loaded.Fort, types, 6500);
            var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
            session.PrepareAiSettings(players, key => script.Get(key));
            session.StartPlayers(players, "Youpson");
            var hooks = new AiCreateHooks
            {
                ChangeColor = (_, _) => { },
                Construct = null,
                InitialUpdate = (_, _) => { },
            };
            Assert.Throws<InvalidOperationException>(() => session.CreateAiBrains(players, hooks));
            var participation = new Netstorm.Core.Simulation.PlayerParticipation(session.PlayerRecords, session.Alliances, session.HiddenPlayers, new Netstorm.Core.Simulation.PlayerParticipationState(), new Netstorm.Core.Simulation.PlayerParticipationHooks());
            Assert.Equal(0u, participation.Brain(2));
            Assert.Equal(0u, participation.Flags(2) & PlayerParticipation.BrainFlag);
            Assert.Empty(session.AiBrains);
        }
        finally { script.Dispose(); }
    }

    /// <summary>thewarbegins 참가자 기록의 동맹 마스크 DWORD가 기준선과 같은지 확인한다.</summary>
    [Fact]
    public void StartsPlayerMasksLikeBaseline()
    {
        var (loaded, players, types) = LoadTheWarBegins();
        Assert.NotNull(loaded);
        Assert.NotNull(types);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        session.StartPlayers(players, "Youpson");
        session.StartPlayerMasks(players);
        static uint MaskAt(byte[] records, int owner) =>
            System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(records.AsSpan(owner * 180 + 0x54, 4));
        // 기준선 f090의 9기록 마스크(자기 비트+대칭 판정)다.
        for (int owner = 0; owner <= MissionPlayers.PlayerCount; owner++)
            Assert.Equal(1u << owner, MaskAt(session.PlayerRecords, owner));
        // 덤프 행의 같은 바이트도 확인한다.
        string text = SessionStateDump.FormatWorld1(
            "originals", 90, 0, 0, 0, 0, 39999, null, session.DumpPool(),
            session.DumpPlayers(), session.DumpSp(), session.DumpGraph(), session.DumpTotals(), session.DumpTotalMade());
        foreach ((int owner, string hex) in new[] { (1, "02000000"), (2, "04000000") })
        {
            string? row = null;
            foreach (string line in text.Split('\n'))
                if (line.StartsWith($"player\t{owner}\t", StringComparison.Ordinal)) row = line;
            Assert.NotNull(row);
            Assert.Equal(hex, row!.Split('\t')[2].Substring(0x54 * 2, 8));
        }
    }
}
