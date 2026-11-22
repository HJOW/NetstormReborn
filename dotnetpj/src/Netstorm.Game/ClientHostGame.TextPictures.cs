using Netstorm.Client.Display;
using Netstorm.Client.Gump;

// 최초 공통 UI/Chat이 같은 실제 자산과 파일 팔레트의 그림 색표를 빌린다.
namespace Netstorm.Client.Host;

/// <summary>RawMenuControls의 Markup/스타일 그림 자산을 실제 장기 Root에 설치한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은 최초 TYPE/정의/SHP의 실제 그림 어댑터다.</summary>
    private MarkupGameAssets? _nativeTextPictures;
    /// <summary>최초 파일 팔레트의 동일 그림자 치환표다.</summary>
    private byte[]? _nativeTextPictureInitialColors;
    /// <summary>Native 색표가 준비되면 원문 현재7번 표를 조회하는 장치다.</summary>
    private Func<byte[]>? _nativeTextPictureCurrentColors;
    /// <summary>Chat 공장도 같은 실제 프레임/타입 자산을 빌린다.</summary>
    public MarkupGameAssets NativeTextPictures => _nativeTextPictures ?? throw new InvalidOperationException("실제 스타일 그림 자산 미설치");
    /// <summary>Native 팔레트 갱신이 설치하는 현재 그림 치환표 getter다.</summary>
    public void BindNativeTextPictureColors(Func<byte[]> colors) { ArgumentNullException.ThrowIfNull(colors); _nativeTextPictureCurrentColors = colors; }
    /// <summary>현재 Native getter가 없으면 원문 초기 파일 팔레트 표를 그대로 사용한다.</summary>
    private byte[] CurrentNativeTextPictureColors() => _nativeTextPictureCurrentColors?.Invoke() ?? _nativeTextPictureInitialColors ?? throw new InvalidOperationException("실제 그림 초기 색표 미설치");
    /// <summary>첫 본문 생성 전에 같은 장기 자산과 실제 Config/표면 출력 장치를 연결한다.</summary>
    private void InitializeNativeTextPictures()
    {
        var startup = _startup ?? throw new InvalidOperationException("본문 그림의 실제 시작 자산 없음");
        var text = _htmlText ?? throw new InvalidOperationException("본문 그림의 실제 조각 저장소 없음");
        _nativeTextPictures = new(AssetTables.TypeTable, AssetTables.Definitions, new ShapeDatabase(AssetTables.ShapeImage)); uint[] palette = new uint[256];
        // 원문 그림 치환표는 일시 날씨 팔레트가 아닌 실제 자산 COL로 구성한다.
        for (int index = 0; index < palette.Length; index++) { var color = startup.Palette.Color((byte)index); palette[index] = color.Red | ((uint)color.Green << 8) | ((uint)color.Blue << 16); }
        _nativeTextPictureInitialColors = SpriteColorMapBuilder.BuildSpriteColorMaps(palette).Shadow;
        text.BindPictures(NativeTextPictures, CurrentNativeTextPictureColors, startup.Configuration);
    }
}
