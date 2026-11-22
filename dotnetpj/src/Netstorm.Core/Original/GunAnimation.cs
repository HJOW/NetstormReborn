using System.Buffers.Binary;

// 같은 실제 Host/Kernel의 Gun통지·애니메이션 부착과 종료를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>원문 썬 캐논의 실제 SquidProcess 파생이다.</summary>
public sealed class GunAnimation : SquidProcess
{
    /// <summary>같은 Pool의 실행 본체다.</summary>
    private readonly RawGunAnimation _logic;
    /// <summary>생성자가 선택한 실제 파생 종류다.</summary>
    private readonly GunAnimationKind _kind;
    /// <summary>값 복사로 소유한 실제64BYTE 이미지다.</summary>
    public GunAnimationFields Fields { get; }
    /// <summary>현재 본체를 빌리고 외부 필드 이미지를 복사한다.</summary>
    public GunAnimation(RawGunAnimation logic, GunAnimationFields fields, GunAnimationKind kind) { _logic = logic; Fields = new(fields.Bytes); _kind = kind; }
    /// <summary>같은 Kernel 프레임이 현재 원문 본체를 실행한다.</summary>
    public override void RunFrame() => _logic.Frame(Fields, _kind);
    /// <summary>부착/전송이 공유하는 실제 Form/부모 DWORD만 기록한다.</summary>
    private void SyncIdentity() { BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(4), (uint)Form()); BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(8), (uint)Parent()); }
    /// <summary>실제 부착 뒤 현재 식별자를 동기화한다.</summary>
    public override void OnAttached() => SyncIdentity();
    /// <summary>현재 식별자와 실제VT로 공통 writer를 호출한다.</summary>
    public ushort WritePacket(Span<byte> packet, bool parentChanged, bool force = false, byte protocol = 1, byte? message = null)
    {
        SyncIdentity(); return message is byte selected ? RawGunAnimation.WriteProcess(Fields.Bytes, packet, parentChanged, force, selected, protocol) : RawGunAnimation.WritePacket(Fields, packet, parentChanged, force, protocol);
    }
}
/// <summary>기존 Gun의 실제 통지/생성 슬롯을 같은 애니메이션 공장에 연결한다.</summary>
public sealed partial class RawGunAnimation
{
    /// <summary>현재 GunProcess와 Animation PID만 실제 Host에서 조회하며 다른 파생을 받아들이지 않는다.</summary>
    public static GunAnimationHooks MakeHostHooks(SquidProcessHost host, GunAnimationHooks hooks)
    {
        GunAnimationHooks current = hooks.Copy(); current.Notify = (form, ev, info, value) => (host.FromForm(form) as GunProcess ?? throw new InvalidOperationException("애니메이션 완료의 현재Gun 미발견")).Notify(ev, info, value);
        current.Kill = (form, flags) => host.Kill(host.FromForm(form) as GunAnimation ?? throw new InvalidOperationException("애니메이션 완료의 현재PID 미발견"), flags); return current;
    }
    /// <summary>현재Gun/부모/단계를 확인하고 원문flags10/현재35·36역할로 실제 부착한다.</summary>
    public static GunAnimation? Create(SquidProcessHost host, RawGunAnimation logic, uint stage, ushort gunForm, ushort target)
    {
        if (!ReferenceEquals(host.Pool(), logic.Pool)) throw new ArgumentException("총 애니메이션/Host 풀 불일치"); GunProcess gun = host.FromForm(gunForm) as GunProcess ?? throw new InvalidOperationException("애니메이션 공장의 현재Gun 미발견"); if (host.Pool().Slot(gun.Parent())[10] != logic.SunCannonType()) throw new InvalidOperationException("애니메이션 공장의 부모파생 미연결");
        GunAnimationKind kind = stage == 2 ? GunAnimationKind.SunRotate : stage == 5 ? GunAnimationKind.SunFire : GunAnimationKind.SunRest; if (stage is not (2 or 5 or 8)) throw new ArgumentOutOfRangeException(nameof(stage)); var fields = new GunAnimationFields(); logic.Initialize(fields, kind, (ushort)gun.Form(), target); return host.Attach(new GunAnimation(logic, fields, kind), host.AnimationProcessType(stage != 2 && stage != 8), gun.Parent(), 0x10) as GunAnimation;
    }
    /// <summary>실제Gun 큰생성슬롯만 교체하며 다른 함수값을 보존한다.</summary>
    public static GunProcessHooks MakeGunHooks(SquidProcessHost host, RawGunAnimation logic, GunProcessHooks hooks)
    {
        if (!ReferenceEquals(host.Pool(), logic.Pool)) throw new ArgumentException("총 애니메이션/Host 풀 불일치"); GunProcessHooks current = hooks.Copy(); current.StartAnimation = (stage, form, target) => { if (Create(host, logic, stage, form, target) is null) throw new InvalidOperationException("총 애니메이션 부모삭제중 공장실패"); }; return current;
    }
}

/// <summary>같은 원문 Host 상태의 두 애니메이션 역할을 소비위치에서 빌린다.</summary>
public sealed partial class SquidProcessHost
{
    /// <summary>발사/회전 공장의 현재35/36 DWORD 역할이다.</summary>
    public uint AnimationProcessType(bool fire) => fire ? _state.FireAnimationType() : _state.RotateAnimationType();
}
