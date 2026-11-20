using static Netstorm.Core.Original.RawMissionPacketFields;

// 일반 로그인과 메타 수신이 같은 원문67h 부모를 사용한다.
namespace Netstorm.Core.Original;

/// <summary>현재 옵션 주소·파생값과 선택 관문을 실제 소유자에서 빌린다.</summary>
public sealed record StateBattleOptionGlobals
{
    /// <summary>0052f7e0의 같은 실제 옵션87바이트이며 미복사 꼬리는 보존한다.</summary>
    public Memory<byte> Options { get; init; }
    /// <summary>005467d8의 현재 CS 존재 DWORD다. 복사 뒤 조회한다.</summary>
    public Func<uint>? Challenge { get; init; }
    /// <summary>005424bc의 실제 환불 비율 DWORD이며 성공한 현재값 조회 뒤에만 쓴다.</summary>
    public UIntBox Value { get; init; } = new();
    /// <summary>재접속 UI 선택 위치의 현재 전투 모드다.</summary>
    public Func<bool>? Battle { get; init; }
    /// <summary>전투가 참일 때 읽는 현재 미션 후처리 관문이다.</summary>
    public Func<bool>? Mission { get; init; }
    /// <summary>전투·비미션에서만 읽는 현재 저장 게임 관문이다.</summary>
    public Func<bool>? Saved { get; init; }
}

/// <summary>도달한 원문 위치에서만 요구하는 옵션 파생 효과 장치다.</summary>
public sealed record StateBattleOptionHooks
{
    /// <summary>길이87초과 진단이며 복귀 뒤 같은 패킷의 현재 WORD를 다시 읽는다.</summary>
    public Action<string>? Assertion { get; init; }
    /// <summary>CS 비영에서만 같은 sender·패킷을 전체0042c920으로 전달한다.</summary>
    public Action<uint, ReadOnlyMemory<byte>>? ChallengeOptions { get; init; }
    /// <summary>004b4860의 현재 범위/제곱 쓰기다.</summary>
    public Action? Range { get; init; }
    /// <summary>0041c8b0의 현재 signed 비용을 조회한다.</summary>
    public Func<int>? Cost { get; init; }
    /// <summary>현재 TYPE+c4에 signed 비용을 변환한 float32를 쓴다.</summary>
    public Action<float>? StoreCost { get; init; }
    /// <summary>0041ca70의 현재 환불 비율을 조회한다.</summary>
    public Func<uint>? CurrentValue { get; init; }
    /// <summary>최종 전투·비미션·저장 관문을 모두 통과할 때 실제 Uber를 재생성한다.</summary>
    public Action? Reconnect { get; init; }
}

/// <summary>RawStateBattleOptions.cpp의004b4900 전체 복사·선택CS·파생값·UI 부모다.</summary>
public static class RawStateBattleOptions
{
    /// <summary>현재 WORD 재조회와 앞방향 DWORD/나머지BYTE 복사를 보존하고 각 효과 뒤 현재 관문을 조회한다.</summary>
    public static void Receive(uint sender, ReadOnlyMemory<byte> packet, StateBattleOptionGlobals globals, StateBattleOptionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        if (Get(packet.Span, 5, 2) > 87) Need(hooks.Assertion)("z->dataLen <= sizeof(globalBattleOptions)");
        uint length = Get(packet.Span, 5, 2); int at = 0;
        // 원본 rep movsd처럼 같은DWORD 전체를 읽고 낮은 주소부터 쓴다. 중첩을 memmove로 바꾸지 않는다.
        for (uint word = length >> 2; word != 0; word--, at += 4) Put(globals.Options.Span, at, Get(packet.Span, 7 + at), 4);
        // 남은0~3바이트도 실제 순서대로 복사하여 물리 오류 전 부분효과를 보존한다.
        for (uint part = length & 3; part != 0; part--, at++) Put(globals.Options.Span, at, Get(packet.Span, 7 + at, 1), 1);
        if (Need(globals.Challenge)() != 0) Need(hooks.ChallengeOptions)(sender, packet);
        Need(hooks.Range)(); int cost = Need(hooks.Cost)(); Need(hooks.StoreCost)((float)cost); globals.Value.Value = Need(hooks.CurrentValue)();
        if (Need(globals.Battle)() && !Need(globals.Mission)() && Need(globals.Saved)()) Need(hooks.Reconnect)();
    }
}
