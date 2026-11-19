// 원문 00413c60/004143b0의 직접 AI 본체 기록기를 실제 Player 조회와 소유 메모리 해석에만 연결한다.
namespace Netstorm.Core.Original;

/// <summary>실제 Player 9기록과 생존 0xb08 본체의 현재 조회를 빌린다.</summary>
/// <remarks>값형 지갑/AI 사본이나 새 생성 성공을 제공하지 않는다. 2026-11-19 0.637에서 R4-5 AI 초기화용으로 추가했다. 근거: cpppj src/o/RawAiCommandState.h 8행의 AiCommandStateHooks다.</remarks>
public sealed class AiCommandStateHooks
{
    /// <summary>각 원문 Player base 재조회에서 같은 현재 1620바이트를 읽는다.</summary>
    /// <remarks>2026-11-19 0.637에서 R4-5 AI 초기화용으로 추가했다. 앞선 본체 해석이 바꾼 포인터를 다음 방문이 현재값으로 읽으므로 호출마다 조회한다.</remarks>
    public Func<byte[]>? Players { get; init; }

    /// <summary>비영 불투명 32비트 토큰을 같은 실제 생존 본체로 해석한다.</summary>
    /// <remarks>2026-11-19 0.637에서 R4-5 AI 초기화용으로 추가했다. 크기가 0xb08이 아니면 원본 진단과 같이 거부한다.</remarks>
    public Func<uint, byte[]>? Body { get; init; }
}

/// <summary>원문 직접 기록기의 현재 Player/본체와 보존 바이트를 같은 순서로 실행한다.</summary>
/// <remarks>2026-11-19 0.637에서 R4-5 AI 초기화용으로 추가했다. 근거: cpppj src/o/RawAiCommandState.h/.cpp의 CrankAiMoney·SetAiParameter다. 생성자·초기 갱신·258 완료 판정(R4-5 잔여)과 달리 게임 난수를 소비하지 않는다.</remarks>
public static class RawAiCommandState
{
    /// <summary>원문 Player 한 기록의 바이트 수다.</summary>
    /// <remarks>2026-11-19 0.637에서 추가했다. 원문 kPlayerBytes와 같다.</remarks>
    public const int PlayerBytes = 180;

    /// <summary>원문 Player 기록 개수다(중립 0번 포함).</summary>
    /// <remarks>2026-11-19 0.637에서 추가했다. 원문 kPlayerSlots와 같다.</remarks>
    public const int PlayerSlots = 9;

    /// <summary>Player 기록 안에서 brain 토큰을 읽는 오프셋이다.</summary>
    /// <remarks>2026-11-19 0.637에서 추가했다. 원문 kBrainOffset(0xb0)과 같다.</remarks>
    public const int BrainOffset = 0xb0;

    /// <summary>실제 AI 본체 한 개의 바이트 수다.</summary>
    /// <remarks>2026-11-19 0.637에서 추가했다. 원문 kBodyBytes(0xb08)와 같다.</remarks>
    public const int BodyBytes = 0xb08;

    /// <summary>직접 지갑 기록이 쓰는 본체 오프셋이다.</summary>
    /// <remarks>2026-11-19 0.637에서 추가했다. 원문 CrankAiMoney의 0x2d8과 같다.</remarks>
    public const int MoneyOffset = 0x2d8;

    /// <summary>직접 지갑이 쓰는 고정 금액이다.</summary>
    /// <remarks>2026-11-19 0.637에서 추가했다. 원문 CrankAiMoney의 30000과 같다.</remarks>
    public const uint MoneyAmount = 30000;

    /// <summary>같은 현재 Player base를 각 방문 위치에서 새로 읽어 해당 소유자의 brain 토큰을 돌려준다.</summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <param name="hooks">현재 Player 조회다.</param>
    /// <returns>해당 기록 +0xb0의 네 바이트다.</returns>
    /// <remarks>2026-11-19 0.637에서 추가했다. 원문을 감싼 널 주소는 호스트의 유효 span으로 합성하지 않는다.</remarks>
    private static uint Brain(uint owner, AiCommandStateHooks hooks)
    {
        byte[] players = (hooks.Players ?? throw new InvalidOperationException("AI 직접 명령의 실제 현재 저장소 미연결"))();
        if (players.Length != PlayerBytes * PlayerSlots)
        {
            throw new ArgumentException("AI 직접 명령의 현재 Player 9기록 누락", nameof(hooks));
        }

        int at = (int)owner * PlayerBytes + BrainOffset;
        return unchecked((uint)(players[at] | ((uint)players[at + 1] << 8) | ((uint)players[at + 2] << 16) | ((uint)players[at + 3] << 24)));
    }

