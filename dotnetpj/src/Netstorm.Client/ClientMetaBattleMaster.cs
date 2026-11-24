// 0x7c·메타 연결이 공유하는 현재 전투의 첫 master 참가자 조회를 이식한다.
// cpppj/src/client/NativeMetaConnectedState.cpp 의 FindMetaBattleMaster(004b2c60)를 이식한다.
// 0.945에서 R4-6 전송·수신 묶음용으로 추가됐다.
using Netstorm.Core.Original;

namespace Netstorm.Client;

// 004b2c60 master 조회의 실제 CS 읽기 본체다. 메뉴·세션을 요구하지 않는다.
public static class ClientMetaBattleMaster
{
    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.945에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "master 조회의 실제 CS 읽기 범위");
        }

        uint value = 0;
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>자기 전투 기록의 8칸에서 flags bit4가 선 첫 참가자를 찾는다. 0.945에서 추가됐다.</summary>
    /// <param name="challenge">같은 실제 CS 전투10·참가자40 기록이다.</param>
    /// <param name="owner">현재 전투 소유자 번호다.</param>
    /// <returns>첫 master 참가자 번호이며 없으면 -1이다.</returns>
    public static int Find(ClientChallengeGlobals challenge, uint owner)
    {
        ArgumentNullException.ThrowIfNull(challenge);
        uint battle = Read(challenge.Participant(owner).Span, 0xa4);
        if (battle == 0)
        {
            return -1;
        }

        Memory<byte> current = challenge.Battle(battle);
        for (uint slot = 0; slot < 8; slot++)
        {
            // 참가자0을 건너뛰지 않고 현재 flags의 bit4만 확인하며 별도 active 조건을 더하지 않는다.
            uint candidate = Read(current.Span, 0x38 + unchecked((int)slot) * 4);
            if (candidate == 0)
            {
                continue;
            }

            if ((Read(challenge.Participant(candidate).Span, 0x64, 1) & 4u) != 0)
            {
                return unchecked((int)candidate);
            }
        }

        return -1;
    }
}
