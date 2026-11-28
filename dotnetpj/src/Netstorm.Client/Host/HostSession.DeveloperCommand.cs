using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

// 개발자 수신도 같은 실제 Host의 상태/공장/공간을 사용한다.
namespace Netstorm.Client.Host;

/// <summary>원문 서비스로 분리된 화면·진단·설정·기록·가상 피해 장치다.</summary>
public sealed record HostDeveloperCommandEffects
{
    /// <summary>현재 월드 좌표의 실제 화면 강조다.</summary>
    public Action<float, float>? Highlight { get; init; }
    /// <summary>44dab0의 실제 현재 객체 상세 덤프다.</summary>
    public Action<int>? TraceObject { get; init; }
    /// <summary>현재 BYTE 값과 NUL을 실제 Config에 전달한다.</summary>
    public Action<string, ReadOnlyMemory<byte>>? SetConfig { get; init; }
    /// <summary>현재 사용자 이름 복사 뒤 실제 창 제목을 갱신한다.</summary>
    public Action? RefreshTitle { get; init; }
    /// <summary>같은 Client ID/CRT/Config로 Subscriber 등록을 마무리하는 실제 서비스다.</summary>
    public FortSubscriberLoadHooks? SubscriberRegistration { get; init; }
    /// <summary>실제 Client 프레임의 heap 통계 출력을 설정한다.</summary>
    public Action<bool>? HeapTrace { get; init; }
    /// <summary>현재 시스템의 실제 DirectDraw 정보를 출력한다.</summary>
    public Action? QueryDirectDraw { get; init; }
    /// <summary>현재 활성 기록 파일에 원문 label을 쓴다.</summary>
    public Action<string>? RecordLabel { get; init; }
    /// <summary>같은 현재 객체의 전체 가상98 피해 본체다. 일반 HP setter로 대신하지 않는다.</summary>
    public Action<int, uint, uint, int>? Damage { get; init; }
}

