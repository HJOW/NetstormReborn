using System.Buffers.Binary;

// 원본 건설 표시의 생성·좌표 동기화·초기 입자 선택을 같은 난수와 버퍼에서 실행한다.
namespace Netstorm.Core.Original;

/// <summary>Build.cpp의00423840/00423900·00423770·00423560 초기 표시 본체다.</summary>
/// <remarks>근거: cpppj/src/o/RawConstructionVisual.cpp·h. 2026-11-20 0.673 이식.</remarks>
public sealed class RawConstructionVisual
{
    /// <summary>원본 표시 객체·공유 색 표·입자 한 칸의 확보 바이트다.</summary>
    private const uint ObjectBytes = 0x88, PaletteBytes = 8, ParticleBytes = 12;
    /// <summary>최소 확보 입자 수와 불투명 픽셀 수의 비율 분모다.</summary>
    private const uint MinParticles = 500, PixelRatio = 6;
    /// <summary>원본 팔레트의 투명 픽셀이다.</summary>
    private const byte Transparent = 255;
    /// <summary>일반 건설의 원본float32 최소 수명이다.</summary>
    private static readonly float NormalMinimum = BitConverter.UInt32BitsToSingle(0x3fa66666);
    /// <summary>현재 부모 raw 자료를 읽는 같은 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>공유 색 표·현재 표시 역할·좌표 배율이다.</summary>
    private readonly ConstructionVisualState _state;
    /// <summary>다른 게임 계산과 같은 현재 난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>원문 표시 효과의 대리자 사본이다.</summary>
    private readonly ConstructionVisualHooks _hooks;

