using System.Buffers.Binary;
using Netstorm.Core.Simulation;

// 같은 실제 Host/Kernel과 미사일 원문 필드의 수명을 연결한다.
namespace Netstorm.Core.Original;

/// <summary>원문 raw 객체와 자기 삭제를 관찰하는 실제 미사일 프로세스다.</summary>
public sealed class Missile : SquidProcess
{
    /// <summary>같은 실제 Pool의 장기 본문이다.</summary>
    private readonly RawMissile _logic;
    /// <summary>소멸 뒤에도 현재 실행 스택이 읽는 외부 수명이다.</summary>
    private readonly PathLifeWatch _watch = new();
    /// <summary>소유한 원문 이미지이며 외부 초기 입력과 별개다.</summary>
    public MissileFields Fields { get; }
    /// <summary>본문을 빌리고 객체 필드는 값복사한다.</summary>
    public Missile(RawMissile logic, MissileFields fields) { _logic = logic; Fields = new(fields.Bytes); }
    /// <summary>실제 Kernel에서 같은 수명 관찰을 전달한다.</summary>
    public override void RunFrame() { PathLifeWatch watch = _watch; _logic.Frame(Fields, watch); }
    /// <summary>실제 부착 뒤 Form/부모 DWORD만 동기화한다.</summary>
    public override void OnAttached() { BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(4), (uint)Form()); BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(8), (uint)Parent()); }
    /// <summary>원문 소멸자의 외부 수명 비트를 즉시 끈다.</summary>
    public override void Dispose() { _watch.Alive = false; base.Dispose(); }
}

/// <summary>같은 Pool의 실제 자산 생성과 flags10 부착 공장이다.</summary>
public sealed partial class RawMissile
{
    /// <summary>실제 Create만 교체하고 Pop/소유자/표시 효과는 그대로 요구한다.</summary>
    public static MissileHooks MakeFactoryHooks(RawSidPool pool, RawSquidFactory factory, MissileHooks hooks)
    {
        if (!ReferenceEquals(pool, factory.Pool)) throw new ArgumentException("미사일 공장/Pool 불일치"); MissileHooks current = hooks.Copy(); current.Create = (type, flags) => checked((ushort)factory.Create(type, (SidAllocation)flags)); return current;
    }
    /// <summary>원문 생성/Pop/비행 효과 뒤 실제 현재 역할과 flags10으로 부착한다.</summary>
    public static Missile? Create(SquidProcessHost host, RawMissile logic, GunProjectileRequest request)
    {
        if (!ReferenceEquals(host.Pool(), logic.Pool)) throw new ArgumentException("미사일 Host/Pool 불일치"); var fields = new MissileFields(); ushort asset = logic.Construct(fields, request); return host.Attach(new Missile(logic, fields), logic.ProcessType(), asset, 0x10) as Missile;
    }
    /// <summary>같은 실제 GunAnimation 요청을 실제 미사일 생성 공장으로 연결한다.</summary>
    public static GunAnimationHooks MakeGunHooks(SquidProcessHost host, RawMissile logic, GunAnimationHooks hooks)
    {
        if (!ReferenceEquals(host.Pool(), logic.Pool)) throw new ArgumentException("총 애니메이션/미사일 Pool 불일치"); GunAnimationHooks current = hooks.Copy(); current.CreateProjectile = request => { if (Create(host, logic, request) is null) throw new InvalidOperationException("미사일 자산 삭제 중 실제 부착 실패"); }; return current;
    }
}
