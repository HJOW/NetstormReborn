using System.Buffers.Binary;

// 같은실제Host/Kernel이소유하는크로스보우발사파생을연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawCrossbowAnimation.cpp의실제35프로세스어댑터다.</summary>
public sealed class CrossbowAnimation:SquidProcess
{
    /// <summary>같은실제풀의장기처리기다.</summary>
    private readonly RawCrossbowAnimation _logic;
    /// <summary>값복사로소유한원문64BYTE다.</summary>
    public GunAnimationFields Fields{get;}
    /// <summary>실제본문을빌리고원문필드를복사한다.</summary>
    public CrossbowAnimation(RawCrossbowAnimation logic,GunAnimationFields fields){_logic=logic;Fields=new(fields.Bytes);}
    /// <summary>같은Kernel틱이원문발사본문을실행한다.</summary>
    public override void RunFrame()=>_logic.Frame(Fields);
    /// <summary>부착/전송이공유하는현재두식별자DWORD만쓴다.</summary>
    private void SyncIdentity(){BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(4),(uint)Form());BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(8),(uint)Parent());}
    /// <summary>실제부착뒤현재Form/부모를동기화한다.</summary>
    public override void OnAttached()=>SyncIdentity();
    /// <summary>같은현재VT/식별자/헤더를공통codec에전달한다.</summary>
    public ushort WritePacket(Span<byte> packet,bool changed,bool force=false,byte protocol=1,byte? message=null){SyncIdentity();return RawGunAnimation.WriteProcess(Fields.Bytes,packet,changed,force,message??(RawGunAnimation.UsesFirePacket(Fields.Bytes)?(byte)0x3b:(byte)0x3c),protocol);}
}
/// <summary>실제Gun통지와같은Crossbow부착/삭제를조립한다.</summary>
public sealed partial class RawCrossbowAnimation
{
    /// <summary>현재Gun과현재Crossbow PID만실제Host에서조회한다.</summary>
    public static GunAnimationHooks MakeHostHooks(SquidProcessHost host,GunAnimationHooks hooks)
    {
        GunAnimationHooks current=hooks.Copy();current.Notify=(form,ev,info,value)=>(host.FromForm(form)as GunProcess??throw new InvalidOperationException("크로스보우 완료현재Gun 미발견")).Notify(ev,info,value);current.Kill=(form,flags)=>host.Kill(host.FromForm(form)as CrossbowAnimation??throw new InvalidOperationException("크로스보우 완료현재PID 미발견"),flags);return current;
    }
    /// <summary>같은Pool/현재Gun/현재부모역할을확인하고현재35역할·flags10으로부착한다.</summary>
    public static CrossbowAnimation? Create(SquidProcessHost host,RawCrossbowAnimation logic,ushort gunForm,ushort target)
    {
        if(!ReferenceEquals(host.Pool(),logic.Pool))throw new ArgumentException("크로스보우 Host/Pool 불일치");GunProcess? gun=host.FromForm(gunForm)as GunProcess;if(gun is null||host.Pool().Slot(gun.Parent())[10]!=logic.Type())throw new InvalidOperationException("크로스보우 공장현재부모 오류");var fields=new GunAnimationFields();logic.Initialize(fields,gunForm,target);return host.Attach(new CrossbowAnimation(logic,fields),host.AnimationProcessType(true),gun.Parent(),0x10)as CrossbowAnimation;
    }
}
