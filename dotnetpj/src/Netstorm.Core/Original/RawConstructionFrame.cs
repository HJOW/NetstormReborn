using System.Buffers.Binary;

// 원본 건설 입자의 시각·활성화·인접 확장·만료·영역 갱신과 출력 제출을 처리한다.
namespace Netstorm.Core.Original;

/// <summary>생성 단계에서 넘겨받은 소유 버퍼와 원본 두 시간 차이다.</summary>
public sealed class ConstructionFrameFields
{
    /// <summary>같은 표시 객체가 소유하는 픽셀·입자 상태다.</summary>
    public ConstructionVisualFields Visual { get; set; } = new();
    /// <summary>시간 차이*10의float 저장값이다.</summary>
    public float Delta { get; set; }
    /// <summary>저장 전 시간 차이에 현재 배율을 곱한float 값이다.</summary>
    public float ScaledDelta { get; set; }
}

/// <summary>프레임 몸체가 경계 뒤 다시 읽는 현재 시각·카메라 전역이다.</summary>
public sealed class ConstructionFrameState
{
    /// <summary>현재 게임 시각이다.</summary>
    public double Now { get; set; }
    /// <summary>ScaledDelta에만 곱하는 현재 배율이다.</summary>
    public float Rate { get; set; } = 1;
    /// <summary>dirty/Draw에서 빼는 현재 가로 원점이다.</summary>
    public float CameraX { get; set; }
    /// <summary>dirty/Draw에서 빼는 현재 세로 원점이다.</summary>
    public float CameraY { get; set; }
}

/// <summary>원본의 색수DWORD와 실제로 읽을 수 있는 인접DWORD 배열을 구별한다.</summary>
public sealed class ConstructionFramePalette
{
    /// <summary>생성 표시의 공유 팔레트와 같은 식별자다.</summary>
    public uint Token { get; set; }
    /// <summary>수명 비율에 곱할signed 색수다.</summary>
    public int Count { get; set; }
    /// <summary>색수 경계의 인접 항목도 명시적으로 포함하는 읽기 배열이다.</summary>
    public IReadOnlyList<uint> Entries { get; set; } = Array.Empty<uint>();
}

/// <summary>원본dirty 큐에 넘기는signed 정수 사각형이다.</summary>
/// <param name="Left">왼쪽 경계다.</param>
/// <param name="Top">위쪽 경계다.</param>
/// <param name="Right">오른쪽 경계다.</param>
/// <param name="Bottom">아래쪽 경계다.</param>
public readonly record struct ConstructionFrameRect(int Left, int Top, int Right, int Bottom);

/// <summary>현재 버퍼를 장치에 제출하는 요청이며 호출 중에만 읽고 보관하지 않는다.</summary>
public sealed class ConstructionFrameDraw
{
    /// <summary>현재 부모·카메라·기준점으로 구한 출력 가로 위치다.</summary>
    public int X { get; init; }
    /// <summary>현재 부모·카메라·기준점으로 구한 출력 세로 위치다.</summary>
    public int Y { get; init; }
    /// <summary>원본 출력 버퍼의 행 폭이다.</summary>
    public int Pitch { get; init; }
    /// <summary>원본 입력 범위0,0,width,height다.</summary>
    public ConstructionFrameRect Clip { get; init; }
    /// <summary>현재 출력 버퍼를 복사 없이 빌린다.</summary>
    public ReadOnlyMemory<byte> Pixels { get; init; }
}

/// <summary>현재 최대HP·dirty 큐·장치·프로세스 종료 장치를 연결한다.</summary>
public sealed class ConstructionFrameHooks
{
    /// <summary>현재 부모의 최대HP이며 호출 중 raw/전역을 바꿀 수 있다.</summary>
    public Func<int, int>? MaxHitPoints { get; set; }
    /// <summary>이전/새 영역을 순서대로 큐에 넣는다.</summary>
    public Action<ConstructionFrameRect>? Invalidate { get; set; }
    /// <summary>현재 픽셀과 원본blit 인자를 호출 중 소비한다.</summary>
    public Action<ConstructionFrameDraw>? Draw { get; set; }
    /// <summary>abstract가 풀린 뒤flags0으로 현재 프로세스를 종료한다.</summary>
    public Action<uint>? Kill { get; set; }
    /// <summary>C++ 함수 값 묶음처럼 대리자를 복사한다.</summary>
    public ConstructionFrameHooks Copy() => (ConstructionFrameHooks)MemberwiseClone();
}

