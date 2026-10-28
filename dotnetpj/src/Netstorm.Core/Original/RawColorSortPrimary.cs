namespace Netstorm.Core.Original;

/// <summary>
/// 실제 439de0의 raw 56바이트 × 256 기록을 소유하고 최초 명도 double을 재계산 없이 빌린다.
/// 근거: cpppj/src/o/RawColorSortPrimary.h/.cpp. (R2c, 2026-10-28)
/// </summary>
public sealed class RawColorSortPrimary
{
    /// <summary>ColorSort 항목 수.</summary>
    public const int ColorCount = 256;

    /// <summary>한 기록의 바이트 수.</summary>
    public const int RecordBytes = 56;

    /// <summary>전체 할당 바이트 수(0x3800).</summary>
    public const int TotalBytes = ColorCount * RecordBytes;

    /// <summary>정확 0x3800 payload이며 생성자가 명시 초기 바이트를 전부 복사한다.</summary>
    private readonly byte[] _bytes = new byte[TotalBytes];

    /// <summary>부모가 성공 반환한 기록만 읽게 하는 호스트 표식이며 원본 필드로 세지 않는다.</summary>
    private bool _constructed;

    /// <summary>
    /// 원본 malloc 직후의 실제 초기 0x3800바이트를 필수로 받는다. 미작성 padding을 임의 0으로 채우지 않는다.
    /// </summary>
    /// <param name="allocationBytes">초기 0x3800바이트.</param>
    public RawColorSortPrimary(byte[] allocationBytes)
    {
        ArgumentNullException.ThrowIfNull(allocationBytes);
        if (allocationBytes.Length != TotalBytes)
        {
            throw new ArgumentException("ColorSort primary 초기 0x3800바이트 계약", nameof(allocationBytes));
        }
        Array.Copy(allocationBytes, _bytes, TotalBytes);
    }

    /// <summary>실제 초기 할당·alias·부분 효과를 위한 같은 raw 저장소다. 기록 번호·미작성 padding을 포함해 원문 물리를 빌린다.</summary>
    public byte[] Bytes => _bytes;

    /// <summary>호스트 표식을 읽고 raw 미작성 padding을 평가하지 않는다.</summary>
    public bool Constructed => _constructed;

    /// <summary>
    /// 기본 53비트/nearest의 439de0만 지원한다. 첫 1024바이트 RGBX를 현재 읽으며 raw와의 실제 alias를 보존한다.
    /// 원문 두 전체 순회와 binary53 연산 순서를 raw 물리에 그대로 적용한다.
    /// </summary>
    /// <param name="logicalRgbx">논리 RGBX 1024바이트 이상.</param>
    /// <param name="checkpoint">선택적 관측 중단 경계. 진입 때 (0,0,0), 각 store 완료 뒤 (누적 수,offset,size)를 받는다.</param>
    public void Construct(byte[] logicalRgbx, Action<uint, uint, uint>? checkpoint = null)
    {
        ArgumentNullException.ThrowIfNull(logicalRgbx);
        if (logicalRgbx.Length < ColorCount * 4)
        {
            throw new ArgumentException("ColorSort primary 논리 RGBX 1024바이트 계약", nameof(logicalRgbx));
        }
        _constructed = false;
        uint writes = 0;
        checkpoint?.Invoke(0, 0, 0);
        // 원문 첫 전체 순회: R→G→번호→B의 실제 저장 순서다
        for (int index = 0; index < ColorCount; index++)
        {
            int record = index * RecordBytes;
            StoreDouble(record + 24, logicalRgbx[index * 4] * RgbUnit(), ref writes, checkpoint);
            StoreDouble(record + 32, logicalRgbx[index * 4 + 1] * RgbUnit(), ref writes, checkpoint);
            byte blue = logicalRgbx[index * 4 + 2];
            StoreIndex(record + 48, (uint)index, ref writes, checkpoint);
            StoreDouble(record + 40, blue * RgbUnit(), ref writes, checkpoint);
        }
        // 원문 두 번째 전체 순회는 저장된 같은 R/G/B에서 H/L/S를 만든다
        for (int index = 0; index < ColorCount; index++)
        {
            int record = index * RecordBytes;
            double red = ReadDouble(record + 24);
            double green = ReadDouble(record + 32);
            double blue = ReadDouble(record + 40);
            double maximum = Math.Max(red, Math.Max(green, blue));
            double minimum = Math.Min(red, Math.Min(green, blue));
            double sum = minimum + maximum;
            double lightness = sum * 0.5;
            double saturation = 0.0;
            double hue = -1.0;
            if (maximum != minimum)
            {
                double delta = maximum - minimum;
                double denominator = lightness <= 0.5 ? sum : (2.0 - maximum) - minimum;
                saturation = delta / denominator;
                if (red == maximum)
                {
                    hue = (green - blue) / delta;
                }
                else if (green == maximum)
                {
                    hue = (blue - red) / delta + 2.0;
                }
                else
                {
                    hue = (red - green) / delta + 4.0;
                }
                hue *= 60.0;
                if (hue < 0.0)
                {
                    hue += 360.0;
                }
            }
            StoreDouble(record, hue, ref writes, checkpoint);
            StoreDouble(record + 8, lightness, ref writes, checkpoint);
            StoreDouble(record + 16, saturation, ref writes, checkpoint);
        }
        _constructed = true;
    }

    /// <summary>
    /// 같은 저장 명도를 읽는다. 미완성 생성자를 성공처럼 읽지 않으며 RGB에서 명도를 재계산하지 않는다.
    /// </summary>
    /// <param name="index">색 번호.</param>
    public double Lightness(int index)
    {
        if (!_constructed)
        {
            throw new InvalidOperationException("ColorSort primary 생성 미완료");
        }
        if (index < 0 || index >= ColorCount)
        {
            throw new ArgumentOutOfRangeException(nameof(index), index, "ColorSort primary 명도 번호 범위 오류");
        }
        return ReadDouble(index * RecordBytes + 8);
    }

    /// <summary>원문 503608의 실제 binary64 정규화 값이다.</summary>
    private static double RgbUnit() => BitConverter.UInt64BitsToDouble(0x3f70101010101010ul);

    /// <summary>같은 raw offset의 8바이트만 읽는다. payload 전체의 구조체 배치는 가정하지 않는다.</summary>
    private double ReadDouble(int offset) => BitConverter.ToDouble(_bytes, offset);

    /// <summary>원문 8바이트 store 하나를 완료한 뒤 현재 관측 경계를 호출한다. 예외 때 앞선 바이트를 되돌리지 않는다.</summary>
    private void StoreDouble(int offset, double value, ref uint writes, Action<uint, uint, uint>? checkpoint)
    {
        BitConverter.GetBytes(value).CopyTo(_bytes, offset);
        writes++;
        checkpoint?.Invoke(writes, (uint)offset, 8);
    }

    /// <summary>+30의 원래 4바이트 번호 store이며 인접 +34 padding은 건드리지 않는다.</summary>
    private void StoreIndex(int offset, uint value, ref uint writes, Action<uint, uint, uint>? checkpoint)
    {
        BitConverter.GetBytes(value).CopyTo(_bytes, offset);
        writes++;
        checkpoint?.Invoke(writes, (uint)offset, 4);
    }
}