/// <summary>RawCampaignSession.cpp5042~5156의 전체 개발자 명령 실행 본체다.</summary>
public sealed partial class HostSession
{
    /// <summary>값 복사한 현재 실제 큰 서비스며 일반 미션 상태를 별도로 만들지 않는다.</summary>
    private HostDeveloperCommandEffects? _developerEffects;
    /// <summary>같은 월드 장치 설치 후 실제 Root 서비스를 공급한다. 명령을 실행하거나 상태를 초기화하지 않는다.</summary>
    public void BindDeveloperCommandDevices(HostDeveloperCommandEffects effects)
    {
        ArgumentNullException.ThrowIfNull(effects); EnsureRuntimeCanRelease(); DeveloperStorage(); _developerEffects = effects with { };
    }
    /// <summary>반납/미설치/CD의잘못된10.78 슬롯 접근을 명시적으로 거부한다.</summary>
    private RawSurfaceStorage DeveloperStorage()
    {
        if (_playerRecordsReleased || ProcessBindingsReturned() || _playerProcessRuntime is null) throw new ObjectDisposedException(nameof(HostSession), "개발자 명령의 실제 World 반납 뒤 호출");
        if (Pool.Edition != OriginalEdition.Patch1078) throw new InvalidOperationException("개발자 명령 전체 본문의 CD 경로 미연결");
        return FixedPacketNeed(_squidPacketStorage);
    }
    /// <summary>signed 입력을 자르지 않고 원문 SID의 물리 범위를 검사한다.</summary>
    private int DeveloperSid(int value) => value >= 0 && value <= 65535 && value < Pool.Capacity ? value : throw new ArgumentOutOfRangeException(nameof(value), "개발자 명령의 현재 SID 물리 범위");
    /// <summary>SID0은 실제 화면/카메라 장치를 조회하지 않는다.</summary>
    private void DeveloperHighlight(int sid)
    {
        if (sid == 0) return; ReadOnlySpan<byte> raw = Pool.Slot(sid);
        float x = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), y = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]); FixedPacketNeed(_developerEffects?.Highlight)(x, y);
    }
    /// <summary>각 trace 직전에 같은 현재 Player 이름의 첫NUL을 조회한다.</summary>
    private string DeveloperPlayerName(uint owner)
    {
        if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "개발자 Player 이름의 물리 범위");
        return DeveloperCString(PlayerRecords.AsSpan((int)owner * 180 + 12, 32));
    }
    /// <summary>현재8비트 문자열의 물리 범위 안 첫NUL까지 읽는다.</summary>
    private static string DeveloperCString(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0); if (end < 0) throw new ArgumentOutOfRangeException(nameof(bytes), "개발자 현재 문자열 NUL 누락"); return Encoding.Latin1.GetString(bytes[..end]);
    }
    /// <summary>실제 Config/문자열 writer에 넘길 NUL 포함 BYTE를 만든다.</summary>
    private static byte[] DeveloperBytes(string value) => Encoding.Latin1.GetBytes(value + '\0');
    /// <summary>매 호출 시 현재 같은 TYPE/그림을 조회하고 원문 수준29 서식을 유지한다.</summary>
    private static void DeveloperDumpType(RawSurfaceStorage storage, uint type)
    {
        if (type >= storage.Types.Count || type >= storage.Shapes.Count) throw new ArgumentOutOfRangeException(nameof(type), "개발자 TYPE 덤프의 현재 물리 범위");
        OriginalTypeRecord record = storage.Types[(int)type]; string name = record.Name.Split('\0')[0];
        string number = unchecked((int)type).ToString("D2", CultureInfo.InvariantCulture);
        storage.TraceGraph(29, $"{number}:{name}, flags={unchecked((int)record.Flags1)}, depth={record.ZOrder} numFrames={storage.Shapes[(int)type].FrameCount}\n");
    }
    /// <summary>같은 실제 Template manager/stream/등록 본체로 Subscriber를 저장한다.</summary>
    private void DeveloperWriteSubscriber(uint slot, uint id, ReadOnlySpan<byte> handle)
    {
        TemplateWriteStream? stream = null;
        RawFortSubscriberWrite.SaveFortSubscriber(slot, id, HandleBuffer, new()
        {
            Manager = owner => BinaryPrimitives.ReadUInt32LittleEndian(WorldTemplates.AsSpan(checked((int)owner * 20), 4)),
            Assertion = line => throw new InvalidOperationException("신규 Subscriber 원문 Template 진단:" + line),
            RegisterSubscriber = () => RawFortSubscriberLoad.FinishFortSubscriberRegistration(new() { GetSubscriber = () => WorldGlobals.MySubscriber, SetSubscriber = value => WorldGlobals.MySubscriber = value }, FixedPacketNeed(_developerEffects?.SubscriberRegistration)),
            Create = (manager, section) => { stream = Templates.CreateManagerSection(manager, section); return 1; },
            Write = (token, bytes) => { if (token != 1 || stream is null) throw new InvalidOperationException("신규 Subscriber 고정 stream 누락"); Templates.WriteSection(stream.Value, bytes); },
        });
    }
    /// <summary>현재 Template/owner/handle/Subscriber를 각 효과 뒤 다시 조회한다. Guide 요청과 개발자 명령이 같은 본체를 공유한다.</summary>
    public void ChangeFortSubscriber(uint id, byte[]? name)
    {
        RawFortSubscriberChange.ChangeFortSubscriber(id, name, ref WorldGlobals.MySubscriberStorage, new()
        {
            Owner = () => MissionPlayer.TemplateOwner,
            Manager = owner => BinaryPrimitives.ReadUInt32LittleEndian(WorldTemplates.AsSpan(checked((int)owner * 20), 4)),
            FortMode = () => OwnerMode.Fort ? 1u : 0u, Handle = () => HandleBuffer,
            Configure = (key, value) => FixedPacketNeed(_developerEffects?.SetConfig)(key, DeveloperBytes(value)),
            Title = () => FixedPacketNeed(_developerEffects?.RefreshTitle)(), Subscriber = DeveloperWriteSubscriber, Notify = NotifyPlayerTemplateState,
        });
    }
    /// <summary>현재 실제 저장소의 같은 객체/장부와 명령별 선택 서비스로 전체 본문을 실행한다.</summary>
    public void ExecuteDeveloperCommand(ReadOnlyMemory<byte> text)
    {
        RawSurfaceStorage storage = DeveloperStorage(); var input = new RawDeveloperCommandInput(text.Span); string key = input.Key;
        uint[] settings = GameAux.MissionSettings; int first, second;
        // 원문 로그는 같은 저장소의 필터/출력 서비스로만 전달한다.
        void Log(string message) => storage.TraceGraph(29, message);
        if (key is "help" or "?") { foreach (string line in DeveloperHelp) Log(line); return; } // 원문32개 도움말 순서를 보존한다.
        if (key is "dump" or "d") { int value = input.One(out first) ? first : 0; if (value == 0) return; int sid = DeveloperSid(value); DeveloperHighlight(sid); FixedPacketNeed(_developerEffects?.TraceObject)(sid); return; }
        if (key is "setowner" or "so")
        {
            if (input.Two(out first, out second) != 2) throw new ArgumentException("개발자 소유자 명령의 원문 미정 정수 입력");
            int sid = DeveloperSid(first), owner = unchecked((sbyte)(byte)second);
            if (Gameplay.Authority.Server) { UnpopWorldObject(sid, 0); SetWorldObjectOwner(sid, unchecked((uint)owner)); RepopWorldObject(sid, 8); return; }
            DeveloperHighlight(sid); Log($"requested owner on {first} set to {owner}\n"); return;
        }
        if (key is "setlocalhitpoints" or "slhp")
        {
            if (input.Two(out first, out second) != 2) { Log("bad command.\n"); return; }
            int sid = DeveloperSid(first); UnpopWorldObject(sid, 0); RawSquidHitPoints.Set(Pool, sid, second); RepopWorldObject(sid, 0); DeveloperHighlight(sid);
            int current = BinaryPrimitives.ReadInt32LittleEndian(Pool.Slot(sid)[26..]); int maximum = Reward.MaxHitPoints(sid); Log($"sid {first} changed to {current} hp.  (max for type = {maximum})\n"); return;
        }
        if (key is "dumpplayers" or "dp")
        {
            // Player1..8의 표시 관문과 이름을 현재 로그 직전에 읽는다.
            for (uint owner = 1; owner < 9; owner++) { uint flags = BinaryPrimitives.ReadUInt32LittleEndian(PlayerRecords.AsSpan((int)owner * 180 + 0x50)); if ((flags & 0x22) != 0) Log($"id:{owner} {DeveloperPlayerName(owner)} {((flags & 1) != 0 ? "" : "Not")}Logged in\n"); }
            return;
        }
        if (key is "dumpally" or "dally")
        {
            if (!input.One(out first)) return; uint owner = unchecked((uint)first); Log($"Ally bits for {first} {DeveloperPlayerName(owner)}\n");
            // 각 trace 복귀 뒤 같은 현재 동맹 bit를 다시 읽는다.
            for (uint target = 1; target < 9; target++) if ((BinaryPrimitives.ReadUInt32LittleEndian(PlayerRecords.AsSpan((int)owner * 180 + 0x54)) & (1u << (int)target)) != 0) Log($" {target} {DeveloperPlayerName(target)}\n");
            return;
        }
        if (key is "setsubid" or "ssi")
        {
            input.BeginArguments(); if (input.Integer(out first) && input.Word() is { } name) ChangeFortSubscriber(unchecked((uint)first), DeveloperBytes(name));
            Log($"subid = {unchecked((int)WorldGlobals.MySubscriber)} handle = {DeveloperCString(HandleBuffer)}\n"); return;
        }
        if (key is "setbridgekillperiod" or "sbkp")
        {
            float period = input.RequiredFloat(); storage.BridgeDecayPeriod = period;
            string number = RawDeveloperCommandInput.FormatFloat(period);
            Log($"bkPERIOD now {number}\nresetting bridge killer\n"); RawStateBridgeReset.ResetDecayScan(storage.BridgeDecayScan, storage.BridgeDecayPeriod, storage.ReadGraphClock()); return;
        }
        if (key is "settracelevel" or "stl") { input.BeginArguments(); string value = input.Word() ?? throw new ArgumentException("개발자 tracelevel의 원문 미정 문자열 입력"); FixedPacketNeed(_developerEffects?.SetConfig)("tracelevel", DeveloperBytes(value)); return; }
        if (key is "sethackmove" or "shm") { settings[1] = input.Required() != 0 ? 1u : 0u; Log($"hackmover now {settings[1]}\n"); return; }
        if (key is "setnodamage" or "snd") { settings[2] = input.Required() != 0 ? 1u : 0u; Log($"nodamagedebug now {settings[2]}\n"); Log("requested server to do same.\n***Not echoed to other clients.  This will FUCK EVERYTHING!***\n"); return; }
        if (key is "shootmyfriends" or "smf") { settings[3] = !input.One(out first) || first != 0 ? 1u : 0u; return; }
        if (key is "fillmytank" or "fmt" or "generatortimer" or "gt") { if (!input.One(out first) || first < 0) { Log("bad command\n"); return; } settings[key is "fillmytank" or "fmt" ? 4 : 0] = key is "fillmytank" or "fmt" ? (uint)first : first != 0 ? 1u : 0u; return; }
        if (key is "quickvortex" or "qv") { GameAux.QuickVortexBox.Value = input.Required() != 0 ? 1u : 0u; return; }
        if (key is "quickbuild" or "qb") { GameAux.QuickBuildBox.Value = !input.One(out first) || first != 0 ? 1u : 0u; return; }
        if (key is "roundtotile" or "rtt") return; // 실제10.78은 인자를 읽지 않는 빈 반환이다.
        if (key is "combatgumpcolsrows" or "cr")
        {
            if (input.Two(out first, out second) != 2) { first = 3; second = 10; }
            settings[5] = (uint)Math.Clamp(first, 1, 3); settings[6] = (uint)Math.Clamp(second, 1, 16); if (settings[5] * settings[6] > 8) settings[0] = 0;
            FixedPacketNeed(_squidPacketEffects?.Command)(GameAux.DeveloperCombatResizeCommand, settings[5], settings[6]); return;
        }
        if (key is "setally" or "sally" or "clrally" or "cally") { if (input.Two(out first, out second) == 2) FixedPacketNeed(_fixedAllianceParticipation).AddAlly(unchecked((uint)first), unchecked((uint)second)); return; }
        if (key is "destroy" or "de") { if (!Gameplay.Authority.Server || !input.One(out first)) return; int sid = DeveloperSid(first); if ((Pool.Slot(sid)[11] & 1) == 0) FixedPacketNeed(_squidPacketEffects?.VirtualDestroy)(sid, 0); return; }
        if (key is "create" or "c")
        {
            if (input.Two(out first, out second) != 2) { Log("bad command.\n"); return; } if (!Gameplay.Authority.Server) return;
            int sid = FixedPacketNeed(_livePlayerHooks?.Create)(unchecked((uint)first), 0); if (sid == 0) throw new InvalidOperationException("개발자 create의 원문 NULL 슬롯");
            BinaryPrimitives.WriteInt32LittleEndian(Pool.AllocatedBytes(sid)[36..], second); PopWorldObject(sid, 20, 20, 0); return;
        }
        if (key is "scatter" or "sct")
        {
            if (!input.One(out first)) { Log("bad command.\n"); return; } if (!Gameplay.Authority.Server) return;
            // 원문3..30 열마다 같은 공장/프레임/15비트 CRT를 한 번 사용한다.
            for (int x = 3; x < 33; x += 3)
            {
                int sid = FixedPacketNeed(_livePlayerHooks?.Create)(unchecked((uint)first), 0); if (sid == 0) throw new InvalidOperationException("개발자 scatter의 원문 NULL 슬롯");
                int type = Pool.Slot(sid)[10]; if (type >= WorldFrames.Length) throw new ArgumentOutOfRangeException(nameof(type)); int count = WorldFrames[type].Codes.Count; uint roll = Crt.Next();
                if (count == 0) throw new ArgumentOutOfRangeException(nameof(count), "개발자 scatter의 원문 나눗셈 프레임 개수");
                BinaryPrimitives.WriteUInt32LittleEndian(Pool.AllocatedBytes(sid)[36..], roll % (uint)count); PopWorldObject(sid, x, (float)GameAux.DeveloperScatterRow, 0);
            }
            GameAux.DeveloperScatterRow = unchecked(GameAux.DeveloperScatterRow + 2) % 10; return;
        }
        if (key is "setabstract" or "sa")
        {
            if (input.Two(out first, out second) != 2) { Log("bad command.\n"); return; } int sid = DeveloperSid(first); UnpopWorldObject(sid, 0);
            if ((Pool.AllocatedBytes(sid)[11] & 4) == 0) storage.AssertDisplay("isVoid()", @"c:\ns\o\squid.h", 0x282);
            Span<byte> raw = Pool.AllocatedBytes(sid); raw[40] ^= unchecked((byte)((raw[40] ^ (byte)second) & 1)); RepopWorldObject(sid, 0); return;
        }
        if (key is "applydamage" or "ad") { if (input.Two(out first, out second) != 2 || first < 0) { Log("bad command\n"); return; } FixedPacketNeed(_developerEffects?.Damage)(DeveloperSid(first), unchecked((uint)second), 0, 0); return; }
        if (key is "dumptypes" or "dts")
        {
            _ = input.One(out first);
            // 각 로그 복귀 뒤 현재 TYPE 개수를 다시 확인하고타입0은 제외한다.
            for (uint type = 1; type < storage.Types.Count; type++) DeveloperDumpType(storage, type);
            return;
        }
        if (key == "dumptype") { DeveloperDumpType(storage, input.One(out first) ? unchecked((uint)first) : 0); return; } // 도움말dt는 실제 별칭이 아니다.
        if (key is "dumpartdata" or "dad")
        {
            if (!input.One(out first)) { Log("bad command\n"); return; } uint type = unchecked((uint)first);
            if (type >= storage.Shapes.Count || type >= storage.Types.Count) throw new ArgumentOutOfRangeException(nameof(type)); int frames = storage.Shapes[(int)type].FrameCount;
            // 최초 프레임 상한을 유지하고 각 진단/헤더는 현재 표에서 다시 조회한다.
            for (int frame = 0; frame < frames; frame++)
            {
                SquidDisplayShape shape = storage.Shapes[(int)type]; OriginalTypeRecord record = storage.Types[(int)type]; int limit = unchecked(shape.FrameCount * ((record.Flags1 & 0x440000) != 0 ? 2 : 1)); bool inRange = frame < limit;
                if (!inRange) storage.RecordDisplayFrameError(type, frame, 0);
                if (!inRange || !shape.Loaded || record.MaxHitPoints is -0x22222223 or -0x32323233) storage.AssertDisplay("frameCheck( abstract, frame, false )", @"..\o\rifttype.h", 0xe1);
                SquidDisplayFrame header = storage.Shapes[(int)type].Frames[frame]; Log($"{frame}: hx={header.HotspotX} hy={header.HotspotY} w={header.Width} h={header.Height} wx={header.MuzzleX} wy={header.MuzzleY}\n");
            }
            return;
        }
        if (key is "dumpgraph" or "dg")
        {
            if (!input.One(out first)) { Log("bad command\n"); return; } if ((uint)first >= storage.Graph.Records.Length) throw new ArgumentOutOfRangeException(nameof(first));
            var record = storage.Graph.Records[first]; Log($"graph index: {first} numSurface: {record.Surfaces} inUse: {record.InUse}\n"); return;
        }
        if (key is "heaptrace" or "ht") { if (!input.One(out first)) { Log("bad command\n"); return; } FixedPacketNeed(_developerEffects?.HeapTrace)(first != 0); return; }
        if (key is "querydirectdraw" or "qdd") { FixedPacketNeed(_developerEffects?.QueryDirectDraw)(); return; }
        if (key is "getpriest" or "gp") { DeveloperGetPriest(storage); return; }
        if (key is "addlabel" or "al") { if (Gameplay.Cursor.Recording == 0) return; input.BeginArguments(); if (input.Word() is { } label) FixedPacketNeed(_developerEffects?.RecordLabel)(label); return; }
    }
    /// <summary>missionPost 관문과 같은 해시의 첫 로컬 사제, 첫 비로그인 Player를 원문 순서로 사용한다.</summary>
    private void DeveloperGetPriest(RawSurfaceStorage storage)
    {
        if (!WorldGlobals.MissionPost) return; var finder = new RawSquidFinder(Pool, FixedPacketNeed(_terrainHash), FixedPacketNeed(_terrainPopTypes)); int priest = 0;
        // 같은 해시 순서에서 현재 로컬 사제의 첫 후보만 고른다.
        for (int candidate = finder.Begin(new(-1000, -1000, 1000, 1000), 0); candidate != 0; candidate = finder.Next())
            if (Pool.Slot(candidate)[10] == GameAux.PriestTypeBox.Value && Pool.Slot(candidate)[34] == MissionPlayer.BattleOwner) { priest = candidate; break; }
        if (priest == 0) return; int created = FixedPacketNeed(_livePlayerHooks?.Create)(GameAux.PriestTypeBox.Value, 0); if (created == 0) throw new InvalidOperationException("개발자 getPriest의 원문 NULL 생성");
        // 첫 비로그인 Player의 WORD를 먼저 쓴 뒤 실제 파생 owner를 설정한다.
        for (uint owner = 1; owner < 9; owner++) if ((BinaryPrimitives.ReadUInt32LittleEndian(PlayerRecords.AsSpan((int)owner * 180 + 0x50)) & 1) == 0)
        {
            RawSquidWord.Set(Pool, created, (ushort)owner, () => BridgeState.BridgeType, sid => FixedPacketNeed(_fortBuilding).NotifySurface(sid)); SetWorldObjectOwner(created, owner); break;
        }
        var board = new RawBoardTarget(storage.BoardTargetState, line => storage.AssertDisplay("false", @"\Ns\O\Boardcoord.cpp", line));
        RawPathPoint point = board.Side(storage.NeighborsExecutor().Footprint(priest), 2, 4); PopWorldObject(created, point.X, point.Y, 0);
        FixedPacketNeed(_developerEffects?.Damage)(created, 10000, Pool.Slot(priest)[34], priest);
    }
}
