using System.Buffers.Binary;
using Netstorm.Core.Original;

// 42b210 Core 기술 합계의 같은 32비트 순회/level+1 합계/마지막 Core+5 BYTE 합산이다.
// cpppj/src/o/RawPlayerNotification.cpp 의 CoreTechnologyWeight 를 이식한다.
// 0.955에서 R4-6 메타 참가자(0x88 이름표 등급) 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 42b210 기술 합계의 실제 개수·TYPE 조회 장치다. 메뉴·세션을 요구하지 않는다.
public static class ClientMetaCoreWeight
{
    /// <summary>같은 현재 TYPE 개수와 비트·등급 조회로 Core 10BYTE 합계를 구한다. 0.955에서 추가됐다.</summary>
    /// <param name="core">같은 실제 Core 10BYTE다.</param>
    /// <param name="count">현재 TYPE 개수를 읽는다.</param>
    /// <param name="type">번호의 실제 TYPE 비트·등급을 읽는다.</param>
    public static byte Weight(ReadOnlySpan<byte> core, Func<int> count, Func<int, (int Bit, int Level)> type)
    {
        // Core 마스크와 마지막 +5 BYTE는 TYPE 조회 뒤에도 현재 물리에서 다시 읽는다.
        if (core.Length < 10)
        {
            throw new ArgumentOutOfRangeException(nameof(core), "Core 기술 합계의 실제 10BYTE 누락");
        }

        uint total = 0;
        // 원래 32개 기술 비트를 낮은 번호부터 순회한다.
        for (uint bit = 0; bit < 32; bit++)
        {
            if ((BinaryPrimitives.ReadUInt32LittleEndian(core.Slice(6, 4)) & (1u << unchecked((int)bit))) == 0)
            {
                continue;
            }

            // 불일치 뒤 현재 개수를 다시 조회하며 첫 일치에서 끝난다.
            int kinds = (count ?? throw new InvalidOperationException("Core 기술 합계의 실제 개수 미연결"))();
            for (int index = 0; index < kinds; index++)
            {
                (int actualBit, int level) = (type ?? throw new InvalidOperationException("Core 기술 합계의 실제 TYPE 미연결"))(index);
                if (actualBit == unchecked((int)bit))
                {
                    total += 1u + unchecked((uint)level);
                    break;
                }
            }
        }

        return unchecked((byte)(total + core[5]));
    }
}
