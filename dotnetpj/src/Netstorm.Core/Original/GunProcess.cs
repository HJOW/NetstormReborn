using System.Buffers.Binary;

// 같은 실제 Kernel/Form/부모를 Gun 원문 필드 이미지와 연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawGunProcess.cpp의 실제 inner Kernel 어댑터다.</summary>
public sealed class GunProcess : SquidProcess
{
    /// <summary>이 객체와 같은 Pool을 읽는 원문 실행 본체다.</summary>
    private readonly RawGunProcess _logic;
    /// <summary>객체가 값 복사해 소유하는 실제0x38 이미지다.</summary>
    public GunProcessFields Fields { get; }
    /// <summary>외부 이미지와 소유권을 분리하고 현재 실행 본체를 빌린다.</summary>
    public GunProcess(RawGunProcess logic, GunProcessFields fields) { _logic = logic; Fields = new(fields.Bytes); }
    /// <summary>현재 Kernel 가상 프레임은 같은 이미지에 직접 실행한다.</summary>
    public override void RunFrame() => _logic.Frame(Fields);
    /// <summary>writer/부착 통지가 공유하는 현재 inner Form/부모 DWORD만 쓴다.</summary>
    private void SyncIdentity()
    {
        BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(44), (uint)Form());
        BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(48), (uint)Parent());
    }
    /// <summary>원문 부착 뒤 통지에서만 대기3→2/6→5를 되돌린다.</summary>
    public override void OnAttached() { SyncIdentity(); RawGunProcess.Attached(Fields); }
    /// <summary>실제 ProcessForm 가상 메시지를 현재 원문 이미지에 전달한다.</summary>
    public override void Notify(uint ev, uint info, uint value) => _logic.Notify(Fields, ev, info, value);
    /// <summary>부착 중 OnAttached보다 앞서 실행되므로 식별자만 동기화하고 단계를 변경하지 않는다.</summary>
    public ushort WritePacket(Span<byte> packet, bool parentChanged, bool force, byte protocol = 1, byte message = 0x3a)
    {
        SyncIdentity(); return RawGunProcess.WriteProcess(Fields, packet, parentChanged, force, protocol, message);
    }
}

/// <summary>원문 flags0 부착/생성의 같은 Pool 계약과 실제 Kernel 소유권을 보존한다.</summary>
public sealed partial class RawGunProcess
{
    /// <summary>준비된 이미지를 같은 실제 Host의 flags0 부착에 넘긴다.</summary>
    public static GunProcess? AttachGunProcess(SquidProcessHost host, RawGunProcess logic, GunProcessFields fields, ushort parent, uint type = 34)
    {
        if (!ReferenceEquals(host.Pool(), logic.Pool)) throw new ArgumentException("GunProcess/Host 풀 불일치");
        return (GunProcess?)host.Attach(new GunProcess(logic, fields), type, parent, 0);
    }
    /// <summary>new0x38의 실제 생성 필드를 준비하고 같은 Host/Kernel에 부착한다.</summary>
    public static GunProcess? CreateGunProcess(SquidProcessHost host, RawGunProcess logic, ushort parent, uint type = 34)
    {
        if (!ReferenceEquals(host.Pool(), logic.Pool)) throw new ArgumentException("GunProcess/Host 풀 불일치");
        var fields = new GunProcessFields(); logic.Initialize(fields, parent); return AttachGunProcess(host, logic, fields, parent, type);
    }
}
