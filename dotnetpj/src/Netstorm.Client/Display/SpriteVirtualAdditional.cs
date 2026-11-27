using Netstorm.Core.Original;

// 현재 객체 가상표의 +a4 추가 그림을 실제 하위 출력기로 분배한다.
namespace Netstorm.Client.Display;

/// <summary>10.78 기본 빈 함수와 공장·다리·메타 추가 그림 경계다.</summary>
public sealed class SpriteVirtualAdditional
{
    /// <summary>현재 가상표와 데이터를 읽는 실제 SID 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>같은 풀의 공장 추가 그림 본체다.</summary>
    private readonly RawFactoryDraw _factory;
    /// <summary>실제 SHP 그림을 현재 픽셀 표면에 전달한다.</summary>
    private readonly Action<SpriteShapeRequest> _shape;
    /// <summary>실제 다리 수명 표시 장치다. 도달 전에는 없어도 된다.</summary>
    private readonly Action<int, ScreenPoint>? _bridge;
    /// <summary>실제 메타 객체 표시 장치다. 도달 전에는 없어도 된다.</summary>
    private readonly Action<int, ScreenPoint>? _meta;

    /// <summary>같은 실제 풀·프레임과 호출 위치에서 사용할 출력 장치를 빌린다.</summary>
    /// <param name="pool">10.78 객체 풀이다.</param>
    /// <param name="frames">내장 TYPE을 포함한 프레임 표다.</param>
    /// <param name="shape">실제 현재 SHP 그림 소비자다.</param>
    /// <param name="bridge">다리 추가 그림 소비자다.</param>
    /// <param name="meta">메타 추가 그림 소비자다.</param>
    public SpriteVirtualAdditional(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, Action<SpriteShapeRequest> shape,
        Action<int, ScreenPoint>? bridge = null, Action<int, ScreenPoint>? meta = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(shape);
        _factory = new RawFactoryDraw(pool, frames);
        _pool = pool;
        _shape = shape;
        _bridge = bridge;
        _meta = meta;
    }

    /// <summary>현재 객체의 +a4를 조회해 해당 실제 추가 그림만 호출한다.</summary>
    /// <param name="sid">현재 객체 SID다.</param>
    /// <param name="point">현재 화면 기준점이다.</param>
    /// <remarks>cpppj RawCampaignFactoryDisplay::Additional의 확인된 네 경로를 따른다. 다른 함수는 후속 이식 전 명시 오류다.</remarks>
    public void Draw(int sid, ScreenPoint point)
    {
        uint address = SpriteVtableLookup.Read(_pool, sid).Additional;
        switch (address)
        {
            case 0x0041c100:
                return;
            case 0x004223c0:
                (_bridge ?? throw new InvalidOperationException("다리 실제 추가 그림 장치 미연결"))(sid, point);
                return;
            case 0x00478d10:
                (_meta ?? throw new InvalidOperationException("메타 실제 추가 그림 장치 미연결"))(sid, point);
                return;
            case 0x00454cf0:
                _factory.Additional(sid, point.X, point.Y, (x, y, type, frame) =>
                {
                    if (frame < 0) throw new ArgumentOutOfRangeException(nameof(sid), "공장 추가 그림 음수 프레임");
                    _shape(new(type, (uint)frame, new(x, y), SpriteShapeMode.Plain));
                });
                return;
            default:
                throw new InvalidOperationException($"객체 추가 그림 미복원: 0x{address:X8}");
        }
    }
}
