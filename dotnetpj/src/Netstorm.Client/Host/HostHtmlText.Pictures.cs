using Netstorm.Client.Gump;
using Netstorm.Core.Original;

// HTML/Chat/controlled 메뉴의 같은 실제 그림 자산과 일시 출력 문맥이다.
namespace Netstorm.Client.Host;

/// <summary>RawMenuControls의 같은 Markup/스타일 자산과 실제 픽셀 출력을 연결한다.</summary>
public sealed partial class HostHtmlText
{
    /// <summary>실제 그림 조각/타입 수의 같은 저장소다.</summary>
    private readonly MarkupState _pictureState = new();
    /// <summary>파서와 출력이 공유하는 현재 TYPE/프레임/표면 경계다.</summary>
    private readonly MarkupHooks _pictureHooks = new();
    /// <summary>같은 현재 스타일 명령의 실제 자산/팔레트/소리 장치다.</summary>
    private readonly StyleTextMarkupHooks _markupHooks = new();
    /// <summary>동일 Config가 읽는 zone 효과 허용 값이다.</summary>
    private readonly StyleTextMarkupState _markupState = new();
    /// <summary>실제 최초 자산의 같은 어댑터다.</summary>
    private MarkupGameAssets? _pictureAssets;
    /// <summary>최초 파일 팔레트 또는 현재 Native 색표7의 조회다.</summary>
    private Func<byte[]>? _pictureColors;
    /// <summary>현재 잠긴 표면에서만 유효한 실제 그림 출력기다.</summary>
    private MarkupHooks? _activePicture;
    /// <summary>같은 그림/스타일에 최초 자산을 한 번 연결하고 팔레트/옵션은 별도 현재 조회로 둔다.</summary>
    public void BindPictures(MarkupGameAssets assets, Func<byte[]> colors, ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(assets); ArgumentNullException.ThrowIfNull(colors); ArgumentNullException.ThrowIfNull(configuration);
        if (_pictureAssets is not null) throw new InvalidOperationException("본문 실제 그림 자산 중복 설치");
        MarkupAssets.BindMarkupAssets(_pictureState, _pictureHooks, assets.Pictures); assets.BindStyle(_markupHooks); _pictureAssets = assets; _pictureColors = colors; ReadPictureOptions(configuration);
    }
    /// <summary>누락 키는 현재 허용값을 유지하는 원문 옵션 적재다.</summary>
    public void ReadPictureOptions(ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(configuration); StyleTextMarkup.LoadStyleTextMarkupOptions(_markupState, configuration.Configuration.Get);
    }
    /// <summary>잠긴 실제 표면의 출력 중에만 같은 그림 장치를 빌리고 예외에도 문맥을 해제한다.</summary>
    internal void WithPictureDraw(GumpPixelTarget target, byte[] currentColors, Action draw)
    {
        ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(currentColors); ArgumentNullException.ThrowIfNull(draw);
        if (_activePicture is not null) throw new InvalidOperationException("본문 그림 실제 출력 재진입"); if (_pictureAssets is null) { draw(); return; }
        byte[] colors = (_pictureColors ?? throw new InvalidOperationException("본문 그림 현재 색표 미연결"))(); if (colors.Length != 256) throw new InvalidOperationException("본문 그림 실제256색 표 필요");
        _activePicture = new MarkupPixelTarget(target, _pictureAssets.Pictures, currentColors, colors).Hooks();
        try { draw(); } finally { _activePicture = null; }
    }
}
