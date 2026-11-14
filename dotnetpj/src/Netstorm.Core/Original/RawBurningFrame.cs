using System.Buffers.Binary;
using Netstorm.Core.Display;

// 원문 연소의 캡처/단계/시각/입자/dirty 순서와 두 판본의 float 저장을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>생성자가 완료값으로 쓰지 않는 timer의 두float다.</summary>
public sealed class BurningFrameFields
{
    /// <summary>시각차*10의float 저장값이다.</summary>
    public float Delta { get; set; }
    /// <summary>같은 넓은 시각차*현재배율의float 저장값이다.</summary>
    public float ScaledDelta { get; set; }
}
/// <summary>각 프레임이 현재 owner에서 읽는 시각/배율/카메라/바람/viewport다.</summary>
public sealed class BurningFrameState
{
    /// <summary>현재 원문 게임시각이다.</summary>
    public double Now { get; set; }
    /// <summary>현재 가로 월드→픽셀 배율이다.</summary>
    public float PixelScaleX { get; set; } = 1;
    /// <summary>현재 세로 월드→픽셀 배율이다.</summary>
    public float PixelScaleY { get; set; } = 1;
    /// <summary>현재 가로 픽셀카메라다.</summary>
    public float CameraX { get; set; }
    /// <summary>현재 세로 픽셀카메라다.</summary>
    public float CameraY { get; set; }
    /// <summary>독립 입력의 현재 바람X다.</summary>
    public float Wind { get; set; }
    /// <summary>timer의현재 추가배율이다.</summary>
    public float DeltaMultiplier { get; set; }
    /// <summary>visible/dirty가 읽는 현재 사각형이다.</summary>
    public DisplayRect Viewport { get; set; }
    /// <summary>visible만 양쪽100픽셀 여백을 적용한다.</summary>
    public bool InsetViewport { get; set; }
    /// <summary>같은 현재 checking/상세SID/복귀진단이다.</summary>
    public BurningVictimState Victim { get; } = new();
    /// <summary>실제 Client의 같은 장기 바람X주소를 빌린다.</summary>
    public Func<float>? SharedWind { get; set; }
    /// <summary>각 이동입자의 가속 위치에서 현재 바람을 다시 읽는다.</summary>
    public float ReadWind() => SharedWind?.Invoke() ?? Wind;
    /// <summary>Draw 진입의 값사본을 만든다. 장기owner 참조는 빌리고 Draw에서 진단을 실행하지 않는다.</summary>
    internal BurningFrameState Copy() => (BurningFrameState)MemberwiseClone();
}
/// <summary>현재 실제SHP 캡처와 nonzero소리정지의 함수 값 묶음이다.</summary>
public sealed class BurningFrameHooks
{
    /// <summary>현재 피해SID의 실제 프레임/기준점/픽셀을 캡처한다.</summary>
    public Func<int, ConstructionVisualImage>? Capture { get; set; }
    /// <summary>선택된 실제 소리 핸들을 정지한다.</summary>
    public Action<uint>? StopSound { get; set; }
    /// <summary>함수 객체는 복사하고 현재 장치 owner를 빌린다.</summary>
    internal BurningFrameHooks Copy() => new() { Capture = Capture, StopSound = StopSound };
}
/// <summary>00424530/CD004a60e0의 실제 연소 프레임과00423d50/CD004a5c70의 캡처다.</summary>
public sealed class RawBurningFrame
{
    /// <summary>현재 raw/판본을 읽는 같은 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재 HP표식을 읽는 실제TYPE다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>같은 생성/Append 몸체다.</summary>
    private readonly RawBurningVisual _visual;
    /// <summary>같은 실제 연소점 선택기다.</summary>
    private readonly RawBurningSources _sources;
    /// <summary>게임 전체와 공유하는 실제 난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>각 소비시점에 조회할 장기 현재상태다.</summary>
    private readonly BurningFrameState _state;
    /// <summary>두 dirty영역을 원문순서로 받는 실제 출력대상이다.</summary>
    private readonly SquidDisplaySink _sink;
    /// <summary>구성 때 복사한 캡처/소리 함수다.</summary>
    private readonly BurningFrameHooks _hooks;
    /// <summary>같은 생성/선택풀과 실제 캡처 장치를 확인하며 소리정지는 선택 때 요구한다.</summary>
    public RawBurningFrame(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawBurningVisual visual, RawBurningSources sources,
        GameRandom random, BurningFrameState state, SquidDisplaySink sink, BurningFrameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(visual); ArgumentNullException.ThrowIfNull(sources);
        ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(sink); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, visual.Pool) || !ReferenceEquals(pool, sources.Pool)) throw new ArgumentException("Burning frame/생성/연소점 SID풀 불일치");
        if (hooks.Capture is null) throw new ArgumentException("Burning frame 실제 ShapeToBuffer 캡처 미연결");
        Pool = pool; _types = types; _visual = visual; _sources = sources; _random = random; _state = state; _sink = sink; _hooks = hooks.Copy();
    }
    /// <summary>원문 ftol의 유한 signed범위만0방향으로 정수화한다.</summary>
    private static int Integer(double value)
    {
        if (!double.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value), "Burning frame 원본 정수화 범위 오류");
        return (int)value;
    }
    /// <summary>같은 현재TYPE의 일반HP bit10 또는 다이스extra bit0을 읽는다.</summary>
    private bool HasHitPoints(ReadOnlySpan<byte> raw)
    {
        int type = raw[10]; if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(raw), "Burning frame 현재TYPE표 범위 오류");
        return (_types[type].Flags2 & 0x400000) != 0 ? (raw[Pool.Edition == OriginalEdition.Patch1078 ? 40 : 35] & 1) != 0 : (_types[type].Flags1 & 0x10) != 0;
    }
    /// <summary>상위16비트와 실제1/65536 단위의 넓은 float난수 반환값이다.</summary>
    private double Random(float minimum, float maximum) => _random.Next(65536) * (1.0 / 65536) * Math.Abs((double)maximum - minimum) + minimum;
    /// <summary>이전 버퍼/행→전체capacity입자zero→실제 현재대상 진단→캡처/새행 순서다.</summary>
    public void Capture(BurningVisualFields fields)
    {
        if (fields.Total > fields.Capacity || fields.Particles.Length != fields.Capacity) throw new ArgumentException("Burning capture 현재입자 배열 오류");
        fields.Image.Pixels = []; fields.Rows = []; fields.Total = 0; Array.Clear(fields.Particles);
        bool patch = Pool.Edition == OriginalEdition.Patch1078;
        if (patch)
        {
            if (RawBurningVictim.Run(() => fields.Victim, BurningVictimEntry.Capture, _state.Victim.Checking, RawBurningVictim.MakeHooks(Pool, _types, _state.Victim)) is null) return;
        }
        else
        {
            if (fields.Victim is 0 or 65535) return;
            ReadOnlySpan<byte> raw = Pool.Slot(fields.Victim); if (BinaryPrimitives.ReadUInt32LittleEndian(raw) == 0 || !HasHitPoints(raw)) return;
        }
        ConstructionVisualImage image = _hooks.Capture!(fields.Victim); long sum = (long)image.Width + image.Height;
        if (image.Width < 0 || image.Height < 0 || sum > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(fields), "Burning capture 행표식 크기 오류");
        fields.Image = image; fields.Rows = new byte[(int)(sum / 5)]; fields.Selected = 0;
    }
    /// <summary>이전영역 각변을 정수화/float저장/카메라감산/다시정수화하고 viewport와 교차한다.</summary>
    private bool Visible(RenderProcessFields r)
    {
        int left = Integer((float)((double)(float)Integer(r.Left) - _state.CameraX)), top = Integer((float)((double)(float)Integer(r.Top) - _state.CameraY));
        int right = Integer((float)((double)(float)Integer(r.Right) - _state.CameraX)), bottom = Integer((float)((double)(float)Integer(r.Bottom) - _state.CameraY));
        int margin = _state.InsetViewport ? 100 : 0; DisplayRect v = _state.Viewport;
        long vl = (long)v.Left + margin, vt = (long)v.Top + margin, vr = (long)v.Right - margin, vb = (long)v.Bottom - margin;
        return vl <= vr && vt <= vb && left <= right && top <= bottom && vl < right && left < vr && vt < bottom && top < vb;
    }
    /// <summary>두 판본의 여백/카메라 감산순서를 보존하고 viewport로 잘라 sink에 직접 전달한다.</summary>
    private void Invalidate(float left, float top, float right, float bottom)
    {
        DisplayRect v = _state.Viewport; bool patch = Pool.Edition == OriginalEdition.Patch1078;
        // PATCH는 여백→카메라, CD는 카메라→여백의 넓은 계산이다.
        int Coordinate(float value, float camera, double edge) => Integer(patch ? ((double)value + edge) - camera : ((double)value - camera) + edge);
        var rect = new DisplayRect(Math.Clamp(Coordinate(left, _state.CameraX, -1), v.Left, v.Right), Math.Clamp(Coordinate(top, _state.CameraY, -1), v.Top, v.Bottom),
            Math.Clamp(Coordinate(right, _state.CameraX, 1), v.Left, v.Right), Math.Clamp(Coordinate(bottom, _state.CameraY, 1), v.Top, v.Bottom));
        if (rect.Left < rect.Right && rect.Top < rect.Bottom) _sink.Invalidate(rect, 0);
    }
    /// <summary>PATCH helper/CD inline의 equal/ordered 비교와float저장을 유지하며 점을 포함한다.</summary>
    private static void IncludePoint(bool patch, float x, float y, ref float left, ref float top, ref float right, ref float bottom)
    {
        float nr = (float)((double)x + 1), nb = (float)((double)y + 1);
        if (patch)
        {
            left = left < x ? left : x; top = top < y ? top : y; right = nr < right ? right : nr;
            if (bottom <= nb) bottom = nb;
        }
        else { left = left <= x ? left : x; top = top <= y ? top : y; right = nr <= right ? right : nr; bottom = nb <= bottom ? bottom : nb; }
    }
    /// <summary>capacity0/첫캡처/첫좌표/visible시각/source/전체capacity입자/두dirty 순서다.</summary>
    public void Step(BurningVisualFields fields, BurningFrameFields frame)
    {
        if (fields.Capacity == 0) return;
        if (fields.Particles.Length != fields.Capacity || fields.Total > fields.Capacity) throw new ArgumentException("Burning frame 현재입자 배열 오류");
        if (fields.Stage == 0) { fields.LastTime = _state.Now; fields.Elapsed = 0; Capture(fields); fields.Stage++; return; }
        bool patch = Pool.Edition == OriginalEdition.Patch1078; int selected = fields.Victim;
        if (patch)
        {
            int? victim = RawBurningVictim.Run(() => fields.Victim, BurningVictimEntry.Frame, _state.Victim.Checking, RawBurningVictim.MakeHooks(Pool, _types, _state.Victim));
            if (victim is null) return; selected = victim.Value;
        }
        else if (selected is 0 or 65535) return;
        ReadOnlySpan<byte> raw = Pool.Slot(selected); if (!patch && !HasHitPoints(raw)) return;
        float x = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), y = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
        float centerX = (float)((double)x * _state.PixelScaleX), centerY = (float)((double)y * _state.PixelScaleY);
        fields.Registration.ZOrder = unchecked((sbyte)raw[patch ? 35 : 33]) - 1;
        if (fields.Stage == 1)
        {
            fields.Registration.Left = fields.Registration.Right = centerX; fields.Registration.Top = fields.Registration.Bottom = centerY;
            fields.Off = true; fields.Stage = 2; return;
        }
        if (!Visible(fields.Registration))
        {
            if (!fields.Off && fields.Sound != 0) (_hooks.StopSound ?? throw new InvalidOperationException("Burning frame 실제 nonzero소리 정지 미연결"))(fields.Sound);
            fields.Off = true; return;
        }
        if (fields.Off) { fields.LastTime = _state.Now; fields.Off = false; }
        double wide = (_state.Now - fields.LastTime) * 10; frame.Delta = (float)wide; frame.ScaledDelta = (float)(wide * _state.DeltaMultiplier); fields.LastTime = _state.Now; fields.Elapsed += frame.Delta;
        _sources.Select(fields); float left = centerX, top = centerY, right = centerX, bottom = centerY;
        // 원문은total대신 전체capacity를 처리하여 같은 프레임에 새 후방입자도 실행한다.
        for (int index = 0; index < fields.Capacity; index++)
        {
            ref BurningParticle p = ref fields.Particles[index]; bool active = patch ? p.Life > 0 : BitConverter.SingleToInt32Bits(p.Life) > 0;
            if (!active) continue;
            if (p.Source)
            {
                if (!(Random(0, 1.4f) < frame.Delta)) continue;
                float px = patch ? (float)(((double)centerX - fields.Image.HotspotX) + p.X) : (float)(((double)p.X - fields.Image.HotspotX) + centerX);
                float py = patch ? (float)(((double)centerY - fields.Image.HotspotY) + p.Y) : (float)(((double)p.Y - fields.Image.HotspotY) + centerY);
                var emitted = new BurningParticle(px, py, 0, -2, (float)Random(4, 9), false);
                if (_visual.Append(fields, emitted)) IncludePoint(patch, emitted.X, emitted.Y, ref left, ref top, ref right, ref bottom);
            }
            else
            {
                p.Life = (float)((double)p.Life - frame.Delta); double velocity = (double)p.VelocityX + _state.ReadWind(); p.VelocityX = (float)velocity;
                p.X = (float)((patch ? velocity : p.VelocityX) * frame.Delta + p.X); p.Y = (float)((double)p.VelocityY * frame.Delta + p.Y);
                IncludePoint(patch, p.X, p.Y, ref left, ref top, ref right, ref bottom);
            }
        }
        right = (float)((double)right + 2); bottom = (float)((double)bottom + 1); float minimum = (float)((double)right - 17);
        left = patch ? (minimum <= left ? minimum : left) : (left <= minimum ? left : minimum); top = (float)((double)top - 2);
        Invalidate(fields.Registration.Left, fields.Registration.Top, fields.Registration.Right, fields.Registration.Bottom); Invalidate(left, top, right, bottom);
        fields.Registration.Left = left; fields.Registration.Top = top; fields.Registration.Right = right; fields.Registration.Bottom = bottom;
    }
}
