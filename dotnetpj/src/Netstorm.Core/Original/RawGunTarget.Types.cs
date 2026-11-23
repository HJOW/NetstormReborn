// 실제 Gun 검색의 값형 문맥과 현재 Player/진단 상태를 정의한다.
namespace Netstorm.Core.Original;

/// <summary>원문 검색·발사에서 값 복사하는 여덟 DWORD 문맥이다.</summary>
public readonly record struct GunTargetQuery
{
    /// <summary>발사자의 Pop X다.</summary>
    public float X { get; init; }
    /// <summary>발사자의 Pop Y다.</summary>
    public float Y { get; init; }
    /// <summary>발자국 반폭을 뺀 현재 X 중심이다.</summary>
    public float CenterX { get; init; }
    /// <summary>발자국 반높이를 뺀 현재 Y 중심이다.</summary>
    public float CenterY { get; init; }
    /// <summary>현재 발사자 TYPE 번호다.</summary>
    public uint Type { get; init; }
    /// <summary>현재 공격 모드의 여덟 방향 값이다.</summary>
    public int Direction { get; init; }
    /// <summary>동맹 표를 읽는 현재 소유자다.</summary>
    public uint Owner { get; init; }
    /// <summary>현재 썬 캐논의 발사 가능 그림 프레임이다.</summary>
    public int Frame { get; init; }
    /// <summary>같은 여덟 DWORD를 산술 변환 없이 값형으로 해석한다.</summary>
    public static GunTargetQuery FromWords(uint[] words)
    {
        ArgumentNullException.ThrowIfNull(words);
        if (words.Length != 8) throw new ArgumentException("Gun 발사 문맥의 실제8DWORD", nameof(words));
        return new() { X = BitConverter.UInt32BitsToSingle(words[0]), Y = BitConverter.UInt32BitsToSingle(words[1]), CenterX = BitConverter.UInt32BitsToSingle(words[2]), CenterY = BitConverter.UInt32BitsToSingle(words[3]), Type = words[4], Direction = unchecked((int)words[5]), Owner = words[6], Frame = unchecked((int)words[7]) };
    }
}

/// <summary>경계를 포함하는 원문의 float 사각형이다.</summary>
/// <param name="Left">왼쪽 경계다.</param>
/// <param name="Top">위쪽 경계다.</param>
/// <param name="Right">오른쪽 경계다.</param>
/// <param name="Bottom">아래쪽 경계다.</param>
public readonly record struct GunTargetRect(float Left, float Top, float Right, float Bottom);

/// <summary>현재 TYPE에서 읽는 사거리·테마·네 방향 프레임·확장 높이다.</summary>
public sealed class GunTargetType
{
    /// <summary>칸 단위 사거리이며 아처는 엄격히 작은 거리만 허용한다.</summary>
    public float Range { get; set; }
    /// <summary>Rain1/Thunder2 캐논의 고정 방향 테마다.</summary>
    public int Theme { get; set; }
    /// <summary>L/M/N/O의 첫 프레임이며 -1은 사용 시0으로 변환한다.</summary>
    public int[] DirectionFrames { get; set; } = new int[4];
    /// <summary>검색 발자국에 더하는 현재 TYPE 높이다.</summary>
    public int HeightExtension { get; set; }
}

/// <summary>같은 현재 Client 전역/Player 표와 독립 입력의 경계다.</summary>
public sealed class GunTargetState
{
    /// <summary>매 표적 판정에서 읽는 친군 사격 DWORD 뷰다.</summary>
    public RawCheckingFlag FriendlyFire { get; set; } = new();
    /// <summary>PATCH의 미지원 group 진단에 쓰는 현재 checking이다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>원문 사정선·최종 발자국 진단 그리기의 현재 선택이다.</summary>
    public bool ShowRanges { get; set; }
    /// <summary>썬 캐논 역할 번호다.</summary>
    public uint SunCannonType { get; set; } = 128;
    /// <summary>썬 펜스 역할 번호다.</summary>
    public uint SunFenceType { get; set; } = 136;
    /// <summary>윈드 아처 역할 번호다.</summary>
    public uint WindArcherType { get; set; } = 140;
    /// <summary>썬더 아처 역할 번호다.</summary>
    public uint ThunderArcherType { get; set; } = 148;
    /// <summary>원문 크로스보우 관측각의 절반이다.</summary>
    public float WindHalfAngle { get; set; } = 30;
    /// <summary>SHP 포구의 현재 가로 픽셀 배율이다.</summary>
    public float ScaleX { get; set; } = 16;
    /// <summary>SHP 포구의 현재 세로 픽셀 배율이다.</summary>
    public float ScaleY { get; set; } = 11;
    /// <summary>현재 조회기가 없을 때만 읽는 독립 소유자별 동맹 마스크다.</summary>
    public uint[] AllyMasks { get; } = new uint[9];
    /// <summary>현재 조회기가 없을 때 읽는 독립 Player 참여 플래그다.</summary>
    public byte[] PlayerFlags { get; } = new byte[9];
    /// <summary>같은 실제 Player+54를 원문 읽기 위치에서 조회한다.</summary>
    public Func<uint, uint>? AllyMaskRead { get; set; }
    /// <summary>진단 출력 조건에서 같은 실제 Player+50을 조회한다.</summary>
    public Func<uint, uint>? PlayerFlagsRead { get; set; }
    /// <summary>같은 현재 Client의 보드 각도 진단이다.</summary>
    public BoardAngleDiagnostics Angles { get; set; } = new();
    /// <summary>현재 소유자의 동맹 비트 마스크를 읽는다.</summary>
    public uint AllyMask(uint owner)
    {
        if (owner >= AllyMasks.Length) throw new ArgumentOutOfRangeException(nameof(owner));
        return AllyMaskRead is { } read ? read(owner) : AllyMasks[owner];
    }
    /// <summary>현재 참여 플래그를 실제 진단 분기가 선택했을 때 읽는다.</summary>
    public uint ReadPlayerFlags(uint owner)
    {
        if (owner >= PlayerFlags.Length) throw new ArgumentOutOfRangeException(nameof(owner));
        return PlayerFlagsRead is { } read ? read(owner) : PlayerFlags[owner];
    }
}

/// <summary>검색·판정을 대체하지 않는 실제 진단 출력 효과다.</summary>
public sealed class GunTargetHooks
{
    /// <summary>사정선은false, 최종 표적 발자국은true로 그리는 경계다.</summary>
    public Action<GunTargetRect, bool>? DrawRange { get; set; }
    /// <summary>원문의 함수 묶음처럼 콜백 값만 복사한다.</summary>
    internal GunTargetHooks Copy() => (GunTargetHooks)MemberwiseClone();
}
