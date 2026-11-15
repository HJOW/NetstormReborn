// 감시 Form 두 개를 만드는 원문 부모와 실제 장치 계약을 담는다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawDependCreate.h의 현재 생성·링크·가상 부착 장치다. 선택된 위치에서만 장치를 요구한다.</summary>
public sealed class DependCreateHooks
{
    /// <summary>두 부모 모두 서버 영역일 때 읽는 현재 권한이다.</summary>
    public Func<bool>? Server { get; set; }
    /// <summary>각 Create 직전에 따로 읽는 전체 현재 타입 DWORD다.</summary>
    public Func<uint>? FormType { get; set; }
    /// <summary>현재 공통 공장이 반환한 객체 토큰이다.</summary>
    public Func<uint, uint, uint>? Create { get; set; }
    /// <summary>반환한 객체의 원문 SID를 계산하는 장치다.</summary>
    public Func<uint, uint>? Sid { get; set; }
    /// <summary>현재 payload의 지정 WORD만 기록한다.</summary>
    public Action<uint, int, ushort>? Word { get; set; }
    /// <summary>해당 원문 위치에서 현재 가상표 토큰을 읽는다.</summary>
    public Func<uint, uint>? Vtable { get; set; }
    /// <summary>저장한 가상표·전체 부모 DWORD·flags로 가상 부착을 호출한다.</summary>
    public Action<uint, uint, uint, uint>? Attach { get; set; }
    /// <summary>서버 진단이 정상 복귀하면 저장한 flags0으로 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>원문 void 함수의 생성 객체 추적용 결과다. 두 번째 객체가 watched, 첫 번째가 watcher다.</summary>
/// <param name="Watched">감시 대상 부모에 붙일 두 번째 생성 객체다.</param>
/// <param name="Watcher">감시하는 부모에 붙일 첫 번째 생성 객체다.</param>
public readonly record struct DependCreateObjects(uint Watched, uint Watcher);

/// <summary>cpppj/src/o/RawDependCreate.cpp의 PATCH004b1510/CD004af940 부모를 실행한다.</summary>
public static class RawDependCreate
{
    /// <summary>선택한 장치만 요구하고 앞선 할당·링크 쓰기는 유지한다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("Depend 생성의 선택된 실제 장치 미연결");

    /// <summary>두 Create·링크 WORD·저장/현재 가상표 읽기·부착 순서를 판본별로 그대로 실행한다.</summary>
    public static DependCreateObjects Create(OriginalEdition edition, uint watched, uint watcher, DependCreateHooks hooks)
    {
        bool patch = edition == OriginalEdition.Patch1078;
        uint firstServer = patch ? 15000u : 6000u;
        // 예약0~4를 제외한 실제 client 번호 집합만 검사한다.
        bool Client(uint parent) => parent >= 5 && parent < firstServer;
        uint flags = Client(watched) || Client(watcher) ? 2u : 0u;
        if (flags == 0 && !Need(hooks.Server)())
            Need(hooks.Assertion)("server", patch ? @"\Ns\O\Squid.cpp" : @"\ns\o\Squid.cpp", patch ? 0xb29u : 0xae0u);
        uint firstType = Need(hooks.FormType)();
        uint first = Need(hooks.Create)(firstType, flags);
        uint secondType = Need(hooks.FormType)();
        uint second = Need(hooks.Create)(secondType, flags);
        ushort secondSid = unchecked((ushort)Need(hooks.Sid)(second));
        uint firstTable;
        if (patch)
        {
            Need(hooks.Word)(first, 20, secondSid);
            Need(hooks.Word)(first, 18, 0);
            Need(hooks.Word)(second, 18, unchecked((ushort)Need(hooks.Sid)(first)));
            Need(hooks.Word)(second, 20, 0);
            firstTable = Need(hooks.Vtable)(first);
        }
        else
        {
            Need(hooks.Word)(first, 18, 0);
            Need(hooks.Word)(second, 20, 0);
            Need(hooks.Word)(first, 20, secondSid);
            firstTable = Need(hooks.Vtable)(first);
            Need(hooks.Word)(second, 18, unchecked((ushort)Need(hooks.Sid)(first)));
        }
        Need(hooks.Attach)(first, firstTable, watcher, 0);
        uint secondTable = Need(hooks.Vtable)(second);
        Need(hooks.Attach)(second, secondTable, watched, 0);
        return new(second, first);
    }
}
