using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은현재 Wind/Host/SHP/실제Flyer예약과74h 상태 수신을 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession.cpp의 ConnectWindAviary/ReceiveAviaryPacket 중 Wind 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재Pool/권한/역할/clock을 빌리는 실제 Wind 본체다.</summary>
    private RawWindAviaryProcess? _windAviaryProcess;
    /// <summary>현재실제 첫nondead 프로세스 조회다.</summary>
    private RawContainedLookup? _windAviaryLookup;
    /// <summary>현재writer 종류와같은예약을 공유하는 전송분배다.</summary>
    private ProcessFormTransmission? _windAviaryPacketTransmission;
    /// <summary>SourceSession의실제건물139이며PE전역과별개인현재역할이다.</summary>
    public UIntBox WindAviaryAssetType { get; } = new() { Value = 139 };
    /// <summary>같은큰생산/홈통지도현재본체를빌린다. 반납뒤는실패한다.</summary>
    public RawWindAviaryProcess WindAviaryBody => FixedPacketNeed(_windAviaryProcess);
    /// <summary>같은현재Host가소유한Wind이미지만 실제종료/갱신에사용한다.</summary>
    private WindAviaryProcess CurrentWindAviary(WindAviaryProcessFields fields)
    {
        ushort form = unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(60)));
        if (PlayerProcesses.FromForm(form) is not WindAviaryProcess current || !ReferenceEquals(current.Fields, fields)) throw new InvalidOperationException("데빌메이커 현재Host/raw이미지 불일치"); return current;
    }
    /// <summary>같은현재SHP의실제프레임을읽고원주만자산번호범위를요구한다.</summary>
    private SquidDisplayFrame WindFrameHeader(ushort sid, int frame, bool assetOnly)
    {
        byte type = Pool.Slot(sid)[10]; IReadOnlyList<SquidDisplayShape> shapes = FixedPacketNeed(_terrainPopShapes);
        if ((assetOnly && type < 70) || type >= shapes.Count || frame < 0 || frame >= shapes[type].Frames.Count) throw new ArgumentOutOfRangeException(nameof(frame), "Wind 현재실제SHP헤더 범위"); return shapes[type].Frames[frame];
    }
    /// <summary>원문현재TYPE/프레임수의개발자검사를수행하고선택된미설치진단은실패한다.</summary>
    private void CheckWindAviaryFrame(ushort sid, int frame, uint line)
    {
        byte type = Pool.Slot(sid)[10]; OriginalTypeRecord record = SquidPacketType(type); if (type >= WorldFrames.Length) throw new ArgumentOutOfRangeException(nameof(sid), "Wind 현재프레임TYPE범위");
        uint multiplier = (record.Flags1 & 0x440000) != 0 ? 2u : 1u; if (frame < 0 || (ulong)frame >= (ulong)WorldFrames[type].Codes.Count * multiplier) throw new InvalidOperationException($"Wind 실제개발자프레임진단 미연결:{line:x}");
    }
    /// <summary>현재server/boss를구별하고같은Host/공간/SHP/역할의실제장치를조립한다.</summary>
    private void BindWindAviaryProcessDevices()
    {
        if (_windAviaryProcess is not null) return;
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage); WindAviaryHooks input = _squidPacketEffects?.WindAviary?.Copy() ?? new(); _windAviaryLookup = new(Pool, DestroyWorldObject);
        var globals = new WindAviaryGlobals(() => Gameplay.Player.FortMode, () => Gameplay.Authority.Server, () => Gameplay.Authority.Boss, () => Gameplay.World.Editor, () => PlayerProcessClock.Now, GameAux.WindAviaryProcessTypeBox, GameAux.WindFlyerTypeBox, WindAviaryAssetType) { FriendlyFire = new(() => GameAux.MissionSettings[3]), Checking = new(() => AiChecking.Read()) };
        _windAviaryProcess = new(Pool, globals, new()
        {
            FindProcess = (parent, type) => (ushort)FixedPacketNeed(_windAviaryLookup).Find(parent, type), Allocate = () => FixedPacketNeed(input.Allocate)(),
            Attach = (fields, parent, type) => { if (fields is null) throw new OutOfMemoryException("Wind NULL inner 부착의원본경계"); _ = PlayerProcesses.Attach(new WindAviaryProcess(WindAviaryBody, fields), type, parent, 8); },
            Damageable = (parent, flags) => { if (input.Damageable is { } selected) selected(parent, flags); else FixedPacketNeed(_vortexAssetCarrier).DamageablePostPop(parent, flags); },
            Assertion = line =>
            {
                if (input.Assertion is { } selected) { selected(line); return; } if (line != 0x275) throw new InvalidOperationException($"Wind 실제선택진단 미연결:{line:x}"); storage.AssertDisplay("!findProcess( getSid(), windAviaryProcessType, 0 )", @"\Ns\O\Flyer.cpp", line);
            },
            SetFrame = (parent, frame) => { if (input.SetFrame is { } selected) selected(parent, frame); else FixedPacketNeed(_liveFrame).Set(parent, frame, 0); },
            CheckFrame = (parent, frame, line) => { if (input.CheckFrame is { } selected) selected(parent, frame, line); else CheckWindAviaryFrame(parent, frame, line); },
            Create = type => input.Create is { } selected ? selected(type) : checked((ushort)FixedPacketNeed(_livePlayerHooks?.Create)(type, 0)),
            SetOwner = (sid, owner) => { if (input.SetOwner is { } selected) selected(sid, owner); else SetWorldObjectOwner(sid, owner); },
            Height = (sid, frame) => input.Height is { } selected ? selected(sid, frame) : WindFrameHeader(sid, frame, false).WalkStepY,
            FootY = sid => input.FootY is { } selected ? selected(sid) : (float)SquidPacketType(Pool.Slot(sid)[10]).FootY,
            Pop = (sid, x, y) => { if (input.Pop is { } selected) selected(sid, x, y); else PopWorldObject(sid, x, y, 0); },
            CreateFlyerProcess = CreateWindFlyerProcess, BindFlyer = (sid, home) => FlyerBody.Bind(sid, home),
            CirclePosition = (parent, flyer, fields) => RawWindCircle.Position(Pool, FixedPacketNeed(_terrainPopTypes), parent, flyer, fields, (sid, _, frame) => { SquidDisplayFrame header = WindFrameHeader(sid, frame, true); return new(header.WalkStepY, header.Height, header.HotspotY); }),
            Finish = fields => PlayerProcesses.Kill(CurrentWindAviary(fields), 0), Destroy = (sid, flags) => DestroyWorldObject(sid, flags),
        });
        BindWindAviaryPacketTransmission(storage);
    }
    /// <summary>실제104BYTE nullable할당과현재속도로같은Flyer예약을기본부착뒤Wind전문화한다.</summary>
    private void CreateWindFlyerProcess(ushort sid, ushort home)
    {
        byte[]? allocation = FixedPacketNeed(_squidPacketEffects?.FlyerAllocation)(104); if (allocation is null) return;
        if (allocation.Length != 104) throw new ArgumentException("Wind 실제104BYTE 할당이미지 필요");
        var fields = new FlyerProcessFields(); allocation.CopyTo(fields.Bytes, 0);
        if (_reservedFlyerProcess is not null) throw new InvalidOperationException("더스트 데빌 비행생성부착 재진입");
        var current = new FlyerProcess(FlyerBody, fields); _reservedFlyerProcess = current;
        try
        {
            float speed = FixedPacketNeed(_squidPacketEffects?.FlyerSpeed)(sid); FlyerBody.Construct(current.Fields, sid, home, BitConverter.SingleToUInt32Bits(speed), .5f); RawFlyerProcess.SpecializeWind(current.Fields);
        }
        catch
        {
            if (ReferenceEquals(_reservedFlyerProcess, current)) { _reservedFlyerProcess = null; current.Dispose(); } throw;
        }
    }
    /// <summary>Native현재2048BYTE/헤더와독립typed64BYTE/최초헤더를구별하고다른파생을보존한다.</summary>
    private void BindWindAviaryPacketTransmission(RawSurfaceStorage storage)
    {
        ProcessFormTransmission other = PlayerProcesses.Transmission()?.Clone() ?? new(); bool writing = false; byte[] buffer = storage.UsesNativeTransmissionDevices() ? GameAux.SquidTransmission.Buffer : new byte[64];
        // 같은실제writer의현재식별자/미작성tail은파생본문이기록한다.
        int Write(WindAviaryProcess process, bool changed, uint force) => storage.UsesNativeTransmissionDevices() ? storage.SerializeProcessForm(process, buffer, changed, force) : process.WritePacket(buffer, changed, force != 0);
        _windAviaryPacketTransmission = new()
        {
            WriteData = (process, changed) => { writing = process is WindAviaryProcess; return process is WindAviaryProcess wind ? Write(wind, changed, 0) : FixedPacketNeed(other.WriteData)(process, changed); },
            WriteFullData = (process, force) => { writing = process is WindAviaryProcess; return process is WindAviaryProcess wind ? Write(wind, false, force) : FixedPacketNeed(other.WriteFullData)(process, force); },
            Packet = length => { if (!writing) return FixedPacketNeed(other.Packet)(length); if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "Wind 실제작성버퍼 범위"); return buffer.AsSpan(0, length).ToArray(); },
            Transmit = length =>
            {
                if (!writing) { FixedPacketNeed(other.Transmit)(length); return; } if (storage.UsesNativeTransmissionDevices()) { storage.BroadcastProcessForm(length); return; }
                if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "Wind 실제packet 길이"); FixedPacketNeed(_squidPacketEffects?.FormPacket)(buffer.AsMemory(0, length));
            },
        };
        PlayerProcesses.SetFormTransmit(_windAviaryPacketTransmission);
    }
    /// <summary>0045e4f0의최초Form/free와진단뒤현재입력/서버/시각/역할을구별하는실제74h수신이다.</summary>
    private void ReceiveWindAviaryPacket(ReadOnlyMemory<byte> packet)
    {
        if (packet.Length < 9) throw new ArgumentOutOfRangeException(nameof(packet), "Wind 수신의실제9BYTE머리 부족"); uint number = BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[4..]);
        if (number == 0 || number > ushort.MaxValue || number >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "Wind 수신의명시FormDWORD 물리범위"); int form = (int)number; bool free = (Pool.Slot(form)[11] & 1) != 0;
        // 정상복귀뒤첫Form/free는유지하고현재mask/본문만다시읽는다.
        void Diagnose(string expression, uint line) => FixedPacketNeed(_squidPacketStorage).AssertDisplay(expression, @"c:\ns\o\_p_windaviaryprocessdata.cpp", line);
        if (free)
        {
            if (packet.Span[8] != 0xff) Diagnose("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", 0x4e); RawWindAviaryProcess logic = WindAviaryBody; WindAviaryProcess? pending = new(logic, new());
            try
            {
                RawWindAviaryState.Initialize(pending.Fields, Gameplay.Authority.Server, PlayerProcessClock.Now); uint parent = RawWindAviaryState.ReadProcess(pending.Fields, packet.Span);
                if (parent > ushort.MaxValue || parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "Wind 수신의DWORD부모SID 물리범위"); WindAviaryProcess owned = pending; pending = null; _ = PlayerProcesses.Attach(owned, GameAux.WindAviaryProcessTypeBox.Value, (int)parent, 0x10, form); return;
            }
            finally { pending?.Dispose(); }
        }
        if ((packet.Span[8] & 1) != 0) Diagnose("(*(char *)(( (char *)zacket+8) ) & 1) == 0", 0x56); WindAviaryProcess? current = PlayerProcesses.FromForm(form) as WindAviaryProcess; _ = RawWindAviaryState.ReadProcess(current is null ? Span<byte>.Empty : current.Fields.Bytes, packet.Span);
    }
    /// <summary>Kernel/Host정리뒤같은현재본문/조회/전송참조를반납한다.</summary>
    private void ReleaseWindAviaryProcessDevices() { _windAviaryProcess = null; _windAviaryLookup = null; _windAviaryPacketTransmission = null; }
}
