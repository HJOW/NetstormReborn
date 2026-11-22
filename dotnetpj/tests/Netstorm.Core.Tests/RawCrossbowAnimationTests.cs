using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
// 실제원문프로세스어댑터의이름을명시한다.
using CrossbowAnimation = Netstorm.Core.Original.CrossbowAnimation;

// 원문공유회전/내림발사2638관측의현재이미지와효과를그대로대조한다.
namespace Netstorm.Core.Tests;

/// <summary>RawCrossbowAnimation.cpp와기존아처140회전의원문관측이다.</summary>
public sealed class RawCrossbowAnimationTests
{
    /// <summary>미도래/NaN기한에서는부모/표적/현재역할/효과를추가로요구하지않는다.</summary>
    [Fact]
    public void DueGuardPrecedesParentTargetAndEffects()
    {
        var scene=new Scene();var role=new UIntBox{Value=140};var state=new CrossbowAnimationState{CurrentType=role};var logic=new RawCrossbowAnimation(scene.Pool,scene.Types,scene.Data,state,new());var fields=new GunAnimationFields(0xa5);Put(fields.Bytes,8,0);BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(44),0);role.Value=300;
        // 원문의unordered비교도기한미도래와같이자연반환한다.
        foreach(ulong bits in new[]{BitConverter.DoubleToUInt64Bits(100),0x7ff8123456789abcul}){BinaryPrimitives.WriteUInt64LittleEndian(fields.Bytes.AsSpan(48),bits);byte[] before=fields.Bytes.ToArray();logic.Frame(fields);Assert.Equal(before,fields.Bytes);}
        BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(48),0);Assert.Throws<InvalidOperationException>(()=>logic.Frame(fields));role.Value=140;Put(fields.Bytes,8,50);scene.Pool.RawSlot(50)[10]=140;scene.Pool.RawSlot(60)[10]=158;scene.Types[158].Flags1=0x10;BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(44),60);int points=0;logic=new RawCrossbowAnimation(scene.Pool,scene.Types,scene.Data,state,new(){TargetPoint=(_,_)=>{points++;return[0,0];}});Assert.Throws<InvalidOperationException>(()=>logic.Launch(fields));Assert.Equal(0,points);
    }
    /// <summary>실제PATCH Host가현재35역할의발사를소유하고Gun통지뒤같은Kernel/Form수명을닫는다.</summary>
    [Fact]
    public void ActualHostOwnsCrossbowAndNotifiesGunBeforeDelete()
    {
        var scene=new Scene(true);scene.Types[140].Group=0;scene.Data[140]=new(20,40);scene.Pool.RawSlot(50).Clear();scene.Pool.RawSlot(50)[10]=140;scene.Pool.RawSlot(60)[10]=158;scene.Pool.RawSlot(60)[11]=2;var kernel=new Kernel();var destroy=new RawSquidDestroy(scene.Pool,new RawUnpop(scene.Pool,new RawSquidHash(),new byte[65536]),scene.Types);byte[] buffer=new byte[64];int packets=0;var fireRole=new UIntBox{Value=44};var host=new SquidProcessHost(scene.Pool,scene.Types,kernel,destroy,new(){Now=10,CurrentFireAnimationType=fireRole},transmit:new(){WriteData=(process,changed)=>Assert.IsType<GunProcess>(process).WritePacket(buffer,changed,false),Transmit=_=>packets++});
        try
        {
            var gunLogic=new RawGunProcess(scene.Pool,scene.Types,new(){Now=10,Boss=false},new());GunProcess gun=Assert.IsType<GunProcess>(RawGunProcess.CreateGunProcess(host,gunLogic,50));Put(gun.Fields.Bytes,28,6);var logic=new RawCrossbowAnimation(scene.Pool,scene.Types,scene.Data,new(){Now=10},RawCrossbowAnimation.MakeHostHooks(host,new(){Unpop=(_,_)=>{},Repop=(_,_)=>{}}));CrossbowAnimation fire=Assert.IsType<CrossbowAnimation>(RawCrossbowAnimation.Create(host,logic,(ushort)gun.Form(),60));int form=fire.Form();Assert.Equal(44,scene.Pool.Slot(form)[10]);Assert.Equal(2,kernel.Size());fire.RunFrame();Assert.Equal(7u,gun.Fields.Stage());Assert.Null(host.FromForm(form));Assert.Equal(1,kernel.Size());Assert.Equal(1u,BinaryPrimitives.ReadUInt32LittleEndian(scene.Pool.Slot(50)[36..]));Assert.True(packets>0);Assert.Equal(0u,destroy.PreDepth);Assert.Equal(0u,destroy.PostDepth);host.Kill(gun,0x40);Assert.Equal(0,kernel.Size());
        }
        finally{kernel.DrainOwnedForShutdown();}
    }
    /// <summary>원문DWORD입력비트를읽는다.</summary>
    private static uint N(string value)=>uint.Parse(value,CultureInfo.InvariantCulture);
    /// <summary>원문DWORD폭만쓴다.</summary>
    private static void Put(Span<byte> raw,int at,uint value)=>BinaryPrimitives.WriteUInt32LittleEndian(raw[at..],value);
    /// <summary>기대전체raw와같은소문자표시다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw)=>Convert.ToHexString(raw).ToLowerInvariant();
    /// <summary>기대값을읽지않는원문입력/같은현재이미지별칭장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>원문SID의실제풀이다.</summary>
        internal RawSidPool Pool{get;}
        /// <summary>현재표적HP입력이다.</summary>
        internal OriginalTypeRecord[] Types{get;}=Enumerable.Range(0,188).Select(i=>new OriginalTypeRecord{ConstructorAddress=OriginalConstructors.Address(i)}).ToArray();
        /// <summary>현재방향수/물리프레임자료다.</summary>
        internal ArcherTypeData[] Data{get;}=Enumerable.Range(0,188).Select(_=>new ArcherTypeData()).ToArray();
        /// <summary>공유회전의현재시각/140역할이다.</summary>
        internal ArcherAnimationState RotateState=new();
        /// <summary>발사의별개현재시각/속도/표시역할이다.</summary>
        internal CrossbowAnimationState State=new();
        /// <summary>회전의원문88BYTE다.</summary>
        internal ArcherAnimationFields Fields{get;}=new();
        /// <summary>발사의원문64BYTE다.</summary>
        internal GunAnimationFields Image{get;}=new();
        /// <summary>실행중인실제현재발사이미지별칭여부다.</summary>
        internal bool FireActive;
        /// <summary>각callback이즉시읽고쓰는같은현재이미지다.</summary>
        internal byte[] Active=>FireActive?Image.Bytes:Fields.Bytes;
        /// <summary>현재독립입력0~11열이다.</summary>
        internal string[] Row=[];
        /// <summary>실제효과의순서/인수다.</summary>
        internal List<string> Events{get;}=[];
        /// <summary>원문5~70슬롯을동일할당자로준비한다.</summary>
        internal Scene(bool server=false)
        {
            Pool=new(24000,server);
            // 네관측슬롯과GunForm의명시번호를그대로쓴다.
            for(int sid=5;sid<=70;sid++)Assert.Equal(sid,Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>입력만읽어현재상태/패딩/다섯raw슬롯을재구성한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(16,row.Length);Row=row;Events.Clear();FireActive=false;State=new(){Now=10};RotateState=new(){Type=140,Now=10};Data[140]=new((int)N(row[5]),(int)N(row[6]));Types[158].Flags1=N(row[4])!=0?0x10u:0;
            // 생성자가쓰지않은패딩을native입력패턴으로남긴다.
            for(int i=0;i<Fields.Bytes.Length;i++)Fields.Bytes[i]=unchecked((byte)(i*13+7));
            bool fire=N(row[0])is 1 or 2 or 4;Put(Fields.Bytes,0,fire?0x50b120u:0x50b450u);Put(Fields.Bytes,4,81);Put(Fields.Bytes,8,50);Put(Fields.Bytes,12,0x42700000);Put(Fields.Bytes,16,0x42700000);Put(Fields.Bytes,20,0x426e0000);Put(Fields.Bytes,24,0x426e0000);Put(Fields.Bytes,32,fire?0x50b11cu:0x50b448u);Fields.Bytes[40]=0x20;BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(42),70);BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44),(ushort)N(row[10]));BinaryPrimitives.WriteDoubleLittleEndian(Fields.Bytes.AsSpan(48),N(row[2])!=0?10:10.25);Put(Fields.Bytes,56,0);
            // free/dead를포함한네관측슬롯과GunForm을다시쓴다.
            foreach((int sid,byte type)in new[]{(50,(byte)140),(51,(byte)140),(60,(byte)158),(61,(byte)158),(70,(byte)34)})
            {
                Span<byte> raw=Pool.RawSlot(sid);raw.Clear();Put(raw,0,0x1131a000);raw[10]=type;raw[11]=sid==60?(byte)N(row[3]):(byte)0;raw[12]=0xa5;raw[34]=1;raw[30]=(byte)N(row[9]);Put(raw,14,BitConverter.SingleToUInt32Bits(sid<60?50.5f:60));Put(raw,18,BitConverter.SingleToUInt32Bits(sid<60?50.5f:60));Put(raw,36,N(row[1]));
            }
        }
        /// <summary>원문외부효과복귀위치에서같은현재이미지/시각을바꾼다.</summary>
        internal void Mutate(string where)
        {
            uint value=N(Row[11]);if(where=="Center"&&value==1)Put(Pool.RawSlot(50),36,7);if(where=="Center"&&value==2)BinaryPrimitives.WriteUInt16LittleEndian(Active.AsSpan(44),61);if(where=="Unpop"&&value==3){State.Now=100;BinaryPrimitives.WriteDoubleLittleEndian(Active.AsSpan(48),777);}if(where=="Repop"&&value==4)Pool.RawSlot(60)[11]=2;
            if(where=="Point"&&value==5){Put(Active,8,51);BinaryPrimitives.WriteUInt16LittleEndian(Active.AsSpan(44),61);Put(Active,12,BitConverter.SingleToUInt32Bits(77.25f));Put(Active,16,BitConverter.SingleToUInt32Bits(78.5f));State.Speed=37;State.DisplayType=82;}if(where=="Missile"&&value==6)BinaryPrimitives.WriteUInt16LittleEndian(Active.AsSpan(42),71);
        }
        /// <summary>실제호출인자만관측하며투사체실행을대신하지않는다.</summary>
        internal void Projectile(GunProjectileRequest request)
        {
            Events.Add("A:128");uint[] args=[request.Firing,request.Target,BitConverter.SingleToUInt32Bits(request.Speed),request.DisplayType,request.EffectType,BitConverter.SingleToUInt32Bits(request.TargetPosition[0]),BitConverter.SingleToUInt32Bits(request.TargetPosition[1]),BitConverter.SingleToUInt32Bits(request.TargetCenter[0]),BitConverter.SingleToUInt32Bits(request.TargetCenter[1]),request.Ricochet,BitConverter.SingleToUInt32Bits(request.FiringPosition[0]),BitConverter.SingleToUInt32Bits(request.FiringPosition[1])];Events.Add("J:"+string.Join(',',args.Select(v=>v.ToString(CultureInfo.InvariantCulture))));Mutate("Missile");
        }
        /// <summary>같은현재이미지를즉시소비하는원문월드효과다.</summary>
        internal GunAnimationHooks Hooks()=>new()
        {
            Center=sid=>{Events.Add($"C:{sid}");Mutate("Center");return sid==50?[50,50]:[BitConverter.UInt32BitsToSingle(N(Row[7])),BitConverter.UInt32BitsToSingle(N(Row[8]))];},Unpop=(sid,flags)=>{Events.Add($"U:{sid}:{flags}");Mutate("Unpop");},Repop=(sid,flags)=>{Events.Add($"P:{sid}:{flags}");Mutate("Repop");},TargetPoint=(sid,target)=>{Events.Add($"T:{sid}:{target}");Mutate("Point");return[52.25f,52.75f];},CreateProjectile=Projectile,Notify=(_,ev,info,value)=>Events.Add($"M:{ev}:{info}:{value}"),Kill=(form,flags)=>Events.Add($"K:{form}:{flags}"),RandomInteger=(low,high)=>{Events.Add($"N:{low}:{high}");Events.Add("F:3");return 3000;},Sound=(_,name)=>Events.Add("S:"+name),
        };
    }
    /// <summary>공유회전812/내림발사1816/두생성자와launch10을모두대조한다.</summary>
    [Theory]
    [InlineData(0,0,812)] [InlineData(1,1,1816)] [InlineData(2,4,10)]
    public void MatchesAllIndependentSharedRotationAndCrossbowFire(int first,int last,int expected)
    {
        var scene=new Scene();int count=0;
        // 결과12~15열은원문기대값으로만사용한다.
        foreach(string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory,"X86/crossbow-animation-x86.tsv")))
        {
            if(line.Length==0||line.StartsWith('#'))continue;string[] row=line.Split('\t');uint kind=N(row[0]);if(kind<first||kind>last)continue;scene.Prepare(row);var rotate=new RawArcherAnimation(scene.Pool,scene.Types,scene.Data,scene.RotateState,new(){Common=scene.Hooks(),AimDirection=sid=>scene.Pool.Slot(sid)[30]});string result="return";
            if(kind==0)rotate.Rotate(scene.Fields);else if(kind==3){rotate.Initialize(scene.Fields,false,70,(ushort)N(row[10]));scene.Events.Add("B:36:50:0:16");Put(scene.Fields.Bytes,4,81);Put(scene.Fields.Bytes,8,50);}else
            {
                scene.Fields.Bytes.AsSpan(0,64).CopyTo(scene.Image.Bytes);scene.FireActive=true;var logic=new RawCrossbowAnimation(scene.Pool,scene.Types,scene.Data,scene.State,scene.Hooks());if(kind==1)logic.Frame(scene.Image);else if(kind==2){logic.Initialize(scene.Image,70,(ushort)N(row[10]));scene.Events.Add("B:35:50:0:16");Put(scene.Image.Bytes,4,81);Put(scene.Image.Bytes,8,50);}else result=logic.Launch(scene.Image)?"1":"0";
            }
            Assert.Equal(row[12],result);Assert.Equal(row[13],string.Join(';',scene.Events));Assert.Equal(row[14],Hex(scene.Active));Assert.Equal(row[15],string.Concat(new[]{50,51,60,61}.Select(sid=>Hex(scene.Pool.Slot(sid)))));count++;
        }
        Assert.Equal(expected,count);
    }
}
