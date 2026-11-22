using System.Buffers.Binary;

// 원문float 저장·넓은 계산·서로 다른 두 난수의 소비 순서를 유지한다.
namespace Netstorm.Core.Original;

/// <summary>PATCH00460000/0045fc70·세 생성기·0045fd60/0045fef0의 실제 계산이다.</summary>
public sealed class RawShrapnelFrame
{
    /// <summary>PE의 정확한binary32 CRT 단위다.</summary>
    private static readonly float CrtUnit = BitConverter.UInt32BitsToSingle(0x38000100);
    /// <summary>붕괴 수명의 정확한binary32 최소값이다.</summary>
    private static readonly float CollapseMinimum = BitConverter.UInt32BitsToSingle(0x3dcccccd);
    /// <summary>게임 난수의binary64 단위다.</summary>
    private const double GameUnit = 1.0 / 65536.0;
    /// <summary>방사 속도의binary64 간격이다.</summary>
    private const double Spread = .3;
    /// <summary>PE의binary32 폭을 넓힌binary64 값이다.</summary>
    private const double CollapseWidth = 2.4000000953674316;
    /// <summary>프레임 간격에 곱하는 원본10배 시간이다.</summary>
    private const double TimeScale = 10;
    /// <summary>현재 부모 좌표를 읽는 같은 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>속도 계산에 쓰는 같은 게임 난수다.</summary>
    private readonly GameRandom _game;
    /// <summary>묶음 크기·수명에 쓰는 별도CRT 난수다.</summary>
    private readonly CrtRandom _crt;
    /// <summary>현재 시각·카메라·배율을 빌린다.</summary>
    private readonly FieryTailFrameState _state;
    /// <summary>실제 외부 출력 장치의 함수 사본이다.</summary>
    private readonly ShrapnelFrameHooks _hooks;

