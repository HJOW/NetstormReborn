using System.Buffers.Binary;

// 건설 진행의 실제0x37 상태 codec과 현재 Form/Kernel 수신을 같은 물리에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>현재 원문 두 BYTE를 전송 호출에 값으로 공급하는 헤더다.</summary>
public readonly struct ConstructionDataHeader
{
    /// <summary>PATCH005325b9/CD0053fc30의 최초1인 protocol BYTE다.</summary>
    public byte Protocol { get; init; }
    /// <summary>PATCH005325b8/CD0053fc2c의 최초0x37인 message BYTE다.</summary>
    public byte Message { get; init; }
    /// <summary>원문 값 구조체의 최초1/37을 준비한다. 호출자 initializer로 현재 두 BYTE를 지정할 수 있다.</summary>
    public ConstructionDataHeader() { Protocol = 1; Message = 0x37; }
}

/// <summary>Source RawConstructionData.h의 실제 새 객체/부착/현재 PID 조회 장치다.</summary>
public sealed class ConstructionDataHooks
{
    /// <summary>new0x38의 실제 필드 저장소를 확보한다. 반환null은 할당 실패다.</summary>
    public Func<ConstructionProcessFields?>? Allocate { get; set; }
    /// <summary>필드·현재 TYPE·부모 DWORD·최초 캡처 Form·flags10을 실제 부착 부모에 넘긴다.</summary>
    public Action<ConstructionProcessFields, uint, uint, uint, uint>? Attach { get; set; }
    /// <summary>실제 Host가 없을 때 현재 PID DWORD의 건설 필드를 조회한다.</summary>
    public Func<uint, ConstructionProcessFields?>? Lookup { get; set; }
    /// <summary>원문 값형 함수 묶음처럼 슬롯만 복사하고 장치는 실행하지 않는다.</summary>
    internal ConstructionDataHooks Copy() => new() { Allocate = Allocate, Attach = Attach, Lookup = Lookup };
}

/// <summary>cpppj/src/o/RawConstructionData.cpp의 현재 비트·부분 효과·실제 프로세스 소유권을 이식한다.</summary>
public sealed class RawConstructionData
{
    /// <summary>부모4·시작8·끝8·추가1과 헤더9를 모두 포함한 최대 길이다.</summary>
    public const int MaximumSize = 30;
    /// <summary>선택 비트까지 포함하는 실제 최소 헤더 길이다.</summary>
    private const int HeaderSize = 9;
    /// <summary>부모 링크와 현재 free/PID를 가진 같은 실제 Pool이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>할당/읽기 뒤 현재 TYPE를 다시 조회하는 상태다.</summary>
    private readonly ConstructionProcessState _state;
    /// <summary>실제 Host 없는 수신에서만 선택하는 값형 장치 사본이다.</summary>
    private readonly ConstructionDataHooks _hooks;
    /// <summary>있으면 현재 실제 Form→PID→프로세스 조회와 부착을 담당한다.</summary>
    private readonly SquidProcessHost? _host;
    /// <summary>실제 새 수신 객체가 빌리는 건설 진행/완료 본체다.</summary>
    private readonly RawConstructionProcess? _logic;
    /// <summary>원문 진단의 정상 복귀와 미설치를 구별하는 같은 Client 장치다.</summary>
    private readonly Action<string, string, uint>? _assertion;

