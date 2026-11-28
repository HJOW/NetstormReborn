using Netstorm.Core.Original;

namespace Netstorm.Client.Display; // 같은 Client 수명에 남는 원본 팔레트 전역을 소유한다.

/// <summary>Screen을 다시 만들어도 유지하는 Client의 00555e08 DWORD 소유자다.</summary>
public sealed class ClientPaletteGlobals
{
    /// <summary>현재 팔레트 읽음 값의 실제 공유 저장소다. Screen/ColorSort는 이 객체 자체를 빌린다.</summary>
    public UIntBox Loaded { get; }

    /// <summary>새 Client의 처음 0인 DWORD를 소유한다.</summary>
    public ClientPaletteGlobals() : this(new UIntBox()) { }

    /// <summary>기존 Screen의 같은 DWORD를 값 조회·초기화·복사 없이 Client 수명으로 인계한다.</summary>
    public ClientPaletteGlobals(UIntBox loaded) => Loaded = loaded ?? throw new ArgumentNullException(nameof(loaded));

    /// <summary>값이 같더라도 다른 Client의 저장소를 빌린 화면은 거부한다. 현재 DWORD는 읽지 않는다.</summary>
    public void RequireScreen(Screen screen)
    {
        ArgumentNullException.ThrowIfNull(screen);
        if (!ReferenceEquals(Loaded, screen.PaletteLoadedOwner))
            throw new ArgumentException("다른 Client의 팔레트 읽음 소유자를 빌린 화면", nameof(screen));
    }
}
