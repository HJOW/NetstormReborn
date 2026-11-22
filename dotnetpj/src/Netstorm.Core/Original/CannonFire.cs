using System.Buffers.Binary;

// 실제캐논발사파생의같은Host/Kernel/통지/직렬화수명을연결한다.
namespace Netstorm.Core.Original;

/// <summary>현재원문64BYTE와실제공통Fire본문을소유하는파생프로세스다.</summary>
public sealed class CannonFire : SquidProcess
{
    /// <summary>같은실제풀의장기실행본문이다.</summary>
    private readonly RawCannonFire _logic;
    /// <summary>외부이미지를값복사해소유한다.</summary>
    public GunAnimationFields Fields { get; }
    /// <summary>현재본문을빌리고원문필드를복사한다.</summary>
    public CannonFire(RawCannonFire logic, GunAnimationFields fields) { _logic = logic; Fields = new(fields.Bytes); }
    /// <summary>같은Kernel이원문공통발사본문을실행한다.</summary>
    public override void RunFrame() => _logic.Frame(Fields);
    /// <summary>실제부착/전송이함께읽는현재식별자DWORD만쓴다.</summary>
    private void SyncIdentity() { BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(4), (uint)Form()); BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(8), (uint)Parent()); }
    /// <summary>실제부착뒤현재Form/부모를동기화한다.</summary>
    public override void OnAttached() => SyncIdentity();
    /// <summary>현재실제VT와같은공통codec에현재식별자/헤더를전달한다.</summary>
    public ushort WritePacket(Span<byte> packet, bool changed, bool force = false, byte protocol = 1, byte? message = null) { SyncIdentity(); return RawGunAnimation.WriteProcess(Fields.Bytes, packet, changed, force, message ?? (RawGunAnimation.UsesFirePacket(Fields.Bytes) ? (byte)0x3b : (byte)0x3c), protocol); }
}
/// <summary>같은실제Host의현재Gun통지와파생CannonFire부착/삭제를연결한다.</summary>
public sealed partial class RawCannonFire
{
    /// <summary>현재Gun가상통지와실제CannonFire Kill만교체한다.</summary>
    public static GunAnimationHooks MakeHostHooks(SquidProcessHost host, GunAnimationHooks hooks)
    {
        GunAnimationHooks current = hooks.Copy(); current.Notify = (form, ev, info, value) => (host.FromForm(form) as GunProcess ?? throw new InvalidOperationException("캐논 완료현재Gun 미발견")).Notify(ev, info, value); current.Kill = (form, flags) => host.Kill(host.FromForm(form) as CannonFire ?? throw new InvalidOperationException("캐논 완료현재PID 미발견"), flags); return current;
    }
    /// <summary>같은Pool/현재Gun/현재부모역할만원문35역할·flags10으로실제부착한다.</summary>
    public static CannonFire? Create(SquidProcessHost host, RawCannonFire logic, ushort gunForm, ushort target)
    {
        if (!ReferenceEquals(host.Pool(), logic.Pool)) throw new ArgumentException("캐논 발사 Host/Pool 불일치"); GunProcess? gun = host.FromForm(gunForm) as GunProcess; if (gun is null || host.Pool().Slot(gun.Parent())[10] != logic.Type()) throw new InvalidOperationException("캐논 발사 공장현재부모 오류"); var fields = new GunAnimationFields(); logic.Initialize(fields, gunForm, target); return host.Attach(new CannonFire(logic, fields), host.AnimationProcessType(true), gun.Parent(), 0x10) as CannonFire;
    }
}
