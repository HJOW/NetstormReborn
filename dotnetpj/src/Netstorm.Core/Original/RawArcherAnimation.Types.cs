using System.Buffers.Binary;

// 아처의88BYTE/현재방향자료와같은실제월드효과를보존한다.
namespace Netstorm.Core.Original;

/// <summary>캐논64BYTE와구별하는원문0x58 객체이미지다.</summary>
public sealed class ArcherAnimationFields
{
    /// <summary>공통접두와각위치/속도/가속도/발사상태의실제바이트다.</summary>
    public byte[] Bytes { get; } = new byte[88];
    /// <summary>일반기본값이나미작성패딩검사의최초패턴이다.</summary>
    public ArcherAnimationFields(byte fill = 0) => Array.Fill(Bytes, fill);
    /// <summary>외부이미지를값복사해소유한다.</summary>
    public ArcherAnimationFields(ReadOnlySpan<byte> raw) { if (raw.Length != 88) throw new ArgumentException("아처88BYTE 필요"); raw.CopyTo(Bytes); }
    /// <summary>현재Form DWORD의하위WORD다.</summary>
    public ushort Form() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(4)));
    /// <summary>현재부모DWORD의하위WORD다.</summary>
    public ushort Parent() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(8)));
    /// <summary>현재Gun Form의실제WORD다.</summary>
    public ushort GunForm() => BinaryPrimitives.ReadUInt16LittleEndian(Bytes.AsSpan(42));
    /// <summary>현재표적의실제WORD다.</summary>
    public ushort Target() => BinaryPrimitives.ReadUInt16LittleEndian(Bytes.AsSpan(44));
    /// <summary>다음회전기한의원문double이다.</summary>
    public double Time() => BinaryPrimitives.ReadDoubleLittleEndian(Bytes.AsSpan(48));
}
/// <summary>현재TYPE+110/+114의방향수와물리프레임수다.</summary>
public sealed class ArcherTypeData
{
    /// <summary>현재회전의방향수이며0은미공급이다.</summary>
    public int Directions { get; set; }
    /// <summary>현재물리프레임수다.</summary>
    public int FrameCount { get; set; }
    /// <summary>현재표의명시입력을준비한다.</summary>
    public ArcherTypeData(int directions = 0, int frameCount = 0) { Directions = directions; FrameCount = frameCount; }
}
/// <summary>매프레임읽는원문시각/경과와mutable 발사각상태상수다.</summary>
public sealed class ArcherAnimationState
{
    /// <summary>현재게임시각이다.</summary>
    public double Now { get; set; }
    /// <summary>현재게임한프레임의원문경과다.</summary>
    public double Delta { get; set; }
    /// <summary>독립입력의최초71역할이다.</summary>
    public uint Type { get; set; } = 71;
    /// <summary>장기Client의현재Sun/Wind Archer 역할이다.</summary>
    public UIntBox? CurrentType { get; set; }
    /// <summary>실제sunDisc 표시83이다.</summary>
    public uint DisplayType { get; set; } = 83;
    /// <summary>발사생성뒤최초각속도다.</summary>
    public float InitialSpeed { get; set; } = 180;
    /// <summary>발사생성뒤최초각가속도다.</summary>
    public float InitialAcceleration { get; set; } = 180;
    /// <summary>실제발사각앞의windup 회수다.</summary>
    public float Winding { get; set; } = 4;
    /// <summary>실제발사각의앞바이어스다.</summary>
    public float FireBias { get; set; } = 30;
    /// <summary>가속도를뒤집는발사앞각도다.</summary>
    public float SpringOffset { get; set; } = 180;
    /// <summary>발사뒤spring을복원하는각도다.</summary>
    public float ResetOffset { get; set; } = 180;
    /// <summary>발사종료의각속도상한이다.</summary>
    public float FinishSpeed { get; set; } = 180;
    /// <summary>정규화된발사종료각도다.</summary>
    public float FinishAngle { get; set; } = 270;
    /// <summary>발사종료각의절대오차상한이다.</summary>
    public float FinishTolerance { get; set; } = 30;
    /// <summary>같은현재Client checking/원문진단장치다.</summary>
    public BoardAngleDiagnostics Angles { get; set; } = new();
    /// <summary>각소비위치에서현재역할을읽는다.</summary>
    public uint ReadType() => CurrentType?.Value ?? Type;
}
/// <summary>공통캐논프레임표와섞지않는아처전용효과묶음이다.</summary>
public sealed class ArcherAnimationHooks
{
    /// <summary>같은실제월드의중심/공간/소리/통지/투사체효과다.</summary>
    public GunAnimationHooks Common { get; set; } = new();
    /// <summary>표적0회전의현재aim BYTE를읽는다.</summary>
    public Func<ushort, int>? AimDirection { get; set; }
    /// <summary>출력WORD bit0 쓰기뒤의실제공간통지다.</summary>
    public Action<ushort>? NotifySurface { get; set; }
    /// <summary>함수값을복사하고현재상태getter 수명은공유한다.</summary>
    internal ArcherAnimationHooks Copy() => new() { Common = Common.Copy(), AimDirection = AimDirection, NotifySurface = NotifySurface };
}
