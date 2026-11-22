using System.Buffers.Binary;

// 미사일판본별raw이미지와현재원문타입/가상getter 상태를보존한다.
namespace Netstorm.Core.Original;

/// <summary>PATCH128/CD앞104BYTE의원문객체와미작성padding이다.</summary>
public sealed class MissileFields
{
    /// <summary>원문최대128BYTE이며CD의바깥24BYTE는따로보존한다.</summary>
    public byte[] Bytes{get;}=new byte[128];
    /// <summary>원문미작성패딩을위한초기패턴이다.</summary>
    public MissileFields(byte fill=0)=>Array.Fill(Bytes,fill);
    /// <summary>외부이미지를값복사해소유한다.</summary>
    public MissileFields(ReadOnlySpan<byte> raw){if(raw.Length!=128)throw new ArgumentException("미사일128BYTE 필요");raw.CopyTo(Bytes);}
    /// <summary>현재Form DWORD의하위WORD다.</summary>
    public ushort Form()=>unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(4)));
    /// <summary>현재표시자산부모DWORD의하위WORD다.</summary>
    public ushort Parent()=>unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(8)));
    /// <summary>현재발사자DWORD의하위WORD다.</summary>
    public ushort Firing()=>unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(20)));
    /// <summary>현재표적DWORD의하위WORD다.</summary>
    public ushort Target()=>unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(24)));
}
/// <summary>원문TYPE표에서생성/이동이직접읽는현재보충자료다.</summary>
public sealed class MissileTypeData
{
    /// <summary>현재발사TYPE+44의실제사거리다.</summary>
    public float Range{get;set;}
    /// <summary>TYPE+110의실제방향수이며0이면표시프레임을고르지않는다.</summary>
    public int FrameCount{get;set;}
    /// <summary>TYPE+70의실제발사음향토큰이다.</summary>
    public uint FiringSound{get;set;}
    /// <summary>TYPE+74의실제충돌음향토큰이다.</summary>
    public uint ImpactSound{get;set;}
    /// <summary>TYPE+48의현재float DPS다.</summary>
    public float DamagePerSecond{get;set;}
    /// <summary>TYPE+1b0의별개비행표적피해선택이다.</summary>
    public bool SpecialFlyerDamage{get;set;}
    /// <summary>TYPE+1b4의교체피해이며교체뒤0.6을다시곱하지않는다.</summary>
    public float FlyerDamagePerSecond{get;set;}
}
/// <summary>매소비위치의현재게임시각/원문역할/단위배율이다.</summary>
public sealed class MissileState
{
    /// <summary>PATCH Handle두관문이같이빌리는현재checking DWORD다.</summary>
    public RawCheckingFlag Checking{get;set;}=new();
    /// <summary>정지시유지되는현재게임시각이다.</summary>
    public double Now{get;set;}
    /// <summary>독립입력의썬캐논128역할이다.</summary>
    public uint SunCannonType{get;set;}=128;
    /// <summary>현재sunDisc83표시역할이다.</summary>
    public uint SunDiscType{get;set;}=83;
    /// <summary>독립입력의썬아처71역할이다.</summary>
    public uint SunArcherType{get;set;}=71;
    /// <summary>독립입력의크로스보우140역할이다.</summary>
    public uint CrossbowType{get;set;}=140;
    /// <summary>독립입력의썬더캐논131역할이다.</summary>
    public uint ThunderCannonType{get;set;}=131;
    /// <summary>현재썬더표시132역할이다.</summary>
    public uint ThunderMissileType{get;set;}=132;
    /// <summary>충돌튜토리얼알림의현재스톤타워78역할이다.</summary>
    public uint StoneTowerType{get;set;}=78;
    /// <summary>실제005429c8의충돌입력명령토큰이다.</summary>
    public uint HitCommand{get;set;}=0x020000b5;
    /// <summary>일반사거리종료의정수여유다.</summary>
    public int RangeMargin{get;set;}=4;
    /// <summary>썬더사거리종료의별개정수여유다.</summary>
    public int ThunderRangeMargin{get;set;}=10;
    /// <summary>같은실제Host가부착하는현재28역할이다.</summary>
    public uint ProcessType{get;set;}=28;
    /// <summary>incomingCorner충돌의공통136음향TYPE다.</summary>
    public uint CornerImpactType{get;set;}=136;
    /// <summary>현재BoardCoord가읽는타일픽셀가로배율이다.</summary>
    public float ScaleX{get;set;}=16;
    /// <summary>현재BoardCoord의별개세로배율이다.</summary>
    public float ScaleY{get;set;}=11;
    /// <summary>독립입력의아이스발사자129역할이다.</summary>
    public uint RainCannonType{get;set;}=129;
    /// <summary>현재아이스충돌/반사표시130역할이다.</summary>
    public uint IceMissileType{get;set;}=130;
    /// <summary>충돌뒤회피난수를선택하는현재146역할이다.</summary>
    public uint FloaterType{get;set;}=146;
    /// <summary>난수반환뒤읽는부호있는00540bbc컷이다.</summary>
    public int AvoidanceCutoff{get;set;}=800;
    /// <summary>같은장기00541258역할을빌린다.</summary>
    public UIntBox? SharedSunCannonType{get;set;}
    /// <summary>같은장기00541174역할을빌린다.</summary>
    public UIntBox? SharedSunArcherType{get;set;}
    /// <summary>같은장기00541288역할을빌린다.</summary>
    public UIntBox? SharedCrossbowType{get;set;}
    /// <summary>같은장기00541264역할을빌린다.</summary>
    public UIntBox? SharedThunderCannonType{get;set;}
    /// <summary>같은장기0054125c역할을빌린다.</summary>
    public UIntBox? SharedRainCannonType{get;set;}
    /// <summary>현재썬캐논역할을소비위치에서읽는다.</summary>
    public uint ReadSunCannonType()=>SharedSunCannonType?.Value??SunCannonType;
    /// <summary>현재썬아처역할을소비위치에서읽는다.</summary>
    public uint ReadSunArcherType()=>SharedSunArcherType?.Value??SunArcherType;
    /// <summary>현재크로스보우역할을소비위치에서읽는다.</summary>
    public uint ReadCrossbowType()=>SharedCrossbowType?.Value??CrossbowType;
    /// <summary>현재썬더캐논역할을소비위치에서읽는다.</summary>
    public uint ReadThunderCannonType()=>SharedThunderCannonType?.Value??ThunderCannonType;
    /// <summary>발사자반납뒤대체피해도같은현재아이스역할을읽는다.</summary>
    public uint ReadRainCannonType()=>SharedRainCannonType?.Value??RainCannonType;
}
