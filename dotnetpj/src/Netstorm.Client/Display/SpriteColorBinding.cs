// 일반 객체 출력이 실제 Client 공통 색표와 현재 소유자 색 번호를 공유한다.
namespace Netstorm.Client.Display;

/// <summary>원본 객체 출력 모드별로 현재 공통 색표를 빌린다.</summary>
public sealed class SpriteColorBinding
{
    /// <summary>팔레트 캐시와 TYPE별 소유자 표를 보유하는 실제 Client 장치다.</summary>
    private readonly ClientColorSortInput _input;
    /// <summary>플레이어 번호의 현재 팔레트 번호를 읽는다.</summary>
    private readonly Func<uint, int> _ownerPalette;

    /// <summary>같은 Client 색표와 현재 게임 소유자 조회를 빌린다.</summary>
    /// <param name="input">현재 공통 색표 소유자다.</param>
    /// <param name="ownerPalette">플레이어 번호별 현재 팔레트 번호 조회다.</param>
    /// <remarks>cpppj RawCampaignFactoryDisplay::Color의 실제 공유 색표 경로에 대응한다.</remarks>
    public SpriteColorBinding(ClientColorSortInput input, Func<uint, int> ownerPalette)
    {
        _input = input ?? throw new ArgumentNullException(nameof(input));
        _ownerPalette = ownerPalette ?? throw new ArgumentNullException(nameof(ownerPalette));
    }

    /// <summary>호출 순간 현재 출력 모드·TYPE·소유자의 실제 256색 표를 반환한다.</summary>
    /// <param name="mode">그림의 색 처리 방식이다.</param>
    /// <param name="type">현재 그림 TYPE이다.</param>
    /// <param name="owner">현재 플레이어 번호다.</param>
    /// <returns>공통 장치가 소유한 현재 색표다.</returns>
    public byte[] Color(SpriteShapeMode mode, uint type, uint owner)
    {
        if (mode == SpriteShapeMode.Owner)
        {
            int palette = _ownerPalette(owner);
            if (palette is < 0 or > 8) throw new ArgumentOutOfRangeException(nameof(owner), "객체 현재 소유자 색 번호 오류");
            return _input.OwnerTables.Color(type, (uint)palette);
        }
        int index = mode switch
        {
            SpriteShapeMode.Bright160 => 0,
            SpriteShapeMode.Abstract => 4,
            SpriteShapeMode.Dead => 6,
            SpriteShapeMode.Shadow => 7,
            _ => throw new ArgumentException("객체 색표가 없는 출력 모드", nameof(mode)),
        };
        return _input.Maps[index];
    }
}
