using System.Buffers.Binary;

// 실제아처파생의같은Host/Kernel수명과Gun통지를연결한다.
namespace Netstorm.Core.Original;

/// <summary>원문회전/발사의88BYTE이미지를소유하는실제SquidProcess다.</summary>
public sealed class ArcherAnimation : SquidProcess
{
    /// <summary>이객체보다오래사는같은풀의실행본체다.</summary>
    private readonly RawArcherAnimation _logic;
    /// <summary>생성때선택한실제Fire파생여부다.</summary>
    private readonly bool _fire;
    /// <summary>외부이미지와분리해소유하는원문필드다.</summary>
    public ArcherAnimationFields Fields { get; }
    /// <summary>실제본문을빌리고원문이미지를값복사한다.</summary>
    public ArcherAnimation(RawArcherAnimation logic, ArcherAnimationFields fields, bool fire) { _logic = logic; Fields = new(fields.Bytes); _fire = fire; }
    /// <summary>같은Kernel이현재회전/발사본문을선택한다.</summary>
    public override void RunFrame() { if (_fire) _logic.Fire(Fields); else _logic.Rotate(Fields); }
    /// <summary>원문부착/전송이같이읽는현재Form/부모DWORD만쓴다.</summary>
    private void SyncIdentity() { BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(4), (uint)Form()); BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(8), (uint)Parent()); }
    /// <summary>실제Host부착뒤현재번호를동기화한다.</summary>
    public override void OnAttached() => SyncIdentity();
    /// <summary>같은원문이미지와실제VT/현재헤더를공통codec에전달한다.</summary>
    public ushort WritePacket(Span<byte> packet, bool changed, bool force = false, byte protocol = 1, byte? message = null) { SyncIdentity(); return RawGunAnimation.WriteProcess(Fields.Bytes, packet, changed, force, message ?? (RawGunAnimation.UsesFirePacket(Fields.Bytes) ? (byte)0x3b : (byte)0x3c), protocol); }
}
/// <summary>현재Gun과Archer수명을같은실제Host에연결한다.</summary>
public sealed partial class RawArcherAnimation
{
    /// <summary>현재Gun가상통지와실제Archer삭제만교체하고다른함수를보존한다.</summary>
    public static ArcherAnimationHooks MakeHostHooks(SquidProcessHost host, ArcherAnimationHooks hooks)
    {
        ArcherAnimationHooks current = hooks.Copy(); current.Common.Notify = (form, ev, info, value) => (host.FromForm(form) as GunProcess ?? throw new InvalidOperationException("아처 완료현재Gun 미발견")).Notify(ev, info, value); current.Common.Kill = (form, flags) => host.Kill(host.FromForm(form) as ArcherAnimation ?? throw new InvalidOperationException("아처 완료현재PID 미발견"), flags); return current;
    }
    /// <summary>같은Gun/현재부모/단계만실제flags10부착뒤Fire생성후단을실행한다.</summary>
    public static ArcherAnimation? Create(SquidProcessHost host, RawArcherAnimation logic, uint stage, ushort gunForm, ushort target)
    {
        if (!ReferenceEquals(host.Pool(), logic.Pool)) throw new ArgumentException("아처/Host 풀 불일치"); GunProcess? gun = host.FromForm(gunForm) as GunProcess; if (gun is null || host.Pool().Slot(gun.Parent())[10] != logic.Type() || stage is not (2 or 5)) throw new InvalidOperationException("아처 공장현재부모/단계 오류"); var fields = new ArcherAnimationFields(); logic.Initialize(fields, stage == 5, gunForm, target); ArcherAnimation? animation = host.Attach(new ArcherAnimation(logic, fields, stage == 5), host.AnimationProcessType(stage == 5), gun.Parent(), 0x10) as ArcherAnimation; if (animation is not null && stage == 5) logic.InitializeFire(animation.Fields); return animation;
    }
    /// <summary>첫부모TYPE가현재역할이면아처를만들고다른부모는기존callback을실행한다.</summary>
    public static GunProcessHooks MakeGunHooks(SquidProcessHost host, RawArcherAnimation logic, GunProcessHooks hooks)
    {
        if (!ReferenceEquals(host.Pool(), logic.Pool)) throw new ArgumentException("아처/Gun 풀 불일치"); GunProcessHooks current = hooks.Copy(); Action<uint, ushort, ushort>? fallback = hooks.StartAnimation;
        current.StartAnimation = (stage, form, target) => { GunProcess gun = host.FromForm(form) as GunProcess ?? throw new InvalidOperationException("아처 공장현재Gun 미발견"); if (host.Pool().Slot(gun.Parent())[10] == logic.Type()) { if (Create(host, logic, stage, form, target) is null) throw new InvalidOperationException("아처 부모삭제중공장실패"); return; } Need(fallback)(stage, form, target); }; return current;
    }
}
