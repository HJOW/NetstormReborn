using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제반더발사와두생성wrapper의417관측을원문전체상태와대조한다.
namespace Netstorm.Core.Tests;

/// <summary>최초raw주소/현재필드·회피난수·통지뒤Form재조회를검사한다.</summary>
public sealed class RawVanderFireTests
{
    /// <summary>생성wrapper의세반환종류와미선택NULL함수/실제동일객체반환을구별한다.</summary>
    [Fact]
    public void NullableWrapperPreservesExactOwnerAndSelectedNullGuard()
    {
        var scene=new Scene();scene.Pool.RawSlot(60)[11]=0;Assert.Throws<ArgumentException>(()=>RawLightningCreate.Create<object>(scene.Pool,0,60,null,null));
        // free/dead는실제생성자를요구하지않고최초표적raw토큰만보존한다.
        foreach(byte state in new byte[]{1,2,3}){scene.Pool.RawSlot(60)[11]=state;LightningCreateResult<object> skipped=RawLightningCreate.Create<object>(scene.Pool,0,60,null,null);Assert.Equal(LightningCreateStatus.TargetUnavailable,skipped.Status);Assert.Equal(60,skipped.SkippedTarget);Assert.Null(skipped.Process);}
        scene.Pool.RawSlot(60)[11]=0;LightningCreateResult<object> failed=RawLightningCreate.Create<object>(scene.Pool,0,60,null,(_,_,_)=>null);Assert.Equal(LightningCreateStatus.AllocationFailed,failed.Status);Assert.Equal(0,failed.SkippedTarget);Assert.Null(failed.Process);var owner=new object();LightningCreateResult<object> made=RawLightningCreate.Create<object>(scene.Pool,0,60,null,(parent,target,sound)=>{Assert.Equal(0,parent);Assert.Equal(60,target);Assert.Null(sound);return owner;});Assert.Equal(LightningCreateStatus.Created,made.Status);Assert.Same(owner,made.Process);Assert.Equal(0,made.SkippedTarget);
    }
    /// <summary>원문stoul처럼음수문자열도DWORD감김으로읽는다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>현재DWORD폭만기록한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>원문raw전체이미지와같은소문자표시다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();
    /// <summary>원문명시입력과실제경계토큰을보관한다. 렌더객체를구현한것으로세지않는다.</summary>
    private sealed class Scene
    {
        /// <summary>같은원문SID의실제풀이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>현재표적HP입력표다.</summary>
        internal OriginalTypeRecord[] Types { get; } = Enumerable.Range(0,188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>반더Step이읽지않는문자표의실제폭이다.</summary>
        internal PriestPlainCanonType[] Frames { get; } = Enumerable.Range(0,188).Select(_ => new PriestPlainCanonType()).ToArray();
        /// <summary>매입력의현재발사전역이다.</summary>
        internal CannonFireState State = new();
        /// <summary>다른경계와공유하는원문단일난수다.</summary>
        internal GameRandom Random = new();
        /// <summary>패딩까지소유하는원문64BYTE다.</summary>
        internal GunAnimationFields Fields { get; } = new();
        /// <summary>현재입력0~9열만소비한다.</summary>
        internal string[] Row = [];
        /// <summary>생성자진입관측의명시불투명객체토큰이다.</summary>
        private readonly object _boundaryObject = new();
        /// <summary>효과의실제순서와인수다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>native와같은client5~70슬롯을확보한다.</summary>
        internal Scene()
        {
            // 원문부모50/51·표적60/61·Gun70번호를그대로사용한다.
            for (int sid=5;sid<=70;sid++) Assert.Equal(sid,Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>원문입력비트만으로각행을재구성한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(16,row.Length); Row=row; Events.Clear(); State=new(){Kind=CannonFireKind.Vander,Type=148,DisplayType=80,EffectType=19,Delay=.3f,Now=10}; Random=new(N(row[6])); Types[146].Flags1=Types[158].Flags1=N(row[4])!=0?0x10u:0;
            // 생성자가쓰지않은원문패딩도그대로남긴다.
            for(int i=0;i<Fields.Bytes.Length;i++) Fields.Bytes[i]=unchecked((byte)(i*13+7));
            Put(Fields.Bytes,0,0x50b170); Put(Fields.Bytes,4,81); Put(Fields.Bytes,8,50); Put(Fields.Bytes,12,0x42700000); Put(Fields.Bytes,16,0x42700000); Put(Fields.Bytes,20,0x426e0000); Put(Fields.Bytes,24,0x426e0000); Put(Fields.Bytes,32,0x50b16c); Fields.Bytes[40]=0x20; BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(42),70); BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44),60); BinaryPrimitives.WriteDoubleLittleEndian(Fields.Bytes.AsSpan(48),N(row[2])!=0?10:10.25); Put(Fields.Bytes,56,0);
            // free/dead를포함한네관측슬롯과GunForm을다시작성한다.
            foreach((int sid,byte type) in new[]{(50,(byte)148),(51,(byte)148),(60,N(row[5])!=0?(byte)146:(byte)158),(61,(byte)158),(70,(byte)34)})
            {
                Span<byte> raw=Pool.RawSlot(sid);raw.Clear();Put(raw,0,0x12400000);BinaryPrimitives.WriteUInt16LittleEndian(raw[6..],81);raw[10]=type;raw[11]=sid==60?(byte)N(row[3]):(byte)0;raw[12]=0xa5;raw[34]=1;Put(raw,14,BitConverter.SingleToUInt32Bits(sid<60?50.5f:60));Put(raw,18,BitConverter.SingleToUInt32Bits(sid<60?50.5f:60));Put(raw,36,N(row[1]));
            }
        }
        /// <summary>선택한원문효과가정상복귀하는위치에서현재필드/슬롯/시각을변경한다.</summary>
        internal void Mutate(string where)
        {
            uint value=N(Row[7]);if(where=="Center"&&value==1)BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44),61);if(where=="Point"&&value==2){Put(Fields.Bytes,8,51);BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44),61);}if(where=="Beam"&&value==3)Pool.RawSlot(60)[11]=2;if(where=="Beam"&&value==4)BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44),61);if(where=="Hit"&&value==5)Pool.RawSlot(60)[11]=2;if(where=="Refresh"&&value==6){Put(Fields.Bytes,8,51);BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44),61);State.Now=100;}if(where=="Unpop"&&value==7)Put(Fields.Bytes,8,51);if(where=="Notify"&&value==8)Put(Fields.Bytes,4,82);
        }
        /// <summary>실제생존wrapper뒤104BYTE생성자진입토큰/NULL만명시공급한다.</summary>
        internal LightningCreateResult<object> Beam(ushort source,ushort target,string? sound)=>RawLightningCreate.Create<object>(Pool,source,target,sound,(parent,victim,name)=>
        {
            Events.Add("A:104");if(N(Row[8])==0)return null;Events.Add($"L:{parent}:{victim}:{name??"null"}");Mutate("Beam");return _boundaryObject;
        });
        /// <summary>같은현재필드의빔/피해/공간/통지경계를기록한다.</summary>
        internal GunAnimationHooks Hooks()=>new()
        {
            Center=sid=>{Events.Add($"C:{sid}");Mutate("Center");return[59.5f,59.5f];},TargetPoint=(source,target)=>{Events.Add($"T:{source}:{target}");Mutate("Point");return[52.25f,52.75f];},Unpop=(sid,flags)=>{Events.Add($"U:{sid}:{flags}");Mutate("Unpop");},Repop=(sid,flags)=>Events.Add($"P:{sid}:{flags}"),Notify=(_,ev,info,value)=>{Events.Add($"M:{ev}:{info}:{value}");Mutate("Notify");},Kill=(sid,flags)=>Events.Add($"K:{sid}:{flags}"),
        };
        /// <summary>실제같은난수와최초source/현재target의반더분기를실행한다.</summary>
        internal CannonBeamHooks BeamHooks()=>new(){Create=(source,target)=>{_ =Beam(source,target,"shortZap.wav");},Refresh=source=>{Events.Add($"R:{source}");Mutate("Refresh");},Hit=(source,target)=>{Events.Add($"H:{source}:{target}");Mutate("Hit");},Random=Random};
    }
    /// <summary>반더본문393건과named/optional wrapper24건을전부비교한다.</summary>
    [Theory]
    [InlineData(false,393)] [InlineData(true,24)]
    public void MatchesAllIndependentVanderBodiesAndNullableWrappers(bool wrappers,int expected)
    {
        var scene=new Scene();int count=0;
        // 출력10~15열은실제원문기대값으로만사용한다.
        foreach(string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory,"X86/vander-fire-x86.tsv")))
        {
            if(line.Length==0||line.StartsWith('#'))continue;string[] row=line.Split('\t');uint kind=N(row[0]);if((kind>=4)!=wrappers)continue;scene.Prepare(row);var logic=new RawCannonFire(scene.Pool,scene.Types,scene.Frames,scene.State,scene.Hooks(),scene.BeamHooks());string result="return";
            if(kind==0){logic.Initialize(scene.Fields,70,60);Put(scene.Fields.Bytes,4,81);Put(scene.Fields.Bytes,8,50);scene.Events.Add("G:70:0:16");scene.Events.Add("B:35:50:0:16");}else if(kind==1)result=logic.Step(scene.Fields,unchecked((int)N(row[1]))).ToString(CultureInfo.InvariantCulture);else if(kind==2)result=logic.Launch(scene.Fields)?"1":"0";else if(kind==3)logic.Frame(scene.Fields);else{LightningCreateResult<object> made=scene.Beam(50,60,kind==4||N(row[9])!=0?"shortZap.wav":null);result=made.Status==LightningCreateStatus.TargetUnavailable?"raw"+made.SkippedTarget.ToString(CultureInfo.InvariantCulture):made.Status==LightningCreateStatus.Created?"beam":"0";}
            Assert.Equal(row[10],result);Assert.Equal(row[11],scene.Events.Count==0?"-":string.Join(';',scene.Events));Assert.Equal(row[12],Hex(scene.Fields.Bytes));Assert.Equal(row[13],string.Concat(new[]{50,51,60,61}.Select(sid=>Hex(scene.Pool.Slot(sid)))));Assert.Equal(N(row[14]),scene.Random.State());byte[] now=new byte[8];BinaryPrimitives.WriteDoubleLittleEndian(now,scene.State.Now);Assert.Equal(row[15],Hex(now));count++;
        }
        Assert.Equal(expected,count);
    }
}
