// 원본 Colorsort의 세 차례 CRT 정렬과 Particle의 요청 길이 병합을 유지한다.
namespace Netstorm.Core.Original;

/// <summary>원본80바이트 항목에서 색 선택이 사용하는 의미 필드다.</summary>
public struct ParticleGradientEntry
{
    /// <summary>시작색을 뺀 RGB double 세 채널이다.</summary>
    public double[] Translated { get; set; }
    /// <summary>원본 팔레트 번호이며 마지막 선택 항목은 끝색 번호로 교체된다.</summary>
    public uint Index { get; set; }
    /// <summary>색 축으로부터의 제곱 거리다.</summary>
    public double Distance { get; set; }
    /// <summary>색 축 위의 투영 비율이다.</summary>
    public double Position { get; set; }
    /// <summary>중복 선택도1로 유지하는 원문 표식이다.</summary>
    public uint Selected { get; set; }
}

/// <summary>한 색 띠의 전체 정렬 결과와 요청/선택 개수다.</summary>
public sealed class ParticleGradient
{
    /// <summary>CRT의 교환 순서를 마친256항목이다.</summary>
    public ParticleGradientEntry[] Entries { get; } = new ParticleGradientEntry[256];
    /// <summary>가장 가까운 시작 팔레트 번호다.</summary>
    public int From { get; internal set; } = -1;
    /// <summary>가장 가까운 끝 팔레트 번호다.</summary>
    public int To { get; internal set; } = -1;
    /// <summary>병합이 실제 선택 수 대신 사용하는 요청 색 수다.</summary>
    public int Count { get; internal set; }
    /// <summary>원통 후보에서 서로 다르게 선택한 항목 수다.</summary>
    public int Selected { get; internal set; }
    /// <summary>원문00555e00의 색 원통 제곱 거리다.</summary>
    public double Threshold { get; internal set; }
}

