using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제아이스80BYTE의3585관측과현재냉각/문맥/표적순서를대조한다.
namespace Netstorm.Core.Tests;

/// <summary>공통Fire재사용/전용Step/생성padding/현재getter와Host수명을검사한다.</summary>
public sealed class RawIceCannonFireTests
{
    /// <summary>냉각실패전active쓰기·NaN기한·문맥값복사와현재표적/active재조회를구별한다.</summary>
    [Fact]
    public void CoolingFailureAndUnorderedDeadlineKeepSourcePartialEffects()
    {
        var scene=new Scene();scene.State=new(){Now=10,Type=129,DisplayType=130,Delay=.08f};scene.Frames[129].Frames=new([new RiftFrameCode((byte)'A',0,0,0),new RiftFrameCode((byte)'A',0,0,0)]);scene.Pool.RawSlot(50)[10]=129;Put(scene.Pool.RawSlot(50),36,0);Put(scene.Fields.Common.Bytes,8,50);BinaryPrimitives.WriteUInt16LittleEndian(scene.Fields.Common.Bytes.AsSpan(44),60);Put(scene.Fields.Extra,0,256);scene.Fields.Extra.AsSpan(4,4).Fill(0xa5);BinaryPrimitives.WriteDoubleLittleEndian(scene.Fields.Extra.AsSpan(8),9);scene.Fields.Common.Bytes[40]=0x80;
        var logic=new RawIceCannonFire(scene.Pool,scene.Types,scene.Frames,scene.State,new(),new());Assert.Throws<InvalidOperationException>(()=>logic.Step(scene.Fields,0));Assert.Equal(0u,BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Extra));Assert.Equal(9,BinaryPrimitives.ReadDoubleLittleEndian(scene.Fields.Extra.AsSpan(8)));Assert.Equal(0x80,scene.Fields.Common.Bytes[40]);Assert.All(scene.Fields.Extra.AsSpan(4,4).ToArray(),value=>Assert.Equal(0xa5,value));
        Put(scene.Fields.Extra,0,1);BinaryPrimitives.WriteUInt64LittleEndian(scene.Fields.Extra.AsSpan(8),0x7ff8123456789abc);uint[] shared=[201,202,203,204,205,206,207,208];int contexts=0;var hooks=new IceCannonFireHooks{Cooling=_=>throw new IOException("NaN전환금지"),Context=parent=>{Assert.Equal(50,parent);contexts++;BinaryPrimitives.WriteUInt16LittleEndian(scene.Fields.Common.Bytes.AsSpan(44),61);return shared;},MayTarget=(context,target)=>{shared[0]=999;Assert.Equal(201u,context[0]);Assert.Equal(61,target);Put(scene.Fields.Extra,0,0);return true;}};logic=new RawIceCannonFire(scene.Pool,scene.Types,scene.Frames,scene.State,new(),hooks);Assert.Equal(1u,logic.Step(scene.Fields,0));Assert.Equal(1,contexts);Assert.Equal(0x7ff8123456789abcul,BinaryPrimitives.ReadUInt64LittleEndian(scene.Fields.Extra.AsSpan(8)));Assert.Equal(0x80,scene.Fields.Common.Bytes[40]);
    }
    /// <summary>실제PATCH Host가같은Gun통지/현재35역할/자기삭제와Kernel/깊이를닫는다.</summary>
    [Fact]
    public void ActualHostUsesIceStepAndDeletesSameOwnedProcess()
    {
        var scene=new Scene(true);scene.Types[129].Group=1;scene.Frames[129].Frames=new([new RiftFrameCode((byte)'A',0,0,0),new RiftFrameCode((byte)'A',0,0,0)]);scene.Pool.RawSlot(50).Clear();scene.Pool.RawSlot(50)[10]=129;scene.Pool.RawSlot(60)[10]=158;scene.Pool.RawSlot(60)[11]=2;var kernel=new Kernel();var destroy=new RawSquidDestroy(scene.Pool,new RawUnpop(scene.Pool,new RawSquidHash(),new byte[65536]),scene.Types);byte[] buffer=new byte[64];int packets=0;var host=new SquidProcessHost(scene.Pool,scene.Types,kernel,destroy,new(){Now=10},transmit:new(){WriteData=(process,changed)=>Assert.IsType<GunProcess>(process).WritePacket(buffer,changed,false),Transmit=_=>packets++});
        try
        {
            var gunLogic=new RawGunProcess(scene.Pool,scene.Types,new(){Now=10,Boss=false},new());GunProcess gun=Assert.IsType<GunProcess>(RawGunProcess.CreateGunProcess(host,gunLogic,50));Put(gun.Fields.Bytes,28,6);var state=new CannonFireState{Now=10,Type=129,DisplayType=130,Delay=.08f};var logic=new RawIceCannonFire(scene.Pool,scene.Types,scene.Frames,state,RawIceCannonFire.MakeHostHooks(host,new(){Unpop=(_,_)=>{},Repop=(_,_)=>{}}),new(){Context=_=>new uint[8],MayTarget=(_,_)=>false});IceCannonFire fire=Assert.IsType<IceCannonFire>(RawIceCannonFire.Create(host,logic,(ushort)gun.Form(),60));int form=fire.Form();Assert.Equal(2,kernel.Size());fire.RunFrame();Assert.Equal(7u,gun.Fields.Stage());Assert.Null(host.FromForm(form));Assert.Equal(1,kernel.Size());Assert.True((scene.Pool.Slot(form)[11]&3)!=0);Assert.True(packets>0);Assert.Equal(0u,destroy.PreDepth);Assert.Equal(0u,destroy.PostDepth);host.Kill(gun,0x40);Assert.Equal(0,kernel.Size());
        }
        finally{kernel.DrainOwnedForShutdown();}
    }
    /// <summary>원문DWORD입력을읽는다.</summary>
    private static uint N(string value)=>uint.Parse(value,CultureInfo.InvariantCulture);
    /// <summary>음수도포함하는원문Step인수다.</summary>
    private static int I(string value)=>int.Parse(value,CultureInfo.InvariantCulture);
    /// <summary>현재DWORD폭만쓴다.</summary>
    private static void Put(Span<byte> raw,int at,uint value)=>BinaryPrimitives.WriteUInt32LittleEndian(raw[at..],value);
    /// <summary>전체raw를원문소문자표시로바꾼다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw)=>Convert.ToHexString(raw).ToLowerInvariant();
    /// <summary>기대값을읽지않는실제원문입력/효과장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>같은원문번호를확보한실제풀이다.</summary>
        internal RawSidPool Pool{get;}
        /// <summary>현재HP/생성자입력표다.</summary>
        internal OriginalTypeRecord[] Types{get;}=Enumerable.Range(0,188).Select(i=>new OriginalTypeRecord{ConstructorAddress=OriginalConstructors.Address(i)}).ToArray();
        /// <summary>현재문자구간입력이다.</summary>
        internal PriestPlainCanonType[] Frames{get;}=Enumerable.Range(0,188).Select(_=>new PriestPlainCanonType()).ToArray();
        /// <summary>현재시각/129역할/130표시/지연전역이다.</summary>
        internal CannonFireState State=new();
        /// <summary>패딩까지포함한공통64/전용16이미지다.</summary>
        internal IceCannonFireFields Fields{get;}=new();
        /// <summary>현재입력0~10열만쓴다.</summary>
        internal string[] Row=[];
        /// <summary>실제효과순서/인수기록이다.</summary>
        internal List<string> Events{get;}=[];
        /// <summary>실제5~70SID를동일순서로할당한다.</summary>
        internal Scene(bool server=false)
        {
            Pool=new(24000,server);
            // 네관측슬롯과GunForm의원문번호를확보한다.
            for(int sid=5;sid<=70;sid++)Assert.Equal(sid,Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>입력만으로표/상태/80BYTE/다섯슬롯을독립초기화한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(15,row.Length);Row=row;Events.Clear();State=new(){Type=129,DisplayType=130,Delay=.08f,Now=10};Types[158].Flags1=N(row[4])!=0?0x10u:0;var codes=new List<RiftFrameCode>();
            // 실제연속/단일/교차/불균등문자구간입력이다.
            for(int i=0;i<32;i++){uint style=N(row[5]);int letter=style==0?65+i/8:style==1?65:style==2?65+i%2:i==0?65:i<6?66:i<19?67:68;codes.Add(new((byte)letter,0,0,0));}
            Frames[129].Frames=new(codes);
            // 공통미작성padding도원문패턴을보존한다.
            for(int i=0;i<64;i++)Fields.Common.Bytes[i]=unchecked((byte)(i*13+7));
            // +44전용padding까지같은원문패턴이다.
            for(int i=0;i<16;i++)Fields.Extra[i]=unchecked((byte)((i+64)*13+7));
            byte[] raw=Fields.Common.Bytes;Put(raw,0,0x50b3f0);Put(raw,4,81);Put(raw,8,50);Put(raw,12,0x42700000);Put(raw,16,0x42700000);Put(raw,20,0x426e0000);Put(raw,24,0x426e0000);Put(raw,32,0x50b3ec);raw[40]=0x20;BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(42),70);BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(44),60);BinaryPrimitives.WriteDoubleLittleEndian(raw.AsSpan(48),N(row[2])!=0?10:10.25);Put(raw,56,0);Put(Fields.Extra,0,N(row[7]));BinaryPrimitives.WriteDoubleLittleEndian(Fields.Extra.AsSpan(8),N(row[8])==0?9:N(row[8])==1?10:10.25);
            // free/dead를포함한네관측슬롯과GunForm을각행마다다시쓴다.
            foreach((int sid,byte type)in new[]{(50,(byte)129),(51,(byte)129),(60,(byte)158),(61,(byte)158),(70,(byte)34)})
            {
                Span<byte> bytes=Pool.RawSlot(sid);bytes.Clear();Put(bytes,0,0x1131a000);bytes[10]=type;bytes[11]=sid==60?(byte)N(row[3]):(byte)0;bytes[12]=0xa5;bytes[34]=1;Put(bytes,14,BitConverter.SingleToUInt32Bits(sid<60?50.5f:60));Put(bytes,18,BitConverter.SingleToUInt32Bits(sid<60?50.5f:60));Put(bytes,36,N(row[1]));
            }
        }
        /// <summary>선언된효과복귀위치에서현재필드/시각만변경한다.</summary>
        internal void Mutate(string where)
        {
            uint value=N(Row[10]);byte[] raw=Fields.Common.Bytes;if(where=="Center"&&value==1)Put(Pool.RawSlot(50),36,2);if(where=="Center"&&value==2)BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(44),61);if(where=="Unpop"&&value==3){State.Now=100;BinaryPrimitives.WriteDoubleLittleEndian(raw.AsSpan(48),777);}if(where=="Repop"&&value==4)Pool.RawSlot(60)[11]=2;
            if(where=="Point"&&value==5){Put(raw,8,51);BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(44),61);Put(raw,12,BitConverter.SingleToUInt32Bits(77.25f));Put(raw,16,BitConverter.SingleToUInt32Bits(78.5f));State.Speed=37;State.DisplayType=133;}if(where=="Missile"&&value==6)BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(42),71);
        }
        /// <summary>실제Missile요청인자만관측하고비행완료로세지않는다.</summary>
        internal void Projectile(GunProjectileRequest request)
        {
            Events.Add("A:128");uint[] args=[request.Firing,request.Target,BitConverter.SingleToUInt32Bits(request.Speed),request.DisplayType,request.EffectType,BitConverter.SingleToUInt32Bits(request.TargetPosition[0]),BitConverter.SingleToUInt32Bits(request.TargetPosition[1]),BitConverter.SingleToUInt32Bits(request.TargetCenter[0]),BitConverter.SingleToUInt32Bits(request.TargetCenter[1]),request.Ricochet,BitConverter.SingleToUInt32Bits(request.FiringPosition[0]),BitConverter.SingleToUInt32Bits(request.FiringPosition[1])];Events.Add("J:"+string.Join(',',args.Select(v=>v.ToString(CultureInfo.InvariantCulture))));Mutate("Missile");
        }
        /// <summary>같은현재입력의공통Fire하위효과다.</summary>
        internal GunAnimationHooks Common()=>new()
        {
            Center=sid=>{Events.Add($"C:{sid}");Mutate("Center");return[60,50];},Unpop=(sid,flags)=>{Events.Add($"U:{sid}:{flags}");Mutate("Unpop");},Repop=(sid,flags)=>{Events.Add($"P:{sid}:{flags}");Mutate("Repop");},TargetPoint=(sid,target)=>{Events.Add($"T:{sid}:{target}");Mutate("Point");return[52.25f,52.75f];},CreateProjectile=Projectile,Notify=(_,ev,info,value)=>Events.Add($"M:{ev}:{info}:{value}"),Kill=(form,flags)=>Events.Add($"K:{form}:{flags}"),
        };
        /// <summary>선택냉각/문맥뒤현재표적과판정뒤현재active를관측한다.</summary>
        internal IceCannonFireHooks Ice()=>new()
        {
            Cooling=sid=>{Events.Add($"E:{sid}");if(N(Row[10])==7){State.Now=100;Put(Fields.Common.Bytes,8,51);}},Context=sid=>{Events.Add($"X:{sid}");if(N(Row[10])==8){Put(Fields.Common.Bytes,8,51);BinaryPrimitives.WriteUInt16LittleEndian(Fields.Common.Bytes.AsSpan(44),61);}return[201,202,203,204,205,206,207,208];},MayTarget=(context,target)=>{Events.Add("A:"+string.Join(',',context.Select(v=>v.ToString(CultureInfo.InvariantCulture)))+','+target.ToString(CultureInfo.InvariantCulture));if(N(Row[10])==9)Put(Fields.Extra,0,0);return N(Row[9])!=0;},
        };
    }
    /// <summary>공통Frame3144/생성1/전용Step432/Launch8의3585건을전부대조한다.</summary>
    [Theory]
    [InlineData(0,3144)] [InlineData(1,1)] [InlineData(2,432)] [InlineData(3,8)]
    public void MatchesAllIndependentIceCannonFramesAndCooling(int kind,int expected)
    {
        var scene=new Scene();int count=0;
        // 결과11~14열은기대원문대조에만사용한다.
        foreach(string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory,"X86/ice-cannon-fire-x86.tsv")))
        {
            if(line.Length==0||line.StartsWith('#'))continue;string[] row=line.Split('\t');if(N(row[0])!=kind)continue;scene.Prepare(row);var logic=new RawIceCannonFire(scene.Pool,scene.Types,scene.Frames,scene.State,scene.Common(),scene.Ice());string result="return";
            switch(kind)
            {
                case 0:logic.Frame(scene.Fields);break;
                case 1:logic.Initialize(scene.Fields,70,60);scene.Events.Add("B:35:50:0:16");Put(scene.Fields.Common.Bytes,4,81);Put(scene.Fields.Common.Bytes,8,50);break;
                case 2:result=logic.Step(scene.Fields,I(row[6])).ToString(CultureInfo.InvariantCulture);break;
                case 3:result=logic.Launch(scene.Fields)?"1":"0";break;
                default:throw new InvalidOperationException("아이스 독립입력의미지본문");
            }
            Assert.Equal(row[11],result);Assert.Equal(row[12],string.Join(';',scene.Events));Assert.Equal(row[13],Hex(scene.Fields.Common.Bytes)+Hex(scene.Fields.Extra));Assert.Equal(row[14],string.Concat(new[]{50,51,60,61}.Select(sid=>Hex(scene.Pool.Slot(sid)))));count++;
        }
        Assert.Equal(expected,count);
    }
}