    /// <summary>첫 예약·부모 변경 전에 필수 표시 장치만 확인한다.</summary>
    public RawConstructionVisual(RawSidPool pool, ConstructionVisualState state, GameRandom random, ConstructionVisualHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Reserve is null || hooks.Attach is null || hooks.CreatePalette is null || hooks.Gradient is null ||
            hooks.BuildPalette is null || hooks.Capture is null || hooks.ReleaseParticles is null)
            throw new ArgumentException("건설 표시 경계 누락", nameof(hooks));
        _pool = pool; _state = state; _random = random; _hooks = hooks.Copy();
    }

    /// <summary>현재 부모 검사→등록→공유 색 표→수명→캡처→좌표/입자 초기화 순서다.</summary>
    public ConstructionVisualFields Create(int parent, bool instant)
    {
        if (_pool.Edition == OriginalEdition.Patch1078) ValidatePatch(parent);
        else
        {
            if (parent == 0) throw new ArgumentOutOfRangeException(nameof(parent), "건설 표시 부모 번호 오류");
            _ = _pool.Slot(parent);
        }
        Reserve(ObjectBytes); var fields = new ConstructionVisualFields { Parent = parent };
        _hooks.Attach!(fields, _state.ProcessType);
        fields.Capacity = fields.Total = 0; fields.Particles = []; fields.LastTime = fields.Elapsed = 0;
        fields.Image.Pixels = []; fields.Selected = fields.Opaque = fields.Active = 0; fields.FirstDraw = false;
        if (_state.Palette == 0)
        {
            Reserve(PaletteBytes); _state.Palette = _hooks.CreatePalette!();
            if (_state.Palette == 0) throw new OutOfMemoryException("건설 공유 색 표 생성 실패");
            // 파랑→흰색8색과 흰색→128/255회색3색은 원본 값형 RGB 사본으로 전달한다.
            _hooks.Gradient!(8, [0, 0, 1], [1, 1, 1], 2.4);
            _hooks.Gradient!(3, [1, 1, 1], [128.0 / 255.0, 128.0 / 255.0, 128.0 / 255.0], 2.4);
            _hooks.BuildPalette!(_state.Palette);
        }
        fields.Palette = _state.Palette; fields.Instant = instant;
        fields.Minimum = instant ? 2f : NormalMinimum; fields.Maximum = instant ? 9f : 2f;
        fields.Image = _hooks.Capture!(parent);
        uint count = PixelCount(fields); Reserve(count);
        fields.LastRow = (fields.Image.Height - 1) * fields.Image.Width;
        fields.Output = new byte[count]; Array.Fill(fields.Output, Transparent);
        Synchronize(fields); return fields;
    }

    /// <summary>현재 등록 부모의 signed 깊이·좌표로 두 모서리를 맞춘 뒤 같은 입자를 재선택한다.</summary>
    public void Synchronize(ConstructionVisualFields fields)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(fields.Parent);
        fields.ZOrder = unchecked((sbyte)raw[_pool.Edition == OriginalEdition.Patch1078 ? 35 : 33]);
        fields.Left = fields.Right = (float)((double)_state.ScaleX * Float(raw, 14));
        fields.Top = fields.Bottom = (float)((double)_state.ScaleY * Float(raw, 18));
        Seed(fields);
    }

    /// <summary>중앙 교차 열·하단 첫 픽셀·폭만큼의 섞기를 실행하며 누적Selected를 보존한다.</summary>
    public void Seed(ConstructionVisualFields fields)
    {
        uint count = PixelCount(fields);
        if (fields.Output.Length != count) throw new ArgumentException("건설 표시 출력 크기 오류", nameof(fields));
        fields.Opaque = (uint)fields.Image.Pixels.Count(pixel => pixel != Transparent);
        fields.Capacity = Math.Max(MinParticles, fields.Opaque / PixelRatio);
        if (fields.Particles.Length != 0) _hooks.ReleaseParticles!();
        Reserve(fields.Capacity * ParticleBytes); fields.Particles = new ConstructionVisualParticle[fields.Capacity]; fields.Total = 0;
        fields.Output[^1] = fields.Image.Pixels[^1]; fields.Image.Pixels[^1] = Transparent;
        int width = fields.Image.Width, height = fields.Image.Height, column = (width - 1) / 2;
        // 중앙에서0,+1,-2,+3 순서로 열을 선택한다.
        for (int step = 0; step < width && fields.Total <= (uint)width * 2; step++)
        {
            column += (step & 1) != 0 ? step : -step;
            // 높이/3 행 자체는 제외하고 그 아래의 첫 불투명 픽셀만 선택한다.
            for (int row = height - 1; row > height / 3; row--)
            {
                uint pixel = (uint)(row * width + column);
                if (fields.Image.Pixels[pixel] == Transparent) continue;
                if (fields.Total < fields.Capacity)
                {
                    ref ConstructionVisualParticle particle = ref fields.Particles[fields.Total++];
                    double fraction = _random.Next(65536) / 65536.0;
                    particle.Life = (float)(fraction * Math.Abs((double)fields.Maximum - fields.Minimum) + fields.Minimum);
                    particle.Pixel = pixel; particle.Color = fields.Image.Pixels[pixel];
                    fields.Image.Pixels[pixel] = Transparent; fields.Selected = unchecked(fields.Selected + 1);
                }
                break;
            }
        }
        if (fields.Total != 0)
        {
            // 폭만큼 두 난수를 소비하고 입자 전체 값을 교환한다.
            for (int index = 0; index < width; index++)
            {
                uint first = _random.Next(fields.Total), second = _random.Next(fields.Total);
                (fields.Particles[first], fields.Particles[second]) = (fields.Particles[second], fields.Particles[first]);
            }
        }
    }

    /// <summary>같은 풀의 건설 시작에 표시 생성을 연결하고 완성 필드의 소유권을 저장소에 넘긴다.</summary>
    public ConstructionProcessHooks MakeProcessHooks(RawSidPool pool, Action<ConstructionVisualFields> store, ConstructionProcessHooks hooks)
    {
        if (!ReferenceEquals(pool, _pool) || store is null) throw new ArgumentException("건설 표시 어댑터 연결 오류");
        ConstructionProcessHooks result = hooks.Copy(); result.Effect = (parent, instant) => store(Create(parent, instant)); return result;
    }
    /// <summary>세션 조립에서 같은 풀인지 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>실패한 원본new의 후속null 접근을 안전한 확보 예외로 바꾼다.</summary>
    private void Reserve(uint bytes)
    {
        if (!_hooks.Reserve!(bytes)) throw new OutOfMemoryException("건설 표시 확보 실패");
    }
    /// <summary>원본 크기 곱·버퍼 폭을 실제 픽셀 쓰기 전에 확인한다.</summary>
    private static uint PixelCount(ConstructionVisualFields fields)
    {
        ConstructionVisualImage image = fields.Image;
        if (image.Width <= 0 || image.Height <= 0 || (ulong)image.Width * (ulong)image.Height > int.MaxValue ||
            (ulong)image.Width * (ulong)image.Height != (ulong)image.Pixels.Length)
            throw new ArgumentException("건설 표시 캡처 크기 오류", nameof(fields));
        return (uint)image.Pixels.Length;
    }
    /// <summary>PATCH의 현재 부모 자산·소유자·공간 상태와 종속Form 체인을 확인한다.</summary>
    private void ValidatePatch(int parent)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(parent); uint vtable = Read(raw, 0);
        if (parent == 0 || raw[34] >= 10 || vtable == 0 || vtable == 0xdddddddd || vtable == 0xcdcdcdcd ||
            raw[10] < 70 || (raw[11] & 7) != 0) throw new InvalidOperationException("건설 표시 부모 유효성 오류");
        int current = BinaryPrimitives.ReadUInt16LittleEndian(raw.Slice(6, 2)); var visited = new HashSet<int>();
        // free/void Form과순환은 거부하며 dead 비트만으로 제외하지 않는다.
        while (current != 0)
        {
            if (current >= _pool.Capacity || !visited.Add(current)) throw new InvalidOperationException("건설 표시 종속 체인 오류");
            ReadOnlySpan<byte> child = _pool.Slot(current);
            if (child[10] >= 70 || (child[11] & 5) != 0) throw new InvalidOperationException("건설 표시 종속 상태 오류");
            current = BinaryPrimitives.ReadUInt16LittleEndian(child.Slice(4, 2));
        }
    }
    /// <summary>원본 비정렬DWORD를 그대로 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4));
    /// <summary>원본 저장 좌표의float 비트를 복원한다.</summary>
    private static float Float(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(Read(raw, offset));
}
