using System.Buffers.Binary;

// 크로스보우의기한앞관문/현재값재조회와원문발사프레임을보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawCrossbowAnimation.h의현재시각/가상getter/140역할이다.</summary>
public sealed class CrossbowAnimationState
{
    /// <summary>같은Kernel이공급하는현재시각이다.</summary>
    public double Now { get; set; }
    /// <summary>대여가없는독립입력의최초140역할이다.</summary>
    public uint Type { get; set; } = 140;
    /// <summary>원문arrow의현재표시81이다.</summary>
    public uint DisplayType { get; set; } = 81;
    /// <summary>원문가상float발사속도다.</summary>
    public float Speed { get; set; } = 35;
    /// <summary>원문가상float프레임지연이다.</summary>
    public float Delay { get; set; } = .2f;
    /// <summary>실제00541288의기존WindArcher 역할을빌린다.</summary>
    public UIntBox? CurrentType { get; set; }
    /// <summary>각소비위치에서현재전체TYPE역할을읽는다.</summary>
    public uint ReadType() => CurrentType?.Value ?? Type;
}
/// <summary>RawCrossbowAnimation.cpp의실제PATCH64BYTE발사본문이다.</summary>
public sealed partial class RawCrossbowAnimation
{
    /// <summary>같은자산/Host의실제풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>원문span처럼빌린현재HP/TYPE표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원문span처럼빌린현재방향수/물리프레임수다.</summary>
    private readonly IReadOnlyList<ArcherTypeData> _data;
    /// <summary>효과뒤에도다시읽는현재시각/속도/표시역할이다.</summary>
    private readonly CrossbowAnimationState _state;
    /// <summary>최초함수값을복사한월드효과다.</summary>
    private readonly GunAnimationHooks _hooks;
    /// <summary>PATCH판본/실제표폭/현재역할을검사하고같은상태를빌린다.</summary>
    public RawCrossbowAnimation(RawSidPool pool,IReadOnlyList<OriginalTypeRecord> types,IReadOnlyList<ArcherTypeData> data,CrossbowAnimationState state,GunAnimationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);ArgumentNullException.ThrowIfNull(types);ArgumentNullException.ThrowIfNull(data);ArgumentNullException.ThrowIfNull(state);ArgumentNullException.ThrowIfNull(hooks);
        if(pool.Edition!=OriginalEdition.Patch1078||types.Count!=188||data.Count!=types.Count||state.ReadType()>=types.Count)throw new ArgumentException("크로스보우 실제판본/TYPE자료 오류");Pool=pool;_types=types;_data=data;_state=state;_hooks=hooks.Copy();
    }
    /// <summary>실제공장의현재부모역할이다.</summary>
    public uint Type()=>_state.ReadType();
    /// <summary>선택한실제효과만요구한다.</summary>
    private static T Need<T>(T? callback)where T:Delegate=>callback??throw new InvalidOperationException("크로스보우 선택된 실제효과 미연결");
    /// <summary>현재원문DWORD를읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw,int at)=>BinaryPrimitives.ReadUInt32LittleEndian(raw[at..]);
    /// <summary>패딩을보존하며DWORD폭만쓴다.</summary>
    private static void Put(Span<byte> raw,int at,uint value)=>BinaryPrimitives.WriteUInt32LittleEndian(raw[at..],value);
    /// <summary>dirty를추가하지않고double필드만쓴다.</summary>
    private static void Time(GunAnimationFields fields,double value)=>BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(48),value);
    /// <summary>현재float좌표쌍을값으로읽는다.</summary>
    private static float[] Point(GunAnimationFields fields,int at)=>[BinaryPrimitives.ReadSingleLittleEndian(fields.Bytes.AsSpan(at)),BinaryPrimitives.ReadSingleLittleEndian(fields.Bytes.AsSpan(at+4))];
    /// <summary>실제파생VT/식별자/두WORD/시각/Rest만초기화한다.</summary>
    public void Initialize(GunAnimationFields fields,ushort gunForm,ushort target)
    {
        if(gunForm==0||gunForm>=Pool.Capacity)throw new ArgumentOutOfRangeException(nameof(gunForm));Put(fields.Bytes,0,0x50b120);Put(fields.Bytes,4,0);Put(fields.Bytes,8,0);Put(fields.Bytes,32,0x50b11c);fields.Bytes[40]=0xff;BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(42),gunForm);BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(44),target);Time(fields,_state.Now);Put(fields.Bytes,56,0);
    }
    /// <summary>현재부모SID와같은실제역할을검사한다.</summary>
    private ushort Parent(GunAnimationFields fields){ushort parent=fields.Parent();if(parent==0||parent>=Pool.Capacity||Pool.Slot(parent)[10]!=Type())throw new InvalidOperationException("크로스보우 현재부모TYPE/번호 오류");return parent;}
    /// <summary>현재부모를한번잡고같은SID에Unpop→DWORD쓰기→Repop40을수행한다.</summary>
    private void SetFrame(GunAnimationFields fields,int frame)
    {
        Action<ushort,uint> unpop=Need(_hooks.Unpop),repop=Need(_hooks.Repop);ushort parent=Parent(fields);unpop(parent,0);Put(Pool.AllocatedBytes(parent),36,unchecked((uint)frame));repop(parent,0x40);
    }
    /// <summary>필요한Gun/Form을모두통지전에보존하고삭제뒤필드를읽지않는다.</summary>
    private void Finish(GunAnimationFields fields){Action<ushort,uint,uint,uint> notify=Need(_hooks.Notify);Action<ushort,uint> kill=Need(_hooks.Kill);ushort gun=fields.GunForm(),form=fields.Form();notify(gun,1,6,7);kill(form,0x10);}
    /// <summary>최초표적상태를검사하고코너효과뒤현재인수로실제arrow생성을요구한다.</summary>
    public bool Launch(GunAnimationFields fields)
    {
        _=Parent(fields);ushort target=fields.Target();if(target==0||target>=Pool.Capacity)throw new ArgumentOutOfRangeException(nameof(fields));ReadOnlySpan<byte> raw=Pool.Slot(target);if((raw[11]&3)!=0)return false;if(raw[10]>=_types.Count)throw new ArgumentOutOfRangeException(nameof(fields));if((_types[raw[10]].Flags1&0x10)==0)return false;
        Func<ushort,ushort,float[]> point=Need(_hooks.TargetPoint);Action<GunProjectileRequest> create=Need(_hooks.CreateProjectile);float[] corner=point(fields.Parent(),fields.Target()).ToArray();var request=new GunProjectileRequest{Firing=fields.Parent(),Target=fields.Target(),Speed=_state.Speed,DisplayType=_state.DisplayType,EffectType=0,TargetPosition=Point(fields,12),TargetCenter=corner,FiringPosition=[-10000,-10000]};create(request);return true;
    }
    /// <summary>기한검사를맨앞에서하고방향첫프레임발사/완료또는역순한프레임을진행한다.</summary>
    public void Frame(GunAnimationFields fields)
    {
        if(!(fields.Time()<=_state.Now))return;ushort parent=Parent(fields);ReadOnlySpan<byte> source=Pool.Slot(parent);ushort target=fields.Target();if(target==0||target>=Pool.Capacity)throw new ArgumentOutOfRangeException(nameof(fields));ReadOnlySpan<byte> raw=Pool.Slot(target);bool dead=(raw[11]&3)!=0;
        if(!dead){Func<ushort,float[]> center=Need(_hooks.Center);Put(fields.Bytes,12,Read(raw,14));Put(fields.Bytes,16,Read(raw,18));float[] current=center(target).ToArray();Put(fields.Bytes,20,BitConverter.SingleToUInt32Bits(current[0]));Put(fields.Bytes,24,BitConverter.SingleToUInt32Bits(current[1]));}
        ArcherTypeData data=_data[source[10]];if(data.Directions<=0||data.FrameCount/data.Directions<=0)throw new InvalidOperationException("크로스보우 현재방향/프레임자료 미공급");int per=data.FrameCount/data.Directions,frame=unchecked((int)Read(source,36));if(frame%per==0){SetFrame(fields,unchecked(frame+per-1));if(!Launch(fields)&&!dead)return;Finish(fields);return;}
        SetFrame(fields,unchecked(frame-1));fields.Bytes[40]|=8;Time(fields,_state.Now);Time(fields,_state.Now+(double)_state.Delay);
    }
}
