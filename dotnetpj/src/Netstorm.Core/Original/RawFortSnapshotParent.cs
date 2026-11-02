namespace Netstorm.Core.Original;

/// <summary>
/// 같은 Template.cpp 진단의 실제 부모 번호를 진단 복귀 뒤 원문 위치에서 계속한다(cpppj <c>o/RawFortSnapshotParent.h</c>의 <c>FortSnapshotParentAssertion</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다.</remarks>
public enum FortSnapshotParentAssertion : uint
{
    /// <summary>004bf9a0 진입에 manager 0이다.</summary>
    MissingFortTemplate = 0x530,

    /// <summary>004bf9a0에 현재 inFortMode 0이다.</summary>
    NotFortMode = 0x531,

    /// <summary>004bf9a0 고정 this·현재 선택 주소 불일치다.</summary>
    FortNotMine = 0x532,

    /// <summary>004bf880 진입에 manager 0이다.</summary>
    MissingBattleTemplate = 0x50e,

    /// <summary>004bf880에 현재 inBattleMode 0이다.</summary>
    NotBattleMode = 0x50f,

    /// <summary>004bf880 고정 this·현재 선택 주소 불일치다.</summary>
    BattleNotMine = 0x510,

    /// <summary>현재 기술 부모를 조회하기 직전에 manager 0이다.</summary>
    MissingTechnologyTemplate = 0x4a6,

    /// <summary>현재 Mission 섹션을 만들기 직전에 manager 0이다.</summary>
    MissingMissionTemplate = 0x1e8,
}

/// <summary>
/// bool·DWORD 소유자의 현재 값을 같은 쓰기 위치에서 빌린다. getter 결과는 쓰기 복제하지 않는다(cpppj <c>o/RawFortSnapshotParent.h</c>의 <c>FortSnapshotParentGlobals</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortSnapshotParentGlobals
{
    /// <summary>00540cac: 현재 Template 선택이다. Player owner 00540c70과 구별한다.</summary>
    public Func<uint>? TemplateOwner { get; set; }

    /// <summary>00594fb8: 부분 진단 뒤 wrapper 첫 번째 분기다.</summary>
    public Func<uint>? Fort { get; set; }

    /// <summary>00594fbc: 부분 진단 뒤 wrapper 두 번째 분기다.</summary>
    public Func<uint>? Battle { get; set; }
}

/// <summary>
/// 부모의 inline Create는 본체 child 진입을 별도로 연결한다. 매 child 분해는 소유하지 않는다(cpppj <c>o/RawFortSnapshotParent.h</c>의 <c>FortSnapshotParentHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortSnapshotParentHooks
{
    /// <summary>고정 this 슬롯의 현재 값 DWORD를 원본 위치에서 읽는다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>전달된 원본 진단이며 복귀 뒤 현재 상태를 계속 읽는다.</summary>
    public Action<FortSnapshotParentAssertion>? Assertion { get; set; }

    /// <summary>004bd6f0: Fort에서는 먼저 실행한다.</summary>
    public Action<int>? TypeNames { get; set; }

    /// <summary>0048fc60: Technology Create 전에 조회한 반환값을 보관한다.</summary>
    public Func<uint>? TechnologyParent { get; set; }

    /// <summary>0044d1b0: 현재 manager로 이름 stream을 만들고 명시 참조를 반환한다.</summary>
    public Func<uint, string, uint>? Create { get; set; }

    /// <summary>004bcea0(parent,stream): Create가 만든 current·기술 부모를 받고 같은 값을 사용한다.</summary>
    public Action<uint, uint>? Technology { get; set; }

    /// <summary>004be430: 같은 this의 실제 Terr 진입이다.</summary>
    public Action<int>? Objects { get; set; }

    /// <summary>004bf6d0: 같은 this의 실제 Territory 진입이다.</summary>
    public Action<int>? Territory { get; set; }

    /// <summary>004befd0: Fort는 Territory 뒤, Battle은 Objects 뒤에 호출한다.</summary>
    public Action<int>? Deck { get; set; }

    /// <summary>004832c0: 부모가 만든 같은 stream이며 실제 공유 난수 writer와 같은 child다.</summary>
    public Action<uint>? Money { get; set; }

    /// <summary>00463ca0(Guide 00562ca0,stream): 실제 현재 미션 업무를 Guide child가 결정한다.</summary>
    public Action<uint>? Mission { get; set; }

    /// <summary>004be7f0: Fort는 Badges 진입이다. Battle은 호출하지 않는다.</summary>
    public Action<int>? Badges { get; set; }

    /// <summary>004beb30: Battle은 조건부 CoreData 진입이다. Badges를 대체하지 않는다.</summary>
    public Action<int>? CoreData { get; set; }

    /// <summary>004bd360: 마지막에 같은 this의 변경 저장 알림 관문이다.</summary>
    public Action<int>? Notify { get; set; }
}

