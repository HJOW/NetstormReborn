// 같은 CS 기록과 Client 수명의 최초 배치 좌표를 구별한다.
namespace Netstorm.Core.Original;

/// <summary>RawChallengeState.h의 장기 BSS 배치표다. CS 기록 Reset은 이 값을 다시 쓰지 않는다.</summary>
public sealed class ChallengeGeometry
{
    /// <summary>005467c8의 최초0이며 모든 좌표 저장 후1을 쓴다.</summary>
    public uint Initialized { get; set; }
    /// <summary>005467c0의12+반경을 float로 저장한 값이다.</summary>
    public float FirstRadius { get; set; }
    /// <summary>005467c4의4+반경을 float로 저장한 별도 값이다.</summary>
    public float SecondRadius { get; set; }
    /// <summary>005467cc의 저장된 중심으로 계산한 정수 폭이다.</summary>
    public int Width { get; set; }
    /// <summary>005467d0의 저장된 중심으로 계산한 정수 높이다.</summary>
    public int Height { get; set; }
    /// <summary>005485b8의 중심+13을 저장한 float다.</summary>
    public float OriginX { get; set; }
    /// <summary>005485bc의 저장된 중심 float다.</summary>
    public float OriginY { get; set; }
    /// <summary>005468b0의 실제9쌍 좌표이며 각 성분은 float 저장 위치다.</summary>
    public float[,] Ring { get; } = new float[9, 2];
    /// <summary>005485c0의 실제40쌍 좌표다. 최초 계산은0번을 보존한다.</summary>
    public float[,] Grid { get; } = new float[40, 2];
}

/// <summary>cpppj/src/o/RawChallengeState.cpp의21328BYTE 초기화와 최초 배치 계산이다.</summary>
public static class RawChallengeState
{
    /// <summary>0042cec0이 요청하는 CS 전체 크기다.</summary>
    public const int Bytes = 0x5350;
    /// <summary>두DWORD 뒤 전투 배열의 시작이다.</summary>
    public const int BattleOffset = 8;
    /// <summary>전투 한 기록의 실제292BYTE다.</summary>
    public const int BattleBytes = 0x124;
    /// <summary>전투 배열의 실제10칸이다.</summary>
    public const int BattleCount = 10;
    /// <summary>전투 배열 뒤 참가자 배열의 시작이다.</summary>
    public const int ParticipantOffset = 0xb70;
    /// <summary>참가자 한 기록의 실제460BYTE다.</summary>
    public const int ParticipantBytes = 0x1cc;
    /// <summary>참가자 배열의 실제40칸이다.</summary>
    public const int ParticipantCount = 40;
    /// <summary>00504820의 실제 double 비트 상수다.</summary>
    private static readonly double Angle = BitConverter.UInt64BitsToDouble(0x3fe921f9f01b866e);
    /// <summary>00504810의 실제 float 각도 증가량이다.</summary>
    private static readonly float Step = BitConverter.UInt32BitsToSingle(0x3f32b8b8);

    /// <summary>정확21328BYTE에서 두DWORD→참가자40→전투10 순서로 초기화한다.</summary>
    public static void Reset(Span<byte> bytes)
    {
        if (bytes.Length != Bytes) throw new ArgumentException("CS 초기화의 실제21328BYTE 필요", nameof(bytes));
        bytes[..8].Clear();
        bytes.Slice(ParticipantOffset, ParticipantCount * ParticipantBytes).Clear();
        bytes.Slice(BattleOffset, BattleCount * BattleBytes).Clear();
    }

    /// <summary>기록을 먼저 초기화한 뒤 미초기화 좌표만 계산한다. 저장 float 비트는 Windows CPP probe로 대조한다.</summary>
    public static void Construct(Span<byte> bytes, ChallengeGeometry geometry)
    {
        ArgumentNullException.ThrowIfNull(geometry);
        Reset(bytes);
        if (geometry.Initialized != 0) return;
        int side = (int)Math.Floor(Math.Sqrt(20.0));
        float square = (float)(side * 8.0);
        double radius = side * 8.0 / (2.0 * Math.Cos(Angle));
        float center = (float)(44.0 + radius);
        int index = 1;
        // 같은4×4 순회를 반복해39번까지 채우며0번은 덮지 않는다.
        while (index < 40)
        {
            // 다음 반복에서도 현재 행을0부터 다시 시작한다.
            for (int row = 0; row < side; row++)
            {
                if (index >= 40) break;
                // 각 현재 번호의 홀짝을 저장 전에 읽는다.
                for (int column = 0; column < side && index < 40; column++)
                {
                    double half = square * 0.5;
                    geometry.Grid[index, 0] = (float)(column * 8.0 + center - half + 13.0);
                    geometry.Grid[index, 1] = (float)(row * 8.0 + center - half - ((index & 1) != 0 ? 0 : 2));
                    index++;
                }
            }
        }
        double angle = 0, ringRadius = 30.0 + radius;
        // 아홉 쌍을 float로 저장한 뒤 각도를 누적하고 마지막 각도는 버린다.
        for (int point = 0; point < 9; point++)
        {
            geometry.Ring[point, 0] = (float)(Math.Cos(angle) * ringRadius + center + 13.0);
            geometry.Ring[point, 1] = (float)(Math.Sin(angle) * ringRadius + center);
            angle += Step;
        }
        geometry.OriginY = center;
        geometry.FirstRadius = (float)(12.0 + radius);
        geometry.SecondRadius = (float)(radius + 4.0);
        geometry.OriginX = (float)(center + 13.0);
        geometry.Width = (int)(center * 2.0 + 26.0);
        geometry.Height = (int)(center * 2.0);
        geometry.Initialized = 1;
    }
}
