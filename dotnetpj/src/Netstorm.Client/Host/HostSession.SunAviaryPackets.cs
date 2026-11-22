using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 현재 훨리베이스/Host/공통 비행 연결과 실제38h 상태 수신을 조립한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession.cpp의 ConnectSunAviary/ReceiveAviaryPacket 중 Sun 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 현재 Pool/전역과 실제 장치를 사용하는 훨리베이스 본체다.</summary>
    private RawSunAviaryProcess? _sunAviaryProcess;
    /// <summary>현재 writer 선택과 같은 실제 예약을 공유하는 전송 분배다.</summary>
    private ProcessFormTransmission? _sunAviaryPacketTransmission;
    /// <summary>첫 nondead 현재 Form을 조회하는 같은 Pool의 본체다.</summary>
    private RawContainedLookup? _sunAviaryLookup;
    /// <summary>후속 실제 생산/홈 통지도 같은 현재 본체를 빌린다. 반납 뒤는 실패한다.</summary>
    public RawSunAviaryProcess SunAviaryBody => FixedPacketNeed(_sunAviaryProcess);
    /// <summary>현재 Form/PID가 소유한 같은48BYTE 이미지인지 확인한다.</summary>
    private SunAviaryProcess CurrentSunAviary(SunAviaryProcessFields fields)
    {
        if (PlayerProcesses.FromForm(fields.Form()) is not SunAviaryProcess current || !ReferenceEquals(current.Fields, fields)) throw new InvalidOperationException("훨리베이스 현재 Host/raw 이미지 불일치");
        return current;
    }
    /// <summary>현재 TYPE/프레임 수만 사용하며 선택된 미복원 진단 위치는 실패한다.</summary>
    private void CheckSunAviaryFrame(ushort sid, int frame, uint line)
    {
        byte type = Pool.Slot(sid)[10]; OriginalTypeRecord record = SquidPacketType(type);
        if (type >= WorldFrames.Length) throw new ArgumentOutOfRangeException(nameof(sid), "훨리베이스 현재 프레임 TYPE 범위");
        uint multiplier = (record.Flags1 & 0x440000) != 0 ? 2u : 1u;
        if (frame < 0 || (ulong)frame >= (ulong)WorldFrames[type].Codes.Count * multiplier) throw new InvalidOperationException($"훨리베이스 실제 개발자 프레임 진단 미연결:{line:x}");
    }
    /// <summary>현재 서버/Editor/시각/역할과 실제 Host/공간을 빌리며 큰 장치는 선택 시점에만 요구한다.</summary>
    private void BindSunAviaryProcessDevices()
    {
        if (_sunAviaryProcess is not null) return;
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage); SunAviaryProcessHooks input = _squidPacketEffects?.SunAviary?.Copy() ?? new();
        _sunAviaryLookup = new(Pool, DestroyWorldObject);
        var globals = new SunAviaryProcessGlobals(() => Gameplay.Authority.Server, () => Gameplay.World.Editor, () => PlayerProcessClock.Now, GameAux.SunAviaryProcessTypeBox, GameAux.SunFlyerTypeBox) { Checking = new(() => AiChecking.Read()) };
        _sunAviaryProcess = new(Pool, globals, new()
        {
            FindProcess = (parent, type) => (ushort)FixedPacketNeed(_sunAviaryLookup).Find(parent, type), Allocate = size => FixedPacketNeed(input.Allocate)(size),
            Attach = (fields, parent, type, flags) =>
            {
                if (fields is null) throw new OutOfMemoryException("훨리베이스 NULL inner 부착의 원본 경계");
                _ = PlayerProcesses.Attach(new SunAviaryProcess(SunAviaryBody, fields), type, parent, flags);
            },
            Damageable = (parent, flags) => { if (input.Damageable is { } selected) selected(parent, flags); else FixedPacketNeed(_vortexAssetCarrier).DamageablePostPop(parent, flags); },
            Assertion = line =>
            {
                if (input.Assertion is { } selected) { selected(line); return; }
                if (line != 0x1e2) throw new InvalidOperationException($"훨리베이스 실제 선택 진단 미연결:{line:x}");
                storage.AssertDisplay("!findProcess( getSid(), sunAviaryProcessType, 0 )", @"\Ns\O\Flyer.cpp", line);
            },
            SetFrame = (parent, frame) => { if (input.SetFrame is { } selected) selected(parent, frame); else FixedPacketNeed(_liveFrame).Set(parent, frame, 0); },
            Sound = (x, y, name) => { if (input.Sound is { } selected) selected(x, y, name); else FixedPacketNeed(_squidPacketEffects?.PlayAtFile)(x, y, name, 0); },
            CheckFrame = (parent, frame, line) => { if (input.CheckFrame is { } selected) selected(parent, frame, line); else CheckSunAviaryFrame(parent, frame, line); },
            Create = (type, flags) => input.Create is { } selected ? selected(type, flags) : checked((ushort)FixedPacketNeed(_livePlayerHooks?.Create)(type, flags)),
            SetOwner = (sid, owner) => { if (input.SetOwner is { } selected) selected(sid, owner); else SetWorldObjectOwner(sid, owner); },
            Pop = (sid, x, y, flags) => { if (input.Pop is { } selected) selected(sid, x, y, flags); else PopWorldObject(sid, x, y, flags); },
            BindFlyer = (sid, home) => FlyerBody.Bind(sid, home), RefreshForm = fields => PlayerProcesses.RefreshForm(CurrentSunAviary(fields)),
            ProcessFields = form => (PlayerProcesses.FromForm(form) as SunAviaryProcess)?.Fields,
            Unpop = (sid, flags) => { if (input.Unpop is { } selected) selected(sid, flags); else UnpopWorldObject(sid, flags); },
            Repop = (sid, flags) => { if (input.Repop is { } selected) selected(sid, flags); else RepopWorldObject(sid, flags); },
        });
        BindSunAviaryPacketTransmission(storage);
    }
    /// <summary>현재 Native2048BYTE/헤더와 독립typed64BYTE/최초헤더를 구별하고 다른 파생 전송을 보존한다.</summary>
    private void BindSunAviaryPacketTransmission(RawSurfaceStorage storage)
    {
        ProcessFormTransmission other = PlayerProcesses.Transmission()?.Clone() ?? new(); bool writing = false; byte[] buffer = storage.UsesNativeTransmissionDevices() ? GameAux.SquidTransmission.Buffer : new byte[64];
        // 현재 실제 writer만 선택하며 같은 Form/부모와 tail은 파생 본체가 기록한다.
        int Write(SunAviaryProcess process, bool changed, uint force) => storage.UsesNativeTransmissionDevices() ? storage.SerializeProcessForm(process, buffer, changed, force) : process.WritePacket(buffer, changed, force != 0);
        _sunAviaryPacketTransmission = new()
        {
            WriteData = (process, changed) => { writing = process is SunAviaryProcess; return process is SunAviaryProcess aviary ? Write(aviary, changed, 0) : FixedPacketNeed(other.WriteData)(process, changed); },
            WriteFullData = (process, force) => { writing = process is SunAviaryProcess; return process is SunAviaryProcess aviary ? Write(aviary, false, force) : FixedPacketNeed(other.WriteFullData)(process, force); },
            Packet = length =>
            {
                if (!writing) return FixedPacketNeed(other.Packet)(length);
                if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "훨리베이스 실제 작성 버퍼 범위"); return buffer.AsSpan(0, length).ToArray();
            },
            Transmit = length =>
            {
                if (!writing) { FixedPacketNeed(other.Transmit)(length); return; }
                if (storage.UsesNativeTransmissionDevices()) { storage.BroadcastProcessForm(length); return; }
                if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "훨리베이스 실제 packet 길이"); FixedPacketNeed(_squidPacketEffects?.FormPacket)(buffer.AsMemory(0, length));
            },
        };
        PlayerProcesses.SetFormTransmit(_sunAviaryPacketTransmission);
    }
    /// <summary>0045e410의 최초Form/free와 진단 이후 현재 입력/역할을 구별하는 실제38h 수신이다.</summary>
    private void ReceiveSunAviaryPacket(ReadOnlyMemory<byte> packet)
    {
        if (packet.Length < 9) throw new ArgumentOutOfRangeException(nameof(packet), "아비어리 수신의 실제9BYTE 머리 부족");
        uint number = BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[4..]);
        if (number == 0 || number > ushort.MaxValue || number >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "아비어리 수신의 명시 Form DWORD 물리 범위");
        int form = (int)number; bool free = (Pool.Slot(form)[11] & 1) != 0;
        // 진단이 복귀해도 최초Form/free는 유지하며 현재mask와 선택필드만 다시 읽는다.
        void Diagnose(string expression, uint line) => FixedPacketNeed(_squidPacketStorage).AssertDisplay(expression, @"c:\ns\o\_p_sunaviaryprocessdata.cpp", line);
        if (free)
        {
            if (packet.Span[8] != 0xff) Diagnose("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", 0x3c);
            RawSunAviaryProcess logic = SunAviaryBody; SunAviaryProcess? pending = new(logic, new());
            try
            {
                logic.Initialize(pending.Fields); uint parent = RawAviaryProcessData.Read(pending.Fields.Bytes, packet.Span);
                if (parent > ushort.MaxValue || parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "아비어리 수신의 DWORD 부모 SID 물리 범위");
                SunAviaryProcess owned = pending; pending = null; _ = PlayerProcesses.Attach(owned, GameAux.SunAviaryProcessTypeBox.Value, (int)parent, 0x10, form); return;
            }
            finally { pending?.Dispose(); }
        }
        if ((packet.Span[8] & 1) != 0) Diagnose("(*(char *)(( (char *)zacket+8) ) & 1) == 0", 0x44);
        _ = RawAviaryProcessData.Read(CurrentAviaryFields(form).Span, packet.Span);
    }
    /// <summary>Kernel/Host 반납 뒤 같은 본체/조회/전송 참조만 해제한다.</summary>
    private void ReleaseSunAviaryProcessDevices() { _sunAviaryProcess = null; _sunAviaryLookup = null; _sunAviaryPacketTransmission = null; }
}