    /// <summary>명시한 저장소/효과만 보관하며 같은 Pool Host와 필수 최초 장치를 검증한다.</summary>
    public RawConstructionData(RawSidPool pool, ConstructionProcessState state, ConstructionDataHooks hooks,
        SquidProcessHost? host = null, Action<string, string, uint>? assertion = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Allocate is null || hooks.Attach is null || (host is null && hooks.Lookup is null) || (host is not null && !ReferenceEquals(host.Pool(), pool)))
            throw new ArgumentException("건설 상태 수신의 저장소/효과 연결 오류");
        _pool = pool; _state = state; _hooks = hooks.Copy(); _host = host; _assertion = assertion;
    }
    /// <summary>새/기존 수신을 같은 실제 건설·Host에 연결하며 다른 Pool의 소유권 혼합을 거부한다.</summary>
    public RawConstructionData(RawSidPool pool, ConstructionProcessState state, RawConstructionProcess logic,
        SquidProcessHost host, Action<string, string, uint>? assertion = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(logic); ArgumentNullException.ThrowIfNull(host);
        if (!ReferenceEquals(logic.Pool, pool) || !ReferenceEquals(host.Pool(), pool)) throw new ArgumentException("건설 상태 수신의 실제 Pool 불일치");
        _pool = pool; _state = state; _hooks = new(); _host = host; _logic = logic; _assertion = assertion;
    }

    /// <summary>실제 물리 길이만 요구하고 선언 WORD나 헤더 종류를 자격 검사로 추가하지 않는다.</summary>
    private static void Require(int actual, int needed)
    {
        if (actual < needed) throw new ArgumentException("건설 상태 패킷 길이 부족");
    }
    /// <summary>선택된 저장 직전 폭을 먼저 검사해 앞선 필드 쓰기를 보존한다.</summary>
    private static ConstructionProcessFields NeedFields(ConstructionProcessFields? fields, int actual, int needed)
    {
        Require(actual, needed); return fields ?? throw new ArgumentException("건설 상태의 실제 필드 누락");
    }
    /// <summary>현재 mask가 선택한 시각/추가 바이트만 길이에 포함한다. 상위 비트는 payload를 만들지 않는다.</summary>
    private static int FieldSize(byte mask) => ((mask & 2) != 0 ? 8 : 0) + ((mask & 4) != 0 ? 8 : 0) + ((mask & 8) != 0 ? 1 : 0);

    /// <summary>004426d0의 실제 필드 비트/부모/force와 미작성 tail을 유지하며 정상 끝에서만 dirty를0으로 만든다.</summary>
    public static ushort Write(ConstructionProcessFields fields, Span<byte> packet, uint form, uint parent, bool force, ConstructionDataHeader? header = null)
    {
        byte mask = force ? (byte)0xff : fields.Dirty;
        Require(packet.Length, HeaderSize + (parent != 0 ? 4 : 0) + FieldSize(mask));
        if (force) fields.Dirty = 0xff;
        packet[0] = header?.Protocol ?? 1; packet[1] = header?.Message ?? 0x37;
        BinaryPrimitives.WriteUInt32LittleEndian(packet[4..], form); int cursor = HeaderSize;
        if (parent != 0) { fields.Dirty |= 1; BinaryPrimitives.WriteUInt32LittleEndian(packet[cursor..], parent); cursor += 4; }
        if ((fields.Dirty & 2) != 0) { BinaryPrimitives.WriteUInt64LittleEndian(packet[cursor..], BitConverter.DoubleToUInt64Bits(fields.Start)); cursor += 8; }
        if ((fields.Dirty & 4) != 0) { BinaryPrimitives.WriteUInt64LittleEndian(packet[cursor..], BitConverter.DoubleToUInt64Bits(fields.End)); cursor += 8; }
        if ((fields.Dirty & 8) != 0) packet[cursor++] = fields.Additional;
        BinaryPrimitives.WriteUInt16LittleEndian(packet[2..], (ushort)cursor); packet[8] = fields.Dirty; fields.Dirty = 0; return (ushort)cursor;
    }
    /// <summary>00443bd0은 includeParent 또는 force일 때만 실제 부모 DWORD를 작성기에 넘긴다.</summary>
    public static ushort WriteProcess(ConstructionProcess process, Span<byte> packet, bool includeParent, bool force, ConstructionDataHeader? header = null) =>
        Write(process.Fields, packet, unchecked((uint)process.Form()), includeParent || force ? unchecked((uint)process.Parent()) : 0, force, header);

    /// <summary>00442760은 선택된 입력만 복원하고 부모 DWORD를 반환한다. dirty/소리/선택되지 않은 필드는 보존한다.</summary>
    public static uint Read(ConstructionProcessFields? fields, ReadOnlySpan<byte> packet)
    {
        Require(packet.Length, HeaderSize); byte mask = packet[8]; uint parent = 0; int cursor = HeaderSize;
        if ((mask & 1) != 0) { Require(packet.Length, cursor + 4); parent = BinaryPrimitives.ReadUInt32LittleEndian(packet[cursor..]); cursor += 4; }
        if ((mask & 2) != 0) { NeedFields(fields, packet.Length, cursor + 8).Start = BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(packet[cursor..])); cursor += 8; }
        if ((mask & 4) != 0) { NeedFields(fields, packet.Length, cursor + 8).End = BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(packet[cursor..])); cursor += 8; }
        if ((mask & 8) != 0) NeedFields(fields, packet.Length, cursor + 1).Additional = packet[cursor];
        return parent;
    }

    /// <summary>004427b0의 최초 Form 캡처와 할당 뒤 현재 packet/TYPE 재조회를 보존하며 실제 Host 소유 새 수신도 수행한다.</summary>
    public void Receive(ReadOnlyMemory<byte> packet)
    {
        Require(packet.Length, HeaderSize); uint number = BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[4..]); byte mask = packet.Span[8];
        if (number == 0 || number >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "건설 상태 Form 번호 오류");
        int form = (ushort)number; ReadOnlySpan<byte> raw = _pool.Slot(form);
        if ((raw[11] & 1) != 0)
        {
            if (mask != 0xff) Assert("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", 0x42);
            ConstructionProcess? owned = _logic is not null ? new(_logic, new()) : null;
            ConstructionProcessFields fields = owned?.Fields ?? _hooks.Allocate!() ?? throw new OutOfMemoryException("건설 상태 수신의 실제 new0x38 실패");
            fields.Dirty = 0xff; fields.Start = 0; fields.End = 0;
            uint parent = Read(fields, packet.Span);
            if (owned is not null)
            {
                if (parent > ushort.MaxValue || parent >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "수신한 건설 부모 DWORD 범위 오류");
                _host!.Attach(owned, _state.Type(), (ushort)parent, 0x10, form);
            }
            else _hooks.Attach!(fields, _state.Type(), parent, number, 0x10);
            return;
        }
        if ((mask & 1) != 0) Assert("(*(char *)(( (char *)zacket+8) ) & 1) == 0", 0x4a);
        ConstructionProcessFields? current = _host is not null ? (_host.FromForm(form) as ConstructionProcess)?.Fields
            : _hooks.Lookup!(BinaryPrimitives.ReadUInt32LittleEndian(raw[18..]));
        _ = Read(current, packet.Span);
    }
    /// <summary>같은 판본별 파일/줄의 실제 진단이 정상 복귀하면 다음 수신 명령을 계속한다.</summary>
    private void Assert(string expression, uint line)
    {
        (_assertion ?? throw new InvalidOperationException("건설 상태의 실제 진단 장치 미연결"))(expression,
            _pool.Edition == OriginalEdition.Patch1078 ? @"c:\ns\o\_p_constructionprocessdata.cpp" : @"\Ns\o\_p_constructionprocessdata.cpp", line);
    }
}
