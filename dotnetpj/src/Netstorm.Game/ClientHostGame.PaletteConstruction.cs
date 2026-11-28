using Netstorm.Client.Display;

namespace Netstorm.Client.Host; // base Game의 초기 효과 전에 팔레트 소유자 인계만 검증한다.

/// <summary>실제 Game 생성 경로에 같은 Client의 팔레트 DWORD 수명을 전달한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은 Screen/색표가 빌리는 장기 Client 팔레트 소유자다.</summary>
    private readonly ClientPaletteGlobals _paletteGlobals;
    /// <summary>재생성 Screen도 이 저장소를 빌리며 값을 초기화하지 않는다.</summary>
    public ClientPaletteGlobals PaletteGlobals => _paletteGlobals;

    /// <summary>순수 검증을 통과한 원래 생성 인자다. 준비/옵션/시각/난수는 실행하지 않는다.</summary>
    private sealed record PaletteConstruction(HostFrameDriver Driver, HostOptions Options, HostStartupScene? Startup,
        HostTimeSource? Clock, ClientPaletteGlobals Palette);

    /// <summary>기존 4인자 API는 Screen의 현재 저장소 자체를 인계한다. preloaded 값도 읽거나 재기록하지 않는다.</summary>
    public ClientHostGame(HostFrameDriver driver, HostOptions options, HostStartupScene? startup = null, HostTimeSource? clock = null)
        : this(ValidatePaletteConstruction(driver, options, startup, clock, null, false)) { }

    /// <summary>제품 Program의 명시 경로다. null/foreign owner는 base Game 생성 전에 거부한다.</summary>
    public ClientHostGame(HostFrameDriver driver, HostOptions options, HostStartupScene? startup, HostTimeSource? clock, ClientPaletteGlobals palette)
        : this(ValidatePaletteConstruction(driver, options, startup, clock, palette, true)) { }

    /// <summary>생성자 위임 인자를 먼저 평가하여 base Game/field initializer보다 앞에서 identity만 확인한다.</summary>
    private static PaletteConstruction ValidatePaletteConstruction(HostFrameDriver driver, HostOptions options,
        HostStartupScene? startup, HostTimeSource? clock, ClientPaletteGlobals? palette, bool explicitOwner)
    {
        ArgumentNullException.ThrowIfNull(driver);
        ArgumentNullException.ThrowIfNull(options);
        if (explicitOwner) ArgumentNullException.ThrowIfNull(palette);
        ClientPaletteGlobals owner = palette ?? new ClientPaletteGlobals(driver.Screen.PaletteLoadedOwner);
        owner.RequireScreen(driver.Screen);
        return new PaletteConstruction(driver, options, startup, clock, owner);
    }
}
