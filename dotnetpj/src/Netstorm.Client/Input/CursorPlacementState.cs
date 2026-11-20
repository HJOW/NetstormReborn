// 커서의 위치 허용과 기준점/전체 폭·높이를 수명 상태와 별개로 보관한다.
namespace Netstorm.Client.Input;

/// <summary>RawCursorPlacement.h의 장기 데이터다. Query/Prepare 계산과 외부 장치는 별도다.</summary>
public sealed class CursorPlacementState
{
    /// <summary>55a2dc의 별도 배치 경고 플래그다.</summary>
    public uint PlacementFlags { get; set; }
    /// <summary>55a290의 현재 배치 허용 DWORD다.</summary>
    public uint PlacementAllowed { get; set; }
    /// <summary>55a294의 현재 설명 허용 DWORD다.</summary>
    public uint DescriptionAllowed { get; set; }
    /// <summary>55a298의 현재 공간 허용 DWORD다.</summary>
    public uint SpaceAllowed { get; set; }
    /// <summary>55a29c의 현재 연관 지역 허용 DWORD다.</summary>
    public uint SourceAllowed { get; set; }
    /// <summary>59ab2c의 현재 공간 차단 결과다.</summary>
    public uint Obstruction { get; set; }
    /// <summary>5427f4의 원문 최초 Gump 수신 명령이다.</summary>
    public uint AcceptEvent { get; set; } = 0x0200003f;
    /// <summary>5325e0의 현재 글자/가로 여백이며 최초16이다.</summary>
    public int LabelSpacing { get; set; } = 16;
    /// <summary>55a2b0..bc의 기준점X/Y·전체폭/높이이며 우하 좌표가 아니다.</summary>
    public int[] Bounds { get; } = new int[4];
    /// <summary>첫 그림의 signed WORD hotspot X다.</summary>
    public int HotspotX { get; set; }
    /// <summary>첫 그림의 signed WORD hotspot Y다.</summary>
    public int HotspotY { get; set; }
    /// <summary>첫 그림의 signed WORD 폭이다.</summary>
    public int ImageWidth { get; set; }
    /// <summary>첫 그림의 signed WORD 높이다.</summary>
    public int ImageHeight { get; set; }
    /// <summary>독립 입력의 현재 화면 원점 X다.</summary>
    public int ScreenOriginX { get; set; }
    /// <summary>독립 입력의 현재 화면 원점 Y다.</summary>
    public int ScreenOriginY { get; set; }
    /// <summary>55a308의 현재 NUL 종료 설명에 대응한다.</summary>
    public string Description { get; set; } = "";
    /// <summary>실제59a91c/920의 같은 현재2칸을 빌린다. 미지정 독립 입력만 자체 원점을 쓴다.</summary>
    public ReadOnlyMemory<int>? SharedScreenOrigin { get; set; }
    /// <summary>소비 시점의 실제 X를 조회한다.</summary>
    public int CurrentOriginX() => SharedScreenOrigin is { } shared ? shared.Span[0] : ScreenOriginX;
    /// <summary>소비 시점의 실제 Y를 조회한다.</summary>
    public int CurrentOriginY() => SharedScreenOrigin is { } shared ? shared.Span[1] : ScreenOriginY;
    /// <summary>객체/고정 배열을 유지한 채 원문 struct{} 초기값을 쓴다.</summary>
    public void Reset()
    {
        PlacementFlags = PlacementAllowed = DescriptionAllowed = SpaceAllowed = SourceAllowed = Obstruction = 0;
        AcceptEvent = 0x0200003f; LabelSpacing = 16; Array.Clear(Bounds); HotspotX = HotspotY = ImageWidth = ImageHeight = ScreenOriginX = ScreenOriginY = 0;
        Description = ""; SharedScreenOrigin = null;
    }
}
