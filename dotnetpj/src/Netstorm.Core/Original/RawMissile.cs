using System.Buffers.Binary;

// 판본별실제미사일생성의저장폭/현재전역과자산효과순서를보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawMissile.cpp의공통raw접근과실제생성본문이다.</summary>
public sealed partial class RawMissile
{
    /// <summary>판본별원문필드offset의값형묶음이다.</summary>
    /// <param name="Speed">float속도위치다.</param><param name="Allowance">float충돌여유다.</param>
    /// <param name="Origin">최초float좌표쌍이다.</param><param name="Position">기록표적좌표쌍이다.</param>
    /// <param name="Endpoint">float종점좌표쌍이다.</param><param name="Box">실제float사각형이다.</param>
    /// <param name="End">double종료시각이다.</param><param name="Start">double시작시각이다.</param>
    /// <param name="Previous">직전float좌표쌍이다.</param>
    private readonly record struct Layout(int Speed,int Allowance,int Origin,int Position,int Endpoint,int Box,int End,int Start,int Previous);
    /// <summary>PATCH128BYTE의실제필드배치다.</summary>
    private static readonly Layout PatchLayout=new(48,52,56,64,72,88,104,112,120);
    /// <summary>CD104BYTE의별개실제필드배치다.</summary>
    private static readonly Layout CdLayout=new(32,36,40,48,56,64,80,88,96);
    /// <summary>자산/Host와같은실제풀이다.</summary>
    public RawSidPool Pool{get;}
    /// <summary>원문span처럼빌린현재타입자료다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원문span처럼빌린현재사거리/음향/피해/방향자료다.</summary>
    private readonly IReadOnlyList<MissileTypeData> _missileTypes;
    /// <summary>소비위치마다읽는현재전역이다.</summary>
    private readonly MissileState _state;
    /// <summary>최초값복사한실제월드함수다.</summary>
    private readonly MissileHooks _hooks;
    /// <summary>판본/두표크기/원문픽셀배율/충돌TYPE를검사하고현재상태를빌린다.</summary>
    public RawMissile(RawSidPool pool,IReadOnlyList<OriginalTypeRecord> types,IReadOnlyList<MissileTypeData> missileTypes,MissileState state,MissileHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);ArgumentNullException.ThrowIfNull(types);ArgumentNullException.ThrowIfNull(missileTypes);ArgumentNullException.ThrowIfNull(state);ArgumentNullException.ThrowIfNull(hooks);
        if(types.Count!=(pool.Edition==OriginalEdition.Patch1078?188:171)||missileTypes.Count!=types.Count)throw new ArgumentException("미사일 실제판본/보충자료 폭 오류");if(!float.IsFinite(state.ScaleX)||!float.IsFinite(state.ScaleY)||state.ScaleX<=0||state.ScaleY<=0||state.CornerImpactType>=types.Count)throw new ArgumentException("미사일 픽셀배율/충돌TYPE 오류");Pool=pool;_types=types;_missileTypes=missileTypes;_state=state;_hooks=hooks.Copy();
    }
    /// <summary>실제같은Host가부착할현재프로세스역할이다.</summary>
    public uint ProcessType()=>_state.ProcessType;
    /// <summary>선택한실제효과만요구한다.</summary>
    private static T Need<T>(T? callback)where T:Delegate=>callback??throw new InvalidOperationException("미사일 선택된 실제효과 미연결");
    /// <summary>원문DWORD의불투명비트를읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw,int at)=>BinaryPrimitives.ReadUInt32LittleEndian(raw[at..]);
    /// <summary>주변padding을보존하며DWORD만쓴다.</summary>
    private static void Put(Span<byte> raw,int at,uint value)=>BinaryPrimitives.WriteUInt32LittleEndian(raw[at..],value);
    /// <summary>원문float저장값을읽는다.</summary>
    private static float F(ReadOnlySpan<byte> raw,int at)=>BinaryPrimitives.ReadSingleLittleEndian(raw[at..]);
    /// <summary>원문float폭으로저장한다.</summary>
    private static void F(Span<byte> raw,int at,float value)=>BinaryPrimitives.WriteSingleLittleEndian(raw[at..],value);
    /// <summary>원문double저장값을읽는다.</summary>
    private static double D(ReadOnlySpan<byte> raw,int at)=>BinaryPrimitives.ReadDoubleLittleEndian(raw[at..]);
    /// <summary>원문double폭으로저장한다.</summary>
    private static void D(Span<byte> raw,int at,double value)=>BinaryPrimitives.WriteDoubleLittleEndian(raw[at..],value);
    /// <summary>현재float좌표쌍을값으로읽는다.</summary>
    private static float[] Point(ReadOnlySpan<byte> raw,int at)=>[F(raw,at),F(raw,at+4)];
    /// <summary>두float좌표만원문폭으로쓴다.</summary>
    private static void Point(Span<byte> raw,int at,float[] point){F(raw,at,point[0]);F(raw,at+4,point[1]);}
    /// <summary>원문정상BoardCoord의열린유효범위다.</summary>
    private static bool Valid(float[] point)=>point[0]>0&&point[1]>0&&point[0]<256&&point[1]<256;
    /// <summary>원문정상좌표Clamp를값복사해수행한다.</summary>
    private static float[] Clamp(float[] point)=>[Math.Clamp(point[0],1,255),Math.Clamp(point[1],1,255)];
    /// <summary>원문유한좌표Distance의double산술이다.</summary>
    private static double Distance(float[] first,float[] second){double dx=(double)first[0]-second[0],dy=(double)first[1]-second[1];return Math.Sqrt(dx*dx+dy*dy);}
    /// <summary>실제ftol64절삭뒤하위DWORD를보존하며불확정정수의하위0을유지한다.</summary>
    private static int DamageInteger(float value){double exact=value;if(!double.IsFinite(exact)||exact>=9223372036854775808.0||exact< -9223372036854775808.0)return 0;return unchecked((int)(uint)(long)exact);}
    /// <summary>SID/현재TYPE의물리범위만검사하고free/dead는선택본문에맡긴다.</summary>
    private ReadOnlySpan<byte> Slot(ushort sid,bool allowZero=false){if((sid==0&&!allowZero)||sid>=Pool.Capacity)throw new ArgumentOutOfRangeException(nameof(sid));ReadOnlySpan<byte> raw=Pool.Slot(sid);if(raw[10]>=_types.Count)throw new ArgumentOutOfRangeException(nameof(sid));return raw;}
    /// <summary>표시자산의PATCH DWORD/CD BYTE 프레임폭만쓴다.</summary>
    private void FrameNumber(ushort sid,uint frame){Span<byte> raw=Pool.AllocatedBytes(sid);if(Pool.Edition==OriginalEdition.Patch1078)Put(raw,36,frame);else raw[34]=unchecked((byte)frame);}
    /// <summary>실제생성필드/현재자산/소유자/음향/Pop/비행효과순서를수행한다.</summary>
    public ushort Construct(MissileFields fields,GunProjectileRequest request)
    {
        bool patch=Pool.Edition==OriginalEdition.Patch1078,reflected=patch&&request.Ricochet!=0;Layout layout=patch?PatchLayout:CdLayout;ReadOnlySpan<byte> firing=Slot(request.Firing),target=Slot(request.Target,reflected);bool archer=firing[10]==_state.ReadSunArcherType(),groupZero=archer||(patch&&firing[10]==_state.ReadCrossbowType()),cannon=firing[10]==_state.ReadSunCannonType()||(patch&&(firing[10]==_state.ReadThunderCannonType()||firing[10]==_state.ReadRainCannonType()));
        if(reflected?firing[10]!=_state.IceMissileType:request.Ricochet!=0||(!groupZero&&!cannon)||Valid(request.FiringPosition))throw new InvalidOperationException("미사일 미복원반사/부모/직접포구 경계");if(!reflected&&_types[firing[10]].Group!=(groupZero?0:1))throw new InvalidOperationException("미사일 실제부모group 오류");if(!float.IsFinite(request.Speed)||request.Speed<=0||!Valid(request.TargetPosition))throw new ArgumentException("미사일 생성좌표/속도 오류");
        if((!Valid(request.FiringPosition)&&_hooks.Muzzle is null)||(!reflected&&!archer&&_hooks.Hotspot is null)||(!reflected&&!groupZero&&_hooks.RayEnd is null)||(target[10]>=10&&_hooks.TargetDimensions is null)||_hooks.Create is null||_hooks.Owner is null||_hooks.Pop is null||(request.EffectType!=0&&_hooks.FlightEffect is null))throw new InvalidOperationException("미사일 실제생성/Pop/비행효과 경계미연결");if(request.DisplayType>=_types.Count)throw new ArgumentOutOfRangeException(nameof(request));
        Put(fields.Bytes,0,patch?0x50d630u:0x501ca0u);Put(fields.Bytes,4,0);Put(fields.Bytes,8,0);
        // 원문초기사각형의네float는모두invalid좌표다.
        for(int at=layout.Box;at<layout.Box+16;at+=4)F(fields.Bytes,at,-10000);
        D(fields.Bytes,layout.End,0);D(fields.Bytes,layout.Start,0);Put(fields.Bytes,20,request.Firing);Put(fields.Bytes,24,request.Target);F(fields.Bytes,layout.Speed,request.Speed);Put(fields.Bytes,12,request.Ricochet);Put(fields.Bytes,16,0);Point(fields.Bytes,layout.Previous,[-10000,-10000]);bool corner=!reflected&&Valid(request.TargetCenter);
        if(patch){if(corner&&!groupZero){Put(fields.Bytes,32,0);Put(fields.Bytes,36,0);}Put(fields.Bytes,40,0);Put(fields.Bytes,44,0);Point(fields.Bytes,80,request.TargetCenter);}
        float[] origin=(Valid(request.FiringPosition)?request.FiringPosition:Need(_hooks.Muzzle)(request.Firing)).ToArray();int typeIndex=firing[10];
        if(!reflected&&!archer)
        {
            int[] hot=Need(_hooks.Hotspot)(request.Firing).ToArray();origin[0]=(float)((double)origin[0]+(double)hot[0]/_state.ScaleX);origin[1]=(float)((double)origin[1]+(double)hot[1]/_state.ScaleY);origin[0]=(float)((double)origin[0]-(double)_types[typeIndex].HotspotX/_state.ScaleX);origin[1]=(float)((double)origin[1]-(double)_types[typeIndex].HotspotY/_state.ScaleY);
        }
        int[] dimensions=target[10]<10?[3,3]:Need(_hooks.TargetDimensions)(request.Target).ToArray();float[] targetCenter=[(float)((double)request.TargetPosition[0]-dimensions[0]*.5),(float)((double)request.TargetPosition[1]-dimensions[1]*.5)];Point(fields.Bytes,layout.Endpoint,targetCenter);Point(fields.Bytes,layout.Position,corner?request.TargetCenter:request.TargetPosition);Put(fields.Bytes,28,corner?1u:0);
        float[] endpoint=Clamp(reflected?targetCenter:groupZero?(patch&&corner?request.TargetCenter:targetCenter):Need(_hooks.RayEnd)(origin.ToArray(),(corner?request.TargetCenter:targetCenter).ToArray(),request.Firing).ToArray());origin=Clamp(origin);Point(fields.Bytes,layout.Endpoint,endpoint);Point(fields.Bytes,layout.Origin,origin);if(!Valid(origin)||!Valid(endpoint)||((firing[11]&1)!=0&&(target[11]&1)!=0))throw new InvalidOperationException("미사일 생성좌표/자산 원문관문");
        F(fields.Bytes,layout.Allowance,0);float distance=(float)Distance(origin,endpoint);if(distance<0||distance>=256)throw new ArgumentOutOfRangeException(nameof(request));D(fields.Bytes,layout.Start,_state.Now);D(fields.Bytes,layout.End,(double)distance/request.Speed+_state.Now);
        ushort asset=Need(_hooks.Create)(request.DisplayType,2);_=Slot(asset);int displayIndex=(int)request.DisplayType;if(_missileTypes[displayIndex].FrameCount>0){if(request.DisplayType==_state.SunDiscType)FrameNumber(asset,1);else FrameNumber(asset,unchecked((uint)Need(_hooks.DirectionFrame)(origin.ToArray(),endpoint.ToArray(),request.DisplayType)));}
        Need(_hooks.Owner)(asset,firing[patch?34:32]);uint sound=_missileTypes[firing[10]].FiringSound!=0?_missileTypes[firing[10]].FiringSound:_missileTypes[displayIndex].FiringSound;if(sound!=0)Need(_hooks.FiringSound)(origin[0],origin[1],sound);Need(_hooks.Pop)(asset,origin[0],origin[1],0);if(request.EffectType!=0)Need(_hooks.FlightEffect)(request.EffectType,asset);return asset;
    }
}
