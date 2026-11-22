using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 실제 비행 본체/Host/현재 owner로39h 상태와86h 기한 명령을 수신한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession.cpp의 비행 수신/현재 객체 선택과 전송 분배 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 실제 Pool/현재 TYPE를 빌리는 공통 비행 본체다.</summary>
    private RawFlyerProcess? _flyerProcess;
    /// <summary>현재 비행 writer 종류와 같은 예약을 공유하는 전송 분배다.</summary>
    private ProcessFormTransmission? _flyerPacketTransmission;
    /// <summary>현재 실제 nondead Form 조회 본체다.</summary>
    private RawContainedLookup? _flyerProcessLookup;
    /// <summary>실제생성자가현재Attach에서소비할같은할당예약객체다.</summary>
    private FlyerProcess? _reservedFlyerProcess;
    /// <summary>같은 큰 Frame/후속 생산도 현재 실제 본체를 빌린다. 반납 뒤는 실패한다.</summary>
    public RawFlyerProcess FlyerBody => FixedPacketNeed(_flyerProcess);
    /// <summary>선택한 DWORD만 원문 소비 위치에서 읽고 실제 물리 부족을 거부한다.</summary>
    private static uint FlyerPacketDword(ReadOnlyMemory<byte> packet, int offset)
    {
        if (offset > packet.Length || 4 > packet.Length - offset) throw new ArgumentOutOfRangeException(nameof(packet), "비행 패킷의 선택된 DWORD 부족");
        return BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[offset..]);
    }
    /// <summary>Source 세 서버/genus/processSid 진단만 실제 Client 진단에 전달한다.</summary>
    private void DiagnoseFlyerBody(uint line)
    {
        string expression = line switch { 0x2e3 or 0x41d => "server", 0x904 or 0x8ee => "flyer.getGenus() & gFLYER", 0x8f5 => "processSid", _ => throw new ArgumentOutOfRangeException(nameof(line), "비행의 다른 진단 위치") };
        FixedPacketNeed(_squidPacketStorage).AssertDisplay(expression, @"\Ns\O\Flyer.cpp", line);
    }
    /// <summary>같은 현재 서버/시각/TYPE/checking과 실제 Host 조회만 빌리며 큰 장치는 미리 실행하지 않는다.</summary>
    private void BindFlyerProcessDevices()
    {
        if (_flyerProcess is not null) return;
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage); FlyerProcessHooks input = _squidPacketEffects?.Flyer?.Copy() ?? new();
        _flyerProcessLookup = new(Pool, DestroyWorldObject);
        var globals = new FlyerProcessGlobals(() => Gameplay.Authority.Server, () => PlayerProcessClock.Now, GameAux.FlyerProcessTypeBox) { Checking = new(() => AiChecking.Read()) };
        _flyerProcess = new(Pool, FixedPacketNeed(_terrainPopTypes), globals, new()
        {
            Assertion = DiagnoseFlyerBody, FindProcess = (parent, type) => (ushort)FixedPacketNeed(_flyerProcessLookup).Find(parent, type),
            Fields = form => (PlayerProcesses.FromForm(form) as FlyerProcess ?? throw new InvalidOperationException("비행 Form의 현재 PID/원문 미연결")).Fields,
            Attach = AttachReservedFlyerProcess,
            Frame = fields => FixedPacketNeed(input.Frame)(fields), Notify = (fields, message, first, second) => FixedPacketNeed(input.Notify)(fields, message, first, second),
            ParentDying = fields => FixedPacketNeed(input.ParentDying)(fields), Pieces = parent => FixedPacketNeed(input.Pieces)(parent), Destroy = (parent, flags) => DestroyWorldObject(parent, flags),
        });
        BindFlyerPacketTransmission(storage);
    }
    /// <summary>현재예약과같은이미지만같은Host로소유권을넘기며단독생성호출은거부한다.</summary>
    private void AttachReservedFlyerProcess(FlyerProcessFields fields, ushort parent, uint type, uint flags)
    {
        FlyerProcess current = _reservedFlyerProcess ?? throw new InvalidOperationException("비행 생성자의 실제 할당 예약 미연결");
        if (!ReferenceEquals(current.Fields, fields)) throw new InvalidOperationException("비행 생성의 현재예약/원문이미지 불일치");
        _reservedFlyerProcess = null;
        if (PlayerProcesses.Attach(current, type, parent, flags) is null) throw new InvalidOperationException("비행 생성의 실제Host 부분부착실패 경계");
    }
    /// <summary>Native는 같은2048BYTE/현재 헤더를 사용하고 typed는 기존64BYTE/최초 헤더와 다른 파생 전송을 보존한다.</summary>
    private void BindFlyerPacketTransmission(RawSurfaceStorage storage)
    {
        ProcessFormTransmission other = PlayerProcesses.Transmission()?.Clone() ?? new(); bool writingFlyer = false;
        byte[] buffer = storage.UsesNativeTransmissionDevices() ? GameAux.SquidTransmission.Buffer : new byte[64];
        // 같은 실제 writer만 선택하고 현재 Form/부모와 미작성 tail은 파생 본체가 처리한다.
        int Write(FlyerProcess process, bool changed, uint force) => storage.UsesNativeTransmissionDevices()
            ? storage.SerializeProcessForm(process, buffer, changed, force) : process.WritePacket(buffer, changed, force != 0);
        _flyerPacketTransmission = new()
        {
            WriteData = (process, changed) => { writingFlyer = process is FlyerProcess; return process is FlyerProcess flyer ? Write(flyer, changed, 0) : FixedPacketNeed(other.WriteData)(process, changed); },
            WriteFullData = (process, force) => { writingFlyer = process is FlyerProcess; return process is FlyerProcess flyer ? Write(flyer, false, force) : FixedPacketNeed(other.WriteFullData)(process, force); },
            Packet = length =>
            {
                if (!writingFlyer) return FixedPacketNeed(other.Packet)(length);
                if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "비행 실제 작성 버퍼 범위"); return buffer.AsSpan(0, length).ToArray();
            },
            Transmit = length =>
            {
                if (!writingFlyer) { FixedPacketNeed(other.Transmit)(length); return; }
                if (storage.UsesNativeTransmissionDevices()) { storage.BroadcastProcessForm(length); return; }
                if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "비행 실제 packet 길이"); FixedPacketNeed(_squidPacketEffects?.FormPacket)(buffer.AsMemory(0, length));
            },
        };
        PlayerProcesses.SetFormTransmit(_flyerPacketTransmission);
    }
    /// <summary>부모 DWORD를 WORD로 줄이기 전에 같은 실제 Pool 범위만 확인한다. 부모0은 원문처럼 허용한다.</summary>
    private int FlyerParentSid(uint value)
    {
        if (value > ushort.MaxValue || value >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(value), "비행 수신/명령의 DWORD 부모 SID 물리 범위"); return (int)value;
    }
    /// <summary>0045e2b0의 새 객체 임시Sun/현재theme/두 번째 생성·재해석과 기존 PID 갱신이다.</summary>
    private void ReceiveFlyerPacket(ReadOnlyMemory<byte> packet)
    {
        if (packet.Length < 9) throw new ArgumentOutOfRangeException(nameof(packet), "비행 수신의 실제9BYTE 머리 부족");
        uint number = FlyerPacketDword(packet, 4);
        if (number == 0 || number > ushort.MaxValue || number >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "비행 수신의 명시 Form DWORD 물리 범위");
        int form = (int)number; bool free = (Pool.Slot(form)[11] & 1) != 0;
        // 진단이 정상 복귀하면 최초 Form/free는 유지하고 현재 mask와 본문만 다시 읽는다.
        void Diagnose(string expression, uint line) => FixedPacketNeed(_squidPacketStorage).AssertDisplay(expression, @"c:\ns\o\_p_baseflyerprocessdata.cpp", line);
        if (free)
        {
            if (packet.Span[8] != 0xff) Diagnose("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", 0x60);
            RawFlyerProcess logic = FlyerBody; FlyerProcess? probe = new(logic, new()); FlyerProcess? pending = null;
            try
            {
                RawFlyerProcess.InitializeReceived(probe.Fields, 2); uint firstParent = RawFlyerProcess.ReadData(probe.Fields, packet.Span);
                int parentSid = FlyerParentSid(firstParent); byte type = Pool.Slot(parentSid)[10]; int theme = SquidPacketType(type).Theme;
                pending = new(logic, new()); RawFlyerProcess.InitializeReceived(pending.Fields, theme); RawFlyerProcess.DestructFields(probe.Fields); probe.Dispose(); probe = null;
                uint parent = RawFlyerProcess.ReadData(pending.Fields, packet.Span); int into = FlyerParentSid(parent);
                FlyerProcess owned = pending; pending = null; _ = PlayerProcesses.Attach(owned, GameAux.FlyerProcessTypeBox.Value, into, 0x10, form); return;
            }
            finally { probe?.Dispose(); pending?.Dispose(); }
        }
        if ((packet.Span[8] & 1) != 0) Diagnose("(*(char *)(( (char *)zacket+8) ) & 1) == 0", 0x76);
        FlyerProcess current = PlayerProcesses.FromForm(form) as FlyerProcess ?? throw new InvalidOperationException("비행 수신의 현재 PID/outer 미발견");
        _ = RawFlyerProcess.ReadData(current.Fields, packet.Span);
    }
    /// <summary>현재TYPE/최초Form을 유지하고 진단 복귀 뒤 그 Form의 현재 PID/실제 이미지에 접근한다.</summary>
    private FlyerProcessFields CurrentFlyerProcess(uint parent, uint line)
    {
        uint type = GameAux.FlyerProcessTypeBox.Value; int parentSid = FlyerParentSid(parent); int form = FixedPacketNeed(_flyerProcessLookup).Find(parentSid, type);
        if (form == 0) FixedPacketNeed(_squidPacketStorage).AssertDisplay("processSid", @"\Ns\O\Flyer.cpp", line);
        return (PlayerProcesses.FromForm(form) as FlyerProcess ?? throw new InvalidOperationException("비행 명령의 현재Form PID에 대한 실제 outer 미연결")).Fields;
    }
    /// <summary>00426490은 먼저command DWORD만 읽고1/2/3에서만parent를 읽어 현재 기한을 변경한다.</summary>
    private void ReceiveFlyerCommand(ReadOnlyMemory<byte> packet)
    {
        uint command = FlyerPacketDword(packet, 12); uint line = command switch { 1 => 0xda, 2 => 0xe4, 3 => 0xee, _ => 0 };
        if (line == 0) return; FlyerProcessFields fields = CurrentFlyerProcess(FlyerPacketDword(packet, 4), line); FlyerBody.Command(fields, command);
    }
    /// <summary>실제 Kernel/Host 정리 뒤에 같은 비행 본체/조회/전송 참조만 반납한다.</summary>
    private void ReleaseFlyerProcessDevices() { _reservedFlyerProcess?.Dispose(); _reservedFlyerProcess = null; _flyerProcess = null; _flyerProcessLookup = null; _flyerPacketTransmission = null; }
}
