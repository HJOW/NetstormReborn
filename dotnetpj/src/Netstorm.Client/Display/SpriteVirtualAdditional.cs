using System.Buffers.Binary;
using Netstorm.Core.Original;

// 현재 객체 가상표의 +a4 추가 그림을 실제 하위 출력기로 분배한다.
namespace Netstorm.Client.Display;

/// <summary>10.78 객체 가상표가 선택한 추가 그림을 실제 출력 장치에 분배한다.</summary>
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
    /// <summary>주문 표시의 현재 TYPE 속성이다. 도달 전에는 없어도 된다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord>? _types;
    /// <summary>주문 도움말 프레임을 읽을 파일 자산 정의다.</summary>
    private readonly IReadOnlyList<RiftTypeDefinition>? _definitions;
    /// <summary>주문 추상 그림의 현재 로컬 소유자다.</summary>
    private readonly Func<uint>? _localOwner;
    /// <summary>제단 추가 자산의 현재 프레임 수를 포함하는 실제 셰이프 표다.</summary>
    private readonly IReadOnlyList<SquidDisplayShape>? _shapes;

    /// <summary>같은 실제 풀·프레임과 호출 위치에서 사용할 출력 장치를 빌린다.</summary>
    /// <param name="pool">10.78 객체 풀이다.</param>
    /// <param name="frames">내장 TYPE을 포함한 프레임 표다.</param>
    /// <param name="shape">실제 현재 SHP 그림 소비자다.</param>
    /// <param name="bridge">다리 추가 그림 소비자다.</param>
    /// <param name="meta">메타 추가 그림 소비자다.</param>
    /// <param name="types">현재 TYPE 속성이다.</param>
    /// <param name="definitions">파일 TYPE 정의다.</param>
    /// <param name="localOwner">현재 로컬 소유자 조회다.</param>
    /// <param name="shapes">현재 TYPE 순서의 실제 셰이프 표다.</param>
    public SpriteVirtualAdditional(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, Action<SpriteShapeRequest> shape,
        Action<int, ScreenPoint>? bridge = null, Action<int, ScreenPoint>? meta = null,
        IReadOnlyList<OriginalTypeRecord>? types = null, IReadOnlyList<RiftTypeDefinition>? definitions = null, Func<uint>? localOwner = null,
        IReadOnlyList<SquidDisplayShape>? shapes = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(shape);
        _factory = new RawFactoryDraw(pool, frames);
        _pool = pool;
        _shape = shape;
        _bridge = bridge;
        _meta = meta;
        _types = types;
        _definitions = definitions;
        _localOwner = localOwner;
        _shapes = shapes;
    }

    /// <summary>현재 객체의 +a4를 조회해 해당 실제 추가 그림만 호출한다.</summary>
    /// <param name="sid">현재 객체 SID다.</param>
    /// <param name="point">현재 화면 기준점이다.</param>
    /// <remarks>cpppj RawCampaignFactoryDisplay::Additional의 확인된 경로를 따른다. 다른 함수는 후속 이식 전 명시 오류다.</remarks>
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
            case 0x0041e3d0:
                DrawSpell(sid, point);
                return;
            case 0x00473710:
                DrawMana(sid, point);
                return;
            case 0x0045ac40:
                DrawManaBolt(sid, point);
                return;
            case 0x00485480:
                DrawCurrentOverlay(sid, point, 122, false);
                return;
            case 0x0046ae30:
                DrawCurrentOverlay(sid, point, 131, false);
                return;
            case 0x0045b560:
                DrawCurrentOverlay(sid, point, 72, true);
                return;
            case 0x004484b0:
                DrawDais(sid, point);
                return;
            case 0x00479340:
                DrawScaledMeta(sid, point);
                return;
            case 0x00485f90:
                ReadOnlySpan<byte> raw = _pool.Slot(sid);
                _shape(new(raw[10], BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]), point, SpriteShapeMode.Abstract));
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

    /// <summary>주문의 도움말 프레임을 제외하고 현재 추상·로컬 소유자 관문을 적용한다.</summary>
    /// <param name="sid">현재 주문 객체 SID다.</param>
    /// <param name="point">현재 화면 기준점이다.</param>
    /// <remarks>cpppj SpellFrameVisibility와 실제 0041e3d0 출력 순서다. 도움말 프레임 제외는 기존 사용자 요청에 따른 표시 규칙이다.</remarks>
    private void DrawSpell(int sid, ScreenPoint point)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        int type = raw[10];
        var types = _types ?? throw new InvalidOperationException("주문 현재 TYPE 표 미연결");
        var definitions = _definitions ?? throw new InvalidOperationException("주문 실제 TYPE 정의 미연결");
        if (type >= types.Count || type < 70 || type - 70 >= definitions.Count) throw new ArgumentOutOfRangeException(nameof(sid), "주문 TYPE 범위 오류");
        uint frame = BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]);
        int helpFrame = definitions[type - 70].SpecialFrames.HelpFrame;
        if ((types[type].Flags2 & TypeFlag2.Bomb) != 0 && helpFrame >= 0 && frame == (uint)helpFrame) return;
        if ((raw[40] & 0x10) == 0) _shape(new((uint)type, frame, point, SpriteShapeMode.Plain));
        else if (raw[34] == (_localOwner ?? throw new InvalidOperationException("주문 현재 로컬 소유자 미연결"))())
            _shape(new((uint)type, frame, point, SpriteShapeMode.Abstract));
    }

    /// <summary>마나의 조건부 그림자 뒤에 현재 프레임을 다시 읽어 본체를 그린다.</summary>
    /// <param name="sid">현재 마나 객체 SID다.</param>
    /// <param name="point">현재 화면 기준점이다.</param>
    /// <remarks>cpppj RawManaDraw의 현재 TYPE 선택·프레임 재조회와 signed 좌표 감김을 따른다.</remarks>
    private void DrawMana(int sid, ScreenPoint point)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint type = raw[10];
        if (BinaryPrimitives.ReadUInt16LittleEndian(raw[12..]) != 0)
        {
            uint shadowFrame = BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]);
            _shape(new(type, shadowFrame, new(unchecked(point.X + 10), unchecked(point.Y + 10)), SpriteShapeMode.Shadow));
        }
        uint frame = BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(sid)[36..]);
        _shape(new(type, frame, point, SpriteShapeMode.Plain));
    }

    /// <summary>마나 볼트의 현재 프레임을 그림자와 본체에 같은 순서로 그린다.</summary>
    /// <param name="sid">현재 마나 볼트 객체 SID다.</param>
    /// <param name="point">현재 화면 기준점이다.</param>
    /// <remarks>cpppj RawCampaignFactoryDisplay의 105번 실제 가상표 경계를 따른다.</remarks>
    private void DrawManaBolt(int sid, ScreenPoint point)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[10] != 105) throw new ArgumentException("마나 볼트 TYPE 불일치", nameof(sid));
        uint frame = BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]);
        _shape(new(105, frame, new(unchecked(point.X + 10), unchecked(point.Y + 10)), SpriteShapeMode.Shadow));
        _shape(new(105, frame, point, SpriteShapeMode.Plain));
    }

    /// <summary>간헐천·썬더 캐논·훨리베이스의 현재 그림에 추상 소유자 관문을 적용한다.</summary>
    /// <param name="sid">현재 객체 SID다.</param>
    /// <param name="point">현재 화면 기준점이다.</param>
    /// <param name="type">실제 가상표에서 요구하는 TYPE이다.</param>
    /// <param name="aviary">훨리베이스의 상태 비트 억제 여부다.</param>
    /// <remarks>cpppj의 각 Capture 추가 그림과 00497280 상태색 분기를 따른다.</remarks>
    private void DrawCurrentOverlay(int sid, ScreenPoint point, byte type, bool aviary)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[10] != type) throw new ArgumentException("추가 그림 TYPE 불일치", nameof(sid));
        if (aviary && (raw[40] & 9) != 0) return;
        uint frame = BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]);
        if ((raw[40] & 0x10) == 0) _shape(new(type, frame, point, SpriteShapeMode.Plain));
        else if (raw[34] == (_localOwner ?? throw new InvalidOperationException("추가 그림 현재 로컬 소유자 미연결"))())
            _shape(new(type, frame, point, SpriteShapeMode.Abstract));
    }

    /// <summary>제단 프레임을 별도 자산의 프레임 수로 나눈 나머지 그림을 표시한다.</summary>
    /// <param name="sid">현재 제단 객체 SID다.</param>
    /// <param name="point">현재 화면 기준점이다.</param>
    /// <remarks>cpppj RawDaisDraw의 signed 나머지와 실제 00497280 소유자 관문을 따른다.</remarks>
    private void DrawDais(int sid, ScreenPoint point)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[10] != 165) throw new ArgumentException("제단 추가 그림 TYPE 불일치", nameof(sid));
        var shapes = _shapes ?? throw new InvalidOperationException("제단 추가 그림 셰이프 표 미연결");
        if (shapes.Count <= 169 || shapes[169] is null || shapes[169].FrameCount <= 0)
            throw new InvalidOperationException("제단 추가 그림 프레임 수 오류");
        int frame = BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) % shapes[169].FrameCount;
        if (frame < 0) throw new ArgumentOutOfRangeException(nameof(sid), "제단 추가 그림 음수 프레임");
        if ((raw[40] & 0x10) == 0) _shape(new(169, (uint)frame, point, SpriteShapeMode.Plain));
        else if (raw[34] == (_localOwner ?? throw new InvalidOperationException("제단 현재 로컬 소유자 미연결"))())
            _shape(new(169, (uint)frame, point, SpriteShapeMode.Abstract));
    }

    /// <summary>메타 표시의 현재 고정소수점 배율과 프레임을 실제 그림 요청으로 전달한다.</summary>
    /// <param name="sid">현재 메타 표시 객체 SID다.</param>
    /// <param name="point">현재 화면 기준점이다.</param>
    /// <remarks>cpppj RawAuxiliaryDraw의 숨김 비트와 정수·분수 WORD, 0의 단위 배율 대체를 따른다.</remarks>
    private void DrawScaledMeta(int sid, ScreenPoint point)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint flags = BinaryPrimitives.ReadUInt32LittleEndian(raw[26..]);
        if ((flags & 0x4000) != 0) return;
        uint scale = ((flags & 0x3fff) << 16) + BinaryPrimitives.ReadUInt16LittleEndian(raw[8..]);
        if (scale == 0) scale = 0x10000;
        uint frame = BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]);
        _shape(new(raw[10], frame, point, SpriteShapeMode.Plain)
            { Scaled = true, ScaleX = unchecked((int)scale), ScaleY = unchecked((int)scale) });
    }
}
