using System.Buffers.Binary;
using Netstorm.Core.Simulation;

// 현재 최대HP/캡처/난수와 같은 실제Append로 연소점 행을 선택한다.
namespace Netstorm.Core.Original;

/// <summary>연소점 부모가 매 호출 읽는 같은 장기 HP/역할/진단 owner다.</summary>
public sealed class BurningSourcesState
{
    /// <summary>독립 입력의 마나HP1/4 옵션이다.</summary>
    public bool WeakenedMana { get; set; }
    /// <summary>독립 입력의 현재 마나/사제TYPE다.</summary>
    public uint ManaType { get; set; } = 158;
    /// <summary>독립 입력의 HP예외 다이스TYPE다.</summary>
    public uint DaisType { get; set; } = 165;
    /// <summary>실제 같은 장기 약화 bool주소의 조회다.</summary>
    public Func<bool>? SharedWeakenedMana { get; set; }
    /// <summary>실제 같은 사제TYPE DWORD다.</summary>
    public UIntBox? SharedManaType { get; set; }
    /// <summary>실제 같은 다이스TYPE DWORD다.</summary>
    public UIntBox? SharedDaisType { get; set; }
    /// <summary>연소 부모가 빌리는 현재checking/상세조회/복귀진단이다.</summary>
    public BurningVictimState Victim { get; } = new();
}
/// <summary>00424110/CD004a5d30의 연소점 선택·300시도/7점폴백·제거다.</summary>
public sealed class RawBurningSources
{
    /// <summary>고정 source행의 대각선 폭이다.</summary>
    private const int RowWidth = 5;
    /// <summary>같은 현재 raw 피해 대상 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>같은 실제TYPE표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>실제150입자Append 몸체다.</summary>
    private readonly RawBurningVisual _visual;
    /// <summary>게임 전체가 공유하는 원문32비트 난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>현재 장기 약화/역할/진단 상태다.</summary>
    private readonly BurningSourcesState _state;
    /// <summary>같은 피해/Append풀을 확인하고 현재자료를 빌린다.</summary>
    public RawBurningSources(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawBurningVisual visual, GameRandom random, BurningSourcesState state)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(visual); ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(state);
        if (!ReferenceEquals(pool, visual.Pool)) throw new ArgumentException("Burning sources/Append SID풀 불일치");
        Pool = pool; _types = types; _visual = visual; _random = random; _state = state;
    }
    /// <summary>원문 정수 난수의 포함하한/제외상한을 같은 스트림으로 계산한다.</summary>
    private int Random(int minimum, int maximum)
    {
        if (maximum <= minimum) throw new ArgumentException("Burning sources 원본 난수 나눗셈 범위 오류");
        return minimum + (int)_random.Next((uint)(maximum - minimum));
    }
    /// <summary>현재 캡처의 정상 픽셀만 읽고0xff투명색을 검사한다.</summary>
    private static bool Opaque(BurningVisualFields fields, int x, int y)
    {
        ConstructionVisualImage image = fields.Image;
        if (x < 0 || y < 0 || x >= image.Width || y >= image.Height) throw new ArgumentOutOfRangeException(nameof(x), "Burning sources 캡처 픽셀 범위 오류");
        long index = (long)y * image.Width + x;
        if (index >= image.Pixels.Length) throw new ArgumentOutOfRangeException(nameof(fields), "Burning sources 캡처 버퍼 미연결");
        return image.Pixels[(int)index] != 255;
    }
    /// <summary>각float를 먼저0방향절삭한 뒤 signed합과 행 나눗셈을 수행한다.</summary>
    private static int Row(BurningParticle particle)
    {
        if (!float.IsFinite(particle.X) || !float.IsFinite(particle.Y) || (double)particle.X < -2147483648.0 || (double)particle.X >= 2147483648.0 || (double)particle.Y < -2147483648.0 || (double)particle.Y >= 2147483648.0)
            throw new ArgumentException("Burning sources 연소점 좌표 오류");
        long sum = (long)(int)particle.X + (int)particle.Y;
        if (sum < int.MinValue || sum > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(particle), "Burning sources 행 덧셈 범위 오류");
        return (int)sum / RowWidth;
    }
    /// <summary>저장HP→실제최대HP/복귀진단→현재TYPE강제1→픽셀/행선택 순서를 유지한다.</summary>
    public void Select(BurningVisualFields fields)
    {
        bool patch = Pool.Edition == OriginalEdition.Patch1078; int selected = fields.Victim;
        if (patch)
        {
            int? victim = RawBurningVictim.Run(() => fields.Victim, BurningVictimEntry.Sources, _state.Victim.Checking, RawBurningVictim.MakeHooks(Pool, _types, _state.Victim));
            if (victim is null) return;
            selected = victim.Value;
        }
        else if (selected is 0 or 65535) return;
        ReadOnlySpan<byte> raw = Pool.Slot(selected);
        if (!patch)
        {
            int number = raw[10]; if (number >= _types.Count) throw new ArgumentOutOfRangeException(nameof(fields), "Burning sources 현재타입 범위 오류");
            OriginalTypeRecord current = _types[number];
            if ((current.Flags2 & 0x400000) != 0 ? (raw[35] & 1) == 0 : (current.Flags1 & 0x10) == 0) return;
        }
        int hitPoints = patch ? BinaryPrimitives.ReadInt32LittleEndian(raw[26..]) : BinaryPrimitives.ReadInt16LittleEndian(raw[26..]);
        int type = raw[10]; if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(fields), "Burning sources 현재타입 범위 오류");
        // 원문 조건연산자가 고정한 같은 bool/DWORD주소를 getter복귀 뒤에도 빌린다.
        Func<bool> quarter = _state.SharedWeakenedMana ?? (() => _state.WeakenedMana); UIntBox? priest = _state.SharedManaType, dais = _state.SharedDaisType;
        var globals = new RiftHitPointGlobals { Checking = new(_state.Victim.Checking), Quarter = new(quarter), Priest = priest is null ? () => _state.ManaType : () => priest.Value, Dais = dais is null ? () => _state.DaisType : () => dais.Value };
        var hooks = new RiftHitPointHooks
        {
            Number = () => RawRiftHitPoints.CurrentRiftTypeNumber(type, _types, _types),
            Assertion = (expression, file, line) => { Action<string, string, uint> assertion = _state.Victim.Assertion ?? throw new InvalidOperationException("연소점 TYPE HP의 실제 진단 장치 미연결"); assertion(expression, file, line); },
        };
        int maximum = RawRiftHitPoints.ReadSelected(Pool.Edition, () => _types[type], globals, hooks);
        int wanted = RawBurningSourceCount.Count(Pool.Edition, hitPoints, maximum, fields.Image.Width, fields.Image.Height);
        int currentType = raw[10]; if (currentType >= _types.Count) throw new ArgumentOutOfRangeException(nameof(fields), "연소점 진단 복귀 뒤 현재TYPE 범위");
        if ((_types[currentType].Flags2 & 0x400) != 0) wanted = 1;
        if (fields.Selected == wanted) return;
        if (fields.Image.Height < 3) { fields.Selected = wanted; return; }
        if (fields.Image.Width < 0 || fields.Image.Height < 0 || (long)fields.Image.Width + fields.Image.Height > int.MaxValue) throw new ArgumentException("Burning sources 이미지 크기 오류");
        if (fields.Total > fields.Capacity || fields.Particles.Length != fields.Capacity) throw new ArgumentException("Burning sources 현재입자 배열 오류");
        int rowCount = (fields.Image.Width + fields.Image.Height) / RowWidth;
        if (fields.Rows.Length < rowCount) throw new ArgumentOutOfRangeException(nameof(fields), "Burning sources 행 표식 미연결");
        if (fields.Selected < wanted)
        {
            int xMaximum = Math.Min(fields.Image.Width - 1, Math.Max(8, fields.Image.Width - 7));
            wanted = Math.Min(wanted, (xMaximum + fields.Image.Height - 3) / RowWidth); int attempts = 300;
            // 300번째 시도 뒤 목표에 닿아도 남은 횟수0이면 폴백을 실행한다.
            while (fields.Selected < wanted)
            {
                int before = attempts--; if (before == 0) break;
                int x = Random(0, xMaximum), y = Random(2, fields.Image.Height - 1), row = (x + y) / RowWidth;
                if (patch && !Opaque(fields, x, y + 1)) continue;
                if (row < 0 || row >= fields.Rows.Length) throw new ArgumentOutOfRangeException(nameof(fields), "Burning sources 새행 인덱스 오류");
                if (fields.Rows[row] != 0 || (!patch && !Opaque(fields, x, y + 1))) continue;
                int count = Random(6, 10), accepted = 0;
                // Append실패와 관계없이 원문이 수락한 점은 현재행을 표식한다.
                for (int index = 0; index < count; index++)
                {
                    int px = x + index / 2, py = y - index / 2;
                    if (px < fields.Image.Width - 1 && py >= 0 && Opaque(fields, px, py))
                    { _visual.Append(fields, new(px, py, 0, 0, 9999, true)); accepted++; fields.Rows[row] = 1; }
                }
                if (accepted > 3) fields.Selected++;
            }
            if (attempts < 1)
            {
                int left = 7;
                // 마지막행을 제외한 y/x순서의 최초7불투명점을 사용한다.
                for (int y = 0; y < fields.Image.Height - 1 && left != 0; y++)
                    // Append실패도 점 하나를 사용한 것으로 센다.
                    for (int x = 0; x < fields.Image.Width && left != 0; x++)
                        if (Opaque(fields, x, y)) { _visual.Append(fields, new(x, y, 0, 0, 9999, true)); left--; }
            }
            return;
        }
        uint first = fields.Total;
        // 선택 감소는 실제life와 무관하게 최초source표식만 찾는다.
        for (uint index = 0; index < fields.Total; index++) if (fields.Particles[(int)index].Source) { first = index; break; }
        if (first == fields.Total) return;
        int oldRow = Row(fields.Particles[(int)first]);
        if (oldRow < 0 || oldRow >= fields.Rows.Length) throw new ArgumentOutOfRangeException(nameof(fields), "Burning sources 제거행 인덱스 오류");
        if (fields.Rows[oldRow] != 1) throw new InvalidOperationException("Burning sources rowTaken 원본 assert");
        fields.Rows[oldRow] = 0;
        // 같은 행의source/life만0으로 두며 total/selected는 보존한다.
        for (uint index = 0; index < fields.Total; index++)
        {
            ref BurningParticle particle = ref fields.Particles[(int)index];
            if (particle.Source && Row(particle) == oldRow) { particle.Source = false; particle.Life = 0; }
        }
    }
}