    /// <summary>PATCH와 모든 장치를 생성 전에 확인한다. 두 난수 스트림을 합치지 않는다.</summary>
    public RawShrapnelFrame(RawSidPool pool, GameRandom game, CrtRandom crt, FieryTailFrameState state, ShrapnelFrameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(crt);
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || hooks.Reserve is null || hooks.Release is null || hooks.Invalidate is null || hooks.Pixel is null || hooks.RemoveForm is null)
            throw new ArgumentException("픽셀 파편 원본/외부 경계 누락");
        _pool = pool; _game = game; _crt = crt; _state = state; _hooks = hooks.Copy();
    }

    /// <summary>정상signed32 범위에서 원본CRT처럼0방향 절삭한다.</summary>
    private static int Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value), "픽셀 파편 정수화 범위 오류");
        return (int)value;
    }
    /// <summary>비정렬 부모 좌표의float 비트를 그대로 읽는다.</summary>
    private static float Float(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4)));
    /// <summary>이미지·용량·꼬리 손상은 변경 전에 거부한다.</summary>
    private static void Validate(ShrapnelFrameFields fields)
    {
        ArgumentNullException.ThrowIfNull(fields);
        long size = (long)fields.Image.Width * fields.Image.Height;
        if (fields.Image.Width < 1 || fields.Image.Height < 1 || size > int.MaxValue || size != fields.Image.Pixels.Length ||
            fields.Particles.Length > int.MaxValue / 24 || fields.Total < 0 || fields.Total > fields.Particles.Length)
            throw new ArgumentException("픽셀 파편 이미지/용량/꼬리 오류");
    }
    /// <summary>상위16비트와 넓은 간격이다. 역순 끝점20/14도20에0..6을 더한다.</summary>
    private double Random(float low, float high) => _game.Next(65536) * GameUnit * Math.Abs((double)high - low) + low;

    /// <summary>조건 검사마다CRT를 다시 소비하며 선택한 작업 픽셀만0xff로 바꾼다.</summary>
    private void Split(ShrapnelFrameFields fields, Span<byte> work, int originX, int originY, int x, int y, float vx, float vy, float life)
    {
        int dx = 0, width = (int)(_crt.Next() % 3 + 3);
        // 실패하는 마지막 조건에도 이전 반복 말미의CRT를 소비한다.
        while (dx < width)
        {
            int dy = 0, height = (int)(_crt.Next() % 3 + 3);
            // 고정 사각형 대신 각 행 검사에서 높이를 다시 뽑는다.
            while (dy < height)
            {
                if (x + dx > 0 && x + dx < fields.Image.Width && y + dy > 0 && y + dy < fields.Image.Height)
                {
                    int pixel = (y + dy) * fields.Image.Width + x + dx;
                    if (work[pixel] != 255)
                    {
                        work[pixel] = 255; float remaining = (float)((double)_crt.Next() * CrtUnit + life);
                        if (fields.Total >= fields.Particles.Length) throw new InvalidOperationException("픽셀 파편 용량 원본 assert");
                        fields.Particles[fields.Total++] = new() { X = (float)(((double)dx + originX) + x), Y = (float)(((double)dy + originY) + y), VelocityX = vx, VelocityY = vy, Life = remaining, Pixel = (uint)(pixel + 1) };
                    }
                }
                ++dy; height = (int)(_crt.Next() % 3 + 3);
            }
            ++dx; width = (int)(_crt.Next() % 3 + 3);
        }
    }
    /// <summary>가상 생성기별 넓은 수학과float 인자 저장 순서를 보존한다.</summary>
    private void Seed(ShrapnelFrameFields fields, ShrapnelKind kind, Span<byte> work, int originX, int originY, int x, int y)
    {
        float vx = 0, vy = 0, life;
        if (kind == ShrapnelKind.Explosion)
        {
            double vertical = (y - fields.Image.Height / 2) * Spread;
            vy = (float)(vertical - Random(20, 14)); life = (float)(((double)_crt.Next() * CrtUnit) * 5 + .5); vx = (float)((x - fields.Image.Width / 2) * Spread);
        }
        else if (kind == ShrapnelKind.Collapse)
        {
            double random = ((double)_crt.Next() * CrtUnit) * CollapseWidth + CollapseMinimum;
            life = (float)((random * Math.Sqrt((double)(fields.Image.Height - y + 1) / fields.Image.Height)) * 10);
        }
        else
        {
            double ratio = (double)y / fields.Image.Height, basis = (1 - ratio) * 12 + 12.5; vx = (float)ratio;
            life = (float)(((double)_crt.Next() * CrtUnit) * Math.Abs(3.5 - basis) + Math.Min(3.5, basis));
        }
        Split(fields, work, originX, originY, x, y, vx, vy, life);
    }

    /// <summary>현재 부모−기준점 절삭→불투명 용량→작업 묶음→타이머 초기화다. 원본 그림은 보존한다.</summary>
    public void Prepare(ShrapnelFrameFields fields, ShrapnelKind kind)
    {
        Validate(fields); int parent = fields.Registration.Parent;
        if (parent < 1 || parent >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields), "픽셀 파편 부모 번호 오류");
        ReadOnlySpan<byte> raw = _pool.Slot(parent); RenderProcessFields rect = fields.Registration; rect.ZOrder = unchecked((sbyte)raw[35]);
        double x = (double)Float(raw, 14) * _state.PixelScaleX, y = (double)Float(raw, 18) * _state.PixelScaleY;
        rect.Left = rect.Right = (float)x; rect.Top = rect.Bottom = (float)y;
        int originX = Ftol(x - fields.Image.HotspotX), originY = Ftol(y - fields.Image.HotspotY), count = Math.Max(1, fields.Image.Pixels.Count(color => color != 255));
        if (fields.Particles.Length != 0) _hooks.Release!(false);
        if (!_hooks.Reserve!((uint)(count * 24))) throw new OutOfMemoryException("픽셀 파편 입자 확보 실패");
        fields.Particles = new ShrapnelParticle[count]; fields.Total = 0;
        if (!_hooks.Reserve!((uint)fields.Image.Pixels.Length)) throw new OutOfMemoryException("픽셀 파편 작업 그림 확보 실패");
        byte[] work = fields.Image.Pixels.ToArray();
        // 아직 선택하지 않은 현재 행을 원문 순서로 검사한다.
        for (int row = 0; row < fields.Image.Height; ++row)
        {
            // 앞선묶음이변경한작업그림의현재값을매번읽는다.
            for (int column = 0; column < fields.Image.Width; ++column)
                if (work[row * fields.Image.Width + column] != 255) Seed(fields, kind, work, originX, originY, column, row);
        }
        _hooks.Release!(true); fields.LastTime = _state.Now; fields.Elapsed = 0;
    }

    /// <summary>간격을 제한하지 않고 전체 용량을 갱신한다. 진입 양수 입자가 없을 때 실제Form을 삭제한다.</summary>
    public void Step(ShrapnelFrameFields fields)
    {
        Validate(fields); int parent = fields.Registration.Parent;
        if (parent < 1 || parent >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields), "픽셀 파편 부모 번호 오류");
        ReadOnlySpan<byte> raw = _pool.Slot(parent);
        float x = (float)((double)Float(raw, 14) * _state.PixelScaleX), y = (float)((double)Float(raw, 18) * _state.PixelScaleY);
        float left = x, top = y, right = x, bottom = y;
        double wide = (_state.Now - fields.LastTime) * TimeScale; fields.Delta = (float)wide; fields.ScaledDelta = (float)(wide * _state.Rate); fields.LastTime = _state.Now; fields.Elapsed += fields.Delta;
        float acceleration = (float)((double)fields.Gravity * fields.Delta); bool alive = false;
        // 꼬리가아닌전체용량에서진입양수수명만갱신한다.
        foreach (ref ShrapnelParticle particle in fields.Particles.AsSpan())
        {
            if (!(particle.Life > 0)) continue; alive = true; particle.Life = (float)((double)particle.Life - fields.Delta);
            double vy = (double)acceleration + particle.VelocityY; particle.VelocityY = (float)vy;
            particle.X = (float)((double)particle.VelocityX * fields.ScaledDelta + particle.X); double nextY = vy * fields.ScaledDelta + particle.Y; particle.Y = (float)nextY;
            int ix = Ftol(particle.X), iy = Ftol(nextY);
            left = Math.Min(left, (float)((double)ix - 3)); top = Math.Min(top, (float)((double)iy - 3)); right = Math.Max(right, (float)((double)ix + 3)); bottom = Math.Max(bottom, (float)((double)iy + 3));
        }
        if (!alive) { _hooks.RemoveForm!(); return; }
        RenderProcessFields rect = fields.Registration; Invalidate(rect.Left, rect.Top, rect.Right, rect.Bottom); Invalidate(left, top, right, bottom);
        rect.Left = left; rect.Top = top; rect.Right = right; rect.Bottom = bottom;
    }
    /// <summary>원본487ba0의여유·카메라·절삭순서로dirty를전달한다.</summary>
    private void Invalidate(float left, float top, float right, float bottom) => _hooks.Invalidate!(new(Ftol(((double)left - 1) - _state.CameraX), Ftol(((double)top - 1) - _state.CameraY), Ftol(((double)right + 1) - _state.CameraX), Ftol(((double)bottom + 1) - _state.CameraY)));
    /// <summary>사용꼬리중양수수명만현재캡처색으로출력한다. 가로만카메라감산뒤float에저장한다.</summary>
    public void Draw(ShrapnelFrameFields fields)
    {
        Validate(fields);
        // 현재사용꼬리의살아있는색주소만조회한다.
        for (int index = 0; index < fields.Total; ++index)
        {
            ShrapnelParticle particle = fields.Particles[index]; if (!(particle.Life > 0)) continue;
            if (particle.Pixel == 0 || particle.Pixel > fields.Image.Pixels.Length) throw new ArgumentOutOfRangeException(nameof(fields), "픽셀 파편 색 주소 토큰 오류");
            float x = (float)((double)Ftol(particle.X) - _state.CameraX); double y = (double)Ftol(particle.Y) - _state.CameraY;
            _hooks.Pixel!(new(Ftol(x), Ftol(y), fields.Image.Pixels[particle.Pixel - 1]));
        }
    }
}
