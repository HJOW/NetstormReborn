using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 실제 Host/장기 상태/저장소로31h와 전체 개발자 명령의 분기를 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 실제 사제 월드 장면과 Source 계약을 공유한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>같은 Source 자산표/수신 장치와 실제 장기 상태를 갖춘 장면이다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface) DeveloperScene(HostSquidPacketEffects? effects = null, RawPriestWorldEffects? priestEffects = null)
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var surface = new ClientSurfaceGlobals(game, new(local), new Kernel(), () => 0);
        var (session, types) = Scene(surface, initializeSurfaceAssets: true); session.BuildPriestWorldDevices(priestEffects);
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) => throw new InvalidOperationException($"객체 패킷 진단:{text} {file}:{line:x}"));
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), effects ?? new()); return (session, types, surface);
    }
    /// <summary>현재 BYTE 명령을 실제 Host 본문에 전달한다.</summary>
    private static void Developer(HostSession session, string text) => session.ExecuteDeveloperCommand(Encoding.Latin1.GetBytes(text));
    /// <summary>원문 헤더와 별개인 선언DWORD를 가진 첫NUL 문자열 패킷이다.</summary>
    private static byte[] DeveloperPacket(string text)
    {
        byte[] body = Encoding.Latin1.GetBytes(text + "\0ignored"); var packet = new byte[8 + body.Length]; packet[0] = 0xff; packet[1] = 0x31; BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), (ushort)packet.Length); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), uint.MaxValue); body.CopyTo(packet, 8); return packet;
    }
    /// <summary>같은 실제 Net 분배기의 기록/깊이와 고정F가 모두 실제 실행기를 호출한다.</summary>
    [Fact]
    public void DeveloperPacketRegistrationAndFixedEntryExecuteActualSharedSettings()
    {
        var (session, _, _) = DeveloperScene(); var connection = new PacketConnection(); int records = 0;
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 785, Record = (_, _, _) => { Assert.Equal(0u, connection.Depth); records++; }, Terminate = _ => throw PathParentForbidden(), Close = _ => throw PathParentForbidden(), Release = _ => throw PathParentForbidden() });
        DeveloperCommandRegistration.Register(dispatch, session.ExecuteDeveloperCommand); var row = dispatch.Registration(0x31); Assert.Equal(0xffff, row.Length); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Null(row.Observer); Assert.NotNull(row.PacketObserver);
        Assert.True(dispatch.Dispatch(connection, DeveloperPacket("QB 0"))); Assert.Equal(0u, session.GameAux.QuickBuildBox.Value); Assert.Equal(1, records); Assert.Equal(0u, connection.Depth);
        session.BindDeveloperCommandDevices(new() { SetConfig = (_, _) => { Assert.Equal(1u, connection.Depth); Assert.Equal(2, records); } }); Assert.True(dispatch.Dispatch(connection, DeveloperPacket("stl 29")));
        session.ReceiveFixedGameplayPacket(0x44dba0, DeveloperPacket("qv -1")); Assert.Equal(1u, session.GameAux.QuickVortexBox.Value);
        session.ReleasePlayerProcesses(); Assert.Throws<ObjectDisposedException>(() => Developer(session, "qb")); Assert.Throws<ObjectDisposedException>(() => session.BindDeveloperCommandDevices(new())); Assert.Equal(1u, session.GameAux.QuickVortexBox.Value);
    }
    /// <summary>같은7DWORD와Quick 상자의 실제 별칭/기본값/정수 접두부를 검사한다.</summary>
    [Theory]
    [InlineData("shm -2", 1, 1u)]
    [InlineData("setHackMove 0", 1, 0u)]
    [InlineData("snd 7", 2, 1u)]
    [InlineData("setNoDamage 0", 2, 0u)]
    [InlineData("smf", 3, 1u)]
    [InlineData("shootMyFriends x", 3, 1u)]
    [InlineData("smf 0", 3, 0u)]
    [InlineData("fmt 257tail", 4, 257u)]
    [InlineData("fillMyTank 0", 4, 0u)]
    [InlineData("gt 23", 0, 1u)]
    [InlineData("generatorTimer 0", 0, 0u)]
    public void DeveloperSettingsUseSamePersistentDwords(string command, int index, uint expected)
    {
        var (session, _, surface) = DeveloperScene(); var logs = new List<string>(); surface.Storage.BindGraphTrace((level, text) => { Assert.Equal(29, level); logs.Add(text); }); uint[] same = session.GameAux.MissionSettings;
        Developer(session, command); Assert.Same(same, session.GameAux.MissionSettings); Assert.Equal(expected, same[index]); Assert.Equal(command.StartsWith("snd") || command.StartsWith("setNoDamage") ? 2 : index is 1 ? 1 : 0, logs.Count); session.ReleasePlayerProcesses();
    }
    /// <summary>요구/기본 정수, bad command 구두점, 인자 없는 rtt/unknown을 구별한다.</summary>
    [Fact]
    public void DeveloperInvalidArgumentsAndNoOpBranchesKeepSourceDifferences()
    {
        var (session, _, surface) = DeveloperScene(); var logs = new List<string>(); surface.Storage.BindGraphTrace((_, text) => logs.Add(text));
        Developer(session, "quickBuild"); Assert.Equal(1u, session.GameAux.QuickBuildBox.Value); Developer(session, "qb 0"); Assert.Equal(0u, session.GameAux.QuickBuildBox.Value);
        Developer(session, "qv 0"); Assert.Equal(0u, session.GameAux.QuickVortexBox.Value); Assert.Throws<ArgumentException>(() => Developer(session, "qv")); Assert.Throws<ArgumentException>(() => Developer(session, "so 1"));
        Developer(session, "fmt -1"); Developer(session, "slhp 1"); Developer(session, "create 1"); Developer(session, "ad -1 3"); Assert.Equal(new[] { "bad command\n", "bad command.\n", "bad command.\n", "bad command\n" }, logs); logs.Clear();
        Developer(session, "roundtotile " + new string('9', 400)); Developer(session, "rtt"); Developer(session, "dt 1"); Developer(session, "css"); Developer(session, "al"); Developer(session, "gp"); Assert.Empty(logs); session.ReleasePlayerProcesses();
    }
    /// <summary>cr은 상태를 먼저 쓴 뒤 현재 사건과 인자를 실제 큐에 전달한다.</summary>
    [Fact]
    public void DeveloperCombatResizePreservesSettingsBeforeActualQueueFailure()
    {
        HostSession? current = null; var calls = new List<(uint, uint, uint)>(); var (session, _, _) = DeveloperScene(new() { Command = (code, columns, rows) => { calls.Add((code, columns, rows)); Assert.Equal(columns, current!.GameAux.MissionSettings[5]); throw new InvalidOperationException("실제 입력 큐 경계"); } }); current = session;
        session.GameAux.DeveloperCombatResizeCommand = 0x12345678; Assert.Equal("실제 입력 큐 경계", Assert.Throws<InvalidOperationException>(() => Developer(session, "cr 1")).Message);
        Assert.Equal((0x12345678u, 3u, 10u), Assert.Single(calls)); Assert.Equal(0u, session.GameAux.MissionSettings[0]);
        Assert.Throws<InvalidOperationException>(() => Developer(session, "combatgumpcolsrows -2 99")); Assert.Equal((0x12345678u, 1u, 16u), calls[1]); session.ReleasePlayerProcesses();
    }
    /// <summary>다리 스캔 로그 복귀 뒤 현재 주기와 게임시각을 다시 읽는다.</summary>
    [Fact]
    public void DeveloperBridgeScanUsesCurrentPeriodAndClockAfterTrace()
    {
        var (session, _, surface) = DeveloperScene(); string? log = null; surface.Storage.BindGraphClock(() => 35.5);
        surface.Storage.BindGraphTrace((level, text) => { Assert.Equal(29, level); log = text; Assert.Equal(1.2f, surface.Storage.BridgeDecayPeriod); surface.Storage.BridgeDecayPeriod = .25f; });
        Developer(session, "SETBRIDGEKILLPERIOD 1.2tail"); Assert.Equal("bkPERIOD now 1.200000\nresetting bridge killer\n", log); Assert.Equal(35.75, surface.Storage.BridgeDecayScan.Next); Assert.Equal(15000, surface.Storage.BridgeDecayScan.Cursor); Assert.Equal(23001, surface.Storage.BridgeDecayScan.Last); session.ReleasePlayerProcesses();
    }
    /// <summary>도움말32줄을 출력하고 실제 설정/기록/heap/시스템 서비스의 원문 인자를 보존한다.</summary>
    [Fact]
    public void DeveloperHelpAndExternalServicesKeepOriginalBytesAndOrder()
    {
        var (session, _, surface) = DeveloperScene(); var logs = new List<string>(); surface.Storage.BindGraphTrace((_, text) => logs.Add(text)); var effects = new List<string>();
        session.BindDeveloperCommandDevices(new() { SetConfig = (key, value) => effects.Add(key + "=" + Convert.ToHexString(value.Span)), HeapTrace = value => effects.Add("heap:" + value), QueryDirectDraw = () => effects.Add("qdd"), RecordLabel = text => effects.Add("label:" + text) });
        Developer(session, "?"); Assert.Equal(32, logs.Count); Assert.StartsWith("setOwner(so)", logs[0]); Assert.Equal("getPriest(gp) - sticks a enemy priest next to your priest\n", logs[^1]);
        Developer(session, "setTraceLevel 017tail ignored"); Developer(session, "ht -2"); Developer(session, "queryDirectDraw"); session.Gameplay.Cursor.Recording = 1; Developer(session, "addLabel étiquette next");
        Assert.Equal(new[] { "tracelevel=3031377461696C00", "heap:True", "qdd", "label:étiquette" }, effects); Assert.Throws<ArgumentException>(() => Developer(session, "stl")); session.ReleasePlayerProcesses();
    }
    /// <summary>각 Player trace 뒤 현재 이름/표시 관문을 다시 읽고 별도 Player 사본을 만들지 않는다.</summary>
    [Fact]
    public void DeveloperPlayerAndAllianceDumpsReadCurrentRecordsAfterTrace()
    {
        var (session, _, surface) = DeveloperScene(); var logs = new List<string>(); Array.Clear(session.PlayerRecords);
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x50), 0x23); "one\0"u8.CopyTo(session.PlayerRecords.AsSpan(180 + 12));
        surface.Storage.BindGraphTrace((_, text) => { logs.Add(text); if (logs.Count == 1) { BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(360 + 0x50), 0x22); "two\0"u8.CopyTo(session.PlayerRecords.AsSpan(360 + 12)); } });
        Developer(session, "dumpplayers"); Assert.Equal(new[] { "id:1 one Logged in\n", "id:2 two NotLogged in\n" }, logs); logs.Clear();
        surface.Storage.BindGraphTrace((_, text) => { logs.Add(text); BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x54), 4); });
        Developer(session, "dally 1"); Assert.Equal(new[] { "Ally bits for 1 one\n", " 2 two\n" }, logs); session.ReleasePlayerProcesses();
    }
    /// <summary>그림 덤프의 최초 상한과 진단 뒤 현재 헤더 재조회를 실제 저장소에서 구별한다.</summary>
    [Fact]
    public void DeveloperTypeArtAndGraphDumpsUseSameCurrentTables()
    {
        var (session, _, surface) = DeveloperScene(); var storage = surface.Storage; var logs = new List<string>(); storage.BindGraphTrace((_, text) => logs.Add(text));
        storage.Types[70].Name = "test\0ignored"; storage.Types[70].Flags1 = uint.MaxValue; storage.Types[70].ZOrder = -3; storage.Shapes[70].FrameCount = 2;
        Developer(session, "dumptype 70"); Assert.Equal("70:test, flags=-1, depth=-3 numFrames=2\n", Assert.Single(logs)); logs.Clear();
        storage.Shapes[70].Loaded = true; storage.Shapes[70].Frames.Clear(); storage.Shapes[70].Frames.Add(new() { HotspotX = 1 }); storage.Shapes[70].Frames.Add(new() { HotspotX = 2 });
        storage.BindGraphTrace((_, text) => { logs.Add(text); storage.Shapes[70].Frames[1].HotspotX = 17; }); Developer(session, "dad 70"); Assert.Equal(2, logs.Count); Assert.StartsWith("1: hx=17 ", logs[1]); logs.Clear();
        storage.Graph.Records[254].Surfaces = -3; storage.Graph.Records[254].InUse = -7; Developer(session, "dg 254"); Assert.Equal("graph index: 254 numSurface: -3 inUse: -7\n", Assert.Single(logs)); logs.Clear();
        Developer(session, "dts 1"); Assert.Equal(storage.Types.Count - 1, logs.Count); Assert.StartsWith("01:", logs[0]); session.ReleasePlayerProcesses();
    }
    /// <summary>불완전 Subscriber 입력도 현재 ID/handle을 출력하고 manager 없음은 변경을 생략한다.</summary>
    [Fact]
    public void DeveloperSubscriberUsesActualChangeWriterAndSharedIdentityBeforeNotify()
    {
        var (session, _, surface) = DeveloperScene(); var logs = new List<string>(); surface.Storage.BindGraphTrace((_, text) => logs.Add(text)); "before\0"u8.CopyTo(session.HandleBuffer); session.WorldGlobals.MySubscriber = 17;
        Developer(session, "ssi 18"); Assert.Equal("subid = 17 handle = before\n", Assert.Single(logs)); logs.Clear(); Developer(session, "ssi 18 absent"); Assert.Equal(17u, session.WorldGlobals.MySubscriber); logs.Clear();
        session.MissionPlayer.TemplateOwner = 1; session.OwnerMode.Fort = true; session.Templates.Install(1, new FortTemplate(), "same"); var effects = new List<string>();
        // 실제 메모리 전용 Template bit는 Changed의 파일 저장 경계를 선택하지 않는다.
        BinaryPrimitives.WriteUInt32LittleEndian(session.WorldTemplates.AsSpan(20 + 8), 1);
        session.BindDeveloperCommandDevices(new() { SetConfig = (key, bytes) => { Assert.Equal(17u, session.WorldGlobals.MySubscriber); effects.Add(key + ":" + Encoding.Latin1.GetString(bytes.Span)); }, RefreshTitle = () => effects.Add("title"), SubscriberRegistration = new() });
        Developer(session, "setsubid 23 after"); Assert.Equal(23u, session.WorldGlobals.MySubscriber); Assert.Equal("after", session.Templates.ReadSubscriberName(1)); Assert.Equal(new[] { "fort.mySubHandle:after\0", "title" }, effects); Assert.Equal("subid = 23 handle = after\n", Assert.Single(logs)); session.ReleasePlayerProcesses();
    }
    /// <summary>실제 미션 파일 저장 진단 전까지 ID/handle/Subscriber stream 쓰기를 보존한다.</summary>
    [Fact]
    public void DeveloperSubscriberMissionNotifyFailureKeepsEarlierSharedWrites()
    {
        var (session, _, surface) = DeveloperScene(); session.MissionPlayer.TemplateOwner = 1; session.OwnerMode.Fort = true; session.Templates.Install(1, new FortTemplate(), "same"); var logs = new List<string>(); surface.Storage.BindGraphTrace((_, text) => logs.Add(text));
        session.BindDeveloperCommandDevices(new() { SetConfig = (_, _) => { }, RefreshTitle = () => { }, SubscriberRegistration = new() });
        Assert.Equal("실제Player Template State진단:InMissionSave", Assert.Throws<InvalidOperationException>(() => Developer(session, "ssi 51 current")).Message);
        Assert.Equal(51u, session.WorldGlobals.MySubscriber); Assert.Equal("current", session.Templates.ReadSubscriberName(1)); Assert.StartsWith("current\0", Encoding.Latin1.GetString(session.HandleBuffer)); Assert.Empty(logs); session.ReleasePlayerProcesses();
    }
    /// <summary>PATCH setally/clrally는 실제 같은 AddAlly를 부르며 반대 방향은 바꾸지 않는다.</summary>
    [Fact]
    public void DeveloperBothAllianceCommandsUseActualSameDirectedAdd()
    {
        var (session, _, _) = DeveloperScene(); var participation = new RawPlayerParticipation(OriginalEdition.Patch1078, session.PlayerRecords, session.Alliances, session.HiddenPlayers, new() { Battle = false }, new()); session.BindFixedAlliancePackets(participation, new());
        Developer(session, "setally 2 3"); Assert.Equal(1u, session.Alliances[2 * 9 + 3]); Assert.Equal(0u, session.Alliances[3 * 9 + 2]); Developer(session, "clrally 2 3"); Assert.Equal(1u, session.Alliances[2 * 9 + 3]); Assert.Equal(8u, BinaryPrimitives.ReadUInt32LittleEndian(session.PlayerRecords.AsSpan(2 * 180 + 0x54)) & 8); session.ReleasePlayerProcesses();
    }
    /// <summary>서버 Owner와 local HP 명령은 같은 실제 Unpop/파생Owner/HP/재Pop을 사용한다.</summary>
    [Fact]
    public void DeveloperServerOwnershipAndLocalHealthUseActualSpatialBodies()
    {
        var priestEffects = new RawPriestWorldEffects(); priestEffects.Shield.ReserveSound = () => false; // 원문 음향 할당 실패 분기를 명시 입력한다.
        var (session, types, _) = DeveloperScene(priestEffects: priestEffects); int priest = PlacePriest(session, types, 0, 9); int highlights = 0; session.BindDeveloperCommandDevices(new() { Highlight = (_, _) => highlights++ }); session.SetWorldLoadingDepth(1);
        Developer(session, $"setowner {priest} 2"); Assert.Equal(2, session.Pool.Slot(priest)[34]); Assert.Equal(0, session.Pool.Slot(priest)[11] & 4);
        Developer(session, $"slhp {priest} 123"); Assert.Equal(123, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(priest)[26..])); Assert.Equal(1, highlights); session.ReleasePlayerProcesses();
    }
    /// <summary>실제 Source 공장/Pop과같은 CRT를 scatter의각열에 전달한다.</summary>
    [Fact]
    public void DeveloperCreateAndScatterUseActualFactoryPopAndSharedCrt()
    {
        var (session, _, _) = DeveloperScene(); Developer(session, "create 84 0"); int before = checked((int)session.Pool.FreeCount);
        int frames = session.WorldFrames[84].Codes.Count; Assert.True(frames > 0); uint seed = session.Crt.State(); var expected = new CrtRandom(seed); var rolls = new List<uint>();
        // 독립 CRT 소비자는 열마다 한 번만 전진한다.
        for (int index = 0; index < 10; index++) rolls.Add(expected.Next() % (uint)frames);
        Developer(session, "sct 84"); Assert.Equal(expected.State(), session.Crt.State()); Assert.Equal(5, session.GameAux.DeveloperScatterRow); Assert.Equal(before - 10, (int)session.Pool.FreeCount);
        var found = new List<(float X, float Y, uint Frame)>();
        // 실제 Pool의 Pop된 고갈된 간헐천만 읽으며 별도 객체 생성 훅으로 대체하지 않는다.
        for (int sid = 1; sid < session.Pool.Capacity; sid++) { ReadOnlySpan<byte> raw = session.Pool.Slot(sid); if ((raw[11] & 5) == 0 && raw[10] == 84) found.Add((BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]), BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]))); }
        Assert.Contains((20f, 20f, 0u), found);
        // 각 열의현재 실제 프레임과 행이독립 난수 결과에 일치해야 한다.
        for (int index = 0; index < 10; index++) Assert.Contains(((float)(3 + index * 3), 3f, rolls[index]), found);
        session.ReleasePlayerProcesses();
    }
    /// <summary>명시 음향 할당 실패 뒤 실제 사제 탐색/Owner/동쪽 배치가 같은 낙하 경계까지 이어진다.</summary>
    [Fact]
    public void DeveloperGetPriestUsesActualHashAndEnemyOwnerBeforeFallSoundBoundary()
    {
        var priestEffects = new RawPriestWorldEffects(); priestEffects.Shield.ReserveSound = () => false; // 실제 음향 할당 실패 입력으로 나머지 기하/피해 경계를 격리한다.
        var (session, types, _) = DeveloperScene(priestEffects: priestEffects); int priest = PlacePriest(session, types, 0, 9); session.WorldGlobals.MissionPost = true; BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x50), 1); BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(360 + 0x50), 0); session.SetWorldLoadingDepth(1); int reached = 0;
        session.BindDeveloperCommandDevices(new() { Damage = (_, _, _, _) => reached++ });
        Assert.Equal("호스트 사제 낙하 위치 소리 실제 몸체 미연결(R6)", Assert.Throws<InvalidOperationException>(() => Developer(session, "getPriest")).Message); Assert.Equal(0, reached); Assert.Equal(2u, session.SidLists.Priests.Count); int enemies = 0;
        // 실제 생성/동쪽 배치/owner 쓰기까지의 부분 효과를 같은 Pool에서 확인한다.
        foreach (uint entry in session.SidLists.Priests.Items()) { int sid = (int)entry; if (sid == priest) continue; enemies++; Assert.Equal(2, session.Pool.Slot(sid)[34]); Assert.True(BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(sid)[14..]) > 50); }
        Assert.Equal(1, enemies); session.SetWorldLoadingDepth(0); session.ReleasePlayerProcesses();
    }
    /// <summary>Dude의 전체 전용 후처리 미연결을 숨기지 않고 앞선 생성/프레임/좌표/공간 등록을 유지한다.</summary>
    [Fact]
    public void DeveloperDudeCreateStopsAtActualMissingPostPopAfterEarlierWrites()
    {
        var (session, _, _) = DeveloperScene(); uint free = session.Pool.FreeCount; uint random = session.Crt.State();
        Assert.Equal("현재 가상 후처리 몸체 미복원", Assert.Throws<InvalidOperationException>(() => Developer(session, "create 70 0")).Message);
        Assert.Equal(free - 1, session.Pool.FreeCount); Assert.Equal(random, session.Crt.State()); int found = 0;
        // 실패 전에 Pop된 실제 Dude 슬롯을 찾아 부분 효과를 확인한다.
        for (int sid = 1; sid < session.Pool.Capacity; sid++) if (session.Pool.Slot(sid)[10] == 70 && (session.Pool.Slot(sid)[11] & 5) == 0) { found++; Assert.Equal(20, BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(sid)[14..])); Assert.Equal(20, BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(sid)[18..])); }
        Assert.Equal(1, found); session.ReleasePlayerProcesses();
    }
    /// <summary>기본 음향 미연결은 사제/보호막 생성 뒤 드러나며 전체 피해는 아직 실행하지 않는다.</summary>
    [Fact]
    public void DeveloperGetPriestKeepsActualSoundBoundaryAndCreatedEnemyObjects()
    {
        var (session, types, _) = DeveloperScene(); _ = PlacePriest(session, types, 0, 9); session.WorldGlobals.MissionPost = true; BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x50), 1); session.SetWorldLoadingDepth(1); int damages = 0;
        session.BindDeveloperCommandDevices(new() { Damage = (_, _, _, _) => damages++ });
        Assert.Equal("호스트 사제 보호막 소리 예약 실제 몸체 미연결(R6)", Assert.Throws<InvalidOperationException>(() => Developer(session, "gp")).Message); Assert.Equal(0, damages); Assert.Equal(2u, session.SidLists.Priests.Count); session.SetWorldLoadingDepth(0); session.ReleasePlayerProcesses();
    }
    /// <summary>서버 가상 피해/전체 삭제 경계를 실제 현재 슬롯에 전달하고 일반 HP 수정으로 대체하지 않는다.</summary>
    [Fact]
    public void DeveloperDamageDestroyAndClientOwnershipUseActualCurrentSlot()
    {
        int deleted = 0; var (session, types, surface) = DeveloperScene(new() { VirtualDestroy = (sid, flags) => { Assert.Equal(0u, flags); deleted = sid; } }); int priest = PlacePriest(session, types, 0, 9); int health = BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(priest)[26..]); var effects = new List<string>(); var logs = new List<string>(); surface.Storage.BindGraphTrace((_, text) => logs.Add(text));
        session.BindDeveloperCommandDevices(new() { Damage = (sid, amount, owner, source) => { Assert.Equal(priest, sid); Assert.Equal(uint.MaxValue, amount); Assert.Equal(0u, owner); Assert.Equal(0, source); effects.Add("damage"); }, Highlight = (x, y) => { Assert.Equal(50, x); Assert.Equal(60, y); effects.Add("highlight"); }, TraceObject = sid => { Assert.Equal(priest, sid); effects.Add("dump"); } });
        Developer(session, $"ad {priest} -1"); Assert.Equal(health, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(priest)[26..])); Developer(session, $"de {priest}"); Assert.Equal(priest, deleted);
        session.Gameplay.Authority.Server = false; Developer(session, $"so {priest} 255"); Assert.Equal(1, session.Pool.Slot(priest)[34]); Assert.Equal($"requested owner on {priest} set to -1\n", Assert.Single(logs)); Developer(session, $"dump {priest}"); Assert.Equal(new[] { "damage", "highlight", "highlight", "dump" }, effects); session.ReleasePlayerProcesses();
    }
}
