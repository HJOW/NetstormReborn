// 원문미사일이선택한위치에서만요구하는실제월드효과를보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawMissile.h의생성/공간/비행/피해/음향/진단함수값이다.</summary>
public sealed class MissileHooks
{
    /// <summary>현재발사자산의실제가상+b4포구좌표다.</summary>
    public Func<ushort,float[]>? Muzzle{get;set;}
    /// <summary>현재SHP프레임의실제+20/+22픽셀hotspot이다.</summary>
    public Func<ushort,int[]>? Hotspot{get;set;}
    /// <summary>실제00481750/CD0044a960의사정선끝점이다.</summary>
    public Func<float[],float[],ushort,float[]>? RayEnd{get;set;}
    /// <summary>표적SHP의현재타일폭/높이다.</summary>
    public Func<ushort,int[]>? TargetDimensions{get;set;}
    /// <summary>같은실제공장의Create(type,2)다.</summary>
    public Func<uint,uint,ushort>? Create{get;set;}
    /// <summary>표시TYPE의실제방향프레임선택이다.</summary>
    public Func<float[],float[],uint,int>? DirectionFrame{get;set;}
    /// <summary>현재표시자산의실제가상+74소유자지정이다.</summary>
    public Action<ushort,byte>? Owner{get;set;}
    /// <summary>표시자산의실제공간해제다.</summary>
    public Action<ushort,uint>? Unpop{get;set;}
    /// <summary>좌표/flags를전달하는같은실제공간등록이다.</summary>
    public Action<ushort,float,float,uint>? Pop{get;set;}
    /// <summary>현재TYPE발사음향토큰의위치재생이다.</summary>
    public Action<float,float,uint>? FiringSound{get;set;}
    /// <summary>같은표시자산에실제비행효과프로세스를부착한다.</summary>
    public Action<uint,ushort>? FlightEffect{get;set;}
    /// <summary>현재발사건물의실제발자국중심이다.</summary>
    public Func<ushort,float[]>? Center{get;set;}
    /// <summary>기록한표적좌표에서실제타입발자국사각형을만든다.</summary>
    public Func<ushort,float[],float[]>? TargetBounds{get;set;}
    /// <summary>현재총의실제직접표적피해다.</summary>
    public Action<ushort,ushort>? Hit{get;set;}
    /// <summary>정상표적충돌의실제시각효과다.</summary>
    public Action<float,float>? HitEffect{get;set;}
    /// <summary>실제입력큐에충돌토큰/표시TYPE/표적TYPE를순서대로넣는다.</summary>
    public Action<uint,uint,uint>? QueueCommand{get;set;}
    /// <summary>현재총의실제사거리갱신이다.</summary>
    public Action<ushort>? Refresh{get;set;}
    /// <summary>incomingCorner/아이스충돌의실제표시효과다.</summary>
    public Action<float,float,uint,float>? Impact{get;set;}
    /// <summary>현재충돌음향토큰의실제위치재생이다.</summary>
    public Action<float,float,uint>? ImpactSound{get;set;}
    /// <summary>표시자산의실제가상Destroy(0)와종속Host삭제다.</summary>
    public Action<ushort,uint>? Destroy{get;set;}
    /// <summary>아이스음향의실제min포함/max제외정수난수다.</summary>
    public Func<int,int,int>? RandomInteger{get;set;}
    /// <summary>아이스파일명이름으로요청하는실제위치재생이다.</summary>
    public Action<float,float,string>? ImpactName{get;set;}
    /// <summary>현재필드/표시자산을소비하는실제00482190반사생성이다.</summary>
    public Action<MissileFields>? Ricochet{get;set;}
    /// <summary>반사경로의실제가상98이며공격자0/현재표시소유자를전달한다.</summary>
    public Action<ushort,int,byte,ushort>? Damage{get;set;}
    /// <summary>같은게임PRNG를한번소비하는실제x87반환난수다.</summary>
    public Func<float,float,double>? RandomFloat{get;set;}
    /// <summary>0041be80진단의식/파일/행이며복귀뒤현재필드를읽는다.</summary>
    public Action<string,string,uint>? Assertion{get;set;}
    /// <summary>Source함수값을복사하고실제캡처상태수명은공유한다.</summary>
    internal MissileHooks Copy()=>(MissileHooks)MemberwiseClone();
}
