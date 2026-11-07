// 배치·붕괴의 표면 연결 판정을 공유한다.
namespace Netstorm.Core.Original;

// 다리 연결 판정 모듈을 담는 네임스페이스다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.

// 원본 00441e40 ↔ CD 004d2e00의 짝수 방향 side·홀수 방향 variant 연결을 양쪽에서 검사한다. 배치 순회와 붕괴 방문이 같은 판정을 쓴다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.
public static class RawBridgeConnects
{
    /// <summary>북부터 시계 방향인 8방향을 연결 비트로 바꾸는 표다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private static readonly int[] DirectionBits = [1, 2, 2, 4, 4, 8, 8, 1];

    /// <summary>다리·폭탄 마스크다(원문 kBridgeOrBombMask = 0x104). 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private const uint BridgeOrBombMask = 0x104u;

    /// <summary>섬·건물 마스크다(원문 kLandObjectMask = 0x1044202). 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private const uint LandObjectMask = 0x1044202u;

    /// <summary>두 프레임 코드가 주어진 방향으로 연결되는지 판정한다.</summary>
    /// <param name="first">기준 칸의 프레임 코드다.</param>
    /// <param name="firstFlags2">기준 칸 타입의 플래그2다.</param>
    /// <param name="second">이웃 칸의 프레임 코드다.</param>
    /// <param name="secondFlags2">이웃 칸 타입의 플래그2다.</param>
    /// <param name="direction">기준에서 이웃을 보는 방향(0~7)이다.</param>
    /// <returns>연결되면 참이다.</returns>
    public static bool Connects(RiftFrameCode first, uint firstFlags2, RiftFrameCode second, uint secondFlags2, int direction)
    {
        if (direction < 0 || direction >= 8)
        {
            throw new ArgumentOutOfRangeException(nameof(direction), direction, "표면 연결 방향 오류");
        }

        int a = direction % 2 == 0 ? first.Side : first.Variant;
        int b = direction % 2 == 0 ? second.Side : second.Variant;
        if ((firstFlags2 & TypeFlag2.Emplacement) != 0)
        {
            a = 'P';
        }

        if ((secondFlags2 & TypeFlag2.Emplacement) != 0)
        {
            b = 'P';
        }

        if ((firstFlags2 & BridgeOrBombMask) != 0 && (secondFlags2 & LandObjectMask) != 0)
        {
            b = 'A';
        }
        else if ((secondFlags2 & BridgeOrBombMask) != 0 && (firstFlags2 & LandObjectMask) != 0)
        {
            a = 'A';
        }

        if (a < 'A' || a > 'P')
        {
            throw new ArgumentOutOfRangeException(nameof(first), a, "표면 연결 코드 오류");
        }

        // 원문 첫 마스크가 0이면 두 번째 방향 표를 조회하지 않는다.
        if ((TerrainBuilder.Connection((char)a) & DirectionBits[direction]) == 0)
        {
            return false;
        }

        // 두 번째 코드가 실제로 필요할 때는 기존 미지원 입력 진단을 유지한다.
        if (b < 'A' || b > 'P')
        {
            throw new ArgumentOutOfRangeException(nameof(second), b, "표면 연결 코드 오류");
        }

        return (TerrainBuilder.Connection((char)b) & DirectionBits[(direction + 4) % 8]) != 0;
    }
}