/// <summary>00422f00·00423200·00422ea0·00422e30·00487e50의 건설 표시 프레임 본체다.</summary>
/// <remarks>근거: cpppj/src/o/RawConstructionFrame.cpp·h. 2026-11-20 0.674 이식.</remarks>
public sealed class RawConstructionFrame
{
    /// <summary>원본 팔레트의 투명 항목이다.</summary>
    private const byte Transparent = 255;
    /// <summary>원본double 시각 차이 계수다.</summary>
    private const double TimeScale = 10;
    /// <summary>부모의 현재 raw 상태를 읽는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 월드→화면 배율이다.</summary>
    private readonly ConstructionVisualState _visualState;
    /// <summary>현재 게임 시각·카메라다.</summary>
    private readonly ConstructionFrameState _state;
    /// <summary>실제 공유 색 표의 현재 읽기 자료다.</summary>
    private readonly ConstructionFramePalette _palette;
    /// <summary>생성 단계와 공유하는 실제 난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>현재 경계 대리자의 사본이다.</summary>
    private readonly ConstructionFrameHooks _hooks;

    /// <summary>첫 픽셀 변경 전에 외부 표시·종료 장치를 확인한다.</summary>
    public RawConstructionFrame(RawSidPool pool, ConstructionVisualState visualState, ConstructionFrameState state,
        ConstructionFramePalette palette, GameRandom random, ConstructionFrameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(visualState); ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(palette); ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.MaxHitPoints is null || hooks.Invalidate is null || hooks.Draw is null || hooks.Kill is null)
            throw new ArgumentException("건설 프레임 경계 누락", nameof(hooks));
        _pool = pool; _visualState = visualState; _state = state; _palette = palette; _random = random; _hooks = hooks.Copy();
    }

    /// <summary>용량 안에서 입자를 추가한다. CD 잘못된 픽셀은total을 먼저 늘리고 실패한다.</summary>
    public bool Append(ConstructionFrameFields fields, int pixel)
    {
        ConstructionVisualFields v = fields.Visual;
        if (v.Total >= v.Capacity) return false;
        if (v.Particles.Length != v.Capacity) throw new ArgumentException("건설 프레임 입자 용량 오류");
        bool valid = pixel >= 0 && pixel < v.Image.Pixels.Length && v.Output.Length == v.Image.Pixels.Length;
        if (!valid && Patch) throw new ArgumentOutOfRangeException(nameof(pixel), "건설 입자 픽셀 오류");
        ref ConstructionVisualParticle particle = ref v.Particles[v.Total++]; if (!valid) return false;
        particle.Life = (float)Life(v.Minimum, v.Maximum); particle.Pixel = (uint)pixel;
        particle.Active = 0; particle.Color = v.Image.Pixels[pixel]; v.Image.Pixels[pixel] = Transparent;
        v.Selected = unchecked(v.Selected + 1); return true;
    }

    /// <summary>원본 선형 픽셀에서 오른쪽·왼쪽·위 순서로 현재 불투명 이웃을 추가한다.</summary>
    public void Expand(ConstructionFrameFields fields, uint pixel)
    {
        ConstructionVisualFields v = fields.Visual; Validate(fields);
        if (pixel <= (uint)v.Image.Width || pixel >= (uint)v.LastRow) return;
        // 행의좌우 경계를 따로 보정하지 않고 앞 선택이 바꾼 버퍼·용량을 다시 읽는다.
        foreach (uint neighbor in new[] { pixel + 1, pixel - 1, pixel - (uint)v.Image.Width })
            if (v.Image.Pixels[neighbor] != Transparent) _ = Append(fields, (int)neighbor);
    }

    /// <summary>판본별 역수float 저장/나눗셈·절삭 순서로 실제 색DWORD를 조회한다.</summary>
    public uint Color(float life, float maximum)
    {
        if (!float.IsFinite(life) || !float.IsFinite(maximum) || maximum <= 0 || _palette.Count < 0)
            throw new ArgumentException("건설 색 표 입력 오류");
        float value = Math.Min(life, maximum); double position;
        if (Patch)
        {
            float reciprocal = (float)(1.0 / maximum);
            position = (double)(float)_palette.Count * value * reciprocal;
        }
        else position = (double)value * _palette.Count / maximum;
        int index = Math.Max(0, Ftol(position));
        if (index >= _palette.Entries.Count) throw new ArgumentOutOfRangeException(nameof(life), "건설 색 표 읽기 범위 오류");
        return _palette.Entries[index];
    }

    /// <summary>첫프레임 초기화 뒤 저장 전 시각 차이로두 배율을 구하고 저장float를double에 누적한다.</summary>
    public void Clock(ConstructionFrameFields fields)
    {
        ConstructionVisualFields v = fields.Visual;
        if (!v.FirstDraw) { v.LastTime = _state.Now; v.Elapsed = 0; v.FirstDraw = true; }
        double difference = (_state.Now - v.LastTime) * TimeScale;
        fields.Delta = (float)difference; fields.ScaledDelta = (float)(difference * _state.Rate);
        v.LastTime = _state.Now; v.Elapsed += fields.Delta;
    }

    /// <summary>시각→현재HP 목표 활성화/확장→만료/색→영역 갱신→현재abstract 종료 순서다.</summary>
    public void Run(ConstructionFrameFields fields)
    {
        ConstructionVisualFields v = fields.Visual;
        if (!Patch && (v.Image.Pixels.Length == 0 || v.Parent == 0 || v.Parent >= _pool.Capacity)) return;
        if (v.Parent == 0) throw new ArgumentOutOfRangeException(nameof(fields), "건설 프레임 부모 오류");
        ReadOnlySpan<byte> raw = _pool.Slot(v.Parent); if (!Patch && (raw[11] & 1) != 0) return; Validate(fields);
        double lx = (double)_visualState.ScaleX * Float(raw, 14) - v.Image.HotspotX;
        double ty = (double)_visualState.ScaleY * Float(raw, 18) - v.Image.HotspotY;
        float left = (float)lx, top = (float)ty;
        float right = (float)((Patch ? (double)left : lx) + v.Image.Width), bottom = (float)(ty + v.Image.Height);
        Clock(fields);
        float savedHp = BinaryPrimitives.ReadInt32LittleEndian(raw.Slice(26, 4));
        int maximum = _hooks.MaxHitPoints!(v.Parent);
        double hp = Patch ? savedHp : BinaryPrimitives.ReadInt16LittleEndian(raw.Slice(26, 2));
        int denominator = unchecked(maximum - 1);
        int target = Ftol(hp / denominator * unchecked((int)v.Opaque));
        // 확장 뒤 현재selected/total을 다시 읽고 활성 앞 구간에서 후보를 선택한다.
        while (unchecked((int)v.Selected) < target && v.Active < v.Total && v.Total < v.Capacity)
        {
            uint index = v.Active + _random.Next(v.Total - v.Active), pixel = v.Particles[index].Pixel;
            Expand(fields, pixel); v.Particles[index].Active = 1;
            (v.Particles[v.Active], v.Particles[index]) = (v.Particles[index], v.Particles[v.Active]); v.Active++;
        }
        uint i = 0;
        // 만료 입자는 마지막 활성/전체 입자로 두 번 덮고 같은 위치를 다시 처리한다.
        while (i < v.Active)
        {
            ref ConstructionVisualParticle particle = ref v.Particles[i];
            double remainder = (double)particle.Life - fields.Delta; particle.Life = (float)remainder;
            if (remainder <= 0)
            {
                v.Output[particle.Pixel] = particle.Color; v.Active--; v.Total--;
                particle = v.Particles[v.Active]; v.Particles[v.Active] = v.Particles[v.Total];
            }
            else
            {
                CheckPalette(v); v.Output[particle.Pixel] = (byte)Color(particle.Life, v.Maximum); i++;
            }
        }
        // 미활성 입자는 원래 색 확정 없이 재시작 수명과 현재 팔레트 색만 갱신한다.
        for (; i < v.Total; i++)
        {
            ref ConstructionVisualParticle particle = ref v.Particles[i];
            double remainder = (double)particle.Life - fields.Delta; particle.Life = (float)remainder;
            if (remainder <= 0)
            {
                double life = Life(0, v.Maximum);
                particle.Life = (float)(Patch ? life + fields.Delta + particle.Life : life + particle.Life + fields.Delta);
            }
            CheckPalette(v); v.Output[particle.Pixel] = (byte)Color(particle.Life, v.Maximum);
        }
        Rectangle(v, left, top, right, bottom);
        if ((raw[Patch ? 40 : 35] & 1) == 0) _hooks.Kill!(0);
    }

    /// <summary>total0이어도capacity가있으면 현재부모·카메라로출력버퍼를제출한다.</summary>
    public void Draw(ConstructionFrameFields fields)
    {
        ConstructionVisualFields v = fields.Visual; if (v.Capacity == 0) return;
        if (v.Total > v.Capacity) throw new ArgumentException("건설 Draw 입자 수 오류");
        if (v.Parent == 0) throw new ArgumentOutOfRangeException(nameof(fields), "건설 Draw 부모 오류");
        ReadOnlySpan<byte> raw = _pool.Slot(v.Parent);
        float x = (float)((double)_visualState.ScaleX * Float(raw, 14) - _state.CameraX);
        double y = (double)_visualState.ScaleY * Float(raw, 18) - _state.CameraY;
        _hooks.Draw!(new ConstructionFrameDraw { X = Ftol((double)x - v.Image.HotspotX), Y = Ftol(y - v.Image.HotspotY), Pitch = v.Image.Width,
            Clip = new(0, 0, v.Image.Width, v.Image.Height), Pixels = v.Output });
    }
    /// <summary>현재 원본 판본이다.</summary>
    private bool Patch => _pool.Edition == OriginalEdition.Patch1078;
    /// <summary>난수 수명은 저장 이전double 값도 보존한다.</summary>
    private double Life(float minimum, float maximum) => _random.Next(65536) / 65536.0 * Math.Abs((double)maximum - minimum) + minimum;
    /// <summary>현재표시의공유팔레트연결을실제색쓰기직전에확인한다.</summary>
    private void CheckPalette(ConstructionVisualFields visual)
    {
        if (visual.Palette != _palette.Token) throw new ArgumentException("건설 색 표 연결 오류");
    }
    /// <summary>이전/새영역을현재카메라로요청한뒤등록사각형을갱신한다.</summary>
    private void Rectangle(ConstructionVisualFields visual, float left, float top, float right, float bottom)
    {
        _hooks.Invalidate!(Rect(visual.Left, visual.Top, visual.Right, visual.Bottom));
        _hooks.Invalidate!(Rect(left, top, right, bottom));
        visual.Left = left; visual.Top = top; visual.Right = right; visual.Bottom = bottom;
    }
    /// <summary>카메라와1픽셀여유를원문순서로정수화한다.</summary>
    private ConstructionFrameRect Rect(float left, float top, float right, float bottom) => new(Ftol((double)left - _state.CameraX - 1),
        Ftol((double)top - _state.CameraY - 1), Ftol((double)right - _state.CameraX + 1), Ftol((double)bottom - _state.CameraY + 1));
    /// <summary>현재픽셀/입자폭과사용중목적지·수명을쓰기전에검사한다.</summary>
    private static void Validate(ConstructionFrameFields fields)
    {
        ConstructionVisualFields v = fields.Visual;
        if (v.Image.Width <= 0 || v.Image.Height <= 0 || (ulong)v.Image.Width * (ulong)v.Image.Height != (ulong)v.Image.Pixels.Length ||
            v.Output.Length != v.Image.Pixels.Length || v.LastRow != (v.Image.Height - 1) * (long)v.Image.Width ||
            v.Capacity > int.MaxValue || v.Particles.Length != v.Capacity || v.Total > v.Capacity || v.Active > v.Total)
            throw new ArgumentException("건설 프레임 저장소 오류");
        // 원문이현재사용하는항목의목적지와유한수명만검사한다.
        for (uint i = 0; i < v.Total; i++)
            if (v.Particles[i].Pixel >= v.Output.Length || !float.IsFinite(v.Particles[i].Life)) throw new ArgumentException("건설 프레임 입자 오류");
    }
    /// <summary>원본signed64절삭의하위DWORD이며비유한/범위밖은0이다.</summary>
    private static int Ftol(double value) => !double.IsFinite(value) || Math.Abs(value) >= 9223372036854775808.0 ? 0 : unchecked((int)(uint)(long)value);
    /// <summary>현재비정렬좌표의float비트를읽는다.</summary>
    private static float Float(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4)));
}
