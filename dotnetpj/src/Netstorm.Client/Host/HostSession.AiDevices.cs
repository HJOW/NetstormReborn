using Netstorm.Core.Original;

// Client 장기 장치가 선택한 동일한 현재 Session의 실제 AI 본체만 빌린다.
namespace Netstorm.Client.Host;

/// <summary>원문 큰 AI 장치를 새 메모리/값형 지갑으로 대신하지 않는 실제 진입점이다.</summary>
public sealed partial class HostSession
{
    /// <summary>같은 실제 AI 메모리와 전체 Runtime의 이번 호출 대여를 함께 반환한다.</summary>
    public AiWalletMemoryAccess BorrowAiWalletMemory() => new(AiBrainMemory, Gameplay.PinCurrentRuntime(this));

    /// <summary>현재 실제 AI 목록 통지 본체로 전달한다. 미설치를 성공이나 관측 로그로 대신하지 않는다.</summary>
    public void NotifyAiObject(uint owner, int sid, bool added) =>
        (_aiObjects ?? throw new InvalidOperationException("실제 AI 객체 통지 미설치")).Notice(owner, sid, added);
}
