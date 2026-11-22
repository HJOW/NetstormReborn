using System.Buffers.Binary;

// 아이스80BYTE의냉각/문맥상태와기존공통Fire본문을같은실제풀에연결한다.
namespace Netstorm.Core.Original;

/// <summary>공통64BYTE와원문추가16BYTE/padding을구별해소유한다.</summary>
public sealed class IceCannonFireFields
{
    /// <summary>기존공통Fire가직접읽고쓰는동일접두다.</summary>
    public GunAnimationFields Common{get;}
    /// <summary>+40active/+44padding/+48double기한의원문이미지다.</summary>
    public byte[] Extra{get;}=new byte[16];
    /// <summary>미작성패딩을포함한명시초기패턴을준비한다.</summary>
    public IceCannonFireFields(byte fill=0){Common=new(fill);Array.Fill(Extra,fill);}
    /// <summary>외부이미지와소유권을분리하는전체80BYTE값복사다.</summary>
    public IceCannonFireFields(IceCannonFireFields other){Common=new(other.Common.Bytes);other.Extra.CopyTo(Extra,0);}
}
/// <summary>원문하위가선택되는위치에서만요구하는아이스전용효과다.</summary>
public sealed class IceCannonFireHooks
{
    /// <summary>현재프레임포인트의실제004a8770냉각표시다.</summary>
    public Action<ushort>? Cooling{get;set;}
    /// <summary>현재0045ae60의실제8DWORD발사문맥이다.</summary>
    public Func<ushort,uint[]>? Context{get;set;}
    /// <summary>실제0046af80의문맥/현재표적판정이다.</summary>
    public Func<uint[],ushort,bool>? MayTarget{get;set;}
    /// <summary>Source함수값을복사하고현재캡처상태는공유한다.</summary>
    internal IceCannonFireHooks Copy()=>(IceCannonFireHooks)MemberwiseClone();
}
/// <summary>RawIceCannonFire.cpp의실제냉각/가상Step 파생이다.</summary>
public sealed partial class RawIceCannonFire
{
    /// <summary>같은실제자산/Host의풀이다.</summary>
    public RawSidPool Pool{get;}
    /// <summary>빌린현재문자프레임span이다.</summary>
    private readonly IReadOnlyList<PriestPlainCanonType> _frames;
    /// <summary>효과뒤현재Now/Type/가상getter를읽는동일전역이다.</summary>
    private readonly CannonFireState _state;
    /// <summary>최초값복사한아이스전용함수다.</summary>
    private readonly IceCannonFireHooks _hooks;
    /// <summary>추적/공간/실제발사/완료순서를공유하는같은공통본문이다.</summary>
    private readonly RawCannonFire _common;
    /// <summary>공통생성자가판본/표를검사하고동일상태로공통본문을구성한다.</summary>
    public RawIceCannonFire(RawSidPool pool,IReadOnlyList<OriginalTypeRecord> types,IReadOnlyList<PriestPlainCanonType> frames,CannonFireState state,GunAnimationHooks common,IceCannonFireHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);Pool=pool;_frames=frames;_state=state;_hooks=hooks.Copy();_common=new(pool,types,frames,state,common);
    }
    /// <summary>실제공장이빌리는현재아이스부모역할이다.</summary>
    public uint Type()=>_state.ReadType();
    /// <summary>선택한실제효과만요구한다.</summary>
    private static T Need<T>(T? callback)where T:Delegate=>callback??throw new InvalidOperationException("아이스 캐논 선택된 실제효과 미연결");
    /// <summary>현재비정렬DWORD를읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw,int at)=>BinaryPrimitives.ReadUInt32LittleEndian(raw[at..]);
    /// <summary>원문DWORD밖패딩을보존하고쓴다.</summary>
    private static void Put(Span<byte> raw,int at,uint value)=>BinaryPrimitives.WriteUInt32LittleEndian(raw[at..],value);
    /// <summary>+48의불투명비트를원문double로읽는다.</summary>
    private static double Deadline(IceCannonFireFields fields)=>BinaryPrimitives.ReadDoubleLittleEndian(fields.Extra.AsSpan(8));
    /// <summary>+44padding과공통dirty를건드리지않고전환기한만쓴다.</summary>
    private static void Deadline(IceCannonFireFields fields,double value)=>BinaryPrimitives.WriteDoubleLittleEndian(fields.Extra.AsSpan(8),value);
    /// <summary>공통생성뒤실제파생VT/active/현재Now+4초만추가한다.</summary>
    public void Initialize(IceCannonFireFields fields,ushort gunForm,ushort target)
    {
        _common.Initialize(fields.Common,gunForm,target);Put(fields.Common.Bytes,0,0x50b3f0);Put(fields.Common.Bytes,32,0x50b3ec);Put(fields.Extra,0,1);Deadline(fields,_state.Now+4);
    }
    /// <summary>unordered기한비교·냉각전환·현재문자구간/8DWORD문맥/표적을순서대로소비한다.</summary>
    public uint Step(IceCannonFireFields fields,int frame)
    {
        if(_state.Now>=Deadline(fields))
        {
            if(Read(fields.Extra,0)!=0){Put(fields.Extra,0,0);Need(_hooks.Cooling)(fields.Common.Parent());Deadline(fields,_state.Now+6);return unchecked((uint)frame);}Put(fields.Extra,0,1);Deadline(fields,_state.Now+4);
        }
        ushort parent=fields.Common.Parent();if(parent==0||parent>=Pool.Capacity)throw new ArgumentOutOfRangeException(nameof(fields));ReadOnlySpan<byte> raw=Pool.Slot(parent);if(raw[10]!=Type())throw new InvalidOperationException("아이스 캐논 현재부모TYPE 오류");uint current=Read(raw,36);RiftTypeFrames table=_frames[raw[10]].Frames;if(current>=table.Codes.Count)throw new ArgumentOutOfRangeException(nameof(fields));RiftTypeFrames.LetterRun run=table.Run(table.Codes[(int)current].Side);int last=unchecked(run.First+run.Count-1),next=unchecked(frame+1);if(next>last)return unchecked((uint)run.First);
        uint[] context=Need(_hooks.Context)(fields.Common.Parent()).ToArray();if(context.Length!=8)throw new ArgumentException("아이스 캐논 실제8DWORD 문맥 필요");if(!Need(_hooks.MayTarget)(context,fields.Common.Target()))return unchecked((uint)next)|0x8000;return unchecked((uint)next)|((next==last&&Read(fields.Extra,0)!=0)?0x4000u:0);
    }
    /// <summary>같은공통추적/공간/발사/완료본문의가상Step만아이스로분배한다.</summary>
    public void Frame(IceCannonFireFields fields)=>_common.FrameWithStep(fields.Common,current=>Step(fields,current));
    /// <summary>동일공통Launch에서현재아이스가상getter를소비한다.</summary>
    public bool Launch(IceCannonFireFields fields)=>_common.Launch(fields.Common);
}