/// <summary>
/// 10.78의 실제 Fort·Battle snapshot 부모의 현재값·분기·조회를 보존한다(cpppj <c>o/RawFortSnapshotParent</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다.</remarks>
public static class RawFortSnapshotParent
{
    /// <summary>원본 this·현재칸의 DWORD 주소 비교에만 쓰는 PE 배열 주소·칸이다.</summary>
    private const uint TemplateBase = 0x005c89f0u;

    /// <summary>원본 this·현재칸의 DWORD 간격이다.</summary>
    private const uint TemplateStride = 20u;

    /// <summary>
    /// 전달된 장치가 빠지면 먼저 원본 효과를 막고 선택 실패로 처리한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">전체 Fort snapshot 부모의 실제 경계 미연결이다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("전체 Fort snapshot 부모 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 원본 LEA와 같은 DWORD 감산 주소로 owner 위치 값이 같은 this 주소로 만든 경우를 보존한다.
    /// </summary>
    /// <param name="slot">Template 칸 번호의 부호 없는 값이다.</param>
    private static uint TemplateAddress(uint slot)
    {
        return unchecked(TemplateBase + slot * TemplateStride);
    }

    /// <summary>
    /// 현재 owner를 같은 슬롯 매핑으로 바꾼다. 실제 20바이트 기록 단위를 가리키지 않으면 물리 접근을 거부한다.
    /// </summary>
    /// <param name="owner">현재 선택 DWORD다.</param>
    private static int CurrentSlot(uint owner)
    {
        uint delta = unchecked(TemplateAddress(owner) - TemplateBase);
        if (delta % TemplateStride != 0)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "현재 Template 주소가 실제 20바이트 기록 경계 밖이다");
        }
        return unchecked((int)(delta / TemplateStride));
    }

    /// <summary>
    /// 현재 기술 부모를 먼저 캡처한 뒤 현재 manager·Create 반환 stream을 바로 보관한다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="hooks">실제 조회·생성 경계다.</param>
    private static void Technology(int slot, FortSnapshotParentHooks hooks)
    {
        if (Need(hooks.Manager)(slot) == 0)
        {
            Need(hooks.Assertion)(FortSnapshotParentAssertion.MissingTechnologyTemplate);
        }
        uint parent = Need(hooks.TechnologyParent)();
        uint manager = Need(hooks.Manager)(slot);
        uint stream = Need(hooks.Create)(manager, "Technology");
        Need(hooks.Technology)(parent, stream);
    }

    /// <summary>
    /// 공통 깊이 분모를 manager에 미리 고정하지 않으며 매번 Create 반환 stream을 같은 child에 전달한다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="hooks">실제 조회·생성 경계다.</param>
    private static void MoneyMission(int slot, FortSnapshotParentHooks hooks)
    {
        uint moneyManager = Need(hooks.Manager)(slot);
        uint moneyStream = Need(hooks.Create)(moneyManager, "Money");
        Need(hooks.Money)(moneyStream);
        if (Need(hooks.Manager)(slot) == 0)
        {
            Need(hooks.Assertion)(FortSnapshotParentAssertion.MissingMissionTemplate);
        }
        uint missionManager = Need(hooks.Manager)(slot);
        uint missionStream = Need(hooks.Create)(missionManager, "Mission");
        Need(hooks.Mission)(missionStream);
    }

    /// <summary>
    /// 004bf9a0: 고정 this를 Fort 진단·TypeNames·기술·Objects·Territory·Deck·Money·Mission·Badges·Notify 순으로 실행한다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="globals">현재 선택·모드 값이다. 호출자가 소유한다.</param>
    /// <param name="hooks">실제 조회·생성 경계다.</param>
    public static void SnapshotFortTemplate(int slot, FortSnapshotParentGlobals globals, FortSnapshotParentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(hooks.Manager)(slot) == 0)
        {
            Need(hooks.Assertion)(FortSnapshotParentAssertion.MissingFortTemplate);
        }
        if (Need(globals.Fort)() == 0)
        {
            Need(hooks.Assertion)(FortSnapshotParentAssertion.NotFortMode);
        }
        if (TemplateAddress(unchecked((uint)slot)) != TemplateAddress(Need(globals.TemplateOwner)()))
        {
            Need(hooks.Assertion)(FortSnapshotParentAssertion.FortNotMine);
        }
        Need(hooks.TypeNames)(slot);
        Technology(slot, hooks);
        Need(hooks.Objects)(slot);
        Need(hooks.Territory)(slot);
        Need(hooks.Deck)(slot);
        MoneyMission(slot, hooks);
        Need(hooks.Badges)(slot);
        Need(hooks.Notify)(slot);
    }

    /// <summary>
    /// 004bf880: 고정 this를 Battle 진단 뒤 Technology·Deck·Objects·Territory·Money·Mission·CoreData·Notify 순으로 실행한다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="globals">현재 선택·모드 값이다. 호출자가 소유한다.</param>
    /// <param name="hooks">실제 조회·생성 경계다.</param>
    public static void SnapshotBattleTemplate(int slot, FortSnapshotParentGlobals globals, FortSnapshotParentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(hooks.Manager)(slot) == 0)
        {
            Need(hooks.Assertion)(FortSnapshotParentAssertion.MissingBattleTemplate);
        }
        if (Need(globals.Battle)() == 0)
        {
            Need(hooks.Assertion)(FortSnapshotParentAssertion.NotBattleMode);
        }
        if (TemplateAddress(unchecked((uint)slot)) != TemplateAddress(Need(globals.TemplateOwner)()))
        {
            Need(hooks.Assertion)(FortSnapshotParentAssertion.BattleNotMine);
        }
        Technology(slot, hooks);
        Need(hooks.Deck)(slot);
        Need(hooks.Objects)(slot);
        Need(hooks.Territory)(slot);
        MoneyMission(slot, hooks);
        Need(hooks.CoreData)(slot);
        Need(hooks.Notify)(slot);
    }

    /// <summary>
    /// 두 번째 if는 첫 번째 전체 Snapshot이 바꾼 현재 모드·owner를 그대로 따른다.
    /// </summary>
    /// <param name="globals">현재 선택·모드 값이다.</param>
    /// <param name="hooks">실제 조회·생성 경계다.</param>
    public static void SnapshotCurrentTemplate(FortSnapshotParentGlobals globals, FortSnapshotParentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(globals.Battle)() != 0)
        {
            int slot = CurrentSlot(Need(globals.TemplateOwner)());
            SnapshotBattleTemplate(slot, globals, hooks);
        }
        if (Need(globals.Fort)() != 0)
        {
            int slot = CurrentSlot(Need(globals.TemplateOwner)());
            SnapshotFortTemplate(slot, globals, hooks);
        }
    }

    /// <summary>
    /// 최초 manager 관문을 반복하지 않으며 Battle 복원 뒤만 owner를 다시 읽어 두 번째 Fort를 사용한다.
    /// </summary>
    /// <param name="globals">현재 선택·모드 값이다.</param>
    /// <param name="hooks">실제 조회·생성 경계다.</param>
    public static void SnapshotCurrentTemplateIfPresent(FortSnapshotParentGlobals globals, FortSnapshotParentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        uint owner = Need(globals.TemplateOwner)();
        int slot = CurrentSlot(owner);
        if (Need(hooks.Manager)(slot) == 0)
        {
            return;
        }
        if (Need(globals.Battle)() != 0)
        {
            SnapshotBattleTemplate(slot, globals, hooks);
            owner = Need(globals.TemplateOwner)();
        }
        if (Need(globals.Fort)() != 0)
        {
            SnapshotFortTemplate(CurrentSlot(owner), globals, hooks);
        }
    }
}