/// <summary>PATCH00439de0/0043ad20·00487d20의 기본53비트 색 띠/병합이다.</summary>
public static class ParticleGradients
{
    /// <summary>PE가 사용하는 RGB 정규화 binary64 상수다.</summary>
    private static readonly double RgbUnit = BitConverter.UInt64BitsToDouble(0x3f70101010101010);
    /// <summary>최근접 검색의 초기 거리다.</summary>
    private const double NearestLimit = 100000;
    /// <summary>축 위치 선택의 초기 오차다.</summary>
    private const double SelectionLimit = 10000;
    /// <summary>영 거리 임계값의 원문 대체값이다.</summary>
    private const double FallbackThreshold = .05;
    /// <summary>마지막 선택 위치를 포함시키는 간격 비율이다.</summary>
    private const double SelectionExtra = .25;
    /// <summary>CRT가 선택 정렬로 바꾸는 구간 크기다.</summary>
    private const int Small = 8;
    /// <summary>원본 CRT의 보관 구간 스택 크기다.</summary>
    private const int Ranges = 30;
    /// <summary>양 끝을 포함하는 원문 정렬 구간이다.</summary>
    /// <param name="Low">현재 첫 항목이다.</param>
    /// <param name="High">현재 마지막 항목이다.</param>
    private readonly record struct SortRange(int Low, int High);
    /// <summary>세 qsort의 서로 다른 비교 규칙이다.</summary>
    private enum SortKey
    {
        /// <summary>색 축 거리만 비교한다.</summary>
        Distance,
        /// <summary>원통 안팎과 축 위치를 비교한다.</summary>
        Cylinder,
        /// <summary>선택 표식과 축 위치를 비교한다.</summary>
        Selected,
    }
    /// <summary>NaN과 동률은0으로 복귀하는 원문 부호 비교다.</summary>
    private static int Sign(double value) => value < 0 ? -1 : value > 0 ? 1 : 0;
    /// <summary>원통 밖과 비선택 항목의 동률까지 보존한다.</summary>
    private static int Compare(ParticleGradientEntry a, ParticleGradientEntry b, SortKey key, double threshold)
    {
        if (key == SortKey.Distance) return Sign(a.Distance - b.Distance);
        if (key == SortKey.Cylinder)
        {
            if (a.Distance >= threshold) return b.Distance < threshold ? 1 : 0;
            if (b.Distance >= threshold) return -1;
            return Sign(a.Position - b.Position);
        }
        if (a.Selected == 0) return b.Selected != 0 ? 1 : 0;
        if (b.Selected == 0) return -1;
        return Sign(a.Position - b.Position);
    }
    /// <summary>PATCH004e6010/004e5fa0의 피벗/동률/교환 순서다. Array.Sort로 대체하지 않는다.</summary>
    private static void Sort(ParticleGradientEntry[] entries, SortKey key, double threshold)
    {
        var stack = new SortRange[Ranges]; int used = 0, low = 0, high = entries.Length - 1;
        // 원문 포인터 대신 현재 항목 인덱스를 비교한다.
        int Cmp(int a, int b) => Compare(entries[a], entries[b], key, threshold);
        // 의미 필드 전체와 RGB 배열 소유권을 함께 교환한다.
        void Swap(int a, int b) { if (a != b) (entries[a], entries[b]) = (entries[b], entries[a]); }
        // 큰 구간을 고정 스택에 보관한다.
        void Push(int first, int last)
        {
            if (used == stack.Length) throw new InvalidOperationException("입자 색 정렬 스택 넘침");
            stack[used++] = new(first, last);
        }
        // 현재 구간과 보관 구간을 원문 분기 순서대로 처리한다.
        for (;;)
        {
            if (high - low + 1 <= Small)
            {
                // 작은 구간은 엄격히 큰 최대 항목만 끝으로 보내는 선택 정렬이다.
                while (low < high)
                {
                    int maximum = low;
                    // 동률 항목으로 최대 위치를 바꾸지 않는다.
                    for (int current = low + 1; current <= high; current++) if (Cmp(current, maximum) > 0) maximum = current;
                    Swap(maximum, high); high--;
                }
            }
            else
            {
                int middle = low + (high - low + 1) / 2;
                if (Cmp(low, middle) > 0) Swap(low, middle);
                if (Cmp(low, high) > 0) Swap(low, high);
                if (Cmp(middle, high) > 0) Swap(middle, high);
                int left = low, right = high;
                // 자기 피벗 비교를 건너뛰며 두 탐색 위치가 교차할 때까지 나눈다.
                for (;;)
                {
                    if (left < middle)
                    {
                        // 피벗 앞 작은 항목과 동률 항목을 지난다.
                        do { left++; } while (left < middle && Cmp(left, middle) <= 0);
                    }
                    if (left >= middle)
                    {
                        // 피벗 뒤의 작은/동률 항목도 끝까지 검사한다.
                        do { left++; } while (left <= high && Cmp(left, middle) <= 0);
                    }
                    // 오른쪽은 엄격히 큰 항목만 지난다.
                    do { right--; } while (right > middle && Cmp(right, middle) > 0);
                    if (left > right) break;
                    Swap(left, right); if (middle == right) middle = left;
                }
                right++;
                if (right > middle)
                {
                    // 피벗 오른쪽의0 비교 항목을 제거한다.
                    do { right--; } while (right > middle && Cmp(right, middle) == 0);
                }
                if (right <= middle)
                {
                    // 피벗을 건너뛰고 왼쪽 동률 구간을 제거한다.
                    do { right--; } while (right > low && Cmp(right, middle) == 0);
                }
                if (right - low < high - left)
                {
                    if (left < high) Push(left, high); high = right;
                    if (low < high) continue;
                }
                else
                {
                    if (low < right) Push(low, right); low = left;
                    if (low < high) continue;
                }
            }
            if (used == 0) return;
            SortRange range = stack[--used]; low = range.Low; high = range.High;
        }
    }
    /// <summary>원본 최근접/축 거리의 B/G/R 제곱 합 순서다.</summary>
    private static double Distance(double[] a, double[] b)
    {
        double red = a[0] - b[0], green = a[1] - b[1], blue = a[2] - b[2];
        double first = blue * blue + green * green; return first + red * red;
    }
    /// <summary>RGBX256색에 원문 정규화·최근접·투영·세 정렬을 적용한다. 대상 RGB를 clamp하지 않는다.</summary>
    public static ParticleGradient Build(ReadOnlySpan<uint> palette, double[] from, double[] to, int colors, double width = 2.4)
    {
        ArgumentNullException.ThrowIfNull(from); ArgumentNullException.ThrowIfNull(to);
        if (palette.Length != 256 || from.Length != 3 || to.Length != 3 || colors < 2 || colors > 256 || !double.IsFinite(width) || width <= 0)
            throw new ArgumentException("입자 색 띠 팔레트/개수/폭 오류");
        // 세 대상 채널은 임의 색 범위로 제한하지 않되 비유한 값은 거부한다.
        for (int channel = 0; channel < 3; channel++)
            if (!double.IsFinite(from[channel]) || !double.IsFinite(to[channel])) throw new ArgumentException("입자 대상 RGB 비유한 값");
        double wanted = colors * width;
        if (!double.IsFinite(wanted) || wanted < 1 || wanted >= 255.5) throw new ArgumentOutOfRangeException(nameof(width), "입자 색 원통 후보 수 범위 오류");
        int rounded = (int)((double)(float)wanted + .5), cylinder = (int)wanted;
        if (rounded >= 256) throw new ArgumentOutOfRangeException(nameof(width), "입자 색 임계 항목 범위 오류");
        var result = new ParticleGradient { Count = colors }; double bestFrom = NearestLimit, bestTo = NearestLimit;
        // 원래 팔레트 번호와 RGB double을 만들며 동일 거리이면 처음 색을 유지한다.
        for (int index = 0; index < result.Entries.Length; index++)
        {
            ref ParticleGradientEntry entry = ref result.Entries[index]; entry.Index = (uint)index; entry.Translated = new double[3];
            // 예약BYTE는 RGB 계산에 참여하지 않는다.
            for (int channel = 0; channel < 3; channel++) entry.Translated[channel] = ((palette[index] >> (8 * channel)) & 255) * RgbUnit;
            double nearFrom = Distance(entry.Translated, from), nearTo = Distance(entry.Translated, to);
            if (nearFrom < bestFrom) { bestFrom = nearFrom; result.From = index; }
            if (nearTo < bestTo) { bestTo = nearTo; result.To = index; }
        }
        if (result.From < 0 || result.To < 0) throw new ArgumentOutOfRangeException(nameof(from), "입자 시작/끝 최근접 색 없음");
        double[] origin = result.Entries[result.From].Translated.ToArray();
        // 원문 값형 origin을 보존한 채 모든 RGB 차이를 저장한다.
        foreach (ParticleGradientEntry entry in result.Entries)
        {
            // 다음 축 조회 전에 세 채널을 모두 원래 순서로 뺀다.
            for (int channel = 0; channel < 3; channel++) entry.Translated[channel] -= origin[channel];
        }
        double[] axis = result.Entries[result.To].Translated;
        double firstNorm = axis[2] * axis[2] + axis[1] * axis[1], norm = firstNorm + axis[0] * axis[0];
        if (!(norm > 0)) throw new InvalidOperationException("입자 색 띠 영길이 축 미복원");
        // R/B/G 내적과 B/G/R 거리의 연산/저장 순서를 유지한다.
        for (int index = 0; index < result.Entries.Length; index++)
        {
            ref ParticleGradientEntry entry = ref result.Entries[index]; double[] color = entry.Translated;
            double firstDot = axis[0] * color[0] + axis[2] * color[2], dot = firstDot + axis[1] * color[1];
            entry.Position = dot / norm;
            double[] projection = [entry.Position * axis[0], entry.Position * axis[1], entry.Position * axis[2]];
            entry.Distance = Distance(color, projection);
        }
        Sort(result.Entries, SortKey.Distance, 0); result.Threshold = result.Entries[rounded].Distance;
        if (result.Threshold == 0) result.Threshold = FallbackThreshold;
        Sort(result.Entries, SortKey.Cylinder, result.Threshold);
        double step = 1.0 / (colors - 1), stop = step * SelectionExtra + 1;
        // 같은 색이 여러 위치에 가까워도 선택 표식을 중복 가산하지 않는다.
        for (double position = 0; position < stop; position += step)
        {
            double best = SelectionLimit; int chosen = -1;
            // 동일 오차이면 앞 항목을 유지하는 엄격한 비교다.
            for (int index = 0; index < cylinder; index++)
            {
                double difference = Math.Abs(position - result.Entries[index].Position);
                if (difference < best) { best = difference; chosen = index; }
            }
            if (chosen < 0) throw new InvalidOperationException("입자 색 띠 후보 선택 실패");
            result.Entries[chosen].Selected = 1;
        }
        // 원통 후보 앞부분의 선택 수만 센다.
        for (int index = 0; index < cylinder; index++) if (result.Entries[index].Selected != 0) result.Selected++;
        Sort(result.Entries, SortKey.Selected, result.Threshold); result.Entries[result.Selected - 1].Index = (uint)result.To;
        return result;
    }
    /// <summary>1~3개의 연속 색 띠에서 선택 수와 무관하게 요청 색 수만큼 앞 번호를 합친다.</summary>
    public static uint[] Merge(params ParticleGradient[] gradients)
    {
        ArgumentNullException.ThrowIfNull(gradients);
        if (gradients.Length == 0 || gradients.Length > 3) throw new ArgumentException("입자 색 띠 병합 개수 오류");
        var result = new List<uint>();
        // null 구멍이 없는 연속 색 띠의 순서를 보존한다.
        foreach (ParticleGradient gradient in gradients)
        {
            if (gradient is null || gradient.Count < 0 || gradient.Count > 256) throw new ArgumentException("입자 색 띠 병합 길이 오류");
            // 실제 선택보다 요청이 클 때도 뒤 정렬 항목을 그대로 읽는다.
            for (int index = 0; index < gradient.Count; index++) result.Add(gradient.Entries[index].Index);
        }
        return result.ToArray();
    }
    /// <summary>실제 PE binary64 상수로 어두운빨강→주황8색과 주황→밝은색3색을 만든다.</summary>
    public static uint[] FieryTail(ReadOnlySpan<uint> palette)
    {
        double[] start = [1, BitConverter.UInt64BitsToDouble(0x3fe8787878787878), BitConverter.UInt64BitsToDouble(0x3f98181818181818)];
        double[] dark = [BitConverter.UInt64BitsToDouble(0x3fe7373737373737), 0, 0], light = [1, 1, BitConverter.UInt64BitsToDouble(0x3fe5b5b5b5b5b5b6)];
        return Merge(Build(palette, dark, start, 8), Build(palette, start, light, 3));
    }
    /// <summary>파랑→흰색8개와 흰색→PE128/255 회색3개를 폭2.4로 합친다.</summary>
    public static uint[] Lightning(ReadOnlySpan<uint> palette)
    {
        double[] blue = [0, 0, 1], white = [1, 1, 1], gray = [128.0 / 255, 128.0 / 255, 128.0 / 255];
        return Merge(Build(palette, blue, white, 8), Build(palette, white, gray, 3));
    }
}