    /// <summary>같은 생존 본체만 반환하며 해제 포인터·널 재조회·잘못된 폭을 성공으로 바꾸지 않는다.</summary>
    /// <param name="token">해석할 비영 토큰이다.</param>
    /// <param name="hooks">현재 본체 해석이다.</param>
    /// <returns>같은 실제 본체의 유일한 소유 배열이다.</returns>
    /// <remarks>2026-11-19 0.637에서 추가했다. 한 DWORD 외의 본체 바이트는 호출자가 보존한다.</remarks>
    private static byte[] Body(uint token, AiCommandStateHooks hooks)
    {
        if (token == 0)
        {
            throw new InvalidOperationException("AI 직접 명령의 재조회된 널 본체");
        }

        byte[] raw = (hooks.Body ?? throw new InvalidOperationException("AI 직접 명령의 실제 현재 저장소 미연결"))(token);
        if (raw.Length != BodyBytes)
        {
            throw new ArgumentException("AI 직접 명령의 실제 0xb08 본체 누락", nameof(hooks));
        }

        return raw;
    }

    /// <summary>원문 리틀 엔디언 물리 네 바이트만 써서 한 DWORD 외의 본체 바이트를 보존한다.</summary>
    /// <param name="raw">대상 본체 배열이다.</param>
    /// <param name="at">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <remarks>2026-11-19 0.637에서 추가했다.</remarks>
    private static void Put(byte[] raw, int at, uint value)
    {
        // 낮은 바이트부터 폭 4바이트로 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            raw[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>원문은 중립 0번을 방문하지 않고 여덟 현재 포인터를 각각 읽어 고정 30000을 쓴다.</summary>
    /// <param name="hooks">현재 Player 조회와 본체 해석이다.</param>
    /// <remarks>2026-11-19 0.637에서 R4-5 AI 초기화용으로 추가했다. 앞선 실제 경계가 후속 Player를 바꾸더라도 다음 방문은 현재값이다. mode/demo/SP는 읽지 않는다.</remarks>
    public static void CrankAiMoney(AiCommandStateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        // 중립 0번을 건너뛰고 1~8번을 번호 순서대로 방문하는 반복문이다.
        for (uint owner = 1; owner < PlayerSlots; owner++)
        {
            uint token = Brain(owner, hooks);
            if (token != 0)
            {
                Put(Body(token, hooks), MoneyOffset, MoneyAmount);
            }
        }
    }

    /// <summary>low3 소유자·8비트 선택기·부호 있는 상위 16비트의 다섯 기록기다. 선택기 0은 두 현재 포인터를 재조회한다.</summary>
    /// <param name="packed">소유자·선택기·값을 packed한 원문 인자다.</param>
    /// <param name="hooks">현재 Player 조회와 본체 해석이다.</param>
    /// <remarks>2026-11-19 0.637에서 R4-5 AI 초기화용으로 추가했다. 소유자 0·없는 brain·미지원 선택기는 선택하지 않은 본체 경계를 요구하지 않는다.</remarks>
    public static void SetAiParameter(uint packed, AiCommandStateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint owner = packed & 7u;
        if (owner == 0)
        {
            return;
        }

        uint token = Brain(owner, hooks);
        if (token == 0)
        {
            return;
        }

        uint selector = (packed >> 4) & 255u;
        if (selector > 4)
        {
            return;
        }

        int value = unchecked((int)packed) >> 16;
        if (selector == 0)
        {
            uint bits = BitConverter.SingleToUInt32Bits((float)value);
            Put(Body(Brain(owner, hooks), hooks), 0x220, bits);
            Put(Body(Brain(owner, hooks), hooks), 0x600, bits);
            return;
        }

        uint word = selector == 1 ? BitConverter.SingleToUInt32Bits((float)value) : unchecked((uint)value);
        int[] offsets = [0, 0x228, 0x230, 0x234, 0x24c];
        Put(Body(token, hooks), offsets[selector], word);
    }
}
