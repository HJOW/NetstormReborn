// 원문 Create 부모의 현재 진단·할당·생성자 반환 객체·저장 VT 호출 순서를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>객체는 할당 SID와 다른 전체 DWORD 토큰일 수 있으며 실제 장치가 해석한다.</summary>
public sealed class RawSquidCreationHooks
{
    /// <summary>PATCH 진입의 현재 fakeSurface TYPE DWORD다.</summary>
    public Func<uint>? FakeSurfaceType { get; set; }
    /// <summary>도달한 원문 진단이 정상 복귀하면 다음 명령으로 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>전체 flags를 같은 실제 풀의 할당에 전달한다.</summary>
    public Func<uint, uint>? Allocate { get; set; }
    /// <summary>할당/진단 뒤 현재 전체 TYPE의 생성자 주소를 읽는다.</summary>
    public Func<uint, uint>? Constructor { get; set; }
    /// <summary>주소와 SID를 받아 실제 반환 객체를 돌려준다. 주소0은 base 생성자다.</summary>
    public Func<uint, uint, uint>? Construct { get; set; }
    /// <summary>반환 객체의 현재 VT DWORD를 TYPE 쓰기 전에 보관한다.</summary>
    public Func<uint, uint>? Vtable { get; set; }
    /// <summary>반환 객체에 원문 전체 TYPE 인자의 하위 BYTE만 쓴다.</summary>
    public Action<uint, byte>? StoreType { get; set; }
    /// <summary>저장한 VT+28을 현재 반환 객체에 실행한다.</summary>
    public Action<uint, uint>? PostCreate { get; set; }
}

/// <summary>004af530/CD004ab390의 부모와 signed SID 진단만 처리한다.</summary>
public static class RawSquidCreation
{
    /// <summary>선택한 장치를 호출 끝까지 보유하고 미연결이면 해당 소비 위치에서 실패한다.</summary>
    private static T Need<T>(T? body) where T : class => body ?? throw new InvalidOperationException("Create의 선택된 실제 장치 미연결");

    /// <summary>선행 효과를 취소하지 않고 할당 뒤 현재 생성자와 실제 반환 객체를 선택한다.</summary>
    public static uint Create(OriginalEdition edition, uint type, uint flags, RawSquidCreationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        bool patch = edition == OriginalEdition.Patch1078;
        // 선택된 진단만 현재 함수 객체의 수명으로 호출한다.
        void Diagnose(string expression, uint line) => Need(hooks.Assertion)(expression, patch ? @"\Ns\O\Squid.cpp" : @"\ns\o\Squid.cpp", line);
        if (patch && type == Need(hooks.FakeSurfaceType)()) Diagnose("type != fakeThreeByThreeSurfaceType", 0x189);
        uint sid = Need(hooks.Allocate)(flags); int signed = unchecked((int)sid), predictable = patch ? 23001 : 14000;
        if ((flags & 1) != 0)
        {
            if (signed < predictable) Diagnose("sid >= FIRST_PREDICTABLE_SID", patch ? 0x191u : 0x188u);
        }
        else if (signed >= predictable) Diagnose("sid < FIRST_PREDICTABLE_SID", patch ? 0x194u : 0x18bu);
        uint address = Need(hooks.Constructor)(type), returned = Need(hooks.Construct)(address, sid);
        uint table = Need(hooks.Vtable)(returned);
        Need(hooks.StoreType)(returned, unchecked((byte)type)); Need(hooks.PostCreate)(returned, table);
        return returned;
    }
}
