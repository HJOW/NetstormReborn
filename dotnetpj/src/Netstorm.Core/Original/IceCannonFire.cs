using System.Buffers.Binary;

// 실제아이스파생의동일Host/Kernel/통지/공통writer 수명을연결한다.
namespace Netstorm.Core.Original;

/// <summary>원문80BYTE를소유하는실제35프로세스다.</summary>
public sealed class IceCannonFire:SquidProcess
{
    /// <summary>같은현재월드의장기본문이다.</summary>
    private readonly RawIceCannonFire _logic;
    /// <summary>값복사해소유한공통64BYTE/추가16BYTE다.</summary>
    public IceCannonFireFields Fields{get;}
    /// <summary>현재본문을빌리고원문이미지전체를복사한다.</summary>
    public IceCannonFire(RawIceCannonFire logic,IceCannonFireFields fields){_logic=logic;Fields=new(fields);}
    /// <summary>같은Kernel이공통Fire/아이스Step을실행한다.</summary>
    public override void RunFrame()=>_logic.Frame(Fields);
    /// <summary>실제부착/전송이같이읽는현재식별자DWORD만쓴다.</summary>
    private void SyncIdentity(){BinaryPrimitives.WriteUInt32LittleEndian(Fields.Common.Bytes.AsSpan(4),(uint)Form());BinaryPrimitives.WriteUInt32LittleEndian(Fields.Common.Bytes.AsSpan(8),(uint)Parent());}
    /// <summary>실제부착뒤현재Form/부모를동기화한다.</summary>
    public override void OnAttached()=>SyncIdentity();
    /// <summary>추가16BYTE는전송하지않고현재VT/헤더/식별자를공통codec에전달한다.</summary>
    public ushort WritePacket(Span<byte> packet,bool changed,bool force=false,byte protocol=1,byte? message=null){SyncIdentity();return RawGunAnimation.WriteProcess(Fields.Common.Bytes,packet,changed,force,message??(RawGunAnimation.UsesFirePacket(Fields.Common.Bytes)?(byte)0x3b:(byte)0x3c),protocol);}
}
/// <summary>현재Gun가상통지와아이스파생의같은Host수명을조립한다.</summary>
public sealed partial class RawIceCannonFire
{
    /// <summary>현재실제Gun과IceCannonFire PID만조회하여통지/삭제한다.</summary>
    public static GunAnimationHooks MakeHostHooks(SquidProcessHost host,GunAnimationHooks hooks)
    {
        GunAnimationHooks current=hooks.Copy();current.Notify=(form,ev,info,value)=>(host.FromForm(form)as GunProcess??throw new InvalidOperationException("아이스 발사 완료현재Gun 미발견")).Notify(ev,info,value);current.Kill=(form,flags)=>host.Kill(host.FromForm(form)as IceCannonFire??throw new InvalidOperationException("아이스 발사 완료현재PID 미발견"),flags);return current;
    }
    /// <summary>같은Pool/현재Gun/현재부모역할을확인하고현재35역할·flags10으로실제부착한다.</summary>
    public static IceCannonFire? Create(SquidProcessHost host,RawIceCannonFire logic,ushort gunForm,ushort target)
    {
        if(!ReferenceEquals(host.Pool(),logic.Pool))throw new ArgumentException("아이스 발사 Host/Pool 불일치");GunProcess? gun=host.FromForm(gunForm)as GunProcess;if(gun is null||host.Pool().Slot(gun.Parent())[10]!=logic.Type())throw new InvalidOperationException("아이스 발사 공장현재부모 오류");var fields=new IceCannonFireFields();logic.Initialize(fields,gunForm,target);return host.Attach(new IceCannonFire(logic,fields),host.AnimationProcessType(true),gun.Parent(),0x10)as IceCannonFire;
    }
}
